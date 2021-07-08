#ifndef __HKDATAPRASE_H_
#define __HKDATAPRASE_H_

#include "cjson.h"
#include "common.h"
#include "HCNetSDK.h"
#include "dataanalysisout.h"
#include "vehicledataanalysis.h"


int vca_DataPrase(char *pAlarmInfo, char *SerialNumber, DataAtt_T *deviceInfo);
int ITS_PlatePrase(char *pAlarmInfo, char *SerialNumber, DataAtt_T *deviceInfo);
int thermometry_DataPrase(char *pAlarmInfo, char *SerialNumber, DataAtt_T *deviceInfo);
int firedetection_DataPrase(char *pAlarmInfo, char *SerialNumber, DataAtt_T *deviceInfo);

#endif
