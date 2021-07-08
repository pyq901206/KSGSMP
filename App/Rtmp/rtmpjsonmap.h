#ifndef __RTMPJSONMAP_H__
#define __RTMPJSONMAP_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif     /* __cpluscplus */
#endif  /* __cpluscplus */

#include "rtmpstart.h"
#include "cjson.h"
cJSON* JsonMapRtmpParamMake(RtmpConfParm_T*info);
int JsonMapRtmpParamPares(cJSON *conf,RtmpConfParm_T *info);

#ifdef __cplusplus
#if __cplusplus
}
#endif     /* __cpluscplus */
#endif  /* __cpluscplus */

#endif

