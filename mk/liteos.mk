#define the dir var
ZMD_OUT_ROOT     	:= $(ZMD_APP_TOPDIR)/out
ZMD_OUT_LIB_ROOT 	:= $(ZMD_APP_TOPDIR)/out/lib
TMP_DIR 			:= $(ZMD_APP_TOPDIR)/out/tmp/
OBJ_DIR 			:= $(ZMD_APP_TOPDIR)/out/lib/OBJ
LITEOSTOPDIR        := $(ZMD_APP_TOPDIR)/../liteos/

export LITEOSTOPDIR

CreateDir       	 = $(shell [ -d $1 ] || mkdir -p $1 || echo ":mkdir '$1' fail")
RemoveDir       	 = $(shell [ -d $1 ] && rm -rf $1 && echo -e "rmdir '$1'\t [ OK ]" || echo ":rm dir '$1' fail")

dummy 				:= $(call CreateDir, $(ZMD_OUT_ROOT))
dummy 				:= $(call CreateDir, $(ZMD_OUT_ROOT)/bin)
dummy 				:= $(call CreateDir, $(ZMD_OUT_ROOT)/burn)
dummy 				:= $(call CreateDir, $(ZMD_OUT_LIB_ROOT))
dummy 				:= $(call CreateDir, $(TMP_DIR))
dummy 				:= $(call CreateDir, $(OBJ_DIR))

ifeq ($(ZMD_APP_HI3518E200),y)
CHIP_ID 			:= CHIP_HI3518E_V200
HIARCH              := hi3518e
HICHIP 				:= 0x3518E200
LITEOS_PLATFORM 	:= hi3518ev200
LIBS_CFLAGS    		:= -mcpu=arm926ej-s
endif

ifeq ($(ZMD_APP_HISI_LITEOS_INIT_SENSOR_IN_UBOOT),y)
VSS_DEFS 			+= -DCFG_INIT_SENSOR_IN_UBOOT
endif

ifeq ($(CFG_SCATTER_FLAG),yes)
VSS_DEFS 			+= -DCFG_SCATTER_FLAG
endif

ifeq ($(CFG_FAST_IMAGE),yes)
VSS_DEFS 			+= -DCFG_FAST_IMAGE
endif

ifeq ($(CHIP_ID),)
$(error CHIP ID not defined! Please check!)
endif

SENSOR_TYPE 		:= OMNIVISION_OV2235_DC_1080P_20FPS
ISP_VERSION 		:= ISP_V2
HIDBG 				:= HI_RELEASE
HI_FPGA 			:= HI_XXXX

LIBS_CFLAGS 		+=  -mno-unaligned-access -fno-aggressive-loop-optimizations -ffunction-sections -fdata-sections

#********************* Macro for version management***************************** 
VER_X 				:= 1
VER_Y 				:= 0
VER_Z 				:= 0
VER_P 				:= 0
VER_B 				:= 10
MPP_CFLAGS 			+= -DVER_X=$(VER_X) -DVER_Y=$(VER_Y) -DVER_Z=$(VER_Z) -DVER_P=$(VER_P) -DVER_B=$(VER_B)
#******************************************************************************* 

include $(LITEOSTOPDIR)/config.mk

CFLAGS 				:= -fno-builtin -nostdinc -nostdlib
CFLAGS 				+= $(LITEOS_MACRO) $(LITEOS_OSDRV_INCLUDE) $(LITEOS_USR_INCLUDE)
CFLAGS 				+= $(LIBS_CFLAGS)
CFLAGS 				+= $(MPP_CFLAGS)
CFLAGS 				+= -D__HuaweiLite__ -D__KERNEL__ -D__LITEOS__
CFLAGS 				+= -Wall -g -D$(HIARCH) -DHICHIP=$(HICHIP) -DSENSOR_TYPE=$(SENSOR_TYPE) -D$(HIDBG) -D$(HI_FPGA) -DCHIP_ID=$(CHIP_ID) -lpthread -lm -ldl -D$(ISP_VERSION)
CFLAGS 				+= -D$(LITEOS_PLATFORM)
CFLAGS 				+= -DLCD_ILI9342

ifeq ($(ZMD_APP_WIFI),y)
CFLAGS 				+= -DCONFIG_DRIVER_HI1131    
CFLAGS 				+= -DCONFIG_NO_CONFIG_WRITE
endif

ifeq ($(ZMD_APP_ENCODE_AUDIO),y)
CFLAGS 				+= -DHI_ACODEC_TYPE_INNER
endif

ifeq ($(LITEOS_PLATFORM),hi3518ev200)
SDK_LIB 			:= -lstream -lcontrol -lmpi -lhi_osal -lhi3518e_sys -lhi3518e_viu -lhi3518e_vpss -lhi3518e_vou -lhi_mipi -lhi_piris\
           				-lhi_sensor_i2c -lhi_sensor_spi -lisp -lhi3518e_isp -lhi_pwm -lhi3518e_vgs -lhi3518e_venc -lhi3518e_rc \
           				-lhi3518e_chnl -lhi3518e_h264e -lhi3518e_jpege -lhi3518e_region -ltde -lhi3518e_tde -lhi_adv7179 -lhi_cipher\
           				-lhi3518e_base -lupvqe -ldnvqe -lVoiceEngine -lhifb -lhi3518e_ive -live -lmd -lhi3518e_adec -lhi3518e_aenc -lhi_cipher \
           				-lhi3518e_aio -lhi3518e_ai -lacodec -lhi3518e_ao -l_hiae -l_hiawb -l_hiaf -l_hidefog -l_hiirauto   
endif

SENSOR_LIB 			:= $(ZMD_LIB_ROOT)/liteos/libsns_sc2235.a

