


include ./config.mk

include ./rules.mk


include $(foreach PART, $(PARTS), $(SOURCES_DIR)/$(PART)/targets.mk)


all: $(TARGETS)


clean:
	rm -r $(OBJECTS_DIR)