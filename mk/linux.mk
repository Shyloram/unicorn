#define the dir var
ZMD_OUT_ROOT     	:= $(ZMD_APP_TOPDIR)/out
ZMD_LIB_ROOT_LINUX  := $(ZMD_LIB_ROOT)/linux

CreateDir       	 = $(shell [ -d $1 ] || mkdir -p $1 || echo ":mkdir '$1' fail")
RemoveDir       	 = $(shell [ -d $1 ] && rm -rf $1 && echo -e "rmdir '$1'\t [ OK ]" || echo ":rm dir '$1' fail")

dummy 				:= $(call CreateDir, $(ZMD_OUT_ROOT))

ifeq ($(ZMD_APP_HI3516E300),y)
HIARCH              := hi3516ev200
endif

CROSS               := arm-himix100-linux-
CC 					:=$(CROSS)gcc
GPP 				:=$(CROSS)g++
AR 					:=$(CROSS)ar
LD 					:=$(CROSS)ld

SENSOR_TYPE 		:= SMART_SC2235_DC_2M_30FPS_12BIT
ISP_VERSION 		:= ISP_V2
HI_FPGA 			:= HI_XXXX

LIBS_CFLAGS 		+= -fno-aggressive-loop-optimizations -ldl -ffunction-sections -fdata-sections -O2
LIBS_CFLAGS 		+= -fstack-protector-strong -fPIC

MPP_CFLAGS  		+= -DHI_RELEASE
MPP_CFLAGS  		+= -Wno-error=implicit-function-declaration
#********************* Macro for version management***************************** 
VER_X 				:= 1
VER_Y 				:= 0
VER_Z 				:= 0
VER_P 				:= 0
VER_B 				:= 10
MPP_CFLAGS 			+= -DVER_X=$(VER_X) -DVER_Y=$(VER_Y) -DVER_Z=$(VER_Z) -DVER_P=$(VER_P) -DVER_B=$(VER_B)
#******************************************************************************* 
MPP_CFLAGS  		+= -DUSER_BIT_32 -DKERNEL_BIT_32 -Wno-date-time -D_GNU_SOURCE

CFLAGS 				:= -Wall -g -D$(HIARCH) -D$(HI_FPGA) -lpthread -lm -ldl -D$(ISP_VERSION)
CFLAGS 				+= -lstdc++
CFLAGS 				+= $(LIBS_CFLAGS)
CFLAGS 				+= $(MPP_CFLAGS)
CFLAGS 				+= -DSENSOR0_TYPE=$(SENSOR_TYPE)  

ifeq ($(ZMD_APP_ENCODE_AUDIO),y)
CFLAGS 				+= -DHI_ACODEC_TYPE_INNER
endif

SENSOR_LIBS         := $(ZMD_LIB_ROOT_LINUX)/lib_hiae.a
SENSOR_LIBS         += $(ZMD_LIB_ROOT_LINUX)/libisp.a                                                                                                                                         
SENSOR_LIBS         += $(ZMD_LIB_ROOT_LINUX)/lib_hidehaze.a
SENSOR_LIBS         += $(ZMD_LIB_ROOT_LINUX)/lib_hidrc.a
SENSOR_LIBS         += $(ZMD_LIB_ROOT_LINUX)/lib_hildci.a
SENSOR_LIBS         += $(ZMD_LIB_ROOT_LINUX)/lib_hiawb.a
SENSOR_LIBS 		+= $(ZMD_LIB_ROOT_LINUX)/libsns_sc2235.a
MPI_LIBS 			:= $(ZMD_LIB_ROOT_LINUX)/libmpi.a
AUDIO_LIBA          := $(ZMD_LIB_ROOT_LINUX)/libVoiceEngine.a \
					   $(ZMD_LIB_ROOT_LINUX)/libupvqe.a \
					   $(ZMD_LIB_ROOT_LINUX)/libdnvqe.a

LINUX_LIBDEP 		:= -Wl,--start-group $(MPI_LIBS) $(SENSOR_LIBS) $(AUDIO_LIBA) $(ZMD_LIB_ROOT_LINUX)/libsecurec.a $(ZMD_LIB) $(ZMD_TIR_LIB) -Wl,--end-group

ZMD_OBJS 			:= $(patsubst %.c,%.o,$(ZMD_SRCS))
ZMD_CPPOBJS 		:= $(patsubst %.cpp,%.o,$(ZMD_CPPSRCS))

TARGET 				 = sample
SAMPLELIB   		 = $(ZMD_OUT_LIB_ROOT)/libsample.a

all: $(TARGET)
	cp $(ZMD_OUT_ROOT)/$(TARGET) ~/nfs
	@echo "" 
	@echo "" 
	@echo "--------------make finish-------------" 
	
clean: kclean
	-rm -rf $(ZMD_CPPOBJS)
	-rm -rf $(ZMD_OBJS)
	-rm -rf $(ZMD_OUT_ROOT)

%.o:%.c
	$(CC) $(CFLAGS) $(ZMD_INCLUDE) -c $< -o $@

%.o:%.cpp
	$(GPP) $(CFLAGS) $(ZMD_INCLUDE) -c $< -o $@

$(SAMPLELIB):$(ZMD_OBJS) $(ZMD_CPPOBJS)
	$(AR) -rcs $@ $(ZMD_OBJS) $(ZMD_CPPOBJS)

$(TARGET):$(ZMD_OBJS) $(ZMD_CPPOBJS)
	$(GPP) $(CFLAGS) -o $(ZMD_OUT_ROOT)/$@ $^ $(LINUX_LIBDEP)

distclean: clean kdistclean

.PHONY: all clean distclean liteos_image liteos_image_clean show
