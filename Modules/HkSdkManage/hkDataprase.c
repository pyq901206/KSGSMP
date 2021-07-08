#include <stdio.h>
#include <stdlib.h>
#include "hkManage.h"
#include "timemanage.h"
#include "hkDataprase.h"
#include "userdefine.h"

#define FILEPATH "/root/images"
extern int GetDeviceType(int channel, char *SerialNumber, HkDeviceParam_T *deviceInfo);


static Plate_Color_E ConfirmTheColorOfThePlate(char *info)
{

	if(strstr(info, "黄") != NULL)
	{
		return yellow;
	}
	else if(strstr(info, "蓝") != NULL)
	{
		return blue;
	}
	else if(strstr(info, "绿") != NULL)
	{
		return green;
	}
	else 
	{
		return platecolor_null;
	}	  
}

static Vehicle_Type_E ConfirmTheTypeOfTheVehicle(char *info)
{
	if(strstr(info, "largeBus") != NULL)
	{
		return largeBus;
	}
	else if(strstr(info, "truck") != NULL)
	{
		return truck;
	}
	else if(strstr(info, "vehicle") != NULL)
	{
		return vehicle;
	}
	else if(strstr(info, "van") != NULL)
	{
		return van;
	}
	else if(strstr(info, "buggy") != NULL)
	{
		return truck;
	}
	else if(strstr(info, "SUVMPV") != NULL)
	{
		return vehicle;
	}
	else if(strstr(info, "mediumBus") != NULL)
	{
		return largeBus;
	}
	else if(strstr(info, "smallCar") != NULL)
	{
		return vehicle;
	}
	else if(strstr(info, "miniCar") != NULL)
	{
		return vehicle;
	}
	else if(strstr(info, "pickupTruck") != NULL)
	{
		return truck;
	}
	else
	{
		return vehicletype_Null;
	}
	return vehicletype_Null;
}

static Vehicle_Color_E ConfirmTheColorOfTheVehicle(char *info)
{
	if(strstr(info, "white") != NULL)
	{
		return vehicle_color_white;
	}
	else if(strstr(info, "silver") != NULL)
	{
		return vehicle_color_silver;
	}
	else if(strstr(info, "gray") != NULL)
	{
		return vehicle_color_gray;
	}
	else if(strstr(info, "black") != NULL)
	{
		return vehicle_color_black;
	}
	else if(strstr(info, "red") != NULL)
	{
		return vehicle_color_red;
	}
	else if(strstr(info, "deepblue") != NULL)
	{
		return vehicle_color_deepBlue;
	}
	else if(strstr(info, "blue") != NULL)
	{
		return vehicle_color_blue;
	}
	else if(strstr(info, "yellow") != NULL)
	{
		return vehicle_color_yellow;
	}
	else if(strstr(info, "green") != NULL)
	{
		return vehicle_color_green;
	}
	else if(strstr(info, "brown") != NULL)
	{
		return vehicle_color_brown;
	}
	else if(strstr(info, "pink") != NULL)
	{
		return vehicle_color_pink;
	}
	else if(strstr(info, "purple") != NULL)
	{
		return vehicle_color_purple;
	}
	else if(strstr(info, "deepgray") != NULL)
	{
		return vehicle_color_deepGray;
	}
	else if(strstr(info, "cyan") != NULL)
	{
		return vehicle_color_cyan;
	}
	else if(strstr(info, "orange") != NULL)
	{
		return vehicle_color_orange;
	}	
	else
	{
		return vehicle_color_unknown;
	}

	return vehicle_color_unknown;
}

static int httpHeadPrase(char *pAlarmInfo, char *msgbuf)
{
	char *ps = NULL;
	char *pl = NULL;
	char  buf[24] = {0};
	int   msglen = 0;
	
	ps = strstr(pAlarmInfo, "Content-Length:");
	ps = ps + strlen("Content-Length:")+1;
	pl = strstr(ps, "\r\n\r\n");
	snprintf(buf, strlen(ps) - strlen(pl) + 1, "%s", ps);

	msglen = atoi(buf);
	pl += strlen("\r\n\r\n");
	snprintf(msgbuf, msglen+1, "%s", pl);

	return KEY_TRUE;
}

