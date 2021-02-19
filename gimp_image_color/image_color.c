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

int blend_fake(TENSOR *source, TENSOR *fake)
{
	int i, j;
	BYTE R, G, B;
	float *source_L, *source_a, *source_b, *fake_a, *fake_b, L, a, b;

	check_tensor(source);	// 1x4xHxW
	check_tensor(fake);	// 1x2xHxW

	if (source->batch != 1 || source->chan != 4 || fake->batch != 1 || fake->chan != 2) {
		g_message("Bad source or fake tensor.");
		return RET_ERROR;
	}
	if (source->height != fake->height || source->width != fake->width) {
		g_message("Source tensor size is not same as fake.");
		return RET_ERROR;
	}

	for (i = 0; i < source->height; i++) {
		source_L = tensor_start_row(source, 0 /*batch*/, 0 /*channel*/, i);
		source_a = tensor_start_row(source, 0 /*batch*/, 1 /*channel*/, i);
		source_b = tensor_start_row(source, 0 /*batch*/, 2 /*channel*/, i);

		fake_a = tensor_start_row(fake, 0 /*batch*/, 0 /*channel*/, i);
		fake_b = tensor_start_row(fake, 0 /*batch*/, 1 /*channel*/, i);

		for (j = 0; j < source->width; j++) {
			L = *source_L; a = *fake_a; b = *fake_b;
			L = L*100.f + 50.f; a *= 110.f; b *= 110.f;
			color_lab2rgb(L, a, b, &R, &G, &B);

			L = (float)R/255.f; a = (float)G/255.f; b = (float)B/255.f;
			*source_L++ = L; *source_a++ = a; *source_b++ = b;
		}
	}
	return RET_OK;
}

int color_source(int image_id, TENSOR *tensors[2])
{
	int i, j, n_layers, ret = RET_ERROR;
	IMAGE *layers[2], *gray_image, *color_image;

	n_layers = image_layers(image_id, ARRAY_SIZE(layers), layers);
	if (n_layers < 1) {
		g_message("Error: Image must at least have 1 layes, general 2, one for gray texture and the other for color.");
		goto free_layers;
	}

	color_image = layers[0];
	if (n_layers == 2) {
		gray_image = layers[1];
		if (gray_image->height != color_image->height || gray_image->width != color_image->width) {
			g_message("Error: The size of gray and color layers is not same.");
			goto free_layers;
		}

		// Create color tensor
		image_foreach(color_image, i, j) {
			if (color_image->ie[i][j].a > 128) {
				color_image->ie[i][j].a = 255;
			} else {
				color_image->ie[i][j].a = 0;  // NO user mark color, dont trust it !!!
				color_image->ie[i][j].r = gray_image->ie[i][j].r;
				color_image->ie[i][j].g = gray_image->ie[i][j].g;
				color_image->ie[i][j].b = gray_image->ie[i][j].b;
			}
		}
	} else {
		image_foreach(color_image, i, j)
			color_image->ie[i][j].a = 0; // NO user mark color, dont trust it !!!
	}
	tensors[0] = tensor_rgb2lab(color_image);		// 0 -- Color tensor
	if (! tensor_valid(tensors[0])) {
		g_message("Error: Create color image tensor.");
		goto free_layers;
	}

	// Create gray tensor
	if (n_layers == 2) {
		gray_image = layers[1];
	} else {
		gray_image = layers[0];
	}
	// we just use it to blend with fake, so set a==255
	image_foreach(gray_image, i, j)
		gray_image->ie[i][j].a = 255;
	tensors[1] = tensor_rgb2lab(gray_image);	// 1 - Gray tensor
	if (! tensor_valid(tensors[1])) {
		g_message("Error: Create gray image tensor.");
		goto free_layers;
	}

	// PASS !!!
	ret = RET_OK;

free_layers:
	for (i = 0; i < n_layers; i++)
		image_destroy(layers[i]);

	return ret;
}

void dump(TENSOR * recv_tensor)
{
	IMAGE *image = image_from_tensor(recv_tensor, 0);
	if (image_valid(image)) {
		image_save(image, "blend.jpg");
		image_destroy(image);
	}
}

TENSOR *onnxrpc(int socket, TENSOR *send_tensor)
{
	int nh, nw, rescode;
	TENSOR *resize_send, *resize_recv, *recv_tensor;

	CHECK_TENSOR(send_tensor);

	// Color server limited: max 512, only accept 8 times !!!
	resize(send_tensor->height, send_tensor->width, 512, 8, &nh, &nw);
	
	CheckPoint("send_tensor->height = %d, send_tensor->width = %d", send_tensor->height, send_tensor->width);

	recv_tensor = NULL;
	resize_recv = NULL;
	if (send_tensor->height == nh && send_tensor->width == nw) {
		// Normal onnx RPC
		CheckPoint();
        if (request_send(socket, IMAGE_COLOR_REQCODE, send_tensor) == RET_OK) {
            recv_tensor = response_recv(socket, &rescode);
        }
	} else {
		// Resize send, Onnx RPC, Resize recv
		CheckPoint();
		resize_send = tensor_zoom(send_tensor, nh, nw); CHECK_TENSOR(resize_send);
        if (request_send(socket, IMAGE_COLOR_REQCODE, resize_send) == RET_OK) {
			CheckPoint();
            resize_recv = response_recv(socket, &rescode);
        }
		CheckPoint();
		recv_tensor = tensor_zoom(resize_recv, send_tensor->height, send_tensor->width);

		tensor_destroy(resize_recv);
		tensor_destroy(resize_send);
	}

	CheckPoint("recv_tensor->height = %d, recv_tensor->width = %d", recv_tensor->height, recv_tensor->width);

	return recv_tensor;
}

int color(gint32 image_id)
{
	int socket, ret = RET_ERROR;
	TENSOR *source[2], *target;

	socket = client_open(IMAGE_COLOR_URL);
	if (socket < 0) {
		g_message("Error: connect server.");
		return RET_ERROR;
	}

	if (color_source(image_id, source) != RET_OK) {
		client_close(socket);
		return RET_ERROR;
	}

	CheckPoint("source[0].height = %d, width = %d", source[0]->height, source[0]->width);
	CheckPoint("source[1].height = %d, width = %d", source[1]->height, source[1]->width);

	target = onnxrpc(socket, source[0]);
	if (tensor_valid(target)) {
		if (blend_fake(source[1], target) == RET_OK) {
			// dump(source[1]);
			tensor_display(source[1], "color");	// Now source has target information !!!
			ret = RET_OK;
		}
		tensor_destroy(target);
	} else {
		g_message("Error: Remote service is not valid or timeout.");
	}

	tensor_destroy(source[0]);
	tensor_destroy(source[1]);

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
