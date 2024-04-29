/************************************************************************************
***
*** Copyright 2020-2024 Dell(18588220928@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "gimp_image_colour"

static void query(void);
static void run(const gchar* name,
    gint nparams, const GimpParam* param, gint* nreturn_vals, GimpParam** return_vals);

// static int is_full_selection(IMAGE* mask)
// {
//     int i, j;
//     image_foreach(mask, i, j)
//     {
//         if (mask->ie[i][j].r != 255)
//             return 0;
//     }
//     return 1;
// }

// static GimpPDBStatusType start_image_colour(gint image_id, gint drawable_id)
static GimpPDBStatusType start_image_colour(gint drawable_id)
{
    gint channels;
    GeglRectangle rect;
    IMAGE *send_image, *recv_image; // , *mask;
    GimpPDBStatusType status = GIMP_PDB_SUCCESS;

    gimp_progress_init("Colour ...");
    recv_image = NULL;
    // mask = vision_get_selection_mask(image_id);
    send_image = vision_get_image_from_drawable(drawable_id, &channels, &rect);
    if (image_valid(send_image)) {
        // // more color weight if pixel is selected (mask marked) ...
        // int i, j;
        // if (mask && mask->height == send_image->height && mask->width == send_image->width && !is_full_selection(mask)) {
        //     image_foreach(mask, i, j)
        //         send_image->ie[i][j].a = (mask->ie[i][j].r > 5) ? 128 : 0; // 5 -- delta
        // } else { //  full selection == None selection !!!
        //     image_foreach(send_image, i, j)
        //         send_image->ie[i][j].a = 0;
        // }

        recv_image = vision_image_service((char*)"image_colour", send_image, NULL);

        if (image_valid(recv_image)) {
            vision_save_image_to_gimp(recv_image, (char*)"colour");
            image_destroy(recv_image);
        } else {
            status = GIMP_PDB_EXECUTION_ERROR;
            g_message("Service not avaible.\n");
        }
        image_destroy(send_image);
    } else {
        status = GIMP_PDB_EXECUTION_ERROR;
        g_message("Source error, try menu 'Image->Precision->8 bit integer'.\n");
    }

    // if (mask)
    //     image_destroy(mask);

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
        _("Color photo via user guide"),
        _("More_Colour_Help"),
        "Dell Du <18588220928@163.com>",
        "Dell Du",
        "2020-2024",
        _("Guide"), "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

    gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/AI/Color/");
}

static void
run(const gchar* name, gint nparams, const GimpParam* param, gint* nreturn_vals, GimpParam** return_vals)
{
    static GimpParam values[1];
    GimpPDBStatusType status = GIMP_PDB_SUCCESS;
    GimpRunMode run_mode;
    gint32 image_id;
    gint32 drawable_id;

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

    if (!gimp_drawable_has_alpha(drawable_id))
        gimp_layer_add_alpha(drawable_id);

    vision_gimp_plugin_init();
    // gimp_image_convert_precision(image_id, GIMP_COMPONENT_TYPE_U8);

    // status = start_image_colour(image_id, drawable_id);
    status = start_image_colour(drawable_id);
    if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush();

    // Output result for pdb
    values[0].data.d_status = status;

    vision_gimp_plugin_exit();
}