//临街告警
int ITS_PlatePrase(char *pAlarmInfo, char *SerialNumber, DataAtt_T *deviceInfo)
{
	if(NULL == pAlarmInfo || NULL == SerialNumber || NULL == deviceInfo)
	{
		return KEY_FALSE;
	}
	FILE *fSnapPic=NULL;
	char filename[512] = {0};
	int iret = 0, i = 0;
	HkDeviceParam_T ipcamInfo = {0};
	VehicleAtt_T  VehicleAttr = {0};
	NET_ITS_PLATE_RESULT struITSPlateResult={0};
	
	memcpy(&struITSPlateResult, pAlarmInfo, sizeof(struITSPlateResult));
	//DF_DEBUG("byChanIndex: %d", struITSPlateResult.byChanIndex);
	Time_T tv = {0};
	GetLocalTime(&tv);
	char plan[32] = {0};
	char out[128] = {0};
	Gb2312ToUtf8(out, 128, struITSPlateResult.struPlateInfo.sLicense, strlen(struITSPlateResult.struPlateInfo.sLicense));
	memcpy(&VehicleAttr.snapTime, &tv.curTime, sizeof(TimeYMD_T));
	if(!strcmp("无车牌", out))
	{
		memcpy(&VehicleAttr.plateNo, &out, 128); 
		return KEY_FALSE;
	}
	memcpy(&VehicleAttr.plateNo, &out[3], 128); 
	VehicleAttr.plateColor = ConfirmTheColorOfThePlate(out);
	//DF_DEBUG("out:%s", out);
	//DF_DEBUG("out:%s", out + 2);
	//DF_DEBUG("plateNo:%s", VehicleAttr.plateNo);
#if 0
	DF_DEBUG("byColor:%d", struITSPlateResult.struPlateInfo.byColor);
	switch(struITSPlateResult.struPlateInfo.byColor)//车牌颜色
	{
	 	case VCA_BLUE_PLATE:
	   		VehicleAttr.plateColor = blue;
	   		break;
  	 	case VCA_YELLOW_PLATE:
	   		VehicleAttr.plateColor = yellow;
	   		break;
	 	case VCA_GREEN_PLATE:
	   		VehicleAttr.plateColor = green;
	   		break;
   		default:
	   		VehicleAttr.plateColor = platecolor_null;
	   		break;
	 }
#endif	
	switch(struITSPlateResult.byVehicleType) //车辆类型
	{
		case VTR_RESULT_OTHER:
			VehicleAttr.vehicleType = vehicletype_Null;
			break;
		case VTR_RESULT_BUS:
		case VTR_RESULT_SMALLTRUCK:
			VehicleAttr.vehicleType = largeBus;
			break;
		case VTR_RESULT_TRUCK:
			VehicleAttr.vehicleType = truck;
			break;
		case VTR_RESULT_CAR:
			VehicleAttr.vehicleType = vehicle;
			break;
		case VTR_RESULT_MINIBUS:
			VehicleAttr.vehicleType = van;
			break;
		default:
			VehicleAttr.vehicleType = vehicletype_Null;
			break;
	}
	VehicleAttr.vehicleColor = (Vehicle_Color_E)struITSPlateResult.struVehicleInfo.byColor;
	DataAnalysisManage_T handle = {0};
	VehicleDataAnalysisManageRegister(&handle);
	VehicleAttr.vehicleTerr = handle.pGetVehiclesTerritory(VehicleAttr.plateNo);
	VehicleDataAnalysisManageUnRegister(&handle);
	iret = GetDeviceType(struITSPlateResult.byChanIndex, SerialNumber, &ipcamInfo);
	if(iret != KEY_TRUE)
	{
		return KEY_FALSE;
	}
	deviceInfo->type = (DeviceType_T)ipcamInfo.sDeviceType;
	//DF_DEBUG("type:%d  vehicleType: %d", deviceInfo->type, VehicleAttr.vehicleType);
	if(deviceInfo->type == DEVICE_TYPE_Frontage)
	{
		 snprintf(deviceInfo->DataUnion.deviceStreet.snapToken, 128, "%04d%02d%02d%02d%02d%02d%03d", tv.curTime.year, tv.curTime.month, 
		   tv.curTime.day, tv.curTime.hour, tv.curTime.min, tv.curTime.sec, 
		   (int)rand()%1000);
		 deviceInfo->DataUnion.deviceStreet.plateColor = VehicleAttr.plateColor;
		 deviceInfo->DataUnion.deviceStreet.vehicleType= VehicleAttr.vehicleType;
		 memcpy(&deviceInfo->DataUnion.deviceStreet.time, &VehicleAttr.snapTime, sizeof(TimeYMD_T));
		 memcpy(&deviceInfo->DataUnion.deviceStreet.plateNo, &VehicleAttr.plateNo, sizeof(TimeYMD_T));
	}
	else if(deviceInfo->type == DEVICE_TYPE_Entrance)
	{
		 snprintf(deviceInfo->DataUnion.deviceEntrance.snapToken, 128, "%04d%02d%02d%02d%02d%02d%03d", tv.curTime.year, tv.curTime.month, 
		   tv.curTime.day, tv.curTime.hour, tv.curTime.min, tv.curTime.sec, 
		   (int)rand()%1000);
		 
		 deviceInfo->DataUnion.deviceEntrance.plateColor = VehicleAttr.plateColor;
		 deviceInfo->DataUnion.deviceEntrance.vehicleType= VehicleAttr.vehicleType;
		 deviceInfo->DataUnion.deviceEntrance.vehicleTerr= VehicleAttr.vehicleTerr;
		 deviceInfo->DataUnion.deviceEntrance.vehicleColor = VehicleAttr.vehicleColor;
		 memcpy(&deviceInfo->DataUnion.deviceEntrance.vehicleStart, &VehicleAttr.snapTime, sizeof(TimeYMD_T));
		 memcpy(&deviceInfo->DataUnion.deviceEntrance.vehicleEnd, &VehicleAttr.snapTime, sizeof(TimeYMD_T));
		 memcpy(&deviceInfo->DataUnion.deviceEntrance.plateNo, &VehicleAttr.plateNo, sizeof(TimeYMD_T));
		 memcpy(&deviceInfo->DataUnion.deviceEntrance.vehicleImagePath, &deviceInfo->DataUnion.deviceEntrance.snapToken, 128);
		 for(i = 0; i < struITSPlateResult.dwPicNum; i++)
		 {
			 if ((struITSPlateResult.struPicInfo[i].dwDataLen != 0)&&(struITSPlateResult.struPicInfo[i].byType== 1)||(struITSPlateResult.struPicInfo[i].byType == 2))
			 {
			 	 if(KEY_FALSE ==  access(FILEPATH, F_OK))
				 {
					mkdir(FILEPATH, S_IRWXU | S_IRWXG | S_IRWXO);
				 }
				 snprintf(filename, 512, "%s/%s.jpg",FILEPATH, deviceInfo->DataUnion.deviceEntrance.snapToken);
				 fSnapPic = fopen(filename,"wb+");
				 if(NULL == fSnapPic)
				 {
					 return KEY_FALSE;
				 }
				 fwrite(struITSPlateResult.struPicInfo[i].pBuffer, struITSPlateResult.struPicInfo[i].dwDataLen,1,fSnapPic);
				 fclose(fSnapPic);
			 }
		 }
	}
	else if(deviceInfo->type == DEVICE_TYPE_Wash)
	{
		snprintf(deviceInfo->DataUnion.deviceWash.snapToken, 128, "%04d%02d%02d%02d%02d%02d%03d", tv.curTime.year, tv.curTime.month, 
		  tv.curTime.day, tv.curTime.hour, tv.curTime.min, tv.curTime.sec, 
		  (int)rand()%1000);
		memcpy(&deviceInfo->DataUnion.deviceWash.plateNo, &VehicleAttr.plateNo, sizeof(TimeYMD_T));
		memcpy(&deviceInfo->DataUnion.deviceWash.vehicleStart, &VehicleAttr.snapTime, sizeof(TimeYMD_T));
		memcpy(&deviceInfo->DataUnion.deviceWash.vehicleEnd, &VehicleAttr.snapTime, sizeof(TimeYMD_T));
	}
	else if(deviceInfo->type == DEVICE_TYPE_Island)
	{
		snprintf(deviceInfo->DataUnion.deviceOliLand.snapToken, 128, "%04d%02d%02d%02d%02d%02d%03d", VehicleAttr.snapTime.year, VehicleAttr.snapTime.month, 
			VehicleAttr.snapTime.day, VehicleAttr.snapTime.hour, VehicleAttr.snapTime.min, VehicleAttr.snapTime.sec, 
			(int)rand()%1000);
		memcpy(&deviceInfo->DataUnion.deviceOliLand.plateNo, &VehicleAttr.plateNo, 128);
		memcpy(&deviceInfo->DataUnion.deviceOliLand.snaptime, &VehicleAttr.snapTime, sizeof(TimeYMD_T));
	}
	//DF_DEBUG("===%s",struITSPlateResult.struPlateInfo.sLicense);
	//DF_DEBUG("===%d",strlen(struITSPlateResult.struPlateInfo.sLicense));
	//DF_DEBUG("车牌号:   %s", out);  //车牌号
	//DF_DEBUG("通道号:   %d", struITSPlateResult.byChanIndex);
	//DF_DEBUG("通道号:   %d", struITSPlateResult.byChanIndexEx);
	//DF_DEBUG("SerialNumber: %s", SerialNumber);
	return KEY_TRUE;
}

