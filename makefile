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
OUTNAME := ghosts
OUTDIR := bin
OUTEXT := 
DEBUG_POSTFIX := d

# compiler flags / search paths
INCDIRS := inc /lol/include
SRCDIRS := src
FRAMEWORKS := OpenGL GLUT
# Libraries that use this makefile system can be added here.  Simply add the
# directories of the makefiles to the PROJECTDIRS variable and they will be
# automatically linked in to the application. Remember to still add the names
# of the libraries to LINK_RELEASE_LIBS and LINK_DEBUG_LIBS
PROJECTDIRS := 
# If you have code that uses other libraries you can add in their directories here
LIBDIRS := /lol/lib
LIBDIRS_DEBUG32 := $(LIBDIRS) 
LIBDIRS_DEBUG64 := $(LIBDIRS) 
LIBDIRS_RELEASE32 := $(LIBDIRS) 
LIBDIRS_RELEASE64 := $(LIBDIRS) 
LINK_LIBS := GLEW freenect
LINK_RELEASE_LIBS := $(LINK_LIBS)
LINK_DEBUG_LIBS := $(LINK_LIBS)
PPDEFINES := 
LFLAGS := 

# file extensions
SRC_EXTS := .c .cc .cpp
HEADER_EXTS := .h

# location of make files to be called before this one
BUILDDEPS := 

# files to be copied into output directory
COPYFILES_DEBUG32 := 
COPYFILES_DEBUG64 := 
COPYFILES_RELEASE32 := 
COPYFILES_RELEASE64 := 

#####################################################################
### most users of this makefile won't need to edit anything below.###
#####################################################################

LIBDIRS_DEBUG32 := $(LIBDIRS_DEBUG32) $(foreach libdir, $(PROJECTDIRS), $(libdir)/lib/debug/32)
LIBDIRS_DEBUG64 := $(LIBDIRS_DEBUG64) $(foreach libdir, $(PROJECTDIRS), $(libdir)/lib/debug/64)
LIBDIRS_RELEASE32 := $(LIBDIRS_RELEASE32) $(foreach libdir, $(PROJECTDIRS), $(libdir)/lib/release/32)
LIBDIRS_RELEASE64 := $(LIBDIRS_RELEASE64) $(foreach libdir, $(PROJECTDIRS), $(libdir)/lib/release/64)

LFLAGS := $(LFLAGS) $(foreach framework, $(FRAMEWORKS), -framework $(framework))
#CFLAGS := $(CFLAGS) $(foreach framework, $(FRAMEWORKS), -framework $(framework))
LFLAGS_RELEASE32 := $(LFLAGS) $(LDFLAGS32) $(foreach LIBDIRS_RELEASE32, $(LIBDIRS_RELEASE32), -L$(LIBDIRS_RELEASE32)) 
LFLAGS_RELEASE64 := $(LFLAGS) $(LDFLAGS64) $(foreach LIBDIRS_RELEASE64, $(LIBDIRS_RELEASE64), -L$(LIBDIRS_RELEASE64)) 
LFLAGS_DEBUG32 := $(LFLAGS) $(LDFLAGS32) $(foreach LIBDIRS_DEBUG32, $(LIBDIRS_DEBUG32), -L$(LIBDIRS_DEBUG32)) 
LFLAGS_DEBUG64 := $(LFLAGS) $(LDFLAGS64) $(foreach LIBDIRS_DEBUG64, $(LIBDIRS_DEBUG64), -L$(LIBDIRS_DEBUG64)) 

LIBFLAGS_RELEASE := $(foreach LINK_RELEASE_LIBS, $(LINK_RELEASE_LIBS), -l$(LINK_RELEASE_LIBS))
LIBFLAGS_DEBUG := $(foreach LINK_DEBUG_LIBS, $(LINK_DEBUG_LIBS), -l$(LINK_DEBUG_LIBS))

LIBFLAGS_RELEASE32 := $(LIBFLAGS_RELEASE)
LIBFLAGS_RELEASE64 := $(LIBFLAGS_RELEASE)
LIBFLAGS_DEBUG32 := $(LIBFLAGS_DEBUG)
LIBFLAGS_DEBUG64 := $(LIBFLAGS_DEBUG)

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
CFLAGS := $(CFLAGS) $(foreach PPDEFINES, $(PPDEFINES), -D$(PPDEFINES))
CFLAGS := $(CFLAGS) -fPIC -Wall 
CFLAGS_DEBUG := $(CFLAGS) -g
CFLAGS_DEBUG32 := $(CFLAGS_DEBUG) $(CFLAGS32)
CFLAGS_DEBUG64 := $(CFLAGS_DEBUG) $(CFLAGS64)
CFLAGS_RELEASE := $(CFLAGS) -DNDEBUG -O2
CFLAGS_RELEASE32 := $(CFLAGS_RELEASE) $(CFLAGS32)
CFLAGS_RELEASE64 := $(CFLAGS_RELEASE) $(CFLAGS64)

CC = gcc
AR = ar

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

.PHONY: clean debug debug32 debug64 release release32 release64 dependencies dist_clean post $(BUILDDEPS) $(LIBFLAGS_RELEASE) $(LIBFLAGS_DEBUG)

