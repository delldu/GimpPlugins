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

static IMAGE *clean_rpc_service(IMAGE * send_image, int msgcode)
{
	char addon[256];

	snprintf(addon, sizeof(addon), "sigma=%d", clean_options.strength);
	// TransWeather model
	if (msgcode == IMAGE_CLEAN_SERVICE_WITH_WEATHER)
		return normal_service("image_clean_with_weather", send_image, addon);

	if (msgcode == IMAGE_CLEAN_SERVICE_WITH_GUIDER )
		return normal_service("image_clean_with_guider", send_image, addon);

	return normal_service("image_clean", send_image, addon);
}

static GimpPDBStatusType start_image_clean(gint drawable_id)
{
	gint channels;
	GeglRectangle rect;
	IMAGE *send_image, *recv_image;
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;

	gimp_progress_init("Clean ...");

	send_image = image_from_drawable(drawable_id, &channels, &rect);
	if (image_valid(send_image)) {
		recv_image = clean_rpc_service(send_image, clean_options.method);
		if (image_valid(recv_image)) {
			image_saveto_drawable(recv_image, drawable_id, channels, &rect);
			image_destroy(recv_image);
		} else {
			status = GIMP_PDB_EXECUTION_ERROR;
			g_message("Clean service is not avaible.\n");
		}
		image_destroy(send_image);
	} else {
		status = GIMP_PDB_EXECUTION_ERROR;
		g_message("Error: Clean source(drawable channel is not 1-4 ?).\n");
	}

	return status;
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
		{GIMP_PDB_INT32, "run-mode", "Run mode"},
		{GIMP_PDB_IMAGE, "image", "Input image"},
		{GIMP_PDB_DRAWABLE, "drawable", "Input drawable"},
		{GIMP_PDB_INT32, "method", "Clean method"},
		{GIMP_PDB_INT32, "strength", "Noise level/Clean strength"},
	};

	gimp_install_procedure(PLUG_IN_PROC,
						   "Image Clean",
						   "This plug-in clean image with PAI",
						   "Dell Du <18588220928@163.com>",
						   "Copyright Dell Du <18588220928@163.com>",
						   "2020-2021", "_Clean", "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/Filters/PAI");
}

static void
run(const gchar * name, gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals)
{
	static GimpParam values[1];
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;
	GimpRunMode run_mode;
	gint32 drawable_id;

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

	switch (run_mode) {
	case GIMP_RUN_INTERACTIVE:
		/* Get options last values if needed */
		gimp_get_data(PLUG_IN_PROC, &clean_options);
		if (!clean_dialog())
			return;
		break;

	case GIMP_RUN_NONINTERACTIVE:
		clean_options.method = param[3].data.d_int32;
		clean_options.strength = param[4].data.d_int32;
		break;

	case GIMP_RUN_WITH_LAST_VALS:
		/*  Get options last values if needed  */
		gimp_get_data(PLUG_IN_PROC, &clean_options);
		break;

	default:
		break;
	}

	gegl_init(NULL, NULL);

	status = start_image_clean(drawable_id);
	if (run_mode != GIMP_RUN_NONINTERACTIVE)
		gimp_displays_flush();

	/*  Store data  */
	if (run_mode == GIMP_RUN_INTERACTIVE)
		gimp_set_data(PLUG_IN_PROC, &clean_options, sizeof(CleanOptions));

	// Output result for pdb
	values[0].data.d_status = status;

	// free resources used by gegl
	gegl_exit();
}
