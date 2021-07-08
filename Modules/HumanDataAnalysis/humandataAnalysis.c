#include "humandataanalysis.h"
static ENUM_ENABLED_E gInnerHumanExitFlag = ENUM_ENABLED_FALSE;
//内部结构体数据
typedef struct _InnerData_T
{
	
}InnerData_T;
static InnerData_T innerData;
static InnerData_T *GetInnerData()
{
	return &innerData;
}

int HumanDataManageRegister(HumanDataManageHandle_T *handle)
{
	if (NULL == handle)
	{
		return KEY_FALSE;
	}
	
	return KEY_TRUE;
}

int HumanDataManageUnRegister(HumanDataManageHandle_T *handle)
{
	if (NULL == handle)
	{
		return KEY_FALSE;
	}
	
	return KEY_TRUE;
}

int HumanDataManageInit(HumanDataManageHandle_T *handle)
{
	gInnerHumanExitFlag = ENUM_ENABLED_TRUE;
	HumanDataManageRegister(handle);
	return KEY_TRUE;
}

int HumanDataManageUnInit(HumanDataManageHandle_T *handle)
{
	gInnerHumanExitFlag = ENUM_ENABLED_FALSE;
	HumanDataManageUnRegister(handle);
	return KEY_TRUE;
}
