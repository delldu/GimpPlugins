/************************************************************************************
***
*** Copyright 2020-2021 Dell(18588220928g@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "plug-in-gimp_image_color"
#define URL "ipc:///tmp/image_color.ipc"

static void query(void);
static void run(const gchar * name,
				gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals);

// IMAGE *get_color_source(int image_id) 
// IMAGE *get_color_target(char *url, IMAGE *input);
// display_color_target(IMAGE *target);

IMAGE *get_color_source(int image_id)
{
	int i, j, n_layers;
	IMAGE *layers[2], *gray_image, *color_image;

	n_layers = image_layers(image_id, ARRAY_SIZE(layers), layers);
	if (n_layers != 2) {
		syslog_error("Color image must have 2 layes, one for gray texture, other for color.");
		goto failure;
	}

	color_image = layers[0];
	gray_image = layers[1];
	// image_save(gray_image, "gray.jpg");
	// image_save(color_image, "color.png");

	if (gray_image->height != color_image->height || gray_image->width != color_image->width) {
		syslog_error("The size of gray and color layers is not same.");
		goto failure;
	}

	// PASS
	image_foreach(color_image, i, j) {
		if (color_image->ie[i][j].a < 128) {
			gray_image->ie[i][j].r = color_image->ie[i][j].r;
			gray_image->ie[i][j].g = color_image->ie[i][j].g;
			gray_image->ie[i][j].b = color_image->ie[i][j].b;
			gray_image->ie[i][j].a = 0;
		}
	}
	image_destroy(color_image);

	return gray_image;

failure:
	for (i = 0; i < n_layers; i++)
		image_destroy(layers[i]);

	return NULL;
}

int display_color_target(IMAGE *target)
{
	gchar name[64];
	gint32 image_ID, layer_ID;
	GimpDrawable *drawable;

	check_image(target);

	image_ID = gimp_image_new(target->width, target->height, GIMP_RGB);
	if (image_ID < 0) {
		syslog_error("Create gimp image.");
		return RET_ERROR;
	}

	g_snprintf(name, sizeof(name), "color_%d", image_ID);
	layer_ID = gimp_layer_new(image_ID, name, target->width, target->height, GIMP_RGBA_IMAGE, 50.0, 
		GIMP_NORMAL_MODE);

	if (layer_ID > 0) {
		drawable = gimp_drawable_get((gint32)layer_ID);
		image_togimp(target, drawable,  0, 0, target->width, target->height);
		if (! gimp_image_insert_layer(image_ID, layer_ID, 0, 0)) {
			syslog_error("Insert layer error.");
		}
		gimp_display_new(image_ID);
		gimp_displays_flush();

		// image_save(target, "s.jpg");
		image_save(target, "s.png");

		// IMAGE *backup = image_fromgimp(drawable, 0, 0, drawable->width, drawable->height);
		// image_save(backup, "b.jpg");
		// image_destroy(backup);

		gimp_drawable_detach(drawable);
	} else {
		syslog_error("Create gimp layer.");
		return RET_ERROR;
	}

	return RET_OK;
}

static void color(GimpDrawable * drawable)
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
						   "Image Color with Deep Learning",
						   "This plug-in color image with deep learning technology",
						   "Dell Du <18588220928@163.com>",
						   "Copyright Dell Du <18588220928@163.com>",
						   "2020-2021", "_Color", "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/Filters/AI");
}

static void
run(const gchar * name, gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals)
{
	static GimpParam values[1];
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;
	// GimpRunMode run_mode;
	// GimpDrawable *drawable;

	/* Setting mandatory output values */
	*nreturn_vals = 1;
	*return_vals = values;
	values[0].type = GIMP_PDB_STATUS;

	if (strcmp(name, PLUG_IN_PROC) != 0 || nparams < 3) {
		values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
		return;
	}

	values[0].data.d_status = status;

	// run_mode = param[0].data.d_int32;

	CheckPoint("Image ID = %d, drawable ID = %d", param[1].data.d_image, param[2].data.d_drawable);

	// IMAGE *get_color_source(int image_id) 
	// IMAGE *get_color_target(char *url, IMAGE *input);
	// display_color_target(IMAGE *target);

	IMAGE *source = get_color_source(param[1].data.d_image);
	if (! image_valid(source)) {
		status = GIMP_PDB_EXECUTION_ERROR;
	} else {
		display_color_target(source);
		image_destroy(source);
	}

	// drawable = gimp_drawable_get(param[2].data.d_drawable);

	// if (gimp_drawable_is_rgb(drawable->drawable_id)) {
	// 	gimp_progress_init("Color...");

	// 	GTimer *timer;
	// 	timer = g_timer_new();

	// 	// color(drawable);

	// 	g_print("image color took %g seconds.\n", g_timer_elapsed(timer, NULL));
	// 	g_timer_destroy(timer);

	// 	if (run_mode != GIMP_RUN_NONINTERACTIVE)
	// 		gimp_displays_flush();
	// } else {
	// 	g_print("Drawable layer is not rgb color.\n");
	// 	status = GIMP_PDB_EXECUTION_ERROR;
	// }
	values[0].data.d_status = status;

	// gimp_drawable_detach(drawable);
}
