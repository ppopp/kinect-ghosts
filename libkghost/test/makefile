#
# APPLICATION MAKEFILE
#
# Created by: Phillip Popp (ppopp@gracenote.com)
#
# Gracenote 2012.  All rights reserved.

ifndef BUILD_ARCH
	include arch.mak
endif

# output product info
OUT_NAME := test
OUT_DIR := bin
OUT_EXT := 
DEBUG_POSTFIX := d

# gtest files
GTESTDIR := ./gtest-1.6.0
# compiler flags / search paths
INC_DIRS := $(GTESTDIR)/include $(GTESTDIR) ../inc /lol/include
SRC_DIRS := src 
PREPROCESSOR_DEFINES := 
# Libraries that use this makefile system can be added here.  Simply add the
# directories of the makefiles to the PROJECT_DIRS variable and they will be
# automatically linked in to the application. Remember to still add the names
# of the libraries to LINK_RELEASE_LIBS and LINK_DEBUG_LIBS
PROJECT_DIRS := ../

FRAMEWORKS := OpenGL GLUT
LINK_LIBS := GLEW freenect
LINK_RELEASE_LIBS := $(LINK_LIBS) kghost
LINK_DEBUG_LIBS := $(LINK_LIBS) kghostd
LINK_RELEASE_32_LIBS := $(LINK_RELEASE_LIBS)
LINK_RELEASE_64_LIBS := $(LINK_RELEASE_LIBS)
LINK_DEBUG_32_LIBS := $(LINK_DEBUG_LIBS)
LINK_DEBUG_64_LIBS := $(LINK_DEBUG_LIBS)
# If you have code that uses other libraries you can add in their directories here
LIB_DIRS := /lol/lib
LIB_DIRS_DEBUG := $(LIB_DIRS)
LIB_DIRS_RELEASE := $(LIB_DIRS)
LIB_DIRS_DEBUG_32 := $(LIB_DIRS_DEBUG) 
LIB_DIRS_DEBUG_64 := $(LIB_DIRS_DEBUG) 
LIB_DIRS_RELEASE_32 := $(LIB_DIRS_RELEASE)
LIB_DIRS_RELEASE_64 := $(LIB_DIRS_RELEASE)

LFLAGS := 

# file extensions
SRC_EXTS := .c .cc .cpp
HEADER_EXTS := .h

# location of make files to be called before this one
BUILDDEPS := $(PROJECT_DIRS)

# files to be copied into output directory
COPYFILES :=
COPYFILES_DEBUG := $(COPYFILES)
COPYFILES_RELEASE := $(COPYFILES)
COPYFILES_DEBUG_32 := $(COPYFILES_DEBUG)
COPYFILES_DEBUG_64 := $(COPYFILES_DEBUG)
COPYFILES_RELEASE_32 := $(COPYFILES_RELEASE)
COPYFILES_RELEASE_64 := $(COPYFILES_RELEASE)

#####################################################################
### most users of this makefile won't need to edit anything below.###
#####################################################################

### Add project information ###
# add project library directories
LIB_DIRS_DEBUG_32 := $(LIB_DIRS_DEBUG_32) $(foreach projdir, $(PROJECT_DIRS), $(projdir)/lib/debug/32)
LIB_DIRS_DEBUG_64 := $(LIB_DIRS_DEBUG_64) $(foreach projdir, $(PROJECT_DIRS), $(projdir)/lib/debug/64)
LIB_DIRS_RELEASE_32 := $(LIB_DIRS_RELEASE_32) $(foreach projdir, $(PROJECT_DIRS), $(projdir)/lib/release/32)
LIB_DIRS_RELEASE_64 := $(LIB_DIRS_RELEASE_64) $(foreach projdir, $(PROJECT_DIRS), $(projdir)/lib/release/64)

