#ifndef __ATTENDANCEMANAGE_H__
#define __ATTENDANCEMANAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <sys/prctl.h>
#include "common.h"
#include "dataanalysisout.h"

typedef struct _AttendanceManage_T{		
//无感考勤 No sense of attendance
//通过名字获取当前员工打卡记录数据
int (*pGetAttendanceHistorySum)(TimeYMD_T startTime,TimeYMD_T endTime, char *fuzzyStr,int *num);

int (*pGetAttendanceHistoryList)(TimeYMD_T startTime,TimeYMD_T endTime,int startPage, int num,char *fuzzyStr, int *sum,AttendanceAtt_T *info);
//记录员工打卡
int (*pSetAttendanceRecord)(_IN_ DataAtt_T *dataInfo);

int (*pGetAttendanceRecord)( TimeYMD_T startTime,TimeYMD_T endTime,int page, int num, int *sum,AttendanceAtt_T *info);

int (*pGetAttendanceRecordSum)( TimeYMD_T startTime,TimeYMD_T endTime, int *num);


//手动补卡		//额外添加的  实现流程应该是 先查找该员工当日是否有打卡记录 如果没有则更新数据库，怎加打卡记录，手动打卡无法添加图片
int (*pSetManualAttendanceRecord)(_IN_ TimeYMD_T currentTime,_IN_ unsigned char *name);
int (*pAddVisitorsInfo)(_IN_ DataAtt_T *dataInfo);
int (*pGetVisitorsInfo)(TimeYMD_T startTime,TimeYMD_T endTime,int page,int num, int *sum,VIPCustomer_T *info );

int (*pPushVolumeOfCommutersNum)(TimeYMD_T time,VolumeOfCommuters_T *info);
int (*pAddVolumeOfCommutersNum)(TimeYMD_T time,VolumeOfCommuters_T *info);
int (*pGetVolumeOfCommutersNum)(TimeYMD_T time,LeaveAndEnterNum_T *info);
int (*pGetVolumeOfCommutersOfLine)(TimeYMD_T startTime,TimeYMD_T endTime,int num,int *sum,LeaveAndEnterNum_T *info);
int (*pGetVolumeOfCommutersOfSex)(TimeYMD_T startTime,TimeYMD_T endTime,int num,VolumeOfCommutersOfSex_T *info);
int (*pGetVolumeOfCommutersOfAge)(TimeYMD_T startTime,TimeYMD_T endTime,int num,VolumeOfCommutersOfAge_T *info);

}AttendanceManage_T;

int AttendanceManageInit(AttendanceManage_T *handle);
int AttendanceManageUnInit(AttendanceManage_T *handle);
int AttendanceManageRegister(AttendanceManage_T *handle);
int AttendanceManageUnRegister(AttendanceManage_T *handle);




#ifdef __cplusplus
}
#endif

#endif

