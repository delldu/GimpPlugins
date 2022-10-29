/************************************************************************************
***
*** Copyright 2020-2022 Dell(18588220928@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "gimp_image_photo_style"


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
						   _("Photo Style"),
						   _("Photo Style"),
						   "Dell Du <18588220928@163.com>",
						   "Dell Du",
						   "2020-2022", 
						   _("Photo Style"),
						   "RGB*, GRAY*", 
						   GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/AI/Transform/");
}

static IMAGE *photo_style_rpc_service(IMAGE *send_image, IMAGE *style_image)
{
	return style_service(AI_TASKSET, "image_photo_style", send_image, style_image, NULL);
}

static GimpPDBStatusType start_image_photo_style(gint32 drawable_id, gint32 style_drawable_id)
{
	gint channels;
	GeglRectangle rect;
	IMAGE *send_image, *style_image, *recv_image;
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;

	gimp_progress_init("Photo style ...");
	send_image = image_from_drawable(drawable_id, &channels, &rect);
	style_image = image_from_drawable(style_drawable_id, &channels, &rect);
	if (image_valid(send_image) && image_valid(style_image)) {
		recv_image = photo_style_rpc_service(send_image, style_image);
		gimp_progress_update(1.0);
		if (image_valid(recv_image)) {
			image_saveto_gimp(recv_image, "photo_style");
			image_destroy(recv_image);
		} else {
			status = GIMP_PDB_EXECUTION_ERROR;
			g_message("Error: Photo style service not available.\n");
		}
	} else {
		status = GIMP_PDB_EXECUTION_ERROR;
		g_message("Error: Photo style source.\n");
	}

	if (image_valid(send_image))
		image_destroy(send_image);
	if (image_valid(style_image))
		image_destroy(style_image);

	return status;				// GIMP_PDB_SUCCESS;
}

static void
run(const gchar * name, gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals)
{
	static GimpParam values[1];
	GimpRunMode run_mode;
	gint32 image_id;
	gint32 drawable_id, style_drawable_id;
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
	image_id = param[1].data.d_image;
	drawable_id = param[2].data.d_drawable;

	style_drawable_id = get_reference_drawable(image_id, drawable_id);
	if (style_drawable_id < 0) {
		g_message("Style Image NOT Found ! Please use menu 'File->Open as layers...' to add one.\n");
		return;
	}

	gegl_init(NULL, NULL);

	status = start_image_photo_style(drawable_id, style_drawable_id);
	if (run_mode != GIMP_RUN_NONINTERACTIVE)
		gimp_displays_flush();

	gegl_exit();
}
