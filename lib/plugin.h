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

	// Get image from Gimp
	IMAGE *image_fromgimp(GimpDrawable * drawable, int x, int y, int width, int height);

	// Set image to gimp
	int image_togimp(IMAGE * image, GimpDrawable * drawable, int x, int y, int width, int height);

	// Get tensor from Gimp
	TENSOR *tensor_fromgimp(GimpDrawable * drawable, int x, int y, int width, int height);

	// Set tensor to gimp
	int tensor_togimp(TENSOR * tensor, GimpDrawable * drawable, int x, int y, int width, int height);


#if defined(__cplusplus)
}
#endif

#endif	// _PLUGIN_H

