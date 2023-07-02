/************************************************************************************
***
*** Copyright 2023 Dell(18588220928@163.com), All Rights Reserved.
***
*** File Author: Dell, Sat 01 Jul 2023 05:09:03 PM CST
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "select_save_to_alpha"

static guint8 alpha_value = 128; // unkown area

void toggle_fg_button(GtkRadioButton *button, gpointer user_data)
{
    (void)button; (void)user_data; // avoid compiler complaint
    alpha_value = 255;
}

void toggle_bg_button(GtkRadioButton *button, gpointer user_data)
{
    (void)button; (void)user_data; // avoid compiler complaint
    alpha_value = 0;
}

void toggle_unkown_button(GtkRadioButton *button, gpointer user_data)
{
    (void)button; (void)user_data; // avoid compiler complaint
   alpha_value = 128;
}

gboolean save_as_dialog()
{
    GtkWidget* dialog;
    GtkWidget* vbox;
    GtkWidget *radio1, *radio2, *radio3;
    gboolean run;

    gimp_ui_init(PLUG_IN_PROC, FALSE);

    dialog = gimp_dialog_new("Save As", "select_save_to_alpha" /*role*/,
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
    radio1 = gtk_radio_button_new_with_label(NULL, "Foregroud"); //gtk_radio_button_new (NULL);
    radio2 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio1), "Background");
    radio3 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio1), "Unkown Area");

    // Pack them into a box, then show all the widgets
    gtk_box_pack_start (GTK_BOX (vbox), radio1, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), radio2, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), radio3, FALSE, FALSE, 0);
    // gtk_container_add (GTK_CONTAINER (dialog), vbox);

    gtk_widget_show_all (dialog);

    g_signal_connect(G_OBJECT(radio1), "toggled", G_CALLBACK (toggle_fg_button), NULL);
    g_signal_connect(G_OBJECT(radio2), "toggled", G_CALLBACK (toggle_bg_button), NULL);
    g_signal_connect(G_OBJECT(radio3), "toggled", G_CALLBACK (toggle_unkown_button), NULL);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (radio1), (alpha_value == 255));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (radio2), (alpha_value == 0));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (radio3), (alpha_value == 128));

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
        _("Save the selection as foregroud to alpha channel"),
        _("More_Select_Save_To_Alpha_Help"),
        "Dell Du <18588220928@163.com>",
        "Dell Du",
        "023", _("Save to Alpha"), "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

    gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/Select/");
}


static GimpPDBStatusType save_to_alpha(gint drawable_id)
{
    int i, j;
    gint channels;
    GeglRectangle rect;
    IMAGE *image;
    GimpPDBStatusType status = GIMP_PDB_SUCCESS;

    image = image_from_drawable(drawable_id, &channels, &rect);

    if (image_valid(image)) {
        image_foreach(image, i, j) {
            image->ie[i][j].a = alpha_value; //bg - 0,  unkown - 128, fg - 255
        }
        image_saveto_drawable(image, drawable_id, channels, &rect);
        image_destroy(image);
    } else {
        status = GIMP_PDB_EXECUTION_ERROR;
    }

    return status;
}


static void
run(const gchar* name, gint nparams, const GimpParam* param, gint* nreturn_vals, GimpParam** return_vals)
{
    static GimpParam values[1];
    GimpRunMode run_mode;
    // gint32 image_id;
    gint32 drawable_id;
    GimpPDBStatusType status = GIMP_PDB_SUCCESS;

    INIT_I18N();
    gegl_init(NULL, NULL);

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

    if (run_mode == GIMP_RUN_INTERACTIVE && ! save_as_dialog())
        return;

    if (!gimp_drawable_has_alpha(drawable_id))
        gimp_layer_add_alpha(drawable_id);

    status = save_to_alpha(drawable_id);

    if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush();

    gegl_exit();

    // Output result for pdb
    values[0].data.d_status = status;
}