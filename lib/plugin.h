/************************************************************************************
***
***	Copyright 2021-2022 Dell(18588220928g@163.com), All Rights Reserved.
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
#include <redos/redos.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#define GIMP_PLUGIN_VERSION "1.1.0"
#define PAI_TASKSET "PAI"
#define TAI_TASKSET "TAI"
#define AI_WORKSPACE "tmp/"

	IMAGE *image_from_drawable(gint32 drawable_id, gint * channels, GeglRectangle * rect);
	int image_saveto_drawable(IMAGE * image, gint32 drawable_id, gint channels, GeglRectangle * rect);

	IMAGE *image_from_select(gint32 drawable_id, int x, int y, int width, int height);
	int image_saveto_select(IMAGE * image, gint32 drawable_id, int x, int y, int width, int height);

	int create_gimp_image(IMAGE * image, char *name_prefix);

	int image_display(IMAGE * image, gchar * name_prefix);

	IMAGE *normal_service(char *taskset_name, char *service_name, IMAGE * send_image, char *addon);
#if defined(__cplusplus)
}
#endif
#endif							// _PLUGIN_H
