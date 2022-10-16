/************************************************************************************
***
***	Copyright 2021-2022 Dell(18588220928@163.com), All Rights Reserved.
***
***	File Author: Dell, 2021-01-16 12:41:12
***
************************************************************************************/

#include "plugin.h"

// Reference https://github.com/h4k1m0u/gimp-plugin

static IMAGE *image_from_rawdata(gint channels, gint height, gint width, guchar * d)
{
	int i, j;
	IMAGE *image;

	if (channels < 1 || channels > 4) {
		syslog_error("channels %d is not in [1-4]", channels);
		return NULL;
	}

	image = image_create(height, width);
	CHECK_IMAGE(image);

	switch (channels) {
	case 1:
		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
				image->ie[i][j].r = *d++;
				image->ie[i][j].g = image->ie[i][j].r;
				image->ie[i][j].b = image->ie[i][j].r;
				image->ie[i][j].a = 255;
			}
		}
		break;
	case 2:
		// Mono Gray + Alpha
		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
				image->ie[i][j].r = *d++;
				image->ie[i][j].g = image->ie[i][j].r;
				image->ie[i][j].b = image->ie[i][j].r;
				image->ie[i][j].a = *d++;
			}
		}
		break;
	case 3:
		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
				image->ie[i][j].r = *d++;
				image->ie[i][j].g = *d++;
				image->ie[i][j].b = *d++;
				image->ie[i][j].a = 255;
			}
		}
		break;
	case 4:
		// faster
		memcpy(image->base, d, height * width * 4 * sizeof(BYTE));
		break;
	default:
		break;
	}

	return image;
}

static int image_to_rawdata(IMAGE * image, gint channels, gint height, gint width, guchar * d)
{
	int i, j;

	check_image(image);
	if (channels < 1 || channels > 4) {
		syslog_error("channels %d is not in [1-4]", channels);
		return RET_ERROR;
	}

	switch (channels) {
	case 1:
		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
				*d++ = image->ie[i][j].r;
			}
		}
		break;
	case 2:
		// Mono Gray + Alpha
		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
				*d++ = image->ie[i][j].r;
				*d++ = image->ie[i][j].a;
			}
		}
		break;
	case 3:
		for (i = 0; i < height; i++) {
			for (j = 0; j < width; j++) {
				*d++ = image->ie[i][j].r;
				*d++ = image->ie[i][j].g;
				*d++ = image->ie[i][j].b;
			}
		}
		break;
	case 4:
		// faster
		memcpy(d, image->base, height * width * 4 * sizeof(BYTE));
		break;
	default:
		break;
	}

	return RET_OK;
}


IMAGE *normal_service(char *taskset_name, char *service_name, IMAGE * send_image, char *addon)
{
	TASKARG taska;
	TASKSET *tasks;
	TIME start_time, wait_time;
	IMAGE *recv_image = NULL;
	char input_file[256], output_file[256], command[TASK_BUFFER_LEN], home_workspace[256];

	CHECK_IMAGE(send_image);

	snprintf(home_workspace, sizeof(home_workspace), "%s/%s", getenv("HOME"), AI_WORKSPACE);
	// ==> getenv("HOME") = /home/dell/snap/gimp/380/

	make_dir(home_workspace);
	get_temp_fname(home_workspace, ".png", input_file, sizeof(input_file));
	get_temp_fname(home_workspace, ".png", output_file, sizeof(output_file));

	image_save(send_image, input_file);

	if (addon) {
		snprintf(command, sizeof(command), "%s(input_file=%s,%s,output_file=%s)",
				 service_name, input_file, addon, output_file);
	} else {
		snprintf(command, sizeof(command), "%s(input_file=%s,output_file=%s)", service_name, input_file, output_file);
	}

	tasks = taskset_create(taskset_name);
	if (set_queue_task(tasks, command, &taska) != RET_OK)
		goto failure;

	// Wait time, e.g, 60 seconds
	wait_time = 60 * 1000;
	start_time = time_now();
	while (time_now() - start_time < wait_time) {
		usleep(300 * 1000);		// 300 ms
		if (get_task_state(tasks, taska.key) == 100 && file_exist(output_file))
			break;
		gimp_progress_update((float) (time_now() - start_time) / wait_time * 0.90);
	}
	gimp_progress_update(0.9);
	if (get_task_state(tasks, taska.key) == 100 && file_exist(output_file)) {
		recv_image = image_load(output_file);
	}

	if (getenv("DEBUG") == NULL) {
		unlink(input_file);
		unlink(output_file);
	}

  failure:
	taskset_destroy(tasks);

	return recv_image;
}

