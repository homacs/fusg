
ifndef __CONFIG_MK__
__CONFIG_MK__=1

ifndef TOP_DIR
TOP_DIR=..
endif


# PARTS
# Sub-directories that contain code to 
# be compiled.
PARTS=fusg-common fusgd fusg fusg-test

# BUILD_BASE
# Directory where build will be performed.
BUILD_BASE=$(TOP_DIR)


# PREFIX
# Toplevel directory to install to or distclean from
PREFIX=/usr/local


ifdef DEBUG
BUILD_DIR_NAME=Debug
CFLAGS=-g -O0 -Wall
else
BUILD_DIR_NAME=Release
CFLAGS=-O2 -Wall
endif


BUILD_DIR=$(BUILD_BASE)/$(BUILD_DIR_NAME)
OBJECTS_DIR=$(BUILD_DIR)/.objects
DEPENDS_DIR=$(OBJECTS_DIR)

SOURCES_DIR=$(TOP_DIR)/sources




-include $(foreach __PART__, $(PARTS), $(SOURCES_DIR)/$(__PART__)/exports.mk)


# For some reason, MAKEFLAGS don't work properly in recursive calls.
define MAKE_PARTS =
	$(MAKE) -j --jobserver-fds=3,4 -C fusg-common $@
	$(MAKE) -j --jobserver-fds=3,4 -C fusgd $@
	$(MAKE) -j --jobserver-fds=3,4 -C fusg $@
	$(MAKE) -j --jobserver-fds=3,4 -C fusg-test $@
endef	




SHELL=/bin/bash



endif # __CONFIG_MK__