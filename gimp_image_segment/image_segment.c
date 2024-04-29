/************************************************************************************
***
*** Copyright 2020-2024 Dell(18588220928@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "gimp_image_segment"

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
        _("Semantic segment, Lightweight !"),
        _("More_Segment_Help"),
        "Dell Du <18588220928@163.com>",
        "Dell Du",
        "2020-2024",
        _("Segment"), "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

    gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/AI/");
}

static GimpPDBStatusType start_image_segment(gint32 drawable_id)
{
    int ret;
    gint channels;
    GeglRectangle rect;
    IMAGE *send_image, *recv_image;
    gint32 image_id;

    gimp_progress_init("Segment ...");

    send_image = vision_get_image_from_drawable(drawable_id, &channels, &rect);
    check_status(image_valid(send_image));

    recv_image = vision_image_service((char*)"image_segment", send_image, NULL);
    check_status(image_valid(recv_image));
    gimp_progress_update(1.0);
    gimp_progress_end();

    // Save segment as new layer
    image_id = gimp_item_get_image(drawable_id);
    check_status(send_image->height == recv_image->height && send_image->width == recv_image->width);
    ret = vision_save_image_as_layer(recv_image, "segment", image_id, 50.0);
    image_destroy(send_image);
    image_destroy(recv_image);
    check_status(ret == RET_OK);

    gimp_display_new(image_id);
    gimp_displays_flush();

    return GIMP_PDB_SUCCESS;
}

static void
run(const gchar* name, gint nparams, const GimpParam* param, gint* nreturn_vals, GimpParam** return_vals)
{
    static GimpParam values[1];
    GimpRunMode run_mode;
    // gint32 image_id;
    gint32 drawable_id;

    // INIT_I18N();

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
    // image_id = param[1].data.d_image;
    drawable_id = param[2].data.d_drawable;

    vision_gimp_plugin_init();
    // gimp_image_convert_precision(image_id, GIMP_COMPONENT_TYPE_U8);

    values[0].data.d_status = start_image_segment(drawable_id);
    if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush();

    vision_gimp_plugin_exit();
}
