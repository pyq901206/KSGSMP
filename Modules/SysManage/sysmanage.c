#include "common.h"
#include "timemanage.h"
#include "sysmanage.h"
#define DAY_S	(24 * 60 * 60)
static ENUM_ENABLED_E sysExit = ENUM_ENABLED_FALSE;
static void* MemoryManage(void * arg)
{
    while(sysExit)
    {
		system("echo 3 >/proc/sys/vm/drop_caches");
		sleep(30);
	}
	return NULL;
}

static void *MaintenanceManage(void * arg)
{
	long curTime = 0, endTime = 0;
	GetUpTime(&curTime);
	endTime = curTime + (5 * DAY_S);
	while (sysExit)
	{
		GetUpTime(&curTime);
		if (curTime > endTime)
		{
			exit(0);
		}
		usleep(50 * 1000);
	}
}

static int SystemRecycle()
{
	pthread_t memoryID;	
	if (pthread_create(&memoryID, 0, MemoryManage,(void*) NULL)){
		DF_ERROR("pthread_create MemoryManage  is err \n");
		assert(0);
	}
	pthread_detach(memoryID);
	return KEY_TRUE;
}

static int SystemMaintenance()
{
	pthread_t maintenanceID;	
	if (pthread_create(&maintenanceID, 0, MaintenanceManage,(void*) NULL)){
		DF_ERROR("pthread_create MemoryManage  is err \n");
		assert(0);
	}
	pthread_detach(maintenanceID);
	return KEY_TRUE;
}

int SystemManageInit()
{
	sysExit = ENUM_ENABLED_TRUE;
	SystemRecycle();
	SystemMaintenance();
	return KEY_TRUE;
}

int SystemManageUnInit()
{
	sysExit = ENUM_ENABLED_FALSE;
	return KEY_TRUE;
}

