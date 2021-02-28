/************************************************************************************
***
*** Copyright 2020-2021 Dell(18588220928g@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "plug-in-gimp_image_color"

#define IMAGE_COLOR_REQCODE 0x0102
// #define IMAGE_COLOR_URL "ipc:///tmp/image_color.ipc"
#define IMAGE_COLOR_URL "tcp://127.0.0.1:9102"


static void query(void);
static void run(const gchar * name,
				gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals);

TENSOR *blend_fake(TENSOR *gray, TENSOR *fake)
{
	int i, j;
	IMAGE *image;
	TENSOR *result;
	float *gray_a, *gray_b, *gray_m, *fake_a, *fake_b;

	CHECK_TENSOR(gray);	// 1x4xHxW
	CHECK_TENSOR(fake);	// 1x2xHxW

	if (gray->batch != 1 || gray->chan != 4 || fake->batch != 1 || fake->chan != 2) {
		g_message("Bad source or fake tensor.");
		return NULL;
	}
	if (gray->height != fake->height || gray->width != fake->width) {
		g_message("Source tensor size is not same as fake.");
		return NULL;
	}

	gray_a = tensor_start_chan(gray, 0 /*batch*/, 1 /*channel*/);
	gray_b = tensor_start_chan(gray, 0 /*batch*/, 2 /*channel*/);
	gray_m = tensor_start_chan(gray, 0 /*batch*/, 3 /*channel*/);

	fake_a = tensor_start_chan(fake, 0 /*batch*/, 0 /*channel*/);
	fake_b = tensor_start_chan(fake, 0 /*batch*/, 1 /*channel*/);

	for (i = 0; i < gray->height; i++) {
		for (j = 0; j < gray->width; j++) {
			*gray_a++ = *fake_a++;
			*gray_b++ = *fake_b++;
			*gray_m++ = 1.0;	// We just want to show, so set 1.0 !!!
		}
	}

	image = tensor_lab2rgb(gray, 0); CHECK_IMAGE(image);
	result = tensor_from_image(image, 1 /*with alpha */); CHECK_TENSOR(result);
	image_destroy(image);

	return result;
}

TENSOR *auto_paint(IMAGE *gray)
{
	TENSOR *source;
	// Alpha must be 255(RGBA) for image_fromgimp !
	source = tensor_rgb2lab(gray);
	// if user paint nothing, we trust it !!!
	tensor_setmask(source, 0.0f);

	return source;
}

TENSOR *user_paint(IMAGE *gray, IMAGE *color)
{
	int i, j;
	TENSOR *source;
	float *source_a, *source_b, *source_m, L, a, b;

	CHECK_IMAGE(gray);
	CHECK_IMAGE(color);

	// Suppose same size between gray and color
	// if (gray->height != color->height || gray->width != color->width) {
	// 	g_message("Error: The size of gray and color layer is not same.");
	// 	return NULL;
	// }
	source = tensor_rgb2lab(gray); CHECK_TENSOR(source);
	for (i = 0; i < color->height; i++) {
		// source_L = tensor_start_row(source, 0, 0, i);
		source_a = tensor_start_row(source, 0, 1, i);
		source_b = tensor_start_row(source, 0, 2, i);
		source_m = tensor_start_row(source, 0, 3, i);
		for (j = 0; j < color->width; j++) {
			// if user paint some something, we trust it !!!
			if (color->ie[i][j].a > 128) {
				color_rgb2lab(color->ie[i][j].r, color->ie[i][j].g, color->ie[i][j].b, &L, &a, &b);
				a /= 110.f; b /= 110.f;
				*source_a++ = a;
				*source_b++ = b;
				*source_m++ = 1.0f;
			}
			else {
				source_a++;	// skip
				source_b++;	// skip
				*source_m++ = 0.f;
			}
		}
	}

	return source;
}