IMAGE *image_from_drawable(gint32 drawable_id, gint * channels, GeglRectangle * rect)
{
	guchar *rawdata;
	gint temp_channels;
	const Babl *format;
	const GeglRectangle *temp_rect;
	GeglBuffer *buffer;
	IMAGE *image;

	// Gegl buffer & shadow buffer for reading/writing
	buffer = gimp_drawable_get_buffer(drawable_id);
	format = gimp_drawable_get_format(drawable_id);
	temp_channels = gimp_drawable_bpp(drawable_id);

	// Read all image at once from input buffer
	temp_rect = gegl_buffer_get_extent(buffer);
	if (temp_rect->width * temp_rect->height < 256) { // 16 * 16
		syslog_error("Image selection size is too small.");
		return NULL;
	}

	rawdata = g_new(guchar, temp_rect->width * temp_rect->height * temp_channels);
	if (rawdata == NULL) {
		syslog_error("allocate memory for rawdata.");
		return NULL;
	}
	gegl_buffer_get(buffer, GEGL_RECTANGLE(temp_rect->x, temp_rect->y, temp_rect->width, temp_rect->height),
					1.0, format, rawdata, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

	// Transform rawdata
	image = image_from_rawdata(temp_channels, temp_rect->height, temp_rect->width, rawdata);
	CHECK_IMAGE(image);

	// Save channels and rect if necessary
	if (channels)
		*channels = temp_channels;
	if (rect)
		memcpy(rect, temp_rect, sizeof(GeglRectangle));

	// Free allocated pointers & buffers
	g_free(rawdata);
	g_object_unref(buffer);

	return image;
}

int image_saveto_drawable(IMAGE * image, gint32 drawable_id, gint channels, GeglRectangle * rect)
{
	guchar *rawdata;
	const Babl *format;
	GeglBuffer *shadow_buffer;

	check_image(image);

	if (image->height != rect->height || image->width != rect->width) {
		syslog_error("image size doesn't match rect.");
		return RET_ERROR;
	}

	rawdata = g_new(guchar, rect->width * rect->height * channels);
	if (rawdata == NULL) {
		syslog_error("allocate memory for rawdata.");
		return RET_ERROR;
	}

	if (image_to_rawdata(image, channels, rect->height, rect->width, rawdata) != RET_OK) {
		syslog_error("image_to_rawdata");
		return RET_ERROR;
	}

	format = gimp_drawable_get_format(drawable_id);
	shadow_buffer = gimp_drawable_get_shadow_buffer(drawable_id);
	// ==> shadow_buffer == drawable->private->shadow

	gegl_buffer_set(shadow_buffer, GEGL_RECTANGLE(rect->x, rect->y, rect->width, rect->height),
					0, format, rawdata, GEGL_AUTO_ROWSTRIDE);
	gimp_progress_update(1.0);

	// Flush required by shadow buffer & merge shadow buffer with drawable & update drawable
	gegl_buffer_flush(shadow_buffer);
	gimp_drawable_merge_shadow(drawable_id, TRUE);
	gimp_drawable_update(drawable_id, rect->x, rect->y, rect->width, rect->height);

	// Free allocated pointers & buffers
	g_free(rawdata);
	g_object_unref(shadow_buffer);

	return RET_OK;
}

#if 0
IMAGE *image_from_select(gint32 drawable_id, int x, int y, int width, int height)
{
	gint channels;
	GimpPixelRgn input_rgn;
	guchar *rgn_data;
	GimpDrawable *drawable;
	IMAGE *image = NULL;

	channels = gimp_drawable_bpp(drawable_id);
	if (channels < 1 || channels > 4) {
		syslog_error("channels %d is not in [1-4]", channels);
		return NULL;
	}

	image = image_create(height, width);
	CHECK_IMAGE(image);

	rgn_data = g_new(guchar, height * width * channels);
	if (!rgn_data) {
		syslog_error("memory allocate (%d bytes).", height * width * channels);
		image_destroy(image);
		return NULL;
	}

	drawable = gimp_drawable_get(drawable_id);
	gimp_pixel_rgn_init(&input_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);
	gimp_pixel_rgn_get_rect(&input_rgn, rgn_data, x, y, width, height);
	image = image_from_rawdata(channels, height, width, rgn_data);
	g_free(rgn_data);
	gimp_drawable_detach(drawable);

	return image;
}

int image_saveto_select(IMAGE * image, gint32 drawable_id, int x, int y, int width, int height)
{
	int ret;
	gint channels;
	GimpPixelRgn output_rgn;
	guchar *rgn_data;
	GimpDrawable *drawable;

	check_image(image);

	channels = gimp_drawable_bpp(drawable_id);
	rgn_data = g_new(guchar, height * width * channels);
	if (!rgn_data) {
		syslog_error("memory allocate (%d bytes).", height * width * channels);
		return RET_ERROR;
	}

	ret = image_to_rawdata(image, channels, height, width, rgn_data);
	if (ret == RET_OK) {
		drawable = gimp_drawable_get(drawable_id);
		gimp_pixel_rgn_init(&output_rgn, drawable, 0, 0, drawable->width, drawable->height, TRUE, FALSE);
		gimp_pixel_rgn_set_rect(&output_rgn, rgn_data, x, y, width, height);
		gimp_drawable_detach(drawable);
	}

	g_free(rgn_data);

	return ret;
}
#endif


int image_saveto_gimp(IMAGE * image, char *name_prefix)
{
	int ret;
	gchar name[64];
	gint32 image_ID, layer_ID;
	GeglRectangle rect;

	check_image(image);
	image_ID = gimp_image_new(image->width, image->height, GIMP_RGB);
	if (image_ID < 0) {
		syslog_error("create gimp image.");
		return RET_ERROR;
	}

	g_snprintf(name, sizeof(name), "%s_%d", name_prefix, image_ID);

	layer_ID = -1;
	layer_ID = gimp_layer_new(image_ID, name, image->width, image->height, GIMP_RGBA_IMAGE, 100.0, GIMP_NORMAL_MODE);

	if (layer_ID > 0) {
		rect.x = 0;
		rect.y = 0;
		rect.height = image->height;
		rect.width = image->width;

		// ret = image_saveto_select(image, (gint32) layer_ID, 0, 0, image->width, image->height);
		ret = image_saveto_drawable(image, layer_ID, 4, &rect);
		if (!gimp_image_insert_layer(image_ID, layer_ID, 0, 0)) {
			syslog_error("insert layer to image.");
			ret = RET_ERROR;
		}
		gimp_display_new(image_ID);

		gimp_displays_flush();
	} else {
		syslog_error("create gimp layer.");
		return RET_ERROR;
	}

	return ret;
}
