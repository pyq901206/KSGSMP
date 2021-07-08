#include "adpapi.h"
static int InitHandleGetBaseInfo(DeviceInfo_T *info){return KEY_FALSE;}
static int InitHandleSetBaseInfo(DeviceInfo_T *info){return KEY_FALSE;}
static int InitHandleGetUsrInfoNum(int *num){return KEY_FALSE;}
static int InitHandleGetUsrInfoList(UsrInfo_T *info){return KEY_FALSE;}
static int InitHandleModifyUsrInfo(int usrID,UsrInfo_T *info){return KEY_FALSE;}
static int InitHandleAddUsr(int usrID,UsrInfo_T *info){return KEY_FALSE;}
static int InitHandleCheckUsr(int usrID,UsrInfo_T *info){return KEY_FALSE;}
static int InitHandleDelUsr(int usrID,UsrInfo_T *info){return KEY_FALSE;}
static int InitHandleGetPublicKey(PublicKey_T *key){return KEY_FALSE;}
static int InitHandleSetPublicKey(PublicKey_T *key){return KEY_FALSE;}
static int InitHandleGetUserRegistInfo(int usrID, UserRegistInfo_T *info){return KEY_FALSE;}
static int InitHandleModifyPassword(int usrID, char *username,char *oldPassword,char *newPassword){return KEY_FALSE;}
static int InitHandleResetPassword(char *username, char *countrycode, char *password){return KEY_FALSE;}
static int InitHandleModifyUsrBindInfo(UsrInfo_T *info, char *oldusername){return KEY_FALSE;}
static int InitHandleGetDevicePayEffectTime(UserRegistInfo_T *info, char *deviceID){return KEY_FALSE;}
static int InitHandleGetDeviceRecType(UserRegistInfo_T *info, char *deviceID){return KEY_FALSE;}
static int InitHandleGetCoverRecType(UserRegistInfo_T *info, char *deviceID){return KEY_FALSE;}
static int InitHandleAddDevice(DeviceStorageInfo_T *info){return KEY_FALSE;}
static int InitHandleDelDevice(char *deviceID){return KEY_FALSE;}
static int InitHandleFindUsr(char *username){return KEY_FALSE;}
static int InitHandleGetDeviceInfo(char *deviceID,DeviceStorageInfo_T *cloudInfo){return KEY_FALSE;}
static int InitHandleSetDeviceInfo(char *deviceID,DeviceStorageInfo_T *cloudInfo){return KEY_FALSE;}
static int InitHandleGetDeviceInfoListNum(int *num){return KEY_FALSE;}
static int InitHandleGetDeviceInfoList(DeviceStorageInfo_T *cloudInfo){return KEY_FALSE;}
static int InitHandleAddVehicleInfo(DataAtt_T *info){return KEY_FALSE;}
static int InitHandleAddStuckPipeData(StuckPipeInfo_T *info, int sum){return KEY_FALSE;}
static int InitHandleGetFuelingRangeProportionByDate(TimeYMD_T date, int *lowPoint, int *mediumPoint, int *highPoint){return KEY_FALSE;}
static int InitHandleGetFuelQuantityByType(TimeYMD_T date, int *point92, int *point95, int *point98){return KEY_FALSE;}
static int InitHandleGetOliGunSum(TimeYMD_T date){return KEY_FALSE;}
static int InitHandleGetOliGunFrequency(TimeYMD_T date, OliGunInfo_T *info){return KEY_FALSE;}
	

int InitAdpAPIRegistHandleNoAuthor(AdpAPIHandle_T *handle)
{
	handle->pGetBaseInfo = InitHandleGetBaseInfo;
	handle->pSetBaseInfo = InitHandleSetBaseInfo;
	handle->pGetUsrInfoNum = InitHandleGetUsrInfoNum;
	handle->pGetUsrInfoList = InitHandleGetUsrInfoList;
	handle->pModifyUsrInfo = InitHandleModifyUsrInfo;
	handle->pAddUsr = InitHandleAddUsr;
	handle->pCheckUsr = InitHandleCheckUsr;
	handle->pDelUsr = InitHandleDelUsr;
	handle->pGetPublicKey = InitHandleGetPublicKey;
	handle->pSetPublicKey = InitHandleSetPublicKey;
	handle->pGetUserRegistInfo = InitHandleGetUserRegistInfo;
	handle->pModifyPassword = InitHandleModifyPassword;
	handle->pResetPassword = InitHandleResetPassword;
	handle->pModifyUsrBindInfo = InitHandleModifyUsrBindInfo;
	handle->pAddDevice = InitHandleAddDevice;
	handle->pDelDevice = InitHandleDelDevice;
	handle->pFindUsr = InitHandleFindUsr;
	handle->pGetDeviceInfo = InitHandleGetDeviceInfo;
	handle->pSetDeviceInfo = InitHandleSetDeviceInfo;
	handle->pGetDeviceInfoListNum = InitHandleGetDeviceInfoListNum;
	handle->pGetDeviceInfoList = InitHandleGetDeviceInfoList;
	handle->pAddVehicleInfo = InitHandleAddVehicleInfo;
	handle->pAddStuckPipeData = InitHandleAddStuckPipeData;
	handle->pGetFuelingRangeProportionByDate = InitHandleGetFuelingRangeProportionByDate;
	handle->pGetFuelQuantityByType = InitHandleGetFuelQuantityByType;
	handle->pGetOliGunSum = InitHandleGetOliGunSum;
	handle->pGetOliGunFrequency = InitHandleGetOliGunFrequency;
	
	handle->usrID = 0;
	handle->sessionid = 0;
	return KEY_TRUE;
}
