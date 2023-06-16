#!/usr/bin/env python

from gimpfu import *


def ImageHeal():
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
    "Fast Repair Image",
    # help
    "Fast Repair Image",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2023",
    # menupath
    "1. Fast Repair",
    # imagetypes (use * for all, leave blank for none)
    "",
    # params
    [],
    # results
    [],
    # function (to call)
    ImageHeal,
    # this can be included this way or the menu value can be directly prepended to the menupath
    menu="<Toolbox>/AI/Repair/",
)

register(
    # name
    "image_deep_patch",
    # blurb
    "Deep Repair Image",
    # help
    "Deep Repair Image",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2023",
    # menupath
    "2. Deep Repair",
    # imagetypes (use * for all, leave blank for none)
    "",
    # params
    [],
    # results
    [],
    # function (to call)
    ImageHeal,
    # this can be included this way or the menu value can be directly prepended to the menupath
    menu="<Toolbox>/AI/Repair/",
)


main()
