#include "common.h"
#include "devicemanage.h"

#ifndef __DATAANALYSISMANAGEOUT_H__
#define __DATAANALYSISMANAGEOUT_H__

typedef enum _ResidenceType_T{
	RESIDENCE_NONE = 0,	//
	RESIDENCE_MIN1,		//С�ڵ���5min
	RESIDENCE_MIN2,     //����5min��С��20min 
	RESIDENCE_MIN3,	    //����20min
}ResidenceType_T;

typedef enum _SerchType_T{
	SERCH_CAR_LOCATION = 0,	//������
	SERCH_CAR_TYPE,			//��������
	SERCH_CAR_ALL
}SerchType_T;

typedef enum _DeviceType_T{
	DEVICE_TYPE_NONE = 0,
	DEVICE_TYPE_Frontage,   //�ٽ�ץ�Ļ�
	DEVICE_TYPE_Entrance,   //�����ץ�Ļ�
	DEVICE_TYPE_Wash,		//ϴ��ץ�Ļ�
	DEVICE_TYPE_Attendance,	//����ץ�Ļ�
	DEVICE_TYPE_Hot_1,		//������ץ�Ļ�1
	DEVICE_TYPE_Hot_2,		//������ץ�Ļ�2
	DEVICE_TYPE_Customer,	//������ץ�Ļ�
	DEVICE_TYPE_Island,		//���͵�ץ�Ļ�
	DEVICE_TYPE_Alarm,		//�澯ץ�Ļ�
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

//��������
typedef enum _Plate_Type_E
{
	plate_type_unknown = 0, 	 //δ֪    
	plate_type_92TypeCivil = 1, //92ʽ���ó� 
	plate_type_arm = 2, 		 //���� 
	plate_type_upDownMilitay = 3, 	  //���¾��� 
	plate_type_92TypeArm = 4, 	 	  //92ʽ�侯�� 
	plate_type_leftRightMilitay = 5, //���Ҿ��� 
	plate_type_02TypePersonalized = 7,  //02ʽ���Ի��� 
	plate_type_yellowTwoLine = 8, //��ɫ˫��β�� 
	plate_type_04NewMilitay = 9,  //04ʽ�¾��� 
	plate_type_embassy = 10, //ʹ�ݳ� 
	plate_type_oneLineArm = 11, //һ�нṹ�����侯�� 
	plate_type_twoLineArm = 12, //���нṹ�����侯�� 
	plate_type_yellow1225FarmVehicle = 13, //��ɫ1225ũ�ó� 
	plate_type_green1325FarmVehicle = 14, //��ɫ1325ũ�ó� 
	plate_type_yellow1325FarmVehicle = 15, //��ɫ1325�ṹũ�ó� 
	plate_type_motorola = 16, //Ħ�г�  
	plate_type_newEnergy = 17, //����Դ���� 
	plate_type_coach = 100, //������ 
	plate_type_tempTravl = 101, //��ʱ��ʻ�� 
	plate_type_trailer = 102, //�ҳ� 
	plate_type_consulate = 103, //������� 
	plate_type_hongKongMacao = 104, //�۰������ 
	plate_type_tempEntry = 105, //��ʱ�뾳��
	plate_type_civilAviation,   //�񺽳���  
}Plate_Type_E;

//������ɫ
typedef enum _Plate_Color_E
{
	platecolor_null = 0, //δ֪	  
	white = 1, //��ɫ 
	yellow, //��ɫ 
	blue,  //��ɫ 
	black, //��ɫ 
	green, //��ɫ 
}Plate_Color_E;

//��������
typedef enum _Vehicle_Type_E
{
	vehicletype_Null = 0, //δ֪	  
	largeBus, 			  //���Ϳͳ� 
	truck,  			  //���� 
	vehicle,  			  //�γ� 
	van,  				  //����� 
	vehicletype_all
}Vehicle_Type_E;

//������ɫ
typedef enum _Vehicle_Color_E{
	vehicle_color_unknown = 0, //δ֪   
	vehicle_color_white,   //�� 
	vehicle_color_silver,  //�� 
	vehicle_color_gray,    //�� 
	vehicle_color_black,   //�� 
	vehicle_color_red,     //�� 
	vehicle_color_deepBlue,  //���� 
	vehicle_color_blue,  //�� 
	vehicle_color_yellow,  //�� 
	vehicle_color_green,  //�� 
	vehicle_color_brown,  //�� 
	vehicle_color_pink,  //�� 
	vehicle_color_purple,  //�� 
	vehicle_color_deepGray,  //��� 
	vehicle_color_cyan,  //�� 
	vehicle_color_orange,  //�� 
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
	int VanCount;	//�����
	int BusCount;   //��ͳ�
	int TruckCount; //�����
	int CarCount;	//С�γ�
	int OtherCount; //����
}VehicleType_T;

typedef struct _PlateColor_T
{
	int BlueCount;    //����
	int YellowCount;  //����
	int GreenCount;   //����
	int OtherCount;   //����
}PlateColor_T;

typedef struct _PlaceAttribution_T
{
	int ProvinceCount;    //ʡ��
	int OutProvinceCount; //ʡ��
}PlaceAttribution_T;

typedef struct _RefuelingType_T
{
	int GasolineCount;	 //����
	int DieselOilCount;  //����
}RefuelingType_T;

typedef struct _StreetTypeAttr
{
	VehicleType_T vehickeType;
	PlateColor_T  plateType;
}StreetTypeAttr;

typedef struct _ProgressRate_T
{
	int PassingCount;	//;����������
	int IncomingCount;	//��վ��������
}ProgressRate_T;

typedef struct _ResidenceTime_T
{
	int count1;  //<=5min
	int count2;  //5-20min
	int count3;  //>=20min
}ResidenceTime_T;

typedef struct _Refueling_T
{
	int total;	 //��������
	int average; //ͣ��ʱ��
}Refueling_T;

//վ��
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
	char snapToken[128]; //ÿ��ץ�ļ�¼ΨһID
	char picPath[128];	 //ͼƬurl
	char plateNo[128];   //���ƺ���
	char territory[128]; //������
	char plateClor[128]; //������ɫ
	char carType[128];   //��������
	int  refuelNo;		 //Ƶ��
	TimeYMD_T time;		 //����ʱ��
}StationVehiclesInfo_T;


