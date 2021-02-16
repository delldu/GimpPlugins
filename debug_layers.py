#!/usr/bin/env python

from gimpfu import *
import pdb

def debug_layers(image, drawable):
  # Help:
  print("layer=image.layers[0]")
  print("layer.get_pixel((0, 0))");
  layer = image.layers[0]

  pdb.set_trace()

  # for layer in image.layers:
  #   layer.opacity = 100.0/n
  #   n = n-1
  # image.flatten()

register(
        "python_fu_debug_layers",
        "Debug layers with pdb",
        "Debug layers with pdb",
        "Dell Du",
        "Dell Du",
        "2021",
        "<Image>/Image/Debug Layers",
        "RGB*, GRAY*",
        [],
        [],
        debug_layers)

main()

