/************************************************************************************
***
*** Copyright 2020-2021 Dell(18588220928g@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "plug-in-gimp_image_nima"

static void query(void);
static void run(const gchar * name,
				gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals);


static char *nima_service(IMAGE *send_image)
{
	int size;
	TASKARG taska;
	TASKSET *tasks;
	TIME start_time, wait_time;
	char input_file[256], output_file[256], command[TASK_BUFFER_LEN], home_workspace[256], *txt;

	CHECK_IMAGE(send_image);

	txt = NULL;

	snprintf(home_workspace, sizeof(home_workspace), "%s/%s", getenv("HOME"), PAI_WORKSPACE);
	make_dir(home_workspace);
	get_temp_fname(home_workspace, ".png", input_file, sizeof(input_file));
	get_temp_fname(home_workspace, ".txt", output_file, sizeof(output_file));

	image_save(send_image, input_file);

	snprintf(command, sizeof(command), "image_nima(input_file=%s,output_file=%s)", 
		input_file, output_file);

	tasks = taskset_create(PAI_TASKSET);
	if (set_queue_task(tasks, command, &taska) != RET_OK)
		goto failure;

	// wait time, e.g, 30 seconds
	wait_time = 30 * 1000;
	start_time = time_now();
	while (time_now() - start_time < wait_time) {
		usleep(300*1000); // 300 ms
		if (get_task_state(tasks, taska.key) == 100 && file_exist(output_file))
			break;
		gimp_progress_update((float)(time_now() - start_time)/wait_time * 0.90);
	}
	gimp_progress_update(0.9);
	if (get_task_state(tasks, taska.key) == 100 && file_exist(output_file)) {
		txt = file_load(output_file, &size);
	}
	unlink(input_file);
	unlink(output_file);

failure:
	taskset_destroy(tasks);

	return txt;
}

static GimpPDBStatusType do_image_nima(GimpDrawable * drawable)
{
	int x, y, height, width;
	IMAGE *send_image;
	char *recv_text = NULL;
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;

	x = y = 0;
	if (! gimp_drawable_mask_intersect(drawable->drawable_id, &x, &y, &width, &height)) {
		height = drawable->height;
		width = drawable->width;
	}
	if (width < 4 || height < 4) {
		g_message("Select region is too small.\n");
		return GIMP_PDB_EXECUTION_ERROR;
	}

	send_image = image_fromgimp(drawable, x, y, width, height);
	if (image_valid(send_image)) {
		recv_text = nima_service(send_image);
		if (recv_text != NULL) {
			g_message("Quality: %s\n", recv_text);
			free(recv_text);
			gimp_progress_update(1.0);
		} else {
			status = GIMP_PDB_EXECUTION_ERROR;
			g_message("Nima service is not avaible.\n");
		}
		image_destroy(send_image);
	} else {
		status = GIMP_PDB_EXECUTION_ERROR;
		g_message("Error: Nima source(drawable channel is not 1-4 ?).\n");
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
		{ GIMP_PDB_INT32, "run-mode", "Run mode" },
		{ GIMP_PDB_IMAGE, "image", "Input image" },
		{ GIMP_PDB_DRAWABLE, "drawable", "Input drawable" }
	};

	gimp_install_procedure(PLUG_IN_PROC,
						   "Image Nima",
						   "This plug-in nima image with PAI",
						   "Dell Du <18588220928@163.com>",
						   "Copyright Dell Du <18588220928@163.com>",
						   "2020-2021", "_Nima", "RGB*, GRAY*", GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/Filters/PAI");
}

static void
run(const gchar * name, gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals)
{
	static GimpParam values[1];
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;
	GimpRunMode run_mode;
	GimpDrawable *drawable;
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

	run_mode = (GimpRunMode)param[0].data.d_int32;
	drawable_id = param[2].data.d_drawable;
	drawable = gimp_drawable_get(drawable_id);
	if (gimp_drawable_is_rgb(drawable_id) || gimp_drawable_is_gray(drawable_id)) {
		gimp_progress_init("Nima ...");

		status = do_image_nima(drawable);

		if (run_mode != GIMP_RUN_NONINTERACTIVE)
			gimp_displays_flush();
	} else {
		status = GIMP_PDB_EXECUTION_ERROR;
		g_message("Cannot patch on indexed color images.");
	}
	gimp_drawable_detach(drawable);

	// Output result for pdb
	values[0].data.d_status = status;
}