typedef struct _AbnormalVehicleInfo_T
{
	int  staytime;		 //ͣ��ʱ��
	TimeYMD_T time;		 //��վʱ��
	char plateNo[128];   //���ƺ���
	char carType[128];   //��������
	char territory[128]; //������
}AbnormalVehicleInfo_T;

//��ʷ��¼
typedef struct _StationHistoryInfo_T
{
	char plateNo[128];   //���ƺ���
	char carType[128];   //��������
	char territory[128]; //������
	char plateClor[128]; //������ɫ
	TimeYMD_T time;		 //����ʱ��
}StationHistoryInfo_T;


typedef struct _VehicleSearchInfo_T
{
	Vehicle_Type_E 	vehicleType;
	VehicleSearchType_E type;
	int	vehicleTerr;
	char fuzzyStr[128];
}VehicleSearchInfo_T;

//��������ݽṹ
typedef struct _DeviceEntrance_T{
	char snapToken[128]; 		  //ÿ��ץ�ļ�¼ΨһID
	char plateNo[128];			  //���ƺ���
	char vehicleImagePath[128];   //ץ�ĳ���ͼƬ����·��
	Plate_Color_E   plateColor;	  //������ɫ
	Vehicle_Type_E  vehicleType;  //��������
	Vehicle_Color_E vehicleColor; //������ɫ
	int 	  vehicleTerr;  	  //����������
	TimeYMD_T vehicleStart; 	  //��վʱ��
	TimeYMD_T vehicleEnd;		  //��վʱ��
	int 	  stay;
}DeviceEntrance_T;

