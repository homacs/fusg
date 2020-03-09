
ifndef __CONFIG_MK__
__CONFIG_MK__=1


PARTS= fusg-common fusgd fusg   fusg-test




ifndef TOP_DIR
TOP_DIR=..
endif




ifdef DEBUG
BUILD_DIR_NAME=Debug
CFLAGS=-g -O0 -Wall
else
BUILD_DIR_NAME=Release
CFLAGS=-O2 -Wall
endif


BUILD_DIR=$(TOP_DIR)/$(BUILD_DIR_NAME)
OBJECTS_DIR=$(BUILD_DIR)/.objects
SOURCES_DIR=$(TOP_DIR)/sources

endif # __CONFIG_MK__