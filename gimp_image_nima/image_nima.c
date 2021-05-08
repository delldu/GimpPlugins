/************************************************************************************
***
*** Copyright 2020-2021 Dell(18588220928g@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "plug-in-gimp_image_nima"

static void query(void);
static void run(const gchar * name,
				gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals);

TENSOR *image_nima(TENSOR *send_tensor)
{
	int socket, ret, rescode;
	TENSOR *stand_tensor, *recv_tensor = NULL;

	CHECK_TENSOR(send_tensor);
	stand_tensor = tensor_zoom(send_tensor, 224, 224);
	CHECK_TENSOR(stand_tensor);

	// Server only accept 1x3x224x224 tensor
	if (stand_tensor->batch > 1)
		stand_tensor->batch = 1;
	if (stand_tensor->chan > 3)
		stand_tensor->chan = 3;

	socket = client_open(IMAGE_NIMA_URL);
	if (socket < 0) {
		g_message("Error: connect server.");
		return NULL;
	}

	ret = tensor_send(socket, IMAGE_NIMA_SERVICE, stand_tensor);
	if (ret == RET_OK) {
		recv_tensor = tensor_recv(socket, &rescode);
		if (! tensor_valid(recv_tensor) || rescode != IMAGE_NIMA_SERVICE) {
			g_message("Error: Remote service is not available.");
		}
	}	
	client_close(socket);
	tensor_destroy(stand_tensor);

	return recv_tensor;
}

void dump_result(TENSOR *recv_tensor)
{
	// dump scores ...
	int i;
	float mean;
	char str[32];

	if (! tensor_valid(recv_tensor)) {
		syslog_error("Bad tensor.");
		return;
	}

	mean = 0.0;
	for (i = 0; i < 10; i++) {
		mean += recv_tensor->data[i] * (i + 1.0);
	}

	snprintf(str, sizeof(str), "%6.2f", mean);
	g_message("Quality: %s\n", str);
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
						   "Image Nima with Deep Learning",
						   "This plug-in nima image with deep learning technology",
						   "Dell Du <18588220928@163.com>",
						   "Copyright Dell Du <18588220928@163.com>",
						   "2020-2021", "_Nima", "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/Filters/PAI");
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
	drawable = gimp_drawable_get(drawable_id);

	x = y = 0;
	if (! gimp_drawable_mask_intersect(drawable_id, &x, &y, &width, &height) || height * width < 64) {
		height = drawable->height;
		width = drawable->width;
	}

	send_tensor = tensor_fromgimp(drawable, x, y, width, height);
	gimp_drawable_detach(drawable);

	if (tensor_valid(send_tensor)) {
		gimp_progress_init("Nima ...");

		gimp_progress_update(0.1);

		recv_tensor = image_nima(send_tensor);

		gimp_progress_update(0.8);
		if (tensor_valid(recv_tensor)) {
			dump_result(recv_tensor);
			tensor_destroy(recv_tensor);
		}

		tensor_destroy(send_tensor);
		gimp_progress_update(1.0);
	} else {
		g_message("Error: Image Nima.");
		status = GIMP_PDB_EXECUTION_ERROR;
	}

	// Output result for pdb
	values[0].data.d_status = status;
}
