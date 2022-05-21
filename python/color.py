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
    "Colorize Image via Guide",
    # help
    "Colorize Image via Guide",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    # menupath
    "1. Guide Color",
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
    "Colorize Image via Examplar",
    # help
    "Colorize Image via Examplar",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    # menupath
    "2. Examplar Color",
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
    "Colorize Image via Semantics",
    # help
    "Colorize Image via Semantics",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    # menupath
    "3. Semantics Color",
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
