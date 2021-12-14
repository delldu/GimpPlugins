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
		{
		 GIMP_PDB_INT32,
		 "run-mode",
		 "Run mode"},
		{
		 GIMP_PDB_IMAGE,
		 "image",
		 "Input image"},
		{
		 GIMP_PDB_DRAWABLE,
		 "drawable",
		 "Input drawable"}
	};

	gimp_install_procedure(PLUG_IN_PROC,
						   "Image Zoom with Deep Learning",
						   "This plug-in zoom image with deep learning technology",
						   "Dell Du <18588220928@163.com>",
						   "Copyright Dell Du <18588220928@163.com>",
						   "2020-2021", "_Zoom4x", "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/Filters/PAI");
}

// remote_procee_call
IMAGE *zoom_service(IMAGE *send_image, int msgcode)
{
	if (msgcode == IMAGE_ZOOM_SERVICE_WITH_PAN)
		return normal_service("image_zoom_with_pan", send_image, NULL);

	return normal_service("image_zoom", send_image, NULL);
}

static void
run(const gchar * name, gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals)
{
	int x, y, height, width;
	IMAGE *send_image, *recv_image;

	static GimpParam values[1];
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;
	GimpRunMode run_mode;
	GimpDrawable *drawable;
	gint32 drawable_id;

	/* Setting mandatory output values */
	*nreturn_vals = 1;
	*return_vals = values;
	values[0].type = GIMP_PDB_STATUS;

	if (strcmp(name, PLUG_IN_PROC) != 0 || nparams < 3) {
		values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
		return;
	}
	values[0].data.d_status = status;

	run_mode = (GimpRunMode)param[0].data.d_int32;
	drawable_id = param[2].data.d_drawable;
	drawable = gimp_drawable_get(drawable_id);

	switch (run_mode) {
	case GIMP_RUN_INTERACTIVE:
		/* Get options last values if needed */
		gimp_get_data(PLUG_IN_PROC, &zoom_options);
		if (! zoom_dialog())
			return;
		break;

	case GIMP_RUN_NONINTERACTIVE:
		if (nparams != 4)
			status = GIMP_PDB_CALLING_ERROR;
		if (status == GIMP_PDB_SUCCESS) {
			zoom_options.method = param[2].data.d_int32;
		}
		break;

	case GIMP_RUN_WITH_LAST_VALS:
		/*  Get options last values if needed  */
		gimp_get_data(PLUG_IN_PROC, &zoom_options);
		break;

	default:
		break;
	}

	// Support local zoom4x
	x = y = 0;
	if (! gimp_drawable_mask_intersect(drawable_id, &x, &y, &width, &height) || height * width < 64) {
		height = drawable->height;
		width = drawable->width;
	}

	send_image = image_fromgimp(drawable, x, y, width, height);
	gimp_drawable_detach(drawable);

	if (image_valid(send_image)) {
		gimp_progress_init("Zoomin ...");

		recv_image = zoom_service(send_image, zoom_options.method);

		if (image_valid(recv_image)) {
			image_display(recv_image, "zoom4x");
			image_destroy(recv_image);
		}
		else {
			g_message("Error: Zoom remote service is not available.");
		}
		image_destroy(send_image);
		gimp_progress_update(1.0);
	} else {
		g_message("Error: Zoom image is not valid (NO RGB).");
		status = GIMP_PDB_EXECUTION_ERROR;
	}

	// Flush all ?
	gimp_displays_flush();

	// Output result for pdb
	values[0].data.d_status = status;
}

