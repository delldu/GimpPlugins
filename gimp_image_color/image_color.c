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
#define IMAGE_COLOR_URL "ipc:///tmp/image_color.ipc"

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
		*R++ = L; 
		if (image->ie[i][j].a > 0) {
			*G++ = a; *B++ = b; *A++ = 0.5f;
		} else {
			*G++ = 0.f; *B++ = 0.f; *A++ = -0.5f;
		}
	}

	return tensor;
}

int blend_fake(TENSOR *source, TENSOR *fake_ab)
{
	int i, j;
	float *source_L, *source_a, *source_b, *source_m;
	float *fake_a, *fake_b;
	float L, a, b;
	BYTE R, G, B;

	check_tensor(source);	// 1x4xHxW
	check_tensor(fake_ab);	// 1x2xHxW

	if (source->batch != 1 || source->chan != 4 || fake_ab->batch != 1 || fake_ab->chan != 2) {
		g_message("Bad source or fake_ab tensor.");
		return RET_ERROR;
	}
	if (source->height != fake_ab->height || source->width != fake_ab->width) {
		g_message("Source tensor size is not same as fake_ab.");
		return RET_ERROR;
	}

	for (i = 0; i < source->height; i++) {
		source_L = tensor_startrow(source, 0 /*batch*/, 0 /*channel*/, i);
		source_a = tensor_startrow(source, 0 /*batch*/, 1 /*channel*/, i);
		source_b = tensor_startrow(source, 0 /*batch*/, 2 /*channel*/, i);
		source_m = tensor_startrow(source, 0 /*batch*/, 3 /*channel*/, i);

		fake_a = tensor_startrow(fake_ab, 0 /*batch*/, 0 /*channel*/, i);
		fake_b = tensor_startrow(fake_ab, 0 /*batch*/, 1 /*channel*/, i);

		for (j = 0; j < source->width; j++) {
			L = *source_L; a = *fake_a; b = *fake_b;
			L = L*100.f + 50.f; a *= 110.f; b *= 110.f;
			color_lab2rgb(L, a, b, &R, &G, &B);
			L = (float)R/255.f;
			a = (float)G/255.f;
			b = (float)B/255.f;
			*source_L++ = L;
			*source_a++ = a;
			*source_b++ = b;
			*source_m++ = 0.f;
		}
	}
	return RET_OK;
}


TENSOR *color_source(int image_id)
{
	int i, j, n_layers;
	IMAGE *layers[2], *gray_image, *color_image;
	TENSOR *tensor = NULL;

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

	image_foreach(color_image, i, j) {
		if (color_image->ie[i][j].a > 128) {
			gray_image->ie[i][j].r = color_image->ie[i][j].r;
			gray_image->ie[i][j].g = color_image->ie[i][j].g;
			gray_image->ie[i][j].b = color_image->ie[i][j].b;
			gray_image->ie[i][j].a = 255;
		} else {
			gray_image->ie[i][j].a = 0;
		}
	}
	tensor = color_normlab(gray_image);

free_layers:
	for (i = 0; i < n_layers; i++)
		image_destroy(layers[i]);

	return tensor;
}

int color(gint32 image_id)
{
	int socket, ret, rescode;
	TENSOR *source, *target;

	socket = client_open(IMAGE_COLOR_URL);
	if (socket < 0) {
		g_message("Error: connect server.");
		return RET_ERROR;
	}

	source = color_source(image_id);
	if (! tensor_valid(source)) {
		client_close(socket);
		return RET_ERROR;
	}

	ret = request_send(socket, IMAGE_COLOR_REQCODE, source);

	if (ret == RET_OK) {
		target = response_recv(socket, &rescode);
		if (tensor_valid(target) && rescode == IMAGE_COLOR_REQCODE) {
			if (blend_fake(source, target) == RET_OK)
				tensor_display(source, "color");	// Now source has target information !!!
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
