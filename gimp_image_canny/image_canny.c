/************************************************************************************
***
*** Copyright 2020-2024 Dell(18588220928@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "gimp_image_canny"

static void query(void);
static void run(const gchar* name,
    gint nparams, const GimpParam* param, gint* nreturn_vals, GimpParam** return_vals);
static gboolean canny_dialog();

struct params {
    gint low;
    gint high;
};

struct params global_threshold = {100, 200};


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
        _("Detect edge with canny"),
        _("More_Canny_Help"),
        "Dell Du <18588220928@163.com>",
        "Dell Du",
        "2020-2024",
        _("Canny"), "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

    gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/AI/Detect/");
}

static GimpPDBStatusType start_image_canny(gint32 drawable_id)
{
    int ret;
    gint channels;
    GeglRectangle rect;
    IMAGE *recv_image;

    gimp_progress_init("Detect Edge ...");
    recv_image = vision_get_image_from_drawable(drawable_id, &channels, &rect);
    check_status(image_valid(recv_image));

    ret = shape_bestedge(recv_image, global_threshold.low/255.0, global_threshold.high/255.0);
    check_status(ret == RET_OK);
    
    vision_save_image_to_gimp(recv_image, (char *)"canny");
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
    // image_id = param[1].data.d_image;
    drawable_id = param[2].data.d_drawable;

    if (run_mode == GIMP_RUN_INTERACTIVE && ! canny_dialog())
        return;

    vision_gimp_plugin_init();

    status = start_image_canny(drawable_id);
    if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush();

    vision_gimp_plugin_exit();
}


static gboolean canny_dialog()
{
    GtkWidget *dialog;
    GtkWidget *main_vbox;
    GtkWidget *table;
    GtkWidget *frame;
    GtkWidget *frame_label;
    gboolean   run;

    GtkObject *low_adjustment;
    GtkObject *high_adjustment;

    gimp_ui_init ("Canny", FALSE);

    dialog = gimp_dialog_new ("Canny Edge", "Canny",
                            NULL, 0,
                            gimp_standard_help_func, "canny_detection",
                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,
                            NULL);

    main_vbox = gtk_vbox_new (FALSE, 6);
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
    gtk_widget_show (main_vbox);
    frame = gtk_frame_new (NULL);
    gtk_widget_show (frame);
    gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
    table = gtk_table_new (2 /*rows*/, 1 /*cols*/, FALSE /*homogeneous*/);
    gtk_widget_show (table);
    gtk_container_add (GTK_CONTAINER (frame), table);
    frame_label = gtk_label_new ("<b>Threshold</b>");
    gtk_widget_show (frame_label);
    gtk_frame_set_label_widget (GTK_FRAME (frame), frame_label);
    gtk_label_set_use_markup (GTK_LABEL (frame_label), TRUE);

    low_adjustment = gimp_scale_entry_new(GTK_TABLE(table), 0 /*col*/, 0 /*row*/,
        "    _Low ", 256 /*SCALE_WIDTH */, 96 /*SPIN_BUTTON_WIDTH*/,
        global_threshold.low, 0, 255 /*[lo, up] */, 1, 10 /*step */, 0 /*digits*/,
        TRUE /*constrain*/, 0 /*unconstrained_lower*/, 0 /*unconstrained_upper*/,
        "Low Threshold" /*tooltip*/, NULL /*help_id*/);
    g_signal_connect(low_adjustment, "value_changed", G_CALLBACK(gimp_int_adjustment_update), 
        &global_threshold.low);

    high_adjustment = gimp_scale_entry_new(GTK_TABLE(table), 0 /*col*/, 1 /*row*/,
        "    _High ", 256 /*SCALE_WIDTH */, 96 /*SPIN_BUTTON_WIDTH*/,
        global_threshold.high, 0, 255 /*[lo, up] */, 1, 10 /*step */, 0 /*digits*/,
        TRUE /*constrain*/, 0 /*unconstrained_lower*/, 0 /*unconstrained_upper*/,
        "High Threshold" /*tooltip*/, NULL /*help_id*/);
    g_signal_connect(high_adjustment, "value_changed", G_CALLBACK(gimp_int_adjustment_update), 
        &global_threshold.high);

    gtk_widget_show (dialog);
    run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);
    gtk_widget_destroy (dialog);

    return run;
}
