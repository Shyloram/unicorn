build/kconfig/mconf:
	@make -C build/kconfig ncurses mconf

menuconfig: build/kconfig/mconf
	@$< Kconfig

kclean:
	@make -C build/kconfig clean

kdistclean: kclean
	-rm -rf .config .config.cmd .config.old zmdconfig.h

.PHONY: menuconfig kclean
