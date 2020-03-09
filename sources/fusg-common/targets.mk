
PART := fusg-common


FUSG_LIB := $(BUILD_DIR)/lib$(PART).a


TARGETS += $(FUSG_LIB)

CSRCS := $(wildcard $(PART)/src/*.c)
COBJS := $(foreach COBJ, $(CSRCS:.c=.o) , $(OBJECTS_DIR)/$(COBJ))
FUSG_LIB_INCL := -I "$(PART)/src" -I "$(PART)/include"

# add specific rules for the binaries
$(FUSG_LIB): $(COBJS)
	$(AR) -cr $@  $^


# add a generic rule for each object in this folder
$(OBJECTS_DIR)/$(PART)/src/%.o: $(PART)/src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(FUSG_LIB_INCL) -c -o $@ $<
