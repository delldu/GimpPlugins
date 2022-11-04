/************************************************************************************
***
*** Copyright 2020-2022 Dell(18588220928@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "gimp_image_selection"


static void query(void);
static void run(const gchar * name,
				gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals);

static int save_selection_result(IMAGE * send_image, IMAGE * recv_image)
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

	ret = image_saveas_layer(send_image, "sources", image_id);
	if (ret == RET_OK) {
		ret = image_saveas_layer(recv_image, "selection", image_id);
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
						   _("Select By Object"),
						   _("Select By Object"),
						   "Dell Du <18588220928@163.com>",
						   "Dell Du",
						   "2020-2022", 
						   _("By Object"),
						   "RGB*, GRAY*", 
						   GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/Select");
}

static IMAGE *selection_rpc_service(int id, IMAGE * send_image)
{
	return normal_service("image_selection", id, send_image, NULL);
}

static GimpPDBStatusType start_image_selection(gint32 drawable_id)
{
	gint channels;
	GeglRectangle rect;	
	IMAGE *send_image, *recv_image;
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;
	char output_file[512];

	send_image = NULL;
	recv_image = NULL;

	get_cache_filename("output", drawable_id, ".png", sizeof(output_file), output_file);
	// Get result if cache file exists !!!
	if (file_exist(output_file)) {
		recv_image = image_load(output_file);
	} else {
		gimp_progress_init("Select ...");
		send_image = image_from_drawable(drawable_id, &channels, &rect);
		if (image_valid(send_image)) {
			recv_image = selection_rpc_service(drawable_id, send_image);
			gimp_progress_update(1.0);
			image_destroy(send_image);
		} else {
			status = GIMP_PDB_EXECUTION_ERROR;
			g_message("Error: Segment source.\n");
		}
	}

	if (image_valid(recv_image)) {
		save_selection_result(send_image, recv_image);
		image_destroy(recv_image);
	} else {
		status = GIMP_PDB_EXECUTION_ERROR;
		g_message("Error: Segment service is not available.");
	}

	return status;				// GIMP_PDB_SUCCESS;
}

static void
run(const gchar * name, gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals)
{
	static GimpParam values[1];
	GimpRunMode run_mode;
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
	drawable_id = param[2].data.d_drawable;

	gegl_init(NULL, NULL);

	status = start_image_selection(drawable_id);
	if (run_mode != GIMP_RUN_NONINTERACTIVE)
		gimp_displays_flush();

	gegl_exit();
}
