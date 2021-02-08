/************************************************************************************
***
***	Copyright 2021 Dell(18588220928g@163.com), All Rights Reserved.
***
***	File Author: Dell, 2021-01-16 12:41:12
***
************************************************************************************/


#include "plugin.h"

IMAGE *image_fromgimp(GimpDrawable * drawable, int x, int y, int width, int height)
{
	gint i, j;
	gint channels;
	GimpPixelRgn input_rgn;
	IMAGE *image = NULL;
	guchar *rgn_data, *d;

	channels = drawable->bpp;
	gimp_pixel_rgn_init(&input_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);

	image = image_create(height, width);
	if (!image_valid(image)) {
		g_print("Create image failure.\n");
		return NULL;
	}

	rgn_data = g_new(guchar, height * width * channels);
	if (!rgn_data) {
		g_print("Memory allocate (%d bytes) failure.\n", height * width * channels);
		image_destroy(image);
		return NULL;
	}

	gimp_pixel_rgn_get_rect(&input_rgn, rgn_data, x, y, width, height);

	d = rgn_data;
	switch (channels) {
	case 1:
		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
				image->ie[i][j].r = *d++;
			}
		}
		break;
	case 2:
		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
				image->ie[i][j].r = *d++;
				image->ie[i][j].g = *d++;
			}
		}
		break;
	case 3:
		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
				image->ie[i][j].r = *d++;
				image->ie[i][j].g = *d++;
				image->ie[i][j].b = *d++;
			}
		}
		break;
	case 4:
		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
				image->ie[i][j].r = *d++;
				image->ie[i][j].g = *d++;
				image->ie[i][j].b = *d++;
				image->ie[i][j].a = *d++;
			}
		}
		break;
	default:
		// Error ?
		g_print("Strange channels: %d\n", channels);
		break;
	}

	g_free(rgn_data);

	return image;
}

// Set image to gimp
int image_togimp(IMAGE * image, GimpDrawable * drawable, int x, int y, int width, int height)
{
	gint i, j;
	gint channels;
	GimpPixelRgn output_rgn;
	guchar *rgn_data, *d;

	channels = drawable->bpp;
	gimp_pixel_rgn_init(&output_rgn, drawable, 0, 0, drawable->width, drawable->height, TRUE, TRUE);

	rgn_data = g_new(guchar, height * width * channels);
	if (!rgn_data) {
		g_print("Memory allocate (%d bytes) failure.\n", height * width * channels);
		return -1;
	}

	d = rgn_data;
	switch (channels) {
	case 1:
		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
				*d++ = image->ie[i][j].r;
			}
		}
		break;
	case 2:
		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
				*d++ = image->ie[i][j].r;
				*d++ = image->ie[i][j].g;
			}
		}
		break;
	case 3:
		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
				*d++ = image->ie[i][j].r;
				*d++ = image->ie[i][j].g;
				*d++ = image->ie[i][j].b;
			}
		}
		break;
	case 4:
		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
				*d++ = image->ie[i][j].r;
				*d++ = image->ie[i][j].g;
				*d++ = image->ie[i][j].b;
				*d++ = image->ie[i][j].a;
			}
		}
		break;
	default:
		// Error ?
		g_print("Strange channels: %d\n", channels);
		break;
	}

	gimp_pixel_rgn_set_rect(&output_rgn, rgn_data, x, y, width, height);

	g_free(rgn_data);

	return 0;
}

TENSOR *tensor_fromgimp(GimpDrawable * drawable, int x, int y, int width, int height)
{
	gint i, j, k;
	gint c, channels;
	GimpPixelRgn input_rgn;
	TENSOR *tensor = NULL;
	guchar *rgn_data, *s, *d;

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
	for (c = 0; i < channels; c++) {
		for (i = 0; i < height; i++) {
			k = i * width * channels; // start row i, for HxWxC format
			for (j = 0; j < width; j++) {
				*d++ = s[k + j*channels + c];
			}
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
	guchar *rgn_data, *s, *d;

	channels = drawable->bpp;
	gimp_pixel_rgn_init(&output_rgn, drawable, 0, 0, drawable->width, drawable->height, TRUE, TRUE);

	rgn_data = g_new(guchar, height * width * channels);
	if (!rgn_data) {
		g_print("Memory allocate (%d bytes) failure.\n", height * width * channels);
		return -1;
	}

	d = rgn_data;
	s = tensor->data;
	for (c = 0; i < channels; c++) {
		for (i = 0; i < height; i++) {
			k = i * width * channels; // start row i for HxWxC
			for (j = 0; j < width; j++) {
				d[k + j*channels + c] = *s++;
			}
		}
	}

	gimp_pixel_rgn_set_rect(&output_rgn, rgn_data, x, y, width, height);

	g_free(rgn_data);

	return 0;
}
