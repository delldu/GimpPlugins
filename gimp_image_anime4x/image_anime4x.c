/************************************************************************************
***
*** Copyright 2020-2024 Dell(18588220928@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "gimp_image_anime4x"

static guint32 anime_scale_times = 4; // 4x

void toggle_4x_button(GtkRadioButton *button, gpointer user_data)
{
    (void)button; (void)user_data; // avoid compiler complaint
    anime_scale_times = 4;
}

void toggle_3x_button(GtkRadioButton *button, gpointer user_data)
{
    (void)button; (void)user_data; // avoid compiler complaint
    anime_scale_times = 3;
}

void toggle_2x_button(GtkRadioButton *button, gpointer user_data)
{
    (void)button; (void)user_data; // avoid compiler complaint
    anime_scale_times = 2;
}

void toggle_1x_button(GtkRadioButton *button, gpointer user_data)
{
    (void)button; (void)user_data; // avoid compiler complaint
    anime_scale_times = 1;
}

static gboolean anime_scale_times_dialog()
{
    GtkWidget* dialog;
    GtkWidget* vbox;
    GtkWidget *radio1, *radio2, *radio3, *radio4;
    gboolean run;

    gimp_ui_init(PLUG_IN_PROC, FALSE);

    dialog = gimp_dialog_new("Scale Times", "scale_times_for_zoom_in" /*role*/,
        NULL /* parent */, 0,
        NULL /*gimp_standard_help_func*/, NULL /* "" */,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
    // gtk_widget_set_size_request(dialog, 480, 240);

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
    // gtk_box_set_homogeneous (GTK_BOX (vbox), TRUE);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
    gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), vbox, FALSE, FALSE, 0);
    gtk_widget_show (vbox);

    // Create a radio button with a GtkEntry widget
    radio4 = gtk_radio_button_new_with_label(NULL, "4X"); //gtk_radio_button_new (NULL);
    radio3 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio4), "3X");
    radio2 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio4), "2X");
    radio1 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio4), "1X");

    // Pack them into a box, then show all the widgets
    gtk_box_pack_start (GTK_BOX (vbox), radio4, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), radio3, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), radio2, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), radio1, FALSE, FALSE, 0);
    // gtk_container_add (GTK_CONTAINER (dialog), vbox);

    gtk_widget_show_all (dialog);

    g_signal_connect(G_OBJECT(radio4), "toggled", G_CALLBACK (toggle_4x_button), NULL);
    g_signal_connect(G_OBJECT(radio3), "toggled", G_CALLBACK (toggle_3x_button), NULL);
    g_signal_connect(G_OBJECT(radio2), "toggled", G_CALLBACK (toggle_2x_button), NULL);
    g_signal_connect(G_OBJECT(radio1), "toggled", G_CALLBACK (toggle_1x_button), NULL);

    run = (gimp_dialog_run(GIMP_DIALOG(dialog)) == GTK_RESPONSE_OK);

    gtk_widget_destroy(dialog);

    return run;
}


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
        _("Super resolution, Super quality, Beautify Anime"),
        _("More_Anime4x_Help"),
        "Dell Du <18588220928@163.com>",
        "Dell Du",
        "2020-2024", _("Anime Image"), "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

    gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/AI/Beautify");
}

static GimpPDBStatusType start_image_anime4x(gint32 drawable_id)
{
    gint channels;
    GeglRectangle rect;
    IMAGE *send_image, *recv_image;
    GimpPDBStatusType status = GIMP_PDB_SUCCESS;
    char command[256];

    snprintf(command, sizeof(command) - 1, "Zoom %dX ...", anime_scale_times);
    gimp_progress_init(command);
    recv_image = NULL;
    send_image = vision_get_image_from_drawable(drawable_id, &channels, &rect);
    if (image_valid(send_image)) {
        snprintf(command, sizeof(command) - 1, "image_anime%dx", anime_scale_times);
        recv_image = vision_image_service(command, send_image, NULL);
        image_destroy(send_image);
    } else {
        status = GIMP_PDB_EXECUTION_ERROR;
        g_message("Source error, try menu 'Image->Precision->8 bit integer'.\n");
    }

    if (status == GIMP_PDB_SUCCESS && image_valid(recv_image)) {
        snprintf(command, sizeof(command) - 1, "anime%dx", anime_scale_times);
        vision_save_image_to_gimp(recv_image, command);
        image_destroy(recv_image);
    } else {
        status = GIMP_PDB_EXECUTION_ERROR;
        g_message("Service not available.\n");
    }

    gimp_progress_update(1.0);
    gimp_progress_end();

    return status; // GIMP_PDB_SUCCESS;
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

    vision_gimp_plugin_init();

    if (run_mode == GIMP_RUN_INTERACTIVE && ! anime_scale_times_dialog())
        return;

    // gimp_image_convert_precision(image_id, GIMP_COMPONENT_TYPE_U8);

    status = start_image_anime4x(drawable_id);
    if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush();

    vision_gimp_plugin_exit();
}
