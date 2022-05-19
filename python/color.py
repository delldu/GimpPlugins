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
    "image_guide_color",
    # blurb
    "Guide Color with Deep Learning",
    # help
    "Guide Color with Deep Learning",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    # menupath
    "1. Guide",
    # imagetypes (use * for all, leave blank for none)
    "",
    # params
    [],
    # results
    [],
    # function (to call)
    ImageColor,
    # this can be included this way or the menu value can be directly prepended to the menupath
    menu="<Toolbox>/AI/Color/",
)

register(
    # name
    "image_example_color",
    # blurb
    "Color Image via Example with Deep Learning",
    # help
    "Color Image via Example with Deep Learning",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    # menupath
    "2. Example",
    # imagetypes (use * for all, leave blank for none)
    "",
    # params
    [],
    # results
    [],
    # function (to call)
    ImageColor,
    # this can be included this way or the menu value can be directly prepended to the menupath
    menu="<Toolbox>/AI/Color/",
)

register(
    # name
    "image_semantic_color",
    # blurb
    "Semantic Color with Deep Learning",
    # help
    "Semantic Color with Deep Learning",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    # menupath
    "3. Semantic",
    # imagetypes (use * for all, leave blank for none)
    "",
    # params
    [],
    # results
    [],
    # function (to call)
    ImageColor,
    # this can be included this way or the menu value can be directly prepended to the menupath
    menu="<Toolbox>/AI/Color/",
)


main()