//��������ݿ�ṹ��
typedef struct _DeviceEntranceMysql_T{
	int 	  staytime;
	int 	  vehicleTerr;  	  //����������
	Vehicle_Type_E  vehicleType;  //��������
	TimeYMD_T vehicleStart; 	  //��վʱ��
	char plateNo[128];			  //���ƺ���
}DeviceEntranceMysql_T;

//�ٽ����ݽṹ
typedef struct _DeviceStreet_T{
	char snapToken[128]; 		  //ÿ��ץ�ļ�¼ΨһID
	Plate_Color_E   plateColor;	  //������ɫ
	Vehicle_Type_E  vehicleType;  //��������
	TimeYMD_T time; 	  		  //ץ��ʱ��
	char plateNo[128];			  //���ƺ���
}DeviceStreet_T;


//ϴ�����ݽṹ
typedef struct _DeviceWash_T{
	char snapToken[128]; 		  //ÿ��ץ�ļ�¼ΨһID
	char plateNo[128];			  //���ƺ���
	TimeYMD_T vehicleStart; 	  //���ʱ��
	TimeYMD_T vehicleEnd;		  //����ʱ��
}DeviceWash_T;

//���͵��������ݽṹ
typedef struct _DeviceOilLand_T
{
	char snapToken[128]; 		  //ÿ��ץ�ļ�¼ΨһID
	char plateNo[128];			  //���ƺ���
	TimeYMD_T snaptime; 	 	  //ץ��ʱ��
}DeviceOilLand_T;


//�����켣
typedef struct _DeviceTrajectory_T
{
	char snapToken[128]; 		  //ÿ��ץ�ļ�¼ΨһID
	char plateNo[128];			  //���ƺ���
	TimeYMD_T arrivalstartdate;   //��վʱ��
	TimeYMD_T arrivalenddate;     //��վʱ��
	TimeYMD_T washstartdate;      //ϴ����ʼʱ��
	TimeYMD_T washenddate;     	  //ϴ����վʱ��
}DeviceTrajectory_T;

typedef enum _DeviceAlarmType_T{
	ALARM_SMOKE = 1,	//��̬���澯
	ALARM_HIGH_TEM		//����
}DeviceAlarmType_T;

//�澯����
typedef enum _DeviceAlarmLeave_T{
	ALARM_LEAVE_1 = 1,
	ALARM_LEAVE_2 = 2
}DeviceAlarmLeave_T;


typedef struct _DeviceAlarmParam_T
{
	int smokeLev;
	int hightempLev;
}DeviceAlarmParam_T;

//�澯���ݽṹ
typedef struct _DeviceAlarm_T{
	char snapToken[128]; 		  //ÿ��ץ�ļ�¼ΨһID
	DeviceAlarmType_T  alarmType; //�澯����
	char alarmPlace[128];		  //�澯�ص�
	TimeYMD_T time; 	 		  //�澯ʱ��
}DeviceAlarm_T;

//web��ʾ�ṹ
typedef struct _DeviceAlarmInfo_T{
	int  alrmtype;			  	  //�澯����
	int  alarmlev;			  	  //�澯����
	char time[128]; 	 		  //�澯ʱ��
	char alarmdata[128];          //�澯�ص�
}DeviceAlarmInfo_T;

typedef struct _DeviceAlarmCount_T{
	int smokecount;
	int hightemcount;
}DeviceAlarmCount_T;

//vip�������ݽṹ
typedef struct _DeviceVipCar_T{
	DeviceEntrance_T  vehicle;
	int  conut;					  //���ʹ���
	TimeYMD_T time; 	 		  //���ڼ���ʱ��
}DeviceVipCar_T;

//ϴ������ռ��
typedef struct _DeviceWashProportion_T
{
	int washcount;				 //ϴ��
	int oilcount;				 //����
	int meanwhilecount;			 //ϴ��+����
}DeviceWashProportion_T;

