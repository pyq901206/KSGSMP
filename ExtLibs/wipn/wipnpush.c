
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "NDT_API.h"
#include "WiPN_API.h"
#include "WiPN_StringEncDec.h"

#include "wipnpush.h"

#if(defined DEBUG_ON )
#define WIPN_DEBUG printf
#else
#define WIPN_DEBUG
#endif
static int g_WipnInitFlag=0;

#define RESEND_NUMBER 1
#define Loc_Title_key  "TITLE FLAG"

static int Add_APINFO(	char *pCMD_AINFO, 
				const unsigned short Size_CMD_AINFO, 
				const char *Cmd, 		// from setCmd 
				const char *title_loc_key,
				const char *title_loc_args[],
				const unsigned short Number_Of_title_loc_args,
				const char *loc_key,
				const char *loc_args[],
				const unsigned short Number_Of_loc_args)
{
	if (!pCMD_AINFO 
		|| 0 == Size_CMD_AINFO
		|| !Cmd
		|| 0 == strlen(Cmd)
		|| !title_loc_key 
		|| !title_loc_args
		|| 0 == Number_Of_title_loc_args
		|| !loc_key
		|| !loc_args
		|| 0 == Number_Of_loc_args)
	{
		return WiPN_ERROR_InvalidParameter;
	}	
	int i = 0;
    for (; i < Number_Of_title_loc_args; i++)
    {
        if (!title_loc_args[i])
        {
            printf("Add_APINFO - title_loc_args[%d] is NULL!!\n", i);
            return WiPN_ERROR_InvalidParameter;
        }
    }
	for (i = 0; i < Number_Of_loc_args; i++)
	{
		if (!loc_args[i])
        {
            printf("Add_APINFO - loc_args[%d] is NULL!!\n", i);
            return WiPN_ERROR_InvalidParameter;
        }
	}
	// Size of (title_loc_key + loc_key) do not exceed 64 byte
	if (64 < (strlen(title_loc_key)+strlen(loc_key)))
	{
		printf("Add_APINFO - The total length of title_loc_key and loc_key does not exceed %d BYTE\n", 64);
        return WiPN_ERROR_ExceedMaxSize;
	}
	// Size of (title_loc_args + loc_args) do not exceed 128 byte
	short TotalSize_title_loc_args = 0;
	for (i = 0; i < Number_Of_title_loc_args; i++)
	{
		TotalSize_title_loc_args += strlen(title_loc_args[i]);
	}
	short TotalSize_loc_args = 0;
	for (i = 0; i < Number_Of_loc_args; i++)
	{
		TotalSize_loc_args += strlen(loc_args[i]);
	}
	if (128 < (TotalSize_title_loc_args+TotalSize_loc_args))
	{
		printf("Add_APINFO - The total length of title_loc_args and loc_args does not exceed %d BYTE\n", 128);
        return WiPN_ERROR_ExceedMaxSize;
	}
	
	//// ------------------------------------------------------
	//// title_loc_args
	unsigned short Size_title_loc_args = TotalSize_title_loc_args
										+strlen("[]")
										+strlen("\"")*Number_Of_title_loc_args*2
										+Number_Of_title_loc_args;
	char *pTitle_loc_args_Buf = (char*)malloc(Size_title_loc_args);
	if (!pTitle_loc_args_Buf)
	{
		//debug_log(__FILE__, "Add_APINFO - Malloc pTitle_loc_args_Buf failed!!\n");
		return WiPN_ERROR_MallocFailed;
	}
	memset(pTitle_loc_args_Buf, 0, Size_title_loc_args);
	
	SNPRINTF(pTitle_loc_args_Buf, Size_title_loc_args, "[\"%s\"", title_loc_args[0]);
	for (i = 1; i < Number_Of_title_loc_args; i++)
	{
		char args[128] = {0};
		memset(args, 0, sizeof(args));
		SNPRINTF(args, sizeof(args), ",\"%s\"", title_loc_args[i]);
		strcat(pTitle_loc_args_Buf, args);
	}
	strcat(pTitle_loc_args_Buf, "]");   // ["abc", "def", "..."]
	printf("title_loc_args=%s. Size=%u byte\n", pTitle_loc_args_Buf, (unsigned)strlen(pTitle_loc_args_Buf));
	
	unsigned short Length_title_loc_args = ((Size_title_loc_args*4/3)/4+2)*4;
	char *title_loc_args_Base64 = (char*)malloc(Length_title_loc_args);
	if (!title_loc_args_Base64)
	{
		//debug_log(__FILE__, "Add_APINFO - Malloc title_loc_args_Base64 Buf failed!!\n");
		if (pTitle_loc_args_Buf) free(pTitle_loc_args_Buf);
		return WiPN_ERROR_MallocFailed;
	}
	memset(title_loc_args_Base64, 0, Length_title_loc_args);
	// base64 Transformation Payload 
	Base64_Encode(pTitle_loc_args_Buf, title_loc_args_Base64);
	unsigned short Size_title_loc_args_Base64 = strlen(title_loc_args_Base64);
	
	//// ------------------------------------------------------
	//// loc_args
	unsigned short Size_loc_args = TotalSize_loc_args
									+strlen("[]")
									+strlen("\"")*Number_Of_loc_args*2
									+Number_Of_loc_args;
	char *pLoc_args_Buf = (char*)malloc(Size_loc_args);
	if (!pLoc_args_Buf)
	{
		//debug_log(__FILE__, "Add_APINFO - Malloc pLoc_args_Buf failed!!\n");
		if (pTitle_loc_args_Buf) free(pTitle_loc_args_Buf);
		if (title_loc_args_Base64) free(title_loc_args_Base64);
		return WiPN_ERROR_MallocFailed;
	}
	memset(pLoc_args_Buf, 0, Size_loc_args);
	
	SNPRINTF(pLoc_args_Buf, Size_loc_args, "[\"%s\"", loc_args[0]);
	for (i = 1; i < Number_Of_loc_args; i++)
	{
		char args[128] = {0};
		memset(args, 0, sizeof(args));
		SNPRINTF(args, sizeof(args), ",\"%s\"", loc_args[i]);
		strcat(pLoc_args_Buf, args);
	}
	strcat(pLoc_args_Buf, "]");   // ["abc", "def", "..."]
	printf("loc_args=%s. Size=%u byte\n\n", pLoc_args_Buf, (unsigned)strlen(pLoc_args_Buf));
	
	unsigned short Length_loc_args = ((Size_loc_args*4/3)/4+2)*4;
	char *loc_args_Base64 = (char*)malloc(Length_loc_args);
	if (!loc_args_Base64)
	{
		//debug_log(__FILE__, "Add_APINFO - Malloc loc_args_Base64 Buf failed!!\n");
		if (pTitle_loc_args_Buf) free(pTitle_loc_args_Buf);
		if (title_loc_args_Base64) free(title_loc_args_Base64);
		if (pLoc_args_Buf) free(pLoc_args_Buf);
		return WiPN_ERROR_MallocFailed;
	}
	memset(loc_args_Base64, 0, Length_loc_args);
	// base64 Transformation Payload 
	Base64_Encode(pLoc_args_Buf, loc_args_Base64);
	unsigned short Size_loc_args_Base64 = strlen(loc_args_Base64);
	
	if ((strlen(Cmd)+Size_title_loc_args_Base64+Size_loc_args_Base64) > (Size_CMD_AINFO-1))
	{
		printf("Add_APINFO - The total length of CMD after base64 exceed %d BYTE!!\n", Size_CMD_AINFO);
		if (pTitle_loc_args_Buf) free(pTitle_loc_args_Buf);
		if (title_loc_args_Base64) free(title_loc_args_Base64);
		if (pLoc_args_Buf) free(pLoc_args_Buf);
		if (loc_args_Base64) free(loc_args_Base64);
		return WiPN_ERROR_ExceedMaxSize;
	}
	//// ------------------------------------------------------
	
	// Format Cmd
	SNPRINTF(pCMD_AINFO, Size_CMD_AINFO, "%s&AINFO=title-loc-key=%s,loc-key=%s,title-loc-args=%s,loc-args=%s&",  
	Cmd,
	title_loc_key, 
	loc_key, 
	title_loc_args_Base64, 
	loc_args_Base64);
	
	if (pTitle_loc_args_Buf) free(pTitle_loc_args_Buf);
	if (title_loc_args_Base64) free(title_loc_args_Base64);
	if (pLoc_args_Buf) free(pLoc_args_Buf);
	if (loc_args_Base64) free(loc_args_Base64);
	return 0;
}


