#!/usr/bin/env python

from gimpfu import *


def ImageZoom():
    br, bg, bb, ba = gimp.get_background()
    fr, fg, fb, fa = gimp.get_foreground()

    gimp.set_background(fr, fg, fb, fa)
    gimp.set_foreground(br, bg, bb, ba)
    return


# see -> http://www.gimp.org/docs/python/
register(
    # name
    "image_zoom4x",
    # blurb
    "Zoom in 4X",
    # help
    "Zoom in 4X",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    # menupath
    "4X Size",
    # imagetypes (use * for all, leave blank for none)
    "",
    # params
    [],
    # results
    [],
    # function (to call)
    ImageZoom,
    # this can be included this way or the menu value can be directly prepended to the menupath
    menu="<Toolbox>/AI/Zoom In/",
)

register(
    # name
    "image_zomm8x",
    # blurb
    "Zoom in 8X Size",
    # help
    "Zoom in 8X Size",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    # menupath
    "8X Size",
    # imagetypes (use * for all, leave blank for none)
    "",
    # params
    [],
    # results
    [],
    # function (to call)
    ImageZoom,
    # this can be included this way or the menu value can be directly prepended to the menupath
    menu="<Toolbox>/AI/Zoom In/",
)

main()
