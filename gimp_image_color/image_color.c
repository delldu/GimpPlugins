/************************************************************************************
***
*** Copyright 2020-2021 Dell(18588220928g@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "plug-in-gimp_image_color"

static void query(void);
static void run(const gchar * name,
				gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals);

TENSOR *image_color_rgb2lab(TENSOR *send_rgb_tensor)
{
	int batch, i, n;
	float *send_rgb_rc, *send_rgb_gc, *send_rgb_bc;
	float *send_lab_lc, *send_lab_ac, *send_lab_bc, *send_lab_mc;	// mask channel
	BYTE R, G, B;
	float L, a, b;
	TENSOR *send_lab_tensor = NULL;

	CHECK_TENSOR(send_rgb_tensor);

	// Lab + alpha channel
	send_lab_tensor = tensor_create(send_rgb_tensor->batch, 4, send_rgb_tensor->height, send_rgb_tensor->width);
	CHECK_TENSOR(send_lab_tensor);

	n = send_rgb_tensor->height * send_rgb_tensor->width;

	for (batch = 0; batch < send_rgb_tensor->batch; batch++) {
		if (send_rgb_tensor->chan >= 3) {
			send_rgb_rc = tensor_start_chan(send_rgb_tensor, batch, 0);	// R
			send_rgb_gc = tensor_start_chan(send_rgb_tensor, batch, 1);	// G
			send_rgb_bc = tensor_start_chan(send_rgb_tensor, batch, 2);	// B
		} else {
			send_rgb_rc = send_rgb_gc = send_rgb_bc = tensor_start_chan(send_rgb_tensor, batch, 0);	// R
		}

		send_lab_lc = tensor_start_chan(send_lab_tensor, batch, 0);	// L
		send_lab_ac = tensor_start_chan(send_lab_tensor, batch, 1);	// a
		send_lab_bc = tensor_start_chan(send_lab_tensor, batch, 2);	// b
		send_lab_mc = tensor_start_chan(send_lab_tensor, batch, 3);	// m -- mask 

		for (i = 0; i < n; i++) {
			R = (BYTE)(send_rgb_rc[i] * 255.0);
			G = (BYTE)(send_rgb_gc[i] * 255.0);
			B = (BYTE)(send_rgb_bc[i] * 255.0);

			color_rgb2lab(R, G, B, &L, &a, &b);

			L = (L - 50.0)/100.0;
			a /= 110.0;
			b /= 110.0;

			send_lab_lc[i] = L;
			send_lab_ac[i] = a;
			send_lab_bc[i] = b;

			// Black or white ?
			if ((R < 10 && G < 10 && B < 10) || (R > 245 && G > 245 && B > 245)) {
				send_lab_mc[i] = 1.0;
			} else {
				send_lab_mc[i] = (ABS(a) > 0.1 ||  ABS(b) > 0.1)? 1.0 : 0.0;
			}
		}
	}

	return send_lab_tensor;
}

TENSOR *image_color_lab2rgb(TENSOR *send_lab_tensor, TENSOR *recv_ab_tensor)
{
	int i, n;
	float *send_lab_lc, *recv_ab_ac, *recv_ab_bc;	// channels
	float *blend_rgb_rc, *blend_rgb_gc, *blend_rgb_bc;
	float L, a, b;
	BYTE R, G, B;
	TENSOR *blend_rgb_tensor = NULL;

	CHECK_TENSOR(send_lab_tensor);
	CHECK_TENSOR(recv_ab_tensor);

	if (send_lab_tensor->chan != 4) {
		g_message("Error: send tensor is not LAB + alpha format.");
		return NULL;
	}

	if (recv_ab_tensor->chan != 2) {
		g_message("Error: this tensor is not genearated by color server.");
		return NULL;
	}

	if (send_lab_tensor->batch != recv_ab_tensor->batch) {
		g_message("Error: send tensor batch is not same as recvs");
		return NULL;
	}

	if (send_lab_tensor->height != recv_ab_tensor->height || send_lab_tensor->width != recv_ab_tensor->width) {
		g_message("Error: send tensor size is not same as recvs.");
		return NULL;
	}

	blend_rgb_tensor = tensor_create(send_lab_tensor->batch, 3, send_lab_tensor->height, send_lab_tensor->width);
	CHECK_TENSOR(blend_rgb_tensor);


	send_lab_lc = tensor_start_chan(send_lab_tensor, 0, 0);	// L
	recv_ab_ac = tensor_start_chan(recv_ab_tensor, 0, 0);	// a
	recv_ab_bc = tensor_start_chan(recv_ab_tensor, 0, 1);	// b

	blend_rgb_rc = tensor_start_chan(blend_rgb_tensor, 0, 0);	// R
	blend_rgb_gc = tensor_start_chan(blend_rgb_tensor, 0, 1);	// G
	blend_rgb_bc = tensor_start_chan(blend_rgb_tensor, 0, 2);	// B

	n = send_lab_tensor->height * send_lab_tensor->width;
	for (i = 0; i < n; i++) {
		L = *send_lab_lc++; L += 0.5; L *= 100.0;
		a = *recv_ab_ac++; a *= 110.0;
		b = *recv_ab_bc++; b *= 110;

		color_lab2rgb(L, a, b, &R, &G, &B);

		*blend_rgb_rc++ = (float)R/255.0;
		*blend_rgb_gc++ = (float)G/255.0;
		*blend_rgb_bc++ = (float)B/255.0;
	}

	return blend_rgb_tensor;
}

TENSOR *color_rpc(TENSOR *send_rgb_tensor)
{
	int socket;
	TENSOR *recv_rgb_tensor = NULL;
	TENSOR *send_lab_tensor, *recv_ab_tensor;

	CHECK_TENSOR(send_rgb_tensor);

	socket = client_open(IMAGE_COLOR_URL);
	if (socket < 0) {
		g_message("Error: connect server.");
		return NULL;
	}

	send_lab_tensor = image_color_rgb2lab(send_rgb_tensor);

	// Color server only accept 8 times !!!
	recv_ab_tensor = resize_rpc(socket, send_lab_tensor, IMAGE_COLOR_SERVICE, 8);

	if (tensor_valid(recv_ab_tensor)) {
		recv_rgb_tensor = image_color_lab2rgb(send_lab_tensor, recv_ab_tensor);
		tensor_destroy(recv_ab_tensor);
	}
	tensor_destroy(send_lab_tensor);

	client_close(socket);

	return recv_rgb_tensor;
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

	// Support local color
	x = y = 0;
	if (! gimp_drawable_mask_intersect(drawable_id, &x, &y, &width, &height) || height * width < 64) {
		height = drawable->height;
		width = drawable->width;
	}

	send_tensor = tensor_fromgimp(drawable, x, y, width, height);
	gimp_drawable_detach(drawable);

	if (tensor_valid(send_tensor)) {
		gimp_progress_init("Coloring ...");

		gimp_progress_update(0.1);

		recv_tensor = color_rpc(send_tensor);

		gimp_progress_update(0.8);
		if (tensor_valid(recv_tensor)) {
			tensor_display(recv_tensor, "color");
			tensor_destroy(recv_tensor);
		}
		else {
			g_message("Error: Color remote service is not avaible.");
		}
		tensor_destroy(send_tensor);
		gimp_progress_update(1.0);
	} else {
		g_message("Error: Color source.");
		status = GIMP_PDB_EXECUTION_ERROR;
	}

	// Flush all ?
	gimp_displays_flush();

	// Output result for pdb
	values[0].data.d_status = status;
}
