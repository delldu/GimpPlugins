## Gimp Developers

`https://developer.gimp.org/git.html`

```
	GimpRGB mycolor;
	mycolor.r = 1.0;
	mycolor.g = 0.0;
	mycolor.b = 0.0;
	mycolor.a = 1.0;


	gimp_image_select_rectangle(param[1].data.d_image, GIMP_CHANNEL_OP_ADD, 100, 100, 400, 200);
	gimp_image_select_color(param[1].data.d_image, GIMP_CHANNEL_OP_ADD, param[2].data.d_drawable, &mycolor);
```


gtk_widget_set_size_request(dialog, 1024, 1024); // width, height ?


CheckPoint("gimp_data_directory = %s",gimp_data_directory());
==> gimp_data_directory = /usr/share/gimp/2.0


gboolean gimp_image_get_resolution (gint32 image_ID, gdouble *xresolution, gdouble *yresolution);
// gimp_image_get_resolution(image_id, &rx, &ry), rx = 72.00, ry=72.00


GimpRGB fg;
gimp_context_get_foreground (&fg);
CheckPoint("Foreground: R=%.2f, G=%.2lf, B=%.2lf, A=%.2lf", fg.r, fg.g, fg.b, fg.a);
# Foreground: R=1.00, G=0.00, B=0.00, A=1.00


  combo = gimp_layer_combo_box_new (smp_constrain, NULL);

  gimp_int_combo_box_prepend (GIMP_INT_COMBO_BOX (combo),
  GIMP_INT_STORE_VALUE,   SMP_INV_GRADIENT,
  GIMP_INT_STORE_LABEL,   _("From reverse gradient"),
  GIMP_INT_STORE_ICON_NAME, GIMP_ICON_GRADIENT,
  -1);
  gimp_int_combo_box_prepend (GIMP_INT_COMBO_BOX (combo),
  GIMP_INT_STORE_VALUE,   SMP_GRADIENT,
  GIMP_INT_STORE_LABEL,   _("From gradient"),
  GIMP_INT_STORE_ICON_NAME, GIMP_ICON_GRADIENT,
  -1);

# void gimp_int_combo_box_prepend (GimpIntComboBox *combo_box, ...)
# {
#   GtkListStore *store;
#   GtkTreeIter   iter;
#   va_list   args;

#   g_return_if_fail (GIMP_IS_INT_COMBO_BOX (combo_box));

#   store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box)));

#   va_start (args, combo_box);

#   gtk_list_store_prepend (store, &iter);
#   gtk_list_store_set_valist (store, &iter, args);

#   va_end (args);
# }

  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo), g_values.sample_id,
  G_CALLBACK (smp_sample_combo_callback),
  NULL);



gimp_run_procedure();



GtkObject *gimp_scale_entry_new(GtkTable  *table,
    gint   column, gint   row,
    const gchar *text,
    gint   scale_width, gint   spinbutton_width,
    gdouble value, gdouble lower, gdouble upper,
    gdouble step_increment, gdouble  page_increment,
    guint digits,
    gboolean constrain,
    gdouble unconstrained_lower,
    gdouble unconstrained_upper,
    const gchar *tooltip, const gchar *help_id);