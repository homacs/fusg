PART := fusg-common


FUSG_LIB      := $(BUILD_DIR)/lib$(PART).a
FUSG_LIB_HDRS := $(wildcard $(SOURCES_DIR)/$(PART)/include/fusg/*.h) 
FUSG_LIB_INCL :=        -I "$(SOURCES_DIR)/$(PART)/include"
