/************************************************************************************
***
*** Copyright 2020-2022 Dell(18588220928@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "gimp_image_denoise"


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
						   _("Denoise"),
						   _("Denoise"),
						   "Dell Du <18588220928@163.com>",
						   "Dell Du",
						   "2020-2022", 
						   _("Denoise"),
						   "RGB*, GRAY*", 
						   GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/AI/Clean/");
}

static IMAGE *denoise_rpc_service(int id, IMAGE * send_image)
{
	return normal_service(AI_TASKSET, "image_denoise", id, send_image, NULL);
}

static GimpPDBStatusType start_image_denoise(gint32 drawable_id)
{
	gint channels;
	GeglRectangle rect;
	IMAGE *send_image, *recv_image;
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;
	char output_file[512];

	send_image = NULL;
	recv_image = NULL;

	get_cache_filename("output", drawable_id, ".png", sizeof(output_file), output_file);
	// Get result if cache file exists !!!
	if (file_exist(output_file)) {
		recv_image = image_load(output_file);
	} else {
		gimp_progress_init("Denoise ...");
		send_image = image_from_drawable(drawable_id, &channels, &rect);
		if (image_valid(send_image)) {
			recv_image = denoise_rpc_service(drawable_id, send_image);
			gimp_progress_update(1.0);
			image_destroy(send_image);
		} else {
			status = GIMP_PDB_EXECUTION_ERROR;
			g_message("Error: Denoise source (drawable channel is not 1-4 ?).\n");
		}
	}
	if (image_valid(recv_image)) {
		image_saveto_drawable(recv_image, drawable_id, channels, &rect);
		image_destroy(recv_image);
	} else {
		status = GIMP_PDB_EXECUTION_ERROR;
		g_message("Error: Denoise service is not available.");
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

	status = start_image_denoise(drawable_id);
	if (run_mode != GIMP_RUN_NONINTERACTIVE)
		gimp_displays_flush();

	gegl_exit();
}
