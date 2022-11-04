/************************************************************************************
***
*** Copyright 2020-2022 Dell(18588220928@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "gimp_image_zoom4x"

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
						   _("Zoom 4X"),
						   _("Zoom 4X"),
						   "Dell Du <18588220928@163.com>",
						   "Dell Du",
						   "2020-2022", _("Zoom 4X"), "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/AI/Zoom In/");
}

static IMAGE *zoom4x_rpc_service(int id, IMAGE * send_image)
{
	return normal_service("image_zoom4x", id, send_image, NULL);
}

static GimpPDBStatusType start_image_zoom4x(gint32 drawable_id)
{
	gint channels;
	GeglRectangle rect;
	IMAGE *send_image, *recv_image;
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;

	gimp_progress_init("Zoom 4X...");
	recv_image = NULL;
	send_image = image_from_drawable(drawable_id, &channels, &rect);
	if (image_valid(send_image)) {
		recv_image = zoom4x_rpc_service(drawable_id, send_image);
		image_destroy(send_image);
	} else {
		status = GIMP_PDB_EXECUTION_ERROR;
		g_message("Error: Zoom4x source.\n");
	}
	gimp_progress_update(1.0);


	if (image_valid(recv_image)) {
		image_saveto_gimp(recv_image, "zoom4x");
		image_destroy(recv_image);
	} else {
		status = GIMP_PDB_EXECUTION_ERROR;
		g_message("Error: Zoom4x service not available.\n");
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

	image_ai_cache_init();

	status = start_image_zoom4x(drawable_id);
	if (run_mode != GIMP_RUN_NONINTERACTIVE)
		gimp_displays_flush();

	image_ai_cache_exit();
}