// Layers: ["user layer"] + "auto layer"
int color_source(int image_id, TENSOR *tensors[2])
{
	int i, n_layers, ret = RET_ERROR;
	IMAGE *layers[2];

	n_layers = image_layers(image_id, ARRAY_SIZE(layers), layers);

	// Checking ...
	if (n_layers < 1) {
		g_message("Error: Image must at least have 1 layes, general 2, one for gray texture and the other for color.");
		goto free_layers;
	}
	if (n_layers == 2) {
		if (layers[0]->height != layers[1]->height || layers[0]->width != layers[1]->width) {
			g_message("Error: The size of source layers is not same.");
			goto free_layers;
		}
	}

	// Create tensors ...
	if (n_layers == 2) {
		tensors[0] = user_paint(layers[1], layers[0]);	// Will send to remote server
		tensors[1] = auto_paint(layers[1]);				// Save source
	} else {
		tensors[0] = auto_paint(layers[0]);				// Will send to remote server
		tensors[1] = tensor_copy(tensors[0]);			// Simple copy for save source
	}

	// PASS !!!
	ret = RET_OK;

free_layers:
	for (i = 0; i < n_layers; i++)
		image_destroy(layers[i]);

	return ret;
}

TENSOR *resize_onnxrpc(int socket, TENSOR *send_tensor)
{
	int nh, nw, rescode;
	TENSOR *resize_send, *resize_recv, *recv_tensor;

	CHECK_TENSOR(send_tensor);

	// Color server limited: max 512, only accept 8 times !!!
	resize(send_tensor->height, send_tensor->width, 512, 8, &nh, &nw);
	recv_tensor = NULL;
	resize_recv = NULL;
	if (send_tensor->height == nh && send_tensor->width == nw) {
		// Normal onnx RPC
        if (request_send(socket, IMAGE_COLOR_REQCODE, send_tensor) == RET_OK) {
            recv_tensor = response_recv(socket, &rescode);
        }
	} else {
		// Resize send, Onnx RPC, Resize recv
		resize_send = tensor_zoom(send_tensor, nh, nw); CHECK_TENSOR(resize_send);
        if (request_send(socket, IMAGE_COLOR_REQCODE, resize_send) == RET_OK) {
            resize_recv = response_recv(socket, &rescode);
        }
		recv_tensor = tensor_zoom(resize_recv, send_tensor->height, send_tensor->width);

		tensor_destroy(resize_recv);
		tensor_destroy(resize_send);
	}

	return recv_tensor;
}

int color(gint32 image_id)
{
	int socket, ret = RET_ERROR;
	TENSOR *source[2], *target, *blend_result;

	socket = client_open(IMAGE_COLOR_URL);
	if (socket < 0) {
		g_message("Error: connect server.");
		return RET_ERROR;
	}
	gimp_progress_init("Color ...");

	gimp_progress_update(0.1);

	if (color_source(image_id, source) != RET_OK) {
		client_close(socket);
		return RET_ERROR;
	}
	gimp_progress_update(0.2);

	target = resize_onnxrpc(socket, source[0]);
	gimp_progress_update(0.9);

	if (tensor_valid(target)) {
		blend_result = blend_fake(source[1], target);
		if (tensor_valid(blend_result)) {
			tensor_display(blend_result, "color");	// Now source has target information !!!
			tensor_destroy(blend_result);

			ret = RET_OK;
		}
		tensor_destroy(target);
	} else {
		g_message("Error: Remote color service is not availabe (maybe timeout).");
	}

	tensor_destroy(source[0]);
	tensor_destroy(source[1]);

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

	/* Setting mandatory output values */
	*nreturn_vals = 1;
	*return_vals = values;
	values[0].type = GIMP_PDB_STATUS;

	if (strcmp(name, PLUG_IN_PROC) != 0 || nparams < 3) {
		values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
		return;
	}
	values[0].data.d_status = status;

	if (color(param[1].data.d_image) != RET_OK)
		status = GIMP_PDB_EXECUTION_ERROR;

	values[0].data.d_status = status;
}
