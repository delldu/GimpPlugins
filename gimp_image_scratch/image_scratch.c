/************************************************************************************
***
*** Copyright 2020-2022 Dell(18588220928@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "gimp_image_scratch"


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
		{GIMP_PDB_INT32, "run-mode", "Run mode"},
		{GIMP_PDB_IMAGE, "image", "Input image"},
		{GIMP_PDB_DRAWABLE, "drawable", "Input drawable"},
	};

	gimp_install_procedure(PLUG_IN_PROC,
						   "Detect Scratch",
						   "Detect Image Scratch with AI",
						   "Dell Du <18588220928@163.com>",
						   "Dell Du",
						   "2020-2022",
						   _("Scratch Detect"),
						   "RGB*, GRAY*", 
						   GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/AI/Detect/");
}

static IMAGE *scratch_rpc_service(IMAGE * send_image)
{
	return normal_service(AI_TASKSET, "image_scratch", send_image, NULL);
}

static GimpPDBStatusType start_image_scratch(gint32 drawable_id)
{
	IMAGE *send_image, *recv_image;
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;

	gimp_progress_init("Detect ...");
	// send_image = image_from_select(drawable_id, x, y, width, height);
	send_image = image_from_drawable(drawable_id, NULL, NULL);
	if (image_valid(send_image)) {
		recv_image = scratch_rpc_service(send_image);
		gimp_progress_update(1.0);
		if (image_valid(recv_image)) {
			image_saveto_gimp(recv_image, "scratch");
			image_destroy(recv_image);
		} else {
			status = GIMP_PDB_EXECUTION_ERROR;
			g_message("Error: Scratch detect service not available.\n");
		}
		image_destroy(send_image);
	} else {
		status = GIMP_PDB_EXECUTION_ERROR;
		g_message("Error: Scratch source.\n");
	}

	return status;				// GIMP_PDB_SUCCESS;
}

static void
run(const gchar * name, gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals)
{
	static GimpParam values[1];
	GimpRunMode run_mode;
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

	run_mode = (GimpRunMode) param[0].data.d_int32;
	drawable_id = param[2].data.d_drawable;

	gegl_init(NULL, NULL);

	status = start_image_scratch(drawable_id);
	if (run_mode != GIMP_RUN_NONINTERACTIVE)
		gimp_displays_flush();

	gegl_exit();
}
