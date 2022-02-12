/************************************************************************************
***
***	Copyright 2021-2022 Dell(18588220928@163.com), All Rights Reserved.
***
***	File Author: Dell, 2021-04-26 11:24:10
***
************************************************************************************/


#include "plugin.h"

#define IMAGE_LIGHT_SERVICE 0x01
#define IMAGE_LIGHT_SERVICE_WITH_CLAHE 0x02

typedef struct {
	gint32 method;
} LightOptions;

/* Set up default values for options */
static LightOptions light_options = {
	IMAGE_LIGHT_SERVICE,		/* method */
};

gboolean light_dialog()
{
	GtkWidget *dialog;
	GtkWidget *main_vbox;
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *combo;
	gboolean run;

	gimp_ui_init("clean", FALSE);

	dialog = gimp_dialog_new("Image Light", "light",
							 NULL, 0,
							 gimp_standard_help_func, "plug-in-gimp_image_light",
							 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	// gtk_widget_set_size_request(dialog, 640, 480);

	// Create Main - Vbox
	// GtkWidget * gtk_vbox_new (gboolean homogeneous, gint spacing);   
	main_vbox = gtk_vbox_new(FALSE, 8);
	gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 2);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), main_vbox);

	// Create method frame and add to main_vbox
	frame = gimp_frame_new("Light Parameters");
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
	combo = gimp_int_combo_box_new("Deep Learning", IMAGE_LIGHT_SERVICE,
								   "Traditonal Method", IMAGE_LIGHT_SERVICE_WITH_CLAHE, NULL);
	gimp_int_combo_box_set_active(GIMP_INT_COMBO_BOX(combo), light_options.method);
	g_signal_connect(combo, "changed", G_CALLBACK(gimp_int_combo_box_get_active), &light_options.method);

	// GtkWidget *gimp_table_attach_aligned(GtkTable *table, gint column, gint row,
	//                   const gchar *label_text, gfloat xalign, gfloat yalign,
	//                   GtkWidget *widget, gint colspan, gboolean left_align);
	gimp_table_attach_aligned(GTK_TABLE(table), 0, 0, "Method: ", 0.0, 0.5, combo, 1, FALSE);


	gtk_widget_show(main_vbox);
	gtk_widget_show(dialog);

	run = (gimp_dialog_run(GIMP_DIALOG(dialog)) == GTK_RESPONSE_OK);

	gtk_widget_destroy(dialog);

	return run;
}
