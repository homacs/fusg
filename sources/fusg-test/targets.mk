
PART := fusg-test

FUSG_TEST := $(BUILD_DIR)/$(PART)


TARGETS += $(FUSG_TEST)

CSRCS := $(wildcard $(PART)/src/*.c)
COBJS := $(foreach COBJ, $(CSRCS:.c=.o) , $(OBJECTS_DIR)/$(COBJ))
FUSGD_INCL := -I "$(PART)/src" -I "fusg-common/include"


# add specific rules for the binaries
$(FUSG_TEST): $(COBJS) $(FUSG_LIB)
	$(CC) $(LDFLAGS) -o $@  $^ -lauparse -laudit -lgdbm


# add a generic rule for each object in this folder
$(OBJECTS_DIR)/$(PART)/src/%.o: $(PART)/src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(FUSGD_INCL) -c -o $@ $<
	

