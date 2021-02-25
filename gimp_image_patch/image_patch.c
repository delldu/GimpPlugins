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

TENSOR *patch_source(int image_id)
{
	int i, j, n_layers;
	IMAGE *layers[2], *source, *mask;
	TENSOR *result = NULL;

	n_layers = image_layers(image_id, ARRAY_SIZE(layers), layers);

	if (n_layers < 1) {
		g_message("Error: Image must at least have 1 layes, general 2, one for source and the other for mask.");
		goto free_layers;
	}
	if (n_layers == 2) {
		if (layers[0]->height != layers[1]->height || layers[0]->width != layers[1]->width) {
			g_message("Error: The size of source and mask layer is not same.");
			goto free_layers;
		}
	}

	if (n_layers == 2) {
		mask = layers[0];
		source = layers[1];
		image_foreach(source, i, j) {
			source->ie[i][j].a = (mask->ie[i][j].a > 128)? 0 : 255;
		}
	} else {
		source = layers[0];
	}

	result = tensor_from_image(source, 1 /* with alpha */);

free_layers:
	for (i = 0; i < n_layers; i++)
		image_destroy(layers[i]);

	return result;
}

int patch(gint32 image_id)
{
	int socket, rescode, ret = RET_ERROR;
	TENSOR *source, *target = NULL;

	socket = client_open(IMAGE_PATCH_URL);
	if (socket < 0) {
		g_message("Error: connect server.");
		return RET_ERROR;
	}
	gimp_progress_init("Patch ...");

	gimp_progress_update(0.1);

	source = patch_source(image_id);
	if (! tensor_valid(source)) {
		g_message("Error: Could not got patch source.");
		client_close(socket);
		return RET_ERROR;
	}
	gimp_progress_update(0.2);

    if (request_send(socket, IMAGE_PATCH_REQCODE, source) == RET_OK) {
        target = response_recv(socket, &rescode);
    }
	gimp_progress_update(0.9);

	if (tensor_valid(target)) {
		tensor_display(target, "patch");	// Now source has target information !!!
		tensor_destroy(target);

		ret = RET_OK;
	} else {
		g_message("Error: Remote patch service is not availabe (maybe timeout).");
	}

	tensor_destroy(source);

	gimp_progress_update(1.0);

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
	static GimpParam values[1];
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;
	// GimpRunMode run_mode;

	/* Setting output values */
	*nreturn_vals = 1;
	*return_vals = values;
	values[0].type = GIMP_PDB_STATUS;

	if (strcmp(name, PLUG_IN_PROC) != 0 || nparams < 3) {
		values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
		return;
	}
	values[0].data.d_status = status;

	if (patch(param[1].data.d_image) != RET_OK)
		status = GIMP_PDB_EXECUTION_ERROR;

	values[0].data.d_status = status;
}
