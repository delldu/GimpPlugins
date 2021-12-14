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
	// #include <vision_service.h>
	#include <redos/redos.h>

	#include <libgimp/gimp.h>
	#include <libgimp/gimpui.h>

	#define GIMP_PLUGIN_VERSION "1.1.0"
	#define PAI_TASKSET "PAI"
	#define PAI_WORKSPACE "/tmp/gimp/"

	IMAGE *image_fromgimp(GimpDrawable * drawable, int x, int y, int width, int height);
	int image_togimp(IMAGE * image, GimpDrawable * drawable, int x, int y, int width, int height);
	int image_display(IMAGE *image, gchar *name_prefix);
	IMAGE *normal_service(char *service_name, IMAGE *send_image, char *addon);
	char *nima_service(IMAGE *send_image);

	#if 0
		// Get image
		TENSOR *tensor_fromgimp(GimpDrawable * drawable, int x, int y, int width, int height);

		// Update image
		int tensor_togimp(TENSOR * tensor, GimpDrawable * drawable, int x, int y, int width, int height);

		// Display new image
		int tensor_display(TENSOR *tensor, gchar *name_prefix);

		TENSOR *normal_rpc(int socket, TENSOR *send_tensor, int reqcode);
		TENSOR *resize_rpc(int socket, TENSOR *send_tensor, int reqcode, int multiples);
		TENSOR *zeropad_rpc(int socket, TENSOR *send_tensor, int reqcode, int multiples);
	#endif

#if defined(__cplusplus)
}
#endif

#endif	// _PLUGIN_H