LFLAGS := $(LFLAGS) $(foreach framework, $(FRAMEWORKS), -framework $(framework))
LFLAGS_DEBUG := $(LFLAGS)
LFLAGS_RELEASE := $(LFLAGS)
LFLAGS_DEBUG_32 := $(LFLAGS_DEBUG) $(LDFLAGS32)
LFLAGS_DEBUG_64 := $(LFLAGS_DEBUG) $(LDFLAGS64)
LFLAGS_RELEASE_32 := $(LFLAGS_RELEASE) $(LDFLAGS32)
LFLAGS_RELEASE_64 := $(LFLAGS_RELEASE) $(LDFLAGS64)
### add library link directory flags ###
LFLAGS_RELEASE_32 := $(LFLAGS_RELEASE_32) $(foreach LIB_DIRS_RELEASE_32, $(LIB_DIRS_RELEASE_32), -L$(LIB_DIRS_RELEASE_32)) 
LFLAGS_RELEASE_64 := $(LFLAGS_RELEASE_64) $(foreach LIB_DIRS_RELEASE_64, $(LIB_DIRS_RELEASE_64), -L$(LIB_DIRS_RELEASE_64)) 
LFLAGS_DEBUG_32 := $(LFLAGS_DEBUG_32) $(foreach LIB_DIRS_DEBUG_32, $(LIB_DIRS_DEBUG_32), -L$(LIB_DIRS_DEBUG_32)) 
LFLAGS_DEBUG_64 := $(LFLAGS_DEBUG_64) $(foreach LIB_DIRS_DEBUG_64, $(LIB_DIRS_DEBUG_64), -L$(LIB_DIRS_DEBUG_64)) 

LIBFLAGS_RELEASE_32 := $(foreach library, $(LINK_RELEASE_32_LIBS), -l$(library))
LIBFLAGS_RELEASE_64 := $(foreach library, $(LINK_RELEASE_64_LIBS), -l$(library))
LIBFLAGS_DEBUG_32 := $(foreach library, $(LINK_DEBUG_32_LIBS), -l$(library))
LIBFLAGS_DEBUG_64 := $(foreach library, $(LINK_DEBUG_64_LIBS), -l$(library))

OUT_DIR_DEBUG := $(OUT_DIR)/debug
OUT_DIR_DEBUG_32 := $(OUT_DIR_DEBUG)/32
OUT_DIR_DEBUG_64 := $(OUT_DIR_DEBUG)/64
OUT_DIR_RELEASE := $(OUT_DIR)/release
OUT_DIR_RELEASE_32 := $(OUT_DIR_RELEASE)/32
OUT_DIR_RELEASE_64 := $(OUT_DIR_RELEASE)/64

OBJ_DIR := obj
DEBUG_OBJ_DIR := $(OBJ_DIR)/debug
DEBUG_OBJ_DIR_32 := $(DEBUG_OBJ_DIR)/32
DEBUG_OBJ_DIR_64 := $(DEBUG_OBJ_DIR)/64
RELEASE_OBJ_DIR := $(OBJ_DIR)/release
RELEASE_OBJ_DIR_32 := $(RELEASE_OBJ_DIR)/32
RELEASE_OBJ_DIR_64 := $(RELEASE_OBJ_DIR)/64

CFLAGS := $(CFLAGS) $(foreach INC_DIRS, $(INC_DIRS), -I$(INC_DIRS))
CFLAGS := $(CFLAGS) $(foreach PREPROCESSOR_DEFINES, $(PREPROCESSOR_DEFINES), -D$(PREPROCESSOR_DEFINES))
CFLAGS := $(CFLAGS) -fPIC -Wall 
CFLAGS_DEBUG := $(CFLAGS) -g
CFLAGS_DEBUG_32 := $(CFLAGS_DEBUG) $(CFLAGS32)
CFLAGS_DEBUG_64 := $(CFLAGS_DEBUG) $(CFLAGS64)
CFLAGS_RELEASE := $(CFLAGS) -DNDEBUG -O2
CFLAGS_RELEASE_32 := $(CFLAGS_RELEASE) $(CFLAGS32)
CFLAGS_RELEASE_64 := $(CFLAGS_RELEASE) $(CFLAGS64)

CC = g++
AR = ar

