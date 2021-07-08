#include "common.h"
#include "mysqlctrl.h"
#include "timemanage.h"
#include <unistd.h>
static MysqlHandle_T g_MysqlHandle;
static int backuprun = ENUM_ENABLED_FALSE;

typedef struct _Tab_Info_T
{
	char col_name[128];
	char data_type[128];
}Tab_Info_T;

static MysqlHandle_T* GetMySqlHandle()
{
	return &g_MysqlHandle;
}

static int MySqlUnInit()
{
	MYSQL *mysqlhandle = GetMySqlHandle()->handle;
    mysql_close(mysqlhandle);
	mysqlhandle = NULL;
	MUTEX_DESTROY(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int InitMysqlHanld()
{
	g_MysqlHandle.handle = NULL;
	g_MysqlHandle.handle = (MYSQL *)malloc(sizeof(MYSQL));
	memset(g_MysqlHandle.handle, 0, sizeof(MYSQL));
	MUTEX_INIT(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static void *MysqlHeartbeatTask(void *argv)
{
	prctl(PR_SET_NAME, (unsigned long)"MysqlHeartbeatTaskThead", 0,0,0);	
	int iret = -1;
	MysqlHandle_T *handle = (MysqlHandle_T *)argv;
	if (NULL == handle || NULL == handle->handle){
		DF_ERROR("conn_ptr is NULL!!");
		return NULL;
	}
	for(;;)
	{		
		MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
		iret = mysql_ping(handle->handle);
		if (iret != 0)
		{
			mysql_close(handle->handle);
			mysql_real_connect(handle->handle, MYSQL_SERVER_ADDRESS, MYSQL_SERVER_USR, MYSQL_SERVER_PASSWD, MYSQL_DB_NAME, 0, NULL, 0);
		}
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		usleep(5*1000*1000);  //先等待init初始化
	}
}

static int MysqlHeartbeat(MysqlHandle_T * mysqlHandle)
{
	pthread_t mysqlTaskID;
	if (pthread_create(&mysqlTaskID, NULL, MysqlHeartbeatTask, (void *)mysqlHandle))
	{
		DF_ERROR("pthread_create AcousticTask  Error");
		return KEY_FALSE;
	}
	pthread_detach(mysqlTaskID);
	return KEY_TRUE;
}

static int MysqlConnect(MysqlHandle_T *Mysqlhandle, const char *host, const char *user, const char *pwd, const char *db)
{  
	char reConnect = 1;
    if (!Mysqlhandle || !Mysqlhandle->handle) {
        return KEY_FALSE;  
    }
	Mysqlhandle->handle	= mysql_init(NULL);
	DF_DEBUG("%s %s %s %s", host, user, pwd, db);
    if (mysql_real_connect(Mysqlhandle->handle, host, user, pwd, db, 0, NULL, 0)) {  
        DF_DEBUG("Connection success\n"); 
    } else {  
        DF_ERROR("Connection failed\n");  
    }
	
	//mysql_set_character_set(Mysqlhandle->handle, "gbk");
	mysql_set_character_set(Mysqlhandle->handle, "utf8");
	mysql_options(Mysqlhandle->handle, MYSQL_OPT_RECONNECT, (char *)&reConnect);   //设置自动连接
	MysqlHeartbeat(Mysqlhandle); 
    return KEY_TRUE;  
}

//selct mysql
int MysqlSelect(MYSQL *conn_ptr, const char *sql)
{
    int res;  
    int cvpn_yes = 1;
	if (NULL == conn_ptr)
	{
        return KEY_FALSE;
	}
    res = mysql_query(conn_ptr, sql); //查询语句
    if (res) 
	{         
        DF_ERROR("SELECT error:%s\n",mysql_error(conn_ptr));     
        cvpn_yes = 0;
    }
	
    if (cvpn_yes == 1){
       return KEY_TRUE; 
    }
    else {
        return KEY_FALSE;
    }
	return KEY_FALSE;
}

//insert and update and delete
int MysqlExecute(MYSQL *conn_ptr, const char *sql)
{
    int res; 
    int cvpn_yes = 1;
    res = mysql_query(conn_ptr, sql); 
    if (!res) {
       // DF_DEBUG("%lu rows affected",(unsigned long)mysql_affected_rows(conn_ptr));   
    } else {
    	if (1062 == mysql_errno(conn_ptr))
    	{
		   DF_WARN("Insert error %d: %s",mysql_errno(conn_ptr),mysql_error(conn_ptr));
     	   return KEY_PRIMARYKEYERROR;
		}
        DF_ERROR("Insert error %d: %s",mysql_errno(conn_ptr),mysql_error(conn_ptr));
        cvpn_yes = 0;
    } 
    if (cvpn_yes == 1){
        return KEY_TRUE;
    }
    else {
        return KEY_FALSE;
    }
}

//发生错误时，输出错误信息，关闭连接，退出程序  
void error_quit(const char *str, MYSQL *connection)  
{  
    DF_ERROR("%s : %d: %s",  
        str, mysql_errno(connection),  
        mysql_error(connection));  
    if( connection != NULL )  
        mysql_close(connection);  
}

//判断数据库是否存在，不存在则创建数据库，并打开
int MysqlCreateDatabase(const char* dbname)
{
    MYSQL *my_con;
  	my_con = GetMySqlHandle()->handle;
    char queryStr[128] = "create database if not exists ";
	sprintf(queryStr,"%s%s", queryStr, dbname);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
    if (0 == mysql_query(my_con,queryStr))
    {	
    	memset(queryStr, 0, sizeof(queryStr));
		sprintf(queryStr,"%s","use ");
		sprintf(queryStr,"%s%s",queryStr,dbname);
        if (0 == mysql_query(my_con,queryStr))
        {
			MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
            return KEY_TRUE;
        }
    }
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
    return KEY_FALSE;
}

int MysqlFindRepeated(MYSQL *my_handle)
{
	int i = 0,j = 0;
    MYSQL_ROW sqlrow; 
    MYSQL_RES *res_ptr;  
	res_ptr = mysql_store_result(my_handle);  //取出结果集
	if(res_ptr)
	{               
		j = mysql_num_fields(res_ptr);
		while((sqlrow = mysql_fetch_row(res_ptr)))
		{
			i++;
		}

		if (mysql_errno(my_handle)) {                      
			error_quit("Retrive error", my_handle);
		}        	
	}
	mysql_free_result(res_ptr);
	return i;
}

//判断数据表是否存在，不存在则创建表
static int MysqlCreateUsrInfoTable(const char* dbTable)
{
	MYSQL *my_con;
	char strMysql[512];
	my_con = GetMySqlHandle()->handle;
	
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strMysql, "create table %s(UsrName varchar(128) not null default '', UsrPasswd varchar(128) not null default '', UsrLevel int not null default 0, UsrId int not null default 0, CountryCode varchar(128) not null default '', CustomName varchar(128) not null default '',PRIMARY KEY (UsrName));", \
		dbTable);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	if (0 == mysql_query(my_con, strMysql))
	{
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_TRUE;
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_FALSE;
}

static int MysqlCreateCameraInfoTable(const char* dbTable)
{
	MYSQL *my_con;
	char strMysql[512];
	my_con = GetMySqlHandle()->handle;
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strMysql, "create table %s(deviceid varchar(128) not null default '',"\
		"devicestatus int not null default 0,"\
		"devicenickname varchar(128) not null default '',"\
		"devicegroupid int not null default 0, "\
		"deviceversion varchar(128) not null default '',"\
		"deviceipaddr varchar(128) not null default '',"\
		"PRIMARY KEY (deviceid));", dbTable);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	if (0 == mysql_query(my_con, strMysql))
	{
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_TRUE;
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_FALSE;
}

//抓拍时间 车牌类型 车牌颜色 车牌号码 
static int MysqlCreateFrontageDataTable(const char* dbTable)
{
	MYSQL *my_con;
	char strMysql[512];
	my_con = GetMySqlHandle()->handle;
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strMysql, "create table %s(id varchar(128) not null default '',"\
		"platecolor int not null default 0,"\
		"vehicletype int not null default 0,"\
		"date datetime NOT NULL,"\
		"PRIMARY KEY (id));", dbTable);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	if (0 == mysql_query(my_con, strMysql))
	{
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_TRUE;
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_FALSE;
}

static int MysqlCreateEntrancedataDataTable(const char* dbTable)
{
	MYSQL *my_con;
	char strMysql[512];
	my_con = GetMySqlHandle()->handle;
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strMysql, "create table %s(id varchar(128) not null default '',"\
 		"platecolor int not null default 0," \
		"plateNo varchar(128) not null default '',"\
		"area int not null default 0,"\
		"vehicletype int not null default 0," \
		"vehiclecolor int not null default 0," \
		"startdate datetime NOT NULL,"\
		"enddate datetime NOT NULL,"\
		"plateImagePath varchar(128) not null default '',"\
		"PRIMARY KEY (id));", dbTable);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	if (0 == mysql_query(my_con, strMysql))
	{
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_TRUE;
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_FALSE;
}

static int MysqlCreateAlarmDataTable(const char* dbTable)
{
	MYSQL *my_con;
	char strMysql[512];
	my_con = GetMySqlHandle()->handle;
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strMysql, "create table %s(id varchar(128) not null default ''," \
		"alarmtype int not null default 0,"\
		"alarmdate datetime not null,"\
		"alarmlocation varchar(128) not null default '',"\
		"PRIMARY KEY (id));", dbTable);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	if (0 == mysql_query(my_con, strMysql))
	{
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_TRUE;
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_FALSE;
}

static int MysqlCreateVipCarDataTable(const char* dbTable)
{
	MYSQL *my_con;
	char strMysql[512];
	my_con = GetMySqlHandle()->handle;
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strMysql, "create table %s(id varchar(128) not null default ''," \
		"platecolor int not null default 0," \
		"plateNo varchar(128) not null default '', "\
		"area int not null default 0,"\
		"vehicletype int not null default 0," \
		"vehiclecolor int not null default 0," \
		"lasttime datetime not null,"\
		"plateImagePath varchar(128) not null default '',"\
		"frequency int not null default 0,"\
		"PRIMARY KEY (id));", dbTable);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	if (0 == mysql_query(my_con, strMysql))
	{
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_TRUE;
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_FALSE;
}

static int MysqlCreateIslandDataTable(const char* dbTable)
{
	MYSQL *my_con;
	char strMysql[512];
	my_con = GetMySqlHandle()->handle;
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strMysql, "create table %s(id varchar(128) not null default ''," \
		"plateNo varchar(128) not null default '', "\
		"date datetime not null,"\
		"PRIMARY KEY (id));", dbTable);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	if (0 == mysql_query(my_con, strMysql))
	{
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_TRUE;
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_FALSE;
}

static int MysqlCreateVipFuelQuantityDataTable(const char* dbTable)
{
	MYSQL *my_con;
	char strMysql[512];
	my_con = GetMySqlHandle()->handle;
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strMysql, "create table %s(id varchar(128) not null default ''," \
		"platecolor int not null default 0," \
		"plateNo varchar(128) not null default '', "\
		"area int not null default 0,"\
		"vehicletype int not null default 0," \
		"vehiclecolor int not null default 0," \
		"lasttime datetime not null,"\
		"plateImagePath varchar(128) not null default '',"\
		"frequency int not null default 0,"\
		"PRIMARY KEY (id));", dbTable);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	if (0 == mysql_query(my_con, strMysql))
	{
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_TRUE;
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_FALSE;
}

static int MysqlCreateWashCarDataTable(const char* dbTable)
{
	MYSQL *my_con;
	char strMysql[512];
	my_con = GetMySqlHandle()->handle;
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strMysql, "create table %s(id varchar(128) not null default ''," \
		"plateNo varchar(128) not null default '', "\
		"startdate datetime not null,"\
		"enddate datetime not null,"\
		"PRIMARY KEY (id));", dbTable);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	if (0 == mysql_query(my_con, strMysql))
	{
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_TRUE;
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_FALSE;
}

static int MysqlCreateCarTrajectoryDataTable(const char* dbTable)
{
	MYSQL *my_con;
	char strMysql[512];
	my_con = GetMySqlHandle()->handle;
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strMysql, "create table %s(id varchar(128) not null default ''," \
		"plateNo varchar(128) not null default '', "\
		"arrivalstartdate datetime not null,"\
		"arrivalenddate datetime not null,"\
		"washstartdate datetime not null,"\
		"washenddate datetime not null,"\
		"PRIMARY KEY (id));", dbTable);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	if (0 == mysql_query(my_con, strMysql))
	{
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_TRUE;
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_FALSE;
}

//无感考勤
//.....唯一token 员工姓名	员工编号	员工备注	打卡时间
//访客记录
//.....唯一token	 客户姓名 客户编号	客户备注 抓拍时间 频次
//入店客流
//.....唯一token 性别     年龄 入店时间

static int MysqlCreateAttendanceDataTable(const char* dbTable)
{
	MYSQL *my_con;
	char strMysql[512];
	my_con = GetMySqlHandle()->handle;
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strMysql, "create table %s(id varchar(128) not null default ''," \
 		"name varchar(128) not null default '', "\
		"number varchar(128) not null default '', "\
		"phonenum varchar(12) not null default '', "\
		"remarks varchar(128) not null default '', "\
		"time datetime not null,"\
		"plateImagePath varchar(128) not null default '',"\
		"snapImagePath varchar(128) not null default '',"\
		"PRIMARY KEY (id));", dbTable);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	if (0 == mysql_query(my_con, strMysql))
	{
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_TRUE;
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_FALSE;
}

static int MysqlCreateVisitorsDataTable(const char* dbTable)
{
	MYSQL *my_con;
	char strMysql[512];
	my_con = GetMySqlHandle()->handle;
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strMysql, "create table %s(id varchar(128) not null default ''," \
 		"name varchar(128) not null default '', "\
		"number varchar(128) not null default '', "\
		"remarks varchar(128) not null default '', "\
		"time datetime not null,"\
		"gradescore int not null default 0,"\
		"frequency int not null default 0,"\
		"plateImagePath varchar(128) not null default '',"\
		"snapImagePath varchar(128) not null default '',"\
		"PRIMARY KEY (id));", dbTable);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	if (0 == mysql_query(my_con, strMysql))
	{
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_TRUE;
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_FALSE;
}

static int MysqlCreatePassengerFlowDataTable(const char* dbTable)
{
	MYSQL *my_con;
	char strMysql[512];
	my_con = GetMySqlHandle()->handle;
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strMysql, "create table %s(id varchar(128) not null default ''," \
		"enter int not null default 0,"\
		"male int not null default 0,"\
		"female int not null default 0,"\
		"kid int not null default 0,"\
		"young int not null default 0,"\
		"old int not null default 0,"\
		"time datetime not null,"\
		"PRIMARY KEY (id));", dbTable);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	if (0 == mysql_query(my_con, strMysql))
	{
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_TRUE;
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_FALSE;
}

//加满率
static int MysqlCreateFuelQuantityDataTable(const char* dbTable) 
{
	MYSQL *my_con;
	char strMysql[512];
	my_con = GetMySqlHandle()->handle;
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strMysql, "create table %s(stationName varchar(128) not null default ''," \
		"oils int not null default 0,"\
		"LPM float(255,2) default 0,"\
		"number int not null default 0,"\
		"time datetime not null)ENGINE=InnoDB DEFAULT CHARSET=utf8;", dbTable);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	if (0 == mysql_query(my_con, strMysql))
	{
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_TRUE;
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_FALSE;
}

static int MySqlCtrlGetUsrInfo(UsrInfo_T *info)
{
	
}

static int MySqlCtrlModifyUsrBindInfo(UsrInfo_T *info, char *oldusrname)
{
	int iret = -1;
	MYSQL *my_handle;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	if (!info){
		DF_DEBUG("info is null ! ");
		return KEY_FALSE;
	}
	memset(strMysql, 0, sizeof(strMysql));
    sprintf(strMysql, "update %s set UsrName = '%s', CountryCode = '%s' where UsrId = %d;", 
		MYSQL_TABLE_USRINFO, info->registInfo.usrname, info->registInfo.countrycode, info->registInfo.usrID);
	
	//DF_DEBUG("strMysql = [%s]", strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlExecute(my_handle, strMysql);
    if( iret != 0 )
    {
        error_quit("update fail", my_handle);
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_FALSE;
    }
	DF_DEBUG("MySqlCtrlModifyUsrBindInfo success !");
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;	
}

static int MySqlCtrlAddUsrInfo(UsrInfo_T *info)
{
	int iret = -1, num = 0;
	MYSQL *my_handle;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[1024] = {0};
	if (!info || 0 >= strlen(info->registInfo.usrname)){
		DF_DEBUG("info is null ! ");
		return KEY_FALSE;
	}
	
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	memset(strMysql, 0, sizeof(strMysql));
	snprintf(strMysql, 1024, "select UsrId from %s where UsrName='%s' and CountryCode='%s';", MYSQL_TABLE_USRINFO, info->registInfo.usrname, info->registInfo.countrycode);  //先查询用户是否已经存在
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		num = MysqlFindRepeated(my_handle);
		if (num > 0){
			MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
			return KEY_USERREPEAT;
		}
	}
    memset(strMysql, 0, sizeof(strMysql));
    snprintf(strMysql, 1024, "insert into %s(UsrName, UsrPasswd, UsrLevel, UsrId, CountryCode, CustomName) value('%s', '%s', %d, %d, '%s', '%s');", 
		MYSQL_TABLE_USRINFO, info->registInfo.usrname, info->registInfo.passWord, info->registInfo.userLevel, info->registInfo.usrID, info->registInfo.countrycode,info->registInfo.customname);

	iret = MysqlExecute(my_handle, strMysql);
    if( iret != 0 )
    {
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_FALSE;
    }
	
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MySqlCtrlDelUsrInfo(UsrInfo_T *info)
{
	//删除用户主表，需要将对应的字表内容也清除
	//删除用户是否要鉴权
	//删除用户是否需要APP提示
	int iret = 0;
	MYSQL *my_handle;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};

	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	if (KEY_TRUE == iret)
	{
		//再删除主表
		memset(strMysql, 0, sizeof(strMysql));
	    sprintf(strMysql, "delete from %s where UsrName = '%s';", 
			MYSQL_TABLE_USRINFO, info->registInfo.usrname);
		iret = MysqlExecute(my_handle, strMysql);
	    if( iret != 0 )
	    {
	        error_quit("del fail", my_handle);  			
			MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
			return KEY_FALSE;
	    }
		DF_DEBUG("Usr %s del success !",info->registInfo.usrname);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MySqlCtrlAddDevice(DeviceStorageInfo_T *info)
{
	int iret = -1, num = 0;
	MYSQL *my_handle;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	if (0 >= strlen(info->deviceInfo.deviceID)){
		DF_DEBUG("deviceID is err ! ");
		return KEY_FALSE;
	}
    memset(strMysql, 0, sizeof(strMysql));
    sprintf(strMysql, "insert into %s(deviceid,devicestatus,devicenickname) value('%s','%d','%s');", 
		MYSQL_TABLE_CAMERA, info->deviceInfo.deviceID, info->deviceInfo.onlineStatus, info->deviceInfo.deviceName);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlExecute(my_handle, strMysql);
    if( iret != 0)
    {
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_FALSE;
    }
	DF_DEBUG("Mysql Device %s Add success !", info->deviceInfo.deviceID);
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MySqlCtrlDelDevice(char *deviceID)
{
	int iret = 0;
	MYSQL *my_handle;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	//再删除主表
	memset(strMysql, 0, sizeof(strMysql));
    sprintf(strMysql, "delete from %s where deviceid = '%s';", 
		MYSQL_TABLE_CAMERA, deviceID);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlExecute(my_handle, strMysql);
    if(iret != 0 )
    {
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_FALSE;
    }
	DF_DEBUG("Mysql Device %s del success !", deviceID);
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MySqlGetDeviceFormUsr(char *username, char *deviceID)
{
	int i = 0, j = 0;
	int iret = -1;
	char mysqlStr[512] = {0};
	char UsrName[128] = {0};
	MYSQL *my_con;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow; 
	if (0 == strlen(username)){
		return KEY_FALSE;
	}
	
	my_con = GetMySqlHandle()->handle;
	sprintf(mysqlStr, "select CameraId from %s where UsrName='%s';", MYSQL_TABLE_CAMERA, username);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_con, mysqlStr);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_con);  //取出结果集
		if(res_ptr)
		{        
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				snprintf(deviceID, 128, "%s", sqlrow[0]);
			}

			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
			}   
			mysql_free_result(res_ptr);     	
		}
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MySqlCtrlGetUsrInfoListNum(int *num)
{
	int i = 0,j = 0;
	int row = 0;
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int iret = -1;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	*num = 0;
	sprintf(strMysql, "select * from %s;", MYSQL_TABLE_USRINFO);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);  //取出结果集
		if(res_ptr)
		{               
			j = mysql_num_fields(res_ptr);
		//	DF_DEBUG("j = %d",j);
			while((sqlrow = mysql_fetch_row(res_ptr))) //循环8次
			{
				(*num) ++;
			}

			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MySqlCtrlGetUsrInfoList(UsrInfo_T *info)
{
	int i = 0,j = 0;
	int row = 0;
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int iret = -1;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	if (!info)
	{
		DF_DEBUG("info is null ! ");
		return KEY_FALSE;
	}
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strMysql, "select * from %s;", MYSQL_TABLE_USRINFO);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);  //取出结果集
		if(res_ptr)
		{               
			j = mysql_num_fields(res_ptr);
			while((sqlrow = mysql_fetch_row(res_ptr))) //循环8次
			{
				snprintf(info[i/j].registInfo.usrname, DF_MAXLEN_USRNAME+1, "%s", sqlrow[0]);
				snprintf(info[i/j].registInfo.passWord, DF_MAXLEN_PASSWD+1, "%s", sqlrow[1]);
				info[i/j].registInfo.userLevel = atoi(sqlrow[2]);
				info[i/j].registInfo.usrID = atoi(sqlrow[3]); 
				snprintf(info[i/j].registInfo.countrycode, DF_MAXLEN_PASSWD+1, "%s", sqlrow[4]);
				snprintf(info[i/j].registInfo.customname, DF_MAXLEN_PASSWD+1, "%s", sqlrow[5]);
				i = i + j;
			}

			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MySqlCtrlSetUsrInfo(UsrInfo_T *info) //必须带用户名参数
{
	int iret;
	MYSQL *my_handle;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	if (!info || 0 >= strlen(info->registInfo.usrname)){
		DF_DEBUG("info is err ! ");
		return KEY_FALSE;
	}
	DF_DEBUG("%s",info->registInfo.passWord);
	memset(strMysql, 0, sizeof(strMysql));
    sprintf(strMysql, "update %s set UsrPasswd = '%s', CustomName = '%s' where UsrName = '%s';", 
		MYSQL_TABLE_USRINFO, info->registInfo.passWord, info->registInfo.customname, info->registInfo.usrname);
	
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlExecute(my_handle, strMysql);
    if( iret != 0 )
    {
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_FALSE;
    }
	DF_DEBUG("UsrInfo Modify success !");
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;	
}

static int MySqlCtrlGetDeviceInfo(DeviceStorageInfo_T *info)
{
	return KEY_TRUE;
}

static int MySqlCtrlSetDeviceInfo(DeviceStorageInfo_T *info)
{
	return KEY_TRUE;
}

static int MySqlCtrlGetDeviceInfoListNum(int *num)
{
	int i = 0,j = 0;
	int row = 0;
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int iret = -1;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[128] = {0};
	*num = 0;

	sprintf(strMysql, "select * from %s;", MYSQL_TABLE_CAMERA);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);  //取出结果集
		if(res_ptr)
		{               
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr))) //循环8次
			{
				(*num) ++;
			}

			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MySqlCtrlGetDeviceInfoList(DeviceStorageInfo_T *info)
{
	return KEY_TRUE;	
}
//出入口
//.....唯一token 车牌号 车牌颜色 车牌所属地 车辆型号 车辆颜色 入口时间 出口时间 车牌图URL
//临街
//.....唯一token   车牌颜色 车辆型号 抓拍时间
//洗车
//.....唯一token   车牌号 入口时间 出口时间 
//异常告警记录
//.....唯一token 告警类型	告警时间 告警地点
//VIP车辆
//.....唯一token   车牌号 近期加油时间 加油频次


//无感考勤
//.....唯一token 员工姓名	员工编号	员工备注	打卡时间
//访客记录
//.....唯一token	 客户姓名 客户编号	客户备注 抓拍时间 频次
//入店客流
//.....唯一token 性别     年龄 入店时间


static int MySqlCtrlAddFrontageInfo(DeviceStreet_T *info)
{
	int iret = -1, num = 0;
	MYSQL *my_handle;
	char table[128] = {0};
	char date[128] = {0};
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	strcpy(table, (char *)MYSQL_TABLE_FRONTAGE);
	snprintf(date, 128, "%04d-%02d-%02d %02d:%02d:%02d", info->time.year, info->time.month, info->time.day, info->time.hour, info->time.min, info->time.sec);
   	sprintf(strMysql, "insert into %s(id,platecolor,vehicletype,date,planNo) value('%s','%d','%d','%s','%s');", 
		table, info->snapToken, info->plateColor, info->vehicleType, date,info->plateNo);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlExecute(my_handle, strMysql);
    if( iret != 0)
    {
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_FALSE;
    }
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MySqlCtrlGetFrontageListSum()
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strMysql, "select count(1) from %s;", MYSQL_TABLE_FRONTAGE);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{               
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				sum = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MySqlCtrlGetFrontageListSumByTime(TimeYMD_T start, TimeYMD_T end)
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char startDate[128] = {0};
	char endDate[128] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", start.year,start.month,start.day, start.hour, start.min, start.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", end.year,end.month,end.day, end.hour, end.min, end.sec);
	sprintf(strMysql, "select count(1) from %s where date < '%s' and date > '%s';", MYSQL_TABLE_FRONTAGE, endDate, startDate);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	//DF_DEBUG("strMysql: %s", strMysql);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{               
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				sum = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MySqlCtrlGetFrontageListSumBySearchtype(TimeYMD_T start, TimeYMD_T end, Vehicle_Type_E Vehicle_Type,Plate_Color_E Plate_Color)
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char startDate[128] = {0};
	char endDate[128] = {0};
	int strmysqllen = 0;
	memset(strMysql, 0, sizeof(strMysql));
	if(start.year == 0){
		strmysqllen += sprintf(strMysql+strmysqllen, "select count(1) from %s", MYSQL_TABLE_FRONTAGE);
		//-1表示不查
		if(Plate_Color != -1){
			strmysqllen += sprintf(strMysql+strmysqllen, " where platecolor = '%d'",Plate_Color);
		}
		if(Vehicle_Type != -1){
			strmysqllen += sprintf(strMysql+strmysqllen, " where vehicletype = '%d'",Vehicle_Type);
		}
	}else{
		sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", start.year,start.month,start.day,start.hour, start.min, start.sec);
		sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", end.year,end.month,end.day,end.hour, end.min, end.sec);
		strmysqllen += sprintf(strMysql+strmysqllen, "select count(1) from %s where date < '%s' and date > '%s'", MYSQL_TABLE_FRONTAGE, endDate, startDate);
		//-1表示不查
		if(Plate_Color != -1){
			strmysqllen += sprintf(strMysql+strmysqllen, " and platecolor = '%d'",Plate_Color);
		}
		if(Vehicle_Type != -1){
			strmysqllen += sprintf(strMysql+strmysqllen, " and vehicletype = '%d'",Vehicle_Type);
		}

	}
	strmysqllen += sprintf(strMysql+strmysqllen, ";");
	//DF_DEBUG("strMysql = %s",strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{               
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				sum = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MySqlCtrlGetFrontageList(int startPage, int num, DeviceStreet_T *info)
{
	int i = 0, j = 0;
	int iret = -1;
	char mysqlStr[512] = {0};
	MYSQL *my_con;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow; 
	if (NULL == info){
		return KEY_FALSE;
	}
	
	my_con = GetMySqlHandle()->handle;
	sprintf(mysqlStr, "select * from %s order by date desc limit %d,%d;",
		MYSQL_TABLE_FRONTAGE, startPage, num);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_con, mysqlStr);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_con);  //取出结果集
		if(res_ptr)
		{         
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				snprintf(info->snapToken, 128, "%s", sqlrow[0]);
				info->plateColor = atoi(sqlrow[1]);
				info->vehicleType = atoi(sqlrow[2]);
				sscanf(sqlrow[3], "%4d-%2d-%2d %2d:%2d:%2d",
									&info->time.year, &info->time.month, &info->time.day,
									&info->time.hour, &info->time.min, &info->time.sec);
				i = i + j;
			}

			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
			}   
			mysql_free_result(res_ptr);     	
		}
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MySqlCtrlGetFrontageListByTime(int startPage, int num, TimeYMD_T start, TimeYMD_T end, DeviceStreet_T *info)
{
	int i = 0, j = 0;
	int iret = -1;
	char mysqlStr[512] = {0};
	MYSQL *my_con;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow; 
	if (NULL == info){
		return KEY_FALSE;
	}
	
	my_con = GetMySqlHandle()->handle;
	sprintf(mysqlStr, "select * from %s order by date desc limit %d,%d;",
		MYSQL_TABLE_FRONTAGE, startPage, num);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_con, mysqlStr);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_con);  //取出结果集
		if(res_ptr)
		{               
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				snprintf(info->snapToken, 128, "%s", sqlrow[0]);
				info->plateColor = atoi(sqlrow[1]);
				info->vehicleType = atoi(sqlrow[2]);
				sscanf(sqlrow[3], "%4d-%2d-%2d %2d:%2d:%2d",
									&info->time.year, &info->time.month, &info->time.day,
									&info->time.hour, &info->time.min, &info->time.sec);
				i = i + j;
			}

			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
			}   
			mysql_free_result(res_ptr);     	
		}
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MysqlCtrlUpdateFrontageInfo(char *token, DeviceStreet_T *info)
{
	int iret = -1;
	MYSQL *my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char enddate[128] = {0};
	if (!info){
		DF_DEBUG("info is null ! ");
		return KEY_FALSE;
	}
	memset(strMysql, 0, sizeof(strMysql));
	snprintf(enddate, 128, "%04d-%02d-%02d %02d:%02d:%02d", info->time.year, info->time.month, info->time.day, info->time.hour, info->time.min, info->time.sec);
    sprintf(strMysql, "update %s set date = '%s' where id = '%s';", 
		MYSQL_TABLE_FRONTAGE, enddate, token);
	
	//DF_DEBUG("strMysql = [%s]", strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlExecute(my_handle, strMysql);
    if( iret != 0 )
    {
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_FALSE;
    }
	//DF_DEBUG("MysqlCtrlUpdateEntranceInfo success !");
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;		
}