#all: $(BUILDDEPS) debug release post
all: $(BUILDDEPS) debug64 release64 post
debug: debug32 debug64
debug32: $(BUILDDEPS) $(OUTDIR_DEBUG32)/$(OUTNAME)$(DEBUG_POSTFIX)$(OUTEXT)
debug64: $(BUILDDEPS) $(OUTDIR_DEBUG64)/$(OUTNAME)$(DEBUG_POSTFIX)$(OUTEXT)
release: release32 release64
release32: $(BUILDDEPS) $(OUTDIR_RELEASE32)/$(OUTNAME)$(OUTEXT)
release64: $(BUILDDEPS) $(OUTDIR_RELEASE64)/$(OUTNAME)$(OUTEXT)
dist_clean: $(BUILDDEPS) clean
post: $(COPYFILES_DEBUG32) $(COPYFILES_RELEASE32) $(COPYFILES_DEBUG64) $(COPYFILES_RELEASE64)

clean: 
	rm -f $(DEBUG_OBJS32)
	rm -f $(DEBUG_OBJS64)
	rm -f $(RELEASE_OBJS32)
	rm -f $(RELEASE_OBJS64)
	rm -f $(OUTDIR_RELEASE32)/$(OUTNAME)$(OUT_EXT)
	rm -f $(OUTDIR_RELEASE64)/$(OUTNAME)$(OUT_EXT)
	rm -f $(OUTDIR_DEBUG32)/$(OUTNAME)$(DEBUG_POSTFIX)$(OUT_EXT)
	rm -f $(OUTDIR_DEBUG64)/$(OUTNAME)$(DEBUG_POSTFIX)$(OUT_EXT)

$(COPYFILES_DEBUG32): debug $(BUILDDEPS)
	cp $@ $(OUTDIR_DEBUG32)

$(COPYFILES_DEBUG64): debug $(BUILDDEPS)
	cp $@ $(OUTDIR_DEBUG64)

$(COPYFILES_RELEASE32): release $(BUILDDEPS)
	cp $@ $(OUTDIR_RELEASE32)

$(COPYFILES_RELEASE64): release $(BUILDDEPS)
	cp $@ $(OUTDIR_RELEASE64)

$(BUILDDEPS):
	cd $@ && $(MAKE) $(MAKECMDGOALS)

$(OUTDIR_RELEASE32)/$(OUTNAME)$(OUTEXT): $(RELEASE_OBJS32) $(LIBFLAGS_RELEASE32) 
	@mkdir -p $(OUTDIR)
	@mkdir -p $(OUTDIR_RELEASE)
	@mkdir -p $(OUTDIR_RELEASE32)
	rm -f $@
	$(CC) $(LFLAGS_RELEASE32) $^ -o $@

$(OUTDIR_RELEASE64)/$(OUTNAME)$(OUTEXT): $(RELEASE_OBJS64) $(LIBFLAGS_RELEASE64)
	@mkdir -p $(OUTDIR)
	@mkdir -p $(OUTDIR_RELEASE)
	@mkdir -p $(OUTDIR_RELEASE64)
	rm -f $@
	$(CC) $(LFLAGS_RELEASE64) $^ -o $@

$(OUTDIR_DEBUG32)/$(OUTNAME)$(DEBUG_POSTFIX)$(OUTEXT): $(DEBUG_OBJS32) $(LIBFLAGS_DEBUG32)
	@mkdir -p $(OUTDIR)
	@mkdir -p $(OUTDIR_DEBUG)
	@mkdir -p $(OUTDIR_DEBUG32)
	rm -f $@
	$(CC) $(LFLAGS_DEBUG32) $^ -o $@

$(OUTDIR_DEBUG64)/$(OUTNAME)$(DEBUG_POSTFIX)$(OUTEXT): $(DEBUG_OBJS64) $(LIBFLAGS_DEBUG64)
	@mkdir -p $(OUTDIR)
	@mkdir -p $(OUTDIR_DEBUG)
	@mkdir -p $(OUTDIR_DEBUG64)
	rm -f $@
	$(CC) $(LFLAGS_DEBUG64) $^ -o $@

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
	$(CC) -c -o $@ $< $(CFLAGS_DEBUG32)

$(DEBUG_OBJDIR64)/%.o: %.cc $(HEADERS)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(DEBUG_OBJDIR)
	@mkdir -p $(DEBUG_OBJDIR64)
	$(CC) -c -o $@ $< $(CFLAGS_DEBUG64)

$(RELEASE_OBJDIR32)/%.o: %.cc $(HEADERS)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(RELEASE_OBJDIR)
	@mkdir -p $(RELEASE_OBJDIR32)
	$(CC) -c -o $@ $< $(CFLAGS_RELEASE32)

$(RELEASE_OBJDIR64)/%.o: %.cc $(HEADERS)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(RELEASE_OBJDIR)
	@mkdir -p $(RELEASE_OBJDIR64)
	$(CC) -c -o $@ $< $(CFLAGS_RELEASE64)

# compile .cpp files
$(DEBUG_OBJDIR32)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(DEBUG_OBJDIR)
	@mkdir -p $(DEBUG_OBJDIR32)
	$(CC) -c -o $@ $< $(CFLAGS_DEBUG32)

$(DEBUG_OBJDIR64)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(DEBUG_OBJDIR)
	@mkdir -p $(DEBUG_OBJDIR64)
	$(CC) -c -o $@ $< $(CFLAGS_DEBUG64)

$(RELEASE_OBJDIR32)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(RELEASE_OBJDIR)
	@mkdir -p $(RELEASE_OBJDIR32)
	$(CC) -c -o $@ $< $(CFLAGS_RELEASE32)

$(RELEASE_OBJDIR64)/%.o: %.cpp $(HEADERS)
	@mkdir -p $(OBJDIR)
	@mkdir -p $(RELEASE_OBJDIR)
	@mkdir -p $(RELEASE_OBJDIR64)
	$(CC) -c -o $@ $< $(CFLAGS_RELEASE64)

