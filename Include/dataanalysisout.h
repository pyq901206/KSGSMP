#include "common.h"
#include "devicemanage.h"

#ifndef __DATAANALYSISMANAGEOUT_H__
#define __DATAANALYSISMANAGEOUT_H__

typedef enum _ResidenceType_T{
	RESIDENCE_NONE = 0,	//
	RESIDENCE_MIN1,		//小于等于5min
	RESIDENCE_MIN2,     //大于5min，小于20min 
	RESIDENCE_MIN3,	    //大于20min
}ResidenceType_T;

typedef enum _SerchType_T{
	SERCH_CAR_LOCATION = 0,	//归属地
	SERCH_CAR_TYPE,			//车辆类型
	SERCH_CAR_ALL
}SerchType_T;

typedef enum _DeviceType_T{
	DEVICE_TYPE_NONE = 0,
	DEVICE_TYPE_Frontage,   //临街抓拍机
	DEVICE_TYPE_Entrance,   //出入口抓拍机
	DEVICE_TYPE_Wash,		//洗车抓拍机
	DEVICE_TYPE_Attendance,	//考勤抓拍机
	DEVICE_TYPE_Hot_1,		//热区域抓拍机1
	DEVICE_TYPE_Hot_2,		//热区域抓拍机2
	DEVICE_TYPE_Customer,	//客流量抓拍机
	DEVICE_TYPE_Island,		//加油岛抓拍机
	DEVICE_TYPE_Alarm,		//告警抓拍机
}DeviceType_T;

typedef enum _VEHICLE_TYPE_E
{
	VEHICLE_TYPE_NONE = 0,
	VEHICLE_TYPE_Car,
}VEHICLE_TYPE_E;

typedef enum _TimingType_E
{
	TIMING_TYPE_NONE = 0,
	TIMING_TYPE_DAY,
	TIMING_TYPE_MONTH,
	TIMING_TYPE_YEAR,
}TimingType_E;

//车牌类型
typedef enum _Plate_Type_E
{
	plate_type_unknown = 0, 	 //未知    
	plate_type_92TypeCivil = 1, //92式民用车 
	plate_type_arm = 2, 		 //警车 
	plate_type_upDownMilitay = 3, 	  //上下军车 
	plate_type_92TypeArm = 4, 	 	  //92式武警车 
	plate_type_leftRightMilitay = 5, //左右军车 
	plate_type_02TypePersonalized = 7,  //02式个性化车 
	plate_type_yellowTwoLine = 8, //黄色双行尾牌 
	plate_type_04NewMilitay = 9,  //04式新军车 
	plate_type_embassy = 10, //使馆车 
	plate_type_oneLineArm = 11, //一行结构的新武警车 
	plate_type_twoLineArm = 12, //两行结构的新武警车 
	plate_type_yellow1225FarmVehicle = 13, //黄色1225农用车 
	plate_type_green1325FarmVehicle = 14, //绿色1325农用车 
	plate_type_yellow1325FarmVehicle = 15, //黄色1325结构农用车 
	plate_type_motorola = 16, //摩托车  
	plate_type_newEnergy = 17, //新能源车牌 
	plate_type_coach = 100, //教练车 
	plate_type_tempTravl = 101, //临时行驶车 
	plate_type_trailer = 102, //挂车 
	plate_type_consulate = 103, //领馆汽车 
	plate_type_hongKongMacao = 104, //港澳入出车 
	plate_type_tempEntry = 105, //临时入境车
	plate_type_civilAviation,   //民航车牌  
}Plate_Type_E;

//车牌颜色
typedef enum _Plate_Color_E
{
	platecolor_null = 0, //未知	  
	white = 1, //白色 
	yellow, //黄色 
	blue,  //蓝色 
	black, //黑色 
	green, //绿色 
}Plate_Color_E;

//车辆类型
typedef enum _Vehicle_Type_E
{
	vehicletype_Null = 0, //未知	  
	largeBus, 			  //大型客车 
	truck,  			  //货车 
	vehicle,  			  //轿车 
	van,  				  //面包车 
	vehicletype_all
}Vehicle_Type_E;

//车辆颜色
typedef enum _Vehicle_Color_E{
	vehicle_color_unknown = 0, //未知   
	vehicle_color_white,   //白 
	vehicle_color_silver,  //银 
	vehicle_color_gray,    //灰 
	vehicle_color_black,   //黑 
	vehicle_color_red,     //红 
	vehicle_color_deepBlue,  //深蓝 
	vehicle_color_blue,  //蓝 
	vehicle_color_yellow,  //黄 
	vehicle_color_green,  //绿 
	vehicle_color_brown,  //棕 
	vehicle_color_pink,  //粉 
	vehicle_color_purple,  //紫 
	vehicle_color_deepGray,  //深灰 
	vehicle_color_cyan,  //青 
	vehicle_color_orange,  //橙 
}Vehicle_Color_E;