static int MySqlCtrlGetLatestFrontageInfo(char *plateNo, DeviceStreet_T *info)
{
	//select * FROM test where date = (SELECT max(test.date) from test where id = "A1001");
	int i = 0, j = 0;
	int iret = -1;
	char mysqlStr[512] = {0};
	char UsrName[128] = {0};
	MYSQL *my_con;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow; 
	if (0 == strlen(plateNo)){
		return KEY_FALSE;
	}
	
	my_con = GetMySqlHandle()->handle;
	sprintf(mysqlStr, "select * FROM %s where date = (SELECT max(%s.date) from %s where planNo = '%s')",
		MYSQL_TABLE_FRONTAGE, MYSQL_TABLE_FRONTAGE, MYSQL_TABLE_FRONTAGE, plateNo);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_con, mysqlStr);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_con);  //取出结果集
		if(res_ptr)
		{      
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				snprintf(info->snapToken, 128, "%s", sqlrow[0]);
				info->plateColor = atoi(sqlrow[1]);
				info->vehicleType = atoi(sqlrow[2]);
				if (0 < strlen(sqlrow[3]))
				{
					sscanf(sqlrow[3], "%4d-%2d-%2d %2d:%2d:%2d",
								&info->time.year, &info->time.month, &info->time.day,
								&info->time.hour, &info->time.min, &info->time.sec);
				}
				snprintf(info->plateNo, 128, "%s", sqlrow[4]);
				i = i + j;
			}

			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
			}   
			mysql_free_result(res_ptr);     	
		}
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}


static int MySqlCtrlAddEntranceInfo(DeviceEntrance_T *info)
{
	int iret = -1, num = 0;
	MYSQL *my_handle;
	char table[128] = {0};
	char startdate[128] = {0};
	char enddate[128] = {0};
	my_handle = GetMySqlHandle()->handle;
	char strMysql[1024] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	strcpy(table, (char *)MYSQL_TABLE_ENTRANCE);
	snprintf(startdate, 128, "%04d-%02d-%02d %02d:%02d:%02d", info->vehicleStart.year, info->vehicleStart.month, info->vehicleStart.day, info->vehicleStart.hour, info->vehicleStart.min, info->vehicleStart.sec);
	snprintf(enddate, 128, "%04d-%02d-%02d %02d:%02d:%02d", info->vehicleEnd.year, info->vehicleEnd.month, info->vehicleEnd.day, info->vehicleEnd.hour, info->vehicleEnd.min, info->vehicleEnd.sec);
	snprintf(strMysql,1024, "insert into %s(id,plateNo,area,platecolor,vehicletype,vehiclecolor,startdate,enddate,plateImagePath) value('%s','%s','%d','%d','%d','%d','%s','%s','%s');", 
		table, info->snapToken, info->plateNo, info->vehicleTerr, info->plateColor, info->vehicleType,\
		info->vehicleColor, startdate, enddate, info->vehicleImagePath);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlExecute(my_handle, strMysql);
    if( iret != 0)
    {
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_FALSE;
    }
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MySqlCtrlGetEntranceListSum()
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strMysql, "select count(1) from %s;", MYSQL_TABLE_ENTRANCE);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{               
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				sum = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MySqlCtrlGetEntranceListSumByLocation(TimeYMD_T start, TimeYMD_T end,char * location)
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char startDate[128] = {0};
	char endDate[128] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	//select count(1) from entrancedata where area in (select id from platnoareaninfo where province_name = '四川省');
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", start.year, start.month,start.day, start.hour, start.min, start.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", end.year,end.month,end.day,end.hour, end.min, end.sec);
	sprintf(strMysql, "select count(1) from %s where startdate < '%s' and startdate > '%s' and area in (select id from platnoareaninfo where province_name = '%s');", MYSQL_TABLE_ENTRANCE, endDate, startDate,location);
	//DF_DEBUG("strMysql = %s",strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{               
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				sum = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MySqlCtrlGetEntranceResidenceByTime(TimeYMD_T start, TimeYMD_T end, ResidenceType_T type)
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int iret = -1, num = 0;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char startDate[128] = {0};
	char endDate[128] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", start.year,start.month,start.day, start.hour, start.min, start.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", end.year,end.month,end.day,end.hour, end.min, end.sec);
	if (RESIDENCE_MIN1 == type)
	{
		sprintf(strMysql, "select count(1) from %s where (startdate < '%s' and startdate > '%s') and (UNIX_TIMESTAMP(enddate) - UNIX_TIMESTAMP(startdate) <= 5*60);", MYSQL_TABLE_ENTRANCE, endDate, startDate);
	}
	else if (RESIDENCE_MIN2 == type)
	{
		sprintf(strMysql, "select count(1) from %s where (startdate < '%s' and startdate > '%s') and (UNIX_TIMESTAMP(enddate) - UNIX_TIMESTAMP(startdate) > 5*60) and (UNIX_TIMESTAMP(enddate) - UNIX_TIMESTAMP(startdate) < 20*60);", MYSQL_TABLE_ENTRANCE, endDate, startDate);
	}
	else if ((RESIDENCE_MIN3 == type))
	{
		sprintf(strMysql, "select count(1) from %s where (startdate < '%s' and startdate > '%s') and (UNIX_TIMESTAMP(enddate) - UNIX_TIMESTAMP(startdate) >= 20*60);", MYSQL_TABLE_ENTRANCE, endDate, startDate);
	}
	//DF_DEBUG("strMysql = %s",strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{               
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				num = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}

	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return num;
}

static int MySqlCtrlGetEntranceRefuelingEfficiency(TimeYMD_T start, TimeYMD_T end, Refueling_T *refueling)
{
	if(NULL == refueling)
	{
		return KEY_FALSE;
	}
	
	MYSQL *my_handle;
	MYSQL_RES *res_ptr;  
	MYSQL_ROW sqlrow;
	int iret = -1, num = 0;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char startDate[128] = {0};
	char endDate[128] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", start.year,start.month,start.day, start.hour, start.min, start.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", end.year,end.month,end.day,end.hour, end.min, end.sec);
	sprintf(strMysql, "select count(1), FLOOR(avg((UNIX_TIMESTAMP(enddate) - UNIX_TIMESTAMP(startdate)))) sec from %s where startdate > '%s' and startdate < '%s';", MYSQL_TABLE_ENTRANCE, startDate, endDate);
	//DF_DEBUG("strMysql = %s",strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{		
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				refueling->total = atoi(sqlrow[0]);
				if(NULL == sqlrow[1]){
					refueling->average = 0;
				}else{
					refueling->average = atoi(sqlrow[1]);
				}
			}
			if (mysql_errno(my_handle)) {					   
				error_quit("Retrive error", my_handle);
			}			
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return iret;
}

static int MySqlCtrlGetEntranceResidenceListInfo(int startPage, int num, TimeYMD_T start, TimeYMD_T end, DeviceEntranceMysql_T *info)
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int iret = -1, count = 0, i = 0, j = 0;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char startDate[128] = {0};
	char endDate[128] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", start.year,start.month,start.day, start.hour, start.min, start.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", end.year,end.month,end.day,end.hour, end.min, end.sec);
	sprintf(strMysql, "select plateNo,area,vehicletype,startdate,UNIX_TIMESTAMP(enddate) - UNIX_TIMESTAMP(startdate) from %s where (UNIX_TIMESTAMP(enddate) - UNIX_TIMESTAMP(startdate))>20*60  and startdate<'%s' and startdate >'%s' limit %d, %d;", 
		MYSQL_TABLE_ENTRANCE, endDate, startDate, (startPage - 1)*num, num);
	//DF_DEBUG("%s", strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);  //取出结果集
		if(res_ptr)
		{               
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				snprintf(info[i/j].plateNo, 128, "%s", sqlrow[0]);
				info[i/j].vehicleTerr = atoi(sqlrow[1]);
				info[i/j].vehicleType = atoi(sqlrow[2]);
				sscanf(sqlrow[3], "%4d-%2d-%2d %2d:%2d:%2d",
									&info[i/j].vehicleStart.year, &info[i/j].vehicleStart.month, &info[i/j].vehicleStart.day,
									&info[i/j].vehicleStart.hour, &info[i/j].vehicleStart.min, &info[i/j].vehicleStart.sec);
				info[i/j].staytime = atoi(sqlrow[4]);
				i = i + j;
				count ++;
			}

			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}   
			mysql_free_result(res_ptr);     	
		}
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return count;
}

