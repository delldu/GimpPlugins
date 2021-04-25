/************************************************************************************
***
*** Copyright 2020-2021 Dell(18588220928g@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"
#include <libgimp/gimpui.h>

#define PLUG_IN_PROC "plug-in-gimp_image_clean"

static void query(void);
static void run(const gchar * name,
				gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals);

typedef struct {
	gint32 method;
	gint radius;
} CleanOptions;

/* Set up default values for options */
static CleanOptions clean_options = {
	IMAGE_CLEAN_SERVICE,		/* method */
	3							/* radius */
};

static gboolean clean_dialog();


TENSOR *clean_rpc(TENSOR *send_tensor)
{
	int socket;
	TENSOR *recv_tensor = NULL;

	socket = client_open(IMAGE_CLEAN_URL);
	if (socket < 0) {
		g_message("Error: connect server.");
		return NULL;
	}

	recv_tensor = normal_rpc(socket, send_tensor, IMAGE_CLEAN_SERVICE);

	if (! tensor_valid(recv_tensor)) {
		g_message("Error: Remote service is not available.");
	}

	client_close(socket);

	return recv_tensor;
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
		{
		 GIMP_PDB_INT32,
		 "run-mode",
		 "Run mode"},
		{
		 GIMP_PDB_IMAGE,
		 "image",
		 "Input image"},
		{
		 GIMP_PDB_DRAWABLE,
		 "drawable",
		 "Input drawable"}
	};

	gimp_install_procedure(PLUG_IN_PROC,
						   "Image Clean with Deep Learning",
						   "This plug-in clean image with deep learning technology",
						   "Dell Du <18588220928@163.com>",
						   "Copyright Dell Du <18588220928@163.com>",
						   "2020-2021", "_Clean", "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/Filters/AI");
}

static void
run(const gchar * name, gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals)
{
	int x, y, height, width;
	TENSOR *send_tensor, *recv_tensor;

	static GimpParam values[1];
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;
	GimpRunMode run_mode;
	GimpDrawable *drawable;
	gint32 image_id;
	gint32 drawable_id;

	/* Setting mandatory output values */
	*nreturn_vals = 1;
	*return_vals = values;
	values[0].type = GIMP_PDB_STATUS;

	if (strcmp(name, PLUG_IN_PROC) != 0 || nparams < 3) {
		values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
		return;
	}
	values[0].data.d_status = status;

	run_mode = (GimpRunMode)param[0].data.d_int32;
	image_id = param[1].data.d_drawable;
	drawable_id = param[2].data.d_drawable;
	drawable = gimp_drawable_get(drawable_id);


	switch (run_mode) {
	case GIMP_RUN_INTERACTIVE:
		/* Get options last values if needed */
		gimp_get_data(PLUG_IN_PROC, &clean_options);
		if (! clean_dialog())
			return;
		break;

	case GIMP_RUN_NONINTERACTIVE:
		if (nparams != 4)
			status = GIMP_PDB_CALLING_ERROR;
		if (status == GIMP_PDB_SUCCESS) {
			clean_options.method = param[2].data.d_int32;
			clean_options.radius = param[3].data.d_int32;
		}
		break;

	case GIMP_RUN_WITH_LAST_VALS:
		/*  Get options last values if needed  */
		gimp_get_data(PLUG_IN_PROC, &clean_options);
		break;

	default:
		break;
	}

	x = y = 0;
	if (! gimp_drawable_mask_intersect(drawable_id, &x, &y, &width, &height) || height * width  < 64) {
		// Drawable region is too small
		height = drawable->height;
		width = drawable->width;
	}

	// Clean server limited: only accept 4 times tensor, 'crop' is against crashing !!!
	width = width/4; width *= 4;
	height = height/4; height *= 4;

	if (width < 4 || height < 4) {
	    if (run_mode != GIMP_RUN_NONINTERACTIVE)
			g_message("Clean image region is too small.\n");
		gimp_drawable_detach(drawable);
		values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
		return;
	}

	send_tensor = tensor_fromgimp(drawable, x, y, width, height);
	if (tensor_valid(send_tensor)) {
		gimp_progress_init("Clean ...");

		gimp_progress_update(0.1);

		// Clean server accept 1x3xhxw
		if (send_tensor->chan == 2) {
			send_tensor->chan = 1;
			recv_tensor = clean_rpc(send_tensor);
			send_tensor->chan = 2;
		} else {
			recv_tensor = clean_rpc(send_tensor);
		}

		gimp_progress_update(0.8);
		if (tensor_valid(recv_tensor)) {
			TENSOR *final_tensor = tensor_reshape(recv_tensor, 
				send_tensor->batch, send_tensor->chan, send_tensor->height, send_tensor->width);

			if (tensor_valid(final_tensor)) {
				if (send_tensor->chan == 2 || send_tensor->chan == 4)
					tensor_setmask(final_tensor, 1.0);

				gimp_image_undo_group_start(image_id);
				tensor_togimp(final_tensor, drawable, x, y, width, height);
				gimp_image_undo_group_end(image_id);
				tensor_destroy(final_tensor);
			}
			tensor_destroy(recv_tensor);
		}
		else {
		    if (run_mode != GIMP_RUN_NONINTERACTIVE)
				g_message("Clean remote service is not avaible.\n");
		}
		tensor_destroy(send_tensor);
		gimp_progress_update(1.0);
	} else {
	    if (run_mode != GIMP_RUN_NONINTERACTIVE)
			g_message("Clean image is not RGB.\n");
		status = GIMP_PDB_EXECUTION_ERROR;
	}

	// Update modified region
	gimp_drawable_flush(drawable);
	// gimp_drawable_merge_shadow(drawable->drawable_id, TRUE);
	gimp_drawable_update(drawable_id, x, y, width, height);


	// Flush all ?
	gimp_displays_flush();
	gimp_drawable_detach(drawable);

	/*  Finally, set options in the core  */
	if (run_mode == GIMP_RUN_INTERACTIVE)
		gimp_set_data(PLUG_IN_PROC, &clean_options, sizeof(CleanOptions));

	// Output result for pdb
	values[0].data.d_status = status;
}


