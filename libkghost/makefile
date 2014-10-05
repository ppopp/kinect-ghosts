#
# LIB MAKEFILE
#
# Created by: Phillip Popp (ppopp@gracenote.com)
#
# Gracenote 2012.  All rights reserved.

ifndef BUILD_ARCH
	include arch.mak
endif

# uncomment this to include IPP headers 
#ifndef IPP_LIB_DIR
#	include ipp.mak
#endif

# output product info
OUTNAME := libkghost
OUTDIR := lib
OUTEXT := .a
DEBUG_POSTFIX := 

# search paths for source files
INCDIRS := $(INCDIRS) inc
SRCDIRS := $(SRCDIRS) src

# preprocessor definitions
PPDEFINES := $(PPDEFINES)
PPDEFINES32 := $(PPDEFINES) 
PPDEFINES64 := $(PPDEFINES) 

ifdef BUILD_PLATFORM
PPDEFINES32 := $(PPDEFINES32) $(BUILD_PLATFORM)_32
PPDEFINES64 := $(PPDEFINES64) $(BUILD_PLATFORM)_64
endif

SRC_EXTS := .c .cc .cpp
HEADER_EXTS := .h

# there are a bunch of depecration warnigns about glu & glut that I hate seeing
CFLAGS := $(CFLAGS) -Wno-deprecated

#CXX = g++
#CC = gcc
#AR = ar

#####################################################################
### most users of this makefile won't need to edit anything below.###
#####################################################################

OUTDIR_DEBUG := $(OUTDIR)/debug
OUTDIR_DEBUG32 := $(OUTDIR_DEBUG)/32
OUTDIR_DEBUG64 := $(OUTDIR_DEBUG)/64
OUTDIR_RELEASE := $(OUTDIR)/release
OUTDIR_RELEASE32 := $(OUTDIR_RELEASE)/32
OUTDIR_RELEASE64 := $(OUTDIR_RELEASE)/64

OBJDIR := obj
DEBUG_OBJDIR := $(OBJDIR)/debug
DEBUG_OBJDIR32 := $(DEBUG_OBJDIR)/32
DEBUG_OBJDIR64 := $(DEBUG_OBJDIR)/64
RELEASE_OBJDIR := $(OBJDIR)/release
RELEASE_OBJDIR32 := $(RELEASE_OBJDIR)/32
RELEASE_OBJDIR64 := $(RELEASE_OBJDIR)/64

CFLAGS := $(CFLAGS) $(foreach INCDIRS, $(INCDIRS), -I$(INCDIRS))
CFLAGS := $(CFLAGS) -fPIC -Wall 
CFLAGS_DEBUG := $(CFLAGS) -g
CFLAGS_DEBUG32 := $(CFLAGS_DEBUG) $(CFLAGS32) $(foreach PPDEFINES32, $(PPDEFINES32), -D$(PPDEFINES32))
CFLAGS_DEBUG64 := $(CFLAGS_DEBUG) $(CFLAGS64) $(foreach PPDEFINES64, $(PPDEFINES64), -D$(PPDEFINES64))
CFLAGS_RELEASE := $(CFLAGS) -DNDEBUG -O2
CFLAGS_RELEASE32 := $(CFLAGS_RELEASE) $(CFLAGS32) $(foreach PPDEFINES32, $(PPDEFINES32), -D$(PPDEFINES32))
CFLAGS_RELEASE64 := $(CFLAGS_RELEASE) $(CFLAGS64) $(foreach PPDEFINES64, $(PPDEFINES64), -D$(PPDEFINES64))