static int MySqlCtrlGetEntranceListSumByTime(TimeYMD_T start, TimeYMD_T end)
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char startDate[128] = {0};
	char endDate[128] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", start.year,start.month,start.day, start.hour, start.min, start.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", end.year,end.month,end.day,end.hour, end.min, end.sec);
	sprintf(strMysql, "select count(1) from %s where startdate < '%s' and startdate > '%s';", MYSQL_TABLE_ENTRANCE, endDate, startDate);
	//DF_DEBUG("strMysql = %s",strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{               
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				sum = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MySqlCtrlGetEntranceListSumBySerchType(TimeYMD_T start, TimeYMD_T end, VehicleSearchInfo_T searchTypeInfo)
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char startDate[128] = {0};
	char endDate[128] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", start.year,start.month,start.day, start.hour,start.min,start.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", end.year,end.month,end.day, end.hour,end.min,end.sec);
	switch (searchTypeInfo.type)
	{
		case VehicleSearch_TYPE_Terr:
		{
			sprintf(strMysql, "select count(1) from %s where startdate < '%s' and startdate > '%s' and area = %d;", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.vehicleTerr);			
			break;
		}
		case VehicleSearch_TYPE_Vehicle:
		{
			sprintf(strMysql, "select count(1) from %s where startdate < '%s' and startdate > '%s' and vehicletype = %d;", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.vehicleType);			
			break;					
		}
		case VehicleSearch_TYPE_Fuzzy:
		{
			sprintf(strMysql, "select count(1) from %s where startdate < '%s' and startdate > '%s' and concat(plateNo) like '%%%s%%';", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.fuzzyStr);			
			break;					
		}
		case VehicleSearch_TYPE_Terr | VehicleSearch_TYPE_Vehicle:
		{
			sprintf(strMysql, "select count(1) from %s where startdate < '%s' and startdate > '%s' and area = %d and vehicletype = %d;", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.vehicleType);			
			break;
		}
		case VehicleSearch_TYPE_Terr | VehicleSearch_TYPE_Fuzzy:
		{
			sprintf(strMysql, "select count(1) from %s where startdate < '%s' and startdate > '%s' and area = %d and concat(plateNo) like '%%%s%%';", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.fuzzyStr);			
			break;
		}
		case VehicleSearch_TYPE_Terr | VehicleSearch_TYPE_Vehicle | VehicleSearch_TYPE_Fuzzy:
		{
			sprintf(strMysql, "select count(1) from %s where startdate < '%s' and startdate > '%s' and area = %d and concat(plateNo) like '%%%s%%' and vehicletype = %d;", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.fuzzyStr, searchTypeInfo.vehicleType);			
			break;
		}
		case VehicleSearch_TYPE_Vehicle | VehicleSearch_TYPE_Fuzzy:
		{
			sprintf(strMysql, "select count(1) from %s where startdate < '%s' and startdate > '%s' and area = %d and vehicletype = %d;", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.vehicleType);			
			break;
		}
		default:
		{
			sprintf(strMysql, "select count(1) from %s where startdate < '%s' and startdate > '%s';", MYSQL_TABLE_ENTRANCE, endDate, startDate);	
			break;
		}
	}
	//DF_DEBUG("strMysql = %s",strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{               
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				sum = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MySqlCtrlGetEntranceListSumBySerchType2(TimeYMD_T start, TimeYMD_T end, VehicleSearchInfo_T searchTypeInfo)
{
	MYSQL *my_handle;
	MYSQL_RES *res_ptr;  
	MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[1024] = {0};
	char startDate[128] = {0};
	char endDate[128] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", start.year,start.month,start.day, start.hour,start.min,start.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", end.year,end.month,end.day, end.hour,end.min,end.sec);
	switch (searchTypeInfo.type)
	{
		case VehicleSearch_TYPE_Terr:
		{
			sprintf(strMysql, "select count(1) from(SELECT plateNo, COUNT(plateNo) AS count,any_value (max(startdate)),any_value (area),any_value (platecolor),any_value (vehicletype),any_value (plateImagePath),any_value (id) FROM %s WHERE startdate < '%s' and startdate > '%s' and area = %d GROUP BY plateNo) as result;", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.vehicleTerr);   
			break;
		}
		case VehicleSearch_TYPE_Vehicle:
		{
			sprintf(strMysql, "select count(1) from(select plateNo, count(plateNo) AS count,any_value (max(startdate)),any_value (area),any_value (platecolor),any_value (vehicletype),any_value (plateImagePath),any_value (id) FROM %s WHERE startdate < '%s' and startdate > '%s' and vehicletype = %d GROUP BY plateNo) as result;", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.vehicleType);   
			break;     
		}
		case VehicleSearch_TYPE_Fuzzy:
		{
			sprintf(strMysql, "select count(1) from(select plateNo, count(plateNo) AS count,any_value (max(startdate)),any_value (area),any_value (platecolor),any_value (vehicletype),any_value (plateImagePath),any_value (id) FROM %s WHERE startdate < '%s' and startdate > '%s' and concat(plateNo) like '%%%s%%' GROUP BY plateNo) as result;", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.fuzzyStr);   
			break;     
		}
		case VehicleSearch_TYPE_Terr | VehicleSearch_TYPE_Vehicle:
		{
			sprintf(strMysql, "select count(1) from(select plateNo, count(plateNo) AS count,any_value (max(startdate)),any_value (area),any_value (platecolor),any_value (vehicletype),any_value (plateImagePath),any_value (id) FROM %s WHERE startdate < '%s' and startdate > '%s' and area = %d and vehicletype = %d GROUP BY plateNo) as result;", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.vehicleType);   
			break;
		}
		case VehicleSearch_TYPE_Terr | VehicleSearch_TYPE_Fuzzy:
		{
			sprintf(strMysql, "select count(1) from(select plateNo, count(plateNo) AS count,any_value (max(startdate)),any_value (area),any_value (platecolor),any_value (vehicletype),any_value (plateImagePath),any_value (id) FROM %s WHERE startdate < '%s' and startdate > '%s' and area = %d and concat(plateNo) like '%%%s%%' GROUP BY plateNo) as result;", MYSQL_TABLE_ENTRANCE,  endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.fuzzyStr);   
			break;
		}
		case VehicleSearch_TYPE_Terr | VehicleSearch_TYPE_Vehicle | VehicleSearch_TYPE_Fuzzy:
		{
			sprintf(strMysql, "select count(1) from(select count(1) from(select plateNo, count(plateNo) AS count,any_value (max(startdate)),any_value (area),any_value (platecolor),any_value (vehicletype),any_value (plateImagePath),any_value (id) FROM %s WHERE startdate < '%s' and startdate > '%s' and area = %d and concat(plateNo) like '%%%s%%' and vehicletype = %d GROUP BY plateNo) as result;",  MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.fuzzyStr, searchTypeInfo.vehicleType);   
			break;
		}
		case VehicleSearch_TYPE_Vehicle | VehicleSearch_TYPE_Fuzzy:
		{
			sprintf(strMysql, "select count(1) from(select plateNo, count(plateNo) AS count,any_value (max(startdate)),any_value (area),any_value (platecolor),any_value (vehicletype),any_value (plateImagePath),any_value (id) FROM %s WHERE startdate < '%s' and startdate > '%s' and area = %d and vehicletype = %d GROUP BY plateNo) as result;", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.vehicleType);   
			break;
		}
		case VehicleSearch_TYPE_Increment:  //新增量
		{
			memset(startDate, 0, sizeof(startDate));
			memset(endDate, 0, sizeof(endDate));
			
			if(start.month == 1)
			{
				start.year -= 1;
				start.month= 12;
			}
			else
			{
				start.month -= 1;
			}
			sprintf(startDate, "%04d-%02d", start.year,start.month);
			sprintf(endDate, "%04d-%02d", end.year,end.month);
			sprintf(strMysql, "select count(1) from(select COUNT(plateNo) AS count, plateNo, any_value(max(startdate)), any_value (area), any_value(platecolor), any_value(vehicletype), any_value(plateImagePath),any_value (id) FROM %s tb1 WHERE DATE_FORMAT(startdate, '%%Y-%%m') = '%s' AND not EXISTS(SELECT tbb.* FROM %s tbb WHERE tb1.plateNo = tbb.plateNo AND tb1.startdate > tbb.startdate AND DATE_FORMAT(tbb.startdate, '%%Y-%%m') = '%s') GROUP BY plateNo) as result;", MYSQL_TABLE_ENTRANCE, endDate, MYSQL_TABLE_ENTRANCE, startDate);
			break;
		}
		case VehicleSearch_TYPE_Increment | VehicleSearch_TYPE_Vehicle:  //新增量 + 车辆类型
		{
			memset(startDate, 0, sizeof(startDate));
			memset(endDate, 0, sizeof(endDate));
			
			if(start.month == 1)
			{
				start.year -= 1;
				start.month= 12;
			}
			else
			{
				start.month -= 1;
			}
			sprintf(startDate, "%04d-%02d", start.year,start.month);
			sprintf(endDate, "%04d-%02d", end.year,end.month);
			sprintf(strMysql,"select count(1) from(select COUNT(plateNo) AS count, plateNo, any_value(max(startdate)), any_value (area), any_value (platecolor), any_value (vehicletype), any_value (plateImagePath),any_value (id) FROM %s tb1 WHERE DATE_FORMAT(startdate, '%%Y-%%m') = '%s' AND not EXISTS(SELECT tbb.* FROM %s tbb WHERE tb1.plateNo = tbb.plateNo AND tb1.startdate > tbb.startdate AND DATE_FORMAT(tbb.startdate, '%%Y-%%m') = '%s') and vehicletype = %d GROUP BY plateNo) as result;", MYSQL_TABLE_ENTRANCE, endDate, MYSQL_TABLE_ENTRANCE, startDate, searchTypeInfo.vehicleType);
			break;
		}
		case VehicleSearch_TYPE_Increment | VehicleSearch_TYPE_Fuzzy:  //新增量 + 关键字
		{
			memset(startDate, 0, sizeof(startDate));
			memset(endDate, 0, sizeof(endDate));
			
			if(start.month == 1)
			{
				start.year -= 1;
				start.month= 12;
			}
			else
			{
				start.month -= 1;
			}
			sprintf(startDate, "%04d-%02d", start.year,start.month);
			sprintf(endDate, "%04d-%02d", end.year,end.month);
			sprintf(strMysql,"select count(1) from(select COUNT(plateNo) AS count, plateNo, any_value(max(startdate)), any_value (area), any_value (platecolor), any_value (vehicletype), any_value (plateImagePath),any_value (id) FROM %s tb1 WHERE DATE_FORMAT(startdate, '%%Y-%%m') = '%s' AND not EXISTS(SELECT tbb.* FROM %s tbb WHERE tb1.plateNo = tbb.plateNo AND tb1.startdate > tbb.startdate AND DATE_FORMAT(tbb.startdate, '%%Y-%%m') = '%s') and concat(plateNo) like '%%%s%%' GROUP BY plateNo) as result;", MYSQL_TABLE_ENTRANCE, endDate, MYSQL_TABLE_ENTRANCE, startDate, searchTypeInfo.fuzzyStr);
			break;
		}
		case VehicleSearch_TYPE_Increment | VehicleSearch_TYPE_Fuzzy | VehicleSearch_TYPE_Vehicle:  //新增量 + 关键字 + 车辆类型
		{
			memset(startDate, 0, sizeof(startDate));
			memset(endDate, 0, sizeof(endDate));
			
			if(start.month == 1)
			{
				start.year -= 1;
				start.month= 12;
			}
			else
			{
				start.month -= 1;
			}
			sprintf(startDate, "%04d-%02d", start.year,start.month);
			sprintf(endDate, "%04d-%02d", end.year,end.month);
			sprintf(strMysql,"select count(1) from(select COUNT(plateNo) AS count, plateNo, any_value(max(startdate)), any_value (area), any_value (platecolor), any_value (vehicletype), any_value (plateImagePath),any_value (id) FROM %s tb1 WHERE DATE_FORMAT(startdate, '%%Y-%%m') = '%s' AND not EXISTS(SELECT tbb.* FROM %s tbb WHERE tb1.plateNo = tbb.plateNo AND tb1.startdate > tbb.startdate AND DATE_FORMAT(tbb.startdate, '%%Y-%%m') = '%s') and concat(plateNo) like '%%%s%%' and vehicletype = %d GROUP BY plateNo) as result;", MYSQL_TABLE_ENTRANCE, endDate, MYSQL_TABLE_ENTRANCE, startDate, searchTypeInfo.fuzzyStr, searchTypeInfo.vehicleType);
			break;
		}
		case VehicleSearch_TYPE_Losses:  //流失量
		{
			memset(startDate, 0, sizeof(startDate));
			memset(endDate, 0, sizeof(endDate));
			
			if(start.month == 1)
			{
				start.year -= 1;
				start.month= 12;
			}
			else
			{
				start.month -= 1;
			}
			sprintf(startDate, "%04d-%02d", start.year,start.month);
			sprintf(endDate, "%04d-%02d", end.year,end.month);
			sprintf(strMysql,"select count(1) from(SELECT COUNT(plateNo) AS count, plateNo, any_value(max(startdate)), any_value (area), any_value (platecolor), any_value (vehicletype), any_value (plateImagePath),any_value (id) FROM %s tb1 WHERE DATE_FORMAT(startdate, '%%Y-%%m') = '%s' AND not EXISTS(SELECT tbb.* FROM %s tbb WHERE tb1.plateNo = tbb.plateNo AND tb1.startdate > tbb.startdate AND DATE_FORMAT(tbb.startdate, '%Y-%m') = '%s') GROUP BY plateNo) as result;", MYSQL_TABLE_ENTRANCE, startDate, MYSQL_TABLE_ENTRANCE, endDate);
			break;
		}
		case VehicleSearch_TYPE_Losses | VehicleSearch_TYPE_Vehicle:  //流失量   + 车辆类型
		{
			memset(startDate, 0, sizeof(startDate));
			memset(endDate, 0, sizeof(endDate));
			
			if(start.month == 1)
			{
				start.year -= 1;
				start.month= 12;
			}
			else
			{
				start.month -= 1;
			}
			sprintf(startDate, "%04d-%02d", start.year,start.month);
			sprintf(endDate, "%04d-%02d", end.year,end.month);
			sprintf(strMysql,"select count(1) from(SELECT COUNT(plateNo) AS count, plateNo, any_value(max(startdate)), any_value (area), any_value (platecolor), any_value (vehicletype), any_value (plateImagePath),any_value (id) FROM %s tb1 WHERE DATE_FORMAT(startdate, '%%Y-%%m') = '%s' AND not EXISTS(SELECT tbb.* FROM %s tbb WHERE tb1.plateNo = tbb.plateNo AND tb1.startdate > tbb.startdate AND DATE_FORMAT(tbb.startdate, '%Y-%m') = '%s') and vehicletype = %d GROUP BY plateNo) as result;", MYSQL_TABLE_ENTRANCE, startDate, MYSQL_TABLE_ENTRANCE, endDate, searchTypeInfo.vehicleType);
			break;
		}
		case VehicleSearch_TYPE_Losses | VehicleSearch_TYPE_Fuzzy:  //流失量    + 关键字
		{
			memset(startDate, 0, sizeof(startDate));
			memset(endDate, 0, sizeof(endDate));
			
			if(start.month == 1)
			{
				start.year -= 1;
				start.month= 12;
			}
			else
			{
				start.month -= 1;
			}
			sprintf(startDate, "%04d-%02d", start.year,start.month);
			sprintf(endDate, "%04d-%02d", end.year,end.month);
			sprintf(strMysql,"select count(1) from(SELECT COUNT(plateNo) AS count, plateNo, any_value(max(startdate)), any_value (area), any_value (platecolor), any_value (vehicletype), any_value (plateImagePath),any_value (id) FROM %s tb1 WHERE DATE_FORMAT(startdate, '%%Y-%%m') = '%s' AND not EXISTS(SELECT tbb.* FROM %s tbb WHERE tb1.plateNo = tbb.plateNo AND tb1.startdate > tbb.startdate AND DATE_FORMAT(tbb.startdate, '%Y-%m') = '%s') and concat(plateNo) like '%%%s%%' GROUP BY plateNo) as result;", MYSQL_TABLE_ENTRANCE, startDate, MYSQL_TABLE_ENTRANCE, endDate, searchTypeInfo.fuzzyStr);
			break;
		}
		case VehicleSearch_TYPE_Losses | VehicleSearch_TYPE_Vehicle | VehicleSearch_TYPE_Fuzzy:  //流失量    + 车辆类型     + 关键字
		{
			memset(startDate, 0, sizeof(startDate));
			memset(endDate, 0, sizeof(endDate));
			
			if(start.month == 1)
			{
				start.year -= 1;
				start.month= 12;
			}
			else
			{
				start.month -= 1;
			}
			sprintf(startDate, "%04d-%02d", start.year,start.month);
			sprintf(endDate, "%04d-%02d", end.year,end.month);
			sprintf(strMysql,"select count(1) from(SELECT COUNT(plateNo) AS count, plateNo, any_value(max(startdate)), any_value (area), any_value (platecolor), any_value (vehicletype), any_value (plateImagePath),any_value (id) FROM %s tb1 WHERE DATE_FORMAT(startdate, '%%Y-%%m') = '%s' AND not EXISTS(SELECT tbb.* FROM %s tbb WHERE tb1.plateNo = tbb.plateNo AND tb1.startdate > tbb.startdate AND DATE_FORMAT(tbb.startdate, '%Y-%m') = '%s') nd concat(plateNo) like '%%%s%%' and vehicletype = %d GROUP BY plateNo) as result;", MYSQL_TABLE_ENTRANCE, startDate, MYSQL_TABLE_ENTRANCE, endDate, searchTypeInfo.fuzzyStr, searchTypeInfo.vehicleType);
			break;
		}
		default:
		{
			sprintf(strMysql, "select count(1) from(SELECT COUNT(plateNo) AS count, plateNo, any_value (max(startdate)),any_value (area),any_value (platecolor),any_value (vehicletype),any_value (plateImagePath),any_value (id) FROM %s WHERE startdate < '%s' and startdate > '%s' GROUP BY plateNo) as result;", MYSQL_TABLE_ENTRANCE, endDate, startDate);   
			break;
		}
	}
	//DF_DEBUG("strMysql = %s",strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{               
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
			sum = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
			error_quit("Retrive error", my_handle);
			}         
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MySqlCtrlGetEntranceList(int startPage, int num, DeviceEntrance_T *info)
{
	int i = 0, j = 0;
	int iret = -1;
	char mysqlStr[512] = {0};
	MYSQL *my_con;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow; 
	if (NULL == info){
		return KEY_FALSE;
	}
	
	my_con = GetMySqlHandle()->handle;
	sprintf(mysqlStr, "select * from %s order by startdate desc limit %d,%d;",
		MYSQL_TABLE_ENTRANCE, startPage, num);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_con, mysqlStr);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_con);  //取出结果集
		if(res_ptr)
		{    
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				snprintf(info->snapToken, 128, "%s", sqlrow[0]);
				info->plateColor = atoi(sqlrow[1]);
				snprintf(info->plateNo, 128, "%s", sqlrow[2]);
				info->vehicleTerr = atoi(sqlrow[3]);
				info->vehicleType = atoi(sqlrow[4]);
				info->vehicleColor = atoi(sqlrow[5]);
				if (0 < strlen(sqlrow[6]))
				{
					sscanf(sqlrow[6], "%4d-%2d-%2d %2d:%2d:%2d",
								&info->vehicleStart.year, &info->vehicleStart.month, &info->vehicleStart.day,
								&info->vehicleStart.hour, &info->vehicleStart.min, &info->vehicleStart.sec);
				}
				if (0 < strlen(sqlrow[7]))
				{
					sscanf(sqlrow[7], "%4d-%2d-%2d %2d:%2d:%2d",
								&info->vehicleEnd.year, &info->vehicleEnd.month, &info->vehicleEnd.day,
								&info->vehicleEnd.hour, &info->vehicleEnd.min, &info->vehicleEnd.sec);
				}
				snprintf(info->vehicleImagePath, 128, "%s", sqlrow[8]);
				i = i + j;
			}

			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
			}   
			mysql_free_result(res_ptr);     	
		}
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MySqlCtrlGetEntranceListByTime(int startPage, int num, TimeYMD_T start, TimeYMD_T end, DeviceEntrance_T *info)
{
	int i = 0, j = 0;
	int iret = -1;
	char mysqlStr[512] = {0};
	char startDate[128] = {0};
	char endDate[128] = {0};
	MYSQL *my_con;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow; 
	if (NULL == info){
		return KEY_FALSE;
	}
	my_con = GetMySqlHandle()->handle;
	sprintf(startDate, "%d-%d-%d", start.year,start.month,start.day);
	sprintf(endDate, "%d-%d-%d", end.year,end.month,end.day);
	sprintf(mysqlStr, "select * from %s where startdate < '%s' and startdate > '%s' order by startdate desc limit %d,%d;",
		MYSQL_TABLE_ENTRANCE, startDate, endDate, startPage, num);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_con, mysqlStr);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_con);  //取出结果集
		if(res_ptr)
		{               
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				snprintf(info->snapToken, 128, "%s", sqlrow[0]);
				info->plateColor = atoi(sqlrow[1]);
				snprintf(info->plateNo, 128, "%s", sqlrow[2]);
				info->vehicleTerr = atoi(sqlrow[3]);
				info->vehicleType = atoi(sqlrow[4]);
				info->vehicleColor = atoi(sqlrow[5]);
				if (0 < strlen(sqlrow[6]))
				{
					sscanf(sqlrow[6], "%4d-%2d-%2d %2d:%2d:%2d",
								&info->vehicleStart.year, &info->vehicleStart.month, &info->vehicleStart.day,
								&info->vehicleStart.hour, &info->vehicleStart.min, &info->vehicleStart.sec);
				}
				if (0 < strlen(sqlrow[7]))
				{
					sscanf(sqlrow[7], "%4d-%2d-%2d %2d:%2d:%2d",
								&info->vehicleEnd.year, &info->vehicleEnd.month, &info->vehicleEnd.day,
								&info->vehicleEnd.hour, &info->vehicleEnd.min, &info->vehicleEnd.sec);
				}
				snprintf(info->vehicleImagePath, 128, "%s", sqlrow[8]);
				i = i + j;
			}

			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
			}   
			mysql_free_result(res_ptr);     	
		}
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MySqlCtrlGetEntranceListBySerchType(int startPage, int num, TimeYMD_T start, TimeYMD_T end, VehicleSearchInfo_T searchTypeInfo, DeviceEntrance_T *info)
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	int i = 0, j = 0;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char startDate[128] = {0};
	char endDate[128] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", start.year,start.month,start.day, start.hour, start.min, start.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", end.year,end.month,end.day, end.hour, end.min, end.sec);
	switch (searchTypeInfo.type)
	{
		case VehicleSearch_TYPE_Terr:
		{
			sprintf(strMysql, "select * from %s where startdate < '%s' and startdate > '%s' and area = %d order by startdate desc limit %d,%d;", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.vehicleTerr, (startPage-1)*num, num);			
			break;
		}
		case VehicleSearch_TYPE_Vehicle:
		{
			sprintf(strMysql, "select * from %s where startdate < '%s' and startdate > '%s' and vehicletype = %d order by startdate desc limit %d,%d;", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.vehicleType, (startPage-1)*num, num);			
			break;					
		}
		case VehicleSearch_TYPE_Fuzzy:
		{
			sprintf(strMysql, "select * from %s where startdate < '%s' and startdate > '%s' and concat(plateNo) like '%%%s%%' order by startdate desc limit %d,%d;", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.fuzzyStr, (startPage-1)*num, num);			
			break;					
		}
		case VehicleSearch_TYPE_Terr | VehicleSearch_TYPE_Vehicle:
		{
			sprintf(strMysql, "select * from %s where startdate < '%s' and startdate > '%s' and area = %d and vehicletype = %d order by startdate desc limit %d,%d;", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.vehicleType, (startPage-1)*num, num);			
			break;
		}
		case VehicleSearch_TYPE_Terr | VehicleSearch_TYPE_Fuzzy:
		{
			sprintf(strMysql, "select * from %s where startdate < '%s' and startdate > '%s' and area = %d and concat(plateNo) like '%%%s%%' order by startdate desc limit %d,%d;", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.fuzzyStr, (startPage-1)*num, num);			
			break;
		}
		case VehicleSearch_TYPE_Terr | VehicleSearch_TYPE_Vehicle | VehicleSearch_TYPE_Fuzzy:
		{
			sprintf(strMysql, "select * from %s where startdate < '%s' and startdate > '%s' and area = %d and concat(plateNo) like '%%%s%%' and vehicletype = %d order by startdate desc limit %d,%d;", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.fuzzyStr, searchTypeInfo.vehicleType, (startPage-1)*num, num);			
			break;
		}
		case VehicleSearch_TYPE_Vehicle | VehicleSearch_TYPE_Fuzzy:
		{
			sprintf(strMysql, "select * from %s where startdate < '%s' and startdate > '%s' and area = %d and vehicletype = %d order by startdate desc limit %d,%d;", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.vehicleType, (startPage-1)*num, num);			
			break;
		}
		default:
		{
			sprintf(strMysql, "select * from %s where startdate < '%s' and startdate > '%s' order by startdate desc limit %d,%d;", MYSQL_TABLE_ENTRANCE, endDate, startDate, (startPage-1)*num, num);	
			break;
		}
	}
	DF_DEBUG("strMysql = %s",strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{         
			j = mysql_num_fields(res_ptr); 
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				snprintf(info[i/j].snapToken, 128, "%s", sqlrow[0]);
				info[i/j].plateColor = atoi(sqlrow[1]);
				snprintf(info[i/j].plateNo, 128, "%s", sqlrow[2]);
				info[i/j].vehicleTerr = atoi(sqlrow[3]);
				info[i/j].vehicleType = atoi(sqlrow[4]);
				info[i/j].vehicleColor = atoi(sqlrow[5]);
				if (0 < strlen(sqlrow[6]))
				{
					sscanf(sqlrow[6], "%4d-%2d-%2d %2d:%2d:%2d",
								&info[i/j].vehicleStart.year, &info[i/j].vehicleStart.month, &info[i/j].vehicleStart.day,
								&info[i/j].vehicleStart.hour, &info[i/j].vehicleStart.min, &info[i/j].vehicleStart.sec);
				}
				if (0 < strlen(sqlrow[7]))
				{
					sscanf(sqlrow[7], "%4d-%2d-%2d %2d:%2d:%2d",
								&info[i/j].vehicleEnd.year, &info[i/j].vehicleEnd.month, &info[i/j].vehicleEnd.day,
								&info[i/j].vehicleEnd.hour, &info[i/j].vehicleEnd.min, &info[i/j].vehicleEnd.sec);
				}
				snprintf(info[i/j].vehicleImagePath, 128, "%s", sqlrow[8]);
				i = i + j;
				sum++;
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MySqlCtrlGetEntranceListBySerchType2(int startPage, int num, TimeYMD_T start, TimeYMD_T end, VehicleSearchInfo_T searchTypeInfo, DeviceEntrance_T *info)
{
	MYSQL *my_handle;
	MYSQL_RES *res_ptr;  
	MYSQL_ROW sqlrow;
	int iret = -1 ,sum = 0;
	int i = 0, j = 0;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[1024] = {0};
	char startDate[128] = {0};
	char endDate[128] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", start.year,start.month,start.day,start.hour,start.min,start.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", end.year,end.month,end.day,end.year, end.month, end.sec);
	switch (searchTypeInfo.type)
	{
		case VehicleSearch_TYPE_Terr:
		{ 
			sprintf(strMysql, "select count(plateNo) AS count, plateNo, any_value (max(startdate)),any_value (area),any_value (platecolor),any_value (vehicletype),any_value (plateImagePath),any_value (id) FROM %s WHERE startdate < '%s' and startdate > '%s' and area = %d GROUP BY plateNo ORDER BY count DESC limit %d,%d;", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.vehicleTerr, (startPage-1)*num, num);   
			break;
		}
		case VehicleSearch_TYPE_Vehicle:
		{
			sprintf(strMysql, "select count(plateNo) AS count, plateNo, any_value (max(startdate)),any_value (area),any_value (platecolor),any_value (vehicletype),any_value (plateImagePath),any_value (id) FROM %s WHERE startdate < '%s' and startdate > '%s' and vehicletype = %d GROUP BY plateNo ORDER BY count DESC limit %d,%d;", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.vehicleType, (startPage-1)*num, num);   
			break;     
		}
		case VehicleSearch_TYPE_Fuzzy:
		{
			sprintf(strMysql, "select count(plateNo) AS count, plateNo, any_value (max(startdate)),any_value (area),any_value (platecolor),any_value (vehicletype),any_value (plateImagePath),any_value (id) FROM %s WHERE startdate < '%s' and startdate > '%s' and concat(plateNo) like '%%%s%%' GROUP BY plateNo ORDER BY count DESC limit %d,%d;", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.fuzzyStr, (startPage-1)*num, num);   
			break;     
		}
		case VehicleSearch_TYPE_Terr | VehicleSearch_TYPE_Vehicle:
		{
			sprintf(strMysql, "select count(plateNo) AS count, plateNo, any_value (max(startdate)),any_value (area),any_value (platecolor),any_value (vehicletype),any_value (plateImagePath),any_value (id) FROM %s WHERE startdate < '%s' and startdate > '%s' and area = %d and vehicletype = %d GROUP BY plateNo ORDER BY count DESC limit %d,%d;", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.vehicleType, (startPage-1)*num, num);   
			break;
		}
		case VehicleSearch_TYPE_Terr | VehicleSearch_TYPE_Fuzzy:
		{
			sprintf(strMysql, "select count(plateNo) AS count, plateNo, any_value (max(startdate)),any_value (area),any_value (platecolor),any_value (vehicletype),any_value (plateImagePath),any_value (id) FROM %s WHERE startdate < '%s' and startdate > '%s' and area = %d and concat(plateNo) like '%%%s%%' GROUP BY plateNo ORDER BY count DESC limit %d,%d;", MYSQL_TABLE_ENTRANCE,  endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.fuzzyStr, (startPage-1)*num, num);   
			break;
		}
		case VehicleSearch_TYPE_Terr | VehicleSearch_TYPE_Vehicle | VehicleSearch_TYPE_Fuzzy:
		{
			sprintf(strMysql, "select count(plateNo) AS count, plateNo, any_value (max(startdate)),any_value (area),any_value (platecolor),any_value (vehicletype),any_value (plateImagePath),any_value (id) FROM %s WHERE startdate < '%s' and startdate > '%s' and area = %d and concat(plateNo) like '%%%s%%' and vehicletype = %d GROUP BY plateNo ORDER BY count DESC limit %d,%d;",  MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.fuzzyStr, searchTypeInfo.vehicleType, (startPage-1)*num, num);   
			break;
		}
		case VehicleSearch_TYPE_Vehicle | VehicleSearch_TYPE_Fuzzy:
		{
			sprintf(strMysql, "select count(plateNo) AS count, plateNo, any_value (max(startdate)),any_value (area),any_value (platecolor),any_value (vehicletype),any_value (plateImagePath),any_value (id) FROM %s WHERE startdate < '%s' and startdate > '%s' and area = %d and vehicletype = %d GROUP BY plateNo ORDER BY count DESC limit %d,%d;", MYSQL_TABLE_ENTRANCE, endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.vehicleType, (startPage-1)*num, num);   
			break;
		}
		case VehicleSearch_TYPE_Increment:  //新增量
		{
			memset(startDate, 0, sizeof(startDate));
			memset(endDate, 0, sizeof(endDate));
			if(start.month == 1)
			{
				start.year -= 1;
				start.month= 12;
			}
			else
			{
				start.month -= 1;
			}
			sprintf(startDate, "%04d-%02d", start.year,start.month);
			sprintf(endDate, "%04d-%02d", end.year,end.month);
			sprintf(strMysql,"select COUNT(plateNo) AS count, plateNo, any_value(max(startdate)), any_value (area), any_value(platecolor), any_value(vehicletype), any_value(plateImagePath),any_value (id) FROM %s tb1 WHERE DATE_FORMAT(startdate, '%%Y-%%m') = '%s' AND not EXISTS(SELECT tbb.* FROM %s tbb WHERE tb1.plateNo = tbb.plateNo AND tb1.startdate > tbb.startdate AND DATE_FORMAT(tbb.startdate, '%%Y-%%m') = '%s') GROUP BY plateNo ORDER BY count DESC limit %d,%d;", MYSQL_TABLE_ENTRANCE, endDate, MYSQL_TABLE_ENTRANCE, startDate, (startPage-1)*num, num);
			break;
		}
		case VehicleSearch_TYPE_Increment | VehicleSearch_TYPE_Vehicle:  //新增量 + 车辆类型
		{
			memset(startDate, 0, sizeof(startDate));
			memset(endDate, 0, sizeof(endDate));
			if(start.month == 1)
			{
				start.year -= 1;
				start.month= 12;
			}
			else
			{
				start.month -= 1;
			}
			sprintf(startDate, "%04d-%02d", start.year,start.month);
			sprintf(endDate, "%04d-%02d", end.year,end.month);
			sprintf(strMysql,"select COUNT(plateNo) AS count, plateNo, any_value(max(startdate)), any_value (area), any_value (platecolor), any_value (vehicletype), any_value (plateImagePath),any_value (id) FROM %s tb1 WHERE DATE_FORMAT(startdate, '%%Y-%%m') = '%s' AND not EXISTS(SELECT tbb.* FROM %s tbb WHERE tb1.plateNo = tbb.plateNo AND tb1.startdate > tbb.startdate AND DATE_FORMAT(tbb.startdate, '%%Y-%%m') = '%s') and vehicletype = %d GROUP BY plateNo ORDER BY count DESC limit %d,%d;", MYSQL_TABLE_ENTRANCE, endDate, MYSQL_TABLE_ENTRANCE, startDate, searchTypeInfo.vehicleType, (startPage-1)*num, num);
			break;
		}
		case VehicleSearch_TYPE_Increment | VehicleSearch_TYPE_Fuzzy:  //新增量 + 关键字
		{
			memset(startDate, 0, sizeof(startDate));
			memset(endDate, 0, sizeof(endDate));
			if(start.month == 1)
			{
				start.year -= 1;
				start.month= 12;
			}
			else
			{
				start.month -= 1;
			}
			sprintf(startDate, "%04d-%02d", start.year,start.month);
			sprintf(endDate, "%04d-%02d", end.year,end.month);
			sprintf(strMysql,"select COUNT(plateNo) AS count, plateNo, any_value(max(startdate)), any_value (area), any_value (platecolor), any_value (vehicletype), any_value (plateImagePath),any_value (id) FROM %s tb1 WHERE DATE_FORMAT(startdate, '%%Y-%%m') = '%s' AND not EXISTS(SELECT tbb.* FROM %s tbb WHERE tb1.plateNo = tbb.plateNo AND tb1.startdate > tbb.startdate AND DATE_FORMAT(tbb.startdate, '%%Y-%%m') = '%s') and concat(plateNo) like '%%%s%%' GROUP BY plateNo ORDER BY count DESC limit %d,%d;", MYSQL_TABLE_ENTRANCE, endDate, MYSQL_TABLE_ENTRANCE, startDate, searchTypeInfo.fuzzyStr, (startPage-1)*num, num);
			break;
		}
		case VehicleSearch_TYPE_Increment | VehicleSearch_TYPE_Fuzzy | VehicleSearch_TYPE_Vehicle:  //新增量 + 关键字 + 车辆类型
		{
			memset(startDate, 0, sizeof(startDate));
			memset(endDate, 0, sizeof(endDate));
			if(start.month == 1)
			{
				start.year -= 1;
				start.month= 12;
			}
			else
			{
				start.month -= 1;
			}
			sprintf(startDate, "%04d-%02d", start.year,start.month);
			sprintf(endDate, "%04d-%02d", end.year,end.month);
			sprintf(strMysql,"select COUNT(plateNo) AS count, plateNo, any_value(max(startdate)), any_value (area), any_value (platecolor), any_value (vehicletype), any_value (plateImagePath),any_value (id) FROM %s tb1 WHERE DATE_FORMAT(startdate, '%%Y-%%m') = '%s' AND not EXISTS(SELECT tbb.* FROM %s tbb WHERE tb1.plateNo = tbb.plateNo AND tb1.startdate > tbb.startdate AND DATE_FORMAT(tbb.startdate, '%%Y-%%m') = '%s') and concat(plateNo) like '%%%s%%' and vehicletype = %d GROUP BY plateNo ORDER BY count DESC limit %d,%d;", MYSQL_TABLE_ENTRANCE, endDate, MYSQL_TABLE_ENTRANCE, startDate, searchTypeInfo.fuzzyStr, searchTypeInfo.vehicleType, (startPage-1)*num, num);
			break;
		}
		case VehicleSearch_TYPE_Losses:  //流失量
		{
			memset(startDate, 0, sizeof(startDate));
			memset(endDate, 0, sizeof(endDate));
			if(start.month == 1)
			{
				start.year -= 1;
				start.month= 12;
			}
			else
			{
				start.month -= 1;
			}
			sprintf(startDate, "%04d-%02d", start.year,start.month);
			sprintf(endDate, "%04d-%02d", end.year,end.month);
			sprintf(strMysql,"SELECT COUNT(plateNo) AS count, plateNo, any_value(max(startdate)), any_value (area), any_value (platecolor), any_value (vehicletype), any_value (plateImagePath),any_value (id) FROM %s tb1 WHERE DATE_FORMAT(startdate, '%%Y-%%m') = '%s' AND not EXISTS(SELECT tbb.* FROM %s tbb WHERE tb1.plateNo = tbb.plateNo AND tb1.startdate > tbb.startdate AND DATE_FORMAT(tbb.startdate, '%%Y-%%m') = '%s') GROUP BY plateNo ORDER BY count DESC limit %d,%d;", MYSQL_TABLE_ENTRANCE, startDate, MYSQL_TABLE_ENTRANCE, endDate, (startPage-1)*num, num);
			break;
		}
		case VehicleSearch_TYPE_Losses | VehicleSearch_TYPE_Vehicle:  //流失量   + 车辆类型
		{
			memset(startDate, 0, sizeof(startDate));
			memset(endDate, 0, sizeof(endDate));
			if(start.month == 1)
			{
				start.year -= 1;
				start.month= 12;
			}
			else
			{
				start.month -= 1;
			}
			sprintf(startDate, "%04d-%02d", start.year,start.month);
			sprintf(endDate, "%04d-%02d", end.year,end.month);
			sprintf(strMysql,"SELECT COUNT(plateNo) AS count, plateNo, any_value(max(startdate)), any_value (area), any_value (platecolor), any_value (vehicletype), any_value (plateImagePath),any_value (id) FROM %s tb1 WHERE DATE_FORMAT(startdate, '%%Y-%%m') = '%s' AND not EXISTS(SELECT tbb.* FROM %s tbb WHERE tb1.plateNo = tbb.plateNo AND tb1.startdate > tbb.startdate AND DATE_FORMAT(tbb.startdate, '%%Y-%%m') = '%s') and vehicletype = %d GROUP BY plateNo ORDER BY count DESC limit %d,%d;", MYSQL_TABLE_ENTRANCE, startDate, MYSQL_TABLE_ENTRANCE, endDate, searchTypeInfo.vehicleType, (startPage-1)*num, num);
			break;
		}
		case VehicleSearch_TYPE_Losses | VehicleSearch_TYPE_Fuzzy:  //流失量    + 关键字
		{
			memset(startDate, 0, sizeof(startDate));
			memset(endDate, 0, sizeof(endDate));
			if(start.month == 1)
			{
				start.year -= 1;
				start.month= 12;
			}
			else
			{
				start.month -= 1;
			}
			sprintf(startDate, "%04d-%02d", start.year,start.month);
			sprintf(endDate, "%04d-%02d", end.year,end.month);
			sprintf(strMysql,"SELECT COUNT(plateNo) AS count, plateNo, any_value(max(startdate)), any_value (area), any_value (platecolor), any_value (vehicletype), any_value (plateImagePath),any_value (id) FROM %s tb1 WHERE DATE_FORMAT(startdate, '%%Y-%%m') = '%s' AND not EXISTS(SELECT tbb.* FROM %s tbb WHERE tb1.plateNo = tbb.plateNo AND tb1.startdate > tbb.startdate AND DATE_FORMAT(tbb.startdate, '%%Y-%%m') = '%s') and concat(plateNo) like '%%%s%%' GROUP BY plateNo ORDER BY count DESC limit %d,%d;", MYSQL_TABLE_ENTRANCE, startDate, MYSQL_TABLE_ENTRANCE, endDate, searchTypeInfo.fuzzyStr, (startPage-1)*num, num);
			break;
		}
		case VehicleSearch_TYPE_Losses | VehicleSearch_TYPE_Vehicle | VehicleSearch_TYPE_Fuzzy:  //流失量    + 车辆类型     + 关键字
		{
			memset(startDate, 0, sizeof(startDate));
			memset(endDate, 0, sizeof(endDate));
			if(start.month == 1)
			{
				start.year -= 1;
				start.month= 12;
			}
			else
			{
				start.month -= 1;
			}
			sprintf(startDate, "%04d-%02d", start.year,start.month);
			sprintf(endDate, "%04d-%02d", end.year,end.month);
			sprintf(strMysql,"SELECT COUNT(plateNo) AS count, plateNo, any_value(max(startdate)), any_value (area), any_value (platecolor), any_value (vehicletype), any_value (plateImagePath),any_value (id) FROM %s tb1 WHERE DATE_FORMAT(startdate, '%%Y-%%m') = '%s' AND not EXISTS(SELECT tbb.* FROM %s tbb WHERE tb1.plateNo = tbb.plateNo AND tb1.startdate > tbb.startdate AND DATE_FORMAT(tbb.startdate, '%%Y-%%m') = '%s') nd concat(plateNo) like '%%%s%%' and vehicletype = %d GROUP BY plateNo ORDER BY count DESC limit %d,%d;", MYSQL_TABLE_ENTRANCE, startDate, MYSQL_TABLE_ENTRANCE, endDate, searchTypeInfo.fuzzyStr, searchTypeInfo.vehicleType, (startPage-1)*num, num);
			break;
		}
		default:
		{
			sprintf(strMysql,"SELECT COUNT(plateNo) AS count, plateNo,any_value (max(startdate)),any_value (area),any_value (platecolor),any_value (vehicletype),any_value (plateImagePath),any_value (id) FROM %s WHERE startdate < '%s' and startdate > '%s' GROUP BY plateNo ORDER BY count DESC limit %d,%d;", MYSQL_TABLE_ENTRANCE, endDate, startDate, (startPage-1)*num, num);   
			break;
		}
	}
	//DF_DEBUG("strMysql = %s",strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{         
			j = mysql_num_fields(res_ptr); 
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				info[i/j].stay = atoi(sqlrow[0]);
				snprintf(info[i/j].plateNo, 128, "%s", sqlrow[1]);
				if (0 < strlen(sqlrow[2]))
				{
					 sscanf(sqlrow[2], "%04d-%02d-%02d %02d:%02d:%02d",
					    &info[i/j].vehicleStart.year, &info[i/j].vehicleStart.month, &info[i/j].vehicleStart.day,
					    &info[i/j].vehicleStart.hour, &info[i/j].vehicleStart.min, &info[i/j].vehicleStart.sec);
				}
				info[i/j].vehicleTerr = atoi(sqlrow[3]);
				info[i/j].vehicleColor = atoi(sqlrow[4]);
				info[i/j].vehicleType = atoi(sqlrow[5]);
				snprintf(info[i/j].vehicleImagePath, 128, "%s", sqlrow[6]);
				snprintf(info[i/j].snapToken, 128, "%s", sqlrow[7]);
				i = i + j;
				sum++;
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}         
		}
		mysql_free_result(res_ptr);
	}
	DF_DEBUG("strMysql = %s",strMysql);
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MysqlCtrlUpdateEntranceInfo(char *token, DeviceEntrance_T *info)
{
	int iret = -1;
	MYSQL *my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char enddate[128] = {0};
	if (!info){
		DF_DEBUG("info is null ! ");
		return KEY_FALSE;
	}
	memset(strMysql, 0, sizeof(strMysql));
	snprintf(enddate, 128, "%04d-%02d-%02d %02d:%02d:%02d", info->vehicleEnd.year, info->vehicleEnd.month, info->vehicleEnd.day, info->vehicleEnd.hour, info->vehicleEnd.min, info->vehicleEnd.sec);
    sprintf(strMysql, "update %s set enddate = '%s' where id = '%s';", 
		MYSQL_TABLE_ENTRANCE, enddate, token);
	
	//DF_DEBUG("strMysql = [%s]", strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlExecute(my_handle, strMysql);
    if( iret != 0 )
    {
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_FALSE;
    }
	//DF_DEBUG("MySqlCtrlModifyUsrBindInfo success !");
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;		
}

