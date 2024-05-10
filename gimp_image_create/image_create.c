/************************************************************************************
***
*** Copyright 2020-2024 Dell(18588220928@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"
#include "create_dialog.h"

#define PLUG_IN_PROC "gimp_image_create"

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
        _("Create photo with stable diffusion, Simple and power"),
        _("More_Create_Help"),
        "Dell Du <18588220928@163.com>",
        "Dell Du",
        "2020-2024", _("_Create ..."), "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

    gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/AI/");
}

static GimpPDBStatusType start_image_redraw(gint32 drawable_id)
{
    char jstr[4096];
    gint channels;
    GeglRectangle rect;
    IMAGE *send_image, *recv_image;

    gimp_progress_init("Create...");
    if (! vision_server_is_running()) {
        return GIMP_PDB_EXECUTION_ERROR;
    }

    send_image = vision_get_image_from_drawable(drawable_id, &channels, &rect);
    check_status(image_valid(send_image));

    // Come from redos.h
    // typedef struct {
    //     // Input
    //     char prompt[2048];
    //     char negative[2048];

    //     // Control
    //     float guide_scale;
    //     float noise_strength;
    //     int seed;
    //     int sample_steps;
    // } SDJson;
    snprintf(jstr, sizeof(jstr), 
        "{\"prompt\":\"%s\", \"negative\":\"%s\", \"guide_scale\":%.2f, \"noise_strength\":%.2f, \"seed\":%d, \"sample_steps\":%d}",
        create_options.prompt, create_options.negative, create_options.model==SDXL_TURBO?1.8:7.5, 
        (float)create_options.strength/100.0, create_options.seed, create_options.sample_steps);

    // model limition ...
    if (create_options.model == SDXL_TURBO) {
        recv_image = vision_json_service((char*)"image_sdxl_create", send_image, jstr);
    }
    else {
        recv_image = vision_json_service((char*)"image_sd21_create", send_image, jstr);
    }

    image_destroy(send_image);
    check_status(image_valid(recv_image));
    // vision_save_image_to_gimp(recv_image, (char*)"image_create");
    gint32 image_id = gimp_item_get_image(drawable_id);
    gimp_selection_none(image_id);
    vision_save_image_as_layer(recv_image, "image_create", image_id, 50.0);
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
    // gint32 image_id;
    gint32 drawable_id;

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

    switch (run_mode) {
    case GIMP_RUN_INTERACTIVE:
        /* Get options last values if needed */
        gimp_get_data(PLUG_IN_PROC, &create_options);
        if (! create_dialog())
            return;
        // Save values for next ...
        gimp_set_data(PLUG_IN_PROC, &create_options, sizeof(create_options));
        break;

    case GIMP_RUN_NONINTERACTIVE:
        gimp_get_data(PLUG_IN_PROC, &create_options);
        break;

    case GIMP_RUN_WITH_LAST_VALS:
        /*  Get options last values if needed  */
        gimp_get_data(PLUG_IN_PROC, &create_options);
        break;

    default:
        break;
    }


    vision_gimp_plugin_init();

    // gimp_image_convert_precision(image_id, GIMP_COMPONENT_TYPE_U8);

    values[0].data.d_status = start_image_redraw(drawable_id);
    if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush();

    vision_gimp_plugin_exit();
}
