#ifndef __HUMANDATA_MANAGE_H__
#define __HUMANDATA_MANAGE_H__

#include "common.h"

typedef enum _FaceRecordFindType_E
{
	FaceRecordFind_TYPE_ALL = 0x01,
	FaceRecordFind_TYPE_NAME,
	FaceRecordFind_TYPE_DATE,
	FaceRecordFind_TYPE_MIXING,
	FaceRecordFind_TYPE_NONE,
}FaceRecordFindType_E;

typedef struct _FaceLibraryInfo_T
{
	char faceId[64]; //编号,服务器内部生成，不能重复
	char faceName[64];
	char usrId[64]; //唯一身份标识 员工工号
	int  gender;     //0-男  1-女
	time_t importTd;	 //导入时间
}FaceLibraryInfo_T;

typedef struct _FaceRecordInfo_T
{	
	char faceId[64]; //编号,根据编号查找入库信息
	char faceRecordId[64]; //人脸记录唯一标识符
	char snapDeviceId[64]; //抓拍设备
	time_t snapTd;	 //抓拍时间
}FaceRecordInfo_T;

typedef struct _FaceStorage_T
{
	FaceRecordInfo_T faceRecordInfo;
	FaceLibraryInfo_T faceLocalInfo;
}FaceStorage_T;

typedef struct _FaceFindByDate_T
{
	TimeYMD_T startTs;
	TimeYMD_T endTs;	
}FaceFindByDate_T;

typedef struct _FaceFindByName_T
{
	char name[128];
}FaceFindByName_T;

typedef struct _FaceFindByMixing_T
{
	FaceFindByDate_T  faceRecordByDate;
	FaceFindByName_T	faceRecordByName;
}FaceFindByMixing_T;

typedef struct _FaceFind_T
{
	FaceRecordFindType_E type;
	union
    {
    	FaceFindByDate_T faceByDataInfo;
    	FaceFindByName_T faceByNameInfo;
		FaceFindByMixing_T faceByMixingInfo;
    };
}FaceFind_T;

//无感考勤
typedef struct _AttendanceHandle_T
{
	//添加店员人脸到底库
	int (*pAddLocalFace)(FaceLibraryInfo_T *faceInfo); //返回人脸ID  //数据直接从海康超脑获取
	//删除店员人脸到底库
	int (*pDelLocalFace)(char *faceId);
	//添加识别打卡记录
	int (*pAddFaceRecording)(FaceRecordInfo_T *faceRecordInfo);
	//删除识别打卡记录
	int (*pDelFaceRecording)(char *faceRecordId);
	//获取打卡记录列表
	int (*pGetLastFaceRecording)(char *faceid, FaceRecordInfo_T *info);
}AttendanceHandle_T;

//客流分析
typedef struct _EnteringAnalysisHandle_T
{
	//添加客流数据
	int (*pAddEnteringData)();
	//获取客流数据		 //可通过某一属性
	int (*pGetEnteringData)();
	//删除客流数据
	int (*pDelEnteringData)();
}EnteringAnalysisHandle_T;

typedef struct _HumanDataManageHandle_T{
	#if 0
	int (*pAddLocalFace)(FaceLibraryInfo_T *faceInfo); //返回人脸ID
	int (*pDelLocalFace)(char *faceId);
	int (*pFindLocalFace)(char *faceId, FaceLibraryInfo_T *faceInfo);
	int (*pModifyLocalFace)(char *faceId, FaceLibraryInfo_T *faceInfo);
	int (*pGetLocalFaceByUsrID)(char *usrID, FaceLibraryInfo_T *faceInfo);
	int (*pGetLocalFaceSum)(FaceFind_T find, int *sum);
	int (*pGetLocalFaceInfoListPage)(int start, int num, FaceLibraryInfo_T *info);
	int (*pGetLocalFaceInfoListPageByDate)(int start, int num, TimeYMD_T startTs, TimeYMD_T endTs, FaceLibraryInfo_T *info);
	int (*pGetLocalFaceInfoListPageByFaceName)(int start, int num, char *faceName, FaceLibraryInfo_T *info);
	int (*pGetLocalFaceInfoListPageByMixing)(int start, int num, TimeYMD_T startTs, TimeYMD_T endTs, char *faceName, FaceLibraryInfo_T *info);	
	
	int (*pAddFaceRecording)(FaceRecordInfo_T *faceRecordInfo);
	int (*pDelFaceRecording)(char *faceRecordId);
	int (*pFindFaceRecord)(char *faceRecordId, FaceRecordInfo_T *faceRecordInfo);
	int (*pGetFaceRecordingByDate)(int start, int num, TimeYMD_T startTd, TimeYMD_T endTd, FaceStorage_T *faceStorage, int *faceStorageNum);
	int (*pGetFaceRecordingByName)(int start, int num, char *faceName, FaceStorage_T *faceStorage, int *faceStorageNum);
	int (*pGetFaceRecordingByMixing)(int start, int num, TimeYMD_T startTd, TimeYMD_T endTd, char *faceName, FaceStorage_T *faceStorage, int *faceStorageNum);
	int (*pGetFaceRecordingList)(int start, int num, FaceStorage_T *faceStorage, int *faceStorageNum);
	int (*pGetFaceRecordingSum)(FaceFind_T find, int *sum);
	int (*pGetLastFaceRecording)(char *faceid, FaceRecordInfo_T *info);
	#endif
	AttendanceHandle_T AttendanceHandle;
	EnteringAnalysisHandle_T EnteringAnalysisHandle;
	//热区分析
}HumanDataManageHandle_T;

_OUT_ int HumanDataManageRegister(HumanDataManageHandle_T *handle);
_OUT_ int HumanDataManageUnRegister(HumanDataManageHandle_T *handle);
_OUT_ int HumanDataManageInit(HumanDataManageHandle_T *handle);
_OUT_ int HumanDataManageUnInit(HumanDataManageHandle_T *handle);


#endif