//混合告警
int vca_DataPrase(char *pAlarmInfo, char *SerialNumber, DataAtt_T *deviceInfo)
{
	if(NULL == pAlarmInfo){
		return KEY_FALSE;
	}
	int  channel = -1, iret = 0;
	int  num = 0, i = 0, cont = 0, mm = 0;
	char msgbuf[4096] = {0};
	char date[64] = {0};
	cJSON *result = NULL;
	VehicleAtt_T  VehicleAttr = {0};
	
	httpHeadPrase(pAlarmInfo, msgbuf);
	cJSON *data = cJSON_Parse(msgbuf);
	if(NULL == data){
		return KEY_FALSE;
	}

	channel = cJSON_GetObjectItemValueInt(data, (char *)"channelID");
	cJSON_GetObjectItemValueString(data, (char *)"dateTime", (char *)date, sizeof(date));
	sscanf(date, "%d-%d-%dT%d:%d:%d", &VehicleAttr.snapTime.year, &VehicleAttr.snapTime.month, 
		&VehicleAttr.snapTime.day, &VehicleAttr.snapTime.hour, &VehicleAttr.snapTime.min, &VehicleAttr.snapTime.sec);

	cJSON *cJCaptureResult = cJSON_GetObjectItem(data, (char *)"CaptureResult");
	if(NULL == cJCaptureResult){
		return KEY_FALSE;
	}

	num = cJSON_GetArraySize(cJCaptureResult);
	for(i = 0; i < num; i++)
	{
		result = cJSON_GetArrayItem(cJCaptureResult, i);
		if(result != NULL)
		{
			cJSON *cJVehicle = cJSON_GetObjectItem(result, (char *)"Vehicle");
			if(NULL != cJVehicle)
			{
				cJSON_GetObjectItemValueString(cJVehicle, (char *)"vehicleImageURL", VehicleAttr.plateImagePath, 128);
				cJSON *cJProperty = cJSON_GetObjectItem(cJVehicle, (char *)"Property");
				if(NULL != cJProperty)
				{
					cont = cJSON_GetArraySize(cJProperty);
					for(mm = 0; mm < cont; mm++)
					{
						cJSON *cJitem = cJSON_GetArrayItem(cJProperty, mm);
						if(NULL != cJitem)
						{
							char description[64] = {0};
							char desvalue[128] = {0};
							cJSON_GetObjectItemValueString(cJitem, (char *)"description", (char *)description, 64);
							cJSON_GetObjectItemValueString(cJitem, (char *)"value", (char *)desvalue, 128);
							if(strcmp(description, "plateColor") == 0)
							{
								VehicleAttr.plateColor = ConfirmTheColorOfThePlate(desvalue); 
							}

							if(strcmp(description, "plateNo") == 0)
							{
								memcpy(VehicleAttr.plateNo, desvalue, 128);
								DataAnalysisManage_T handle = {0};
								VehicleDataAnalysisManageRegister(&handle);
								VehicleAttr.vehicleTerr = handle.pGetVehiclesTerritory(desvalue);
								VehicleDataAnalysisManageUnRegister(&handle);
							}
							
							if(strcmp(description, "vehicleType") == 0)
							{
								VehicleAttr.vehicleType = ConfirmTheTypeOfTheVehicle(desvalue);
							}

							if(strcmp(description, "vehicleColor") == 0)
							{
								VehicleAttr.vehicleColor = ConfirmTheColorOfTheVehicle(desvalue);
							}
						}
					}
				}
			}
		}
	}

	HkDeviceParam_T ipcamInfo = {0};
	iret = GetDeviceType(channel, SerialNumber, &ipcamInfo);
	if(iret != KEY_TRUE)
	{
		return KEY_FALSE;
	}
	deviceInfo->type = (DeviceType_T)ipcamInfo.sDeviceType;
	if(DEVICE_TYPE_Frontage == deviceInfo->type)
	{
		snprintf(deviceInfo->DataUnion.deviceStreet.snapToken, 128, "%04d%02d%02d%02d%02d%02d%03d", VehicleAttr.snapTime.year, VehicleAttr.snapTime.month, 
			VehicleAttr.snapTime.day, VehicleAttr.snapTime.hour, VehicleAttr.snapTime.min, VehicleAttr.snapTime.sec, 
			(int)rand()%1000);
		deviceInfo->DataUnion.deviceStreet.vehicleType = VehicleAttr.vehicleType;
		deviceInfo->DataUnion.deviceStreet.plateColor = VehicleAttr.plateColor;		
		memcpy(&deviceInfo->DataUnion.deviceStreet.plateNo, &VehicleAttr.plateNo, 128);
		memcpy(&deviceInfo->DataUnion.deviceStreet.time, &VehicleAttr.snapTime, sizeof(TimeYMD_T));
	}
	else if(DEVICE_TYPE_Entrance == deviceInfo->type)
	{
		snprintf(deviceInfo->DataUnion.deviceEntrance.snapToken, 128, "%04d%02d%02d%02d%02d%02d%03d", VehicleAttr.snapTime.year, VehicleAttr.snapTime.month, 
			VehicleAttr.snapTime.day, VehicleAttr.snapTime.hour, VehicleAttr.snapTime.min, VehicleAttr.snapTime.sec, 
			(int)rand()%1000);
		deviceInfo->DataUnion.deviceEntrance.plateColor  = VehicleAttr.plateColor;
		deviceInfo->DataUnion.deviceEntrance.vehicleType = VehicleAttr.vehicleType;
		deviceInfo->DataUnion.deviceEntrance.vehicleColor= VehicleAttr.vehicleColor;
		deviceInfo->DataUnion.deviceEntrance.vehicleTerr = VehicleAttr.vehicleTerr;
		memcpy(&deviceInfo->DataUnion.deviceEntrance.vehicleStart, &VehicleAttr.snapTime, sizeof(TimeYMD_T));
		memcpy(&deviceInfo->DataUnion.deviceEntrance.vehicleEnd, &VehicleAttr.snapTime, sizeof(TimeYMD_T));
		memcpy(&deviceInfo->DataUnion.deviceEntrance.plateNo, &VehicleAttr.plateNo, 128);
		memcpy(&deviceInfo->DataUnion.deviceEntrance.vehicleImagePath, &VehicleAttr.plateImagePath, 128);
	}
	else if(DEVICE_TYPE_Wash == deviceInfo->type)
	{
		snprintf(deviceInfo->DataUnion.deviceOliLand.snapToken, 128, "%04d%02d%02d%02d%02d%02d%03d", VehicleAttr.snapTime.year, VehicleAttr.snapTime.month, 
			VehicleAttr.snapTime.day, VehicleAttr.snapTime.hour, VehicleAttr.snapTime.min, VehicleAttr.snapTime.sec, 
			(int)rand()%1000);
		memcpy(&deviceInfo->DataUnion.deviceWash.plateNo, &VehicleAttr.plateNo, sizeof(TimeYMD_T));
		memcpy(&deviceInfo->DataUnion.deviceWash.vehicleStart, &VehicleAttr.snapTime, sizeof(TimeYMD_T));
		memcpy(&deviceInfo->DataUnion.deviceWash.vehicleEnd, &VehicleAttr.snapTime, sizeof(TimeYMD_T));
	}
	else if(DEVICE_TYPE_Island == deviceInfo->type)
	{
		snprintf(deviceInfo->DataUnion.deviceOliLand.snapToken, 128, "%04d%02d%02d%02d%02d%02d%03d", VehicleAttr.snapTime.year, VehicleAttr.snapTime.month, 
			VehicleAttr.snapTime.day, VehicleAttr.snapTime.hour, VehicleAttr.snapTime.min, VehicleAttr.snapTime.sec, 
			(int)rand()%1000);
		memcpy(&deviceInfo->DataUnion.deviceOliLand.plateNo, &VehicleAttr.plateNo, 128);
		memcpy(&deviceInfo->DataUnion.deviceOliLand.snaptime, &VehicleAttr.snapTime, sizeof(TimeYMD_T));
	}
	else
	{
		return KEY_FALSE;
	}
	return KEY_TRUE;
}

