#!/usr/bin/env python

from gimpfu import *


def ImageColor():
    br, bg, bb, ba = gimp.get_background()
    fr, fg, fb, fa = gimp.get_foreground()

    gimp.set_background(fr, fg, fb, fa)
    gimp.set_foreground(br, bg, bb, ba)
    return


# see -> http://www.gimp.org/docs/python/
register(
    # name
    "image_fast_patch",
    # blurb
    "Fast Repair Image with Deep Learning",
    # help
    "Fast Repair Image with Deep Learning",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    # menupath
    "1. Fast",
    # imagetypes (use * for all, leave blank for none)
    "",
    # params
    [],
    # results
    [],
    # function (to call)
    ImageColor,
    # this can be included this way or the menu value can be directly prepended to the menupath
    menu="<Toolbox>/AI/Patch/",
)

register(
    # name
    "image_deep_patch",
    # blurb
    "Deep Repair Image with Deep Learning",
    # help
    "Deep Repair Image with Deep Learning",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    # menupath
    "2. Deep",
    # imagetypes (use * for all, leave blank for none)
    "",
    # params
    [],
    # results
    [],
    # function (to call)
    ImageColor,
    # this can be included this way or the menu value can be directly prepended to the menupath
    menu="<Toolbox>/AI/Patch/",
)

main()
