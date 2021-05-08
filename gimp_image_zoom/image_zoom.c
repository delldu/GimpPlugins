/************************************************************************************
***
*** Copyright 2020-2021 Dell(18588220928g@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "plug-in-gimp_image_zoom"

#include "zoom_dialog.c"


static void query(void);
static void run(const gchar * name,
				gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals);


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
						   "Image Zoom with Deep Learning",
						   "This plug-in zoom image with deep learning technology",
						   "Dell Du <18588220928@163.com>",
						   "Copyright Dell Du <18588220928@163.com>",
						   "2020-2021", "_Zoom4x", "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/Filters/PAI");
}


// remote_procee_call
TENSOR *zoom_rpc(TENSOR *send_tensor, int msgcode)
{
	int socket;
	TENSOR *recv_tensor = NULL;

	CHECK_TENSOR(send_tensor);

	socket = client_open(IMAGE_ZOOM_URL);
	if (socket < 0) {
		g_message("Error: connect server.");
		return NULL;
	}

	recv_tensor = normal_rpc(socket, send_tensor, msgcode);
	client_close(socket);

	return recv_tensor;
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
	drawable_id = param[2].data.d_drawable;
	drawable = gimp_drawable_get(drawable_id);

	switch (run_mode) {
	case GIMP_RUN_INTERACTIVE:
		/* Get options last values if needed */
		gimp_get_data(PLUG_IN_PROC, &zoom_options);
		if (! zoom_dialog())
			return;
		break;

	case GIMP_RUN_NONINTERACTIVE:
		if (nparams != 4)
			status = GIMP_PDB_CALLING_ERROR;
		if (status == GIMP_PDB_SUCCESS) {
			zoom_options.method = param[2].data.d_int32;
		}
		break;

	case GIMP_RUN_WITH_LAST_VALS:
		/*  Get options last values if needed  */
		gimp_get_data(PLUG_IN_PROC, &zoom_options);
		break;

	default:
		break;
	}

	msgcode = DEFINE_SERVICE(zoom_options.method, 0);

	// Support local zoom4x
	x = y = 0;
	if (! gimp_drawable_mask_intersect(drawable_id, &x, &y, &width, &height) || height * width < 64) {
		height = drawable->height;
		width = drawable->width;
	}

	send_tensor = tensor_fromgimp(drawable, x, y, width, height);
	gimp_drawable_detach(drawable);

	if (tensor_valid(send_tensor)) {
		gimp_progress_init("Zoomin ...");

		gimp_progress_update(0.1);

		if (send_tensor->chan == 2 || send_tensor->chan == 4) {
			send_tensor->chan = send_tensor->chan - 1;
			recv_tensor = zoom_rpc(send_tensor, msgcode);
			send_tensor->chan = send_tensor->chan + 1;	// Restore
		} else {
			recv_tensor = zoom_rpc(send_tensor, msgcode);
		}

		gimp_progress_update(0.8);
		if (tensor_valid(recv_tensor)) {
			tensor_display(recv_tensor, "zoom4x");
			tensor_destroy(recv_tensor);
		}
		else {
			g_message("Error: Zoom remote service is not available.");
		}
		tensor_destroy(send_tensor);
		gimp_progress_update(1.0);
	} else {
		g_message("Error: Zoom image is not valid (NO RGB).");
		status = GIMP_PDB_EXECUTION_ERROR;
	}

	// Flush all ?
	gimp_displays_flush();

	// Output result for pdb
	values[0].data.d_status = status;
}