int thermometry_DataPrase(char *pAlarmInfo, char *SerialNumber, DataAtt_T *deviceInfo)
{
	int iret = 0;
	Time_T tv = {0};
	HkDeviceParam_T ipcamInfo = {0};
	NET_DVR_THERMOMETRY_ALARM *pThermometry = NULL;

	if(NULL == pAlarmInfo || NULL == SerialNumber || NULL == deviceInfo){
		return KEY_FALSE;
	}

	pThermometry =(NET_DVR_THERMOMETRY_ALARM *) pAlarmInfo;
	iret = GetDeviceType(pThermometry->dwChannel, SerialNumber, &ipcamInfo);
	if(iret != KEY_TRUE){
		return KEY_FALSE;
	}
	deviceInfo->type = (DeviceType_T)ipcamInfo.sDeviceType;
	deviceInfo->DataUnion.deviceAlarm.alarmType = ALARM_HIGH_TEM;
	memcpy(deviceInfo->DataUnion.deviceAlarm.alarmPlace, ipcamInfo.sDeviceName, NET_DVR_MAX_LEN);

	GetLocalTime(&tv);
	memcpy(&deviceInfo->DataUnion.deviceAlarm.time, &tv.curTime, sizeof(TimeYMD_T));
	snprintf(deviceInfo->DataUnion.deviceAlarm.snapToken, 128, "%04d%02d%02d%02d%02d%02d%03d", deviceInfo->DataUnion.deviceAlarm.time.year, deviceInfo->DataUnion.deviceAlarm.time.month, 
		deviceInfo->DataUnion.deviceAlarm.time.day, deviceInfo->DataUnion.deviceAlarm.time.hour, deviceInfo->DataUnion.deviceAlarm.time.min, deviceInfo->DataUnion.deviceAlarm.time.sec, 
		(int)rand()%1000);
	
	return KEY_TRUE;
}

