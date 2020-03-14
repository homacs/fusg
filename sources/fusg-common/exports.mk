PART := fusg-common


FUSG_LIB      := $(BUILD_DIR)/lib$(PART).a
FUSG_LIB_HDRS := $(shell find $(SOURCES_DIR)/$(PART)/include -name "*.h") 
FUSG_LIB_INCL :=          -I "$(SOURCES_DIR)/$(PART)/include"
