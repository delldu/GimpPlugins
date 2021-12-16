/************************************************************************************
***
*** Copyright 2020-2021 Dell(18588220928g@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "plug-in-gimp_image_zoom"

#include "zoom_dialog.c"


static void query(void);
static void run(const gchar * name,
				gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals);


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
		{GIMP_PDB_INT32, "method", "Zoom method (1, 2)"},
	};

	gimp_install_procedure(PLUG_IN_PROC,
						   "Image Zoom",
						   "This plug-in zoom image with PAI",
						   "Dell Du <18588220928@163.com>",
						   "Copyright Dell Du <18588220928@163.com>",
						   "2020-2021", "_Zoom", "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/Filters/PAI");
}

static IMAGE *zoom_rpc_service(IMAGE * send_image, int msgcode)
{
	if (msgcode == IMAGE_ZOOM_SERVICE_WITH_PAN)
		return normal_service("IMAGE_ZOOM_SERVICE_WITH_PAN", send_image, NULL);

	return normal_service("image_zoom", send_image, NULL);
}

static GimpPDBStatusType start_image_zoomx(gint drawable_id)
{
	gint channels;
	GeglRectangle rect;
	IMAGE *send_image, *recv_image;
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;

	gimp_progress_init("Zoom ...");
	send_image = image_from_drawable(drawable_id, &channels, &rect);
	if (image_valid(send_image)) {
		recv_image = zoom_rpc_service(send_image, zoom_options.method);
		if (image_valid(recv_image)) {
			image_display(recv_image, "zoom");
			image_destroy(recv_image);
		} else {
			status = GIMP_PDB_EXECUTION_ERROR;
			g_message("Error: Zoom service is not available.");
		}
		image_destroy(send_image);
	} else {
		status = GIMP_PDB_EXECUTION_ERROR;
		g_message("Error: Zoom source(drawable channel is not 1-4 ?).\n");
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

	switch (run_mode) {
	case GIMP_RUN_INTERACTIVE:
		gimp_get_data(PLUG_IN_PROC, &zoom_options);
		if (!zoom_dialog())
			return;
		break;

	case GIMP_RUN_NONINTERACTIVE:
		zoom_options.method = param[3].data.d_int32;
		break;

	case GIMP_RUN_WITH_LAST_VALS:
		gimp_get_data(PLUG_IN_PROC, &zoom_options);
		break;

	default:
		break;
	}

	gegl_init(NULL, NULL);

	status = start_image_zoomx(drawable_id);
	if (run_mode != GIMP_RUN_NONINTERACTIVE)
		gimp_displays_flush();
	/*  Store data  */
	if (run_mode == GIMP_RUN_INTERACTIVE)
		gimp_set_data(PLUG_IN_PROC, &zoom_options, sizeof(ZoomOptions));

	gegl_exit();
}
