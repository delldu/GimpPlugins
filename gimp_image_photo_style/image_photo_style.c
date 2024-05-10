/************************************************************************************
***
*** Copyright 2020-2024 Dell(18588220928@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "gimp_image_photo_style"

static void query(void);
static void run(const gchar* name,
    gint nparams, const GimpParam* param, gint* nreturn_vals, GimpParam** return_vals);

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
        { GIMP_PDB_INT32, "run-mode", "Run mode" },
        { GIMP_PDB_IMAGE, "image", "Input image" },
        { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" },
    };

    gimp_install_procedure(PLUG_IN_PROC,
        _("Transform your photo like a reality expert"),
        _("More_Photo_Style_Help"),
        "Dell Du <18588220928@163.com>",
        "Dell Du",
        "2020-2024",
        _("Photo Style"), "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

    gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/AI/Transform/");
}

static GimpPDBStatusType start_image_photo_style(gint32 drawable_id, gint32 style_drawable_id)
{
    gint channels;
    GeglRectangle rect;
    IMAGE *send_image, *style_image, *recv_image;

    gimp_progress_init("Photo Style ...");
    if (! vision_server_is_running()) {
        return GIMP_PDB_EXECUTION_ERROR;
    }
    
    style_image = vision_get_image_from_drawable(style_drawable_id, &channels, &rect);
    check_status(image_valid(style_image));

    send_image = vision_get_image_from_drawable(drawable_id, &channels, &rect);
    check_status(image_valid(send_image));

    recv_image = vision_style_service("image_photo_style", send_image, style_image);
    image_destroy(send_image);
    image_destroy(style_image);
    check_status(image_valid(recv_image));
    vision_save_image_to_gimp(recv_image, (char*)"photo_style");
    image_destroy(recv_image);


    gimp_progress_update(1.0);
    gimp_progress_end();

    return GIMP_PDB_SUCCESS;
}


static void
run(const gchar* name, gint nparams, const GimpParam* param, gint* nreturn_vals, GimpParam** return_vals)
{
    static GimpParam values[1];
    GimpRunMode run_mode;
    gint32 image_id;
    gint32 drawable_id, style_drawable_id;

    /* Setting mandatory output values */
    *nreturn_vals = 1;
    *return_vals = values;
    values[0].type = GIMP_PDB_STATUS;
    values[0].data.d_status = GIMP_PDB_SUCCESS;
    if (strcmp(name, PLUG_IN_PROC) != 0 || nparams < 3) {
        values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
        return;
    }

    run_mode = (GimpRunMode)param[0].data.d_int32;
    image_id = param[1].data.d_image;
    drawable_id = param[2].data.d_drawable;

    style_drawable_id = vision_get_reference_drawable(image_id, drawable_id);
    if (style_drawable_id < 0) {
        gchar* filename = vision_select_image_filename(PLUG_IN_PROC, _("Load Style Image"));
        check_avoid(filename != NULL);

        style_drawable_id = gimp_file_load_layer(run_mode, image_id, filename);
        check_avoid(style_drawable_id > 0);
        // gimp_layer_set_opacity(style_drawable_id, 50.0);
        g_free(filename);
        check_avoid(gimp_image_insert_layer(image_id, style_drawable_id, 0, 0));
        // Scale size...
        gimp_layer_scale(style_drawable_id, 
            gimp_drawable_width(drawable_id)/2, gimp_drawable_height(drawable_id)/2, FALSE /*gboolean local_origin*/);

        if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush();
    }
    if (style_drawable_id < 0) {
        g_message("NO Style image, please use menu 'File->Open as Layers...' to add one.\n");
        return;
    }

    vision_gimp_plugin_init();
    // gimp_image_convert_precision(image_id, GIMP_COMPONENT_TYPE_U8);

    values[0].data.d_status = start_image_photo_style(drawable_id, style_drawable_id);
    if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush();

    vision_gimp_plugin_exit();
}
