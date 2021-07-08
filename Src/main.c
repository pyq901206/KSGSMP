/******************************************************************* 
|  模块:配置�?
|  版本: 1.0
|  作�? tanfan [tanfan0406@163.com]
|  说明: 本模块为启动模块
|  修改: 
******************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "common.h"
#include <errno.h>
#include "adpapi.h"
#include "mqttapi.h"
#include "mysqlctrl.h"
#include "httpapi.h"
#include "webs.h"
#include "rtmpstart.h"
#include "cictapi.h"
#include "hj212api.h"

static int  runflag = 0;
static void sigexit(int arg)
{
    printf("Catch exit signal %d\n", arg);
    runflag = 0;
}

int main()
{	
	int iret = -1;
    signal(SIGINT, sigexit);
    signal(SIGILL, sigexit);
    signal(SIGKILL, sigexit);
    signal(SIGPIPE, SIG_IGN); //socket操作需�?否则socket异常会导致程序退�?
    signal(SIGCHLD, SIG_DFL); //system()需�?SIGCHLD设置为SIG_DFL否则调用有风�?
    runflag = 1;
	srand(time(NULL));
	
	iret = AdpHandleInit();	
	if (KEY_TRUE != iret){
		AdpHandleUnInit();
		DF_DEBUG("AdpHandleInit Fail .");
		return KEY_FALSE;
	}
	RtmpInit();
	HttpApiInit();
	HttpApiStart(1886);
	WebServerHttpInit(1880);
	MqttStart("127.0.0.1");
	CICTApiStart("127.0.0.1");
	HJ212ApiStart(1891);
	while(runflag>0){
		usleep(1000*1000);	
	}
	RtmpDeInit();
	MqttStop();
	CICTApiStop();
	HJ212ApiStop();
	HttpApiStop();
	HttpApiUnInit();
	WebServerHttpUnInit();
	AdpHandleUnInit();
	return KEY_TRUE;
}