//ϴ��ռ��
typedef struct _DeviceWashStatistics_T
{
	int washcount;      		//ϴ������
	int totalcount;				//վ������
	int washwithOutaddnum;		//ֻϴ����
	int addwithOutwashnum;	    //ֻ�Ӳ�ϴ
	int addandwashnum;		    //�ȼӺ�ϴ
	int washandaddnum;			//��ϴ���
}DeviceWashStatistics_T;

typedef struct _TurnInRate_T{
	char carType[128];			  //��������
	char plateNo[128];			  //���ƺ���
	int  conut;					  //���ʹ���
	TimeYMD_T time; 	 		  //���ڼ���ʱ��
}TurnInRate_T;

typedef struct _VehicleAtt_T
{
	 TimeYMD_T snapTime;			//ץ��ʱ��
	 Plate_Type_E    plateType;     //��������
	 Plate_Color_E   plateColor;    //������ɫ
	 Vehicle_Type_E  vehicleType;   //��������
	 Vehicle_Color_E vehicleColor;  //������ɫ
	 int vehicleTerr;      		    //����������
	 char plateNo[128];             //���ƺ���
	 char plateImagePath[128];      //ץ��ȫ��ͼƬ����·��
}VehicleAtt_T;
 
typedef struct _AttendanceAtt_T{
	TimeYMD_T snapTime;     //ץ��ʱ��
	char phonenum[12];
	unsigned char name[64];   //��������
	char plateImagePath[128];   //ץ��ȫ��ͼƬ����·��
	char snapImagePath[128];   //ץ������ͼƬ����·��
	char snapToken[128];    //ÿ��ץ�ļ�¼ΨһID
	char usrid[128];	
}AttendanceAtt_T,*pAttendanceAtt_T;


typedef struct _VIPCustomer_T{
	int  GradeScore;
	int	StoreFrequency;				//����Ƶ��
	TimeYMD_T snapTime;  			//ץ��ʱ��
	unsigned char name[64];			//��������
	char plateImagePath[128]; 		//ץ��ȫ��ͼƬ����·��
	char snapImagePath[128]; 		//ץ������ͼƬ����·��
	char snapToken[128]; 			//ÿ��ץ�ļ�¼ΨһID
}VIPCustomer_T,*pVIPCustomer_T;

typedef struct _Customer_T{
	TimeYMD_T snapTime;  			//ץ��ʱ��
	unsigned char name[64];			//��������
	char plateImagePath[128]; 		//ץ��ȫ��ͼƬ����·��
	char snapImagePath[128]; 		//ץ������ͼƬ����·��
	char snapToken[128]; 			//ÿ��ץ�ļ�¼ΨһID
}Customer_T,*pCustomer_T;

typedef enum _FuelQuantitySystemType_E
{
	FuelQuantityType_NULL,
	FuelQuantityType_PointSale,		//����ϵͳ
	FuelQuantityType_StuckPipe,		//����ϵͳ
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
	int dwLeaveNum;        // �뿪����
	int dwEnterNum;        // ��������	
	char time[128];  	   //ץ��ʱ��
}LeaveAndEnterNum_T;

typedef struct _Sex_T
{
	int male;				//����
	int female;				//Ů��
}VolumeOfCommutersOfSex_T;

typedef struct _Age_T
{
	int kid;				//С��
	int young;				//����
	int old;				//����
}VolumeOfCommutersOfAge_T;


typedef struct _VolumeOfCommuters_T{
	char snapToken[128]; 			//ÿ��ץ�ļ�¼ΨһID
	LeaveAndEnterNum_T				LeaveAndEnterAttr;
	VolumeOfCommutersOfSex_T		SexAttr;
	VolumeOfCommutersOfAge_T		AgeAttr;
}VolumeOfCommuters_T,*pVolumeOfCommuters_T;

typedef struct _DataAtt_T
{	
	DeviceType_T type;				 //�豸����		
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