# helper methods
find_sources_with_ext = $(foreach SRCDIR, $(1), $(wildcard $(SRCDIR)/*$(EXT)))
create_objects_with_ext = $(patsubst %$(EXT), %.o, $(patsubst %, $(1)/%, $(notdir $(call find_sources_with_ext, $(2)))))
create_objects = $(foreach EXT, $(SRC_EXTS), $(call create_objects_with_ext, $(1), $(2)))

# get headers
HEADERS := $(foreach EXT, $(HEADER_EXTS), $(call find_sources_with_ext, $(SRCDIRS)))
HEADERS += $(foreach EXT, $(HEADER_EXTS), $(call find_sources_with_ext, $(INCDIRS)))

# set search paths for source files
VPATH += $(foreach SRCDIRS, $(SRCDIRS), $(SRCDIRS):)

# set target object files
RELEASE_OBJS32 := $(call create_objects, $(RELEASE_OBJDIR32), $(SRCDIRS))
RELEASE_OBJS64 := $(call create_objects, $(RELEASE_OBJDIR64), $(SRCDIRS))
DEBUG_OBJS32 := $(call create_objects, $(DEBUG_OBJDIR32), $(SRCDIRS))
DEBUG_OBJS64 := $(call create_objects, $(DEBUG_OBJDIR64), $(SRCDIRS))

.PHONY: clean debug debug32 debug64 release release32 release64 distclean $(BUILDDEPS)

all: $(BUILDDEPS) debug release
debug: debug32 debug64 
debug32: $(OUTDIR_DEBUG32)/$(OUTNAME)$(DEBUG_POSTFIX)$(OUTEXT)
debug64: $(OUTDIR_DEBUG64)/$(OUTNAME)$(DEBUG_POSTFIX)$(OUTEXT)
release: release32 release64 
release32: $(OUTDIR_RELEASE32)/$(OUTNAME)$(OUTEXT)
release64: $(OUTDIR_RELEASE64)/$(OUTNAME)$(OUTEXT)
distclean: $(BUILDDEPS) clean


clean: 
	rm -f $(DEBUG_OBJS32)
	rm -f $(DEBUG_OBJS64)
	rm -f $(RELEASE_OBJS32)
	rm -f $(RELEASE_OBJS64)
	rm -f $(OUTDIR_RELEASE32)/$(OUTNAME)$(OUTEXT)
	rm -f $(OUTDIR_RELEASE64)/$(OUTNAME)$(OUTEXT)
	rm -f $(OUTDIR_DEBUG32)/$(OUTNAME)$(DEBUG_POSTFIX)$(OUTEXT)
	rm -f $(OUTDIR_DEBUG64)/$(OUTNAME)$(DEBUG_POSTFIX)$(OUTEXT)

$(BUILDDEPS):
	cd $@ && $(MAKE) $(MAKECMDGOALS)

$(OUTDIR_RELEASE32)/$(OUTNAME)$(OUTEXT): $(RELEASE_OBJS32)
	@mkdir -p $(OUTDIR)
	@mkdir -p $(OUTDIR_RELEASE)
	@mkdir -p $(OUTDIR_RELEASE32)
	rm -f $@
	$(AR) -cq $@ $(RELEASE_OBJS32) 

$(OUTDIR_RELEASE64)/$(OUTNAME)$(OUTEXT): $(RELEASE_OBJS64)
	@mkdir -p $(OUTDIR)
	@mkdir -p $(OUTDIR_RELEASE)
	@mkdir -p $(OUTDIR_RELEASE64)
	rm -f $@
	$(AR) -cq $@ $(RELEASE_OBJS64) 

$(OUTDIR_DEBUG32)/$(OUTNAME)$(DEBUG_POSTFIX)$(OUTEXT): $(DEBUG_OBJS32)
	@mkdir -p $(OUTDIR)
	@mkdir -p $(OUTDIR_DEBUG)
	@mkdir -p $(OUTDIR_DEBUG32)
	rm -f $@
	$(AR) -cq $@ $(DEBUG_OBJS32)

$(OUTDIR_DEBUG64)/$(OUTNAME)$(DEBUG_POSTFIX)$(OUTEXT): $(DEBUG_OBJS64)
	@mkdir -p $(OUTDIR)
	@mkdir -p $(OUTDIR_DEBUG)
	@mkdir -p $(OUTDIR_DEBUG64)
	rm -f $@
	$(AR) -cq $@ $(DEBUG_OBJS64)

# compile .c files
$(DEBUG_OBJDIR32)/%.o: %.c $(HEADERS)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(DEBUG_OBJDIR)
	@mkdir -p $(DEBUG_OBJDIR32)
	$(CC) -c -o $@ $< $(CFLAGS_DEBUG32)

$(DEBUG_OBJDIR64)/%.o: %.c $(HEADERS)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(DEBUG_OBJDIR)
	@mkdir -p $(DEBUG_OBJDIR64)
	$(CC) -c -o $@ $< $(CFLAGS_DEBUG64)

$(RELEASE_OBJDIR32)/%.o: %.c $(HEADERS)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(RELEASE_OBJDIR)
	@mkdir -p $(RELEASE_OBJDIR32)
	$(CC) -c -o $@ $< $(CFLAGS_RELEASE32)

$(RELEASE_OBJDIR64)/%.o: %.c $(HEADERS)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(RELEASE_OBJDIR)
	@mkdir -p $(RELEASE_OBJDIR64)
	$(CC) -c -o $@ $< $(CFLAGS_RELEASE64)

# compile .cc files
$(DEBUG_OBJDIR32)/%.o: %.cc $(HEADERS)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(DEBUG_OBJDIR)
	@mkdir -p $(DEBUG_OBJDIR32)
	$(CXX) -c -o $@ $< $(CFLAGS_DEBUG32)

$(DEBUG_OBJDIR64)/%.o: %.cc $(HEADERS)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(DEBUG_OBJDIR)
	@mkdir -p $(DEBUG_OBJDIR64)
	$(CXX) -c -o $@ $< $(CFLAGS_DEBUG64)

$(RELEASE_OBJDIR32)/%.o: %.cc $(HEADERS)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(RELEASE_OBJDIR)
	@mkdir -p $(RELEASE_OBJDIR32)
	$(CXX) -c -o $@ $< $(CFLAGS_RELEASE32)

$(RELEASE_OBJDIR64)/%.o: %.cc $(HEADERS)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(RELEASE_OBJDIR)
	@mkdir -p $(RELEASE_OBJDIR64)
	$(CXX) -c -o $@ $< $(CFLAGS_RELEASE64)

# compile .cpp files
$(DEBUG_OBJDIR32)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(DEBUG_OBJDIR)
	@mkdir -p $(DEBUG_OBJDIR32)
	$(CXX) -c -o $@ $< $(CFLAGS_DEBUG32)

$(DEBUG_OBJDIR64)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(DEBUG_OBJDIR)
	@mkdir -p $(DEBUG_OBJDIR64)
	$(CXX) -c -o $@ $< $(CFLAGS_DEBUG64)

$(RELEASE_OBJDIR32)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(RELEASE_OBJDIR)
	@mkdir -p $(RELEASE_OBJDIR32)
	$(CXX) -c -o $@ $< $(CFLAGS_RELEASE32)

$(RELEASE_OBJDIR64)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(RELEASE_OBJDIR)
	@mkdir -p $(RELEASE_OBJDIR64)
	$(CXX) -c -o $@ $< $(CFLAGS_RELEASE64)
