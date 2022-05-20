#!/usr/bin/env python

from gimpfu import *


def ImageBlend():
    br, bg, bb, ba = gimp.get_background()
    fr, fg, fb, fa = gimp.get_foreground()

    gimp.set_background(fr, fg, fb, fa)
    gimp.set_foreground(br, bg, bb, ba)
    return


# see -> http://www.gimp.org/docs/python/
register(
    # name
    "image_blend_style",
    # blurb
    "Blend Foreground Style to Background Image",
    # help
    "",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    # menupath
    "1. Blend Style",
    # imagetypes (use * for all, leave blank for none)
    "",
    # params
    [],
    # results
    [],
    # function (to call)
    ImageBlend,
    # this can be included this way or the menu value can be directly prepended to the menupath
    menu="<Toolbox>/AI/Blend/",
)

register(
    # name
    "image_blend_content",
    # blurb
    "Blend Foreground Content to Background Image",
    # help
    "",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    # menupath
    "2. Blend Content",
    # imagetypes (use * for all, leave blank for none)
    "",
    # params
    [],
    # results
    [],
    # function (to call)
    ImageBlend,
    # this can be included this way or the menu value can be directly prepended to the menupath
    menu="<Toolbox>/AI/Blend/",
)


main()
