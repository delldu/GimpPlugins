/************************************************************************************
***
*** Copyright 2020 Dell(18588220928g@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "plug-in-gimp_patch"

static void query(void);
static void run(const gchar * name,
				gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals);
static void init_proc(void);
static void quit_proc(void);

static void patch(GimpDrawable * drawable)
{
	gint x, y, width, height;
	IMAGE *image;
	// gboolean has_alpha;

	if (!gimp_drawable_mask_intersect(drawable->drawable_id, &x, &y, &width, &height) || width < 1 || height < 1) {
		g_print("Drawable region is empty.\n");
		return;
	}
	// has_alpha = gimp_drawable_has_alpha (drawable->drawable_id);

	image = image_fromgimp(drawable, x, y, width, height);
	if (image_valid(image)) {
		gimp_progress_update(0.1);

		color_togray(image);

		gimp_progress_update(0.8);
		image_togimp(image, drawable, x, y, width, height);

		image_destroy(image);
		gimp_progress_update(1.0);
	}
	// Update region
	gimp_drawable_flush(drawable);
	gimp_drawable_merge_shadow(drawable->drawable_id, TRUE);
	gimp_drawable_update(drawable->drawable_id, x, y, width, height);
}


GimpPlugInInfo PLUG_IN_INFO = {
  init_proc,
  quit_proc,
	query,
	run
};

MAIN()

static void init_proc(void)
{
  g_print("Init patch ...\n");
}

static void quit_proc(void)
{
  g_print("Exit patch ...\n");
}

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
						   "Image Patch with Deep Learning",
						   "This plug-in patch image with deep learning technology",
						   "Dell Du <18588220928@163.com>",
						   "Copyright Dell Du <18588220928@163.com>",
						   "2020", "_Patch", "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/Filters/AI");
}

static void
run(const gchar * name, gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals)
{
	static GimpParam values[1];
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;
	GimpRunMode run_mode;
	GimpDrawable *drawable;

	/* Setting mandatory output values */
	*nreturn_vals = 1;
	*return_vals = values;
	values[0].type = GIMP_PDB_STATUS;

	if (strcmp(name, PLUG_IN_PROC) != 0 || nparams < 3) {
		values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
		return;
	}

	values[0].data.d_status = status;

	run_mode = param[0].data.d_int32;
	drawable = gimp_drawable_get(param[2].data.d_drawable);

	if (gimp_drawable_is_rgb(drawable->drawable_id) || gimp_drawable_is_gray(drawable->drawable_id)) {
		gimp_progress_init("Patch...");

		GTimer *timer;
		timer = g_timer_new();

		patch(drawable);

		g_print("image patch took %g seconds.\n", g_timer_elapsed(timer, NULL));
		g_timer_destroy(timer);

		if (run_mode != GIMP_RUN_NONINTERACTIVE)
			gimp_displays_flush();
	} else {
		status = GIMP_PDB_EXECUTION_ERROR;
	}
	values[0].data.d_status = status;

	gimp_drawable_detach(drawable);
}
