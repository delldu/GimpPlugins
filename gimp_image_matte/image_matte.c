/************************************************************************************
***
*** Copyright 2020-2021 Dell(18588220928g@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "plug-in-gimp_image_matting"

static void query(void);
static void run(const gchar * name,
				gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals);


IMAGE *matte_service(IMAGE *send_image)
{
	return normal_service("image_matte", send_image, NULL);
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
						   "Image Matte",
						   "This plug-in matte image with PAI",
						   "Dell Du <18588220928@163.com>",
						   "Copyright Dell Du <18588220928@163.com>",
						   "2020-2021", "_Matte", "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/Filters/PAI");
}

static void
run(const gchar * name, gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals)
{
	int x, y, height, width;
	IMAGE *send_image, *recv_image;

	static GimpParam values[1];
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;
	// GimpRunMode run_mode;
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

	// run_mode = (GimpRunMode)param[0].data.d_int32;

	// image_id = param[1].data.d_int32;
	drawable_id = param[2].data.d_drawable;

	gimp_layer_add_alpha(drawable_id);

	drawable = gimp_drawable_get(drawable_id);
	if (drawable->bpp < 4) {
		g_message("Image format is not RGBA.");
		values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
		return;
	}

	// Support local matting ...
	x = y = 0;
	if (! gimp_drawable_mask_intersect(drawable_id, &x, &y, &width, &height) || height * width < 64) {
		height = drawable->height;
		width = drawable->width;
	}

	send_image = image_fromgimp(drawable, x, y, width, height);
	if (image_valid(send_image)) {
		gimp_progress_init("Matting...");

		recv_image = matte_service(send_image);

		if (image_valid(recv_image)) {
			image_display(recv_image, "matte");
			image_destroy(recv_image);
		}

		image_destroy(send_image);
		gimp_progress_update(1.0);
	} else {
		g_message("Error: Matting image error.");
		status = GIMP_PDB_EXECUTION_ERROR;
	}

	// Update modified region
	gimp_drawable_flush(drawable);
	// gimp_drawable_merge_shadow(drawable_id, TRUE);
	gimp_drawable_update(drawable_id, x, y, width, height);

	// Flush all ?
	gimp_displays_flush();
	gimp_drawable_detach(drawable);

	// Output result for pdb
	values[0].data.d_status = status;
}
