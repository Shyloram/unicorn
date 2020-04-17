#ifndef _PQ_SCENE_H_
#define _PQ_SCENE_H_


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

/*---------------------------------------------------------------------------
   								Includes
 ---------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//#include "iniparser.h"
//#include "dictionary.h"


#define PQ_ERR_SCENE_LOAD_FAILED    0xF001
#define PQ_ERR_SCENE_EMPTY_NAME     0xF002
#define PQ_ERR_SCENE_INVALID_SCENE  0xF003



int HI_PQ_SCENE_LoadScene(const char* pcFileName, const char* pcSceneName);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif



