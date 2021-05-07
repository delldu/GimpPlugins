/************************************************************************************
***
***	Copyright 2021 Dell(18588220928g@163.com), All Rights Reserved.
***
***	File Author: Dell, 2021-01-16 12:41:12
***
************************************************************************************/

#include "plugin.h"

// Just for reference
// IMAGE *image_fromgimp(GimpDrawable * drawable, int x, int y, int width, int height)
// {
// 	gint i, j;
// 	gint channels;
// 	GimpPixelRgn input_rgn;
// 	IMAGE *image = NULL;
// 	guchar *rgn_data, *d;

// 	channels = drawable->bpp;
// 	gimp_pixel_rgn_init(&input_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);

// 	image = image_create(height, width);
// 	if (!image_valid(image)) {
// 		g_print("Create image failure.\n");
// 		return NULL;
// 	}

// 	rgn_data = g_new(guchar, height * width * channels);
// 	if (!rgn_data) {
// 		g_print("Memory allocate (%d bytes) failure.\n", height * width * channels);
// 		image_destroy(image);
// 		return NULL;
// 	}

// 	gimp_pixel_rgn_get_rect(&input_rgn, rgn_data, x, y, width, height);

// 	d = rgn_data;
// 	switch (channels) {
// 	case 1:
// 		for (i = 0; i < height; i++) {
// 			for (j = 0; j < width; j++) {
// 				image->ie[i][j].r = *d++;
// 				image->ie[i][j].g = image->ie[i][j].r;
// 				image->ie[i][j].b = image->ie[i][j].r;
// 				image->ie[i][j].a = 255;
// 			}
// 		}
// 		break;
// 	case 2:
// 		// Mono Gray + Alpha
// 		for (i = 0; i < height; i++) {
// 			for (j = 0; j < width; j++) {
// 				image->ie[i][j].r = *d++;
// 				image->ie[i][j].g = image->ie[i][j].r;
// 				image->ie[i][j].b = image->ie[i][j].r;
// 				image->ie[i][j].a = *d++;
// 			}
// 		}
// 		break;
// 	case 3:
// 		for (i = 0; i < height; i++) {
// 			for (j = 0; j < width; j++) {
// 				image->ie[i][j].r = *d++;
// 				image->ie[i][j].g = *d++;
// 				image->ie[i][j].b = *d++;
// 				image->ie[i][j].a = 255;
// 			}
// 		}
// 		break;
// 	case 4:
// 		for (i = 0; i < height; i++) {
// 			for (j = 0; j < width; j++) {
// 				image->ie[i][j].r = *d++;
// 				image->ie[i][j].g = *d++;
// 				image->ie[i][j].b = *d++;
// 				image->ie[i][j].a = *d++;
// 			}
// 		}
// 		break;
// 	default:
// 		// Error ?
// 		g_print("Invalid channels: %d\n", channels);
// 		break;
// 	}

// 	g_free(rgn_data);

// 	return image;
// }


TENSOR *tensor_fromgimp(GimpDrawable * drawable, int x, int y, int width, int height)
{
	gint i, j, k;
	gint c, channels;
	GimpPixelRgn input_rgn;
	TENSOR *tensor = NULL;
	guchar *rgn_data, *s;
	float *d;

	channels = drawable->bpp;
	gimp_pixel_rgn_init(&input_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);

	tensor = tensor_create(1 /*batch*/, channels, height, width);
	if (!tensor_valid(tensor)) {
		g_print("Create tensor.\n");
		return NULL;
	}

	rgn_data = g_new(guchar, height * width * channels);
	if (!rgn_data) {
		g_print("Memory allocate (%d bytes) failure.\n", height * width * channels);
		tensor_destroy(tensor);
		return NULL;
	}

	gimp_pixel_rgn_get_rect(&input_rgn, rgn_data, x, y, width, height);

	s = rgn_data;		// source
	d = tensor->data;	// destion
	for (c = 0; c < channels; c++) {
		for (i = 0; i < height; i++) {
			k = i * width * channels; // start row i, for HxWxC format
			for (j = 0; j < width; j++)
				*d++ = ((float)s[k + j*channels + c])/255.0;
		}
	}

	g_free(rgn_data);

	return tensor;
}

int tensor_togimp(TENSOR * tensor, GimpDrawable * drawable, int x, int y, int width, int height)
{
	gint i, j, k;
	gint c, channels;
	GimpPixelRgn output_rgn;
	guchar *rgn_data, *d;
	float *s;

	channels = drawable->bpp;
	if (channels != tensor->chan) {
		g_print("Error: Tensor channels is not same with drawable bpp");
		return RET_ERROR;
	}
	gimp_pixel_rgn_init(&output_rgn, drawable, 0, 0, drawable->width, drawable->height, TRUE, FALSE);

	rgn_data = g_new(guchar, height * width * channels);
	if (!rgn_data) {
		g_print("Memory allocate (%d bytes) failure.\n", height * width * channels);
		return RET_ERROR;
	}

	d = rgn_data;
	s = tensor->data;
	for (c = 0; c < channels; c++) {
		for (i = 0; i < height; i++) {
			k = i * width * channels; // start row i for HxWxC
			for (j = 0; j < width; j++)
				d[k + j*channels + c] = (guchar)((*s++)*255.0);
		}
	}

	gimp_pixel_rgn_set_rect(&output_rgn, rgn_data, x, y, width, height);

	g_free(rgn_data);

	return RET_OK;
}

