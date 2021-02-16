/************************************************************************************
***
*** Copyright 2020-2021 Dell(18588220928g@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "plug-in-gimp_image_zoom"
#define IMAGE_ZOOM_REQCODE 0x0105
#define IMAGE_ZOOM_URL "ipc:///tmp/image_zoom.ipc"

static void query(void);
static void run(const gchar * name,
				gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals);

int zoom(GimpDrawable * drawable)
{
	int socket, ret, rescode;
	TENSOR *source, *target;

	socket = client_open(IMAGE_ZOOM_URL);
	if (socket < 0) {
		g_message("Error: connect server.");
		return RET_ERROR;
	}

	source = tensor_fromgimp(drawable, 0, 0, drawable->width, drawable->height);
	if (! tensor_valid(source)) {
		client_close(socket);
		return RET_ERROR;
	}

	ret = request_send(socket, IMAGE_ZOOM_REQCODE, source);

	if (ret == RET_OK) {
		target = response_recv(socket, &rescode);
		if (tensor_valid(target) && rescode == IMAGE_ZOOM_REQCODE) {
			tensor_display(target, "zoom");
			tensor_destroy(target);
		} else {
			g_message("Error: Remote service is not valid or timeout.");
			ret = RET_ERROR;
		}
	}	
	tensor_destroy(source);

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
						   "2020-2021", "_Zoom", "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/Filters/AI");
}

static void
run(const gchar * name, gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals)
{
	static GimpParam values[1];
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;
	// GimpRunMode run_mode;
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

	// run_mode = param[0].data.d_int32;
	drawable = gimp_drawable_get(param[2].data.d_drawable);

	if (gimp_drawable_is_rgb(drawable->drawable_id)) {
		gimp_progress_init("Zoom...");

		GTimer *timer;
		timer = g_timer_new();
		if (zoom(drawable) != RET_OK) {
			status = GIMP_PDB_EXECUTION_ERROR;
		}
		g_print("image zoom took %g seconds.\n", g_timer_elapsed(timer, NULL));
		g_timer_destroy(timer);
	} else {
		g_message("Drawable is not RGBA format.");
		status = GIMP_PDB_EXECUTION_ERROR;
	}
	values[0].data.d_status = status;

	gimp_drawable_detach(drawable);
}
