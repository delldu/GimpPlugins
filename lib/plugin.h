/************************************************************************************
***
***	Copyright 2021-2024 Dell(18588220928@163.com), All Rights Reserved.
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
#define INIT_I18N()                                               \
    G_STMT_START                                                  \
    {                                                             \
        bindtextdomain(GETTEXT_PACKAGE, gimp_locale_directory()); \
        textdomain(GETTEXT_PACKAGE);                              \
    }                                                             \
    G_STMT_END

#define GIMP_PLUGIN_VERSION "1.1.0"
#define AI_TASKSET "TAI"

int vision_save_image_as_layer(IMAGE* image, char* name_prefix, gint32 image_id, float alpha);
int vision_save_image_to_drawable(IMAGE* image, gint32 drawable_id, gint channels, GeglRectangle* rect);
int vision_save_image_to_gimp(IMAGE* image, char* name_prefix);
gint32 vision_get_reference_drawable(gint32 image_id, gint32 drawable_id);
IMAGE* vision_get_image_from_drawable(gint32 drawable_id, gint* channels, GeglRectangle* rect);
IMAGE *vision_get_selection_mask(gint image_id);

IMAGE* vision_image_service(char* service_name, IMAGE* send_image, char* addon);
char* vision_text_service(char* service_name, IMAGE* send_image, char* addon);
IMAGE* vision_style_service(char* service_name, IMAGE* send_image, IMAGE* style_image);
IMAGE* vision_color_service(char* service_name, IMAGE* send_image, IMAGE* color_image);
IMAGE* vision_json_service(char* service_name, IMAGE* send_image, char *jstr);


int vision_gimp_plugin_init();
int vision_get_cache_filename(char* prefix, int namesize, char* filename);
void vision_gimp_plugin_exit();

gchar* vision_select_image_filename(char *plug_in, char* title);


#if defined(__cplusplus)
}
#endif
#endif // _PLUGIN_H
