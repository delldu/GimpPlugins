/************************************************************************************
***
***	Copyright 2021-2022 Dell(18588220928@163.com), All Rights Reserved.
***
***	File Author: Dell, 2021-04-26 11:24:10
***
************************************************************************************/


#include "plugin.h"

#define IMAGE_CLEAN_SERVICE 0x01
#define IMAGE_CLEAN_SERVICE_WITH_WEATHER 0x02
#define IMAGE_CLEAN_SERVICE_WITH_GUIDE 0x03

typedef struct {
	gint32 method;
	gint32 strength;			// Noise level
} CleanOptions;

/* Set up default values for options */
static CleanOptions clean_options = {
	IMAGE_CLEAN_SERVICE,		/* method */
	30							/* strength */
};

#define SCALE_WIDTH        256
#define SPIN_BUTTON_WIDTH   96

gboolean clean_dialog()
{
	GtkWidget *dialog;
	GtkWidget *main_vbox;
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *combo;
	GtkObject *adjustment;
	gboolean run;

	gimp_ui_init("clean", FALSE);

	dialog = gimp_dialog_new("Image Clean", "clean",
							 NULL, 0,
							 gimp_standard_help_func, "plug-in-gimp_image_clean",
							 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	// gtk_widget_set_size_request(dialog, 640, 480);

	// Create Main - Vbox
	// GtkWidget * gtk_vbox_new (gboolean homogeneous, gint spacing);   
	main_vbox = gtk_vbox_new(FALSE, 8);
	gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 2);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), main_vbox);

	// Create method frame and add to main_vbox
	frame = gimp_frame_new("Clean Parameters");
	// void gtk_box_pack_start (GtkBox *box, GtkWidget *child, gboolean expand, gboolean fill, guint padding);  
	gtk_box_pack_start(GTK_BOX(main_vbox), frame, FALSE, FALSE, 8);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 2);
	// gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_OUT);      
	gtk_widget_show(frame);

	// Create table
	// GtkWidget *gtk_table_new (guint rows, guint columns, gboolean homogeneous);  
	table = gtk_table_new(2, 2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(table), 2);
	gtk_table_set_row_spacings(GTK_TABLE(table), 2);
	gtk_container_add(GTK_CONTAINER(frame), table);
	gtk_widget_show(table);

	// GtkWidget * gimp_int_combo_box_new (const gchar *first_label, gint first_value, ...)
	combo = gimp_int_combo_box_new("Deep Cleaning", IMAGE_CLEAN_SERVICE,
								   // "CUDA BM3d", IMAGE_CLEAN_SERVICE_WITH_BM3D,
								   "Guided Filter", IMAGE_CLEAN_SERVICE_WITH_GUIDE,
								   "Clean Weather", IMAGE_CLEAN_SERVICE_WITH_WEATHER, NULL);
	gimp_int_combo_box_set_active(GIMP_INT_COMBO_BOX(combo), clean_options.method);
	g_signal_connect(combo, "changed", G_CALLBACK(gimp_int_combo_box_get_active), &clean_options.method);

	// GtkWidget *gimp_table_attach_aligned(GtkTable *table, gint column, gint row,
	//                   const gchar *label_text, gfloat xalign, gfloat yalign,
	//                   GtkWidget *widget, gint colspan, gboolean left_align);
	gimp_table_attach_aligned(GTK_TABLE(table), 0, 0, "Method: ", 0.0, 0.5, combo, 1, FALSE);

	// GtkObject *gimp_scale_entry_new(GtkTable *table, gint column, gint row,
	//  const gchar *text, gint scale_width, gint spinbutton_width,
	//  gdouble value, gdouble lower, gdouble upper,
	//  gdouble step_increment, gdouble  page_increment,
	//  guint digits,
	//  gboolean constrain,
	//  gdouble unconstrained_lower,
	//  gdouble unconstrained_upper,
	//  const gchar *tooltip, const gchar *help_id);

	adjustment = gimp_scale_entry_new(GTK_TABLE(table), 0, 1,
									  "Strength: ", SCALE_WIDTH, SPIN_BUTTON_WIDTH,
									  clean_options.strength, 0, 100 /*[lo, up] */ , 1, 10 /*step */ , 0, TRUE, 0, 0,
									  "Clean Strength", NULL);
	g_signal_connect(adjustment, "value_changed", G_CALLBACK(gimp_int_adjustment_update), &clean_options.strength);

	gtk_widget_show(main_vbox);
	gtk_widget_show(dialog);

	run = (gimp_dialog_run(GIMP_DIALOG(dialog)) == GTK_RESPONSE_OK);

	gtk_widget_destroy(dialog);

	return run;
}
