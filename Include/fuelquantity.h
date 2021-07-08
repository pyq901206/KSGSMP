#ifndef _FUELQUANTITY_H__
#define _FUELQUANTITY_H__
#include "dataanalysisout.h"

typedef enum _FillRateType_E
{
	FillRateType_ALL = 1,
	FillRateType_LOW,    //10L以下
	FillRateType_MIDDLE, //10L-30L
 	FillRateType_HIGH,   //30L以上
}FillRateType_E;

typedef enum _FuelQuantityType_E
{
	FillQuantityType_92 = 1,    //92汽油
	FillQuantityType_95,    //95汽油
 	FillQuantityType_98,   //98汽油
}FuelQuantityType_E;

typedef struct _OliGunInfo_T
{
	int number;     //枪号
	int frequency;  //频率
}OliGunInfo_T;
typedef struct _FuelQuantityHandle_T
{
	int (*pAddStuckPipeData)(StuckPipeInfo_T *info, int sum);
	int (*pGetFuelingRangeProportionByDate)(TimeYMD_T date, int *lowPoint, int *mediumPoint, int *highPoint);
	int (*pGetFuelQuantityByType)(TimeYMD_T date, int *point92, int *point95, int *point98);
	int (*pGetOliGunSum)(TimeYMD_T date);
	int (*pGetOliGunFrequency)(TimeYMD_T date, OliGunInfo_T *info);
}FuelQuantityHandle_T;

int FuelQuantityAnalysisManageInit(FuelQuantityHandle_T *handle);
int FuelQuantityAnalysisManageUnInit(FuelQuantityHandle_T *handle);



#endif

