#ifndef __ATTENDANCEDATAMAP_H__
#define __ATTENDANCEDATAMAP_H__

#include <stdio.h>
#include "dataanalysisout.h"

int JsonMapPushAttendance(char *msg, AttendanceAtt_T *info);
int JsonMapPushVisitors(char *msg, VIPCustomer_T *info);
int JsonMapPushVolumeOfCommuters(char *msg, VolumeOfCommuters_T *info);



#endif