int firedetection_DataPrase(char *pAlarmInfo, char *SerialNumber, DataAtt_T *deviceInfo)
{
	int iret = 0;
	Time_T tv = {0};
	HkDeviceParam_T ipcamInfo = {0};
	NET_DVR_FIREDETECTION_ALARM *pFiredetection = NULL;
	
	if(NULL == pAlarmInfo || NULL == SerialNumber || NULL == deviceInfo){
		return KEY_FALSE;
	}

	pFiredetection = (NET_DVR_FIREDETECTION_ALARM *)pAlarmInfo;
	iret = GetDeviceType(pFiredetection->struDevInfo.byIvmsChannel, SerialNumber, &ipcamInfo);
	if(iret != KEY_TRUE){
		return KEY_FALSE;
	}
	
	deviceInfo->type = (DeviceType_T)ipcamInfo.sDeviceType;
	deviceInfo->DataUnion.deviceAlarm.alarmType = ALARM_SMOKE;
	memcpy(deviceInfo->DataUnion.deviceAlarm.alarmPlace, ipcamInfo.sDeviceName, NET_DVR_MAX_LEN);

	GetLocalTime(&tv);
	memcpy(&deviceInfo->DataUnion.deviceAlarm.time, &tv.curTime, sizeof(TimeYMD_T));
	snprintf(deviceInfo->DataUnion.deviceAlarm.snapToken, 128, "%04d%02d%02d%02d%02d%02d%03d", deviceInfo->DataUnion.deviceAlarm.time.year, deviceInfo->DataUnion.deviceAlarm.time.month, 
		deviceInfo->DataUnion.deviceAlarm.time.day, deviceInfo->DataUnion.deviceAlarm.time.hour, deviceInfo->DataUnion.deviceAlarm.time.min, deviceInfo->DataUnion.deviceAlarm.time.sec, 
		(int)rand()%1000);
	
	return KEY_TRUE;
}

