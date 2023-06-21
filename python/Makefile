INSTALL_DIR=~/.config/GIMP/2.10/plug-ins
# ~/.var/app/org.gimp.GIMP/config/GIMP/2.10/plug-ins
# /home/dell/.config/GIMP/2.10/plug-ins

install:
	# chmod +x *.py
	cp -v *.py ${INSTALL_DIR}

clean:
	rm -rf ${INSTALL_DIR}/*.py

format:
	black -l 120 *.py
