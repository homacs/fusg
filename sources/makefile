
include ./config.mk



# Run through build stages
# 1. depend: build dependency files
# 2. build: compile and link binaries
all:
	@$(MAKE) -j8 depend
	@$(MAKE) -j8 build


depend:
	@echo "====================== $@ ========================"
	+@$(MAKE_PARTS)

build: 
	@echo "====================== $@ ========================"
	+@$(MAKE_PARTS)


install:
	@echo "====================== $@ ========================"
	+@$(MAKE_PARTS)

distclean:
	@echo "====================== $@ ========================"
	+@$(MAKE_PARTS)


clean:
	rm -rf $(BUILD_DIR)


