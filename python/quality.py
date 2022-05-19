#!/usr/bin/env python

from gimpfu import *


def ImageANIMA():
    br, bg, bb, ba = gimp.get_background()
    fr, fg, fb, fa = gimp.get_foreground()

    gimp.set_background(fr, fg, fb, fa)
    gimp.set_foreground(br, bg, bb, ba)
    return


# see -> http://www.gimp.org/docs/python/
register(
    # name
    "image_quality",
    # blurb
    "Advanced Neural Image Assessment with Deep Learning",
    # help
    "Advanced Neural Image Assessment with Deep Learning",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    # menupath
    "Quality",
    # imagetypes (use * for all, leave blank for none)
    "",
    # params
    [],
    # results
    [],
    # function (to call)
    ImageANIMA,
    # this can be included this way or the menu value can be directly prepended to the menupath
    menu="<Toolbox>/AI/",
)

main()