// Just for reference
// int image_layers(int image_id, int max_layers, IMAGE *layers[])
// {
// 	gint i;
// 	gint32 layer_count, *layer_id_list;
// 	GimpDrawable *drawable;
	
// 	layer_id_list = gimp_image_get_layers ((gint32)image_id, &layer_count);
// 	if (layer_count > max_layers)
// 		layer_count = max_layers;

// 	for (i = 0; i < layer_count; i++) {
// 		drawable = gimp_drawable_get(layer_id_list[i]);
// 		layers[i] = image_fromgimp(drawable, 0, 0, drawable->width, drawable->height);
// 		gimp_drawable_detach(drawable);
// 	}
// 	g_free (layer_id_list);

// 	return layer_count;
// }


int tensor_display(TENSOR *tensor, gchar *name_prefix)
{
	gchar name[64];
	GimpDrawable *drawable;
	gint32 image_ID, layer_ID;

	check_tensor(tensor);

	if (tensor->chan != 3 && tensor->chan != 4) {
		g_message("Error: Tensor channel must be 3 or 4");
		return RET_ERROR;
	}

	image_ID = gimp_image_new(tensor->width, tensor->height, GIMP_RGB);
	if (image_ID < 0) {
		g_message("Error: Create gimp image.");
		return RET_ERROR;
	}

	g_snprintf(name, sizeof(name), "%s_%d", name_prefix, image_ID);

	layer_ID = -1;
	if (tensor->chan == 4)
		layer_ID = gimp_layer_new(image_ID, name, tensor->width, tensor->height, GIMP_RGBA_IMAGE, 100.0, 
			GIMP_NORMAL_MODE);
	else if (tensor->chan == 3)
		layer_ID = gimp_layer_new(image_ID, name, tensor->width, tensor->height, GIMP_RGB_IMAGE, 100.0, 
			GIMP_NORMAL_MODE);

	if (layer_ID > 0) {
		drawable = gimp_drawable_get((gint32)layer_ID);
		tensor_togimp(tensor, drawable,  0, 0, tensor->width, tensor->height);
		if (! gimp_image_insert_layer(image_ID, layer_ID, 0, 0)) {
			g_message("Error: Insert layer error.");
		}
		gimp_display_new(image_ID);
		gimp_displays_flush();

		gimp_drawable_detach(drawable);
	} else {
		g_message("Error: Create gimp layer.");
		return RET_ERROR;
	}

	return RET_OK;
}

TENSOR *normal_rpc(int socket, TENSOR *send_tensor, int reqcode)
{
	int rescode = -1;
	TENSOR *recv_tensor = NULL;

	CHECK_TENSOR(send_tensor);

    if (tensor_send(socket, reqcode, send_tensor) == RET_OK) {
        recv_tensor = tensor_recv(socket, &rescode);
    }

    if (SERVICE_CODE(rescode) != SERVICE_CODE(reqcode)) {
    	// Bad service response
    	syslog_error("reqcode = 0x%x, rescode = 0x%x", reqcode, rescode);
    	tensor_destroy(recv_tensor);
    	return NULL;
    }

	return recv_tensor;
}


TENSOR *zeropad_rpc(int socket, TENSOR *send_tensor, int reqcode, int multiples)
{
	int nh, nw;
	TENSOR *resize_send, *resize_recv, *recv_tensor;

	CHECK_TENSOR(send_tensor);

	// Server only accept multiples tensor !!!
	nh = (send_tensor->height + multiples - 1)/multiples; nh *= multiples;
	nw = (send_tensor->width + multiples - 1)/multiples; nw *= multiples;

	recv_tensor = NULL;
	resize_recv = NULL;
	if (send_tensor->height == nh && send_tensor->width == nw) {
		// Normal onnx RPC
		recv_tensor = normal_rpc(socket, send_tensor, reqcode);
	} else {
		// Resize send, Onnx RPC, Resize recv
		resize_send = tensor_zeropad(send_tensor, nh, nw); CHECK_TENSOR(resize_send);
		resize_recv = normal_rpc(socket, resize_send, reqcode);
		recv_tensor = tensor_zeropad(resize_recv, send_tensor->height, send_tensor->width);

		tensor_destroy(resize_recv);
		tensor_destroy(resize_send);
	}

	return recv_tensor;
}

TENSOR *resize_rpc(int socket, TENSOR *send_tensor, int reqcode, int multiples)
{
	int nh, nw;
	TENSOR *resize_send, *resize_recv, *recv_tensor;

	CHECK_TENSOR(send_tensor);

	// Server only multiples times tensor !!!
	nh = (send_tensor->height + multiples - 1)/multiples; nh *= multiples;
	nw = (send_tensor->width + multiples - 1)/multiples; nw *= multiples;

	recv_tensor = NULL;
	resize_recv = NULL;
	if (send_tensor->height == nh && send_tensor->width == nw) {
		// Normal onnx RPC
		recv_tensor = normal_rpc(socket, send_tensor, reqcode);
	} else {
		// Resize send, Onnx RPC, Resize recv
		resize_send = tensor_zoom(send_tensor, nh, nw); CHECK_TENSOR(resize_send);
		resize_recv = normal_rpc(socket, resize_send, reqcode); CHECK_TENSOR(resize_recv);
		recv_tensor = tensor_zoom(resize_recv, send_tensor->height, send_tensor->width); CHECK_TENSOR(recv_tensor);

		tensor_destroy(resize_recv);
		tensor_destroy(resize_send);
	}

	return recv_tensor;
}

