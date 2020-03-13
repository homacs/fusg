TOP_DIR=../..
include ../config.mk


PART = fusgd
include ../part-pre.mk


HEADERS   += $(FUSG_LIB_HDRS)
INCLUDES  += $(FUSG_LIB_INCL)
OBJECTS   += $(FUSG_LIB)
LIBRARIES +=-lauparse -laudit -lgdbm



EXECUTABLE=$(BUILD_DIR)/$(PART)




include ../part-post.mk

