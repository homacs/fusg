
include ./config.mk



BUILD_STAGES = depend build



all:
	@$(MAKE) -j8 depend
	@$(MAKE) -j8 build

depend:
	@$(MAKE) -j --jobserver-fds=3,4 -C fusg-common $@
	@$(MAKE) -j --jobserver-fds=3,4 -C fusgd $@
	@$(MAKE) -j --jobserver-fds=3,4 -C fusg $@
	@$(MAKE) -j --jobserver-fds=3,4 -C fusg-test $@

build:
	@$(MAKE) -j --jobserver-fds=3,4 -C fusg-common $@
	@$(MAKE) -j --jobserver-fds=3,4 -C fusgd $@
	@$(MAKE) -j --jobserver-fds=3,4 -C fusg $@
	@$(MAKE) -j --jobserver-fds=3,4 -C fusg-test $@

clean:
	rm -rf $(BUILD_DIR)


