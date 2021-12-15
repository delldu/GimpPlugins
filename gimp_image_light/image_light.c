/************************************************************************************
***
*** Copyright 2020-2021 Dell(18588220928g@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "plug-in-gimp_image_light"

#include "light_dialog.c"

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

static IMAGE *light_service(IMAGE *send_image, gint32 msgcode)
{
	if (msgcode == IMAGE_LIGHT_SERVICE_WITH_CLAHE)
		return normal_service("image_light_with_clahe", send_image, NULL);
	// else
	return normal_service("image_light", send_image, NULL);
}

static GimpPDBStatusType do_image_light(GimpDrawable * drawable)
{
	int x, y, height, width;
	IMAGE *send_image, *recv_image;
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;

	// Support local lighting
	x = y = 0;
	if (! gimp_drawable_mask_intersect(drawable->drawable_id, &x, &y, &width, &height) || height * width < 64) {
		height = drawable->height;
		width = drawable->width;
	}
	if (width < 4 || height < 4) {
		g_message("Select region is too small.\n");
		return GIMP_PDB_EXECUTION_ERROR;
	}

	send_image = image_fromgimp(drawable, x, y, width, height);
	if (image_valid(send_image)) {
		recv_image = light_service(send_image, light_options.method);

		if (image_valid(recv_image)) {
			image_togimp(recv_image, drawable, x, y, width, height);
			image_destroy(recv_image);

			gimp_progress_update(1.0);

			/*  merge the shadow, update the drawable  */
			gimp_drawable_flush(drawable);
			gimp_drawable_merge_shadow(drawable->drawable_id, TRUE);
			gimp_drawable_update(drawable->drawable_id, x, y, width, height);
		}
		else {
			status = GIMP_PDB_EXECUTION_ERROR;
			g_message("Error: Light service is not avaible.");
		}
		image_destroy(send_image);
	} else {
		status = GIMP_PDB_EXECUTION_ERROR;
		g_message("Error: Light source(drawable channel is not 1-4 ?).\n");
	}

 	return status;
}


static void query(void)
{
	static GimpParamDef args[] = {
		{ GIMP_PDB_INT32, "run-mode", "Run mode" },
		{ GIMP_PDB_IMAGE, "image", "Input image" },
		{ GIMP_PDB_DRAWABLE, "drawable", "Input drawable" }
	};

	gimp_install_procedure(PLUG_IN_PROC,
						   "Image Light",
						   "This plug-in light image with PAI",
						   "Dell Du <18588220928@163.com>",
						   "Copyright Dell Du <18588220928@163.com>",
						   "2020-2021", "_Light", "RGB*, GRAY*", 
						   GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/Filters/PAI");
}

static void
run(const gchar * name, gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals)
{
	static GimpParam values[1];
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;
	GimpRunMode run_mode;
	GimpDrawable *drawable;
	// gint32 image_id;
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
	// image_id = param[1].data.d_drawable;
	drawable_id = param[2].data.d_drawable;

	switch (run_mode) {
	case GIMP_RUN_INTERACTIVE:
		/* Get options last values if needed */
		gimp_get_data(PLUG_IN_PROC, &light_options);
		if (! light_dialog())
			return;
		break;

	case GIMP_RUN_NONINTERACTIVE:
		if (nparams != 4)
			status = GIMP_PDB_CALLING_ERROR;
		if (status == GIMP_PDB_SUCCESS) {
			light_options.method = param[2].data.d_int32;
		}
		break;

	case GIMP_RUN_WITH_LAST_VALS:
		/*  Get options last values if needed  */
		gimp_get_data(PLUG_IN_PROC, &light_options);
		break;

	default:
		break;
	}

	drawable = gimp_drawable_get(drawable_id);
	/*  Make sure that the drawable is RGB or GRAY color  */
	if (gimp_drawable_is_rgb(drawable_id) || gimp_drawable_is_gray(drawable_id)) {
		gimp_progress_init("Light ...");

		status = do_image_light(drawable);

		if (run_mode != GIMP_RUN_NONINTERACTIVE)
			gimp_displays_flush();

		/*  Store data  */
		if (run_mode == GIMP_RUN_INTERACTIVE)
			gimp_set_data(PLUG_IN_PROC, &light_options, sizeof(LightOptions));
	} else {
		status = GIMP_PDB_EXECUTION_ERROR;
		g_message("Cannot light on indexed color images.");
	}
	gimp_drawable_detach(drawable);

	// Output result for pdb
	values[0].data.d_status = status;
}
