/************************************************************************************
***
*** Copyright 2020-2023 Dell(18588220928@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "gimp_image_color"

static void query(void);
static void run(const gchar* name,
    gint nparams, const GimpParam* param, gint* nreturn_vals, GimpParam** return_vals);

static IMAGE* color_rpc_service(IMAGE* send_image, IMAGE* color_image)
{
    TASKARG taska;
    TASKSET* tasks;
    IMAGE* recv_image = NULL;
    TIME start_time, wait_time;
    char input_file[512], color_file[512], output_file[512], command[TASK_BUFFER_LEN];

    CHECK_IMAGE(send_image);

    image_ai_cache_filename((char*)"image_color_input", sizeof(input_file), input_file);
    image_ai_cache_filename((char*)"image_color_color", sizeof(color_file), color_file);
    image_ai_cache_filename((char*)"image_color_output", sizeof(output_file), output_file);

    image_save(send_image, input_file);
    image_save(color_image, color_file);

    snprintf(command, sizeof(command), "image_color(input_file=%s,color_file=%s,output_file=%s)",
        input_file, color_file, output_file);

    tasks = taskset_create(AI_TASKSET);
    if (set_queue_task(tasks, command, &taska) != RET_OK)
        goto failure;

    // wait time, e.g, 30 seconds
    wait_time = 30 * 1000;
    start_time = time_now();
    while (time_now() - start_time < wait_time) {
        usleep(300 * 1000); // 300 ms
        if (get_task_state(tasks, taska.key) == 100 && file_exist(output_file))
            break;
        gimp_progress_update((float)(time_now() - start_time) / wait_time * 0.90);
    }
    gimp_progress_update(0.9);
    if (get_task_state(tasks, taska.key) == 100 && file_exist(output_file)) {
        recv_image = image_load(output_file);
    }

    if (getenv("DEBUG") == NULL) { // Debug mode ? NO !
        unlink(input_file);
        unlink(color_file);
        unlink(output_file);

        delete_task(tasks, taska.key);
    }

failure:
    taskset_destroy(tasks);

    return recv_image;
}

static GimpPDBStatusType start_image_color(gint drawable_id, gint color_drawable_id)
{
    gint channels;
    GeglRectangle rect;
    IMAGE *send_image, *color_image, *recv_image;
    GimpPDBStatusType status = GIMP_PDB_SUCCESS;

    gimp_progress_init("Color ...");
    recv_image = NULL;
    color_image = image_from_drawable(color_drawable_id, &channels, &rect);
    send_image = image_from_drawable(drawable_id, &channels, &rect);
    if (image_valid(send_image) && image_valid(color_image)) {
        recv_image = color_rpc_service(send_image, color_image);
    } else {
        status = GIMP_PDB_EXECUTION_ERROR;
        g_message("Source error, try menu 'Image->Precision->8 bit integer'.\n");
    }

    if (status == GIMP_PDB_SUCCESS && image_valid(recv_image)) {
        image_saveto_gimp(recv_image, (char*)"color");
        image_destroy(recv_image);
    } else {
        status = GIMP_PDB_EXECUTION_ERROR;
        g_message("Service not avaible.\n");
    }
    if (image_valid(send_image))
        image_destroy(send_image);
    if (image_valid(color_image))
        image_destroy(color_image);

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
        _("Coloring Photo via Reference Picture"),
        _("More_Color_Help"),
        "Dell Du <18588220928@163.com>",
        "Dell Du",
        "2020-2023",
        _("Reference Color"), "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

    gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/AI/Color");
}

static void
run(const gchar* name, gint nparams, const GimpParam* param, gint* nreturn_vals, GimpParam** return_vals)
{
    static GimpParam values[1];
    GimpPDBStatusType status = GIMP_PDB_SUCCESS;
    GimpRunMode run_mode;
    gint32 image_id;
    gint32 drawable_id, color_drawable_id;

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

    image_ai_cache_init();
    // gimp_image_convert_precision(image_id, GIMP_COMPONENT_TYPE_U8);

    if (gimp_image_base_type(image_id) != GIMP_RGB)
        gimp_image_convert_rgb(image_id);

    // if (! gimp_drawable_has_alpha(drawable_id))
    //  gimp_layer_add_alpha(drawable_id);

    color_drawable_id = get_reference_drawable(image_id, drawable_id);
    if (color_drawable_id < 0) {
        gchar* filename = select_image_filename(PLUG_IN_PROC, _("Load Reference Color Image"));
        if (filename != NULL) {
            color_drawable_id = gimp_file_load_layer(run_mode, image_id, filename);
            if (color_drawable_id > 0) {
                gimp_layer_set_opacity(color_drawable_id, 50.0);
                if (!gimp_image_insert_layer(image_id, color_drawable_id, 0, 0)) {
                    syslog_error("Call gimp_image_insert_layer().");
                    color_drawable_id = -1; // force set -1 ==> error
                }
                if (run_mode != GIMP_RUN_NONINTERACTIVE)
                    gimp_displays_flush();
            }
            g_free(filename);
        }
    }

    if (color_drawable_id < 0) {
        g_message("No reference color image, please use menu 'File->Open as Layers...' to add one.\n");
        return;
    }

    status = start_image_color(drawable_id, color_drawable_id);
    if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush();

    // Output result for pdb
    values[0].data.d_status = status;

    image_ai_cache_exit();
}
