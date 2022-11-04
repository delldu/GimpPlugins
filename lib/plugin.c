/************************************************************************************
***
***	Copyright 2021-2022 Dell(18588220928@163.com), All Rights Reserved.
***
***	File Author: Dell, 2021-01-16 12:41:12
***
************************************************************************************/

#include "plugin.h"

#define CACHE_PATH "image_ai_cache"

// Reference https://github.com/h4k1m0u/gimp-plugin

static IMAGE *image_from_rawdata(gint channels, gint height, gint width, guchar * d)
{
	int i, j;
	IMAGE *image;

	if (channels < 1 || channels > 4) {
		syslog_error("Channels %d is not in [1-4]", channels);
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

static int image_saveto_rawdata(IMAGE * image, gint channels, gint height, gint width, guchar * d)
{
	int i, j;

	check_image(image);
	if (channels < 1 || channels > 4) {
		syslog_error("Channels %d is not in [1-4]", channels);
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


IMAGE *normal_service(char *service_name, int id, IMAGE * send_image, char *addon)
{
	TASKARG taska;
	TASKSET *tasks;
	TIME start_time, wait_time;
	IMAGE *recv_image = NULL;
	char input_file[512], output_file[512], command[TASK_BUFFER_LEN];

	CHECK_IMAGE(send_image);

	image_ai_cache_filename("input", id, ".png", sizeof(input_file), input_file);
	image_ai_cache_filename("output", id, ".png", sizeof(output_file), output_file);
	image_save(send_image, input_file);

	if (addon) {
		snprintf(command, sizeof(command), "%s(input_file=%s,%s,output_file=%s)",
				 service_name, input_file, addon, output_file);
	} else {
		snprintf(command, sizeof(command), "%s(input_file=%s,output_file=%s)", service_name, input_file, output_file);
	}

	tasks = taskset_create(AI_TASKSET);
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

	if (getenv("DEBUG") == NULL) {	// NOT denug Mode
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
	const Babl *format;
	GeglBuffer *buffer;
	IMAGE *image;

	if (!gimp_drawable_mask_intersect(drawable_id, &(rect->x), &(rect->y), &(rect->width), &(rect->height))) {
		syslog_error("Call gimp_drawable_mask_intersect()");
		return NULL;
	}
	CheckPoint("Drawable mask: x=%d, y=%d, width=%d, height=%d", rect->x, rect->y, rect->width, rect->height);
	if (rect->width * rect->height < 256) {	// 16 * 16
		syslog_error("Drawable mask size is too small.");
		return NULL;
	}
	// Gegl buffer & shadow buffer for reading/writing
	buffer = gimp_drawable_get_buffer(drawable_id);
	format = gimp_drawable_get_format(drawable_id);
	*channels = gimp_drawable_bpp(drawable_id);

	// Read all image at once from input buffer
	// memcpy(rect,  gegl_buffer_get_extent(buffer), sizeof(GeglRectangle))

	rawdata = g_new(guchar, rect->width * rect->height * (*channels));
	if (rawdata == NULL) {
		syslog_error("Allocate memory for rawdata.");
		return NULL;
	}
	gegl_buffer_get(buffer, GEGL_RECTANGLE(rect->x, rect->y, rect->width, rect->height),
					1.0, format, rawdata, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

	// Transform rawdata
	image = image_from_rawdata(*channels, rect->height, rect->width, rawdata);
	CHECK_IMAGE(image);

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
		syslog_error("Image size doesn't match rect.");
		return RET_ERROR;
	}

	rawdata = g_new(guchar, rect->width * rect->height * channels);
	if (rawdata == NULL) {
		syslog_error("Allocate memory for rawdata.");
		return RET_ERROR;
	}

	if (image_saveto_rawdata(image, channels, rect->height, rect->width, rawdata) != RET_OK) {
		syslog_error("Call image_saveto_rawdata()");
		return RET_ERROR;
	}

	format = gimp_drawable_get_format(drawable_id);
	shadow_buffer = gimp_drawable_get_shadow_buffer(drawable_id);
	// ==> shadow_buffer == drawable->private->shadow

	gegl_buffer_set(shadow_buffer, GEGL_RECTANGLE(rect->x, rect->y, rect->width, rect->height),
					0, format, rawdata, GEGL_AUTO_ROWSTRIDE);

	// Flush required by shadow buffer & merge shadow buffer with drawable & update drawable
	gegl_buffer_flush(shadow_buffer);
	gimp_drawable_merge_shadow(drawable_id, TRUE);
	gimp_drawable_update(drawable_id, rect->x, rect->y, rect->width, rect->height);

	// Free allocated pointers & buffers
	g_free(rawdata);
	g_object_unref(shadow_buffer);

	return RET_OK;
}


static int image_saveto_region(IMAGE * image, gint32 drawable_id, GeglRectangle * rect)
{
	int ret;
	gint channels;
	GimpPixelRgn output_rgn;
	guchar *rgn_data;
	GimpDrawable *drawable;

	check_image(image);

	channels = gimp_drawable_bpp(drawable_id);
	rgn_data = g_new(guchar, rect->height * rect->width * channels);
	if (!rgn_data) {
		syslog_error("Memory allocate (%d bytes).", rect->height * rect->width * channels);
		return RET_ERROR;
	}

	ret = image_saveto_rawdata(image, channels, rect->height, rect->width, rgn_data);
	if (ret == RET_OK) {
		drawable = gimp_drawable_get(drawable_id);
		gimp_pixel_rgn_init(&output_rgn, drawable, 0, 0, drawable->width, drawable->height, TRUE, FALSE);
		gimp_pixel_rgn_set_rect(&output_rgn, rgn_data, rect->x, rect->y, rect->width, rect->height);
		gimp_drawable_detach(drawable);
	}

	g_free(rgn_data);

	return ret;
}


int image_saveto_gimp(IMAGE * image, char *name_prefix)
{
	gint32 image_id;
	int ret = RET_OK;

	check_image(image);
	image_id = gimp_image_new(image->width, image->height, GIMP_RGB);
	if (image_id < 0) {
		syslog_error("Call gimp_image_new().");
		return RET_ERROR;
	}
	ret = image_saveas_layer(image, name_prefix, image_id);
	if (ret == RET_OK) {
		gimp_display_new(image_id);
		gimp_displays_flush();
	} else {
		syslog_error("Call image_saveas_layer().");
	}

	return ret;
}

int image_saveas_layer(IMAGE * image, char *name_prefix, gint32 image_id)
{
	gchar name[64];
	gint32 layer_id;
	GeglRectangle rect;
	int ret = RET_OK;

	check_image(image);
	g_snprintf(name, sizeof(name), "%s_%d", name_prefix, image_id);

	layer_id = gimp_layer_new(image_id, name, image->width, image->height, GIMP_RGBA_IMAGE, 100.0, GIMP_NORMAL_MODE);
	if (layer_id > 0) {
		rect.x = rect.y = 0;
		rect.height = image->height;
		rect.width = image->width;
		ret = image_saveto_region(image, layer_id, &rect);
		if (!gimp_image_insert_layer(image_id, layer_id, 0, 0)) {
			syslog_error("Call gimp_image_insert_layer().");
			ret = RET_ERROR;
		}
	} else {
		syslog_error("Call gimp_layer_new().");
		return RET_ERROR;
	}

	return ret;
}



gint32 get_reference_drawable(gint32 image_id, gint32 drawable_id)
{
	gint *layer_ids = NULL;
	gint i, num_layers, ret = -1;

	layer_ids = gimp_image_get_layers(image_id, &num_layers);
	for (i = 0; i < num_layers; i++) {
		if (layer_ids[i] != drawable_id) {
			ret = layer_ids[i];
			break;
		}
	}

	if (layer_ids)
		g_free(layer_ids);

	return ret;
}

IMAGE *get_selection_mask(gint32 image_id)
{
	// guchar *rawdata;
	// gint temp_channels;
	// const Babl *format;
	// const GeglRectangle *rect;
	gint channels;
	GeglRectangle rect;
	gint32 select_id;

	// GeglBuffer *select_buffer;
	// IMAGE *image;

	/* Get selection channel */
	if (gimp_selection_is_empty(image_id)) {
		return NULL;
	}
	select_id = gimp_image_get_selection(image_id);
	if (select_id < 0) {
		return NULL;
	}

	return image_from_drawable(select_id, &channels, &rect);
	// select_buffer = gimp_drawable_get_buffer(select_id);
	// format = gimp_drawable_get_format(select_id);
	// temp_channels = gimp_drawable_bpp(select_id);

	// // Read all image at once from input buffer
	// rect = gegl_buffer_get_extent(select_buffer);

	// rawdata = g_new(guchar, rect->width * rect->height * temp_channels);
	// if (rawdata == NULL) {
	//  syslog_error("Allocate memory for rawdata.");
	//  return NULL;
	// }
	// gegl_buffer_get(select_buffer, GEGL_RECTANGLE(rect->x, rect->y, rect->width, rect->height),
	//              1.0, format, rawdata, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

	// // Transform rawdata
	// image = image_from_rawdata(temp_channels, rect->height, rect->width, rawdata);
	// CHECK_IMAGE(image);

	// // Free allocated pointers & buffers
	// g_free(rawdata);

	// g_object_unref (select_buffer);

	// return image;
}

IMAGE *style_service(char *service_name, int send_id, IMAGE * send_image, int style_id, IMAGE * style_image)
{
	TASKARG taska;
	TASKSET *tasks;
	TIME start_time, wait_time;
	IMAGE *recv_image = NULL;
	char input_file[512], style_file[512], output_file[512], command[TASK_BUFFER_LEN];

	CHECK_IMAGE(send_image);

	image_ai_cache_filename("input", send_id, ".png", sizeof(input_file), input_file);
	image_ai_cache_filename("style", style_id, ".png", sizeof(style_file), style_file);
	image_ai_cache_filename("output", send_id, ".png", sizeof(output_file), output_file);

	image_save(send_image, input_file);
	image_save(style_image, style_file);

	snprintf(command, sizeof(command), "%s(input_file=%s,style_file=%s,output_file=%s)",
			 service_name, input_file, style_file, output_file);

	tasks = taskset_create(AI_TASKSET);
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
	if (getenv("DEBUG") == NULL) {	// NOT denug Mode
		unlink(input_file);
		unlink(style_file);
		unlink(output_file);
	}

  failure:
	taskset_destroy(tasks);

	return recv_image;
}

int image_ai_cache_init()
{
	int ret;
	char image_cache_path[512];

	ret = snprintf(image_cache_path, sizeof(image_cache_path), "%s/%s", getenv("HOME"), CACHE_PATH);
	if (ret > 0) {
		make_dir(image_cache_path);
	}

	gegl_init(NULL, NULL);

	return (ret > 0) ? RET_OK : RET_ERROR;
}

int image_ai_cache_filename(char *prefix, gint32 drawable_id, char *postfix, int namesize, char *filename)
{
	int ret;
	ret = snprintf(filename, namesize - 1, "%s/%s/%s_%d%s", getenv("HOME"), CACHE_PATH, prefix, drawable_id, postfix);

	return (ret > 0) ? RET_OK : RET_ERROR;
}

void image_ai_cache_exit()
{
	gegl_exit();
}
