/******************************************************************************

  Copyright (C), 2012-2050, Hisilicon Tech. Co., Ltd.

******************************************************************************
  File Name     : Hi_comm_def.h
  Version       : Initial Draft
  Created       : 2013/10/17
  Description   :comm struct definations
  History       :
  1.Date        : 2013/10/17
    Modification: This file is created.
******************************************************************************/

#ifndef __HI_COMM_DEF_H__
#define __HI_COMM_DEF_H__


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */


#include "hi_type.h"

#include <unistd.h>
#include <errno.h>

#include <linux/reboot.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifndef __LITEOS__

#include <dlfcn.h>
#include <netinet/in.h>
#include <sys/reboot.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <sys/stat.h>
#include <stdarg.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <semaphore.h>

#include <sys/socket.h>

#include "hi_hal_def.h"



#define PQ_DBG_ZERO_DATA  8
#define BACKLOG 20
#define CMDBUF_SIZE (0xFF)
#define OFFSET_OF(s, m) (size_t)&(((s *)0)->m)
#define GET_VALUE(buf, s, m) ((*(s *)buf).m)
#define MAX_DATA_LEN 257*4
#define CMD_OFFSET 4
#define CMD_MAX_NUM 14
#define VIRTUAL_REG_HEAD 0xFF000000
#define DDR_REG_HEAD 0xFF000000
#define DATA_OFFSET 8
#define ECHO_BYE 0xB0



/*-----------------------------------------------------------------------------*/
/* DEFINE DBG_API_DUMP            0xFF090000                                                                 */
/*------------------------------------------------------------------------- --*/


#define HI_ERR_NEGATIVE_LEN             (-12)
#define HI_ERR_MALLOC_FAILED            (-11)

#define HI_ERR_OUT_OF_VIR_ADD_RANGE     (-8)
#define HI_ERR_NO_SUCH_CMD              (-7)
#define HI_ERR_INVALID_REG_TYPE         (-6)
#define HI_ERR_INVALID_ADDR_TYPE        (-5)

#define HI_ERR_RREG_REG_FAILED          (-4)
#define HI_ERR_RREG_VIRREG_FAILED       (-2)

#define HI_ERR_RREG_ISMPI               0x02
#define HI_ERR_RREG_SENDDATA            0x04
#define HI_ERR_RREG_SENDEND             0x05


#define HI_ERR_GET_MPI_FAILED           0x11
#define HI_ERR_SET_MPI_FAILED           0x12
#define HI_ERR_READ_VIR_REGS_FAILED     0x13
#define HI_ERR_WRITE_VIR_REGS_FAILED    0x14
#define HI_ERR_READ_REGS_FAILED         0x15
#define HI_ERR_WRITE_REGS_FAILED        0x16


#define HI_ERR_GET_INI_ITEM_FAILED      0x21
#define HI_ERR_OPEN_FILE_FAILED         0x22

#define HI_ERR_PARSE_BIN_FILE_FAILED    0x31
#define HI_ERR_GENERATE_BIN_FILE_FAILED 0x32


#define HI_ERR_INVALID_RAW_CMD          0x51
#define HI_ERR_GET_RAW_BUFLEN_FAILED    0x52
#define HI_ERR_START_RAW_FAILED         0x53
#define HI_ERR_START_DOING_FPN         0x54

#define HI_ERR_GET_YUV_BUFLEN_FAILED    0x61
#define HI_ERR_START_YUV_CMD_FAILED     0x62
#define HI_ERR_START_YUV_VI_FAILED      0x63
#define HI_ERR_START_YUV_VPSS_FAILED    0x64

#define HI_ERR_GET_DEBUG_BUFLEN_FAILED  0x71
#define HI_ERR_DEBUG_INVALID_PARAM      0x72
#define HI_ERR_DEBUG_START_FAILED       0x73

#define HI_ERR_LOAD_RAW_TIMEOUT         0x81
#define HI_ERR_LOAD_RAW_DATAERROR       0x82
#define HI_ERR_LOAD_RAW_LENTHERROR      0x83
#define HI_ERR_LOAD_START_FAILED        0x84
#define HI_ERR_RAW_BUSY                 0x85
#define HI_ERR_RAW_MALLOC_ERROR         0x86

#define HI_ERR_COMM_NULL_PTR            0x1001
#define HI_ERR_COMM_MALLOC_FAILED       0x1002


#define HI_ERR_COMM_OPEN_I2C            0x1011
#define HI_ERR_COMM_READ_I2C            0x1012
#define HI_ERR_COMM_WRITE_I2C           0x1013


#define HI_ERR_COMM_OPEN_SPI            0x1014
#define HI_ERR_COMM_READ_SPI            0x1015
#define HI_ERR_COMM_WRITE_SPI           0x1016
#define HI_ERR_COMM_MODE_SPI            0x1017

#define HI_ERR_RESET_PARAM_FAILED       0x2002
#define HI_ERR_PARSE_SCENE_INI_FAILED   0x2003
#define HI_ERR_GET_FRAME_DEPTH_FAILED    0x2004
#define HI_ERR_SET_FRAME_DEPTH_FAILED    0x2005
#define HI_ERR_GET_WB_STAINFO_FAILED    0x2006
#define HI_ERR_SET_WB_STAINFO_FAILED    0x2007
#define HI_ERR_GET_AWB_ATTR_FAILED     0x2008

#define PQ_LOAD_RAW_CONTINUE 0x1
#define PQ_LOAD_RAW_STOP  0x2

