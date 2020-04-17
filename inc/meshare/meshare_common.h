#ifndef __MESHARE_COMMON_H__
#define __MESHARE_COMMON_H__

#include "zmdconfig.h"

/*************************************************
 *
 * meshare 库各文件共用的头文件，
 * 此头文件只允许被其他文件 #inlcude 
 * 禁止再 #include 其他任何自定义的头文件
 *
 * 本模块中默认 MSH / msh 为 meshare 的缩写
 *
 * **********************************************/

#define MSH_SUCCESS         (0)
#define MSH_FAILURE         (-1)

#ifdef ZMD_APP_DEBUG_MSH
#define MSHLOG(format, ...)     fprintf(stdout, "\033[37m[MSHLOG][Func:%s][Line:%d], " format"\033[0m", __FUNCTION__,  __LINE__, ##__VA_ARGS__)
#else
#define MSHLOG(format, ...)
#endif
#define MSHERR(format, ...)     fprintf(stdout, "\033[31m[MSHERR][Func:%s][Line:%d], " format"\033[0m", __FUNCTION__,  __LINE__, ##__VA_ARGS__)
#define MSHINF(format, ...)     fprintf(stdout, "\033[34m[MSHINF][Func:%s][Line:%d], " format"\033[0m", __FUNCTION__,  __LINE__, ##__VA_ARGS__)



#endif  /* __MESHARE_COMMON_H__ */