static int MySqlCtrlGetLatestEntranceInfo(char *plateNo, DeviceEntrance_T *info)
{
	//select * FROM test where date = (SELECT max(test.date) from test where id = "A1001");
	int i = 0, j = 0;
	int iret = -1;
	char mysqlStr[512] = {0};
	char UsrName[128] = {0};
	MYSQL *my_con;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow; 
	if (0 == strlen(plateNo)){
		return KEY_FALSE;
	}
	
	my_con = GetMySqlHandle()->handle;
	sprintf(mysqlStr, "select * FROM %s where startdate = (SELECT max(%s.startdate) from %s where plateNo = '%s')",
		MYSQL_TABLE_ENTRANCE, MYSQL_TABLE_ENTRANCE, MYSQL_TABLE_ENTRANCE, plateNo);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_con, mysqlStr);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_con);  //取出结果集
		if(res_ptr)
		{       
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				snprintf(info->snapToken, 128, "%s", sqlrow[0]);
				info->plateColor = atoi(sqlrow[1]);
				snprintf(info->plateNo, 128, "%s", sqlrow[2]);
				info->vehicleTerr =  atoi(sqlrow[3]);
				info->vehicleType = atoi(sqlrow[4]);
				info->vehicleColor = atoi(sqlrow[5]);
				if (0 < strlen(sqlrow[6]))
				{
					sscanf(sqlrow[6], "%4d-%2d-%2d %2d:%2d:%2d",
								&info->vehicleStart.year, &info->vehicleStart.month, &info->vehicleStart.day,
								&info->vehicleStart.hour, &info->vehicleStart.min, &info->vehicleStart.sec);
				}
				if (0 < strlen(sqlrow[7]))
				{
					sscanf(sqlrow[7], "%4d-%2d-%2d %2d:%2d:%2d",
								&info->vehicleEnd.year, &info->vehicleEnd.month, &info->vehicleEnd.day,
								&info->vehicleEnd.hour, &info->vehicleEnd.min, &info->vehicleEnd.sec);
				}
				snprintf(info->vehicleImagePath, 128, "%s", sqlrow[8]);
				i = i + j;
			}

			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
			}   
			mysql_free_result(res_ptr);     	
		}
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MySqlCtrlAddAlarmInfo(DeviceAlarm_T *info)
{
	int iret = -1, num = 0;
	MYSQL *my_handle;
	char table[128] = {0};
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char alarmdate[128] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	strcpy(table, (char *)MYSQL_TABLE_ALARM);
	snprintf(alarmdate, 128, "%04d-%02d-%02d %02d:%02d:%02d", info->time.year, info->time.month, info->time.day, info->time.hour, info->time.min, info->time.sec);
  	sprintf(strMysql, "insert into %s(id,alarmtype,alarmdate,alarmlocation) value('%s','%d','%s','%s');", 
		table, info->snapToken, info->alarmType, alarmdate, info->alarmPlace);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlExecute(my_handle, strMysql);
    if( iret != 0)
    {
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_FALSE;
    }
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MySqlCtrlGetAlarmListSum()
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strMysql, "select count(1) from %s;", MYSQL_TABLE_ALARM);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{     
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				sum = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MySqlCtrlGetAlarmList(int startPage, int num, DeviceAlarm_T *info)
{
	int i = 0, j = 0;
	int iret = -1;
	char mysqlStr[512] = {0};
	MYSQL *my_con;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow; 
	if (NULL == info){
		return KEY_FALSE;
	}
	
	my_con = GetMySqlHandle()->handle;
	sprintf(mysqlStr, "select * from %s order by alarmdate desc limit %d,%d;",
		MYSQL_TABLE_ALARM, startPage, num);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_con, mysqlStr);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_con);  //取出结果集
		if(res_ptr)
		{    
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				snprintf(info->snapToken, 128, "%s", sqlrow[0]);
				info->alarmType = atoi(sqlrow[1]);
				if (0 < strlen(sqlrow[2]))
				{
					sscanf(sqlrow[2], "%4d-%2d-%2d %2d:%2d:%2d",
								&info->time.year, &info->time.month, &info->time.day,
								&info->time.hour, &info->time.min, &info->time.sec);
				}
				snprintf(info->alarmPlace, 128, "%s", sqlrow[3]);
				i = i + j;
			}

			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
			}   
			mysql_free_result(res_ptr);     	
		}
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MySqlCtrlGetAlarmListBySearchType(int startPage, int num, TimeYMD_T start, TimeYMD_T end,AlarmLevelSearchType_E type,DeviceAlarm_T *info)
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	int i = 0, j = 0;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char startDate[128] = {0};
	char endDate[128] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", start.year,start.month,start.day, start.hour, start.min, start.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", end.year,end.month,end.day, end.hour, end.min, end.sec);
	switch (type)
	{
		case AlarmLevelSearchType_TYPE_HighTemp:
		{
			sprintf(strMysql, "select * from %s where alarmdate < '%s' and alarmdate > '%s' and alarmtype = %d order by alarmdate desc limit %d,%d;", MYSQL_TABLE_ALARM, endDate, startDate,2 , (startPage-1)*num, num);			
			break;
		}
		case AlarmLevelSearchType_TYPE_Smoking:
		{
			sprintf(strMysql, "select * from %s where alarmdate < '%s' and alarmdate > '%s' and alarmtype = %d order by alarmdate desc limit %d,%d;", MYSQL_TABLE_ALARM, endDate, startDate,1, (startPage-1)*num, num);			
			break;					
		}
		case AlarmLevelSearchType_TYPE_Smoking|AlarmLevelSearchType_TYPE_HighTemp:
		{
			sprintf(strMysql, "select * from %s where alarmdate < '%s' and alarmdate > '%s' and (alarmtype = %d or alarmtype = %d) order by alarmdate desc limit %d,%d;", MYSQL_TABLE_ALARM, endDate, startDate,1,2, (startPage-1)*num, num);			
			break;					
		}
		default:
		{
			sprintf(strMysql, "select * from %s where alarmdate < '%s' and alarmdate > '%s' order by startdate desc limit %d,%d;", MYSQL_TABLE_ALARM, endDate, startDate, (startPage-1)*num, num);	
			break;
		}
	}
	//DF_DEBUG("strMysql = %s",strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{         
			j = mysql_num_fields(res_ptr); 
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				snprintf(info[i/j].snapToken, 128, "%s", sqlrow[0]);
				info[i/j].alarmType = atoi(sqlrow[1]);
				if (0 < strlen(sqlrow[2]))
				{
					sscanf(sqlrow[2], "%4d-%2d-%2d %2d:%2d:%2d",
								&info[i/j].time.year, &info[i/j].time.month, &info[i/j].time.day,
								&info[i/j].time.hour, &info[i/j].time.min, &info[i/j].time.sec);
				}
				snprintf(info[i/j].alarmPlace, 128, "%s", sqlrow[3]);
				i = i + j;
				sum++;
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MySqlCtrlGetAlarmListSumBySearchType( TimeYMD_T start, TimeYMD_T end,AlarmLevelSearchType_E type)
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	int i = 0, j = 0;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char startDate[128] = {0};
	char endDate[128] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", start.year,start.month,start.day, start.hour, start.min, start.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", end.year,end.month,end.day, end.hour, end.min, end.sec);
	switch (type)
	{
		case AlarmLevelSearchType_TYPE_HighTemp:
		{
			sprintf(strMysql, "select count(1) from %s where alarmdate < '%s' and alarmdate > '%s' and alarmtype = %d order by alarmdate;", MYSQL_TABLE_ALARM, endDate, startDate,2 );			
			break;
		}
		case AlarmLevelSearchType_TYPE_Smoking:
		{
			sprintf(strMysql, "select count(1) from %s where alarmdate < '%s' and alarmdate > '%s' and alarmtype = %d order by alarmdate;", MYSQL_TABLE_ALARM, endDate, startDate,1);			
			break;					
		}
		case AlarmLevelSearchType_TYPE_Smoking|AlarmLevelSearchType_TYPE_HighTemp:
		{
			sprintf(strMysql, "select count(1) from %s where alarmdate < '%s' and alarmdate > '%s' and (alarmtype = %d or alarmtype = %d) order by alarmdate;", MYSQL_TABLE_ALARM, endDate, startDate,1,2);			
			break;					
		}
		default:
		{
			sprintf(strMysql, "select count(1) from %s where alarmdate < '%s' and alarmdate > '%s' order by startdate;", MYSQL_TABLE_ALARM, endDate, startDate);	
			break;
		}
	}
	//DF_DEBUG("strMysql = %s",strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{               
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				sum = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}



