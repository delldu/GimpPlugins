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


TENSOR *color_normlab(IMAGE * image)
{
	int i, j;
	TENSOR *tensor;
	float *R, *G, *B, *A, L, a, b;

	CHECK_IMAGE(image);

	tensor = tensor_create(1, sizeof(RGBA_8888), image->height, image->width);
	CHECK_TENSOR(tensor);

	R = tensor->data;
	G = R + tensor->height * tensor->width;
	B = G + tensor->height * tensor->width;
	A = B + tensor->height * tensor->width;

	image_foreach(image, i, j) {
		color_rgb2lab(image->ie[i][j].r, image->ie[i][j].g, image->ie[i][j].b, &L, &a, &b);
		L = (L - 50.f)/100.f; a /= 110.f; b /= 110.f;
		*R++ = L; *G++ = a; *B++ = b;
		*A++ = (image->ie[i][j].a > 128)? 1.0f : 0.f;
	}

	return tensor;
}

int blend_fake(TENSOR *gray_source, TENSOR *fake_ab)
{
	int i, j;
	float *gray_source_L, *gray_source_a, *gray_source_b;
	float *fake_a, *fake_b;
	float L, a, b;
	BYTE R, G, B;

	check_tensor(gray_source);	// 1x4xHxW
	check_tensor(fake_ab);	// 1x2xHxW

	if (gray_source->batch != 1 || gray_source->chan != 4 || fake_ab->batch != 1 || fake_ab->chan != 2) {
		g_message("Bad gray_source or fake_ab tensor.");
		return RET_ERROR;
	}
	if (gray_source->height != fake_ab->height || gray_source->width != fake_ab->width) {
		g_message("Source tensor size is not same as fake_ab.");
		return RET_ERROR;
	}

	for (i = 0; i < gray_source->height; i++) {
		gray_source_L = tensor_startrow(gray_source, 0 /*batch*/, 0 /*channel*/, i);
		gray_source_a = tensor_startrow(gray_source, 0 /*batch*/, 1 /*channel*/, i);
		gray_source_b = tensor_startrow(gray_source, 0 /*batch*/, 2 /*channel*/, i);

		fake_a = tensor_startrow(fake_ab, 0 /*batch*/, 0 /*channel*/, i);
		fake_b = tensor_startrow(fake_ab, 0 /*batch*/, 1 /*channel*/, i);

		for (j = 0; j < gray_source->width; j++) {
			L = *gray_source_L; a = *fake_a; b = *fake_b;
			L = L*100.f + 50.f; a *= 110.f; b *= 110.f;
			color_lab2rgb(L, a, b, &R, &G, &B);
			// CheckPoint("r = %d, g=%d, b=%d, L=%.4f, a=%.4f, b=%.4f", 
			// 	R, G, B,
			// 	L, a, b);

			L = (float)R/255.f;
			a = (float)G/255.f;
			b = (float)B/255.f;
			*gray_source_L++ = L;
			*gray_source_a++ = a;
			*gray_source_b++ = b;
		}
	}
	return RET_OK;
}

int color_source(int image_id, TENSOR *tensors[2])
{
	int i, j, n_layers, ret;
	IMAGE *layers[2], *gray_image, *color_image;

	ret = RET_ERROR;

	n_layers = image_layers(image_id, ARRAY_SIZE(layers), layers);
	if (n_layers != 2) {
		g_message("Error: Image must have 2 layes, one for gray texture and the other for color.");
		goto free_layers;
	}

	color_image = layers[0];
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
			color_image->ie[i][j].a = 0;

			color_image->ie[i][j].r = gray_image->ie[i][j].r;
			color_image->ie[i][j].g = gray_image->ie[i][j].g;
			color_image->ie[i][j].b = gray_image->ie[i][j].b;
		}
	}
	tensors[0] = color_normlab(color_image);		// 0 -- Color tensor
	if (! tensor_valid(tensors[0])) {
		g_message("Error: Create color image tensor.");
		goto free_layers;
	}

	// Create gray tensor
	image_foreach(gray_image, i, j)
		gray_image->ie[i][j].a = 255;
	tensors[1] = color_normlab(gray_image);	// 1 - Gray tensor
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

int color(gint32 image_id)
{
	int socket, ret, rescode;
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

	ret = request_send(socket, IMAGE_COLOR_REQCODE, source[0]);

	if (ret == RET_OK) {
		target = response_recv(socket, &rescode);
		if (tensor_valid(target) && rescode == IMAGE_COLOR_REQCODE) {
			if (blend_fake(source[1], target) == RET_OK) {
				// dump(source[1]);
				tensor_display(source[1], "color");	// Now source has target information !!!
			}
			tensor_destroy(target);
		} else {
			g_message("Error: Remote service is not valid or timeout.");
			ret = RET_ERROR;
		}
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
