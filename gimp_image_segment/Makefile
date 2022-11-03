#/************************************************************************************
#***
#***	Copyright 2020-2022 Dell(18588220928@163.com), All Rights Reserved.
#***
#***	File Author: Dell, 2020-11-16 11:09:15
#***
#************************************************************************************/
#
INSTALL_DIR := $(shell grep -w INSTALL_DIR ../Makefile | cut -d '=' -f 2)

TARGET1 := gimp_image_segment
TARGET2 := gimp_image_selection

SOURCE1 :=  \
	image_segment.c

SOURCE2 :=  \
	image_selection.c

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

OBJECT1 := $(addsuffix .o,$(basename ${SOURCE1}))
OBJECT2 := $(addsuffix .o,$(basename ${SOURCE2}))

#****************************************************************************
# Compile block
#****************************************************************************
all: ${TARGET1} ${TARGET2}

${TARGET1}: ${OBJECT1}
	${CC} ${LDFLAGS} -o ${TARGET1} ${OBJECT1} ${LIBS}

${TARGET2}: ${OBJECT2}
	${CC} ${LDFLAGS} -o ${TARGET2} ${OBJECT2} ${LIBS}


install: ${TARGET1} ${TARGET2}
	install -m 755 ${TARGET1} ${INSTALL_DIR}
	install -m 755 ${TARGET2} ${INSTALL_DIR}

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
	rm -rf ${OBJECT1} ${TARGET1} ${OBJECT2} ${TARGET2}

uninstall:
	rm -rf ${INSTALL_DIR}/${TARGET1}
	rm -rf ${INSTALL_DIR}/${TARGET2}
