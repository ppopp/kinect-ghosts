#
# ARCHITECTURE MAKEFILE INCLUDE
#
# Created by: Phillip Popp (ppopp@gracenote.com)
# On: Januray 24th, 2011
#
# Gracenote 2011.  All rights reserved.

ARCH_MAK_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
BUILD_ARCH := $(shell $(ARCH_MAK_DIR)/build_arch.sh)
BUILD_PLATFORM :=

# determine library directory based upon architecture
ifeq ($(BUILD_ARCH),MAC_X86_32)
	BUILD_PLATFORM := MAC_X86
	CFLAGS32 := -arch i386
	CPPFLAGS32 := -arch i386
	LDFLAGS32 := -arch i386
	CFLAGS64 := -arch x86_64
	CPPFLAGS64 := -arch x86_64
	LDFLAGS64 := -arch x86_64
endif

ifeq ($(BUILD_ARCH),MAC_X86_64)
	BUILD_PLATFORM := MAC_X86
	CFLAGS32 := -arch i386
	CPPFLAGS32 := -arch i386
	LDFLAGS32 := -arch i386
	CFLAGS64 := -arch x86_64
	CPPFLAGS64 := -arch x86_64
	LDFLAGS64 := -arch x86_64
endif

ifeq ($(BUILD_ARCH),LINUX_X86_32)
	BUILD_PLATFORM := LINUX_X86
	CFLAGS32 := -m32
	CPPFLAGS32 := -m32
	LDFLAGS32 := -m32
	CFLAGS64 := -m64
	CPPFLAGS64 := -m64
	LDFLAGS64 := -m64
endif

ifeq ($(BUILD_ARCH),LINUX_X86_64)
	BUILD_PLATFORM := LINUX_X86
	CFLAGS32 := -m32
	CPPFLAGS32 := -m32
	LDFLAGS32 := -m32
	CFLAGS64 := -m64
	CPPFLAGS64 := -m64
	LDFLAGS64 := -m64
endif

