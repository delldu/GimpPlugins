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
    "Automatic Enhance with Deep Learning",
    # help
    "Automatic Enhance with Deep Learning",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    # menupath
    "1. Auto",
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
    "Global Enhance with Deep Learning",
    # help
    "Global Enhance with Deep Learning",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    # menupath
    "2. Global",
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
    "Local Enhance with Deep Learning",
    # help
    "Local Enhance with Deep Learning",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    # menupath
    "3. Local",
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
