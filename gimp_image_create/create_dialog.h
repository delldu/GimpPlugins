/************************************************************************************
***
***	Copyright 2021-2024 Dell(18588220928@163.com), All Rights Reserved.
***
***	File Author: Dell, 2021-04-26 11:24:10
***
************************************************************************************/

// ??????????????????????????????????????????????????????????????????????????????????????????????????
// Compact sliders
//     Sliders typically used in GEGL-based filters and tools’ options now have a compact style 
//     by default: they take a lot less space vertically and have a vastly improved interaction model.

#include "plugin.h"

#define SDXL_TURBO 0x01
#define SD21_TURBO 0x02

#define SCALE_WIDTH 0
#define SPIN_BUTTON_WIDTH 128

#define common_positive "best quality, masterpiece, ultra highres"
#define common_negative "worst quality, low quality, low resolution, ugly"

typedef struct {
    gchar prompt[2048];
    gchar negative[1024];
    gint32 strength; // Create strength, 0-100
    gint32 sample_steps;
    gint32 seed;
    gint32 model; // SDXL, SD21 ...
} CreateOptions;

/* Set up default values for options */
static CreateOptions create_options = {
    .prompt[0] = '\0',
    .negative[0] = '\0',
    .strength = 75,
    .sample_steps = 5,
    .seed = -1,
    .model = SDXL_TURBO,
};

static GtkTextBuffer *make_text_editor(GtkWidget* table, int row, int col, char *label, char *text);
static void final_text_edited(char *text, size_t size, GtkTextBuffer *buffer, char *addon_prompt);
static gboolean create_dialog();
// -------------------------------------------------------------------------------------------------------------------

static GtkTextBuffer *make_text_editor(GtkWidget* table, int row, int col, char *label, char *text)
{
    GtkWidget *text_view;
    GtkTextBuffer *buffer;

    text_view = gtk_text_view_new();
    gtk_text_view_set_editable (GTK_TEXT_VIEW (text_view), TRUE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD_CHAR);

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW (text_view));
    gtk_text_buffer_set_text (buffer, text, -1);

    gimp_table_attach_aligned(GTK_TABLE(table), col/*col*/, row/*row*/, label, 
        0.0/*xalign*/, 0.0/*yalign*/, text_view, 1 /*colspan*/, FALSE /*left_align*/);

    return buffer;
}

// Save buffer content to text
static void final_text_edited(char *text, size_t size, GtkTextBuffer *buffer, char *addon_prompt)
{
    gchar *txt;
    GtkTextIter start, end;

    gtk_text_buffer_get_start_iter (buffer, &start);
    gtk_text_buffer_get_end_iter (buffer, &end);

    /* Get the entire buffer text. */
    txt = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

    snprintf(text, size, "%s, %s", txt, addon_prompt);

    g_free (txt);
}


