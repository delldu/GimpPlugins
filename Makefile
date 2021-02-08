#/************************************************************************************
#***
#***	Copyright 2020-2021 Dell(18588220928g@163.com), All Rights Reserved.
#***
#***	File Author: Dell, 2020-11-16 11:31:56
#***
#************************************************************************************/
#

XSUBDIRS :=  \
	lib \
	gimp_image_clean \
	gimp_image_zoom \
	gimp_image_color \
	gimp_image_patch \
	gimp_image_nima


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
