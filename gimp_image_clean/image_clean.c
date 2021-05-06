/************************************************************************************
***
*** Copyright 2020-2021 Dell(18588220928g@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"
#include "clean_dialog.c"

#define PLUG_IN_PROC "plug-in-gimp_image_clean"

static void query(void);
static void run(const gchar * name,
				gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals);


TENSOR *clean_rpc(TENSOR *send_tensor, int msgcode)
{
	int socket;
	TENSOR *recv_tensor = NULL;

	socket = client_open(IMAGE_CLEAN_URL);
	if (socket < 0) {
		g_message("Error: connect server.");
		return NULL;
	}

	// xxxx8888 Need add clean_options to msgcode here 
	recv_tensor = normal_rpc(socket, send_tensor, msgcode);

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
	int x, y, height, width, msgcode;
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
			clean_options.strength = param[3].data.d_int32;
		}
		break;

	case GIMP_RUN_WITH_LAST_VALS:
		/*  Get options last values if needed  */
		gimp_get_data(PLUG_IN_PROC, &clean_options);
		break;

	default:
		break;
	}

	msgcode = DEFINE_SERVICE(clean_options.method, clean_options.strength);

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
			recv_tensor = clean_rpc(send_tensor, msgcode);
			send_tensor->chan = 2;
		} else {
			recv_tensor = clean_rpc(send_tensor, msgcode);
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