static int MySqlCtrlAddVipCarInfo(DeviceVipCar_T *info)
{
	int iret = -1, num = 0;
	MYSQL *my_handle;
	char table[128] = {0};
	char dateTime[128] = {0};
	my_handle = GetMySqlHandle()->handle;
	char strMysql[1024] = {0};
	if (NULL == info)
	{
		return KEY_FALSE;
	}
	memset(strMysql, 0, sizeof(strMysql));
	strcpy(table, (char *)MYSQL_TABLE_VIPCar);
	snprintf(dateTime, 128, "%04d-%02d-%02d %02d:%02d:%02d", info->time.year, info->time.month, info->time.day, info->time.hour, info->time.min, info->time.sec);
   	snprintf(strMysql,1024, "insert into %s(id,platecolor,plateNo,area,vehicletype,vehiclecolor,lasttime,plateImagePath,frequency) value('%s','%d','%s','%d','%d','%d','%s','%s','%d');", 
		table, info->vehicle.snapToken, info->vehicle.plateColor, info->vehicle.plateNo, info->vehicle.vehicleTerr, info->vehicle.vehicleType, info->vehicle.vehicleColor, dateTime, info->vehicle.vehicleImagePath, info->conut);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlExecute(my_handle, strMysql);
    if( iret != 0)
    {
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_FALSE;
    }
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MysqlCtrGetVipCarInfoByPlateNo(char *platNo, DeviceVipCar_T *info)
{
	//select count(1) from vipcardata where plateNo = 'A-LY712A';
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int iret = -1;
	int i = 0, j = 0;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strMysql, "select * from %s where plateNo = '%s';", MYSQL_TABLE_VIPCar, platNo);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{      
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				snprintf(info->vehicle.snapToken, 128, "%s", sqlrow[0]);
				info->vehicle.plateColor = atoi(sqlrow[1]);
				snprintf(info->vehicle.plateNo, 128, "%s", sqlrow[2]);
				info->vehicle.vehicleTerr = atoi(sqlrow[3]);
				info->vehicle.vehicleType = atoi(sqlrow[4]);
				info->vehicle.vehicleColor = atoi(sqlrow[5]);
				if (0 < strlen(sqlrow[6]))
				{
					sscanf(sqlrow[6], "%4d-%2d-%2d %2d:%2d:%2d",
								&info->time.year, &info->time.month, &info->time.day,
								&info->time.hour, &info->time.min, &info->time.sec);
				}
				snprintf(info->vehicle.vehicleImagePath, 128, "%s", sqlrow[7]);
				info->conut = atoi(sqlrow[8]);
				i = i + j;		
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MysqlCtrUpdateVipCarInfo(char *token, DeviceVipCar_T *info)
{
	int iret = -1;
	MYSQL *my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char dateTime[128] = {0};
	if (!info){
		DF_DEBUG("info is null ! ");
		return KEY_FALSE;
	}
	memset(strMysql, 0, sizeof(strMysql));
	snprintf(dateTime, 128, "%04d-%02d-%02d %02d:%02d:%02d", info->time.year, info->time.month, info->time.day, info->time.hour, info->time.min, info->time.sec);
    sprintf(strMysql, "update %s set lasttime = '%s', frequency = '%d' where id = %s;", 
		MYSQL_TABLE_VIPCar, dateTime, info->conut, token);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlExecute(my_handle, strMysql);
    if( iret != 0 )
    {
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_FALSE;
    }
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;			
}

static int MySqlCtrlGetVipCarListSum()
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strMysql, "select count(1) from %s;", MYSQL_TABLE_VIPCar);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{      
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				sum = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MySqlCtrlGetVipCarList(int startPage, int num, DeviceVipCar_T *info)
{
	int i = 0, j = 0;
	int iret = -1;
	char mysqlStr[512] = {0};
	MYSQL *my_con;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow; 
	if (NULL == info){
		return KEY_FALSE;
	}
	
	my_con = GetMySqlHandle()->handle;
	sprintf(mysqlStr, "select * from %s order by lasttime desc limit %d,%d;",
		MYSQL_TABLE_VIPCar, startPage, num);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_con, mysqlStr);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_con);  //取出结果集
		if(res_ptr)
		{     
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				snprintf(info->vehicle.snapToken, 128, "%s", sqlrow[0]);
				info->vehicle.plateColor = atoi(sqlrow[1]);
				snprintf(info->vehicle.plateNo, 128, "%s", sqlrow[2]);
				info->vehicle.vehicleTerr = atoi(sqlrow[3]);
				info->vehicle.vehicleType = atoi(sqlrow[4]);
				info->vehicle.vehicleColor = atoi(sqlrow[5]);
				if (0 < strlen(sqlrow[6]))
				{
					sscanf(sqlrow[6], "%4d-%2d-%2d %2d:%2d:%2d",
								&info->time.year, &info->time.month, &info->time.day,
								&info->time.hour, &info->time.min, &info->time.sec);
				}
				snprintf(info->vehicle.vehicleImagePath, 128, "%s", sqlrow[7]);
				info->conut = atoi(sqlrow[8]);
				i = i + j;
			}

			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
			}   
			mysql_free_result(res_ptr);     	
		}
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return iret;
}

