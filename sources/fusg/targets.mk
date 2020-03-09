

PART := fusg

FUSG_BIN := $(BUILD_DIR)/$(PART)


TARGETS += $(FUSG_BIN)


CSRCS := $(wildcard $(PART)/src/*.c)
COBJS := $(foreach COBJ, $(CSRCS:.c=.o) , $(OBJECTS_DIR)/$(COBJ))
FUSG_INCL := -I "$(PART)/src" -I "fusg-common/include"


# add specific rules for the binaries
$(FUSG_BIN): $(COBJS) $(FUSG_LIB)
	$(CC) $(LDFLAGS) -o $@  $^ -lgdbm


# add a generic rule for each object in this folder
$(OBJECTS_DIR)/$(PART)/src/%.o: $(PART)/src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(FUSG_INCL) -c -o $@ $<
	



