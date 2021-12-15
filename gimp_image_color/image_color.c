/************************************************************************************
***
*** Copyright 2020-2021 Dell(18588220928g@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "plug-in-gimp_image_color"

static void query(void);
static void run(const gchar * name,
				gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals);


static IMAGE *color_service(IMAGE *send_image)
{
	return normal_service("image_color", send_image, NULL);
}

static GimpPDBStatusType image_color(GimpDrawable * drawable)
{
	int x, y, height, width;
	IMAGE *send_image, *recv_image;
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;

	// Support local color
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
		recv_image = color_service(send_image);

		if (image_valid(recv_image)) {
			image_togimp(recv_image, drawable, x, y, width, height);
			image_destroy(recv_image);

			gimp_progress_update(1.0);
			/*  merge the shadow, update the drawable  */
			gimp_drawable_flush(drawable);
			gimp_drawable_merge_shadow(drawable->drawable_id, TRUE);
			gimp_drawable_update(drawable->drawable_id, x, y, width, height);
		} else {
			status = GIMP_PDB_EXECUTION_ERROR;
			g_message("Error: Color service is not avaible.");
		}
		image_destroy(send_image);
		gimp_progress_update(1.0);
	} else {
		status = GIMP_PDB_EXECUTION_ERROR;
		g_message("Error: Color source(drawable channel is not 1-4 ?).\n");
	}

 	return status;
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
		{ GIMP_PDB_INT32, "run-mode", "Run mode" },
		{ GIMP_PDB_IMAGE, "image", "Input image" },
		{ GIMP_PDB_DRAWABLE, "drawable", "Input drawable" }
	};

	gimp_install_procedure(PLUG_IN_PROC,
						   "Image Color",
						   "This plug-in color image with PAI",
						   "Dell Du <18588220928@163.com>",
						   "Copyright Dell Du <18588220928@163.com>",
						   "2020-2021", "_Color", "RGB*, GRAY*", 
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
	gint32 drawable_id;

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

	drawable = gimp_drawable_get(drawable_id);
	if (gimp_drawable_is_rgb(drawable_id) || gimp_drawable_is_gray(drawable_id)) {
		gimp_progress_init("Color ...");

		status = image_color(drawable);

		if (run_mode != GIMP_RUN_NONINTERACTIVE)
			gimp_displays_flush();
	} else {
		status = GIMP_PDB_EXECUTION_ERROR;
		g_message("Cannot color on indexed color images.");
	}
	gimp_drawable_detach(drawable);

	// Output result for pdb
	values[0].data.d_status = status;
}