static int MySqlCtrlGetVipCarListBySerchType(int startPage, int num, TimeYMD_T start, TimeYMD_T end, VehicleSearchInfo_T searchTypeInfo,DeviceVipCar_T *info)
{
	int i = 0, j = 0;
	int iret = -1;
	int sum = 0;
	MYSQL *my_con;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow; 
	if (NULL == info){
		return KEY_FALSE;
	}
	char strMysql[512] = {0};
	char startDate[128] = {0};
	char endDate[128] = {0};
	my_con = GetMySqlHandle()->handle;
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", start.year,start.month,start.day, start.hour, start.min, start.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", end.year,end.month,end.day, end.hour, end.min, end.sec);
	switch (searchTypeInfo.type)
	{
		case VehicleSearch_TYPE_Terr:
		{
			sprintf(strMysql, "select * from %s where lasttime < '%s' and lasttime > '%s' and area = %d order by lasttime desc limit %d,%d;", MYSQL_TABLE_VIPCar, endDate, startDate, searchTypeInfo.vehicleTerr, startPage, num);			
			break;
		}
		case VehicleSearch_TYPE_Vehicle:
		{
			sprintf(strMysql, "select * from %s where lasttime < '%s' and lasttime > '%s' and vehicletype = %d order by lasttime desc limit %d,%d;", MYSQL_TABLE_VIPCar, endDate, startDate, searchTypeInfo.vehicleType, startPage, num);			
			break;					
		}
		case VehicleSearch_TYPE_Fuzzy:
		{
			sprintf(strMysql, "select * from %s where lasttime < '%s' and lasttime > '%s' and concat(platNo) like '%%%s%%' order by lasttime desc limit %d,%d;", MYSQL_TABLE_VIPCar, endDate, startDate, searchTypeInfo.fuzzyStr, startPage, num);			
			break;					
		}
		case VehicleSearch_TYPE_Terr | VehicleSearch_TYPE_Vehicle:
		{
			sprintf(strMysql, "select * from %s where lasttime < '%s' and lasttime > '%s' and area = %d and vehicletype = %d order by lasttime desc limit %d,%d;", MYSQL_TABLE_VIPCar, endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.vehicleType, startPage, num);			
			break;
		}
		case VehicleSearch_TYPE_Terr | VehicleSearch_TYPE_Fuzzy:
		{
			sprintf(strMysql, "select * from %s where lasttime < '%s' and lasttime > '%s' and area = %d and concat(platNo) like '%%%s%%' order by lasttime desc limit %d,%d;", MYSQL_TABLE_VIPCar, endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.fuzzyStr, startPage, num);			
			break;
		}
		case VehicleSearch_TYPE_Terr | VehicleSearch_TYPE_Vehicle | VehicleSearch_TYPE_Fuzzy:
		{
			sprintf(strMysql, "select * from %s where lasttime < '%s' and lasttime > '%s' and area = %d and concat(platNo) like '%%%s%%' and vehicletype = %d order by lasttime desc limit %d,%d;", MYSQL_TABLE_VIPCar, endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.fuzzyStr, searchTypeInfo.vehicleType, startPage, num);			
			break;
		}
		case VehicleSearch_TYPE_Vehicle | VehicleSearch_TYPE_Fuzzy:
		{
			sprintf(strMysql, "select * from %s where lasttime < '%s' and lasttime > '%s' and area = %d and vehicletype = %d order by lasttime desc limit %d,%d;", MYSQL_TABLE_VIPCar, endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.vehicleType, startPage, num);			
			break;
		}
		default:
		{
			sprintf(strMysql, "select * from %s where lasttime < '%s' and lasttime > '%s' order by lasttime desc limit %d,%d;", MYSQL_TABLE_VIPCar, endDate, startDate, startPage, num);	
			break;
		}
	}

	//DF_DEBUG("strMysql: %s", strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_con, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_con);  //取出结果集
		if(res_ptr)
		{     
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				snprintf(info[i/j].vehicle.snapToken, 128, "%s", sqlrow[0]);
				info[i/j].vehicle.plateColor = atoi(sqlrow[1]);
				snprintf(info[i/j].vehicle.plateNo, 128, "%s", sqlrow[2]);
				info[i/j].vehicle.vehicleTerr = atoi(sqlrow[3]);
				info[i/j].vehicle.vehicleType = atoi(sqlrow[4]);
				info[i/j].vehicle.vehicleColor = atoi(sqlrow[5]);
				if (0 < strlen(sqlrow[6]))
				{
					sscanf(sqlrow[6], "%4d-%2d-%2d %2d:%2d:%2d",
								&info[i/j].time.year, &info[i/j].time.month, &info[i/j].time.day,
								&info[i/j].time.hour, &info[i/j].time.min, &info[i/j].time.sec);
				}
				snprintf(info[i/j].vehicle.vehicleImagePath, 128, "%s", sqlrow[7]);
				info[i/j].conut = atoi(sqlrow[8]);
				i = i + j;
				sum ++;
			}

			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
			}   
			mysql_free_result(res_ptr);     	
		}
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MySqlCtrlGetVipCarListSumBySerchType(TimeYMD_T start, TimeYMD_T end, VehicleSearchInfo_T searchTypeInfo)
{
	 int i = 0, j = 0;
	 int iret = -1;
	 char mysqlStr[512] = {0};
	 int sum = 0;
	 MYSQL *my_con;
	 MYSQL_RES *res_ptr;  
	 MYSQL_ROW sqlrow; 
	 
	 char strMysql[512] = {0};
	 char startDate[128] = {0};
	 char endDate[128] = {0};
	 my_con = GetMySqlHandle()->handle;
	 sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", start.year,start.month,start.day, start.hour, start.min, start.sec);
	 sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", end.year,end.month,end.day, end.hour, end.min, end.sec);

	 switch (searchTypeInfo.type)
	 {
		  case VehicleSearch_TYPE_Terr:
		  {
		   	sprintf(strMysql, "select count(1) from %s where lasttime < '%s' and lasttime > '%s' and area = %d;", MYSQL_TABLE_VIPCar, endDate, startDate, searchTypeInfo.vehicleTerr);   
		   	break;
		  }
		  case VehicleSearch_TYPE_Vehicle:
		  {
		   	sprintf(strMysql, "select count(1) from %s where lasttime < '%s' and lasttime > '%s' and vehicletype = %d;", MYSQL_TABLE_VIPCar, endDate, startDate, searchTypeInfo.vehicleType);   
		   	break;     
		  }
		  case VehicleSearch_TYPE_Fuzzy:
		  {
		   	sprintf(strMysql, "select count(1) from %s where lasttime < '%s' and lasttime > '%s' and concat(platNo) like '%%%s%%';", MYSQL_TABLE_VIPCar, endDate, startDate, searchTypeInfo.fuzzyStr);   
		  	 break;     
		  }
		  case VehicleSearch_TYPE_Terr | VehicleSearch_TYPE_Vehicle:
		  {
		   	sprintf(strMysql, "select count(1) from %s where lasttime < '%s' and lasttime > '%s' and area = %d and vehicletype = %d;", MYSQL_TABLE_VIPCar, endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.vehicleType);   
		   	break;
		  }
		  case VehicleSearch_TYPE_Terr | VehicleSearch_TYPE_Fuzzy:
		  {
		   	sprintf(strMysql, "select count(1) from %s where lasttime < '%s' and lasttime > '%s' and area = %d and concat(platNo) like '%%%s%%';", MYSQL_TABLE_VIPCar, endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.fuzzyStr);   
		   	break;
		  }
		  case VehicleSearch_TYPE_Terr | VehicleSearch_TYPE_Vehicle | VehicleSearch_TYPE_Fuzzy:
		  {
		   	sprintf(strMysql, "select count(1) from %s where lasttime < '%s' and lasttime > '%s' and area = %d and concat(platNo) like '%%%s%%' and vehicletype = %d;", MYSQL_TABLE_VIPCar, endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.fuzzyStr, searchTypeInfo.vehicleType);   
		   	break;
		  }
		  case VehicleSearch_TYPE_Vehicle | VehicleSearch_TYPE_Fuzzy:
		  {
		  	 sprintf(strMysql, "select count(1) from %s where lasttime < '%s' and lasttime > '%s' and area = %d and vehicletype = %d;", MYSQL_TABLE_VIPCar, endDate, startDate, searchTypeInfo.vehicleTerr, searchTypeInfo.vehicleType);   
		   	 break;
		  }
		  default:
		  {
		   	 sprintf(strMysql, "select count(1) from %s where lasttime < '%s' and lasttime > '%s';", MYSQL_TABLE_VIPCar, endDate, startDate); 
		   	 break;
		  }
	 }
	//DF_DEBUG("strMysql: %s", strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_con, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_con);
		if(res_ptr)
		{      
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				sum = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MySqlCtrlAddWashCarInfo(DeviceWash_T *info)
{
	int iret = -1, num = 0;
	MYSQL *my_handle;
	char table[128] = {0};
	char startdate[128] = {0};
	char enddate[128] = {0};
	my_handle = GetMySqlHandle()->handle;
	char strMysql[1024] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	strcpy(table, (char *)MYSQL_TABLE_WASHCar);
	snprintf(startdate, 128, "%04d-%02d-%02d %02d:%02d:%02d", info->vehicleStart.year, info->vehicleStart.month, info->vehicleStart.day, info->vehicleStart.hour, info->vehicleStart.min, info->vehicleStart.sec);
	snprintf(enddate, 128, "%04d-%02d-%02d %02d:%02d:%02d", info->vehicleEnd.year, info->vehicleEnd.month, info->vehicleEnd.day, info->vehicleEnd.hour, info->vehicleEnd.min, info->vehicleEnd.sec);
	snprintf(strMysql,1024, "insert into %s(id,plateNo,startdate,enddate) value('%s','%s','%s','%s');", 
		table, info->snapToken, info->plateNo, startdate, enddate);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlExecute(my_handle, strMysql);
    if( iret != 0)
    {
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_FALSE;
    }
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MySqlCtrlWashCarListSumByTime(TimeYMD_T start, TimeYMD_T end)
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char startDate[128] = {0};
	char endDate[128] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", start.year,start.month,start.day, start.hour, start.min, start.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", end.year,end.month,end.day,end.hour, end.min, end.sec);
	sprintf(strMysql, "select count(1) from %s where startdate < '%s' and startdate > '%s';", MYSQL_TABLE_WASHCar, endDate, startDate);
	DF_DEBUG("strMysql = %s",strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{               
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				sum = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MySqlCtrlGetLastWashCarInfo(char *plateNo, DeviceWash_T *info)
{
	int i = 0, j = 0;
	int iret = -1;
	char mysqlStr[512] = {0};
	char UsrName[128] = {0};
	MYSQL *my_con;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow; 
	if (0 == strlen(plateNo)){
		return KEY_FALSE;
	}
	
	my_con = GetMySqlHandle()->handle;
	sprintf(mysqlStr, "select * FROM %s where startdate = (SELECT max(%s.startdate) from %s where plateNo = '%s')",
		MYSQL_TABLE_WASHCar, MYSQL_TABLE_WASHCar, MYSQL_TABLE_WASHCar, plateNo);
	//DF_DEBUG("mysqlStr:%s", mysqlStr);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_con, mysqlStr);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_con);  //取出结果集
		if(res_ptr)
		{      
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				snprintf(info->snapToken, 128, "%s", sqlrow[0]);
				snprintf(info->plateNo, 128, "%s", sqlrow[1]);
				if (0 < strlen(sqlrow[2]))
				{
					sscanf(sqlrow[2], "%4d-%2d-%2d %2d:%2d:%2d",
								&info->vehicleStart.year, &info->vehicleStart.month, &info->vehicleStart.day,
								&info->vehicleStart.hour, &info->vehicleStart.min, &info->vehicleStart.sec);
				}
				if (0 < strlen(sqlrow[3]))
				{
					sscanf(sqlrow[3], "%4d-%2d-%2d %2d:%2d:%2d",
								&info->vehicleEnd.year, &info->vehicleEnd.month, &info->vehicleEnd.day,
								&info->vehicleEnd.hour, &info->vehicleEnd.min, &info->vehicleEnd.sec);
				}
				i = i + j;
			}
			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
			}   
			mysql_free_result(res_ptr);     	
		}
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MySqlCtrlSetWashCarInfo(char *token, DeviceWash_T *info)
{
	int iret = -1;
	MYSQL *my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char enddate[128] = {0};
	if (!info){
		DF_DEBUG("info is null ! ");
		return KEY_FALSE;
	}
	memset(strMysql, 0, sizeof(strMysql));
	snprintf(enddate, 128, "%04d-%02d-%02d %02d:%02d:%02d", info->vehicleEnd.year, info->vehicleEnd.month,
		info->vehicleEnd.day, info->vehicleEnd.hour, info->vehicleEnd.min, info->vehicleEnd.sec);
    sprintf(strMysql, "update %s set enddate = '%s' where id = '%s';", 
		MYSQL_TABLE_WASHCar, enddate, token);
	
	//DF_DEBUG("strMysql = [%s]", strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlExecute(my_handle, strMysql);
    if( iret != 0 )
    {
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_FALSE;
    }
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;	
}

//车辆轨迹
static int MySqlCtrlAddTrajectoryInfo(DeviceTrajectory_T *info)
{
	int iret = -1, num = 0;
	MYSQL *my_handle;
	char table[128] = {0};
	char arrivalstartdate[128] = {0};
	char arrivalenddate[128] = {0};
	char washstartdate[128] = {0};
	char washenddate[128] = {0};
	my_handle = GetMySqlHandle()->handle;
	char strMysql[1024] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	strcpy(table, (char *)MYSQL_TABLE_CARTRAJECTORY);
	snprintf(arrivalstartdate, 1024, "%04d-%02d-%02d %02d:%02d:%02d", info->arrivalstartdate.year, info->arrivalstartdate.month, info->arrivalstartdate.day, 
		info->arrivalstartdate.hour, info->arrivalstartdate.min, info->arrivalstartdate.sec);
	snprintf(arrivalenddate, 1024, "%04d-%02d-%02d %02d:%02d:%02d", info->arrivalenddate.year, info->arrivalenddate.month, info->arrivalenddate.day, 
		info->arrivalenddate.hour, info->arrivalenddate.min, info->arrivalenddate.sec);
	snprintf(washstartdate, 1024, "%04d-%02d-%02d %02d:%02d:%02d", info->washstartdate.year, info->washstartdate.month, info->washstartdate.day, 
		info->washstartdate.hour, info->washstartdate.min, info->washstartdate.sec);
	snprintf(washenddate, 1024, "%04d-%02d-%02d %02d:%02d:%02d", info->washenddate.year, info->washenddate.month, info->washenddate.day, 
		info->washenddate.hour, info->washenddate.min, info->washenddate.sec);
	snprintf(strMysql,1024, "insert into %s(id,plateNo,arrivalstartdate,arrivalenddate,washstartdate,washenddate) value('%s','%s','%s','%s','%s','%s');", 
		table, info->snapToken, info->plateNo, arrivalstartdate, arrivalenddate, washstartdate, washenddate);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	//DF_DEBUG("strMysql: %s", strMysql);
	iret = MysqlExecute(my_handle, strMysql);
	if(iret != 0)
	{
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_FALSE;
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MySqlCtrlGetTrajectoryInfo(char *token, DeviceTrajectory_T *info)
{
	int i = 0, j = 0;
	int iret = -1;
	char mysqlStr[512] = {0};
	MYSQL *my_con;
	MYSQL_RES *res_ptr;  
	MYSQL_ROW sqlrow; 
	if (NULL == info){
		return KEY_FALSE;
	}
	
	my_con = GetMySqlHandle()->handle;
	sprintf(mysqlStr, "select * from %s where id = '%s';",
		MYSQL_TABLE_CARTRAJECTORY, token);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_con, mysqlStr);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_con);  //取出结果集
		if(res_ptr)
		{     
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			j = mysql_num_fields(res_ptr);    
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				snprintf(info->plateNo, 128, "%s", sqlrow[1]);
				sscanf(sqlrow[2], "%4d-%2d-%2d %2d:%2d:%2d",
				        &info->arrivalstartdate.year, &info->arrivalstartdate.month, &info->arrivalstartdate.day,
				        &info->arrivalstartdate.hour, &info->arrivalstartdate.min, &info->arrivalstartdate.sec);
				sscanf(sqlrow[3], "%4d-%2d-%2d %2d:%2d:%2d",
				        &info->arrivalenddate.year, &info->arrivalenddate.month, &info->arrivalenddate.day,
				        &info->arrivalenddate.hour, &info->arrivalenddate.min, &info->arrivalenddate.sec);
				sscanf(sqlrow[4], "%4d-%2d-%2d %2d:%2d:%2d",
				        &info->washstartdate.year, &info->washstartdate.month, &info->washstartdate.day,
				        &info->washstartdate.hour, &info->washstartdate.min, &info->washstartdate.sec);
				sscanf(sqlrow[5], "%4d-%2d-%2d %2d:%2d:%2d",
				        &info->washenddate.year, &info->washenddate.month, &info->washenddate.day,
				        &info->washenddate.hour, &info->washenddate.min, &info->washenddate.sec);
				i = i + j;
			}
			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
				}   
			mysql_free_result(res_ptr);      
		}
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return iret;
}

static int MysqlCtrlUpdateTrajectoryInfo(char *token, DeviceTrajectory_T *info)
{
	int iret = -1;
	MYSQL *my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char szArrivalstartdate[128] = {0};  
	char szArrivalenddate[128] = {0};
	char szWashstartdate[128] = {0};
	char szWashenddate[128] = {0};
	if (!info){
		DF_DEBUG("info is null ! ");
		return KEY_FALSE;
	}
	memset(strMysql, 0, sizeof(strMysql));
	snprintf(szArrivalstartdate, 128, "%04d-%02d-%02d %02d:%02d:%02d", info->arrivalstartdate.year, info->arrivalstartdate.month, info->arrivalstartdate.day, info->arrivalstartdate.hour, info->arrivalstartdate.min, info->arrivalstartdate.sec);
	snprintf(szArrivalenddate, 128, "%04d-%02d-%02d %02d:%02d:%02d", info->arrivalenddate.year, info->arrivalenddate.month, info->arrivalenddate.day, info->arrivalenddate.hour, info->arrivalenddate.min, info->arrivalenddate.sec);
	snprintf(szWashstartdate, 128, "%04d-%02d-%02d %02d:%02d:%02d", info->washstartdate.year, info->washstartdate.month, info->washstartdate.day, info->washstartdate.hour, info->washstartdate.min, info->washstartdate.sec);
	snprintf(szWashenddate, 128, "%04d-%02d-%02d %02d:%02d:%02d", info->washenddate.year, info->washenddate.month, info->washenddate.day, info->washenddate.hour, info->washenddate.min, info->washenddate.sec);
	sprintf(strMysql, "update %s set arrivalstartdate = '%s',arrivalenddate = '%s',washstartdate = '%s',washenddate = '%s' where id = '%s';", 
	MYSQL_TABLE_CARTRAJECTORY, szArrivalstartdate, szArrivalenddate, szWashstartdate, szWashenddate, token);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlExecute(my_handle, strMysql);
	if( iret != 0 )
	{
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_FALSE;
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;  
}

static int MysqlCtrlGetaddWithoutWashNum(TimeYMD_T startTime, TimeYMD_T endTime)
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char startDate[128] = {0};
	char endDate[128] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", startTime.year,startTime.month,startTime.day, startTime.hour, startTime.min, startTime.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", endTime.year,endTime.month,endTime.day,endTime.hour, endTime.min, endTime.sec);
	//select count(1) from cartrajectory where arrivalstartdate = washstartdate and arrivalstartdate >'2021-07-01 09:00:00' and arrivalstartdate < '2021-07-01 23:00:00';
	sprintf(strMysql, "select count(1) from %s where arrivalstartdate = washstartdate and arrivalstartdate >'%s' and arrivalstartdate < '%s';", MYSQL_TABLE_CARTRAJECTORY, startDate, endDate);
	DF_DEBUG("strMysql = %s",strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{               
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				sum = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MysqlCtrlGetWashWithoutaddNum(TimeYMD_T startTime, TimeYMD_T endTime)
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char startDate[128] = {0};
	char endDate[128] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", startTime.year,startTime.month,startTime.day, startTime.hour, startTime.min, startTime.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", endTime.year,endTime.month,endTime.day,endTime.hour, endTime.min, endTime.sec);
	sprintf(strMysql, "select count(1) from %s where UNIX_TIMESTAMP(arrivalenddate) - UNIX_TIMESTAMP(washenddate) > 0 and UNIX_TIMESTAMP(arrivalenddate) - UNIX_TIMESTAMP(washenddate) < 60 and arrivalstartdate >'%s' and arrivalstartdate < '%s';", MYSQL_TABLE_CARTRAJECTORY, startDate, endDate);
	DF_DEBUG("strMysql = %s",strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{               
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				sum = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MysqlCtrlGetaddAndwashNum(TimeYMD_T startTime, TimeYMD_T endTime)
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char startDate[128] = {0};
	char endDate[128] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", startTime.year,startTime.month,startTime.day, startTime.hour, startTime.min, startTime.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", endTime.year,endTime.month,endTime.day,endTime.hour, endTime.min, endTime.sec);
	//select count(1) from cartrajectory where arrivalstartdate != washstartdate and UNIX_TIMESTAMP(arrivalstartdate) - UNIX_TIMESTAMP(washstartdate) >3*60 and arrivalstartdate >'2021-07-01 09:00:00' and arrivalstartdate < '2020-07-01 23:00:00';
	sprintf(strMysql, "select count(1) from %s where arrivalstartdate != washstartdate and UNIX_TIMESTAMP(arrivalstartdate) - UNIX_TIMESTAMP(washstartdate) >3*60 and arrivalstartdate >'%s' and arrivalstartdate < '%s';", MYSQL_TABLE_CARTRAJECTORY, startDate, endDate);
	DF_DEBUG("strMysql = %s",strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{               
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				sum = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MysqlCtrlGetwashAndaddNum(TimeYMD_T startTime, TimeYMD_T endTime)
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char startDate[128] = {0};
	char endDate[128] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", startTime.year,startTime.month,startTime.day, startTime.hour, startTime.min, startTime.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", endTime.year,endTime.month,endTime.day,endTime.hour, endTime.min, endTime.sec);
	//select count(1) from cartrajectory where arrivalstartdate != washstartdate and UNIX_TIMESTAMP(arrivalenddate) - UNIX_TIMESTAMP(washenddate) > 5 * 60 and arrivalstartdate >'2020-07-01 09:00:00' and arrivalstartdate < '2020-07-01 23:00:00';#先洗
	sprintf(strMysql, "select count(1) from %s where arrivalstartdate != washstartdate and UNIX_TIMESTAMP(arrivalenddate) - UNIX_TIMESTAMP(washenddate) > 5 * 60 and arrivalstartdate >'%s' and arrivalstartdate < '%s';", MYSQL_TABLE_CARTRAJECTORY, startDate, endDate);
	DF_DEBUG("strMysql = %s",strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{               
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				sum = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

//加油岛
static int MySqlCtrlAddIsLandInfo(DeviceOilLand_T *info)
{
	int iret = -1, num = 0;
	MYSQL *my_handle;
	char table[128] = {0};
	char startdate[128] = {0};
	my_handle = GetMySqlHandle()->handle;
	char strMysql[1024] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	strcpy(table, (char *)MYSQL_TABLE_ISLAND);
	snprintf(startdate, 128, "%04d-%02d-%02d %02d:%02d:%02d", info->snaptime.year, info->snaptime.month, info->snaptime.day, info->snaptime.hour, info->snaptime.min, info->snaptime.sec);
	snprintf(strMysql,1024, "insert into %s(id,plateNo,date) value('%s','%s','%s');", 
		table, info->snapToken, info->plateNo, startdate);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlExecute(my_handle, strMysql);
    if( iret != 0)
    {
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_FALSE;
    }
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MySqlCtrlGetIsLandListSumByTime(TimeYMD_T start, TimeYMD_T end)
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char startDate[128] = {0};
	char endDate[128] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", start.year,start.month,start.day, start.hour, start.min, start.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", end.year,end.month,end.day,end.hour, end.min, end.sec);
	sprintf(strMysql, "select count(1) from %s where startdate < '%s' and startdate > '%s';", MYSQL_TABLE_ISLAND, endDate, startDate);
	DF_DEBUG("strMysql = %s",strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{               
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				sum = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MySqlCtrlGetLastIsLandInfo(char *plateNo, DeviceOilLand_T *info)
{
	int i = 0, j = 0;
	int iret = -1;
	char mysqlStr[512] = {0};
	char UsrName[128] = {0};
	MYSQL *my_con;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow; 
	if (0 == strlen(plateNo)){
		return KEY_FALSE;
	}
	
	my_con = GetMySqlHandle()->handle;
	sprintf(mysqlStr, "select * FROM %s where date = (SELECT max(%s.date) from %s where plateNo = '%s')",
		MYSQL_TABLE_ISLAND, MYSQL_TABLE_ISLAND, MYSQL_TABLE_ISLAND, plateNo);
	//DF_DEBUG("mysqlStr:%s", mysqlStr);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_con, mysqlStr);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_con);  //取出结果集
		if(res_ptr)
		{      
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				snprintf(info->snapToken, 128, "%s", sqlrow[0]);
				snprintf(info->plateNo, 128, "%s", sqlrow[1]);
				if (0 < strlen(sqlrow[2]))
				{
					sscanf(sqlrow[2], "%4d-%2d-%2d %2d:%2d:%2d",
								&info->snaptime.year, &info->snaptime.month, &info->snaptime.day,
								&info->snaptime.hour, &info->snaptime.min, &info->snaptime.sec);
				}
				i = i + j;
			}
			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
			}   
			mysql_free_result(res_ptr);     	
		}
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MySqlCtrlSetIsLandInfo(char *token, DeviceOilLand_T *info)
{
	int iret = -1;
	MYSQL *my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char enddate[128] = {0};
	if (!info){
		DF_DEBUG("info is null ! ");
		return KEY_FALSE;
	}
	memset(strMysql, 0, sizeof(strMysql));
	snprintf(enddate, 128, "%04d-%02d-%02d %02d:%02d:%02d", info->snaptime.year, info->snaptime.month,
		info->snaptime.day, info->snaptime.hour, info->snaptime.min, info->snaptime.sec);
    sprintf(strMysql, "update %s set date = '%s' where id = '%s';", 
		MYSQL_TABLE_ISLAND, enddate, token);
	//DF_DEBUG("strMysql = [%s]", strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlExecute(my_handle, strMysql);
    if( iret != 0 )
    {
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_FALSE;
    }
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;	
}

//VIPCunstom_T
//AttendanceAtt_T
//无感考勤
//.....唯一token 员工姓名	员工编号	员工备注	打卡时间
//访客记录
//.....唯一token	 客户姓名 客户编号	客户备注 抓拍时间 频次
//入店客流
//.....唯一token 性别     年龄 入店时间
static int MySqlCtrlAddAttendanceInfo(AttendanceAtt_T *info)
{
	int iret = -1, num = 0;
	MYSQL *my_handle;
	char table[128] = {0};
	char dateTime[128] = {0};
	my_handle = GetMySqlHandle()->handle;
	char strMysql[1024] = {0};
	if (NULL == info)
	{
		return KEY_FALSE;
	}
	memset(strMysql, 0, sizeof(strMysql));
	strcpy(table, (char *)MYSQL_TABLE_ATTENDDANCE);
	snprintf(dateTime, 128, "%04d-%02d-%02d %02d:%02d:%02d", info->snapTime.year, info->snapTime.month, info->snapTime.day, info->snapTime.hour, info->snapTime.min, info->snapTime.sec);
   	snprintf(strMysql, 1024, "insert into %s(id,name,number,phonenum,remarks,time,plateImagePath,snapImagePath) value('%s','%s','%s','%s','%s','%s','%s','%s');", 
		table, info->snapToken, info->name, info->usrid,info->phonenum, "", dateTime, info->plateImagePath, info->snapImagePath);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlExecute(my_handle, strMysql);
    if( iret != 0)
    {
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_FALSE;
    }
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MySqlCtrlGetAttendanceHistorySum(TimeYMD_T startTime,TimeYMD_T endTime, char *fuzzyStr)
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	char startDate[128] = {0}; 
	char endDate[128] = {0};
	int sum  = 0;
	int iret = -1;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", startTime.year,startTime.month,startTime.day, startTime.hour, startTime.min, startTime.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", endTime.year,endTime.month,endTime.day, endTime.hour, endTime.min, endTime.sec);
	sprintf(strMysql, "select count(1) from %s where time < '%s' and time > '%s' and concat(name,number,phonenum) like '%%%s%%';", MYSQL_TABLE_ATTENDDANCE,endDate, startDate, fuzzyStr);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{     
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				sum = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}


static int MySqlCtrlGetAttendanceHistoryBySearchType(TimeYMD_T startTime,TimeYMD_T endTime, int startPage, int num,char *fuzzyStr, AttendanceAtt_T *info)
{
	int i = 0, j = 0;
	int iret = -1;
	char mysqlStr[512] = {0};
	char startDate[128] = {0}; 
	char endDate[128] = {0};
	MYSQL *my_con;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow; 
	if (NULL == info){
		return KEY_FALSE;
	}
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", startTime.year,startTime.month,startTime.day, startTime.hour, startTime.min, startTime.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", endTime.year,endTime.month,endTime.day, endTime.hour, endTime.min, endTime.sec);
	my_con = GetMySqlHandle()->handle;
	sprintf(mysqlStr, "select * from %s where time < '%s' and time > '%s' and concat(name,number,phonenum) like '%%%s%%' order by time desc limit %d,%d;", MYSQL_TABLE_ATTENDDANCE, endDate, startDate, fuzzyStr,((startPage -1) * num), num);			
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	printf("sql =%s\n",mysqlStr);
	iret = MysqlSelect(my_con, mysqlStr);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_con);  //å–å‡ºç»“æžœé›?
		if(res_ptr)
		{ 
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				snprintf(info[i/j].snapToken, 128, "%s", sqlrow[0]);
				snprintf(info[i/j].name, 64, "%s", sqlrow[1]);
				snprintf(info[i/j].usrid, 128, "%s", sqlrow[2]);
				snprintf(info[i/j].phonenum, 12, "%s", sqlrow[3]);
				if (0 < strlen(sqlrow[5]))
				{
					sscanf(sqlrow[5], "%4d-%2d-%2d %2d:%2d:%2d",
								&info[i/j].snapTime.year, &info[i/j].snapTime.month, &info[i/j].snapTime.day,
								&info[i/j].snapTime.hour, &info[i/j].snapTime.min, &info[i/j].snapTime.sec);
				}
				snprintf(info[i/j].plateImagePath, 128, "%s", sqlrow[6]);
				snprintf(info[i/j].snapImagePath, 128, "%s", sqlrow[7]);
				i = i + j;
			}

			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
			}   
			mysql_free_result(res_ptr);     	
		}
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return iret;
}

static int MysqlCtrlGetLastVisitorsInfoSumByDate(TimeYMD_T startTime, TimeYMD_T endTime)
{
 int i = 0, j = 0;
 int iret = -1;
 int sum = 0;
 char mysqlStr[512] = {0};
 char startDate[128] = {0};
 char endDate[128] = {0};
 MYSQL *my_con;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
 my_con = GetMySqlHandle()->handle;
 sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", startTime.year,startTime.month,startTime.day, startTime.hour, startTime.min, startTime.sec);
sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", endTime.year,endTime.month,endTime.day, endTime.hour, endTime.min, endTime.sec);
 sprintf(mysqlStr, "SELECT count(1) from %s t1 left join %s t2 ON t1.name = t2.name and t1.time < t2.time where t2.id is null and (t1.time > '%s' and t1.time < '%s')",
  MYSQL_TABLE_VISITORS, MYSQL_TABLE_VISITORS, startDate, endDate);
 DF_DEBUG("mysqlStr = %s\n",mysqlStr);
 MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
 iret = MysqlSelect(my_con, mysqlStr);
 if (KEY_TRUE == iret)
 {
  res_ptr = mysql_store_result(my_con);  //å–å‡ºç»“æžœé›†
  if(res_ptr)
  {               
   j = mysql_num_fields(res_ptr);          
   while((sqlrow = mysql_fetch_row(res_ptr)))
   {
    sum = atoi(sqlrow[0]);
   }

   if (mysql_errno(my_con)) {                      
    error_quit("Retrive error", my_con);
   }   
   mysql_free_result(res_ptr);      
  }
 }
 MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
 return sum; 
}


static int MysqlCtrlGetLastVisitorsInfoByDate(TimeYMD_T startTime, TimeYMD_T endTime,int page,int num, VIPCustomer_T *info)
{
	int i = 0, j = 0;
	int iret = -1;
	char mysqlStr[512] = {0};
	char startDate[128] = {0};
	char endDate[128] = {0};
	MYSQL *my_con;
	MYSQL_RES *res_ptr;  
	MYSQL_ROW sqlrow;
	my_con = GetMySqlHandle()->handle;
	 sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", startTime.year,startTime.month,startTime.day, startTime.hour, startTime.min, startTime.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", endTime.year,endTime.month,endTime.day, endTime.hour, endTime.min, endTime.sec);
	sprintf(mysqlStr, "SELECT t1.* from %s t1 left join %s t2 ON t1.name = t2.name and t1.time < t2.time where t2.id is null and (t1.time > '%s' and t1.time < '%s') order by time desc limit %d,%d;",
	MYSQL_TABLE_VISITORS, MYSQL_TABLE_VISITORS, startDate, endDate,((page -1) * num), num);
	DF_DEBUG("mysqlStr = %s\n",mysqlStr);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);

	iret = MysqlSelect(my_con, mysqlStr);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_con);  //È¡³ö½á¹û¼¯
		if(res_ptr)
		{               
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{	
				snprintf(info[i/j].snapToken, 128, "%s", sqlrow[0]);
				snprintf(info[i/j].name, 64, "%s", sqlrow[1]);

				if (0 < strlen(sqlrow[4]))
				{
				 	sscanf(sqlrow[4], "%4d-%2d-%2d %2d:%2d:%2d",
				    &info[i/j].snapTime.year, &info[i/j].snapTime.month, &info[i/j].snapTime.day,
				    &info[i/j].snapTime.hour, &info[i/j].snapTime.min, &info[i/j].snapTime.sec);
				}
					info[i/j].GradeScore = atoi(sqlrow[5]);
					info[i/j].StoreFrequency = atoi(sqlrow[6]);
					snprintf(info[i/j].plateImagePath, 128, "%s", sqlrow[7]);
					snprintf(info[i/j].snapImagePath, 128, "%s", sqlrow[8]);
					i = i + j;
			}

			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
			}   
			mysql_free_result(res_ptr);      
		}
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE; 
}


static int MysqlCtrlGetFirstAttenddanceInfoSumByDate(TimeYMD_T startTime, TimeYMD_T endTime)
{
 int i = 0, j = 0;
 int iret = -1;
 int sum = 0;
 char mysqlStr[512] = {0};
 char startDate[128] = {0};
 char endDate[128] = {0};
 MYSQL *my_con;
 MYSQL_RES *res_ptr;  
 MYSQL_ROW sqlrow;
 
 my_con = GetMySqlHandle()->handle;
 sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", startTime.year,startTime.month,startTime.day, startTime.hour, startTime.min, startTime.sec);
 sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", endTime.year,endTime.month,endTime.day, endTime.hour, endTime.min, endTime.sec);

 sprintf(mysqlStr, "SELECT count(1) FROM(SELECT T1.* FROM(SELECT * FROM %s WHERE time > '%s' AND time < '%s') AS T1 LEFT JOIN (SELECT * FROM %s WHERE time > '%s' AND time < '%s' )  T2 ON T1. NAME = T2. NAME AND T1.time > T2.time WHERE T2. NAME IS NULL)AS result;",
  MYSQL_TABLE_ATTENDDANCE,startDate, endDate, MYSQL_TABLE_ATTENDDANCE, startDate, endDate);

 


 printf("sql = %s\n",mysqlStr);
 MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
 iret = MysqlSelect(my_con, mysqlStr);
 if (KEY_TRUE == iret)
 {
  res_ptr = mysql_store_result(my_con);  //å–å‡ºç»“æžœé›†
  if(res_ptr)
  {               
   j = mysql_num_fields(res_ptr);          
   while((sqlrow = mysql_fetch_row(res_ptr)))
   {
    sum = atoi(sqlrow[0]);
   }

   if (mysql_errno(my_con)) {                      
    error_quit("Retrive error", my_con);
   }   
   mysql_free_result(res_ptr);      
  }
 }
 MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
 return sum; 
}


static int MysqlCtrlGetFirstAttenddanceInfoByDate(TimeYMD_T startTime, TimeYMD_T endTime,int page,int num, AttendanceAtt_T *info)
{
	int i = 0, j = 0;
	int iret = -1;
	char mysqlStr[512] = {0};
	char startDate[128] = {0};
	char endDate[128] = {0};
	MYSQL *my_con;
	MYSQL_RES *res_ptr;  
	MYSQL_ROW sqlrow;
	my_con = GetMySqlHandle()->handle;
	 sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", startTime.year,startTime.month,startTime.day, startTime.hour, startTime.min, startTime.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", endTime.year,endTime.month,endTime.day, endTime.hour, endTime.min, endTime.sec);
	sprintf(mysqlStr, "SELECT T1.* FROM(SELECT * FROM %s WHERE time > '%s' AND time < '%s') AS T1 LEFT JOIN (SELECT * FROM %s WHERE time > '%s' AND time < '%s' ) T2 ON T1. NAME = T2. NAME AND T1.time > T2.time WHERE T2. NAME IS NULL order by time desc limit %d,%d;",
	MYSQL_TABLE_ATTENDDANCE, startDate, endDate,MYSQL_TABLE_ATTENDDANCE, startDate, endDate,((page -1) * num),num);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	printf("sql=%s\n",mysqlStr);
	iret = MysqlSelect(my_con, mysqlStr);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_con);  //È¡³ö½á¹û¼¯
		if(res_ptr)
		{               
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
	
				snprintf(info[i/j].snapToken, 128, "%s", sqlrow[0]);
				snprintf(info[i/j].name, 64, "%s", sqlrow[1]);
				snprintf(info[i/j].usrid, 128, "%s", sqlrow[2]);
				snprintf(info[i/j].phonenum, 12, "%s", sqlrow[3]);
				if (0 < strlen(sqlrow[5]))
				{
				 	sscanf(sqlrow[5], "%4d-%2d-%2d %2d:%2d:%2d",
				    &info[i/j].snapTime.year, &info[i/j].snapTime.month, &info[i/j].snapTime.day,
				    &info[i/j].snapTime.hour, &info[i/j].snapTime.min, &info[i/j].snapTime.sec);
				}
					snprintf(info[i/j].plateImagePath, 128, "%s", sqlrow[6]);
					snprintf(info[i/j].snapImagePath, 128, "%s", sqlrow[7]);
					i = i + j;
			}

			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
			}   
			mysql_free_result(res_ptr);      
		}
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE; 
}

