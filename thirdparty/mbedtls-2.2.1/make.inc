ifeq ($(ZMD_APP_THI_MBEDTLS),y)
ZMD_INCLUDE += -I$(ZMD_TIR_ROOT)/mbedtls-2.2.1/include/mbedtls
ZMD_TIR_LIB += $(wildcard $(ZMD_TIR_ROOT)/mbedtls-2.2.1/lib/*.a)
endif
