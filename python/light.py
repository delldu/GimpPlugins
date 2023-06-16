#!/usr/bin/env python

from gimpfu import *


def ImageLight():
    br, bg, bb, ba = gimp.get_background()
    fr, fg, fb, fa = gimp.get_foreground()

    gimp.set_background(fr, fg, fb, fa)
    gimp.set_foreground(br, bg, bb, ba)
    return


# see -> http://www.gimp.org/docs/python/
register(
    # name
    "image_low_light",
    # blurb
    "Automatic Enhance Low Light Image",
    # help
    "Automatic Enhance Low Light Image",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2023",
    # menupath
    "1. Auto Enhance",
    # imagetypes (use * for all, leave blank for none)
    "",
    # params
    [],
    # results
    [],
    # function (to call)
    ImageLight,
    # this can be included this way or the menu value can be directly prepended to the menupath
    menu="<Toolbox>/AI/Light/",
)

register(
    # name
    "image_global_light",
    # blurb
    "Global Light Enhance",
    # help
    "Global Light Enhance",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2023",
    # menupath
    "2. Global Enhance",
    # imagetypes (use * for all, leave blank for none)
    "",
    # params
    [],
    # results
    [],
    # function (to call)
    ImageLight,
    # this can be included this way or the menu value can be directly prepended to the menupath
    menu="<Toolbox>/AI/Light/",
)

register(
    # name
    "image_local_light",
    # blurb
    "Local Light Enhance",
    # help
    "Local Light Enhance",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2023",
    # menupath
    "3. Local Enhance",
    # imagetypes (use * for all, leave blank for none)
    "",
    # params
    [],
    # results
    [],
    # function (to call)
    ImageLight,
    # this can be included this way or the menu value can be directly prepended to the menupath
    menu="<Toolbox>/AI/Light/",
)

main()