static int MySqlCtrlGetLatestAttendanceInfo(char *usrId, AttendanceAtt_T *info)
{
	int i = 0, j = 0;
	int iret = -1;
	char mysqlStr[512] = {0};
	char UsrName[128] = {0};
	MYSQL *my_con;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow; 
	if (0 == strlen(usrId)){
		return KEY_FALSE;
	}
	
	my_con = GetMySqlHandle()->handle;
	sprintf(mysqlStr, "select * FROM %s where time = (SELECT max(%s.time) from %s where number = '%s');",
		MYSQL_TABLE_ATTENDDANCE, MYSQL_TABLE_ATTENDDANCE, MYSQL_TABLE_ATTENDDANCE, usrId);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);	
	iret = MysqlSelect(my_con, mysqlStr);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_con);  //取出结果集
		if(res_ptr)
		{ 
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				snprintf(info->snapToken, 128, "%s", sqlrow[0]);
				snprintf(info->name, 64, "%s", sqlrow[1]);
				snprintf(info->usrid, 128, "%s", sqlrow[2]);
				snprintf(info->phonenum, 12, "%s", sqlrow[3]);
				if (0 < strlen(sqlrow[5]))
				{
					sscanf(sqlrow[5], "%4d-%2d-%2d %2d:%2d:%2d",
								&info->snapTime.year, &info->snapTime.month, &info->snapTime.day,
								&info->snapTime.hour, &info->snapTime.min, &info->snapTime.sec);
				}
				snprintf(info->plateImagePath, 128, "%s", sqlrow[6]);
				snprintf(info->snapImagePath, 128, "%s", sqlrow[7]);
				i = i + j;
			}

			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
			}   
			mysql_free_result(res_ptr);     	
		}
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;	
}

static int MySqlCtrlAddVisitorsInfo(VIPCustomer_T *info)
{	
	int iret = -1, num = 0;
	MYSQL *my_handle;
	char table[128] = {0};
	char dateTime[128] = {0};
	my_handle = GetMySqlHandle()->handle;
	char strMysql[1024] = {0};
	if (NULL == info)
	{
		return KEY_FALSE;
	}
	memset(strMysql, 0, sizeof(strMysql));
	strcpy(table, (char *)MYSQL_TABLE_VISITORS);
	snprintf(dateTime, 128, "%04d-%02d-%02d %02d:%02d:%02d", info->snapTime.year, info->snapTime.month, info->snapTime.day, info->snapTime.hour, info->snapTime.min, info->snapTime.sec);
   	snprintf(strMysql, 1024,"insert into %s(id,name,number,remarks,time,gradescore,frequency,plateImagePath,snapImagePath) value('%s','%s','%s','%s','%s','%d','%d','%s','%s');", 
		table, info->snapToken, info->name, info->snapToken, "", dateTime, info->GradeScore,info->StoreFrequency,info->plateImagePath,info->snapImagePath);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlExecute(my_handle, strMysql);
    if( iret != 0)
    {
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_FALSE;
    }
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MySqlCtrlGetVisitorsListSum()
{	
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[1024] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	snprintf(strMysql, 1024, "select count(1) from %s;", MYSQL_TABLE_VISITORS);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{        
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				sum = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MySqlCtrlGetVisitorsList(int startPage, int num, VIPCustomer_T *info)
{	
	int i = 0, j = 0;
	int iret = -1;
	char mysqlStr[512] = {0};
	MYSQL *my_con;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow; 
	if (NULL == info){
		return KEY_FALSE;
	}
	
	my_con = GetMySqlHandle()->handle;
	sprintf(mysqlStr, "select * from %s order by time desc limit %d,%d;",
		MYSQL_TABLE_VISITORS, startPage, num);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_con, mysqlStr);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_con);  //取出结果集
		if(res_ptr)
		{     
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				snprintf(info->snapToken, 128, "%s", sqlrow[0]);
				snprintf(info->name, 64, "%s", sqlrow[1]);
				if (0 < strlen(sqlrow[4]))
				{
					sscanf(sqlrow[4], "%4d-%2d-%2d %2d:%2d:%2d",
								&info->snapTime.year, &info->snapTime.month, &info->snapTime.day,
								&info->snapTime.hour, &info->snapTime.min, &info->snapTime.sec);
				}
				info->GradeScore = atoi(sqlrow[5]);
				info->StoreFrequency = atoi(sqlrow[6]);
				snprintf(info->plateImagePath, 128, "%s", sqlrow[7]);
				snprintf(info->snapImagePath, 128, "%s", sqlrow[8]);
				i = i + j;
			}

			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
			}   
			mysql_free_result(res_ptr);     	
		}
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return iret;
}

static int MySqlCtrlGetLatestVisitorsInfo(char *name, VIPCustomer_T *info)
{
	int i = 0, j = 0;
	int iret = -1;
	char mysqlStr[512] = {0};
	char UsrName[128] = {0};
	MYSQL *my_con;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow; 
	if (0 == strlen(name)){
		return KEY_FALSE;
	}
	
	my_con = GetMySqlHandle()->handle;
	sprintf(mysqlStr, "select * FROM %s where time = (SELECT max(%s.time) from %s where name = '%s');",
		MYSQL_TABLE_VISITORS, MYSQL_TABLE_VISITORS, MYSQL_TABLE_VISITORS, name);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_con, mysqlStr);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_con);  //取出结果集
		if(res_ptr)
		{             

			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				snprintf(info->snapToken, 128, "%s", sqlrow[0]);
				snprintf(info->name, 64, "%s", sqlrow[1]);
				if (0 < strlen(sqlrow[4]))
				{
					sscanf(sqlrow[4], "%4d-%2d-%2d %2d:%2d:%2d",
								&info->snapTime.year, &info->snapTime.month, &info->snapTime.day,
								&info->snapTime.hour, &info->snapTime.min, &info->snapTime.sec);
				}
				info->GradeScore = atoi(sqlrow[5]);
				info->StoreFrequency = atoi(sqlrow[6]);
				snprintf(info->plateImagePath, 128, "%s", sqlrow[7]);
				snprintf(info->snapImagePath, 128, "%s", sqlrow[8]);
				i = i + j;
			}

			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
			}   
			mysql_free_result(res_ptr);     	
		}
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;	
}

static int MySqlCtrlAddPassengerFlowInfo(TimeYMD_T time, VolumeOfCommuters_T *info)
{
	int iret = -1, num = 0;
	MYSQL *my_handle;
	char table[128] = {0};
	char dateTime[128] = {0};
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	if (NULL == info)
	{
		return KEY_FALSE;
	}
	memset(strMysql, 0, sizeof(strMysql));
	strcpy(table, (char *)MYSQL_TABLE_PASSENGERFLOW);
	snprintf(dateTime, 128, "%04d-%02d-%02d %02d:%02d:%02d", time.year, time.month, time.day, time.hour, time.min, time.sec);
   	sprintf(strMysql, "insert into %s(id,enter,male,female,kid,young,old,time) value('%s','%d','%d','%d','%d','%d','%d','%s');", \
		table, info->snapToken, info->LeaveAndEnterAttr.dwEnterNum,info->SexAttr.male, info->SexAttr.female, info->AgeAttr.kid, info->AgeAttr.young, info->AgeAttr.old, dateTime);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlExecute(my_handle, strMysql);
    if( iret != 0)
    {
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_FALSE;
    }
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return iret;
}

static int MySqlCtrlGetPassengerFlowListSum(TimeYMD_T starttime)
{	
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strMysql, "SELECT SUM(enter) FROM %s WHERE DATE_FORMAT(time,'%Y-%%m-%%d') ='%04d-%02d-%02d';", MYSQL_TABLE_PASSENGERFLOW,starttime.year,starttime.month,starttime.day);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{        
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				sum = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}


static int MySqlCtrlGetPassengerFlowLineOfHour(TimeYMD_T startTime,TimeYMD_T endTime,int num,LeaveAndEnterNum_T *info)
{	
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	int i = 0,j = 0;
	char startDate[128] = {0}; 
	char endDate[128] = {0};
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	sprintf(startDate, "%04d-%02d-%02d", startTime.year,startTime.month,startTime.day);
	sprintf(endDate, "%04d-%02d-%02d", endTime.year,endTime.month,endTime.day);
	memset(strMysql, 0, sizeof(strMysql));
	
	sprintf(strMysql, "SELECT sum(enter) AS count,DATE_FORMAT(time, '%Y-%%m-%%d %H') AS time FROM %s where DATE_FORMAT(time, '%Y-%%m-%%d')='%s' GROUP BY time;", MYSQL_TABLE_PASSENGERFLOW,startDate);
	//printf("strMysql = %s\n",strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{        
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr))){
				if(NULL == sqlrow[0])
				{
					break;
				}
			    info[i/j].dwEnterNum = atoi(sqlrow[0]);
				
				sprintf(info[i/j].time,"%s:00",sqlrow[1]);
				//printf("%d\n",i/j);
				//printf("dwEnterNum = %d\n",info[i/j].dwEnterNum);
				//printf("time = %s\n",info[i/j].time);
				i = i + j;
				sum++;
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MySqlCtrlGetPassengerFlowLineOfDay(TimeYMD_T startTime,TimeYMD_T endTime,int num,LeaveAndEnterNum_T *info)
{	
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	int i = 0,j = 0;
	char startDate[128] = {0}; 
	char endDate[128] = {0};
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
		
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", startTime.year,startTime.month,startTime.day, startTime.hour, startTime.min, startTime.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", endTime.year,endTime.month,endTime.day, endTime.hour, endTime.min, endTime.sec);
	memset(strMysql, 0, sizeof(strMysql));	
	sprintf(strMysql, "SELECT sum(enter) AS count,DATE_FORMAT(time, '%Y-%%m-%%d') AS time FROM %s where time>'%s' and time<='%s' GROUP BY DATE_FORMAT(time, '%Y-%%m-%%d');", MYSQL_TABLE_PASSENGERFLOW,startDate,endDate);
	//printf("strMysql = %s\n",strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{        
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr))){
				if(NULL == sqlrow[0])
				{
					break;
				}
			    info[i/j].dwEnterNum = atoi(sqlrow[0]);
				snprintf(info[i/j].time, 128, "%s", sqlrow[1]);
				//printf("%s\n",info[i/j].time);
				i = i + j;
				sum++;
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MySqlCtrlGetPassengerFlowSumOfSex(TimeYMD_T startTime,TimeYMD_T endTime,int num,VolumeOfCommutersOfSex_T *info)
{	
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	int i = 0,j = 0;
	char startDate[128] = {0}; 
	char endDate[128] = {0};
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", startTime.year,startTime.month,startTime.day, startTime.hour, startTime.min, startTime.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", endTime.year,endTime.month,endTime.day, endTime.hour, endTime.min, endTime.sec);
	memset(strMysql, 0, sizeof(strMysql));
	//printf("startDate=%s %s\n",startDate,endDate);
	sprintf(strMysql, "SELECT sum(male),sum(female) FROM %s where time>'%s' and time<='%s';", MYSQL_TABLE_PASSENGERFLOW,startDate,endDate);
	//printf("strMysql=%s\n",strMysql);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{        
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			        
			while((sqlrow = mysql_fetch_row(res_ptr))){
				if(NULL == sqlrow[0])
				{
					break;
				}
			    info->male = atoi(sqlrow[0]);
				info->female = atoi(sqlrow[1]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MySqlCtrlGetPassengerFlowSumOfAge(TimeYMD_T startTime,TimeYMD_T endTime,int num,VolumeOfCommutersOfAge_T *info)
{	
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	int i = 0,j = 0;
	char startDate[128] = {0}; 
	char endDate[128] = {0};
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	sprintf(startDate, "%04d-%02d-%02d %02d:%02d:%02d", startTime.year,startTime.month,startTime.day, startTime.hour, startTime.min, startTime.sec);
	sprintf(endDate, "%04d-%02d-%02d %02d:%02d:%02d", endTime.year,endTime.month,endTime.day, endTime.hour, endTime.min, endTime.sec);
	memset(strMysql, 0, sizeof(strMysql));	
	sprintf(strMysql, "SELECT sum(kid),sum(young),sum(old) FROM %s where time>'%s' and time<='%s';", MYSQL_TABLE_PASSENGERFLOW,startDate,endDate);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{       
			if(mysql_num_rows(res_ptr) == 0)
			{
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}  
			while((sqlrow = mysql_fetch_row(res_ptr))){
				if(NULL == sqlrow[0])
				{
					break;
				}

				info->kid = atoi(sqlrow[0]);
				info->young = atoi(sqlrow[1]);
				info->old = atoi(sqlrow[2]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MySqlCtrlGetPassengerFlowList(int startPage, int num, VolumeOfCommuters_T *info)
{	
	int i = 0, j = 0;
	int iret = -1;
	char mysqlStr[512] = {0};
	MYSQL *my_con;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow; 
	if (NULL == info){
		return KEY_FALSE;
	}
	my_con = GetMySqlHandle()->handle;
	sprintf(mysqlStr, "select * from %s order by time desc limit %d,%d;",
		MYSQL_TABLE_PASSENGERFLOW, startPage, num);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_con, mysqlStr);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_con);  //取出结果集
		if(res_ptr)
		{       
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
			#if 0
				snprintf(info->snapToken, 128, "%s", sqlrow[0]);
				info->sexGroup = atoi(sqlrow[1]);
				info->ageGroup = atoi(sqlrow[2]);
				if (0 < strlen(sqlrow[3]))
				{
					sscanf(sqlrow[3], "%4d-%2d-%2d %2d:%2d:%2d",
								&info->snapTime.year, &info->snapTime.month, &info->snapTime.day,
								&info->snapTime.hour, &info->snapTime.min, &info->snapTime.sec);
				}
			#endif
				i = i + j;
			}

			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
			}   
			mysql_free_result(res_ptr);     	
		}
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return iret;
}

static int MySqlCtrlGetPlatNoAreaById(int AreaId,char * province_name,char * city_name)
{	
	int iret = KEY_FALSE;
	int i = 0, j = 0;
	char mysqlStr[512] = {0};
	MYSQL *my_con;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow; 
	if (NULL == province_name || NULL == city_name || AreaId <= 0){
		return iret;
	}
	my_con = GetMySqlHandle()->handle;
	sprintf(mysqlStr, "select province_name,city_name from %s where id = %d",
		MYSQL_TABLE_PLATNOAREA, AreaId);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_con, mysqlStr);
	if (KEY_TRUE == iret){
		res_ptr = mysql_store_result(my_con);  //取出结果集
		if(res_ptr){
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr))){
				snprintf(province_name, 32, "%s", sqlrow[0]);
				snprintf(city_name, 32, "%s", sqlrow[1]);
				i = i + j;
			}
			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
			}  
			mysql_free_result(res_ptr);
		}
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return iret;
}

static int MySqlCtrlGetPlatNoAreaId(char * province_code, char *city_code,int * AreaId)
{	
	int iret = KEY_FALSE;
	int i = 0, j = 0;
	char mysqlStr[512] = {0};
	MYSQL *my_con;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow; 
	if (NULL == province_code || NULL == city_code || NULL == AreaId){
		return iret;
	}
	my_con = GetMySqlHandle()->handle;
	sprintf(mysqlStr, "SELECT id FROM %s WHERE province_code = '%s' AND city_code = '%s';", \
		MYSQL_TABLE_PLATNOAREA, province_code,city_code);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	
	iret = MysqlSelect(my_con, mysqlStr);
	if (KEY_TRUE == iret){
		res_ptr = mysql_store_result(my_con);  //取出结果集
		if(res_ptr){
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr))){
				*AreaId = atoi(sqlrow[0]);;
				i = i + j;
			}
			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
			}  
			mysql_free_result(res_ptr);
		}
	}
	
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return iret;
}

static int MySqlCtrlAddStuckPipeData(StuckPipeInfo_T *info)
{
	int iret = -1, num = 0;
	MYSQL *my_handle;
	char table[128] = {0};
	char date[128] = {0};
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	strcpy(table, (char *)MYSQL_TABLE_FUELQUANTITY);
	snprintf(date, 128, "%04d-%02d-%02d %02d:%02d:%02d", info->date.year, info->date.month, info->date.day, info->date.hour, info->date.min, info->date.sec);
   	sprintf(strMysql, "insert into %s(stationName,oils,LPM,number,time) value('%s','%d','%.2f','%d','%s');", 
		table, info->stationName, info->oils, info->LPM, info->number, date);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlExecute(my_handle, strMysql);
    if( iret != 0)
    {
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_FALSE;
    }
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;	
}

static int MysqlDelStuckPipeDataByDate(TimeYMD_T date)
{
	int iret = 0;
	MYSQL *my_handle;
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	char strDate[128] = {0};
	sprintf(strDate, "%04d-%02d", date.year, date.month);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	//再删除主表
	memset(strMysql, 0, sizeof(strMysql));
    sprintf(strMysql, "delete from %s where date_format(time,'%%Y-%%m') = '%s';", 
		MYSQL_TABLE_FUELQUANTITY, strDate);
	iret = MysqlExecute(my_handle, strMysql);
    if( iret != 0 )
    {		
		MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
		return KEY_FALSE;
    }
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return KEY_TRUE;
}

static int MysqlGetFillRateSumByType(TimeYMD_T date, FillRateType_E findType)
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int iret = -1;
	int sum  = 0;
	char strDate[128] = {0};
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strDate, "%04d-%02d", date.year, date.month);
	if (FillRateType_ALL == findType)
	{
		sprintf(strMysql, "select count(1) from %s where date_format(time,'%%Y-%%m') = '%s';", MYSQL_TABLE_FUELQUANTITY, strDate);
	}
	else if (FillRateType_LOW == findType)
	{
		sprintf(strMysql, "select count(1) from %s where date_format(time,'%%Y-%%m') = '%s' and LPM <= 10.00;", MYSQL_TABLE_FUELQUANTITY, strDate);
	}
	else if (FillRateType_MIDDLE == findType)
	{
		sprintf(strMysql, "select count(1) from %s where date_format(time,'%%Y-%%m') = '%s' and (LPM > 10 and LPM < 30.00);", MYSQL_TABLE_FUELQUANTITY, strDate);
	}
	else if (FillRateType_HIGH == findType)
	{
		sprintf(strMysql, "select count(1) from %s where date_format(time,'%%Y-%%m') = '%s' and LPM >= 30.00;", MYSQL_TABLE_FUELQUANTITY, strDate);
	}
	else
	{
		return -1;
	}
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{        
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				sum = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MysqlGetFuelQuantityByType(TimeYMD_T date, FuelQuantityType_E findType, int *quantity)
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int iret = -1;
	char strDate[128] = {0};
	my_handle = GetMySqlHandle()->handle;
	char strMysql[512] = {0};
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strDate, "%04d-%02d", date.year, date.month);
	if (FillQuantityType_92 == findType)
	{
		sprintf(strMysql, "select sum(LPM) from %s where oils = '92' and date_format(time,'%%Y-%%m') = '%s';", MYSQL_TABLE_FUELQUANTITY, strDate);
	}
	else if (FillQuantityType_95 == findType)
	{
		sprintf(strMysql, "select sum(LPM) from %s where oils = '95' and date_format(time,'%%Y-%%m') = '%s';", MYSQL_TABLE_FUELQUANTITY, strDate);
	}
	else if (FillQuantityType_98 == findType)
	{
		sprintf(strMysql, "select sum(LPM) from %s where oils = '98' and date_format(time,'%%Y-%%m') = '%s';", MYSQL_TABLE_FUELQUANTITY, strDate);
	}
	else
	{
		return -1;
	}
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{        
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				*quantity = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return iret;
}

static int MysqlGetOliGunSum(TimeYMD_T date)
{
	MYSQL *my_handle;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	int sum  = 0;
	int iret = -1;
	char strDate[128] = {0};
	char strMysql[512] = {0};
	my_handle = GetMySqlHandle()->handle;
	memset(strMysql, 0, sizeof(strMysql));
	sprintf(strDate, "%04d-%02d", date.year, date.month);
	sprintf(strMysql, "select count(distinct number) from %s where date_format(time,'%%Y-%%m') = '%s';", MYSQL_TABLE_FUELQUANTITY, strDate);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_handle, strMysql);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(my_handle);
		if(res_ptr)
		{        
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				sum = atoi(sqlrow[0]);
			}
			if (mysql_errno(my_handle)) {                      
				error_quit("Retrive error", my_handle);
			}        	
		}
		mysql_free_result(res_ptr);
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return sum;
}

static int MysqlGetOliGunFrequency(TimeYMD_T date, OliGunInfo_T *info)
{
	int iret = KEY_FALSE;
	int i = 0, j = 0;
	char mysqlStr[512] = {0};
	char strDate[128] = {0};
	MYSQL *my_con;
    MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow; 
	sprintf(strDate, "%04d-%02d", date.year, date.month);
	my_con = GetMySqlHandle()->handle;
	sprintf(mysqlStr, "select number,count(number) from %s where date_format(time,'%%Y-%%m') = '%s' group by number;",
		MYSQL_TABLE_FUELQUANTITY, strDate);
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	iret = MysqlSelect(my_con, mysqlStr);
	if (KEY_TRUE == iret){
		res_ptr = mysql_store_result(my_con);  //取出结果集
		if(res_ptr){
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return KEY_FALSE;
			}
			j = mysql_num_fields(res_ptr);          
			while((sqlrow = mysql_fetch_row(res_ptr))){
			    info[i/j].number = atoi(sqlrow[0]);
			    info[i/j].frequency = atoi(sqlrow[1]);
				i = i + j;
			}
			if (mysql_errno(my_con)) {                      
				error_quit("Retrive error", my_con);
			}  
			mysql_free_result(res_ptr);
		}
	}
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	return iret;	
}

static Tab_Info_T *GetTableInfo(int * num)
{
	int i = 0,j = 0;
	MYSQL_RES *res_ptr;  
    MYSQL_ROW sqlrow;
	char gettabinfocmd[256] = {0};
	Tab_Info_T * table_info;
	MUTEX_LOCK(&GetMySqlHandle()->selfMutex);
	snprintf(gettabinfocmd,sizeof(gettabinfocmd),"desc %s;",MYSQL_TABLE_CAMERA);
	int iret = MysqlSelect(GetMySqlHandle()->handle, gettabinfocmd);
	if (KEY_TRUE == iret)
	{
		res_ptr = mysql_store_result(GetMySqlHandle()->handle);  //取出结果集
		if(res_ptr)
		{          
			if(mysql_num_rows(res_ptr) == 0){
				mysql_free_result(res_ptr);
				MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
				return NULL;
			}
			j = mysql_num_fields(res_ptr);
			(*num) = mysql_num_rows(res_ptr);
			table_info = (Tab_Info_T *)malloc((*num)*sizeof(Tab_Info_T));
			while((sqlrow = mysql_fetch_row(res_ptr))) //循环8次
			{	
				snprintf(table_info[i/j].col_name, 128, "%s", sqlrow[0]);
				snprintf(table_info[i/j].data_type, 128, "%s", sqlrow[1]);
				i = i + j;
			}
			if (mysql_errno(GetMySqlHandle()->handle)) {                      
				error_quit("Retrive error", GetMySqlHandle()->handle);
			}
		}
		else{
			MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
			return NULL;	
		}
		mysql_free_result(res_ptr);
	}
	
	MUTEX_UNLOCK(&GetMySqlHandle()->selfMutex);
	DF_DEBUG("GetTableInfo suc");
	return table_info;
}