find_sources_with_ext = $(foreach SRCDIR, $(1), $(wildcard $(SRCDIR)/*$(EXT)))
create_objects_with_ext = $(patsubst %$(EXT), %.o, $(patsubst %, $(1)/%, $(notdir $(call find_sources_with_ext, $(2)))))
create_objects = $(foreach EXT, $(SRC_EXTS), $(call create_objects_with_ext, $(1), $(2)))

# get headers
HEADERS := $(foreach EXT, $(HEADER_EXTS), $(call find_sources_with_ext, $(SRC_DIRS)))
HEADERS += $(foreach EXT, $(HEADER_EXTS), $(call find_sources_with_ext, $(INC_DIRS)))

# set search paths for source files
VPATH += $(foreach SRC_DIRS, $(SRC_DIRS), $(SRC_DIRS):)
VPATH += $(GTESTDIR)/src

# set target object files
RELEASE_OBJS_32 := $(call create_objects, $(RELEASE_OBJ_DIR_32), $(SRC_DIRS))
RELEASE_OBJS_64 := $(call create_objects, $(RELEASE_OBJ_DIR_64), $(SRC_DIRS))
DEBUG_OBJS_32 := $(call create_objects, $(DEBUG_OBJ_DIR_32), $(SRC_DIRS))
DEBUG_OBJS_64 := $(call create_objects, $(DEBUG_OBJ_DIR_64), $(SRC_DIRS))

# add in gtest main object
RELEASE_OBJS_32 += $(RELEASE_OBJ_DIR_32)/gtest-all.o
RELEASE_OBJS_64 += $(RELEASE_OBJ_DIR_64)/gtest-all.o
DEBUG_OBJS_32 += $(DEBUG_OBJ_DIR_32)/gtest-all.o
DEBUG_OBJS_64 += $(DEBUG_OBJ_DIR_64)/gtest-all.o

.PHONY: clean debug debug32 debug64 release release32 release64 dependencies dist_clean post $(BUILDDEPS) $(LIBFLAGS_RELEASE_64) $(LIBFLAGS_RELEASE_32) $(LIBFLAGS_DEBUG_32) $(LIBFLAGS_DEBUG_64)

all: $(BUILDDEPS) debug release post
debug: debug64
debug32: $(OUT_DIR_DEBUG_32)/$(OUT_NAME)$(DEBUG_POSTFIX)$(OUT_EXT)
debug64: $(OUT_DIR_DEBUG_64)/$(OUT_NAME)$(DEBUG_POSTFIX)$(OUT_EXT)
release: release64
release32: $(OUT_DIR_RELEASE_32)/$(OUT_NAME)$(OUT_EXT)
release64: $(OUT_DIR_RELEASE_64)/$(OUT_NAME)$(OUT_EXT)
dist_clean: $(BUILDDEPS) clean
post: $(COPYFILES_DEBUG_32) $(COPYFILES_RELEASE_32) $(COPYFILES_DEBUG_64) $(COPYFILES_RELEASE_64)
dependencies: $(BUILDDEPS)

clean: 
	rm -f $(DEBUG_OBJS_32)
	rm -f $(DEBUG_OBJS_64)
	rm -f $(RELEASE_OBJS_32)
	rm -f $(RELEASE_OBJS_64)
	rm -f $(OUT_DIR_RELEASE_32)/$(OUT_NAME)$(OUT_EXT)
	rm -f $(OUT_DIR_RELEASE_64)/$(OUT_NAME)$(OUT_EXT)
	rm -f $(OUT_DIR_DEBUG_32)/$(OUT_NAME)$(DEBUG_POSTFIX)$(OUT_EXT)
	rm -f $(OUT_DIR_DEBUG_64)/$(OUT_NAME)$(DEBUG_POSTFIX)$(OUT_EXT)

$(COPYFILES_DEBUG_32): debug $(BUILDDEPS)
	cp $@ $(OUT_DIR_DEBUG_32)

$(COPYFILES_DEBUG_64): debug $(BUILDDEPS)
	cp $@ $(OUT_DIR_DEBUG_64)

$(COPYFILES_RELEASE_32): release $(BUILDDEPS)
	cp $@ $(OUT_DIR_RELEASE_32)

$(COPYFILES_RELEASE_64): release $(BUILDDEPS)
	cp $@ $(OUT_DIR_RELEASE_64)

$(BUILDDEPS):
	cd $@ && $(MAKE) $(MAKECMDGOALS)

$(OUT_DIR_RELEASE_32)/$(OUT_NAME)$(OUT_EXT): $(RELEASE_OBJS_32) $(LIBFLAGS_RELEASE_32) | dependencies
	@mkdir -p $(OUT_DIR)
	@mkdir -p $(OUT_DIR_RELEASE)
	@mkdir -p $(OUT_DIR_RELEASE_32)
	rm -f $@
	$(CC) $(LFLAGS_RELEASE_32) $^ -o $@

$(OUT_DIR_RELEASE_64)/$(OUT_NAME)$(OUT_EXT): $(RELEASE_OBJS_64) $(LIBFLAGS_RELEASE_64) | dependencies
	@mkdir -p $(OUT_DIR)
	@mkdir -p $(OUT_DIR_RELEASE)
	@mkdir -p $(OUT_DIR_RELEASE_64)
	rm -f $@
	$(CC) $(LFLAGS_RELEASE_64) $^ -o $@

$(OUT_DIR_DEBUG_32)/$(OUT_NAME)$(DEBUG_POSTFIX)$(OUT_EXT): $(DEBUG_OBJS_32) $(LIBFLAGS_DEBUG_32) | dependencies
	@mkdir -p $(OUT_DIR)
	@mkdir -p $(OUT_DIR_DEBUG)
	@mkdir -p $(OUT_DIR_DEBUG_32)
	rm -f $@
	$(CC) $(LFLAGS_DEBUG_32) $^ -o $@

$(OUT_DIR_DEBUG_64)/$(OUT_NAME)$(DEBUG_POSTFIX)$(OUT_EXT): $(DEBUG_OBJS_64) $(LIBFLAGS_DEBUG_64) | dependencies
	@mkdir -p $(OUT_DIR)
	@mkdir -p $(OUT_DIR_DEBUG)
	@mkdir -p $(OUT_DIR_DEBUG_64)
	rm -f $@
	$(CC) $(LFLAGS_DEBUG_64) $^ -o $@

# compile .c files
$(DEBUG_OBJ_DIR_32)/%.o: %.c $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(DEBUG_OBJ_DIR)
	@mkdir -p $(DEBUG_OBJ_DIR_32)
	$(CC) -c -o $@ $< $(CFLAGS_DEBUG_32)

$(DEBUG_OBJ_DIR_64)/%.o: %.c $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(DEBUG_OBJ_DIR)
	@mkdir -p $(DEBUG_OBJ_DIR_64)
	$(CC) -c -o $@ $< $(CFLAGS_DEBUG_64)

$(RELEASE_OBJ_DIR_32)/%.o: %.c $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(RELEASE_OBJ_DIR)
	@mkdir -p $(RELEASE_OBJ_DIR_32)
	$(CC) -c -o $@ $< $(CFLAGS_RELEASE_32)

$(RELEASE_OBJ_DIR_64)/%.o: %.c $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(RELEASE_OBJ_DIR)
	@mkdir -p $(RELEASE_OBJ_DIR_64)
	$(CC) -c -o $@ $< $(CFLAGS_RELEASE_64)

# compile .cc files
$(DEBUG_OBJ_DIR_32)/%.o: %.cc $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(DEBUG_OBJ_DIR)
	@mkdir -p $(DEBUG_OBJ_DIR_32)
	$(CC) -c -o $@ $< $(CFLAGS_DEBUG_32)

$(DEBUG_OBJ_DIR_64)/%.o: %.cc $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(DEBUG_OBJ_DIR)
	@mkdir -p $(DEBUG_OBJ_DIR_64)
	$(CC) -c -o $@ $< $(CFLAGS_DEBUG_64)

$(RELEASE_OBJ_DIR_32)/%.o: %.cc $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(RELEASE_OBJ_DIR)
	@mkdir -p $(RELEASE_OBJ_DIR_32)
	$(CC) -c -o $@ $< $(CFLAGS_RELEASE_32)

$(RELEASE_OBJ_DIR_64)/%.o: %.cc $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(RELEASE_OBJ_DIR)
	@mkdir -p $(RELEASE_OBJ_DIR_64)
	$(CC) -c -o $@ $< $(CFLAGS_RELEASE_64)

# compile .cpp files
$(DEBUG_OBJ_DIR_32)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(DEBUG_OBJ_DIR)
	@mkdir -p $(DEBUG_OBJ_DIR_32)
	$(CC) -c -o $@ $< $(CFLAGS_DEBUG_32)

$(DEBUG_OBJ_DIR_64)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(DEBUG_OBJ_DIR)
	@mkdir -p $(DEBUG_OBJ_DIR_64)
	$(CC) -c -o $@ $< $(CFLAGS_DEBUG_64)

$(RELEASE_OBJ_DIR_32)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(RELEASE_OBJ_DIR)
	@mkdir -p $(RELEASE_OBJ_DIR_32)
	$(CC) -c -o $@ $< $(CFLAGS_RELEASE_32)

$(RELEASE_OBJ_DIR_64)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(RELEASE_OBJ_DIR)
	@mkdir -p $(RELEASE_OBJ_DIR_64)
	$(CC) -c -o $@ $< $(CFLAGS_RELEASE_64)

