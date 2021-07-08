#include "common.h"
#include "fuelquantity.h"
#include "mysqlctrl.h"

typedef struct _InnerInfoHandle_T
{
	MySqlManage_T mysqlHandle;
}InnerInfoHandle_T;
static InnerInfoHandle_T g_innerInfo;
static InnerInfoHandle_T *GetInnerInfoHandle()
{
	return &g_innerInfo;
}

static int AddStuckPipeData(StuckPipeInfo_T *info, int sum)
{
	int i = 0;
	int iret = -1;
	if (NULL == info)
	{
		return iret;
	}
	iret = GetInnerInfoHandle()->mysqlHandle.pDelStuckPipeDataByDate(info->date);
	if (KEY_FALSE == iret)
	{
		return iret;
	}
	
	for (i = 0; i < sum; i++)
	{
		GetInnerInfoHandle()->mysqlHandle.pAddStuckPipeData(&info[i]);
	}
	return iret;
}

static int GetFuelingRangeProportionByDate(TimeYMD_T date, int *lowPoint, int *mediumPoint, int *highPoint)
{
	int iret = -1;
	int allSum = 0, lowSum = 0, middleSum = 0, highSum = 0;
	MySqlManage_T *sqlHandle = &GetInnerInfoHandle()->mysqlHandle;
	if (NULL == sqlHandle)
	{
		return iret;
	}
	if (NULL == sqlHandle->pGetFillRateSumByType)
	{
		return iret;
	}
	//获取加满率整数
	allSum = sqlHandle->pGetFillRateSumByType(date, FillRateType_ALL);
	//获取30L以上数量
	*highPoint = sqlHandle->pGetFillRateSumByType(date, FillRateType_HIGH);
	//获取10-30L数量
	*mediumPoint = sqlHandle->pGetFillRateSumByType(date, FillRateType_MIDDLE);
	//获取小于10数量
	*lowPoint = sqlHandle->pGetFillRateSumByType(date, FillRateType_LOW);
	//*highPoint = (float)((float)highSum / (float)allSum) * 100;
	//*mediumPoint = (float)((float)middleSum / (float)allSum) * 100;	
	//*lowPoint = 100 - *highPoint - *mediumPoint;
	return KEY_TRUE;
}

static int GetFuelQuantityByType(TimeYMD_T date, int *point92, int *point95, int *point98)
{
	int iret = -1;
	int allSum = 0, lowSum = 0, middleSum = 0, highSum = 0;
	MySqlManage_T *sqlHandle = &GetInnerInfoHandle()->mysqlHandle;
	if (NULL == sqlHandle)
	{
		return iret;
	}
	
	if (NULL == sqlHandle->pGetFuelQuantityByType)
	{
		return iret;
	}

	sqlHandle->pGetFuelQuantityByType(date, FillQuantityType_92, point92);
	sqlHandle->pGetFuelQuantityByType(date, FillQuantityType_95, point95);
	sqlHandle->pGetFuelQuantityByType(date, FillQuantityType_98, point98);
	return KEY_TRUE;
}

static int GetOliGunSum(TimeYMD_T date)
{
	int iret = -1;
	int sum = 0;
	MySqlManage_T *sqlHandle = &GetInnerInfoHandle()->mysqlHandle;
	if (NULL == sqlHandle)
	{
		return iret;
	}
	
	if (NULL == sqlHandle->pGetOliGunSum)
	{
		return iret;
	}
	return sqlHandle->pGetOliGunSum(date);
}

static int GetOliGunFrequency(TimeYMD_T date, OliGunInfo_T *info)
{
	int iret = -1;
	int sum = 0;
	MySqlManage_T *sqlHandle = &GetInnerInfoHandle()->mysqlHandle;
	if (NULL == sqlHandle)
	{
		return iret;
	}

	if (NULL == sqlHandle->pGetOliGunFrequency)
	{
		return iret;
	}
	iret = sqlHandle->pGetOliGunFrequency(date, info);
	return iret;
}


int FuelQuantityAnalysisManageRegister(FuelQuantityHandle_T *handle)
{
	handle->pAddStuckPipeData = AddStuckPipeData;
	handle->pGetFuelingRangeProportionByDate = GetFuelingRangeProportionByDate;
	handle->pGetFuelQuantityByType = GetFuelQuantityByType;
	handle->pGetOliGunSum = GetOliGunSum;
	handle->pGetOliGunFrequency = GetOliGunFrequency;
	return KEY_TRUE;
}

int FuelQuantityAnalysisManageUnRegister(FuelQuantityHandle_T *handle)
{
	handle->pAddStuckPipeData = NULL;
	handle->pGetFuelingRangeProportionByDate = NULL;
	handle->pGetFuelQuantityByType = NULL;
	handle->pGetOliGunSum = NULL;
	handle->pGetOliGunFrequency = NULL;
	return KEY_TRUE;
}

int FuelQuantityAnalysisManageInit(FuelQuantityHandle_T *handle)
{
	MySqlManageRegister(&GetInnerInfoHandle()->mysqlHandle);
	FuelQuantityAnalysisManageRegister(handle);
	return KEY_TRUE;
}

int FuelQuantityAnalysisManageUnInit(FuelQuantityHandle_T *handle)
{
	MySqlManageUnRegister(&GetInnerInfoHandle()->mysqlHandle);
	FuelQuantityAnalysisManageUnRegister(handle);
	return KEY_TRUE;
}

