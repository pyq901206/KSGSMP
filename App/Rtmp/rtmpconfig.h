
#ifndef __RTMPCONFIG_H__
#define __RTMPCONFIG_H__

#include "rtmpstart.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif     /* __cpluscplus */
#endif  /* __cpluscplus */

#ifndef D_RTMPCONF_STR
#define D_RTMPCONF_STR "ConfRtmpParam"
#endif

int RtmpParamSetToConfig(RtmpConfParm_T *info);
int RtmpParamGetFromConfig(RtmpConfParm_T *info);


#ifdef __cplusplus
#if __cplusplus
}
#endif     /* __cpluscplus */
#endif  /* __cpluscplus */

#endif