typedef enum _SearchType_E
{
	VehicleSearch_TYPE_NULL = 0,
	VehicleSearch_TYPE_Terr = 1 << 0,
	VehicleSearch_TYPE_Vehicle = 1 << 1,
	VehicleSearch_TYPE_Fuzzy = 1 << 2,
	VehicleSearch_TYPE_Increment = 1 << 3,
	VehicleSearch_TYPE_Losses = 1 << 4
}VehicleSearchType_E;

typedef enum _AlarmLevelSearchType_E
{
	AlarmLevelSearchType_TYPE_NULL = 0,
	AlarmLevelSearchType_TYPE_HighTemp = 1 << 0,
	AlarmLevelSearchType_TYPE_Smoking = 1 << 1,
}AlarmLevelSearchType_E;

typedef enum _VechicleSituation_E
{
	Situation_TYPE_NULL = 0,
	Situation_TYPE_Increment = 1,
	Situation_TYPE_Losses    = 2
}VechicleSituation_E;

typedef struct _VehicleType_T
{
	int VanCount;	//面包车
	int BusCount;   //大客车
	int TruckCount; //大货车
	int CarCount;	//小轿车
	int OtherCount; //其他
}VehicleType_T;

typedef struct _PlateColor_T
{
	int BlueCount;    //蓝牌
	int YellowCount;  //黄牌
	int GreenCount;   //绿牌
	int OtherCount;   //其他
}PlateColor_T;

typedef struct _PlaceAttribution_T
{
	int ProvinceCount;    //省内
	int OutProvinceCount; //省外
}PlaceAttribution_T;

typedef struct _RefuelingType_T
{
	int GasolineCount;	 //汽油
	int DieselOilCount;  //柴油
}RefuelingType_T;

typedef struct _StreetTypeAttr
{
	VehicleType_T vehickeType;
	PlateColor_T  plateType;
}StreetTypeAttr;

typedef struct _ProgressRate_T
{
	int PassingCount;	//途经车辆总数
	int IncomingCount;	//进站车辆总数
}ProgressRate_T;

typedef struct _ResidenceTime_T
{
	int count1;  //<=5min
	int count2;  //5-20min
	int count3;  //>=20min
}ResidenceTime_T;

typedef struct _Refueling_T
{
	int total;	 //车辆总数
	int average; //停留时间
}Refueling_T;

//站内
typedef struct _StationVehicles_T
{
	VechicleSituation_E situationType;
	Vehicle_Type_E 	vehicleType;
	TimeYMD_T vehicleStart; 	  
	TimeYMD_T vehicleEnd;
	char fuzzyStr[128];
}StationVehicles_T;

typedef struct _StationVehiclesInfo_T
{
	char snapToken[128]; //每次抓拍记录唯一ID
	char picPath[128];	 //图片url
	char plateNo[128];   //车牌号码
	char territory[128]; //所属地
	char plateClor[128]; //车牌颜色
	char carType[128];   //车辆类型
	int  refuelNo;		 //频次
	TimeYMD_T time;		 //加油时间
}StationVehiclesInfo_T;


typedef struct _AbnormalVehicleInfo_T
{
	int  staytime;		 //停留时间
	TimeYMD_T time;		 //进站时间
	char plateNo[128];   //车牌号码
	char carType[128];   //车辆类型
	char territory[128]; //归属地
}AbnormalVehicleInfo_T;

//历史记录
typedef struct _StationHistoryInfo_T
{
	char plateNo[128];   //车牌号码
	char carType[128];   //车辆类型
	char territory[128]; //所属地
	char plateClor[128]; //车牌颜色
	TimeYMD_T time;		 //加油时间
}StationHistoryInfo_T;


typedef struct _VehicleSearchInfo_T
{
	Vehicle_Type_E 	vehicleType;
	VehicleSearchType_E type;
	int	vehicleTerr;
	char fuzzyStr[128];
}VehicleSearchInfo_T;

