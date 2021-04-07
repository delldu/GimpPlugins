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