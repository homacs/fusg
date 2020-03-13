CSRCS = $(wildcard src/*.c)
CHDRS = $(wildcard src/*.h) $(wildcard include/*.h)
COBJS = $(foreach COBJ, $(CSRCS:.c=.o) , $(OBJECTS_DIR)/$(PART)/$(COBJ))
CDEPS = $(foreach CDEP, $(CSRCS:.c=.d) , $(DEPENDS_DIR)/$(PART)/$(CDEP))

HEADERS=$(CHDRS)
OBJECTS=$(COBJS)
INCLUDES=-I src -I include
LIBRARIES=

