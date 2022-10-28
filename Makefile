#/************************************************************************************
#***
#***	Copyright 2020-2022 Dell(18588220928@163.com), All Rights Reserved.
#***
#***	File Author: Dell, 2020-11-16 11:31:56
#***
#************************************************************************************/
#
# /home/dell/snap/gimp/380/.config/GIMP/2.10/plug-ins
# /home/dell/snap/gimp/current/.config/GIMP/2.10/plug-ins
INSTALL_DIR=/home/dell/.config/GIMP/2.10/plug-ins

XSUBDIRS :=  \
	lib \
	gimp_image_autops \
	gimp_image_denoise \
	gimp_image_deblur \
	gimp_image_defocus \
	gimp_image_dehaze \
	gimp_image_derain \
	gimp_image_desnow \
	gimp_image_dereflection \
	gimp_image_colour \
	gimp_image_color \
	gimp_image_light \
	gimp_image_curve \
	gimp_image_zoom \
	gimp_image_matte \
	gimp_image_segment \
	gimp_image_shadow \
	gimp_image_scratch \
	gimp_image_patch \
	gimp_image_face_detect \
	gimp_image_face_beauty \
	gimp_image_artist_style \
	gimp_image_photo_style \
	gimp_image_shape_style \
	gimp_image_tanet \

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
	#./gimpinstall.sh debug_layers.py	

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

i18n:
	# /usr/share/locale/zh_CN/LC_MESSAGES/tai.mo
	find . -name "*.c" > /tmp/file_list.txt
	xgettext -k_ -o /tmp/tai.pot --language=c --files-from /tmp/file_list.txt
	msginit --no-translator -l zh_CN.gb2312 -i /tmp/tai.pot -o tai.po
	# sudo msgfmt tai.po -o /usr/share/locale/zh_CN/LC_MESSAGES/tai.mo
