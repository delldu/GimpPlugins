/************************************************************************************
***
***	Copyright 2021 Dell(18588220928g@163.com), All Rights Reserved.
***
***	File Author: Dell, 2021-01-16 12:40:58
***
************************************************************************************/


#ifndef _PLUGIN_H
#define _PLUGIN_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>

	#include <nimage/image.h>
	#include <libgimp/gimp.h>

	#define IMAGE_CLEAN_SERVICE 0x01010000
	#define IMAGE_CLEAN_SERVICE_WITH_GUIDED_FILTER 0x01010001
	#define IMAGE_CLEAN_SERVICE_WITH_BM3D 0x01010002
	#define IMAGE_CLEAN_URL "tcp://127.0.0.1:9101"

	#define IMAGE_COLOR_SERVICE 0x01020000
	#define IMAGE_COLOR_URL "tcp://127.0.0.1:9102"

	#define IMAGE_NIMA_SERVICE 0x01030000
	#define IMAGE_NIMA_URL "tcp://127.0.0.1:9103"

	#define IMAGE_PATCH_SERVICE 0x01040000
	#define IMAGE_PATCH_URL "tcp://127.0.0.1:9104"

	#define IMAGE_ZOOM_SERVICE 0x01050000
	#define IMAGE_ZOOM_SERVICE_WITH_PAN 0x01050001
	#define IMAGE_ZOOM_URL "tcp://127.0.0.1:9105"

	#define IMAGE_LIGHT_SERVICE 0x01060000
	#define IMAGE_LIGHT_URL "tcp://127.0.0.1:9106"

	#define IMAGE_MATTING_SERVICE 0x01070000
	#define IMAGE_MATTING_URL "tcp://127.0.0.1:9107"


	// Get image
	TENSOR *tensor_fromgimp(GimpDrawable * drawable, int x, int y, int width, int height);

	// Update image
	int tensor_togimp(TENSOR * tensor, GimpDrawable * drawable, int x, int y, int width, int height);

	// Display new image
	int tensor_display(TENSOR *tensor, gchar *name_prefix);

	TENSOR *normal_rpc(int socket, TENSOR *send_tensor, int reqcode);
	TENSOR *resize_rpc(int socket, TENSOR *send_tensor, int reqcode, int multiples);
	TENSOR *zeropad_rpc(int socket, TENSOR *send_tensor, int reqcode, int multiples);

#if defined(__cplusplus)
}
#endif

#endif	// _PLUGIN_H

