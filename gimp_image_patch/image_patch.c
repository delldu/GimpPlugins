/************************************************************************************
***
*** Copyright 2020-2021 Dell(18588220928g@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "plug-in-gimp_image_patch"
#define IMAGE_PATCH_REQCODE 0x0104
// #define IMAGE_PATCH_URL "ipc:///tmp/image_patch.ipc"
#define IMAGE_PATCH_URL "tcp://127.0.0.1:9104"

static void query(void);
static void run(const gchar * name,
				gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals);

// remote_procee_call
TENSOR *patch_rpc(TENSOR *send_tensor)
{
	int socket;
	TENSOR *recv_tensor = NULL;

	CHECK_TENSOR(send_tensor);

	if (send_tensor->chan != 4) {
		g_message("Image is not RGBA format");
		return NULL;
	}

	socket = client_open(IMAGE_PATCH_URL);
	if (socket < 0) {
		g_message("Error: connect server.");
		return NULL;
	}

	// Server only accept 128 multiples
	recv_tensor = zeropad_rpc(socket, send_tensor, IMAGE_PATCH_REQCODE, 128);
	client_close(socket);

	return recv_tensor;
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
						   "Image Patch with Deep Learning",
						   "This plug-in patch image with deep learning technology",
						   "Dell Du <18588220928@163.com>",
						   "Copyright Dell Du <18588220928@163.com>",
						   "2020-2021", "_Patch", "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/Filters/AI");
}

static void
run(const gchar * name, gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals)
{
	int x, y, height, width;
	TENSOR *send_tensor, *recv_tensor;

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
	drawable_id = param[2].data.d_drawable;

	// Add alpha channel !!!
	gimp_layer_add_alpha(drawable_id);

	drawable = gimp_drawable_get(drawable_id);

	x = y = 0;
	height = drawable->height;
	width = drawable->width;

	send_tensor = tensor_fromgimp(drawable, x, y, width, height);
	gimp_drawable_detach(drawable);

	if (tensor_valid(send_tensor)) {
		gimp_progress_init("Patching ...");

		gimp_progress_update(0.1);

		recv_tensor = patch_rpc(send_tensor);

		gimp_progress_update(0.8);
		if (tensor_valid(recv_tensor)) {
			tensor_display(recv_tensor, "patch");
			tensor_destroy(recv_tensor);
		}
		else {
			g_message("Error: Patch remote service is not avaible.");
		}
		tensor_destroy(send_tensor);
		gimp_progress_update(1.0);
	} else {
		g_message("Error: Patch image is not valid (NO RGB).");
		status = GIMP_PDB_EXECUTION_ERROR;
	}

	// Flush all ?
	gimp_displays_flush();

	// Output result for pdb
	values[0].data.d_status = status;
}
