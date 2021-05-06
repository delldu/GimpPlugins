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
	#include <vision_service.h>

	#include <libgimp/gimp.h>
	#include <libgimp/gimpui.h>

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

