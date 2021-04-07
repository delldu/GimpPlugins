/************************************************************************************
***
*** Copyright 2020-2021 Dell(18588220928g@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "plug-in-gimp_image_light"
#define IMAGE_LIGHT_REQCODE 0x0106
// #define IMAGE_LIGHT_URL "ipc:///tmp/image_light.ipc"
#define IMAGE_LIGHT_URL "tcp://127.0.0.1:9106"

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
						   "Image Light with Deep Learning",
						   "This plug-in light image with deep learning technology",
						   "Dell Du <18588220928@163.com>",
						   "Copyright Dell Du <18588220928@163.com>",
						   "2020-2021", "_Light", "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/Filters/AI");
}

TENSOR *light_source(int image_id)
{
	int i, n_layers;
	IMAGE *layers[2];
	TENSOR *result = NULL;

	n_layers = image_layers(image_id, ARRAY_SIZE(layers), layers);

	if (n_layers < 1) {
		g_message("Error: Image must at least have 1 layes.");
		goto free_layers;
	}

	result = tensor_from_image(layers[0], 0 /* without alpha */);

free_layers:
	for (i = 0; i < n_layers; i++)
		image_destroy(layers[i]);

	return result;
}

int light(gint32 image_id)
{
	int socket, rescode, ret = RET_ERROR;
	TENSOR *source, *target = NULL;

	socket = client_open(IMAGE_LIGHT_URL);
	if (socket < 0) {
		g_message("Error: connect server.");
		return RET_ERROR;
	}
	gimp_progress_init("Light ...");

	gimp_progress_update(0.1);

	source = light_source(image_id);
	if (! tensor_valid(source)) {
		g_message("Error: Could not got patch source.");
		client_close(socket);
		return RET_ERROR;
	}
	gimp_progress_update(0.2);

    if (request_send(socket, IMAGE_LIGHT_REQCODE, source) == RET_OK) {
        target = response_recv(socket, &rescode);
    }
	gimp_progress_update(0.9);

	if (tensor_valid(target)) {
		tensor_display(target, "light");	// Now source has target information !!!
		tensor_destroy(target);

		ret = RET_OK;
	} else {
		g_message("Error: Remote light service is not availabe (maybe timeout).");
	}

	tensor_destroy(source);

	gimp_progress_update(1.0);

	return ret;
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

	if (light(param[1].data.d_image) != RET_OK)
		status = GIMP_PDB_EXECUTION_ERROR;

	values[0].data.d_status = status;
}
