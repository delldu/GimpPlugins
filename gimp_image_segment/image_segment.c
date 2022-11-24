/************************************************************************************
***
*** Copyright 2020-2022 Dell(18588220928@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "gimp_image_segment"

static void query(void);
static void run(const gchar * name,
				gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals);

static int save_segment_result(IMAGE * send_image, IMAGE * recv_image)
{
	gint32 image_id;
	int ret = RET_OK;

	check_image(send_image);
	check_image(recv_image);

	if (send_image->height != recv_image->height || send_image->width != recv_image->width) {
		syslog_error("Recv image size does not match send image.");
		return RET_ERROR;
	}

	image_id = gimp_image_new(send_image->width, send_image->height, GIMP_RGB);
	if (image_id < 0) {
		syslog_error("Call gimp_image_new().");
		return RET_ERROR;
	}

	ret = image_saveas_layer(send_image, "sources", image_id, 100.0);
	if (ret == RET_OK) {
		ret = image_saveas_layer(recv_image, "segment", image_id, 50.0);
	}

	if (ret == RET_OK) {
		gimp_display_new(image_id);
		gimp_displays_flush();
	} else {
		syslog_error("Call image_saveas_layer().");
	}

	return ret;
}


GimpPlugInInfo PLUG_IN_INFO = {
	NULL,
	NULL,
	query,
	run
};

MAIN()

static void query(void)
{
	static GimpParamDef args[] = {
		{GIMP_PDB_INT32, "run-mode", "Run mode"},
		{GIMP_PDB_IMAGE, "image", "Input image"},
		{GIMP_PDB_DRAWABLE, "drawable", "Input drawable"},
	};

	gimp_install_procedure(PLUG_IN_PROC,
						   _("Segment"),
						   _("Segment"),
						   "Dell Du <18588220928@163.com>",
						   "Dell Du",
						   "2020-2022",
						   _("Segment"), "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/AI/Matte and Segment/");
}

static GimpPDBStatusType start_image_segment(gint32 drawable_id)
{
	gint channels;
	GeglRectangle rect;
	IMAGE *send_image, *recv_image;
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;

	gimp_progress_init("Segment ...");
	recv_image = NULL;
	send_image = image_from_drawable(drawable_id, &channels, &rect);
	if (image_valid(send_image)) {
		recv_image = normal_service("image_segment", send_image, NULL);
	} else {
		status = GIMP_PDB_EXECUTION_ERROR;
		g_message("Source error, try menu 'Image->Precision->8 bit integer'.\n");
	}
	gimp_progress_update(1.0);

	if (status != GIMP_PDB_SUCCESS)
		return status;
	
	if (image_valid(recv_image)) {
		save_segment_result(send_image, recv_image);
		image_destroy(recv_image);
	} else {
		status = GIMP_PDB_EXECUTION_ERROR;
		g_message("Segment service not available.");
	}
	if (image_valid(send_image)) {
		image_destroy(send_image);
	}

	return status;				// GIMP_PDB_SUCCESS;
}

static void
run(const gchar * name, gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals)
{
	static GimpParam values[1];
	GimpRunMode run_mode;
	// gint32 image_id;
	gint32 drawable_id;
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;

	// INIT_I18N();

	/* Setting mandatory output values */
	*nreturn_vals = 1;
	*return_vals = values;
	values[0].type = GIMP_PDB_STATUS;
	values[0].data.d_status = status;

	if (strcmp(name, PLUG_IN_PROC) != 0 || nparams < 3) {
		values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
		return;
	}

	run_mode = (GimpRunMode) param[0].data.d_int32;
	// image_id = param[1].data.d_image;
	drawable_id = param[2].data.d_drawable;

	image_ai_cache_init();
	// gimp_image_convert_precision(image_id, GIMP_COMPONENT_TYPE_U8);

	status = start_image_segment(drawable_id);
	if (run_mode != GIMP_RUN_NONINTERACTIVE)
		gimp_displays_flush();

	image_ai_cache_exit();
}