static gboolean create_dialog()
{
    GtkWidget* dialog;
    GtkWidget* main_vbox;
    gboolean run;
    GtkTextBuffer *prompt_buffer, *negative_buffer;

    gimp_ui_init("redraw", FALSE);

    dialog = gimp_dialog_new("Create Parameters", "redraw",
        NULL, 0,
        gimp_standard_help_func, "plug-in-gimp_image_redraw",
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
        GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 1280, 800);

    // GtkWidget * gtk_vbox_new (gboolean homogeneous, gint spacing);
    main_vbox = gtk_vbox_new(FALSE, 8);
    gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 12);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), main_vbox);


    // GtkWidget *gtk_table_new (guint rows, guint columns, gboolean homogeneous);
    GtkWidget *table1 = gtk_table_new(2, 2, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table1), 12);
    gtk_table_set_col_spacings(GTK_TABLE(table1), 2);
    gtk_table_set_row_spacings(GTK_TABLE(table1), 2);
    gtk_container_add(GTK_CONTAINER(main_vbox), table1);
    gtk_widget_show(table1);


    GtkWidget *table3 = gtk_table_new(4, 2, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table3), 12);
    gtk_table_set_col_spacings(GTK_TABLE(table3), 2);
    gtk_table_set_row_spacings(GTK_TABLE(table3), 2);
    gtk_container_add(GTK_CONTAINER(main_vbox), table3);
    gtk_widget_show(table3);

    // Prompts ...
    prompt_buffer =  make_text_editor(table1, 0, 0, "Prompt ", create_options.prompt);
    negative_buffer =  make_text_editor(table1, 1, 0, "Negative ", create_options.negative);

    // Strength
    {
        GtkObject *strength_adjustment = gimp_scale_entry_new(
            GTK_TABLE(table3), 0 /*col*/, 0 /*row*/,
            "Strength ", SCALE_WIDTH, SPIN_BUTTON_WIDTH,
            create_options.strength, 0 /*lower*/, 100 /*upper */, 
            1 /*step_increment*/, 10 /*page_increment*/,
            0 /*digits*/,
            TRUE /*constrain*/, 0 /*unconstrained_lower*/, 0 /*unconstrained_upper*/,
            "Create strength" /*tooltip*/, NULL /*help_id*/);

        g_signal_connect(strength_adjustment, "value_changed", G_CALLBACK(gimp_int_adjustment_update), 
            &create_options.strength);
    }

    // Sample steps
    {
        GtkObject *steps_adjustment = gimp_scale_entry_new(
            GTK_TABLE(table3), 0 /*col*/, 1 /*row*/,
            "Steps ", SCALE_WIDTH, SPIN_BUTTON_WIDTH,
            create_options.sample_steps, 1 /*lower*/, 50 /*upper */, 
            1 /*step_increment*/, 10 /*page_increment*/,
            0 /*digits*/,
            TRUE /*constrain*/, 0 /*unconstrained_lower*/, 0 /*unconstrained_upper*/,
            "Sample steps" /*tooltip*/, NULL /*help_id*/);

        g_signal_connect(steps_adjustment, "value_changed", G_CALLBACK(gimp_int_adjustment_update), 
            &create_options.sample_steps);
    }

    // Seed
    {
        GtkObject *seed_adjustment = gimp_scale_entry_new(
            GTK_TABLE(table3), 0 /*col*/, 2 /*row*/,
            "Seed ", SCALE_WIDTH, SPIN_BUTTON_WIDTH,
            create_options.seed, -1 /*lower*/, 65535 /*upper */, 
            1 /*step_increment*/, 10 /*page_increment*/,
            0 /*digits*/,
            TRUE /*constrain*/, 0 /*unconstrained_lower*/, 0 /*unconstrained_upper*/,
            "Seed" /*tooltip*/, NULL /*help_id*/);

        g_signal_connect(seed_adjustment, "value_changed", G_CALLBACK(gimp_int_adjustment_update), 
            &create_options.seed);
    }

    // Model
    {
        GtkWidget *model_combox = gimp_int_combo_box_new(
            "SDXL -- High quality", SDXL_TURBO,
            "SD21 -- Lower quality", SD21_TURBO, 
            NULL
        );
        gtk_widget_set_size_request(model_combox, 320, 32);

        gimp_int_combo_box_set_active(GIMP_INT_COMBO_BOX(model_combox), create_options.model);
        g_signal_connect(model_combox, "changed", G_CALLBACK(gimp_int_combo_box_get_active), &create_options.model);

        gimp_table_attach_aligned(GTK_TABLE(table3), 0/*col*/, 3/*row*/, "Model ", 
            0.0/*xalign*/, 0.0/*yalign*/, model_combox, 1 /*colspan*/, FALSE /*left_align*/);
    }


    gtk_widget_show(main_vbox);
    gtk_widget_show(dialog);

    run = (gimp_dialog_run(GIMP_DIALOG(dialog)) == GTK_RESPONSE_OK);
    if (run) {
        final_text_edited(create_options.prompt, sizeof(create_options.prompt), prompt_buffer, common_positive);
        final_text_edited(create_options.negative, sizeof(create_options.negative), negative_buffer, common_negative);
    }

    gtk_widget_destroy(dialog);

    return run;
}