static gboolean clean_dialog()
{
	GtkWidget *dialog;
	GtkWidget *frame;
	GtkWidget *main_vbox;

	GtkWidget *method_hbox;
	GtkWidget *radius_hbox;

	GtkWidget *method_label;
	GtkWidget *radius_label;
	GtkWidget *method_spinbutton;
	GtkWidget *radius_spinbutton;
	GtkWidget *frame_label;

	GtkObject *method_spinbutton_adj;
	GtkObject *radius_spinbutton_adj;
	gboolean run;

	gimp_ui_init("clean", FALSE);

	dialog = gimp_dialog_new("Image Clean", "clean",
							 NULL, 0,
							 gimp_standard_help_func, PLUG_IN_PROC,
							 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);

	// Create Frame and add to window
	frame = gtk_frame_new(NULL);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 6);

	frame_label = gtk_label_new("<b>Modify</b>");
	gtk_widget_show(frame_label);
	gtk_frame_set_label_widget(GTK_FRAME(frame), frame_label);
	gtk_label_set_use_markup(GTK_LABEL(frame_label), TRUE);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), frame);
	gtk_widget_show(frame);

	// Create main_vbox and add tp frame
	main_vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 12);
	gtk_container_add(GTK_CONTAINER(GTK_FRAME(frame)), main_vbox);
	gtk_widget_show (main_vbox);

	// Create method hbox and pack to main_vbox
	method_hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start (GTK_BOX (main_vbox), method_hbox, TRUE, TRUE, 0);
	gtk_widget_show(method_hbox);

	method_label = gtk_label_new_with_mnemonic("_Method:");
	gtk_widget_show(method_label);
	gtk_box_pack_start(GTK_BOX(method_hbox), method_label, FALSE, FALSE, 6);
	gtk_label_set_justify(GTK_LABEL(method_label), GTK_JUSTIFY_RIGHT);

	method_spinbutton = gimp_spin_button_new(&method_spinbutton_adj, clean_options.method, 1, 32, 1, 1, 1, 5, 0);
	gtk_box_pack_start(GTK_BOX(method_hbox), method_spinbutton, FALSE, FALSE, 0);
	gtk_widget_show(method_spinbutton);

	// // GtkWidget *fixed = gtk_fixed_new();

	// // GtkWidget *combo = gtk_combo_box_new_text();
	// // gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Ubuntu");
	// // gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Mandriva");
	// // gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Fedora");
	// // gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Mint");
	// // gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Gentoo");
	// // gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "Debian");

	// // gtk_fixed_put(GTK_FIXED(fixed), combo, 50, 50);
	// // // gtk_box_pack_start(GTK_BOX(method_hbox), fixed, FALSE, FALSE, 6);
	// // gtk_container_add(GTK_BOX(method_hbox), fixed);

	// Create radius_hbox and packaed to main_vbox
	radius_hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start (GTK_BOX (main_vbox), radius_hbox, TRUE, TRUE, 0);

	gtk_widget_show(radius_hbox);

	radius_label = gtk_label_new_with_mnemonic("_Radius:");
	gtk_widget_show(radius_label);
	gtk_box_pack_start(GTK_BOX(radius_hbox), radius_label, FALSE, FALSE, 6);
	gtk_label_set_justify(GTK_LABEL(radius_label), GTK_JUSTIFY_RIGHT);

	radius_spinbutton = gimp_spin_button_new(&radius_spinbutton_adj, clean_options.radius, 1, 32, 1, 1, 1, 5, 0);
	gtk_box_pack_start(GTK_BOX(radius_hbox), radius_spinbutton, FALSE, FALSE, 0);
	gtk_widget_show(radius_spinbutton);

	// Create connection signal for clean_options
	g_signal_connect(method_spinbutton_adj, "value_changed", G_CALLBACK(gimp_int_adjustment_update), &clean_options.method);
	g_signal_connect(radius_spinbutton_adj, "value_changed", G_CALLBACK(gimp_int_adjustment_update), &clean_options.radius);

	gtk_widget_show(dialog);

	run = (gimp_dialog_run(GIMP_DIALOG(dialog)) == GTK_RESPONSE_OK);

	gtk_widget_destroy(dialog);

	return run;
}