static void *MysqlBackUpThread(void *argv)
{
	char szcmd[256] = {0};
	//获取表结构信息
	int num = 0;
	int row_num1 = 0,row_num2 = 0;
	int iret = KEY_FALSE;
	Tab_Info_T *tabinfo ;
	tabinfo = GetTableInfo(&row_num1);
	if( tabinfo == NULL)
	{
		return NULL;
	}
	int x = 0;
	system("mkdir ./backup");
	while(backuprun)
	{
		Tab_Info_T *info ;
		info = GetTableInfo(&row_num2);
		if(info == NULL)
		{
			return NULL;
		}

		if(row_num1 != row_num2)
		{
			return NULL;
		}
		int i ;
		for (i = 0;i<row_num2;i++)
		{
			//DF_DEBUG("tabinfo[%d].col_name = %s,tabinfo[%d].col_name = %s",i,tabinfo[i].col_name,i,info[i].col_name);
			iret = strcmp(tabinfo[i].col_name,info[i].col_name);
			if(iret)
			{
				break;
			}
			iret = strcmp(tabinfo[i].data_type,info[i].data_type);
			if(iret)
			{
				break;
			}
		}
		free(info);
		if(iret)
		{
			return NULL;
		}
	
		//延时判断
		long cur_time = 0;
		long next_time = 0;
		GetUpTime(&next_time);
		next_time += MYSQL_BACKUP_TIME;		
		for (;backuprun != ENUM_ENABLED_FALSE;)
		{
			GetUpTime(&cur_time);
			if (cur_time > next_time)
			{
				snprintf(szcmd,sizeof(szcmd),"mysqldump -u%s -p%s -h %s --default-character-set=utf8 %s > ./backup/backup.sql",
				MYSQL_SERVER_USR,MYSQL_SERVER_PASSWD,MYSQL_SERVER_ADDRESS,MYSQL_DB_NAME);
				system(szcmd);
				break;
			}
			usleep(1 * 1000 * 1000);
		}
	}
	free(tabinfo);
	return NULL;
}

static int MysqlBackUpStartTask()
{
	pthread_t MysqlBackUpTaskID;
	backuprun = ENUM_ENABLED_TRUE;
	if (pthread_create(&MysqlBackUpTaskID, 0, MysqlBackUpThread, (void *)NULL))
	{
		DF_DEBUG("MysqlBackUpStartTask is err \n");
		return KEY_FALSE;
	}
	pthread_detach(MysqlBackUpTaskID);
	return KEY_TRUE;
}

int MysqlInitDefaultUsr(UsrInfo_T *pinfo)
{
	int num = 0;
	snprintf(pinfo[num].registInfo.usrname, DF_MAXLEN_USRNAME + 1, "%s", "gzroot@u");
	snprintf(pinfo[num].registInfo.passWord, DF_MAXLEN_USRNAME + 1, "%s", "gzroot@p");
	pinfo[num].registInfo.userLevel = ENUM_USRLEVEL_ROOT;
	pinfo[num].registInfo.usrID = 10000;
	num ++;

	snprintf(pinfo[num].registInfo.usrname, DF_MAXLEN_USRNAME + 1, "%s", "gzadmin@u");
	snprintf(pinfo[num].registInfo.passWord, DF_MAXLEN_USRNAME + 1, "%s", "gzadmin@p");
	pinfo[num].registInfo.userLevel = ENUM_USRLEVEL_ROOT;
	pinfo[num].registInfo.usrID = 10001;
	num ++;

	snprintf(pinfo[num].registInfo.usrname, DF_MAXLEN_USRNAME + 1, "%s", "gzregister@u");
	snprintf(pinfo[num].registInfo.passWord, DF_MAXLEN_USRNAME + 1, "%s", "gzregister@p");
	pinfo[num].registInfo.userLevel = ENUM_USRLEVEL_ROOT;
	pinfo[num].registInfo.usrID = 10002;
	num ++;

	snprintf(pinfo[num].registInfo.usrname, DF_MAXLEN_USRNAME + 1, "%s", "gzdevice@u");
	snprintf(pinfo[num].registInfo.passWord, DF_MAXLEN_USRNAME + 1, "%s", "gzdevice@p");
	pinfo[num].registInfo.userLevel = ENUM_USRLEVEL_ROOT;
	pinfo[num].registInfo.usrID = 10003;
	num ++;
	return num;
}

int MySqlManageRegister(MySqlManage_T *mysqlHandle)
{
	if (NULL == mysqlHandle){
		return KEY_FALSE;	
	}
	//用户信息属性
	mysqlHandle->pAddUsrInfo = MySqlCtrlAddUsrInfo;
	mysqlHandle->pDelUsrInfo = MySqlCtrlDelUsrInfo;
	mysqlHandle->pGetUsrInfo = MySqlCtrlGetUsrInfo;
	mysqlHandle->pModifyUsrBindInfocb = MySqlCtrlModifyUsrBindInfo;
	mysqlHandle->pGetUsrInfoList = MySqlCtrlGetUsrInfoList;
	mysqlHandle->pGetUsrInfoListNum = MySqlCtrlGetUsrInfoListNum;
	mysqlHandle->pSetUsrInfo = MySqlCtrlSetUsrInfo;	
	//设备信息属性
	mysqlHandle->pAddDevice = MySqlCtrlAddDevice;
	mysqlHandle->pDelDevice = MySqlCtrlDelDevice;
	mysqlHandle->pGetDeviceInfo = MySqlCtrlGetDeviceInfo;
	mysqlHandle->pSetDeviceInfo = MySqlCtrlSetDeviceInfo;
	mysqlHandle->pGetDeviceInfoListNum = MySqlCtrlGetDeviceInfoListNum;
	mysqlHandle->pGetDeviceInfoList = MySqlCtrlGetDeviceInfoList;
	//车辆数据
	//临街数据
	mysqlHandle->pAddFrontageInfo = MySqlCtrlAddFrontageInfo;
	mysqlHandle->pGetFrontageListSum = MySqlCtrlGetFrontageListSum;
	mysqlHandle->pGetFrontageListSumByTime = MySqlCtrlGetFrontageListSumByTime;
	mysqlHandle->pGetFrontageList = MySqlCtrlGetFrontageList;
	mysqlHandle->pGetFrontageListByTime = MySqlCtrlGetFrontageListByTime;
	mysqlHandle->pGetLatestFrontageInfo = MySqlCtrlGetLatestFrontageInfo;
	mysqlHandle->pSetFrontageInfo = MysqlCtrlUpdateFrontageInfo;
	mysqlHandle->pGetFrontageListSumBySearchtype = MySqlCtrlGetFrontageListSumBySearchtype;


	//出入口数据
	mysqlHandle->pAddEntranceInfo = MySqlCtrlAddEntranceInfo;
	mysqlHandle->pGetEntranceListSum = MySqlCtrlGetEntranceListSum;
	mysqlHandle->pGetEntranceListSumByTime = MySqlCtrlGetEntranceListSumByTime;
	mysqlHandle->pGetEntranceListSumBySerchType = MySqlCtrlGetEntranceListSumBySerchType;
	mysqlHandle->pGetEntranceListSumBySerchType2= MySqlCtrlGetEntranceListSumBySerchType2;

	
	mysqlHandle->pGetEntranceList = MySqlCtrlGetEntranceList;
	mysqlHandle->pGetEntranceListByTime = MySqlCtrlGetEntranceListByTime;
	mysqlHandle->pGetEntranceListBySerchType = MySqlCtrlGetEntranceListBySerchType;
	mysqlHandle->pGetEntranceListBySerchType2 = MySqlCtrlGetEntranceListBySerchType2;
	mysqlHandle->pGetLatestEntranceInfo = MySqlCtrlGetLatestEntranceInfo;
	mysqlHandle->pSetEntranceInfo = MysqlCtrlUpdateEntranceInfo;
	mysqlHandle->pGetEntranceListSumByLocation = MySqlCtrlGetEntranceListSumByLocation;
	mysqlHandle->pGetEntranceResidenceByTime = MySqlCtrlGetEntranceResidenceByTime;
	mysqlHandle->pGetEntranceRefuelingEfficiency = MySqlCtrlGetEntranceRefuelingEfficiency;
	mysqlHandle->pGetEntranceResidenceListInfo = MySqlCtrlGetEntranceResidenceListInfo;

	
	
	//异常告警数据
	mysqlHandle->pAddAlarmInfo = MySqlCtrlAddAlarmInfo;
	mysqlHandle->pGetAlarmListSum = MySqlCtrlGetAlarmListSum;
	mysqlHandle->pGetAlarmList = MySqlCtrlGetAlarmList;
	mysqlHandle->pGetAlarmListBySearchType = MySqlCtrlGetAlarmListBySearchType;
	mysqlHandle->pGetAlarmListSumBySearchType = MySqlCtrlGetAlarmListSumBySearchType;

	
	//VIP车辆
	mysqlHandle->pAddVipCarInfo = MySqlCtrlAddVipCarInfo;
	mysqlHandle->pGetVipCarListSum = MySqlCtrlGetVipCarListSum;
	mysqlHandle->pGetVipCarList = MySqlCtrlGetVipCarList;
	mysqlHandle->pGetVipCarInfoByPlateNo = MysqlCtrGetVipCarInfoByPlateNo;
	mysqlHandle->pSetVipCarInfo = MysqlCtrUpdateVipCarInfo;
	mysqlHandle->pGetVipCarListBySerchType = MySqlCtrlGetVipCarListBySerchType;
	mysqlHandle->pGetVipCarListSumBySerchType = MySqlCtrlGetVipCarListSumBySerchType;

	//洗车车辆
	mysqlHandle->pAddWashCarInfo = MySqlCtrlAddWashCarInfo;
	mysqlHandle->pGetWashCarListSumByTime = MySqlCtrlWashCarListSumByTime;
	mysqlHandle->pGetLastWashCarInfo = MySqlCtrlGetLastWashCarInfo;
	mysqlHandle->pSetWashCarInfo = MySqlCtrlSetWashCarInfo;

	//车辆轨迹
	mysqlHandle->pAddTrajectoryInfo = MySqlCtrlAddTrajectoryInfo;
	mysqlHandle->pGetTrajectoryInfo = MySqlCtrlGetTrajectoryInfo;
	mysqlHandle->pUpdateTrajectoryInfo = MysqlCtrlUpdateTrajectoryInfo;
	mysqlHandle->pGetaddWithoutWashNum = MysqlCtrlGetaddWithoutWashNum;
	mysqlHandle->pGetWashWithoutaddNum = MysqlCtrlGetWashWithoutaddNum;
	mysqlHandle->pGetaddAndwashNum	   = MysqlCtrlGetaddAndwashNum;
	mysqlHandle->pGetwashAndaddNum	   = MysqlCtrlGetwashAndaddNum;


	//加油岛车辆
	mysqlHandle->pAddIsLandInfo = MySqlCtrlAddIsLandInfo;
	mysqlHandle->pGetIsLandListSumByTime = MySqlCtrlGetIsLandListSumByTime;
	mysqlHandle->pGetLastIsLandInfo = MySqlCtrlGetLastIsLandInfo;
	mysqlHandle->pSetIsLandInfo = MySqlCtrlSetIsLandInfo;

	//店内数据分析
	//无感考勤
	mysqlHandle->pAddAttendanceInfo = MySqlCtrlAddAttendanceInfo;
	mysqlHandle->pGetAttendanceHistorySum = MySqlCtrlGetAttendanceHistorySum;
	mysqlHandle->pGetAttendanceHistoryList = MySqlCtrlGetAttendanceHistoryBySearchType;
	mysqlHandle->pGetLatestAttendanceInfo = MySqlCtrlGetLatestAttendanceInfo;
	mysqlHandle->pGetFirstAttenddanceInfoByDate = MysqlCtrlGetFirstAttenddanceInfoByDate;
	mysqlHandle->pGetFirstAttenddanceInfoSumByDate = MysqlCtrlGetFirstAttenddanceInfoSumByDate;
	mysqlHandle->pGetLastVisitorsInfoByDate = MysqlCtrlGetLastVisitorsInfoByDate;
	mysqlHandle->pGetLastVisitorsInfoSumByDate = MysqlCtrlGetLastVisitorsInfoSumByDate;
	mysqlHandle->pAddVisitorsInfo = MySqlCtrlAddVisitorsInfo;
	mysqlHandle->pGetVisitorsListSum = MySqlCtrlGetVisitorsListSum;
	mysqlHandle->pGetVisitorsList = MySqlCtrlGetVisitorsList;
	mysqlHandle->pGetLatestVisitorsInfo = MySqlCtrlGetLatestVisitorsInfo;
	//入店客流
	mysqlHandle->pAddPassengerFlowInfo = MySqlCtrlAddPassengerFlowInfo;
	mysqlHandle->pGetPassengerFlowListSum = MySqlCtrlGetPassengerFlowListSum;
	mysqlHandle->pGetPassengerFlowList = MySqlCtrlGetPassengerFlowList;
	mysqlHandle->pGetPassengerFlowLineOfHour = MySqlCtrlGetPassengerFlowLineOfHour;
	mysqlHandle->pGetPassengerFlowLineOfDay = MySqlCtrlGetPassengerFlowLineOfDay;
	mysqlHandle->pGetPassengerFlowSumOfSex = MySqlCtrlGetPassengerFlowSumOfSex;
	mysqlHandle->pGetPassengerFlowSumOfAge = MySqlCtrlGetPassengerFlowSumOfAge;
	//车牌信息查找
	mysqlHandle->pGetPlatNoAreaById = MySqlCtrlGetPlatNoAreaById;
	mysqlHandle->pGetPlatNoAreaId = MySqlCtrlGetPlatNoAreaId;
	//加满率
	mysqlHandle->pAddStuckPipeData = MySqlCtrlAddStuckPipeData;
	mysqlHandle->pDelStuckPipeDataByDate = MysqlDelStuckPipeDataByDate;
	//油品分析
	mysqlHandle->pGetFillRateSumByType = MysqlGetFillRateSumByType;
	mysqlHandle->pGetFuelQuantityByType = MysqlGetFuelQuantityByType;
	//提枪次数分析
	mysqlHandle->pGetOliGunSum = MysqlGetOliGunSum;
	mysqlHandle->pGetOliGunFrequency = MysqlGetOliGunFrequency;
	return KEY_TRUE;
}

int MySqlManageUnRegister(MySqlManage_T *mysqlHandle)
{
	if (NULL == mysqlHandle){
		return KEY_FALSE;	
	}
	//用户信息属性
	mysqlHandle->pAddUsrInfo = NULL;
	mysqlHandle->pDelUsrInfo = NULL;
	mysqlHandle->pGetUsrInfo = NULL;
	mysqlHandle->pModifyUsrBindInfocb = NULL;
	mysqlHandle->pGetUsrInfoList = NULL;
	mysqlHandle->pGetUsrInfoListNum = NULL;
	mysqlHandle->pSetUsrInfo = NULL;	
	//设备信息属性
	mysqlHandle->pAddDevice = NULL;
	mysqlHandle->pDelDevice = NULL;
	mysqlHandle->pGetDeviceInfo = NULL;
	mysqlHandle->pSetDeviceInfo = NULL;
	mysqlHandle->pGetDeviceInfoListNum = NULL;
	mysqlHandle->pGetDeviceInfoList = NULL;
	mysqlHandle->pGetUsrInfoList = NULL;
	mysqlHandle->pGetUsrInfoListNum = NULL;
	//车辆数据
	//临街数据
	mysqlHandle->pAddFrontageInfo = NULL;
	mysqlHandle->pGetFrontageListSum = NULL;
	mysqlHandle->pGetFrontageListSumByTime = NULL;
	mysqlHandle->pGetFrontageList = NULL;
	mysqlHandle->pGetFrontageListByTime = NULL;
	mysqlHandle->pGetLatestFrontageInfo = NULL;
	mysqlHandle->pSetFrontageInfo = NULL;
	mysqlHandle->pGetFrontageListSumBySearchtype = NULL;
	//出入口数据
	mysqlHandle->pAddEntranceInfo = NULL;
	mysqlHandle->pGetEntranceListSum = NULL;
	mysqlHandle->pGetEntranceListSumByTime = NULL;
	mysqlHandle->pGetEntranceListSumBySerchType = NULL;
	mysqlHandle->pGetEntranceList = NULL;
	mysqlHandle->pGetEntranceListByTime = NULL;
	mysqlHandle->pGetEntranceListBySerchType = NULL;
	//mysqlHandle->pGetEntranceListSumBySerchType = MySqlCtrlGetEntranceListSumBySerchType
	mysqlHandle->pGetLatestEntranceInfo = NULL;
	mysqlHandle->pSetEntranceInfo = NULL;
	mysqlHandle->pGetEntranceListSumByLocation = NULL;
	mysqlHandle->pGetEntranceResidenceByTime = NULL;
	mysqlHandle->pGetEntranceRefuelingEfficiency = NULL;


	//异常告警数据
	mysqlHandle->pAddAlarmInfo = NULL;
	mysqlHandle->pGetAlarmListSum = NULL;
	mysqlHandle->pGetAlarmList = NULL;
	mysqlHandle->pGetAlarmListBySearchType = NULL;
	mysqlHandle->pGetAlarmListSumBySearchType = NULL;
	
	//VIP车辆
	mysqlHandle->pAddVipCarInfo = NULL;
	mysqlHandle->pGetVipCarListSum = NULL;
	mysqlHandle->pGetVipCarList = NULL;
	mysqlHandle->pGetVipCarInfoByPlateNo = NULL;
	mysqlHandle->pSetVipCarInfo = NULL;
	mysqlHandle->pGetVipCarListBySerchType = NULL;
	mysqlHandle->pGetVipCarListSumBySerchType = NULL; 

	//洗车车辆
	mysqlHandle->pAddWashCarInfo = NULL;
	mysqlHandle->pGetWashCarListSumByTime = NULL;
	mysqlHandle->pGetLastWashCarInfo = NULL;
	mysqlHandle->pSetVipCarInfo = NULL;

	//车辆轨迹
	mysqlHandle->pAddTrajectoryInfo = NULL;
	mysqlHandle->pGetTrajectoryInfo = NULL;
	mysqlHandle->pUpdateTrajectoryInfo = NULL;
	mysqlHandle->pGetaddWithoutWashNum = NULL;
	mysqlHandle->pGetWashWithoutaddNum = NULL;
	mysqlHandle->pGetaddAndwashNum	   = NULL;
	mysqlHandle->pGetwashAndaddNum	   = NULL;


	//加油岛车辆
	mysqlHandle->pAddIsLandInfo = NULL;
	mysqlHandle->pGetIsLandListSumByTime = NULL;
	mysqlHandle->pGetLastIsLandInfo = NULL;
	mysqlHandle->pSetIsLandInfo = NULL;
	
	//店内数据分析
	//无感考勤
	mysqlHandle->pAddAttendanceInfo = NULL;
	mysqlHandle->pGetAttendanceHistorySum = NULL;
	mysqlHandle->pGetAttendanceHistoryList = NULL;
	mysqlHandle->pGetLatestAttendanceInfo = NULL;
	mysqlHandle->pGetFirstAttenddanceInfoByDate = NULL;
	mysqlHandle->pGetFirstAttenddanceInfoSumByDate =NULL;
	mysqlHandle->pGetLastVisitorsInfoByDate = NULL;
	mysqlHandle->pGetLastVisitorsInfoSumByDate = NULL;
	mysqlHandle->pAddVisitorsInfo = NULL;
	mysqlHandle->pGetVisitorsListSum = NULL;
	mysqlHandle->pGetVisitorsList = NULL;
	mysqlHandle->pGetLatestVisitorsInfo = NULL;
	//入店客流
	mysqlHandle->pAddPassengerFlowInfo = NULL;
	mysqlHandle->pGetPassengerFlowListSum = NULL;
	mysqlHandle->pGetPassengerFlowList = NULL;

	//车牌信息查找
	mysqlHandle->pGetPlatNoAreaById = NULL;
	mysqlHandle->pGetPlatNoAreaId = NULL;

	//加满率
	mysqlHandle->pAddStuckPipeData = NULL;
	mysqlHandle->pDelStuckPipeDataByDate = NULL;
	//油品分析
	mysqlHandle->pGetFillRateSumByType = NULL;
	mysqlHandle->pGetFuelQuantityByType = NULL;
	//提枪次数分析
	mysqlHandle->pGetOliGunSum = NULL;
	mysqlHandle->pGetOliGunFrequency = NULL;
	return KEY_TRUE;
}

static int MySqlInit()
{
	int iret = KEY_FALSE;
	InitMysqlHanld();
	iret = MysqlConnect(GetMySqlHandle(), MYSQL_SERVER_ADDRESS, MYSQL_SERVER_USR, MYSQL_SERVER_PASSWD, MYSQL_DB_NAME); //mysql 初始化
	if (KEY_TRUE == iret)
	{
		iret = MysqlCreateDatabase(MYSQL_DB_NAME);
		if (KEY_TRUE == iret)
		{
			MysqlCreateUsrInfoTable(MYSQL_TABLE_USRINFO);
			MysqlCreateCameraInfoTable(MYSQL_TABLE_CAMERA);
			MysqlCreateFrontageDataTable(MYSQL_TABLE_FRONTAGE);
			MysqlCreateEntrancedataDataTable(MYSQL_TABLE_ENTRANCE);
			//MysqlCreateExitDataTable(MYSQL_TABLE_EXIT);
			MysqlCreateAlarmDataTable(MYSQL_TABLE_ALARM);
			MysqlCreateVipCarDataTable(MYSQL_TABLE_VIPCar);
			MysqlCreateWashCarDataTable(MYSQL_TABLE_WASHCar);
			MysqlCreateCarTrajectoryDataTable(MYSQL_TABLE_CARTRAJECTORY);
			MysqlCreateIslandDataTable(MYSQL_TABLE_ISLAND);
			MysqlCreateAttendanceDataTable(MYSQL_TABLE_ATTENDDANCE);
			MysqlCreateVisitorsDataTable(MYSQL_TABLE_VISITORS);
			MysqlCreatePassengerFlowDataTable(MYSQL_TABLE_PASSENGERFLOW);
			MysqlCreateFuelQuantityDataTable(MYSQL_TABLE_FUELQUANTITY);
			
			//创建root用户
			int i = 0, num = 0;
			UsrInfo_T info[10];
			memset(info, 0, sizeof(UsrInfo_T) * 10);
			num = MysqlInitDefaultUsr(info);
			for (i = 0; i < num; i++){
				MySqlCtrlAddUsrInfo(&info[i]);
			}
		}
		else
		{
			DF_ERROR("DB Create Err!");
			return KEY_FALSE;
		}
	}
	return KEY_TRUE;
}

int MySqlManageInit(MySqlManage_T *mysqlHandle)
{
	if (NULL == mysqlHandle)
	{
		return KEY_FALSE;
	}
	//初始化MySql,并创建数据库与数据表
	MySqlInit();
	MySqlManageRegister(mysqlHandle);
	MysqlBackUpStartTask();
	return KEY_TRUE;
}

int MySqlManageUnInit(MySqlManage_T *mysqlHandle)
{
	if (NULL == mysqlHandle)
	{
		return KEY_FALSE;
	}
	MySqlManageUnRegister(mysqlHandle);
	//注销数据库
	MySqlUnInit();
	return KEY_TRUE;
}



