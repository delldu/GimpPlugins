/************************************************************************************
***
*** Copyright 2020-2022 Dell(18588220928@163.com), All Rights Reserved.
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
		{GIMP_PDB_INT32, "method", "Zoom in method (1, 2)"},
	};

	gimp_install_procedure(PLUG_IN_PROC,
						   "Image Zoom In",
						   "This plug-in zoom image with AI",
						   "Dell Du <18588220928@163.com>",
						   "Copyright Dell Du <18588220928@163.com>",
						   "2020-2022", "_Zoom In", "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/Filters/AI/1. Basic");
}

static IMAGE *zoom_rpc_service(IMAGE * send_image, int msgcode)
{
	char addon[64];
	if (msgcode == IMAGE_ZOOMS_SERVICE)
		return normal_service(PAI_TASKSET, "image_zooms", send_image, NULL);

	if (msgcode == IMAGE_ZOOMX_SERVICE) {
		snprintf(addon, sizeof(addon), "zoom_times=%d", zoom_options.times);
		return normal_service(TAI_TASKSET, "image_zoomx", send_image, addon);
	}

	return normal_service(PAI_TASKSET, "image_zoom", send_image, NULL);
}

static GimpPDBStatusType start_image_zoom(gint32 drawable_id)
{
	IMAGE *send_image, *recv_image;
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;
	gint x, y, width, height;

	if (!gimp_drawable_mask_intersect(drawable_id, &x, &y, &width, &height) || width < 8 || height < 8) {
		g_message("Error: Select or region size is too small.\n");
		return GIMP_PDB_EXECUTION_ERROR;
	}

	gimp_progress_init("Zoom ...");
	send_image = image_from_select(drawable_id, x, y, width, height);
	if (image_valid(send_image)) {
		recv_image = zoom_rpc_service(send_image, zoom_options.method);
		gimp_progress_update(1.0);
		if (image_valid(recv_image)) {
			create_gimp_image(recv_image, "zoom");
			image_destroy(recv_image);
		} else {
			status = GIMP_PDB_EXECUTION_ERROR;
			g_message("Error: Zoom service is not available.");
		}
		image_destroy(send_image);
	} else {
		status = GIMP_PDB_EXECUTION_ERROR;
		g_message("Error: Zoom source (drawable channel is not 1-4 ?).\n");
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

	status = start_image_zoom(drawable_id);
	if (run_mode != GIMP_RUN_NONINTERACTIVE)
		gimp_displays_flush();
	/*  Store data  */
	if (run_mode == GIMP_RUN_INTERACTIVE)
		gimp_set_data(PLUG_IN_PROC, &zoom_options, sizeof(ZoomOptions));

	gegl_exit();
}
