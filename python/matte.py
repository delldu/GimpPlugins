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
    "image_matte",
    # blurb
    "Salient Matte with Deep Learning",
    # help
    "Salient Matte with Deep Learning",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    # menupath
    "1. Salient",
    # imagetypes (use * for all, leave blank for none)
    "",
    # params
    [],
    # results
    [],
    # function (to call)
    ImageColor,
    # this can be included this way or the menu value can be directly prepended to the menupath
    menu="<Toolbox>/AI/Matte/",
)

register(
    # name
    "image_segment",
    # blurb
    "Semantic Matte with Deep Learning",
    # help
    "Semantic Matte with Deep Learning",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    # menupath
    "2. Semantic",
    # imagetypes (use * for all, leave blank for none)
    "",
    # params
    [],
    # results
    [],
    # function (to call)
    ImageColor,
    # this can be included this way or the menu value can be directly prepended to the menupath
    menu="<Toolbox>/AI/Matte/",
)

main()
