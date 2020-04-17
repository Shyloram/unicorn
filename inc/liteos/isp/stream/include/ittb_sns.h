#ifndef _ITTB_SNS_H_
#define _ITTB_SNS_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

typedef void(*fun_sns_init)();
typedef int (*fun_sns_regsiter_callback)();
typedef int (*fun_sns_unregsiter_callback)();
typedef int(*fun_sns_config)(char* str);


#define ITTB_CFG_LINE_MAX_LEN 1000;

typedef struct stsns_fun
{
    void* 					Handle;
    void*                   HandleConfig;
    char*					strError;
    fun_sns_init				FnSnsInit;
    fun_sns_regsiter_callback	FnSnsRegCallBack;
    fun_sns_unregsiter_callback	FnSnsUnRegCallBack;
    fun_sns_config          FnSnsConfig;
} sns_fun_st;

typedef enum ittb_sensor_mode
{
    SENSOR_MODE_LINE = 0,
    SENSOR_MODE_WDR
} ITTB_SENSOR_MODE_E;

/***
Usage: hisi_isptool ar0331 3M wdr/line
**/
typedef struct sensor_def
{

    HI_BOOL bUseIsp;

} ITTB_SENSOR_DEF_S;

HI_S32 snsfm_init(HI_S32 s32SnsType, sns_fun_st* pstSnsFun);
void snsfm_deinit(sns_fun_st stSnsFun);
void sns_init(sns_fun_st stSnsFun);
HI_S32 sns_register_callback(sns_fun_st stSnsFun);
HI_S32 sns_unregister_callback(sns_fun_st stSnsFun);
HI_S32 sns_configs(char* sDllFile, sns_fun_st stSnsFun);


HI_S32 Ittb_Sns_Init(char* sDllFile, sns_fun_st* pstSnsFun);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /*ITTB_SNS_H*/
