/************************************************************************************
***
***	Copyright 2021-2024 Dell(18588220928@163.com), All Rights Reserved.
***
***	File Author: Dell, 2021-01-16 12:41:12
***
************************************************************************************/

#include "plugin.h"

#define CACHE_PATH "image_ai_cache"

extern void md5sum(uint8_t* initial_msg, size_t initial_len, uint8_t* digest);

static int image_saveto_rawdata(IMAGE* image, gint channels, gint height, gint width, guchar* d);
static IMAGE* image_from_rawdata(gint channels, gint height, gint width, guchar* d);
static void update_preview_cb(GtkFileChooser *file_chooser, gpointer data);

// ------------------------------------------------------------------------------------------------


// Reference https://github.com/h4k1m0u/gimp-plugin

static IMAGE* image_from_rawdata(gint channels, gint height, gint width, guchar* d)
{
    int i, j;
    IMAGE* image;

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
        memcpy(image->base, d, 4 * height * width);
        break;
    default:
        break;
    }

    return image;
}

static int image_saveto_rawdata(IMAGE* image, gint channels, gint height, gint width, guchar* d)
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
        memcpy(d, image->base, 4 * height * width);
        break;
    default:
        break;
    }

    return RET_OK;
}


IMAGE* vision_get_image_from_drawable(gint32 drawable_id, gint* channels, GeglRectangle* rect)
{
    guchar* rawdata;
    const Babl* format;
    GeglBuffer* buffer;
    IMAGE* image;

    if (!gimp_drawable_mask_intersect(drawable_id, 
        &(rect->x), &(rect->y), &(rect->width), &(rect->height))) {
        syslog_error("Call gimp_drawable_mask_intersect()");
        return NULL;
    }
#if 0 // xxxx8888    
    if (rect->width * rect->height < 256) { // 16 * 16
        syslog_error("Drawable mask size is too small.");
        return NULL;
    }
#endif

    // Gegl buffer & shadow buffer for reading/writing
    buffer = gimp_drawable_get_buffer(drawable_id);
    format = gimp_drawable_get_format(drawable_id);
    *channels = gimp_drawable_bpp(drawable_id);

    // Read all image at once from input buffer
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

int vision_save_image_to_drawable(IMAGE* image, gint32 drawable_id, gint channels, GeglRectangle* rect)
{
    guchar* rawdata;
    const Babl* format;
    GeglBuffer* shadow_buffer;

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
    gimp_drawable_merge_shadow(drawable_id, TRUE); // xxxx8888
    gimp_drawable_update(drawable_id, rect->x, rect->y, rect->width, rect->height);

    // Free allocated pointers & buffers
    g_free(rawdata);
    g_object_unref(shadow_buffer);

    return RET_OK;
}

int vision_save_image_to_gimp(IMAGE* image, char* name_prefix)
{
    gint32 image_id;
    int ret = RET_OK;

    check_image(image);
    image_id = gimp_image_new(image->width, image->height, GIMP_RGB);
    if (image_id < 0) {
        syslog_error("Call gimp_image_new().");
        return RET_ERROR;
    }
    ret = vision_save_image_as_layer(image, name_prefix, image_id, 100.0);
    if (ret == RET_OK) {
        gimp_display_new(image_id);
        gimp_displays_flush();
    } else {
        syslog_error("Call vision_save_image_as_layer().");
    }

    return ret;
}

int vision_save_image_as_layer(IMAGE* image, char* name_prefix, gint32 image_id, float alpha)
{
    gchar name[64];
    gint32 layer_id;
    GeglRectangle rect;
    int ret = RET_ERROR;

    check_image(image);
    g_snprintf(name, sizeof(name), "%s_%d", name_prefix, image_id);

    layer_id = gimp_layer_new(image_id, name, image->width, image->height, GIMP_RGBA_IMAGE, alpha, GIMP_NORMAL_MODE);
    if (layer_id > 0) {
        if (!gimp_image_insert_layer(image_id, layer_id, 0, 0)) {
            syslog_error("Call gimp_image_insert_layer().");
        } else {
            ret = RET_OK;
        }
    } else {
        syslog_error("Call gimp_layer_new().");
        return RET_ERROR;
    }

    if (ret == RET_OK) {
        rect.x = rect.y = 0;
        rect.height = image->height;
        rect.width = image->width;
        ret = vision_save_image_to_drawable(image, layer_id, 4 /*rgba*/, &rect);  
    }

    return ret;
}

gint32 vision_get_reference_drawable(gint32 image_id, gint32 drawable_id)
{
    gint* layer_ids = NULL;
    gint num_layers, ret = -1;

    layer_ids = gimp_image_get_layers(image_id, &num_layers);
    for (int i = 0; i < num_layers; i++) {
        if (layer_ids[i] != drawable_id) {
            ret = layer_ids[i];
            break;
        }
    }

    if (layer_ids)
        g_free(layer_ids);

    return ret;
}

// IMAGE* vision_get_selection_mask(gint32 image_id)
// {
//     gint channels;
//     GeglRectangle rect;
//     gint32 select_id;

//     /* Get selection channel */
//     if (gimp_selection_is_empty(image_id)) {
//         return NULL;
//     }
//     select_id = gimp_image_get_selection(image_id);
//     if (select_id < 0) {
//         return NULL;
//     }

//     return vision_get_image_from_drawable(select_id, &channels, &rect);
// }


int vision_gimp_plugin_init()
{
    int ret;
    char image_cache_path[512];

    ret = snprintf(image_cache_path, sizeof(image_cache_path), "%s/%s", getenv("HOME"), CACHE_PATH);
    if (ret > 0) {
        make_dir(image_cache_path);
    }

    INIT_I18N();
    gegl_init(NULL, NULL);
    // gimp_help_disable_tooltips();

    return (ret > 0) ? RET_OK : RET_ERROR;
}

int vision_get_cache_filename(char* prefix, int namesize, char* filename)
{
    int ret;
    if (strchr(prefix, '.') != NULL) {
        ret = snprintf(filename, namesize - 1, "%s/%s/%s_%d", getenv("HOME"), CACHE_PATH, prefix, getpid());
    } else { // no ext, suppose it is image
        ret = snprintf(filename, namesize - 1, "%s/%s/%s_%d.png", getenv("HOME"), CACHE_PATH, prefix, getpid());
    }
    return (ret > 0) ? RET_OK : RET_ERROR;
}

void vision_gimp_plugin_exit()
{
    gegl_exit();
}

static void update_preview_cb(GtkFileChooser *file_chooser, gpointer data)
{
    GtkWidget *preview;
    char *filename;
    GdkPixbuf *pixbuf;
    gboolean have_preview;

    preview = GTK_WIDGET (data);
    filename = gtk_file_chooser_get_preview_filename(file_chooser);

    pixbuf = gdk_pixbuf_new_from_file_at_size (filename, 256, 256, NULL);
    have_preview = (pixbuf != NULL);
    g_free (filename);

    gtk_image_set_from_pixbuf (GTK_IMAGE (preview), pixbuf);
    if (pixbuf)
        g_object_unref (pixbuf);

    gtk_file_chooser_set_preview_widget_active (file_chooser, have_preview);
}

gchar* vision_select_image_filename(char *plug_in, char* title)
{
    GtkWidget* dialog;
    GtkFileFilter* filter;
    GtkFileChooser* chooser;
    gchar* filename = NULL;

    gimp_ui_init(plug_in, TRUE /*preview*/);

    dialog = gtk_file_chooser_dialog_new(title,
        NULL,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        _("_Cancel"), GTK_RESPONSE_CANCEL,
        _("_Open"), GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    chooser = GTK_FILE_CHOOSER(dialog);

    gtk_window_set_position(GTK_WINDOW(chooser), GTK_WIN_POS_MOUSE);
    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("All image files (*.*)"));
    gtk_file_filter_add_pixbuf_formats(filter);
    gtk_file_chooser_add_filter(chooser, filter);
    // gtk_file_chooser_set_preview_widget_active(chooser, TRUE);

    GtkWidget *preview = gtk_image_new();
    gtk_file_chooser_set_preview_widget(chooser, preview);
    g_signal_connect (chooser, "update-preview", G_CALLBACK(update_preview_cb), preview);

    gtk_widget_show(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        filename = gtk_file_chooser_get_filename(chooser);
        syslog_info("filename is %s", filename);
    }

    gtk_widget_destroy(dialog);

    return filename;
}

IMAGE* vision_image_service(char* service_name, IMAGE* send_image, char* addon)
{
    TASKARG taskarg;
    TASKSET* taskset;
    TIME start_time, wait_time;
    IMAGE* recv_image = NULL;
    char input_file[512], output_file[512], command[TASK_BUFFER_LEN];

    CHECK_IMAGE(send_image);

    snprintf(command, sizeof(command), "%s_input", service_name);
    vision_get_cache_filename(command, sizeof(input_file), input_file);
    snprintf(command, sizeof(command), "%s_output", service_name);
    vision_get_cache_filename(command, sizeof(output_file), output_file);

    image_save(send_image, input_file);

    if (addon) {
        snprintf(command, sizeof(command), "%s(input_file=%s,%s,output_file=%s)",
            service_name, input_file, addon, output_file);
    } else {
        snprintf(command, sizeof(command), "%s(input_file=%s,output_file=%s)", service_name, input_file, output_file);
    }

    taskset = redos_open(GIMP_AI_SERVER);
    if (redos_queue_task(taskset, command, &taskarg) != RET_OK)
        goto failure;

    // Wait time, e.g, 300 seconds
    wait_time = 300 * 1000;
    start_time = time_now();
    while (time_now() - start_time < wait_time) {
        usleep(300 * 1000); // 300 ms
        if (redos_get_state(taskset, taskarg.key) == 100 && file_exist(output_file))
            break;
        gimp_progress_update((float)(time_now() - start_time) / wait_time * 0.90);
    }
    gimp_progress_update(0.9);
    if (redos_get_state(taskset, taskarg.key) == 100 && file_exist(output_file)) {
        recv_image = image_load(output_file);
    }

    if (getenv("DEBUG") == NULL) { // Debug mode ? NO
        unlink(input_file);
        unlink(output_file);
    }
    redos_delete_task(taskset, taskarg.key);

failure:
    redos_close(taskset);

    return recv_image;
}


IMAGE* vision_color_service(char* service_name, IMAGE* send_image, IMAGE* color_image)
{
    TASKARG taskarg;
    TASKSET* taskset;
    IMAGE* recv_image = NULL;
    TIME start_time, wait_time;
    char input_file[512], color_file[512], output_file[512], command[TASK_BUFFER_LEN];

    CHECK_IMAGE(send_image);
    CHECK_IMAGE(color_image);

    snprintf(command, sizeof(command), "%s_input", service_name);
    vision_get_cache_filename(command, sizeof(input_file), input_file);
    snprintf(command, sizeof(command), "%s_color", service_name);
    vision_get_cache_filename(command, sizeof(color_file), color_file);
    snprintf(command, sizeof(command), "%s_output", service_name);
    vision_get_cache_filename(command, sizeof(output_file), output_file);

    image_save(send_image, input_file);
    image_save(color_image, color_file);

    snprintf(command, sizeof(command), "%s(input_file=%s,color_file=%s,output_file=%s)",
        service_name, input_file, color_file, output_file);

    taskset = redos_open(GIMP_AI_SERVER);
    if (redos_queue_task(taskset, command, &taskarg) != RET_OK)
        goto failure;

    // wait time, e.g, 60 seconds
    wait_time = 60 * 1000;
    start_time = time_now();
    while (time_now() - start_time < wait_time) {
        usleep(300 * 1000); // 300 ms
        if (redos_get_state(taskset, taskarg.key) == 100 && file_exist(output_file))
            break;
        gimp_progress_update((float)(time_now() - start_time) / wait_time * 0.90);
    }
    gimp_progress_update(0.9);
    if (redos_get_state(taskset, taskarg.key) == 100 && file_exist(output_file)) {
        recv_image = image_load(output_file);
    }

    if (getenv("DEBUG") == NULL) { // Debug mode ? NO !
        unlink(input_file);
        unlink(color_file);
        unlink(output_file);
    }
    redos_delete_task(taskset, taskarg.key);

failure:
    redos_close(taskset);

    return recv_image;
}


IMAGE* vision_style_service(char* service_name, IMAGE* send_image, IMAGE* style_image)
{
    TASKARG taskarg;
    TASKSET* taskset;
    TIME start_time, wait_time;
    IMAGE* recv_image = NULL;
    char input_file[512], style_file[512], output_file[512], command[TASK_BUFFER_LEN];

    CHECK_IMAGE(send_image);

    snprintf(command, sizeof(command), "%s_input", service_name);
    vision_get_cache_filename(command, sizeof(input_file), input_file);
    snprintf(command, sizeof(command), "%s_style", service_name);
    vision_get_cache_filename(command, sizeof(style_file), style_file);
    snprintf(command, sizeof(command), "%s_output", service_name);
    vision_get_cache_filename(command, sizeof(output_file), output_file);

    image_save(send_image, input_file);
    image_save(style_image, style_file);

    snprintf(command, sizeof(command), "%s(input_file=%s,style_file=%s,output_file=%s)",
        service_name, input_file, style_file, output_file);

    taskset = redos_open(GIMP_AI_SERVER);
    if (redos_queue_task(taskset, command, &taskarg) != RET_OK)
        goto failure;

    // Wait time, e.g, 60 seconds
    wait_time = 60 * 1000;
    start_time = time_now();
    while (time_now() - start_time < wait_time) {
        usleep(300 * 1000); // 300 ms
        if (redos_get_state(taskset, taskarg.key) == 100 && file_exist(output_file))
            break;
        gimp_progress_update((float)(time_now() - start_time) / wait_time * 0.90);
    }
    gimp_progress_update(0.9);
    if (redos_get_state(taskset, taskarg.key) == 100 && file_exist(output_file)) {
        recv_image = image_load(output_file);
    }

    if (getenv("DEBUG") == NULL) { // Debug Mode ? NO
        unlink(input_file);
        unlink(style_file);
        unlink(output_file);
    }
    redos_delete_task(taskset, taskarg.key);

failure:
    redos_close(taskset);

    return recv_image;
}

char* vision_text_service(char* service_name, IMAGE* send_image, char* addon)
{
    TASKARG taskarg;
    TASKSET* taskset;
    TIME start_time, wait_time;
    char input_file[512], output_file[512], command[TASK_BUFFER_LEN], *txt = NULL;

    CHECK_IMAGE(send_image);

    snprintf(command, sizeof(command), "%s_input", service_name);
    vision_get_cache_filename(command, sizeof(input_file), input_file);
    snprintf(command, sizeof(command), "%s_output", service_name);
    vision_get_cache_filename(command, sizeof(output_file), output_file);

    // vision_get_cache_filename((char*)"image_tanet_input", sizeof(input_file), input_file);
    // vision_get_cache_filename((char*)"image_tanet_output", sizeof(output_file), output_file);
    image_save(send_image, input_file);

    if (addon) {
        snprintf(command, sizeof(command), "%s(input_file=%s,%s,output_file=%s)",
            service_name, input_file, addon, output_file);
    } else {
        snprintf(command, sizeof(command), "%s(input_file=%s,output_file=%s)", service_name, input_file, output_file);
    }

    taskset = redos_open(GIMP_AI_SERVER);
    if (redos_queue_task(taskset, command, &taskarg) != RET_OK)
        goto failure;

    // wait time, e.g, 60 seconds
    wait_time = 60 * 1000;
    start_time = time_now();
    while (time_now() - start_time < wait_time) {
        usleep(300 * 1000); // 300 ms
        if (redos_get_state(taskset, taskarg.key) == 100 && file_exist(output_file))
            break;
        gimp_progress_update((float)(time_now() - start_time) / wait_time * 0.90);
    }
    gimp_progress_update(0.9);
    if (redos_get_state(taskset, taskarg.key) == 100 && file_exist(output_file)) {
        int size;
        txt = file_load(output_file, &size);
    }

    if (getenv("DEBUG") == NULL) { // Debug mode ? NO
        unlink(input_file);
        unlink(output_file);
    }
    redos_delete_task(taskset, taskarg.key);

failure:
    redos_close(taskset);

    return txt;
}

IMAGE* vision_json_service(char* service_name, IMAGE* send_image, char *jstr)
{
    TASKARG taskarg;
    TASKSET* taskset;
    IMAGE* recv_image = NULL;
    TIME start_time, wait_time;
    char input_file[512], json_file[512], output_file[512], command[TASK_BUFFER_LEN];

    CHECK_IMAGE(send_image);


    snprintf(command, sizeof(command), "%s_input", service_name);
    vision_get_cache_filename(command, sizeof(input_file), input_file);
    snprintf(command, sizeof(command), "%s_json.txt", service_name);
    vision_get_cache_filename(command, sizeof(json_file), json_file);
    snprintf(command, sizeof(command), "%s_output", service_name);
    vision_get_cache_filename(command, sizeof(output_file), output_file);

    image_save(send_image, input_file);
    file_save(json_file, jstr, strlen(jstr));
    file_chown(json_file, input_file); // model is -rw------- ???

    snprintf(command, sizeof(command), "%s(input_file=%s,json_file=%s,output_file=%s)",
        service_name, input_file, json_file, output_file);

    taskset = redos_open(GIMP_AI_SERVER);
    if (redos_queue_task(taskset, command, &taskarg) != RET_OK)
        goto failure;

    // wait time, e.g, 60 seconds
    wait_time = 60 * 1000;
    start_time = time_now();
    while (time_now() - start_time < wait_time) {
        usleep(300 * 1000); // 300 ms
        if (redos_get_state(taskset, taskarg.key) == 100 && file_exist(output_file))
            break;
        gimp_progress_update((float)(time_now() - start_time) / wait_time * 0.90);
    }
    gimp_progress_update(0.9);
    if (redos_get_state(taskset, taskarg.key) == 100 && file_exist(output_file)) {
        recv_image = image_load(output_file);
    }

    if (getenv("DEBUG") == NULL) { // Debug mode ? NO !
        unlink(input_file);
        unlink(json_file);
        unlink(output_file);
    }
    redos_delete_task(taskset, taskarg.key);

failure:
    redos_close(taskset);

    return recv_image;
}

IMAGE *vision_get_selection_mask(gint image_id)
{
    gint channels, channel_id;
    GeglRectangle rect;
    IMAGE *mask_image;

    channel_id = gimp_selection_save(image_id);
    CheckPoint("image_id = %d, channel_id = %d", image_id, channel_id);
    if (channel_id < 0) {
        syslog_error("Save selection to channel");
        return NULL;
    }

    mask_image = vision_get_image_from_drawable(channel_id, &channels, &rect);
    gimp_image_remove_channel(image_id, channel_id);

    // debug ...
    // if (image_valid(mask_image)) {
    //     char filename[1024];
    //     vision_get_cache_filename("selection", sizeof(filename), filename);
    //     CheckPoint("selection filename = %s", filename);
    //     image_save(mask_image, filename);
    // } else {
    //     syslog_error("Create selection_mask image");
    // }

    return mask_image;
}

int vision_server_is_running()
{
    int ret;

    TASKSET* taskset = redos_open(GIMP_AI_SERVER);
    ret = (redos_pong(taskset) == RET_OK);
    redos_close(taskset);

    if (! ret) {
        g_message("AI server is not running ... Please start it\n");
    }

    return ret;
}