/************************************************************************************
***
*** Copyright 2020-2022 Dell(18588220928@163.com), All Rights Reserved.
***
*** File Author: Dell, 2020-11-16 12:16:01
***
************************************************************************************/

#include "plugin.h"

#define PLUG_IN_PROC "gimp_image_tanet"

static void query(void);
static void run(const gchar * name,
				gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals);


static char *tanet_rpc_service(int id, IMAGE * send_image)
{
	int size;
	TASKARG taska;
	TASKSET *tasks;
	TIME start_time, wait_time;
	char input_file[512], output_file[512], command[TASK_BUFFER_LEN], *txt = NULL;

	CHECK_IMAGE(send_image);

	get_cache_filename("input", id, ".png", sizeof(input_file), input_file);
	get_cache_filename("output", id, ".txt", sizeof(output_file), output_file);
	image_save(send_image, input_file);

	snprintf(command, sizeof(command), "image_tanet(input_file=%s,output_file=%s)", input_file, output_file);

	tasks = taskset_create(AI_TASKSET);
	if (set_queue_task(tasks, command, &taska) != RET_OK)
		goto failure;

	// wait time, e.g, 30 seconds
	wait_time = 30 * 1000;
	start_time = time_now();
	while (time_now() - start_time < wait_time) {
		usleep(300 * 1000);		// 300 ms
		if (get_task_state(tasks, taska.key) == 100 && file_exist(output_file))
			break;
		gimp_progress_update((float) (time_now() - start_time) / wait_time * 0.90);
	}
	gimp_progress_update(0.9);
	if (get_task_state(tasks, taska.key) == 100 && file_exist(output_file)) {
		txt = file_load(output_file, &size);
	}

  failure:
	taskset_destroy(tasks);

	return txt;
}

static GimpPDBStatusType start_image_tanet(gint32 drawable_id)
{
	int size;
	IMAGE *send_image;
	char *recv_text;
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;
	char output_file[512];

	send_image = NULL;
	recv_text = NULL;

	get_cache_filename("output", drawable_id, ".png", sizeof(output_file), output_file);
	// Get result if cache file exists !!!
	if (file_exist(output_file)) {
		recv_text = file_load(output_file, &size);
	} else {
		gimp_progress_init("Assessment ...");

		// send_image = image_from_select(drawable_id, x, y, width, height);
		send_image = image_from_drawable(drawable_id, NULL, NULL);
		if (image_valid(send_image)) {
			recv_text = tanet_rpc_service(drawable_id, send_image);
			image_destroy(send_image);
		} else {
			status = GIMP_PDB_EXECUTION_ERROR;
			g_message("Error: Aesthetics source.\n");
		}
	}
	if (recv_text != NULL) {
		g_message("Aesthetic Score: %s\n", recv_text);
		free(recv_text);
		gimp_progress_update(1.0);
	} else {
		status = GIMP_PDB_EXECUTION_ERROR;
		g_message("Aesthetics assessment service not avaible.\n");
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
		{GIMP_PDB_DRAWABLE, "drawable", "Input drawable"}
	};

	gimp_install_procedure(PLUG_IN_PROC,
						   _("Aesthetics Assess"),
						   _("Aesthetics Assess"),
						   "Dell Du <18588220928@163.com>",
						   "Dell Du",
						   "2020-2022", 
						   _("AA"),
						   "RGB*, GRAY*",
						   GIMP_PLUGIN, G_N_ELEMENTS(args), 0, args, NULL);

	gimp_plugin_menu_register(PLUG_IN_PROC, "<Image>/AI/");
}

static void
run(const gchar * name, gint nparams, const GimpParam * param, gint * nreturn_vals, GimpParam ** return_vals)
{
	static GimpParam values[1];
	GimpPDBStatusType status = GIMP_PDB_SUCCESS;
	GimpRunMode run_mode;
	gint32 drawable_id;

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

	status = start_image_tanet(drawable_id);
	if (run_mode != GIMP_RUN_NONINTERACTIVE)
		gimp_displays_flush();

	// Output result for pdb
	values[0].data.d_status = status;

	gegl_exit();
}