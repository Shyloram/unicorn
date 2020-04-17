#ifndef __HI_HAL_DEF_H__
#define __HI_HAL_DEF_H__
#include "hi_type.h"
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __LITEOS__
#define CFG_FILE "config.ini"
#else
#define CFG_FILE "config.cfg"
#endif
#define PQ_DBG_EMERG      0   /* system is unusable                   */
#define PQ_DBG_ALERT      1   /* action must be taken immediately     */
#define PQ_DBG_CRIT       2   /* critical conditions                  */
#define PQ_DBG_ERR        3   /* error conditions                     */
#define PQ_DBG_WARN       4   /* warning conditions                   */
#define PQ_DBG_NOTICE     5   /* normal but significant condition     */
#define PQ_DBG_INFO       6   /* informational                        */
#define PQ_DBG_DEBUG      7   /* debug-level messages                 */

#define OFFSET_OF(s, m) (size_t)&(((s *)0)->m)
#define GET_VALUE(buf, s, m) ((*(s *)buf).m)


#define PQ_PARSE_ISVIRREG 1
#define PQ_PARSE_ISREG 0

/*ReadReg and WriteReg*/
#define HI_ERR_INVALID_3A_ADDRESS       (-13)


#define HI_ERR_REG_ADDR_NOT_MATCH       (-10)
#define HI_ERR_GET_REAL_ADD_FAILED      (-9)


#define HI_ERR_RREG_NULLBUF             (-3)
#define HI_ERR_RREG_INVALID_ADD_TYPE    0x03

#define HI_ERR_RREG_INVALID_BIT_RANGE   0x06


#define HI_ERR_DATA_TOO_LONG            0x17
#define HI_ERR_CVT_GAMMA_TABLE_FAILED   0x18
#define HI_ERR_NOT_EQUAL_GAMMA_RGB      0x19
#define HI_ERR_GAMMA_LEN_INCONFORMITY   0x1A



#define HI_ERR_DOING_DP                 0x71
#define HI_ERR_GET_DP_FAILED            0x81
#define HI_ERR_SET_DP_FAILED            0x82
#define HI_ERR_SET_DP_TIMEOUT           0x83

#define HI_ERR_GET_AE_ATTR_FAILED       0x91
#define HI_ERR_SET_AE_ATTR_FAILED       0x92


#define HI_ERR_ACM_PEN_DLL              0x1021
#define HI_ERR_GET_ACM                  0x1022
#define HI_ERR_SET_ACM                  0x1023
#define HI_ERR_ACM_MODE                 0x1024

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



typedef struct hiMDBG_MSG
{
    HI_U32  u32datalen;
    HI_U8  au8MagicNum[4];
    HI_U8  au32ChipVersion[16];
    HI_U8  au32PQVersion[16];
    HI_U32 u32CMD;
    HI_U8   u8RegType[4];
    HI_U32 u32addr;
    HI_U32 u32Len;
    HI_U8  au8Para[32];
    HI_U8*   pu8Data;
    HI_U32  u32DataCheckSum;
} HI_MDBG_MSG_S;


typedef struct tagPQ_DBG_BIT_RANGE
{
    HI_U32 u32OrgBit;
    HI_U32 u32EndBit;
} PQ_DBG_BIT_RANGE_S;

typedef struct tagPQ_DBG_MSG_PARA
{
    PQ_DBG_BIT_RANGE_S stBitRange;
    HI_BOOL bSigned;
} PQ_DBG_MSG_PARA_S;

typedef struct hiPQ_DBT_DLL
{
    HI_VOID* Handle_AE;
    HI_VOID* Handle_AF;
    HI_VOID* Handle_AWB;
    HI_VOID* Handle_ISP;
} HI_PQ_DBG_DLL;


typedef enum hiPQ_DBG_CMD
{
    PQ_CMD_READ_REG = 0,
    PQ_CMD_WRITE_REG = 1,
    PQ_CMD_READ_MATRIX = 8,
    PQ_CMD_WRITE_MATRIX = 9,
    PQ_CMD_READ_REGS = 10,
    PQ_CMD_WRITE_REGS = 11,
    PQ_CMD_READ_GAMMA = 12,
    PQ_CMD_WRITE_GAMMA = 13,
    PQ_CMD_WRITE_ACM = 15,
    PQ_CMD_EXPORT_BIN = 18,
    PQ_CMD_IMPORT_BIN = 19,
    PQ_CMD_SOLIDIFY_BIN = 20,
    PQ_CMD_READ_SCENE   = 23,
    PQ_CMD_WRITE_SCENE  = 24,
    PQ_CMD_READ_YUV =  101,
    PQ_CMD_READ_RAW = 102,
    PQ_CMD_READ_AWB = 103,
    PQ_CMD_READ_I2C = 104,
    PQ_CMD_WRITE_I2C = 105,
    PQ_CMD_READ_3A = 106,
    PQ_CMD_SEND_RAW = 132,
    PQ_CMD_CAPTURE = 133,
    PQ_CMD_READ_SPI = 134,
    PQ_CMD_WRITE_SPI = 135,
    PQ_CMD_READ_ISP = 136,
    PQ_CMD_GET_WBEWS = 137,
    PQ_CMD_SET_WBEWS = 138,
#if((defined(ITTB_HICHIP)) && (ITTB_HICHIP== 0x3516a))
    PQ_CMD_SET_CALIBRITE = 200,
#else
    PQ_CMD_READ_JPG = 200,
#endif
    PQ_CMD_BUTT,
} PQ_DBG_CMD_E;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* End of #ifndef __HI_HAL_DEF_H__ */


