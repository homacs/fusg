
include ./config.mk



# run through build stages
# 1. depend: build dependency files
# 2. build: compile and link binaries
all:
	@$(MAKE) -j8 depend
	@$(MAKE) -j8 build

depend:
	+@$(EXEC_PARTS)

build: 
	+@$(EXEC_PARTS)



clean:
	rm -rf $(BUILD_DIR)


