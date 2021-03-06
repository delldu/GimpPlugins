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

int normal_input(TENSOR *tensor)
{
	int i, j;
	float *tensor_R, *tensor_G, *tensor_B;

	check_tensor(tensor);

	// Normal ...
	tensor_R = tensor_start_chan(tensor, 0 /*batch*/, 0 /*channel */);
	tensor_G = tensor_start_chan(tensor, 0 /*batch*/, 1 /*channel */);
	tensor_B = tensor_start_chan(tensor, 0 /*batch*/, 2 /*channel */);
	for (i = 0; i < tensor->height; i++) {
		for (j = 0; j < tensor->width; j++) {
			*tensor_R = (*tensor_R - 0.485f)/0.229f;
			*tensor_G = (*tensor_G - 0.456f)/0.224f;
			*tensor_B = (*tensor_B - 0.406f)/0.225f;

			tensor_R++; tensor_G++; tensor_B++;
		}
	}

	return RET_OK;
}

int normal_output(TENSOR *tensor)
{
	int i, size;
	float *data, min, max, d;

	check_tensor(tensor);

    // ma = torch.max(d)
    // mi = torch.min(d)
    // dn = (d-mi)/(ma-mi)
	// return dn
	size = tensor->chan * tensor->height * tensor->width;
	data = tensor->data;
	min = max = *data++;
	for (i = 1; i < size; i++, data++) {
		if (*data > max)
			max = *data;
		if (*data < min)
			min = *data;
	}

	data = tensor->data;
	max = max - min;
	if (max > 1e-3) {
		for (i = 0; i < size; i++, data++) {
			d = *data - min;
			d /= max;
			*data = d;
		}
	}

	return RET_OK;
}

TENSOR *matting_send_recv(int socket, TENSOR *send_tensor)
{
	int nh, nw, rescode;
	TENSOR *resize_send, *resize_recv, *recv_tensor;

	CHECK_TENSOR(send_tensor);

	send_tensor->chan = 3; // !!!

	// Matting server limited: only accept 4 times tensor !!!
	nh = (send_tensor->height + 3)/4; nh *= 4;
	nw = (send_tensor->width + 3)/4; nw *= 4;
	// Limited memory !!!
	while(nh > 512)
		nh /= 2;
	while (nw > 512)
		nw /= 2;

	recv_tensor = NULL;
	resize_recv = NULL;
	if (send_tensor->height == nh && send_tensor->width == nw) {
		// Normal onnx RPC
		normal_input(send_tensor);
        if (tensor_send(socket, IMAGE_MATTING_SERVICE, send_tensor) == RET_OK) {
            recv_tensor = tensor_recv(socket, &rescode);
            normal_output(recv_tensor);
        }
	} else {
		// Resize send, Onnx RPC, Resize recv
		resize_send = tensor_zoom(send_tensor, nh, nw); CHECK_TENSOR(resize_send);
		normal_input(resize_send);
        if (tensor_send(socket, IMAGE_MATTING_SERVICE, resize_send) == RET_OK) {
            resize_recv = tensor_recv(socket, &rescode);
        }
        normal_output(resize_recv);
		recv_tensor = tensor_zoom(resize_recv, send_tensor->height, send_tensor->width);

		tensor_destroy(resize_recv);
		tensor_destroy(resize_send);
	}

	return recv_tensor;
}

TENSOR *matting_rpc(TENSOR *send_tensor)
{
	int socket;
	TENSOR *recv_tensor = NULL;

	socket = client_open(IMAGE_MATTING_URL);
	if (socket < 0) {
		g_message("Error: connect server.");
		return NULL;
	}
	recv_tensor = matting_send_recv(socket, send_tensor);
	client_close(socket);

	return recv_tensor;
}

int blend_mask(TENSOR *send_tensor, TENSOR *recv_tensor)
{
	int n;
	// GimpRGB background;
	float *send_A, *recv_A;

	check_tensor(send_tensor);
	check_tensor(recv_tensor);

	send_A = tensor_start_chan(send_tensor, 0 /*batch*/, 3 /*channel*/);
	recv_A = tensor_start_chan(recv_tensor, 0 /*batch*/, 0 /*channel*/);

	// if (! gimp_palette_get_background(&background)) {
	// 	// Green Screen
	// 	background.r = 0.0;
	// 	background.g = 1.0;
	// 	background.b = 0.0;
	// 	background.a = 1.0;
	// }

	// send_tensor->chan == 4
	if (send_tensor->chan == 4) {
		n = send_tensor->height * send_tensor->width;
		memcpy(send_A, recv_A, n * sizeof(float));
		return RET_OK;
	}

	// others, we dont know how to deal with ...!!!
	return RET_ERROR;
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
						   "Image Matting with Deep Learning",
						   "This plug-in matting image with deep learning technology",
						   "Dell Du <18588220928@163.com>",
						   "Copyright Dell Du <18588220928@163.com>",
						   "2020-2021", "_Matting", "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/Filters/PAI");
}

static void
run(const gchar * name, gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals)
{
	int x, y, height, width;
	TENSOR *send_tensor, *recv_tensor, *send_clone;

	static GimpParam values[1];
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;
	// GimpRunMode run_mode;
	GimpDrawable *drawable;
	gint32 image_id, drawable_id;

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

	image_id = param[1].data.d_int32;
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

	send_tensor = tensor_fromgimp(drawable, x, y, width, height);
	if (tensor_valid(send_tensor)) {
		gimp_progress_init("Matting...");

		gimp_progress_update(0.1);

		send_clone = tensor_copy(send_tensor);

		recv_tensor = matting_rpc(send_tensor);

		gimp_progress_update(0.8);
		if (tensor_valid(recv_tensor)) {
			// Blend(send_tensor + recv_tensor) ==> send_tensor
			
			blend_mask(send_clone, recv_tensor);

			gimp_image_undo_group_start(image_id);
			tensor_togimp(send_clone, drawable, x, y, width, height);
			gimp_image_undo_group_end(image_id);

			tensor_destroy(recv_tensor);
		}
		tensor_destroy(send_clone);

		tensor_destroy(send_tensor);
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