//加密指令字符串，base64编码
static int setCmd(	char *pCmd, 
			unsigned short SizeOfCmd,
			const CHAR *pDeviceDID, 
			const CHAR *pIPNLicense, 
			ULONG EventCH, 
			const char *pTitle, 
			const char *pContent, 
			const char *pSoundName, 
			unsigned short Badge, 
			const char *pPayload
			)
{
	if (!pCmd || 0 == SizeOfCmd || !pDeviceDID || !pIPNLicense)
	{
		return WiPN_ERROR_InvalidParameter;
	}	
	if (0 == strlen(pDeviceDID) || 0 == strlen(pIPNLicense) || 0xFFFFFFFF < EventCH)
	{
		return WiPN_ERROR_InvalidParameter;
	}
	if (!pTitle) pTitle = "";
	if (!pContent) pContent = "";
	if (!pSoundName) pSoundName = "";
	if (!pPayload) pPayload = "";
	if (SIZE_Content+SIZE_Payload < strlen(pContent)+strlen(pPayload))
	{
		WIPN_DEBUG("setCmd - Length Of Content and Payload Exceed %d bytes!!\n", SIZE_Content+SIZE_Payload);
		return WiPN_ERROR_ExceedMaxSize;
	}
	
	unsigned short Length_Title = ((strlen(pTitle)*4/3)/4+2)*4;
	char *title_Base64 = (char *)malloc(Length_Title);
	if (!title_Base64)
	{
		WIPN_DEBUG("setCmd - Malloc title_Base64 Buf failed!!\n");
		return WiPN_ERROR_MallocFailed;
	}
	memset(title_Base64, 0, sizeof(title_Base64));
	// base64 转换标题
	Base64_Encode(pTitle, title_Base64);

	unsigned short Length_Content = (((strlen(pContent)*4)/3)/4+2)*4;
	char *Content_Base64 = (char *)malloc(Length_Content);
	if (!Content_Base64)
	{
		WIPN_DEBUG("setCmd - Malloc Content_Base64 Buf failed!!\n");
		free((void*)title_Base64);
		return WiPN_ERROR_MallocFailed;
	}
	memset(Content_Base64, 0, Length_Content);
	// base64 转换 Content 内容
	Base64_Encode(pContent, Content_Base64);
	
	unsigned short LengthAfterBase64 = 	strlen(pDeviceDID)
										+strlen(pIPNLicense)
										+sizeof(EventCH)
										+strlen(title_Base64)
										+strlen(Content_Base64)
										+strlen(pSoundName)
										+sizeof(Badge);
										//+sizeof(Message_type);
	memset(pCmd, '\0', SizeOfCmd);
	if (NULL != pPayload && 0 != strlen(pPayload))
	{
		unsigned short Length_Payload = ((strlen(pPayload)*4/3)/4+2)*4;
		char *Payload_Base64 = (char*)malloc(Length_Payload);
		if (!Payload_Base64)
		{
			WIPN_DEBUG("setCmd - Malloc Payload_Base64 Buf failed!!\n");
			free((void*)title_Base64);
			free((void*)Content_Base64);
			return WiPN_ERROR_MallocFailed;
		}
		memset(Payload_Base64, 0, Length_Payload);
		// base64 转换 Payload 内容
		Base64_Encode(pPayload, Payload_Base64);
		
		unsigned short Length_Payload1 = ((strlen(Payload_Base64)*4/3)/4+2)*4;
		char *Payload1_Base64 = (char*)malloc(Length_Payload1);
		if(!Payload1_Base64)
		{
			free((void*)title_Base64);
			free((void*)Content_Base64);
			free((void*)Payload_Base64);
		}
		Base64_Encode(Payload_Base64, Payload1_Base64);
		if (LengthAfterBase64 + strlen(Payload1_Base64) > SizeOfCmd - 1)
		{
			free((void*)title_Base64);
			free((void*)Content_Base64);
			free((void*)Payload_Base64);
			free((void*)Payload1_Base64);
			return WiPN_ERROR_ExceedMaxSize;
		}
	
		SNPRINTF(pCmd, SizeOfCmd, "DID=%s&LNS=%s&CH=%lu&PINFO=title=%s,content=%s,sound=%s,badge=%d,payload=%s&", pDeviceDID, pIPNLicense, EventCH, title_Base64, Content_Base64, pSoundName, Badge, Payload1_Base64);
		
		if (Payload_Base64) 
			free((void*)Payload_Base64);
		if (Payload1_Base64) 
		    free((void*)Payload1_Base64);
	}
	else 
	{
		if (LengthAfterBase64 > SizeOfCmd - 1)
		{
			free((void*)title_Base64);
			free((void*)Content_Base64);
			return WiPN_ERROR_ExceedMaxSize;
		}
		SNPRINTF(pCmd, SizeOfCmd, "DID=%s&LNS=%s&CH=%lu&PINFO=title=%s,content=%s,sound=%s,badge=%d&", pDeviceDID, pIPNLicense, EventCH, title_Base64, Content_Base64, pSoundName, Badge);
	}
	if (title_Base64) 
		free((void*)title_Base64);
	if (Content_Base64) 
		free((void*)Content_Base64);
	
	return 0;
}
int WipnGetVesion(char*vesion)
{
	unsigned int ver;
	char tpversion[64]={0};
	char*description=NDT_PPCS_GetAPIVersion(&ver);
	sprintf(tpversion,"NDT API Version:%d.%d.%d.%d",(ver&0xFF000000)>>24,(ver&0x00FF0000)>>16,(ver&0x0000FF00)>>8,(ver&0x000000FF)>>0);
	memcpy(vesion,tpversion,strlen(tpversion));
	return 0;
}