//出入口数据结构
typedef struct _DeviceEntrance_T{
	char snapToken[128]; 		  //每次抓拍记录唯一ID
	char plateNo[128];			  //车牌号码
	char vehicleImagePath[128];   //抓拍车辆图片本地路径
	Plate_Color_E   plateColor;	  //车牌颜色
	Vehicle_Type_E  vehicleType;  //车辆类型
	Vehicle_Color_E vehicleColor; //车辆颜色
	int 	  vehicleTerr;  	  //车辆所属地
	TimeYMD_T vehicleStart; 	  //进站时间
	TimeYMD_T vehicleEnd;		  //出站时间
	int 	  stay;
}DeviceEntrance_T;

//出入口数据库结构体
typedef struct _DeviceEntranceMysql_T{
	int 	  staytime;
	int 	  vehicleTerr;  	  //车辆所属地
	Vehicle_Type_E  vehicleType;  //车辆类型
	TimeYMD_T vehicleStart; 	  //进站时间
	char plateNo[128];			  //车牌号码
}DeviceEntranceMysql_T;

//临街数据结构
typedef struct _DeviceStreet_T{
	char snapToken[128]; 		  //每次抓拍记录唯一ID
	Plate_Color_E   plateColor;	  //车牌颜色
	Vehicle_Type_E  vehicleType;  //车辆类型
	TimeYMD_T time; 	  		  //抓拍时间
	char plateNo[128];			  //车牌号码
}DeviceStreet_T;


//洗车数据结构
typedef struct _DeviceWash_T{
	char snapToken[128]; 		  //每次抓拍记录唯一ID
	char plateNo[128];			  //车牌号码
	TimeYMD_T vehicleStart; 	  //入口时间
	TimeYMD_T vehicleEnd;		  //出口时间
}DeviceWash_T;

//加油岛车辆数据结构
typedef struct _DeviceOilLand_T
{
	char snapToken[128]; 		  //每次抓拍记录唯一ID
	char plateNo[128];			  //车牌号码
	TimeYMD_T snaptime; 	 	  //抓拍时间
}DeviceOilLand_T;


//车辆轨迹
typedef struct _DeviceTrajectory_T
{
	char snapToken[128]; 		  //每次抓拍记录唯一ID
	char plateNo[128];			  //车牌号码
	TimeYMD_T arrivalstartdate;   //进站时间
	TimeYMD_T arrivalenddate;     //出站时间
	TimeYMD_T washstartdate;      //洗车开始时间
	TimeYMD_T washenddate;     	  //洗车出站时间
}DeviceTrajectory_T;

typedef enum _DeviceAlarmType_T{
	ALARM_SMOKE = 1,	//动态火点告警
	ALARM_HIGH_TEM		//高温
}DeviceAlarmType_T;

//告警级别
typedef enum _DeviceAlarmLeave_T{
	ALARM_LEAVE_1 = 1,
	ALARM_LEAVE_2 = 2
}DeviceAlarmLeave_T;


typedef struct _DeviceAlarmParam_T
{
	int smokeLev;
	int hightempLev;
}DeviceAlarmParam_T;

//告警数据结构
typedef struct _DeviceAlarm_T{
	char snapToken[128]; 		  //每次抓拍记录唯一ID
	DeviceAlarmType_T  alarmType; //告警类型
	char alarmPlace[128];		  //告警地点
	TimeYMD_T time; 	 		  //告警时间
}DeviceAlarm_T;

//web显示结构
typedef struct _DeviceAlarmInfo_T{
	int  alrmtype;			  	  //告警类型
	int  alarmlev;			  	  //告警级别
	char time[128]; 	 		  //告警时间
	char alarmdata[128];          //告警地点
}DeviceAlarmInfo_T;

typedef struct _DeviceAlarmCount_T{
	int smokecount;
	int hightemcount;
}DeviceAlarmCount_T;

//vip车辆数据结构
typedef struct _DeviceVipCar_T{
	DeviceEntrance_T  vehicle;
	int  conut;					  //加油次数
	TimeYMD_T time; 	 		  //近期加油时间
}DeviceVipCar_T;

//洗车加油占比
typedef struct _DeviceWashProportion_T
{
	int washcount;				 //洗车
	int oilcount;				 //加油
	int meanwhilecount;			 //洗车+加油
}DeviceWashProportion_T;

//洗车占比
typedef struct _DeviceWashStatistics_T
{
	int washcount;      		//洗车总数
	int totalcount;				//站内总数
	int washwithOutaddnum;		//只洗不加
	int addwithOutwashnum;	    //只加不洗
	int addandwashnum;		    //先加后洗
	int washandaddnum;			//先洗后加
}DeviceWashStatistics_T;

