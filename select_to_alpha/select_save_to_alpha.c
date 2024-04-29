/************************************************************************************
***
*** Copyright 2024 Dell(18588220928@163.com), All Rights Reserved.
***
*** File Author: Dell, Sat 01 Jul 2024 05:09:03 PM CST
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

void test_drag_path(gint32 image_id)
{
    // https://cpp.hotexamples.com/examples/-/-/gimp_vectors_stroke_get_points/cpp-gimp_vectors_stroke_get_points-function-examples.html    

    gint *vectors, num_vectors;
    gint *strokes, num_strokes;

    GimpVectorsStrokeType type;
    gint num_points;
    gdouble *points;
    gboolean closed;

    vectors = gimp_image_get_vectors(image_id, &num_vectors);
    CheckPoint("num_vectors = %d", num_vectors); // num_vectors = 1, one path ?

    for (int i = 0; i < num_vectors; i++) {
        strokes = gimp_vectors_get_strokes(vectors[i], &num_strokes);
        CheckPoint("Path %d, num_strokes = %d", i, num_strokes);

        for (int j = 0; j < num_strokes; j++) {
            type = gimp_vectors_stroke_get_points(vectors[i], strokes[j], &num_points, &points, &closed);
            if (type != GIMP_VECTORS_STROKE_TYPE_BEZIER) {
                g_free(points);
                continue;
            }
            CheckPoint("j = %d, num_points = %d, closed = %s", j, num_points, closed?"True":"False");
            if (num_points >= 12) {
                CheckPoint("Drag Points: (%.2lf, %.2lf) --> (%.2lf, %.2lf)", points[0], points[1], points[6], points[7]);
            } else if (num_points >= 6) {
                CheckPoint("Pin Point: x=%.2lf, y=%.2lf", points[0], points[1]);
            }
            g_free(points);
        }
        g_free(strokes);
    }
    g_free(vectors);
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
        _("Save selection as foregroud/background/unkown to alpha channel"),
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

    // gimp_drawable_width()
    image = vision_get_image_from_drawable(drawable_id, &channels, &rect);
    check_status(image_valid(image));
    image_foreach(image, i, j) {
        // debug ...
        // image->ie[i][j].r = 255;
        // image->ie[i][j].g = 0;
        // image->ie[i][j].b = 0;
        image->ie[i][j].a = alpha_value; //bg - 0,  unkown - 128, fg - 255
    }
    vision_save_image_to_drawable(image, drawable_id, channels, &rect);
    image_destroy(image);

    return GIMP_PDB_SUCCESS;
}

static void
run(const gchar* name, gint nparams, const GimpParam* param, gint* nreturn_vals, GimpParam** return_vals)
{
    static GimpParam values[1];
    GimpRunMode run_mode;
    // gint32 image_id;
    gint32 drawable_id;

    INIT_I18N();

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

    // test_drag_path(image_id);
    // IMAGE *selection_mask = vision_get_selection_mask(drawable_id);
    // image_destroy(select_mask);

    vision_gimp_plugin_init();

    if (run_mode == GIMP_RUN_INTERACTIVE && ! save_as_dialog())
        return;

    if (!gimp_drawable_has_alpha(drawable_id))
        gimp_layer_add_alpha(drawable_id);

    values[0].data.d_status = save_to_alpha(drawable_id);
    if (run_mode != GIMP_RUN_NONINTERACTIVE)
        gimp_displays_flush();

    vision_gimp_plugin_exit();    
}