int WipnNetDetect(WIPN_NETINFO_S *netinfo)
{
	st_NDT_NetInfo NetInfo;
	if(g_WipnInitFlag!=1)
	{
		WIPN_DEBUG("WIPN NOT INIT \n");
		return -1;
	}
	NDT_PPCS_NetworkDetect(&NetInfo,5000);
	memcpy(netinfo->lanip,NetInfo.LanIP,16);
	memcpy(netinfo->wanip,NetInfo.WanIP,16);
	netinfo->lanport=NetInfo.LanPort;
	return 0;
}
int WipnInit(char*	initstring,char*initkey,char*did)
{
	int ret;
	if(g_WipnInitFlag==0)
	{
		ret=NDT_PPCS_Initialize(initstring,0,did,initkey);
		if((0 != ret)&&( (-1) != ret))  
		{
			WIPN_DEBUG("NDT_PPCS_Initialize is error %d \n",ret,getErrorCodeInfo(ret));
			return -1;
		}
	g_WipnInitFlag=1;
	}
	return 0;
}

int WipnDeInit()
{
	int ret;
	NDT_PPCS_DeInitialize();
	//bg=1;
	g_WipnInitFlag=0;
	return 0;
}

//#define Loc_Key      "MOVE_DELECT_ALARM"

int WipnExecute(char*did ,char*license,WIPN_SERVICE_S *PushMessage)
{
	int ret=0;
	int i=0,j=0;
	char retstring[32]={0};
	char cmd[630]={0};
	//char *querDID[2]={QUERY_SERVER_DID_Test_1,QUERY_SERVER_DID_Test_2};
	const char*querDID[(QUERY_SERVER_NUMBER/4+1)*4] = {QUERY_SERVER_DID_1, 
																QUERY_SERVER_DID_2,
																QUERY_SERVER_DID_3,
																QUERY_SERVER_DID_4,
																QUERY_SERVER_DID_5,
																QUERY_SERVER_DID_6};
    char querystring[600]={0};
	char utct[16]={0};
	if(g_WipnInitFlag!=1)
	{
		WIPN_DEBUG("WIPN NOT INIT \n");
		return -1;
	}
	for(i=0;i<RESEND_NUMBER;i++)
	{
		ret=WiPN_Query(did,querDID,querystring,sizeof(querystring),NULL,0,utct,sizeof(utct));//
		if(0<=ret)
		{
	
			break;
		}
		else
		{
			j++;
		}
	}
	if(j==RESEND_NUMBER)
	{
		WIPN_DEBUG("WiPN_Query is error %d %s\n",ret,getErrorCodeInfo(ret));
		return -1;
		//pthread_exit(0);
	}
	j=0;
	ret=setCmd(cmd,sizeof(cmd),did,license,0,PushMessage->title,PushMessage->content,PushMessage->soundname,1,PushMessage->payload);
	if(0!=ret)
	{
		WIPN_DEBUG("setcmd %d %s\n",ret,getErrorCodeInfo(ret));
		return -1;
	}
	unsigned short Number_Of_title_loc_args = 1;	// 推送国际化的标题的个数
	const char *title_loc_args[1] = {PushMessage->title};		// 推送国际化的标题，标题可自定义
	unsigned short Number_Of_loc_args = 2;			// 推送国际化的推送内容的个数
	const char *loc_args[2] = {"device1","device2"};	// 推送国际化的推送内容，内容可自定义
	
	char CMD_AINFO[600] = {0};
					
	memset(CMD_AINFO, 0, sizeof(CMD_AINFO));
	ret = Add_APINFO(CMD_AINFO, 		// out put buffer, for save Cmd+APINFO
					sizeof(CMD_AINFO), 	// the size of the out put buffer
					cmd, 				// from setCmd
					Loc_Title_key, 
					title_loc_args,
					Number_Of_title_loc_args,
					PushMessage->content,
					loc_args, 
					Number_Of_loc_args);
	for(i=0;i<RESEND_NUMBER;i++)
	{
		ret=WiPN_Post(querystring,CMD_AINFO,retstring,sizeof(retstring),utct,sizeof(utct));
		//printf("retstring=%s\n",retstring);
		if((0<=ret)&&(strcmp(retstring,"OK")==0||strcmp(retstring,"No Subscriber")==0))
		{
			WIPN_DEBUG("WiPN_Post is success ----------------------------------\n");
			//bg++;
			break;
		}
		else
		{
			j++;	
		}
	}
	if(j==RESEND_NUMBER)
	{
		if(0>ret)
		{
			WIPN_DEBUG("WiPN_Post is error %d %s\n",ret,getErrorCodeInfo(ret));
			return -1;
		}
		else
		WIPN_DEBUG("WARNIN : Post is succsess but maybe the device is invaild \n");
	}
	return 0;
}


































