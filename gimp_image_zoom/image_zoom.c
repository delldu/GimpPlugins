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
		{ GIMP_PDB_INT32, "run-mode", "Run mode" },
		{ GIMP_PDB_IMAGE, "image", "Input image" },
		{ GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
	    { GIMP_PDB_INT32, "method", "Zoom method (1, 2)" },
	};

	gimp_install_procedure(PLUG_IN_PROC,
						   "Image Zoom",
						   "This plug-in zoom image with PAI",
						   "Dell Du <18588220928@163.com>",
						   "Copyright Dell Du <18588220928@163.com>",
						   "2020-2021", "_Zoom", "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/Filters/PAI");
}

static IMAGE *zoom_service(IMAGE *send_image, int msgcode)
{
	if (msgcode == IMAGE_ZOOM_SERVICE_WITH_PAN)
		return normal_service("IMAGE_ZOOM_SERVICE_WITH_PAN", send_image, NULL);

	return normal_service("image_zoom", send_image, NULL);
}

static GimpPDBStatusType do_image_zoomx(GimpDrawable * drawable)
{
	int x, y, height, width;
	IMAGE *send_image, *recv_image;
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;

	// Support local zoom4x
	x = y = 0;
	if (! gimp_drawable_mask_intersect(drawable->drawable_id, &x, &y, &width, &height)) {
		height = drawable->height;
		width = drawable->width;
	}
	if (width < 4 || height < 4) {
		g_message("Select region is too small.\n");
		return GIMP_PDB_EXECUTION_ERROR;
	}

	send_image = image_fromgimp(drawable, x, y, width, height);
	if (image_valid(send_image)) {
		recv_image = zoom_service(send_image, zoom_options.method);

		if (image_valid(recv_image)) {
			image_display(recv_image, "zoom");
			image_destroy(recv_image);
		}
		else {
			status = GIMP_PDB_EXECUTION_ERROR;
			g_message("Error: Zoom service is not available.");
		}
		image_destroy(send_image);
	} else {
		status = GIMP_PDB_EXECUTION_ERROR;
		g_message("Error: Zoom source(drawable channel is not 1-4 ?).\n");
	}

 	return status; // GIMP_PDB_SUCCESS;
}

static void
run(const gchar * name, gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals)
{
	static GimpParam values[1];
	GimpRunMode run_mode;
	GimpDrawable *drawable;
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

	run_mode = (GimpRunMode)param[0].data.d_int32;
	drawable_id = param[2].data.d_drawable;

	switch (run_mode) {
	case GIMP_RUN_INTERACTIVE:
		gimp_get_data(PLUG_IN_PROC, &zoom_options);
		if (! zoom_dialog())
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

	drawable = gimp_drawable_get(drawable_id);
	if (gimp_drawable_is_rgb(drawable_id) || gimp_drawable_is_gray(drawable_id)) {
		gimp_progress_init("Zoom ...");

		status = do_image_zoomx(drawable);

		if (run_mode != GIMP_RUN_NONINTERACTIVE)
			gimp_displays_flush();

		/*  Store data  */
		if (run_mode == GIMP_RUN_INTERACTIVE)
			gimp_set_data(PLUG_IN_PROC, &zoom_options, sizeof(ZoomOptions));
	} else {
		status = GIMP_PDB_EXECUTION_ERROR;
		g_message("Cannot zoom on indexed color images.");
	}
	gimp_drawable_detach(drawable);

	// Output result for pdb
	values[0].data.d_status = status;
}

