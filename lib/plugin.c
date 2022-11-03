/************************************************************************************
***
***	Copyright 2021-2022 Dell(18588220928@163.com), All Rights Reserved.
***
***	File Author: Dell, 2021-01-16 12:41:12
***
************************************************************************************/

#include "plugin.h"

// Reference https://github.com/h4k1m0u/gimp-plugin
static int ade20k_class_colors[150] = {
	0x787878, 0xb47878, 0x06e6e6, 0x503232, 0x04c803, 0x787850, 0x8c8c8c, 0xcc05ff, 
	0xe6e6e6, 0x04fa07, 0xe005ff, 0xebff07, 0x96053d, 0x787846, 0x08ff33, 0xff0652, 
	0x8fff8c, 0xccff04, 0xff3307, 0xcc4603, 0x0066c8, 0x3de6fa, 0xff0633, 0x0b66ff, 
	0xff0747, 0xff09e0, 0x0907e6, 0xdcdcdc, 0xff095c, 0x7009ff, 0x08ffd6, 0x07ffe0, 
	0xffb806, 0x0aff47, 0xff290a, 0x07ffff, 0xe0ff08, 0x6608ff, 0xff3d06, 0xffc207, 
	0xff7a08, 0x00ff14, 0xff0829, 0xff0599, 0x0633ff, 0xeb0cff, 0xa09614, 0x00a3ff, 
	0x8c8c8c, 0xfa0a0f, 0x14ff00, 0x1fff00, 0xff1f00, 0xffe000, 0x99ff00, 0x0000ff, 
	0xff4700, 0x00ebff, 0x00adff, 0x1f00ff, 0x0bc8c8, 0xff5200, 0x00fff5, 0x003dff, 
	0x00ff70, 0x00ff85, 0xff0000, 0xffa300, 0xff6600, 0xc2ff00, 0x008fff, 0x33ff00, 
	0x0052ff, 0x00ff29, 0x00ffad, 0x0a00ff, 0xadff00, 0x00ff99, 0xff5c00, 0xff00ff, 
	0xff00f5, 0xff0066, 0xffad00, 0xff0014, 0xffb8b8, 0x001fff, 0x00ff3d, 0x0047ff, 
	0xff00cc, 0x00ffc2, 0x00ff52, 0x000aff, 0x0070ff, 0x3300ff, 0x00c2ff, 0x007aff, 
	0x00ffa3, 0xff9900, 0x00ff0a, 0xff7000, 0x8fff00, 0x5200ff, 0xa3ff00, 0xffeb00, 
	0x08b8aa, 0x8500ff, 0x00ff5c, 0xb800ff, 0xff001f, 0x00b8ff, 0x00d6ff, 0xff0070, 
	0x5cff00, 0x00e0ff, 0x70e0ff, 0x46b8a0, 0xa300ff, 0x9900ff, 0x47ff00, 0xff00a3, 
	0xffcc00, 0xff008f, 0x00ffeb, 0x85ff00, 0xff00eb, 0xf500ff, 0xff007a, 0xfff500, 
	0x0abed4, 0xd6ff00, 0x00ccff, 0x1400ff, 0xffff00, 0x0099ff, 0x0029ff, 0x00ffcc, 
	0x2900ff, 0x29ff00, 0xad00ff, 0x00f5ff, 0x4700ff, 0x7a00ff, 0x00ffb8, 0x005cff, 
	0xb8ff00, 0x0085ff, 0xffd600, 0x19c2c2, 0x66ff00, 0x5c00ff};

static char const *ade20k_class_names[] = {
	"wall", "building", "sky", "floor", "tree", "ceiling", "road", "bed ",
	"windowpane", "grass", "cabinet", "sidewalk", "person", "earth", "door", "table",
	"mountain", "plant", "curtain", "chair", "car", "water", "painting", "sofa",
	"shelf", "house", "sea", "mirror", "rug", "field", "armchair", "seat",
	"fence", "desk", "rock", "wardrobe", "lamp", "bathtub", "railing", "cushion",
	"base", "box", "column", "signboard", "chest of drawers", "counter", "sand", "sink",
	"skyscraper", "fireplace", "refrigerator", "grandstand", "path", "stairs", "runway", "case",
	"pool table", "pillow", "screen door", "stairway", "river", "bridge", "bookcase", "blind",
	"coffee table", "toilet", "flower", "book", "hill", "bench", "countertop", "stove",
	"palm", "kitchen island", "computer", "swivel chair", "boat", "bar", "arcade machine", "hovel",
	"bus", "towel", "light", "truck", "tower", "chandelier", "awning", "streetlight",
	"booth", "television receiver", "airplane", "dirt track", "apparel", "pole", "land", "bannister",
	"escalator", "ottoman", "bottle", "buffet", "poster", "stage", "van", "ship",
	"fountain", "conveyer belt", "canopy", "washer", "plaything", "swimming pool", "stool", "barrel",
	"basket", "waterfall", "tent", "bag", "minibike", "cradle", "oven", "ball",
	"food", "step", "tank", "trade name", "microwave", "pot", "animal", "bicycle",
	"lake", "dishwasher", "screen", "blanket", "sculpture", "hood", "sconce", "vase",
	"traffic light", "tray", "ashcan", "fan", "pier", "crt screen", "plate", "monitor",
	"bulletin board", "shower", "radiator", "glass", "clock", "flag", NULL
};


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

static int image_to_rawdata(IMAGE * image, gint channels, gint height, gint width, guchar * d)
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


