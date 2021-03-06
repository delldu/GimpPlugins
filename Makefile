#/************************************************************************************
#***
#***	Copyright 2020-2021 Dell(18588220928g@163.com), All Rights Reserved.
#***
#***	File Author: Dell, 2020-11-16 11:31:56
#***
#************************************************************************************/
#
INSTALL_DIR=~/snap/gimp/current/.config/GIMP/2.10/plug-ins

XSUBDIRS :=  \
	lib \
	gimp_image_clean \
	gimp_image_zoom \
	gimp_image_color \
	gimp_image_patch \
	gimp_image_nima \
	gimp_image_light \
	gimp_image_matting


BSUBDIRS :=


all: 
	@for d in $(XSUBDIRS)  ; do \
		if [ -d $$d ] ; then \
			$(MAKE) -C $$d || exit 1; \
		fi \
	done	

install:
	@for d in $(XSUBDIRS)  ; do \
		if [ -d $$d ] ; then \
			$(MAKE) -C $$d install || exit 1; \
		fi \
	done
	./gimpinstall.sh debug_layers.py	


uninstall:
	@for d in $(XSUBDIRS)  ; do \
		if [ -d $$d ] ; then \
			$(MAKE) -C $$d uninstall || exit 1; \
		fi \
	done	

clean:
	@for d in $(XSUBDIRS) ; do \
		if [ -d $$d ] ; then \
			$(MAKE) -C $$d clean || exit 1; \
		fi \
	done	

test:
	@for d in $(XSUBDIRS) ; do \
		if [ -d $$d ] ; then \
			$(MAKE) -C $$d test || exit 1; \
		fi \
	done	
