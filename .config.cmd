deps_config := \
	thirdparty/Kconfig \
	src/encode/Kconfig \
	src/Kconfig \
	Kconfig

.config config.h: \
	$(deps_config)


$(deps_config): ;