LITEOS_LIBDEP 		:= --start-group $(SDK_LIB)  $(ZMD_LIB) $(ZMD_TIR_LIB) -lsample $(LITEOS_LIBS) --end-group

SAMPLE_LDFLAGS 		:=
SAMPLE_LDFLAGS 		+= -L$(LITEOSTOPDIR)/tools/scripts/ld
SAMPLE_LDFLAGS 		+= -L$(LITEOSTOPDIR)/platform/bsp/$(LITEOS_PLATFORM)
SAMPLE_LDFLAGS 		+= -T$(LITEOSTOPDIR)/liteos.ld
SAMPLE_LDFLAGS 		+= -L$(ZMD_OUT_LIB_ROOT)/OBJ -L$(ZMD_OUT_LIB_ROOT)
SAMPLE_LDFLAGS 		+= $(LITEOS_TABLES_LDFLAGS)

ZMD_INCLUDE 		+= -I$(LITEOSTOPDIR)/platform/bsp/$(LITEOS_PLATFORM)/include
ZMD_OBJS 			:= $(patsubst %.c,%.o,$(ZMD_SRCS))
ZMD_CPPOBJS 		:= $(patsubst %.cpp,%.o,$(ZMD_CPPSRCS))

TARGET 				 = sample
SAMPLELIB   		 = $(ZMD_OUT_LIB_ROOT)/libsample.a

all: $(TARGET)
	@echo "--------------make finish-------------" 
	
clean: kclean
	-rm -rf $(ZMD_CPPOBJS)
	-rm -rf $(ZMD_OBJS)
	-rm -rf $(ZMD_OUT_ROOT)

%.o:%.c
	$(CC) $(CFLAGS) $(LITEOS_CFLAGS) $(VSS_DEFS) $(ZMD_INCLUDE) -c $< -o $@

%.o:%.cpp
	$(GPP) $(CFLAGS) $(VSS_DEFS) $(ZMD_INCLUDE) -c $< -o $@

$(SAMPLELIB):$(ZMD_OBJS) $(ZMD_CPPOBJS)
	$(AR) -rcs $@ $(ZMD_OBJS) $(ZMD_CPPOBJS)

lib_copy:
	# This step is very important. All the lib files should be copied to the $(ZMD_OUT_LIB_ROOT) .
	@cp -rf $(LITEOSTOPDIR)/out/$(LITEOS_PLATFORM)/lib/*.a     $(ZMD_OUT_LIB_ROOT)
	@cp -rf $(ZMD_LIB_ROOT)/liteos/*.a    $(ZMD_OUT_LIB_ROOT)
	@cp -rf $(ZMD_LIB_ROOT)/liteos/extdrv/*.a    $(ZMD_OUT_LIB_ROOT)
	# scatter
	if [[ "$(CFG_SCATTER_FLAG)" == "yes" && "$(CFG_FAST_IMAGE)" != "yes" ]];then \
		$(LITEOSTOPDIR)/tools/scripts/scatter_sr/scatter.sh  $(CROSS_COMPILE)   n    y    $(LITEOSTOPDIR)/tools/scripts/scatter_sr \
		$(LITEOSTOPDIR)/tools/scripts/ld     $(TMP_DIR)    $(OBJ_DIR)  ;\
	fi

$(TARGET):  $(SAMPLELIB) lib_copy
	$(LD) $(SAMPLE_LDFLAGS) --gc-sections -Map=$(ZMD_OUT_ROOT)/bin/sample.map -o $(ZMD_OUT_ROOT)/bin/sample $(LITEOS_LIBDEP)
	$(OBJCOPY) -O binary $(ZMD_OUT_ROOT)/bin/sample $(ZMD_OUT_ROOT)/burn/sample.bin
	$(OBJDUMP) -d $(ZMD_OUT_ROOT)/bin/sample >$(ZMD_OUT_ROOT)/bin/sample.asm
	@chmod u+x $(LITEOSTOPDIR)/tools/scripts/scatter_sr/scatter_size.sh 
	$(LITEOSTOPDIR)/tools/scripts/scatter_sr/scatter_size.sh    $(ZMD_OUT_ROOT)/bin/sample
	@chmod u+x $(ZMD_TOOLS_ROOT)/hi_lzma
	$(ZMD_TOOLS_ROOT)/hi_lzma   $(ZMD_OUT_ROOT)/burn/sample.bin  $(ZMD_OUT_ROOT)/burn/sample.bin.lzma

scatter:
	@chmod u+x $(ZMD_APP_TOPDIR)/script/gen_image.sh
	$(ZMD_APP_TOPDIR)/script/gen_image.sh
	$(ZMD_SCRIPT_ROOT)/makescatterlzma.sh $(ZMD_OUT_ROOT)/bin/sample $(ZMD_APP_TOPDIR)

sclean: clean
	-cp $(ZMD_SCRIPT_ROOT)/wow_orignal.ld    $(LITEOSTOPDIR)/tools/scripts/ld/wow.ld   -f
	-cp $(ZMD_SCRIPT_ROOT)/scatter_orignal.ld     $(LITEOSTOPDIR)/tools/scripts/ld/scatter.ld  -f
	@if [[ -f  $(LITEOSTOPDIR)/tools/scripts/scatter_sr/liblist.sh.bak ]]; then \
		rm $(LITEOSTOPDIR)/tools/scripts/scatter_sr/liblist.sh; \
		mv $(LITEOSTOPDIR)/tools/scripts/scatter_sr/liblist.sh.bak $(LITEOSTOPDIR)/tools/scripts/scatter_sr/liblist.sh;\
	fi

distclean: clean kdistclean

.PHONY: all clean distclean liteos_image liteos_image_clean show
