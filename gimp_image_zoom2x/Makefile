#/************************************************************************************
#***
#***	Copyright 2020-2022 Dell(18588220928@163.com), All Rights Reserved.
#***
#***	File Author: Dell, 2020-11-16 11:09:15
#***
#************************************************************************************/
#
INSTALL_DIR := $(shell grep -w INSTALL_DIR ../Makefile | cut -d '=' -f 2)

TARGET := gimp_image_zoom2x

SOURCE :=  \
	image_zoom2x.c

INCS	:= -I../lib $(shell gimptool-2.0 --cflags) $(shell nimagetool --cflags)
LIBS	:= -L../lib -lplugin $(shell gimptool-2.0 --libs) $(shell nimagetool --libs) $(shell redostool --libs)

DEFINES :=

CFLAGS := -O2 -fPIC -Wall -Wextra -Wno-deprecated-declarations
LDFLAGS := -fPIC

#****************************************************************************
# Makefile code common to all platforms
#****************************************************************************
CFLAGS   := ${CFLAGS} ${DEFINES}
CXXFLAGS := ${CXXFLAGS} ${DEFINES}

OBJECT := $(addsuffix .o,$(basename ${SOURCE}))

#****************************************************************************
# Compile block
#****************************************************************************
all: ${TARGET}

${TARGET}: ${OBJECT}
	${CC} ${LDFLAGS} -o ${TARGET} ${OBJECT} ${LIBS}

install: ${TARGET}
	install -m 755 ${TARGET} ${INSTALL_DIR}

#****************************************************************************
# common rules
#****************************************************************************
%.o : %.cpp
	${CXX} ${CXXFLAGS} ${INCS} -c $< -o $@

%.o : %.c
	${CC} ${CFLAGS} ${INCS} -c $< -o $@

#****************************************************************************
# Depend block
#****************************************************************************
depend:

clean:
	rm -rf ${OBJECT} ${TARGET}

uninstall:
	rm -rf ${INSTALL_DIR}/${TARGET}
