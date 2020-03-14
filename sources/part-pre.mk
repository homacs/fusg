

# PART
# Name of the binary to be build.
# This usually equals the directory name.
#
PART= $(shell __D="`pwd`" ; echo "$${__D\#\#/*/}" )


# All headers, sources and their associated object and dependency files.
CSRCS = $(shell find src -name "*.c" 2>/dev/null)
CHDRS = $(shell find src -name "*.h" 2>/dev/null) $(shell find include -name "*.h" 2>/dev/null)
COBJS = $(foreach COBJ, $(CSRCS:.c=.o) , $(OBJECTS_DIR)/$(PART)/$(COBJ))
CDEPS = $(foreach CDEP, $(CSRCS:.c=.d) , $(DEPENDS_DIR)/$(PART)/$(CDEP))


# These variables are supposed to be 
# extended by the part makefile to add 
# artifacts required by a the part.
HEADERS  =$(CHDRS)
INCLUDES =-I src -I include
OBJECTS  =$(COBJS)
LIBRARIES=

all:
	@echo "need target (depend, build, install, distclean, clean)"