typedef struct _TurnInRate_T{
	char carType[128];			  //车辆类型
	char plateNo[128];			  //车牌号码
	int  conut;					  //加油次数
	TimeYMD_T time; 	 		  //近期加油时间
}TurnInRate_T;

typedef struct _VehicleAtt_T
{
	 TimeYMD_T snapTime;			//抓拍时间
	 Plate_Type_E    plateType;     //车牌类型
	 Plate_Color_E   plateColor;    //车牌颜色
	 Vehicle_Type_E  vehicleType;   //车辆类型
	 Vehicle_Color_E vehicleColor;  //车辆颜色
	 int vehicleTerr;      		    //车辆所属地
	 char plateNo[128];             //车牌号码
	 char plateImagePath[128];      //抓拍全景图片本地路径
}VehicleAtt_T;
 
typedef struct _AttendanceAtt_T{
	TimeYMD_T snapTime;     //抓拍时间
	char phonenum[12];
	unsigned char name[64];   //打卡人名字
	char plateImagePath[128];   //抓拍全景图片本地路径
	char snapImagePath[128];   //抓拍人脸图片本地路径
	char snapToken[128];    //每次抓拍记录唯一ID
	char usrid[128];	
}AttendanceAtt_T,*pAttendanceAtt_T;


typedef struct _VIPCustomer_T{
	int  GradeScore;
	int	StoreFrequency;				//进店频率
	TimeYMD_T snapTime;  			//抓拍时间
	unsigned char name[64];			//打卡人名字
	char plateImagePath[128]; 		//抓拍全景图片本地路径
	char snapImagePath[128]; 		//抓拍人脸图片本地路径
	char snapToken[128]; 			//每次抓拍记录唯一ID
}VIPCustomer_T,*pVIPCustomer_T;

typedef struct _Customer_T{
	TimeYMD_T snapTime;  			//抓拍时间
	unsigned char name[64];			//打卡人名字
	char plateImagePath[128]; 		//抓拍全景图片本地路径
	char snapImagePath[128]; 		//抓拍人脸图片本地路径
	char snapToken[128]; 			//每次抓拍记录唯一ID
}Customer_T,*pCustomer_T;

typedef enum _FuelQuantitySystemType_E
{
	FuelQuantityType_NULL,
	FuelQuantityType_PointSale,		//零售系统
	FuelQuantityType_StuckPipe,		//卡管系统
}FuelQuantitySystemType_E;

typedef struct _StuckPipeInfo_T
{
	char stationName[128];
	int oils;
	float LPM;
	int number;
	TimeYMD_T date;
}StuckPipeInfo_T;

typedef struct _PointSaleInfo_T
{

}PointSaleInfo_T;

typedef enum _ReportType_E
{
	REPORTTYPE_DAY = 0,
	REPORTTYPE_WEEK = 1,
	REPORTTYPE_MON= 2,
	REPORTTYPE_CUSTOM= 3,
}ReportType_E;

typedef struct _LeaveAndEnterNum_T
{
	int dwLeaveNum;        // 离开人数
	int dwEnterNum;        // 进入人数	
	char time[128];  	   //抓拍时间
}LeaveAndEnterNum_T;

typedef struct _Sex_T
{
	int male;				//男性
	int female;				//女性
}VolumeOfCommutersOfSex_T;

typedef struct _Age_T
{
	int kid;				//小孩
	int young;				//中年
	int old;				//老年
}VolumeOfCommutersOfAge_T;


typedef struct _VolumeOfCommuters_T{
	char snapToken[128]; 			//每次抓拍记录唯一ID
	LeaveAndEnterNum_T				LeaveAndEnterAttr;
	VolumeOfCommutersOfSex_T		SexAttr;
	VolumeOfCommutersOfAge_T		AgeAttr;
}VolumeOfCommuters_T,*pVolumeOfCommuters_T;

typedef struct _DataAtt_T
{	
	DeviceType_T type;				 //设备类型		
	union
	{
		DeviceStreet_T    deviceStreet;
		DeviceEntrance_T  deviceEntrance;
		DeviceWash_T	  deviceWash;
		DeviceVipCar_T 	  deviceVipCar;
		DeviceAlarm_T	  deviceAlarm;
		DeviceOilLand_T   deviceOliLand;
		AttendanceAtt_T	  attendanceattr;
		VIPCustomer_T	  vipcustomerattr;
		Customer_T		  customerattr;
		VolumeOfCommuters_T volumeofcommutersattr;
	}DataUnion;
}DataAtt_T;



#endif

