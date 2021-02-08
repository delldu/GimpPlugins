#/************************************************************************************
#***
#***	Copyright 2021 Dell(18588220928g@163.com), All Rights Reserved.
#***
#***	File Author: Dell, 2021-01-16 12:41:44
#***
#************************************************************************************/
#

LIB_NAME := libplugin

INCS	:= \
	$(shell gimptool-2.0 --cflags) $(shell nimagetool --cflags)

	# -I$(BUILD_DIR)/include
SOURCE :=  \
	plugin.c

DEFINES := 
CFLAGS := -O2 -fPIC -Wall -Wextra
LDFLAGS := -fPIC

#****************************************************************************
# Makefile code common to all platforms
#****************************************************************************
CFLAGS   := ${CFLAGS} ${DEFINES}
CXXFLAGS := ${CXXFLAGS} ${DEFINES}
OBJECTS := $(addsuffix .o,$(basename ${SOURCE}))

#****************************************************************************
# Compile block
#****************************************************************************
all: staticlib
# sharelib
# staticlib

sharelib: $(OBJECTS)
	$(LD) $(LDFLAGS) -shared -soname $(LIB_NAME).so -o $(LIB_NAME).so $(OBJECTS)
	# cp $(LIB_NAME).so ${INSTALL_DIR}/lib
	# mv $(LIB_NAME).so ../lib


staticlib:$(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB_NAME).a $(OBJECTS)
	# cp $(LIB_NAME).a ${INSTALL_DIR}/lib
	# mv $(LIB_NAME).a ../lib


#****************************************************************************
# Depend block
#****************************************************************************
depend:

#****************************************************************************
# common rules
#****************************************************************************
%.o : %.cpp
	${CXX} ${CXXFLAGS} ${INCS} -c $< -o $@

%.o : %.c
	${CC} ${CFLAGS} ${INCS} -c $< -o $@

install:
	@echo "Nothing to do."

uninstall:
	@echo "Nothing to do."

clean:
	rm -rf *.a *.so *.o $(OBJECTS)