#if ((defined(ITTB_HICHIP)) && (ITTB_HICHIP== 0x3518))
#define PQ_CHIP_NAME "hi3518"
#define CUR_VERSION "1.0.b.0"
#endif
#if((defined(ITTB_HICHIP)) && (ITTB_HICHIP== 0x3516a))
#define PQ_CHIP_NAME "hi3516a"
#ifdef  __LITEOS__
#define CUR_VERSION "5.0.5.0"
#else
#define CUR_VERSION "1.0.8.0"
#endif

#endif

#if((defined(ITTB_HICHIP)) && (ITTB_HICHIP== 0x3518e))
#define PQ_CHIP_NAME "hi3518e"
#ifdef  __LITEOS__
#define CUR_VERSION "5.0.6.0"
#else
#define CUR_VERSION "1.0.5.0"
#endif

#endif
#if((defined(ITTB_HICHIP)) && (ITTB_HICHIP== 0x3519))
#define PQ_CHIP_NAME "hi3519"
#define CUR_VERSION "1.0.5.0"
#endif

#if((defined(ITTB_HICHIP)) && (ITTB_HICHIP== 0x3519101))
#define PQ_CHIP_NAME "hi3519"
#define CUR_VERSION "0.0.0.0"
#endif

#if ((defined(ITTB_HICHIP)) && (ITTB_HICHIP== 0x3516c300))
#define PQ_CHIP_NAME "hi3516c300"
#define CUR_VERSION "0.0.1.0"
#endif

#define PQ_CHECK_POINTER(ptr)\
    do {\
        if (HI_NULL == ptr)\
        {\
            printf("Null Pointer in %s!\n", __FUNCTION__);\
            return HI_FAILURE;\
        }\
    }while(0)

#define PQ_SAFE_CLOSE_FILE(fp)\
    do {\
        if (HI_NULL != fp)\
        {\
            fclose(fp);\
            fp = HI_NULL;\
        }\
    }while(0)

#define PQ_SAFE_DELETE_PTR(ptr)\
    do {\
        if (HI_NULL != ptr)\
        {\
            free(ptr);\
            ptr = HI_NULL;\
        }\
    }while(0)

#define PQ_PTHREAD_MUTEX_LOCK(ptr)\
    do {\
        if (HI_SUCCESS!= pthread_mutex_lock(ptr))\
        {\
            printf("pthread_mutex_lock err\n");\
        }\
    }while(0)
            
#define PQ_PTHREAD_MUTEX_UNLOCK(ptr)\
    do {\
        if (HI_SUCCESS!= pthread_mutex_unlock(ptr))\
        {\
            printf("pthread_mutex_unlock err\n");\
        }\
    }while(0)

#ifndef hi_usleep
#define hi_usleep(usec) \
do { \
	struct timespec req; \
	req.tv_sec 	= (usec) / 1000000; \
	req.tv_nsec = ((usec) % 1000000) * 1000; \
	nanosleep (&req, NULL); \
} while (0)
#endif

#ifndef H_usleep_in_mutex
#ifdef  __LITEOS__
#define H_usleep_in_mutex(usec)\
do { \
	struct timespec req; \
	req.tv_sec 	= (usec) / 1000000; \
	req.tv_nsec = ((usec) % 1000000) * 1000; \
	nanosleep (&req, NULL); \
} while (0)
#else
#define H_usleep_in_mutex(usec)\
do { \
	struct timeval timeout; \
	timeout.tv_sec 	= (usec) / 1000000; \
	timeout.tv_usec = (usec) % 1000000; \
	select(0, NULL, NULL, NULL, &timeout); \
} while (0)
#endif
#endif


#define CFG_FILE "config.cfg"

#define MAX_LEN 3*1024*1024
#define FILE_PATH_LEN 128
#define PQ_SNS_CFG_FILE "ittb_hi3518_sns.cfg"
#define PQ_PARSE_ISI2C    2


typedef struct hiDBG_YUV
{
    HI_U8  au8Head[16];
    HI_U32 u32PicWith;
    HI_U32 u32PicHigtht;
    HI_U32 u32PicType;
} HI_DBG_YUV_E;


struct LONGPLAY_ARGS
{
    HI_U8* pBuf; //paras
    HI_U8* pData;
    HI_U32 add;
    HI_U32 cmd;
    HI_U32 u32DataLen;
    HI_S32* pHeartAddr;
    HI_S32 s32Ret;
    HI_S32 s32Socket_id;
    HI_PQ_DBG_DLL strDllHandle;
};


/*ECHO ERROR NUM*/
typedef enum hiDBG_ECHO
{
    HI_MAGIC_ERROR = 0xE0,
    HI_CHECKSUM_ERROR = 0xE1,
    HI_CHIP_ERROR = 0xE2,
    HI_VERSION_ERROR = 0xE3,
    HI_INVALID_CMD = 0xE4,
    HI_INVALID_ADDRESS_TYPE = 0xE5,
    HI_LEN_ERROR = 0xE6,
    HI_RUN_ERROR = 0xE7,
    HI_UNABLE_READ = 0xE8,
    HI_SPECIFIC_ERROR = 0xEE,
    HI_MDBG_ECHO_BUTT
} HI_DBG_ECHO_E;





typedef struct hiISP_REG_ADD_RANGE_S
{
    HI_U32 u32IspRegAddrMin;
    HI_U32 u32IspRegAddrMax;
    HI_U32 u32IspExtRegAddrMin;
    HI_U32 u32IspExtRegAddrMax;
    HI_U32 u32AeExtRegAddrMin;
    HI_U32 u32AeExtRegAddrMax;
    HI_U32 u32AwbExtRegAddrMin;
    HI_U32 u32AwbExtRegAddrMax;
} ISP_REG_ADD_RANGE_S;






#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* End of #ifndef __HI_COMM_DEF_H__ */
