#!/usr/bin/env python

from gimpfu import *


def ImageMatte():
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
    "Salience Matte",
    # help
    "Salience Matte",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    # menupath
    "1. Salience Matting",
    # imagetypes (use * for all, leave blank for none)
    "",
    # params
    [],
    # results
    [],
    # function (to call)
    ImageMatte,
    # this can be included this way or the menu value can be directly prepended to the menupath
    menu="<Toolbox>/AI/Matte/",
)

register(
    # name
    "image_segment",
    # blurb
    "Semantics Matte",
    # help
    "Semantics Matte",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    # menupath
    "2. Semantic Matting",
    # imagetypes (use * for all, leave blank for none)
    "",
    # params
    [],
    # results
    [],
    # function (to call)
    ImageMatte,
    # this can be included this way or the menu value can be directly prepended to the menupath
    menu="<Toolbox>/AI/Matte/",
)

main()
