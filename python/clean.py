#!/usr/bin/env python

# import gtk
import time
from gimpfu import *


def ImageDenoise():
    # # Using gtk to display an info type message see -> http://www.gtk.org/api/2.6/gtk/GtkMessageDialog.html#GtkMessageType
    # message = gtk.MessageDialog(type=gtk.MESSAGE_INFO, buttons=gtk.BUTTONS_OK)
    # message.set_markup("Remove Noise \nThis Dialog Will Cause Unexpected Issues During Batch Procedures")
    # message.run()
    # message.destroy()

    # # Using GIMP's interal procedure database see -> http://docs.gimp.org/en/glossary.html#glossary-pdb
    pdb.gimp_message("Start removing noise ...\n")
    return


def ImageDeblur():
    # todo
    pdb.gimp_message("Start removing blur ...")
    time.sleep(5)
    return


def ImageDefocus():
    # todo
    pdb.gimp_message("Start removing focus blur ...")
    time.sleep(5)
    return


# see -> http://www.gimp.org/docs/python/
register(
    # name
    "image_autops",
    # blurb
    "Automatic Remove Noise",
    # help
    "Automatic Remove Noise",
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
    ImageDenoise,
    # this can be included this way or the menu value can be directly prepended to the menupath
    menu="<Toolbox>/AI/Clean/",
)


register(
    # name
    "image_denoise",
    # blurb
    "Remove Noise",
    # help
    "Remove Noise",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    # menupath
    "2. Denoise ...",
    # imagetypes (use * for all, leave blank for none)
    "",
    # params
    [],
    # results
    [],
    # function (to call)
    ImageDenoise,
    # this can be included this way or the menu value can be directly prepended to the menupath
    menu="<Toolbox>/AI/Clean/",
)

# register(
#     # name
#     "image_cbdnet",
#     # blurb
#     "Blind Remove Noise",
#     # help
#     "Blind Remove Noise",
#     # author
#     "Dell Du",
#     # copyright
#     "Dell Du <18588220928@163.com>",
#     # date
#     "2022",
#     # menupath
#     "2. Blind Denoise ...",
#     # imagetypes (use * for all, leave blank for none)
#     "",
#     # params
#     [],
#     # results
#     [],
#     # function (to call)
#     ImageDenoise,
#     # this can be included this way or the menu value can be directly prepended to the menupath
#     menu="<Toolbox>/AI/Clean/",
# )


register(
    # name
    "image_deblur",
    # blurb
    "Remove Blur",
    # help
    "Remove Blur",
    # author
    "Dell Du",
    # copyright
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    # menupath
    "3. Deblur",
    # imagetypes (use * for all, leave blank for none)
    "",
    # params
    [],
    # results
    [],
    # function (to call)
    ImageDeblur,
    # this can be included this way or the menu value can be directly prepended to the menupath
    menu="<Toolbox>/AI/Clean/",
)

register(
    # name
    "image_defocus",
    # blurb
    "Remove Focus Blur",
    # help
    "",
    "Dell Du",
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    "4. Defocus",
    "",  # image types: "" means plugin will not push the current image as a variable
    [],
    [],
    ImageDefocus,
    menu="<Toolbox>/AI/Clean/",
)

register(
    # name
    "image_descratch",
    # blurb
    "Remove Scratch",
    # help
    "",
    "Dell Du",
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    "5. Remove Scratch ...",
    "",  # image types: "" means plugin will not push the current image as a variable
    [],
    [],
    ImageDefocus,
    menu="<Toolbox>/AI/Clean/",
)


register(
    # name
    "image_dereflect",
    # blurb
    "Remove Reflection",
    # help
    "",
    "Dell Du",
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    "6. Remove Reflection",
    "",  # image types: "" means plugin will not push the current image as a variable
    [],
    [],
    ImageDefocus,
    menu="<Toolbox>/AI/Clean/",
)

register(
    "image_deshadow",
    "Detect or Remove Shadow",
    "Detect or Remove Shadow",
    "Dell Du",
    "Dell Du <18588220928@163.com>",
    "2022",
    "7. Remove Shadow ...",
    "",  # image types: "" means plugin will not push the current image as a variable
    [],
    [],
    ImageDefocus,
    menu="<Toolbox>/AI/Clean/",
)

register(
    # name
    "image_dehaze",
    # blurb
    "Remove Haze",
    # help
    "",
    "Dell Du",
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    "1. Dehaze",
    "",  # image types: "" means plugin will not push the current image as a variable
    [],
    [],
    ImageDefocus,
    menu="<Toolbox>/AI/Clean",
)


register(
    # name
    "image_derain",
    # blurb
    "Remove Rain",
    # help
    "",
    "Dell Du",
    "Dell Du <18588220928@163.com>",
    # date
    "2022",
    "2. Derain",
    "",  # image types: "" means plugin will not push the current image as a variable
    [],
    [],
    ImageDefocus,
    menu="<Toolbox>/AI/Clean",
)

main()