IMAGE *normal_service(char *taskset_name, char *service_name, int id, IMAGE * send_image, char *addon)
{
	TASKARG taska;
	TASKSET *tasks;
	TIME start_time, wait_time;
	IMAGE *recv_image = NULL;
	char input_file[512], output_file[512], command[TASK_BUFFER_LEN];

	CHECK_IMAGE(send_image);

	get_cache_filename("input", id, ".png", sizeof(input_file), input_file);
	get_cache_filename("output", id, ".png", sizeof(output_file), output_file);

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
	} else {
		syslog_debug("input_file: %s", input_file);
		syslog_debug("output_file: %s", output_file);
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
		syslog_error("Allocate memory for rawdata.");
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
		syslog_error("Image size doesn't match rect.");
		return RET_ERROR;
	}

	rawdata = g_new(guchar, rect->width * rect->height * channels);
	if (rawdata == NULL) {
		syslog_error("Allocate memory for rawdata.");
		return RET_ERROR;
	}

	if (image_to_rawdata(image, channels, rect->height, rect->width, rawdata) != RET_OK) {
		syslog_error("Call image_to_rawdata()");
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


static int image_saveto_region(IMAGE * image, gint32 drawable_id, GeglRectangle *rect)
{
	int ret;
	gint channels;
	GimpPixelRgn output_rgn;
	guchar *rgn_data;
	GimpDrawable *drawable;

	check_image(image);

	channels = gimp_drawable_bpp(drawable_id);
	rgn_data = g_new(guchar, rect->height * rect->width * channels);
	if (! rgn_data) {
		syslog_error("Memory allocate (%d bytes).", rect->height * rect->width * channels);
		return RET_ERROR;
	}

	ret = image_to_rawdata(image, channels, rect->height, rect->width, rgn_data);
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
		if (! gimp_image_insert_layer(image_id, layer_id, 0, 0)) {
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
	guchar *rawdata;
	gint temp_channels;
	const Babl *format;
	const GeglRectangle *rect;
	gint32 select_id;
	GeglBuffer *select_buffer;
	IMAGE *image;

	/* Get selection channel */
	if (gimp_selection_is_empty(image_id)) {
		return NULL;
	}
	select_id = gimp_image_get_selection(image_id);
	if (select_id < 0) {
		return NULL;
	}

	select_buffer = gimp_drawable_get_buffer(select_id);
	format = gimp_drawable_get_format(select_id);
	temp_channels = gimp_drawable_bpp(select_id);

	// Read all image at once from input buffer
	rect = gegl_buffer_get_extent(select_buffer);

	rawdata = g_new(guchar, rect->width * rect->height * temp_channels);
	if (rawdata == NULL) {
		syslog_error("Allocate memory for rawdata.");
		return NULL;
	}
	gegl_buffer_get(select_buffer, GEGL_RECTANGLE(rect->x, rect->y, rect->width, rect->height),
					1.0, format, rawdata, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

	// Transform rawdata
	image = image_from_rawdata(temp_channels, rect->height, rect->width, rawdata);
	CHECK_IMAGE(image);

	// Free allocated pointers & buffers
	g_free(rawdata);

	g_object_unref (select_buffer);

	return image;
}

IMAGE *style_service(char *taskset_name, char *service_name, int send_id, IMAGE *send_image, 
	int style_id, IMAGE *style_image, char *addon)
{
	TASKARG taska;
	TASKSET *tasks;
	TIME start_time, wait_time;
	IMAGE *recv_image = NULL;
	char input_file[512], style_file[512], output_file[512], command[TASK_BUFFER_LEN];

	CHECK_IMAGE(send_image);

	get_cache_filename2("output", send_id, style_id, ".png", sizeof(output_file), output_file);
	// return result if cache file exists !!!
	if (file_exist(output_file)) {
		return image_load(output_file);
	}

	get_cache_filename("input", send_id, ".png", sizeof(input_file), input_file);
	get_cache_filename("style", style_id, ".png", sizeof(style_file), style_file);

	image_save(send_image, input_file);
	image_save(style_image, style_file);

	if (addon) {
		snprintf(command, sizeof(command), "%s(input_file=%s,style_file=%s,%s,output_file=%s)",
			service_name, input_file, style_file, addon, output_file);
	} else {
		snprintf(command, sizeof(command), "%s(input_file=%s,style_file=%s,output_file=%s)",
			service_name, input_file, style_file, output_file);
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
		unlink(style_file);
		unlink(output_file);
	}

  failure:
	taskset_destroy(tasks);

	return recv_image;
}

int get_segment_color(int c)
{
	int n = ARRAY_SIZE(ade20k_class_colors);
	return ade20k_class_colors[ c % n];
}


char *get_segment_name(int c)
{
	int n = ARRAY_SIZE(ade20k_class_names);
	return (char *)ade20k_class_names[ c % n];
}


int get_cache_filename(char *prefix, int id, char *postfix, int namesize, char *filename)
{
	#define CACHE_PATH "image_ai_cache"

	char image_cache_path[512];
	snprintf(image_cache_path, sizeof(image_cache_path), "%s/%s", getenv("HOME"), CACHE_PATH);

	make_dir(image_cache_path);
	return (snprintf(filename, namesize - 1, "%s/%s_%08d%s", image_cache_path, prefix, id, postfix) > 0)?RET_OK : RET_ERROR;
	
	#undef CACHE_PATH
}

int get_cache_filename2(char *prefix, int id1, int id2, char *postfix, int namesize, char *filename)
{
	#define CACHE_PATH "image_ai_cache"

	char image_cache_path[512];
	snprintf(image_cache_path, sizeof(image_cache_path), "%s/%s", getenv("HOME"), CACHE_PATH);

	make_dir(image_cache_path);
	return (snprintf(filename, namesize - 1, "%s/%s_%08d_%08d%s", image_cache_path, prefix, id1, id2, postfix) > 0)?RET_OK : RET_ERROR;
	
	#undef CACHE_PATH
}
