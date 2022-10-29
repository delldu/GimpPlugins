/************************************************************************************
***
***	Copyright 2021-2022 Dell(18588220928@163.com), All Rights Reserved.
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


// /usr/share/locale/zh_CN/LC_MESSAGES/gimp20.mo
#include <glib/gi18n.h> // ==> #define _(string) gettext(string)
#define GETTEXT_PACKAGE "gimp20"
#define INIT_I18N() G_STMT_START { \
  bindtextdomain (GETTEXT_PACKAGE, gimp_locale_directory ()); \
  textdomain (GETTEXT_PACKAGE); \
} G_STMT_END


#define GIMP_PLUGIN_VERSION "1.1.0"
#define AI_TASKSET "TAI"
#define AI_WORKSPACE "tmp/"
	int image_saveas_layer(IMAGE * image, char *name_prefix, gint32 image_id);
	IMAGE *image_from_drawable(gint32 drawable_id, gint *channels, GeglRectangle * rect);
	int image_saveto_drawable(IMAGE *image, gint32 drawable_id, gint channels, GeglRectangle * rect);
	int image_saveto_gimp(IMAGE * image, char *name_prefix);
	gint32 get_reference_drawable(gint32 image_id, gint32 drawable_id);
	IMAGE *get_selection_mask(gint32 image_id);

	IMAGE *normal_service(char *taskset_name, char *service_name, IMAGE * send_image, char *addon);
	IMAGE *style_service(char *taskset_name, char *service_name, IMAGE *send_image, IMAGE *style_image, char *addon);

#if defined(__cplusplus)
}
#endif
#endif							// _PLUGIN_H
