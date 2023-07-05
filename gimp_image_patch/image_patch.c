/************************************************************************************
***
*** Copyright 2020-2023 Dell(18588220928@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "gimp_image_patch"

static void query(void);
static void run(const gchar* name,
    gint nparams, const GimpParam* param, gint* nreturn_vals, GimpParam** return_vals);

static GimpPDBStatusType start_image_patch(gint drawable_id)
{
    int i, j;
    gint channels;
    GeglRectangle rect;
    IMAGE *send_image, *recv_image;
    GimpPDBStatusType status = GIMP_PDB_SUCCESS;

    gimp_progress_init("Patch ...");
    recv_image = NULL;
    send_image = image_from_drawable(drawable_id, &channels, &rect);
    // make sure image is not leak masked infomation
    image_foreach(send_image, i, j)
    {
        if (send_image->ie[i][j].a < 230) {
            send_image->ie[i][j].r = send_image->ie[i][j].g = send_image->ie[i][j].b = 0;
            send_image->ie[i][j].a = 0;
        } else {
            send_image->ie[i][j].a = 255;
        }
    }
    if (image_valid(send_image)) {
        recv_image = normal_service((char*)"image_patch", send_image, NULL);
        image_destroy(send_image);
    } else {
        status = GIMP_PDB_EXECUTION_ERROR;
        g_message("Source error, try menu 'Image->Precision->8 bit integer'.\n");
    }

    if (status == GIMP_PDB_SUCCESS && image_valid(recv_image)) {
        // image_saveto_gimp(recv_image, (char *)"patch");
        image_saveto_drawable(recv_image, drawable_id, channels, &rect);
        image_destroy(recv_image);
    } else {
        status = GIMP_PDB_EXECUTION_ERROR;
        g_message("Service not avaible.\n");
    }

    gimp_progress_update(1.0);
    gimp_progress_end();

    return status;
}

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
        { GIMP_PDB_DRAWABLE, "drawable", "Input drawable" }
    };

    gimp_install_procedure(PLUG_IN_PROC,
        _("Inpainting Photo"),
        _("More_Patch_Help"),
        "Dell Du <18588220928@163.com>",
        "Dell Du",
        "2020-2023", _("Patch"), "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

    gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/AI/");
}

static void
run(const gchar* name, gint nparams, const GimpParam* param, gint* nreturn_vals, GimpParam** return_vals)
{
    static GimpParam values[1];
    GimpRunMode run_mode;
    gint32 image_id;
    gint32 drawable_id;
    GimpPDBStatusType status = GIMP_PDB_SUCCESS;

    // INIT_I18N();

    /* Setting mandatory output values */
    *nreturn_vals = 1;
    *return_vals = values;
    values[0].type = GIMP_PDB_STATUS;
    values[0].data.d_status = status;

    if (strcmp(name, PLUG_IN_PROC) != 0 || nparams < 3) {
        values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
        return;
    }

    run_mode = (GimpRunMode)param[0].data.d_int32;
    image_id = param[1].data.d_image;
    drawable_id = param[2].data.d_drawable;

    if (gimp_image_base_type(image_id) != GIMP_RGB)
        gimp_image_convert_rgb(image_id);

    // Add alpha channel !!!
    if (!gimp_drawable_has_alpha(drawable_id))
        gimp_layer_add_alpha(drawable_id);

    image_ai_cache_init();
    // gimp_image_convert_precision(image_id, GIMP_COMPONENT_TYPE_U8);

    status = start_image_patch(drawable_id);
    if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush();

    // Output result for pdb
    values[0].data.d_status = status;

    image_ai_cache_exit();
}
