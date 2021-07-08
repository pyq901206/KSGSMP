#include "WiPN_API.h"

//// 用于记录每次 recvFrom 返回时设备的当前时间
time_t g_Time_ServerRet = 0;
//// Mode:0->内网广播优先其次公网,1->仅公网,2->仅内网广播.
INT32 g_SendToMode = 1;	

static void mSecSleep(UINT32 ms)
{
#if defined WINDOWS
	Sleep(ms);
#elif defined LINUX
	usleep(ms * 1000);
#endif
}

// 获取错误信息
const char *getErrorCodeInfo(int err)
{
    if (0 < err)
	{
		return "NoError, May be a Socket handle value or Data Size!";
	}
    switch (err)
    {
        case 0: return "NDT_ERROR_NoError";
        case -1: return "NDT_ERROR_AlreadyInitialized";
        case -2: return "NDT_ERROR_NotInitialized";
        case -3: return "NDT_ERROR_TimeOut";
        case -4: return "NDT_ERROR_ScketCreateFailed";
        case -5: return "NDT_ERROR_ScketBindFailed";
        case -6: return "NDT_ERROR_HostResolveFailed";
        case -7: return "NDT_ERROR_ThreadCreateFailed";
        case -8: return "NDT_ERROR_MemoryAllocFailed";
        case -9: return "NDT_ERROR_NotEnoughBufferSize";
        case -10: return "NDT_ERROR_InvalidInitString";
        case -11: return "NDT_ERROR_InvalidAES128Key";
        case -12: return "NDT_ERROR_InvalidDataOrSize";
        case -13: return "NDT_ERROR_InvalidDID";
        case -14: return "NDT_ERROR_InvalidNDTLicense";
        case -15: return "NDT_ERROR_InvalidHandle";
        case -16: return "NDT_ERROR_ExceedMaxDeviceHandle";
        case -17: return "NDT_ERROR_ExceedMaxClientHandle";
        case -18: return "NDT_ERROR_NetworkDetectRunning";
        case -19: return "NDT_ERROR_SendToRunning";
        case -20: return "NDT_ERROR_RecvRunning";
        case -21: return "NDT_ERROR_RecvFromRunning";
        case -22: return "NDT_ERROR_SendBackRunning";
        case -23: return "NDT_ERROR_DeviceNotOnRecv";
        case -24: return "NDT_ERROR_ClientNotOnRecvFrom";
        case -25: return "NDT_ERROR_NoAckFromCS";
        case -26: return "NDT_ERROR_NoAckFromPushServer";
        case -27: return "NDT_ERROR_NoAckFromDevice";
        case -28: return "NDT_ERROR_NoAckFromClient";
        case -29: return "NDT_ERROR_NoPushServerKnowDevice";
        case -30: return "NDT_ERROR_NoPushServerKnowClient";
        case -31: return "NDT_ERROR_UserBreak";
        case -32: return "NDT_ERROR_SendToNotRunning";
        case -33: return "NDT_ERROR_RecvNotRunning";
        case -34: return "NDT_ERROR_RecvFromNotRunning";
        case -35: return "NDT_ERROR_SendBackNotRunning";
        case -36: return "NDT_ERROR_RemoteHandleClosed";
        case -99: return "NDT_ERROR_FAESupportNeeded";
        // WiPN 的错误信息
        case WiPN_ERROR_InvalidParameter: return "WiPN_ERROR_InvalidParameter";
        case WiPN_ERROR_iPNStringEncFailed: return "WiPN_ERROR_iPNStringEncFailed";
        case WiPN_ERROR_iPNStringDncFailed: return "WiPN_ERROR_iPNStringDncFailed";
        case WiPN_ERROR_GetPostServerStringItemFailed: return "WiPN_ERROR_GetPostServerStringItemFailed";
        case WiPN_ERROR_GetSubscribeServerStringItemFailed: return "WiPN_ERROR_GetSubscribeServerStringItemFailed";
        case WiPN_ERROR_GetUTCTStringItemFailed: return "WiPN_ERROR_GetUTCTStringItemFailed";
        case WiPN_ERROR_GetNumberFromPostServerStringFailed: return "WiPN_ERROR_GetNumberFromPostServerStringFailed";
        case WiPN_ERROR_GetNumberFromSubscribeServerStringFailed: return "WiPN_ERROR_GetNumberFromSubscribeServerStringFailed";
        case WiPN_ERROR_GetDIDFromPostServerStringFailed: return "WiPN_ERROR_GetDIDFromPostServerStringFailed";
        case WiPN_ERROR_GetDIDFromSubscribeServerStringFailed: return "WiPN_ERROR_GetDIDFromSubscribeServerStringFailed";
        case WiPN_ERROR_GetRETStringItemFailed: return "WiPN_ERROR_GetRETStringItemFailed";
        case WiPN_ERROR_MallocFailed: return "WiPN_ERROR_MallocFailed";
		case WiPN_ERROR_ExceedMaxSize: return "WiPN_ERROR_ExceedMaxSize";
		// WiPN Web 的错误信息
		case WiPN_ERROR_TimeOut: return "WiPN_ERROR_TimeOut";
		case WiPN_ERROR_SocketCreateFailed: return "WiPN_ERROR_SocketCreateFailed";
		case WiPN_ERROR_SocketConnectFailed: return "WiPN_ERROR_SocketConnectFailed";
		case WiPN_ERROR_InvalidSocketID: return "WiPN_ERROR_InvalidSocketID";
		case WiPN_ERROR_SetSockOptFailed: return "WiPN_ERROR_SetSockOptFailed";
		case WiPN_ERROR_SocketWriteFailed: return "WiPN_ERROR_SocketWriteFailed";
		case WiPN_ERROR_SocketReadFailed: return "WiPN_ERROR_SocketReadFailed";
		case WiPN_ERROR_RemoteSocketClosed: return "WiPN_ERROR_RemoteSocketClosed";
		case WiPN_ERROR_SelectError: return "WiPN_ERROR_SelectError";
		case WiPN_ERROR_GetContentFromHttpRetDataFailed: return "WiPN_ERROR_GetContentFromHttpRetDataFailed";
		case WiPN_ERROR_GetNDTRETItemFailed: return "WiPN_ERROR_GetNDTRETItemFailed";
		case WiPN_ERROR_DeviceTokenIsEmpty: return "WiPN_ERROR_DeviceTokenIsEmpty";
        default:
            return "Unknow, something is wrong!";
    }
}

//// 根据 ItemName 获取相应的字符串
//// ret=0 OK, ret=-1: Invalid Parameter, ret=-2: No such Item
int GetStringItem(	const char *SrcStr, 
					const char *ItemName, 
					const char Seperator, 
					char *RetString, 
					const int MaxSize)
{
    if (!SrcStr || !ItemName || !RetString || 0 == MaxSize)
	{
		return -1;
	}
    const char *pFand = SrcStr;
    while (1)
    {
        pFand = strstr(pFand, ItemName);
        if (!pFand)
		{
			return -2;
		} 
        pFand += strlen(ItemName);
        if ('=' != *pFand)
		{
			continue;
		}     
        else
		{
			break;
		}    
    }
    
    pFand += 1;
    int i = 0;
    while (1)
    {
        if (Seperator == *(pFand + i) || '\0' == *(pFand + i) || i >= (MaxSize - 1))
		{
			break;
		} 
        else
		{
			*(RetString + i) = *(pFand + i);
		}     
        i++;
    }
    *(RetString + i) = '\0';
    
    return 0;
}

// 获取 PostServer/SubscribeServer 服务器数量
int pharse_number(const CHAR *pServerString, unsigned short *Number, unsigned short *SizeOfDID)
{
    if (!pServerString || !Number)
	{
		return -1;
	}
	// 获取 PostServerDID 数量
    CHAR buf[8];
    memset(buf, 0, sizeof(buf));
    const CHAR *pS = pServerString;
    const CHAR *p1 = strstr(pServerString, ",");
    if (!p1)
	{
		return -1;
	}  
    if (p1 - pS > sizeof(buf) - 1)
	{
		return -1;
	}  
    int i = 0;
    while (1)
    {
        if (pS + i >= p1)
		{
			break;
		} 
        buf[i] = *(pS + i);
        i++;
    }
    buf[i] = '\0';
    *Number = atoi(buf);
    
	// 获取 PostServerDID 长度
	p1 += 1; 	// 指向第一个 DID
	const char *p2 = strstr(p1, ",");
	if (!p2)	// -> 只有一个 DID -> "01,ABCD-123456-ABCDEF"
	{
		*SizeOfDID = strlen(p1);
	}	
	else
	{
		*SizeOfDID = (unsigned short)(p2 - p1);
	}	
	//printf("SizeOfDID= %d\n", *SizeOfDID);
    return 0;
}

// 获取 PostServer/SubscribeServer 服务器DID
const char *pharse_DID(const char *pServerString, int index)
{
    if (!pServerString || 0 > index)
	{
		return NULL;
	}
    const char *p1 = strstr(pServerString, ",");
    if (!p1)
	{
		return NULL;
	}  
    p1 += 1;		// -> 指向第一个 DID
    
    const char *p2 = strstr(p1, ",");
    if (!p2) // -> 只有一个 DID
	{
		unsigned short LengthOfDID = strlen(p1);
		if (0 == LengthOfDID)
		{
			return NULL;
		}			
		//printf("LengthOfDID= %d\n", LengthOfDID);
		char *pDID = (char *)malloc((LengthOfDID/4+1)*4);
		if (!pDID)
		{
			printf("pharse_DID - malloc failed!!\n");
			return NULL;
		}
		memset(pDID, '\0', (LengthOfDID/4+1)*4);
		memcpy(pDID, p1, LengthOfDID);
		return pDID;
	}
    unsigned short SizeOfDID = (unsigned short)(p2 - p1);
    //printf("SizeOfDID= %u\n", SizeOfDID);
    
    p1 = pServerString;
    int i = 0;
    for (; i < index + 1; i++)
    {
        p1 = strstr(p1, ",");
        if (!p1)
		{
			 break;
		}    
        p1 += 1;
    }
    if (!p1)
	{
		return NULL;
	}  
    char *pDID = (char *)malloc((SizeOfDID/4+1)*4);
	if (!pDID)
	{
		printf("pharse_DID - malloc failed!!\n");
		return NULL;
	}
    memset(pDID, '\0', (SizeOfDID/4+1)*4);
    memcpy(pDID, p1, SizeOfDID);
    //printf("p_DID = %s\n", p_DID);
    
    return pDID;
}

//// -------------------------------- WiPN API Begin --------------------------------
//// WiPN_Query for WiPN API
INT32 WiPN_Query(const CHAR *pDeviceDID, 
				const CHAR *QueryServerDID[], 
				CHAR *pPostServerString, 
				UINT32 SizeOfPostServerString, 
				CHAR *pSubscribeServerString, 
				UINT32 SizeOfSubscribeServerString, 
				CHAR *pUTCTString, 
				UINT32 SizeOfUTCTString)
{
    if (!pDeviceDID || 0 == strlen(pDeviceDID))
    {
        //printf("WiPN_Query - pDeviceDID is NULL!!\n");
        return WiPN_ERROR_InvalidParameter;
    }
	if (!QueryServerDID)
    {
       // printf("WiPN_Query - QueryServerDID Buf is NULL!!\n");
        return WiPN_ERROR_InvalidParameter;
    }
    int i = 0;
    for (; i < QUERY_SERVER_NUMBER; i++)
    {
        if (NULL == QueryServerDID[i])
        {
           // printf("WiPN_Query - QueryServerDIDbuf[%d] have no DID!!\n", i);
            return WiPN_ERROR_InvalidParameter;
        }
    }
    if (!pPostServerString && !pSubscribeServerString)
    {
        //printf("WiPN_Query - pPostServerString && pSubscribeServerString is NULL!!\n");
        return WiPN_ERROR_InvalidParameter;
    }
    if (NULL != pPostServerString && 0 == SizeOfPostServerString)
    {
       // printf("WiPN_Query - SizeOfPostServerString= %d\n", SizeOfPostServerString);
        return WiPN_ERROR_InvalidParameter;
    }
    if (NULL != pSubscribeServerString && 0 == SizeOfSubscribeServerString)
    {
        //printf("WiPN_Query - SizeOfSubscribeServerString= %d\n", SizeOfSubscribeServerString);
        return WiPN_ERROR_InvalidParameter;
    }
    if (!pUTCTString)
    {
        //printf("WiPN_Query - pUTCTString is NULL!!\n");
        return WiPN_ERROR_InvalidParameter;
    }
    if (NULL != pUTCTString && 0 == SizeOfUTCTString)
    {
       // printf("WiPN_Query - SizeOfUTCTString= 0 !!\n");
        return WiPN_ERROR_InvalidParameter;
    }
	
	// 存放: 1.Cmd:加密前的指令
	//		2.Response: 解密后的服务器回应的数据
	unsigned short SizeOf_CR_Buffer = 640;
	char CR_Buffer[640] = {0};
	/*char *CR_Buffer = (char *)malloc(SizeOf_CR_Buffer);
	if (!CR_Buffer)
	{
		printf("WiPN_Query - Malloc Query Cmd Buf failed!!\n");
		return WiPN_ERROR_MallocFailed;
	}*/  
	
	// 存放: 1.Cmd: 加密后的指令(Cmd加密后长度会翻倍)
	//		2.Response: 未解密的服务器回应的数据(RecvFrom 所接收到的数据)
	unsigned short SizeOfBuffer = SizeOf_CR_Buffer*2;
	char *Buffer = (char *)malloc(SizeOfBuffer);
	if (!Buffer)
	{
		//printf("WiPN_Query - Malloc Buffer failed!!\n");
		//free((void *)CR_Buffer);
		return WiPN_ERROR_MallocFailed;
	}
	
	// 格式化查询指令到 CR_Buffer
    sprintf(CR_Buffer, "DID=%s&", pDeviceDID);
    printf("QueryCmd= %s\nSize= %u byte\n", CR_Buffer, (UINT32)strlen(CR_Buffer));	
	
	// 加密查询命令到 Buffer
    if (0 > iPN_StringEnc(STRING_ENC_DEC_KEY, CR_Buffer, Buffer, SizeOfBuffer))
    {
       // printf("WiPN_Query - iPN_StringEnc: Query Cmd Enc failed!\n");
		//free((void *)CR_Buffer);
		free((void *)Buffer);
		return WiPN_ERROR_iPNStringEncFailed;
    }

	int ret = 0;
    int QueryServerHandle = -1;
    short retryCount_RecvFrom = 0;
	srand((UINT32)time(NULL));
	
	short index = abs(rand() % QUERY_SERVER_NUMBER);
    while (1)
    {
        if (0 > QueryServerHandle)
        {      
            for (i = 0; i < QUERY_SERVER_NUMBER; i++)
            {
				index = (index + 1) % QUERY_SERVER_NUMBER;
				
				unsigned short SizeOfQueryCmd = strlen(Buffer);
				//printf("send cmd to QueryServer, QueryServerDID[%d]= %s. SizeOfQueryCmd= %u byte. sending...\n", index, QueryServerDID[index], SizeOfQueryCmd);

				// 发送查询指令到 QueryServer
				ret = NDT_PPCS_SendTo(QueryServerDID[index], Buffer, SizeOfQueryCmd, g_SendToMode);
				
				if (0 > ret)
                {
                    printf("send cmd to QueryServer failed!! ret= %d [%s]\n\n", ret, getErrorCodeInfo(ret));
					printf("retry to send ...\n\n");
                    continue;
                }
                else
                {
                    //printf("send cmd to QueryServer success!! \n");
					QueryServerHandle = ret;
                    break;
                }
            }
            if (0 > QueryServerHandle)
            {
               // printf("WiPN_Query - Get QueryServerHandle failed! QueryServerDID[%d]= %s. ret= %d [%s]\n", index, QueryServerDID[index], ret, getErrorCodeInfo(ret));
                break;
            }
        }
        else
        {
			//printf("Waiting for QueryServer response, please wait ...\n");                
			UINT16 SizeToRead = SizeOfBuffer;
			memset(Buffer, 0, SizeOfBuffer);
			
			ret = NDT_PPCS_RecvFrom(QueryServerHandle, Buffer, &SizeToRead, 3000); 
			
			// 记录 RecvFrom 返回的当前时间
			time_t Time_ServerRet = time(NULL);
			struct tm *ptm = localtime((const time_t *)&Time_ServerRet);
            
            if (0 > ret)
            {
				//printf("WiPN_Query - NDT_PPCS_RecvFrom: QueryServerDID[%d]= %s. ret= %d. [%s]\n", index, QueryServerDID[index], ret, getErrorCodeInfo(ret));
				if (ptm)
				{
					//printf("LocalTime: %d-%02d-%02d %02d:%02d:%02d\n", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
				} 
				
                // -26 -27 -3
                if ((NDT_ERROR_NoAckFromPushServer == ret || NDT_ERROR_NoAckFromDevice == ret || NDT_ERROR_TimeOut == ret) && 3 > retryCount_RecvFrom)
                {
                    retryCount_RecvFrom++;
                    continue;
                }
                else if (NDT_ERROR_RemoteHandleClosed == ret) // -36
				{
					//printf("WiPN_Query - QueryServer already call CloseHandle(). QueryServerDID[%d]= %s\n", index, QueryServerDID[index]);
				} 
                break;
            }
            else
            {
				// 解密接收到的数据, 输出到 CR_Buffer             
                if (0 > iPN_StringDnc(STRING_ENC_DEC_KEY, Buffer, CR_Buffer, SizeOf_CR_Buffer))
                {
                   // printf("WiPN_Query - NDT_PPCS_RecvFrom - iPN_StringDnc: RecvFrom Data Dec failed! QueryServerDID[%d]= %s. \n", index, QueryServerDID[index]);
                    ret = WiPN_ERROR_iPNStringDncFailed;
                    break;
                }
				//printf("\nFrom QueryServer: \n");
				//printf("QueryServerDID[%d]= %s\n", index, QueryServerDID[index]);
				//printf("QueryServerHandle= %d\n", QueryServerHandle);
				//printf("RecvFromData: %s\nSizeOfRecvFromData: %u byte\n", Buffer, (UINT32)strlen(Buffer));
				//printf("Data: %s\nSizeOfData: %u byte\n", CR_Buffer, (UINT32)strlen(CR_Buffer));
				if (ptm)
				{
					//printf("LocalTime: %d-%02d-%02d %02d:%02d:%02d\n", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
				}		
				
                // 拆分 CR_Buffer 获取 PostServerString , 其中 '&' 为截取结束标志符
                if (NULL != pPostServerString && 0 > GetStringItem(CR_Buffer, "Post", '&', pPostServerString, SizeOfPostServerString))
                {
                   // printf("WiPN_Query - Get PostServerString failed!\n");
                    ret = WiPN_ERROR_GetPostServerStringItemFailed;
                }
				// 拆分 CR_Buffer 获取 SubscribeString 
                if (NULL != pSubscribeServerString && 0 > GetStringItem(CR_Buffer, "Subs", '&', pSubscribeServerString, SizeOfSubscribeServerString))
                {
                   // printf("WiPN_Query - Get SubscribeServerString failed!\n");
                    ret = WiPN_ERROR_GetSubscribeServerStringItemFailed;
                }
				// 拆分 CR_Buffer 获取 UTCTString 
                memset(pUTCTString, 0, SizeOfUTCTString);
                if (0 > GetStringItem(CR_Buffer, "UTCT", '&', pUTCTString, SizeOfUTCTString))
                {
                  //  printf("WiPN_Query - Get UTCTString failed!\n");
                    ret = WiPN_ERROR_GetUTCTStringItemFailed;
                }
				else // g_Time_ServerRet 必须要与 UTCTString 时间同步更新
				{
					g_Time_ServerRet = Time_ServerRet;
				}
            } // ret > 0
            break;
        } // Handle > 0
    } // while
	if (0 <= QueryServerHandle)
	{
		NDT_PPCS_CloseHandle(QueryServerHandle);
		//printf("WiPN_Query - NDT_PPCS_CloseHandle(%d)\n\n", QueryServerHandle);
	}
	//if (CR_Buffer)
	//	free((void *)CR_Buffer);
	if (Buffer)
		free((void *)Buffer);
	
    return ret;
} // WiPN Query

// WiPN_QueryByServer 查询
INT32 WiPN_QueryByServer(const CHAR *pServerString,
						const CHAR *pAES128Key,
						const CHAR *pDeviceDID, 
						const CHAR *QueryServerDID[], 
						CHAR *pPostServerString, 
						UINT32 SizeOfPostServerString, 
						CHAR *pSubscribeServerString, 
						UINT32 SizeOfSubscribeServerString, 
						CHAR *pUTCTString, 
						UINT32 SizeOfUTCTString)
{
    if (!pServerString || 0 == strlen(pServerString))
	{
		printf("WiPN_QueryByServer - pServerString is NULL!!\n");
        return WiPN_ERROR_InvalidParameter;
	}
	if (!pAES128Key || 0 == strlen(pAES128Key))
	{
		printf("WiPN_QueryByServer - pAES128Key is NULL!!\n");
        return WiPN_ERROR_InvalidParameter;
	}
	if (!pDeviceDID || 0 == strlen(pDeviceDID))
    {
        printf("WiPN_QueryByServer - pDeviceDID is NULL!!\n");
        return WiPN_ERROR_InvalidParameter;
    }
	if (!QueryServerDID)
    {
        printf("WiPN_QueryByServer - QueryServerDID Buf is NULL!!\n");
        return WiPN_ERROR_InvalidParameter;
    }
    int i = 0;
    for (; i < QUERY_SERVER_NUMBER; i++)
    {
        if (NULL == QueryServerDID[i])
        {
            printf("WiPN_QueryByServer - QueryServerDIDbuf[%d] have no DID!!\n", i);
            return WiPN_ERROR_InvalidParameter;
        }
    }
    if (!pPostServerString && !pSubscribeServerString)
    {
        printf("WiPN_QueryByServer - pPostServerString && pSubscribeServerString is NULL!!\n");
        return WiPN_ERROR_InvalidParameter;
    }
    if (NULL != pPostServerString && 0 == SizeOfPostServerString)
    {
        printf("WiPN_QueryByServer - SizeOfPostServerString= %d\n", SizeOfPostServerString);
        return WiPN_ERROR_InvalidParameter;
    }
    if (NULL != pSubscribeServerString && 0 == SizeOfSubscribeServerString)
    {
        printf("WiPN_QueryByServer - SizeOfSubscribeServerString= %d\n", SizeOfSubscribeServerString);
        return WiPN_ERROR_InvalidParameter;
    }
    if (!pUTCTString)
    {
        printf("WiPN_QueryByServer - pUTCTString is NULL!!\n");
        return WiPN_ERROR_InvalidParameter;
    }
    if (NULL != pUTCTString && 0 == SizeOfUTCTString)
    {
        printf("WiPN_QueryByServer - SizeOfUTCTString= 0 !!\n");
        return WiPN_ERROR_InvalidParameter;
    }
	
	// 存放: 1.Cmd:加密前的指令
	//		2.Response: 解密后的服务器回应的数据
	unsigned short SizeOf_CR_Buffer = 640;
	char CR_Buffer[640] = {0};
	/*char *CR_Buffer = (char *)malloc(SizeOf_CR_Buffer);
	if (!CR_Buffer)
	{
		printf("WiPN_Query - Malloc Query Cmd Buf failed!!\n");
		return WiPN_ERROR_MallocFailed;
	}*/  
	
	// 存放: 1.Cmd: 加密后的指令(Cmd加密后长度会翻倍)
	//		2.Response: 未解密的服务器回应的数据(RecvFrom 所接收到的数据)
	unsigned short SizeOfBuffer = SizeOf_CR_Buffer*2;
	char *Buffer = (char *)malloc(SizeOfBuffer);
	if (!Buffer)
	{
		printf("WiPN_QueryByServer - Malloc Buffer failed!!\n");
		//free((void *)CR_Buffer);
		return WiPN_ERROR_MallocFailed;
	}
	
	// 格式化查询指令到 CR_Buffer
    sprintf(CR_Buffer, "DID=%s&", pDeviceDID);
    printf("QueryCmd= %s\nSize= %u byte\n", CR_Buffer, (UINT32)strlen(CR_Buffer));	
	
	// 加密查询命令到 Buffer
    if (0 > iPN_StringEnc(STRING_ENC_DEC_KEY, CR_Buffer, Buffer, SizeOfBuffer))
    {
        printf("WiPN_QueryByServer - iPN_StringEnc: Query Cmd Enc failed!\n");
		//free((void *)CR_Buffer);
		free((void *)Buffer);
		return WiPN_ERROR_iPNStringEncFailed;
    }

	int ret = 0;
    int QueryServerHandle = -1;
    short retryCount_RecvFrom = 0;
	srand((UINT32)time(NULL));
	
	short index = abs(rand() % QUERY_SERVER_NUMBER);
    while (1)
    {
        if (0 > QueryServerHandle)
        {      
            for (i = 0; i < QUERY_SERVER_NUMBER; i++)
            {
				index = (index + 1) % QUERY_SERVER_NUMBER;
				
				unsigned short SizeOfQueryCmd = strlen(Buffer);
				printf("send cmd to QueryServer, QueryServerDID[%d]= %s. SizeOfQueryCmd= %u byte. sending...\n", index, QueryServerDID[index], SizeOfQueryCmd);
				
				ret = NDT_PPCS_SendToByServer(QueryServerDID[index], Buffer, SizeOfQueryCmd, g_SendToMode, pServerString, pAES128Key);
				
				if (0 > ret)
                {
                    printf("send cmd to QueryServer failed!! ret= %d [%s]\n\n", ret, getErrorCodeInfo(ret));
					printf("retry to send ...\n\n");
                    continue;
                }
                else
                {
                    printf("send cmd to QueryServer Success!! \n");
					QueryServerHandle = ret;
                    break;
                }
            }
            if (0 > QueryServerHandle)
            {
                printf("WiPN_QueryByServer - Get QueryServerHandle failed! QueryServerDID[%d]= %s. ret= %d [%s]\n", index, QueryServerDID[index], ret, getErrorCodeInfo(ret));
                break;
            }
        }
        else
        {
			printf("Waiting for QueryServer response, please wait ...\n");        
            UINT16 SizeToRead = SizeOfBuffer;
			memset(Buffer, 0, SizeOfBuffer);
			
			ret = NDT_PPCS_RecvFrom(QueryServerHandle, Buffer, &SizeToRead, 3000); 
			
			// 记录 RecvFrom 返回的当前时间
			time_t Time_ServerRet = time(NULL);
			struct tm *ptm = localtime((const time_t *)&Time_ServerRet);
            
            if (0 > ret)
            {
				printf("WiPN_QueryByServer - NDT_PPCS_RecvFrom: QueryServerDID[%d]= %s. ret= %d. [%s]\n", index, QueryServerDID[index], ret, getErrorCodeInfo(ret));
				if (ptm)
				{
					printf("LocalTime: %d-%d-%d %02d:%02d:%02d\n", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
				}
				
                // -26 -27 -3
                if ((NDT_ERROR_NoAckFromPushServer == ret || NDT_ERROR_NoAckFromDevice == ret || NDT_ERROR_TimeOut == ret) && 3 > retryCount_RecvFrom)
                {
                    retryCount_RecvFrom++;
                    continue;
                }
                else if (NDT_ERROR_RemoteHandleClosed == ret) // -36
				{
					printf("WiPN_QueryByServer - QueryServer already call CloseHandle(). QueryServerDID[%d]= %s\n", index, QueryServerDID[index]);
				} 
                break;
            }
            else
            {
				// 解密接收到的数据, 输出到 CR_Buffer              
                if (0 > iPN_StringDnc(STRING_ENC_DEC_KEY, Buffer, CR_Buffer, SizeOf_CR_Buffer))
                {
                    printf("WiPN_QueryByServer - NDT_PPCS_RecvFrom - iPN_StringDnc: RecvFrom Data Dec failed! QueryServerDID[%d]= %s. \n", index, QueryServerDID[index]);
                    ret = WiPN_ERROR_iPNStringDncFailed;
                    break;
                }
				printf("\nFrom QueryServer: \n");
				printf("QueryServerDID[%d]= %s\n", index, QueryServerDID[index]);
				printf("QueryServerHandle= %d\n", QueryServerHandle);
				//printf("RecvFromData: %s\nSizeOfRecvFromData: %u byte\n", Buffer, (UINT32)strlen(Buffer));
				printf("Data: %s\nSizeOfData: %u byte\n", CR_Buffer, (UINT32)strlen(CR_Buffer));
				if (ptm)
				{
					printf("LocalTime: %d-%02d-%02d %02d:%02d:%02d\n", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
				}	
				
                // 拆分 CR_Buffer 获取 PostServerString , 其中 '&' 为截取结束标志符
                if (NULL != pPostServerString && 0 > GetStringItem(CR_Buffer, "Post", '&', pPostServerString, SizeOfPostServerString))
                {
                    printf("WiPN_QueryByServer - Get PostServerString failed!\n");
                    ret = WiPN_ERROR_GetPostServerStringItemFailed;
                }
				// 拆分 CR_Buffer 获取 SubscribeString 
                if (NULL != pSubscribeServerString && 0 > GetStringItem(CR_Buffer, "Subs", '&', pSubscribeServerString, SizeOfSubscribeServerString))
                {
                    printf("WiPN_QueryByServer - Get SubscribeServerString failed!\n");
                    ret = WiPN_ERROR_GetSubscribeServerStringItemFailed;
                }
				// 拆分 CR_Buffer 获取 UTCTString 
                memset(pUTCTString, 0, SizeOfUTCTString);
                if (0 > GetStringItem(CR_Buffer, "UTCT", '&', pUTCTString, SizeOfUTCTString))
                {
                    printf("WiPN_QueryByServer - Get UTCTString failed!\n");
                    ret = WiPN_ERROR_GetUTCTStringItemFailed;
                }
				else // g_Time_ServerRet 必须要与 UTCTString 时间同步更新
				{
					g_Time_ServerRet = Time_ServerRet;
				}
            } // ret > 0
            break;
        } // Handle > 0
    } // while
	if (0 <= QueryServerHandle)
	{
		NDT_PPCS_CloseHandle(QueryServerHandle);
		printf("WiPN_QueryByServer - NDT_PPCS_CloseHandle(%d)\n\n", QueryServerHandle);
	}
	//if (CR_Buffer)
	//	free((void *)CR_Buffer);
	if (Buffer)
		free((void *)Buffer);
	
    return ret;
}

#if SUBSCRIBE_FLAG
//// WiPN_Subscribe 订阅
INT32 WiPN_Subscribe(const CHAR *pSubscribeServerString,
                     const CHAR *pSubCmd,
                     CHAR *pRETString,
                     UINT32 SizeOfRETString,
                     CHAR *pUTCTString,
                     UINT32 SizeOfUTCTString)
{
	if (   !pSubscribeServerString 
		|| !pSubCmd 
		|| !pRETString 
		|| !pUTCTString)
	{
		return WiPN_ERROR_InvalidParameter;
	}
    if (   0 == strlen(pSubscribeServerString) 
		|| 0 == strlen(pSubCmd) 
		|| 0 == SizeOfRETString
		|| 0 == strlen(pUTCTString)
		|| 0 == SizeOfUTCTString)
	{
		return WiPN_ERROR_InvalidParameter;
	}
	//// -------------------------- 获取 SubscribeServer DID -------------------------
    // 获取 SubscribeServerString 中 SubscribeServer 的个数
    unsigned short NumberOfSubscribeServer = 0;
    unsigned short SizeOfDID = 0;
    if (0 > pharse_number(pSubscribeServerString, &NumberOfSubscribeServer, &SizeOfDID) || 0 == NumberOfSubscribeServer)
	{
		return WiPN_ERROR_GetNumberFromSubscribeServerStringFailed;
	}
    SizeOfDID = (SizeOfDID/4+1)*4; // DID 之间保持足够间隔
    
    // 根据 SubscribeServer DID 的个数分配内存空间
    CHAR *pSubscribeServerDID = (CHAR *)malloc(SizeOfDID*NumberOfSubscribeServer);
    if (!pSubscribeServerDID)
    {
        printf("WiPN_Subscribe - Malloc SubscribeServerDID Buf failed!!\n");
        return WiPN_ERROR_MallocFailed;
    }
    memset(pSubscribeServerDID, '\0', SizeOfDID*NumberOfSubscribeServer);
    
    // 获取 SubscribeServerString 中的 SubscribeServer DID, 并保存
    const CHAR *pDID = NULL;
    int i = 0;
    for ( ; i < NumberOfSubscribeServer; i++)
    {
        pDID = pharse_DID(pSubscribeServerString, i);
        if (!pDID)
        {
            free((void *)pSubscribeServerDID);
            return WiPN_ERROR_GetDIDFromSubscribeServerStringFailed;
        }
        memcpy(&pSubscribeServerDID[SizeOfDID*i], pDID, strlen(pDID));
        //printf("SubscribeServerDID[%d]= %s\n", i, &pSubscribeServerDID[SizeOfDID*i]);
        free((void*)pDID);
		pDID = NULL;
    }
	//// -------------------------------------------------------------------------
    // 计算 SubscribeCmd 指令大小
    UINT32 Length_SubscribeCmd = strlen("UTCT=0x&")
										+strlen(pSubCmd)
										+strlen(pUTCTString); 
	// NDT 一次最大只能发送1280字节数据，根据加密算法，加密之前的数据不能超过630字节
    UINT32 MaxSizeOfCmd = 630;
    if (MaxSizeOfCmd < Length_SubscribeCmd)
    {
        printf("WiPN_Subscribe - Length Of SubscribeCmd is Exceed %d bytes!!\n", MaxSizeOfCmd);
        free((void *)pSubscribeServerDID);
        return WiPN_ERROR_ExceedMaxSize;
    }
	
	// 存放: 1.Cmd:加密前的指令
	//		2.Response: 解密后的服务器回应的数据
	unsigned short SizeOf_CR_Buffer = (Length_SubscribeCmd/4+1)*4;
	char *CR_Buffer = (char *)malloc(SizeOf_CR_Buffer);
	if (!CR_Buffer)
	{
		printf("WiPN_Subscribe - Malloc Subscribe Cmd Buf failed!!\n");
		free((void *)pSubscribeServerDID);
		return WiPN_ERROR_MallocFailed;
	}   
	
	// 存放: 1.Cmd: 加密后的指令(Cmd加密后长度会翻倍)
	//		2.Response: 未解密的服务器回应的数据(RecvFrom 所接收到的数据)
	unsigned short SizeOfBuffer = SizeOf_CR_Buffer*2;
	char *Buffer = (char *)malloc(SizeOfBuffer);
	if (!Buffer)
	{
		printf("WiPN_Subscribe - Malloc Buffer failed!!\n");
		free((void *)pSubscribeServerDID);
		free((void *)CR_Buffer);
		return WiPN_ERROR_MallocFailed;
	}
	
    int ret = 0;
    int SubscribeServerHandle = -1;
	short retryCount_RecvFrom = 0;
    srand((UINT32)time(NULL));
	unsigned short PrintfFlag = 0;
	
    short index = 0;
    unsigned short j = abs(rand() % NumberOfSubscribeServer);
    while (1)
    {
        if (0 > SubscribeServerHandle)
        {
            for (i = 0; i < NumberOfSubscribeServer; i++)
            {
                j = (j + 1) % NumberOfSubscribeServer;
                index = SizeOfDID * j;

                // 计算 UTCT 时间
                UINT32 UTCT_Subscribe = time(NULL) - g_Time_ServerRet + strtol(pUTCTString, NULL, 16);
                
                // 格式化 Subscribe 指令到 CR_Buffer
                memset(CR_Buffer, 0, SizeOf_CR_Buffer);
                SNPRINTF(CR_Buffer, SizeOf_CR_Buffer, "%sUTCT=0x%X&", pSubCmd, (UINT32)UTCT_Subscribe);
				if (0 == PrintfFlag)
				{
					PrintfFlag = 1;
					printf("SubscribeCmd= %s\nSubscribeCmdSize= %u byte (Not Encrypted Size)\n\n", CR_Buffer, (UINT32)strlen(CR_Buffer));
				}
				
                // 加密 Subscribe 指令            
                if (0 > iPN_StringEnc(STRING_ENC_DEC_KEY, CR_Buffer, Buffer, SizeOfBuffer))
                {
                    printf("WiPN_Subscribe - iPN_StringEnc: Subscribe Cmd Enc failed!\n");
                    free((void *)pSubscribeServerDID);
                    free((void *)CR_Buffer);
                    free((void *)Buffer);
                    return WiPN_ERROR_iPNStringEncFailed;
                }
				
				UINT16 SizeOfSubscribeCmd = strlen(Buffer);
                printf("send cmd to SubscribeServer, SubscribeServerDID[%d]= %s. SizeOfSubscribeCmd= %d byte (Encrypted Size). sending...\n", j, &pSubscribeServerDID[index], SizeOfSubscribeCmd);
                
                ret = NDT_PPCS_SendTo(&pSubscribeServerDID[index], Buffer, SizeOfSubscribeCmd, g_SendToMode);
				
                if (0 > ret)
                {
                    printf("send cmd to SubscribeServer failed! ret= %d [%s]\n", ret, getErrorCodeInfo(ret));
                    printf("retry to send ...\n\n");
                    continue;
                }
                else
                {
					SubscribeServerHandle = ret;
                    printf("send cmd to SubscribeServer success! \n");
                    break;
                }
            }
            if (0 > SubscribeServerHandle)
            {
                printf("WiPN_Subscribe - Get SubscribeServerHandle failed! SubscribeServerDID[%d]= %s. ret= %d [%s]\n", j, &pSubscribeServerDID[index], ret, getErrorCodeInfo(ret));
                break;
            }
        }
        else
        {
            printf("Waiting for SubscribeServer response, please wait ...\n");
			UINT16 SizeToRead = SizeOfBuffer;
            memset(Buffer, 0, SizeOfBuffer);
            
			ret = NDT_PPCS_RecvFrom(SubscribeServerHandle, Buffer, &SizeToRead, 3000);
            
            // 记录 recvFrom 返回的当前时间
            time_t Time_ServerRet = time(NULL);         
			struct tm *ptm = localtime((const time_t *)&Time_ServerRet); 
            if (0 > ret)
            {
				printf("WiPN_Subscribe - NDT_PPCS_RecvFrom: SubscribeServerDID[%d]= %s. ret= %d. [%s]\n", j, &pSubscribeServerDID[index], ret, getErrorCodeInfo(ret));
               	if (ptm)
				{
					printf("LocalTime: %d-%02d-%02d %02d:%02d:%02d]\n", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
				}  
   
                // -26 -27 -3
                if ((NDT_ERROR_NoAckFromPushServer == ret || NDT_ERROR_NoAckFromDevice == ret || NDT_ERROR_TimeOut == ret) && 3 > retryCount_RecvFrom)
                {
                    retryCount_RecvFrom++;
                    continue;
                }
                else if (NDT_ERROR_RemoteHandleClosed == ret) // -36
				{
					printf("WiPN_Subscribe - SubscribeServer already call CloseHandle(). SubscribeServerDID[%d]= %s\n", j, &pSubscribeServerDID[index]);
				} 
                break;
            }
            else
            {
                // 解密接收到的数据，输出到 CR_Buffer             
                if (0 > iPN_StringDnc(STRING_ENC_DEC_KEY, Buffer, CR_Buffer, SizeOf_CR_Buffer))
                {
                    printf("WiPN_Subscribe - NDT_PPCS_RecvFrom - iPN_StringDnc: RecvFrom Data Dec failed! SubscribeServerDID[%d]= %s.\n", j, &pSubscribeServerDID[index]);
                    ret = WiPN_ERROR_iPNStringDncFailed;
                    break ;
                }
				
				printf("\nFrom SubscribeServer: \n");
				printf("SubscribeServerDID[%d]: %s\n", j, &pSubscribeServerDID[index]);
				printf("SubscribeServerHandle= %d\n", SubscribeServerHandle);
				//printf("RecvFromData: %s\nSizeOfRecvFromData: %u byte\n", Buffer, (UINT32)strlen(Buffer));
				printf("Data: %s\nSizeOfData: %u byte\n", CR_Buffer, (UINT32)strlen(CR_Buffer));              
                if (ptm)
				{
					printf("LocalTime: %d-%02d-%02d %02d:%02d:%02d\n", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
				}  
              
                if (0 > GetStringItem(CR_Buffer, "RET", '&', pRETString, SizeOfRETString))
                {
                    printf("WiPN_Subscribe - Get RETString failed!\n");
                    ret = WiPN_ERROR_GetRETStringItemFailed;
                    break;
                }
                if (0 > GetStringItem(CR_Buffer, "UTCT", '&', pUTCTString, SizeOfUTCTString))
                {
                    printf("WiPN_Subscribe - Get UTCTString failed!\n");
                    ret =  WiPN_ERROR_GetUTCTStringItemFailed;
                    break;
                }
                else // g_Time_ServerRet 必须要与 UTCTString 时间同步更新
				{
					g_Time_ServerRet = Time_ServerRet;
				}        
                break;
            } // ret > 0
        } // Handle > 0
    } // while (1)
    
    if (0 <= SubscribeServerHandle)
	{
		NDT_PPCS_CloseHandle(SubscribeServerHandle);
		printf("WiPN_Subscribe - NDT_PPCS_CloseHandle(%d)\n\n", SubscribeServerHandle);
	}
	if (pSubscribeServerDID)
		free((void *)pSubscribeServerDID);
    if (CR_Buffer)
		free((void *)CR_Buffer);	
    if (Buffer)
		free((void *)Buffer);
    
    return ret;
} // WiPN_Subscribe

//// WiPN_SubscribeByServer 订阅
INT32 WiPN_SubscribeByServer(const CHAR *pServerString,
							const CHAR *pAES128Key,
							const CHAR *pSubscribeServerString,
							const CHAR *pSubCmd,
							CHAR *pRETString,
							UINT32 SizeOfRETString,
							CHAR *pUTCTString,
							UINT32 SizeOfUTCTString)
{
    if (   !pServerString
		|| !pAES128Key
		|| !pSubscribeServerString 
		|| !pSubCmd 
		|| !pRETString 
		|| !pUTCTString )
	{
		return WiPN_ERROR_InvalidParameter;
	}
    if (   0 == strlen(pServerString) 
		|| 0 == strlen(pAES128Key) 
		|| 0 == strlen(pSubscribeServerString) 
		|| 0 == strlen(pSubCmd) 
		|| 0 == strlen(pUTCTString)
		|| 0 == SizeOfRETString
		|| 0 == SizeOfUTCTString)
	{
		return WiPN_ERROR_InvalidParameter;
	}
	//// -------------------------- 获取 SubscribeServer DID -------------------------
    // 获取 SubscribeServerString 中 SubscribeServer 的个数
    unsigned short NumberOfSubscribeServer = 0;
    unsigned short SizeOfDID = 0;
    if (0 > pharse_number(pSubscribeServerString, &NumberOfSubscribeServer, &SizeOfDID) || 0 == NumberOfSubscribeServer)
	{
		return WiPN_ERROR_GetNumberFromSubscribeServerStringFailed;
	}
    SizeOfDID = (SizeOfDID/4+1)*4; // DID 之间保持足够间隔
    
    // 根据 SubscribeServer DID 的个数分配内存空间
    CHAR *pSubscribeServerDID = (CHAR *)malloc(SizeOfDID*NumberOfSubscribeServer);
    if (!pSubscribeServerDID)
    {
        printf("WiPN_SubscribeByServer - Malloc SubscribeServerDID Buf failed!!\n");
        return WiPN_ERROR_MallocFailed;
    }
    memset(pSubscribeServerDID, '\0', SizeOfDID*NumberOfSubscribeServer);
    
    // 获取 SubscribeServerString 中的 SubscribeServer DID, 并保存
    const CHAR *pDID = NULL;
    int i = 0;
    for ( ; i < NumberOfSubscribeServer; i++)
    {
        pDID = pharse_DID(pSubscribeServerString, i);
        if (!pDID)
        {
            free((void *)pSubscribeServerDID);
            return WiPN_ERROR_GetDIDFromSubscribeServerStringFailed;
        }
        memcpy(&pSubscribeServerDID[SizeOfDID*i], pDID, strlen(pDID));
        //printf("SubscribeServerDID[%d]= %s\n", i, &pSubscribeServerDID[SizeOfDID*i]);
        free((void*)pDID);
		pDID = NULL;
    }
	//// -------------------------------------------------------------------------
	
    // 计算 SubscribeCmd 指令大小
    UINT32 Length_SubscribeCmd = strlen("UTCT=0x&")
										+strlen(pSubCmd)
										+strlen(pUTCTString);
	// NDT 一次最大只能发送1280字节数据，根据加密算法，加密之前的数据不能超过630字节
    UINT32 MaxSizeOfCmd = 630;
    if (MaxSizeOfCmd < Length_SubscribeCmd)
    {
        printf("WiPN_SubscribeByServer - Length Of SubscribeCmd is Exceed %d bytes!!\n", MaxSizeOfCmd);
        free((void *)pSubscribeServerDID);
        return WiPN_ERROR_ExceedMaxSize;
    }
	
	// 存放: 1.Cmd:加密前的指令
	//		2.Response: 解密后的服务器回应的数据
	unsigned short SizeOf_CR_Buffer = (Length_SubscribeCmd/4+1)*4;
	char *CR_Buffer = (char *)malloc(SizeOf_CR_Buffer);
	if (!CR_Buffer)
	{
		printf("WiPN_SubscribeByServer - Malloc SubscribeCmd Buf failed!!\n");
		free((void *)pSubscribeServerDID);
		return WiPN_ERROR_MallocFailed;
	}   
	
	// 存放: 1.Cmd: 加密后的指令(Cmd加密后长度会翻倍)
	//		2.Response: 未解密的服务器回应的数据(RecvFrom 所接收到的数据)
	unsigned short SizeOfBuffer = SizeOf_CR_Buffer*2;
	char *Buffer = (char *)malloc(SizeOfBuffer);
	if (!Buffer)
	{
		printf("WiPN_SubscribeByServer - Malloc Buffer failed!!\n");
		free((void *)pSubscribeServerDID);
		free((void *)CR_Buffer);
		return WiPN_ERROR_MallocFailed;
	}
	
	int ret = 0;
    int SubscribeServerHandle = -1;
    short retryCount_RecvFrom = 0;
	srand((UINT32)time(NULL));
	unsigned short PrintfFlag = 0;
	
    short index = 0;
    unsigned short j = abs(rand() % NumberOfSubscribeServer);
    while (1)
    {
        if (0 > SubscribeServerHandle)
        {
            for (i = 0; i < NumberOfSubscribeServer; i++)
            {
                j = (j + 1) % NumberOfSubscribeServer;
                index = SizeOfDID * j;

                // 计算 UTCT 时间
                UINT32 UTCT_Subscribe = time(NULL) - g_Time_ServerRet + strtol(pUTCTString, NULL, 16);
                
                // 格式化 Subscribe 指令到 CR_Buffer
                memset(CR_Buffer, 0, SizeOf_CR_Buffer);
                SNPRINTF(CR_Buffer, SizeOf_CR_Buffer, "%sUTCT=0x%X&", pSubCmd, (UINT32)UTCT_Subscribe);
				if (0 == PrintfFlag)
				{
					PrintfFlag = 1;
					printf("SubscribeCmd= %s\nSubscribeCmdSize= %u byte (Not Encrypted Size)\n\n", CR_Buffer, (UINT32)strlen(CR_Buffer));
				}
				
                // 加密 Subscribe 指令, 输出到 Buffer               
                if (0 > iPN_StringEnc(STRING_ENC_DEC_KEY, CR_Buffer, Buffer, SizeOfBuffer))
                {
                    printf("WiPN_SubscribeByServer - iPN_StringEnc: SubscribeCmd Enc failed!\n");
                    free((void *)pSubscribeServerDID);
                    free((void *)CR_Buffer);
                    free((void *)Buffer);
                    return WiPN_ERROR_iPNStringEncFailed;
                }
				
				UINT16 SizeOfSubscribeCmd = strlen(Buffer);
                printf("send cmd to SubscribeServer, SubscribeServerDID[%d]= %s. SizeOfSubscribeCmd= %u byte (Encrypted Size). sending...\n", j, &pSubscribeServerDID[index], SizeOfSubscribeCmd);
                
				// 发送加密之后的订阅指令到 SubscribeServer
				ret = NDT_PPCS_SendToByServer(&pSubscribeServerDID[index], Buffer, SizeOfSubscribeCmd, g_SendToMode, pServerString, pAES128Key);
                
                if (0 > ret)
                {
                    printf("send cmd to SubscribeServer failed! ret= %d [%s]\n", ret, getErrorCodeInfo(ret));
                    printf("retry to send ...\n\n");
                    continue;
                }
                else
                {
					SubscribeServerHandle = ret;
                    printf("send cmd to SubscribeServer success! \n");
                    break;
                }
            }
            if (0 > SubscribeServerHandle)
            {
                printf("WiPN_SubscribeByServer - Get SubscribeServerHandle failed! SubscribeServerDID[%d]= %s. ret= %d [%s]\n", j, &pSubscribeServerDID[index], ret, getErrorCodeInfo(ret));
                break;
            }
        }
        else
        {
            printf("Waiting for SubscribeServer response, please wait ...\n");
			UINT16 SizeToRead = SizeOfBuffer;
            memset(Buffer, 0, SizeOfBuffer);
			
            ret = NDT_PPCS_RecvFrom(SubscribeServerHandle, Buffer, &SizeToRead, 3000);
            
            // 记录 recvFrom 返回的时间
            time_t Time_ServerRet = time(NULL);          
			struct tm *ptm = localtime((const time_t *)&Time_ServerRet);  
            
			if (0 > ret)
            {      
				printf("WiPN_SubscribeByServer - NDT_PPCS_RecvFrom: SubscribeServerDID[%d]= %s. ret= %d. [%s]\n", j, &pSubscribeServerDID[index], ret, getErrorCodeInfo(ret));
				if (ptm)
				{
					printf("LocalTime: %d-%02d-%02d %02d:%02d:%02d\n", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
				} 
       
                // -26 -27 -3
                if ((NDT_ERROR_NoAckFromPushServer == ret || NDT_ERROR_NoAckFromDevice == ret || NDT_ERROR_TimeOut == ret) && 3 > retryCount_RecvFrom)
                {
                    retryCount_RecvFrom++;
                    continue;
                }
                else if (NDT_ERROR_RemoteHandleClosed == ret) // -36
				{
					printf("WiPN_SubscribeByServer - SubscribeServer already call CloseHandle(). SubscribeServerDID[%d]= %s.\n", j, &pSubscribeServerDID[index]);
				} 
                break;
            }
            else
            {
                // 解密接收到的数据, 输出到 CR_Buffer                
                if (0 > iPN_StringDnc(STRING_ENC_DEC_KEY, Buffer, CR_Buffer, SizeOf_CR_Buffer))
                {
                    printf("WiPN_SubscribeByServer - NDT_PPCS_RecvFrom - iPN_StringDnc: RecvFrom Data Dec failed! SubscribeServerDID[%d]= %s.\n", j, &pSubscribeServerDID[index]);
                    ret = WiPN_ERROR_iPNStringDncFailed;
                    break ;
                }
				printf("\nFrom SubscribeServer: \n");
				printf("SubscribeServerDID[%d]: %s\n", j, &pSubscribeServerDID[index]);
				printf("SubscribeServerHandle= %d\n", SubscribeServerHandle);
				//printf("RecvFromData: %s\nSizeOfRecvFromData: %u byte\n", Buffer, (UINT32)strlen(Buffer));
				printf("Data: %s\nSizeOfData: %u byte\n", CR_Buffer, (UINT32)strlen(CR_Buffer));           
                if (ptm)
				{
					printf("LocalTime: %d-%02d-%02d %02d:%02d:%02d\n", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
				}  
            
                if (0 > GetStringItem(CR_Buffer, "RET", '&', pRETString, SizeOfRETString))
                {
                    printf("WiPN_SubscribeByServer - Get RETString failed!\n");
                    ret = WiPN_ERROR_GetRETStringItemFailed;
                    break;
                }
                if (0 > GetStringItem(CR_Buffer, "UTCT", '&', pUTCTString, SizeOfUTCTString))
                {
                    printf("WiPN_SubscribeByServer - Get UTCTString failed!\n");
                    ret =  WiPN_ERROR_GetUTCTStringItemFailed;
                    break;
                }
                else // g_Time_ServerRet 必须要与 UTCTString 时间同步更新
				{
					g_Time_ServerRet = Time_ServerRet;
				}        
                break;
            } // ret > 0
        } // Handle > 0
    } // while (1)
    
    if (0 <= SubscribeServerHandle)
	{
		NDT_PPCS_CloseHandle(SubscribeServerHandle);
		printf("WiPN_SubscribeByServer - NDT_PPCS_CloseHandle(%d)\n\n", SubscribeServerHandle);
	}
	if (pSubscribeServerDID)
		free((void *)pSubscribeServerDID);
	if (CR_Buffer)
		free((void *)CR_Buffer);
	if (Buffer)
		free((void *)Buffer);
    
    return ret;
} // WiPN_Subscribe


//// WiPN_UnSubscribe 取消订阅
INT32 WiPN_UnSubscribe(const CHAR *pSubscribeServerString,
                     const CHAR *pSubCmd,
                     CHAR *pRETString,
                     UINT32 SizeOfRETString,
                     CHAR *pUTCTString,
                     UINT32 SizeOfUTCTString)
{
	if (   !pSubscribeServerString 
		|| !pSubCmd 
		|| !pRETString 
		|| !pUTCTString)
	{
		return WiPN_ERROR_InvalidParameter;
	}
    if (   0 == strlen(pSubscribeServerString) 
		|| 0 == strlen(pSubCmd) 
		|| 0 == SizeOfRETString
		|| 0 == strlen(pUTCTString)
		|| 0 == SizeOfUTCTString)
	{
		return WiPN_ERROR_InvalidParameter;
	}
	//// -------------------------- 获取 SubscribeServer DID -------------------------
    // 获取 SubscribeServerString 中 SubscribeServer 的个数
    unsigned short NumberOfSubscribeServer = 0;
    unsigned short SizeOfDID = 0;
    if (0 > pharse_number(pSubscribeServerString, &NumberOfSubscribeServer, &SizeOfDID) || 0 == NumberOfSubscribeServer)
	{
		return WiPN_ERROR_GetNumberFromSubscribeServerStringFailed;
	} 
    SizeOfDID = (SizeOfDID/4+1)*4; // DID 之间保持足够间隔
    
    // 根据 SubscribeServer DID 的个数分配内存空间
    CHAR *pSubscribeServerDID = (CHAR *)malloc(SizeOfDID*NumberOfSubscribeServer);
    if (!pSubscribeServerDID)
    {
        printf("WiPN_UnSubscribe - Malloc SubscribeServerDID Buf failed!!\n");
        return WiPN_ERROR_MallocFailed;
    }
    memset(pSubscribeServerDID, '\0', SizeOfDID*NumberOfSubscribeServer);
    
    // 获取 SubscribeServerString 中的 SubscribeServer DID, 并保存
    const CHAR *pDID = NULL;
    int i = 0;
    for (; i < NumberOfSubscribeServer; i++)
    {
        pDID = pharse_DID(pSubscribeServerString, i);
        if (!pDID)
        {
            free((void *)pSubscribeServerDID);
            return WiPN_ERROR_GetDIDFromSubscribeServerStringFailed;
        }
        memcpy(&pSubscribeServerDID[SizeOfDID*i], pDID, strlen(pDID));
        //printf("SubscribeServerDID[%d]= %s\n", i, &pSubscribeServerDID[SizeOfDID*i]);
        free((void*)pDID);
		pDID = NULL;
    }
    //// ---------------------------------------------------------------------------
    // 计算 UnSubscribeCmd 指令大小
    UINT32 Length_UnSubscribeCmd = 	strlen("UTCT=0x&ACT=UnSubscribe&")
											+strlen(pSubCmd)
											+strlen(pUTCTString);
    
	// NDT 一次最大只能发送1280字节数据，根据加密算法，加密之前的数据不能超过630字节
    UINT32 MaxSizeOfCmd = 630;
    if (MaxSizeOfCmd < Length_UnSubscribeCmd)
    {
        printf("WiPN_UnSubscribe - Length Of UnSubscribeCmd is Exceed %d bytes!!\n", MaxSizeOfCmd);
        free((void *)pSubscribeServerDID);
        return WiPN_ERROR_ExceedMaxSize;
    }
	
	// 存放: 1.Cmd:加密前的指令
	//		2.Response: 解密后的服务器回应的数据
	unsigned short SizeOf_CR_Buffer = (Length_UnSubscribeCmd/4+1)*4;
	char *CR_Buffer = (char *)malloc(SizeOf_CR_Buffer);
	if (!CR_Buffer)
	{
		printf("WiPN_UnSubscribe - Malloc UnSubscribeCmd Buf failed!!\n");
		free((void *)pSubscribeServerDID);
		return WiPN_ERROR_MallocFailed;
	}   
	
	// 存放: 1.Cmd: 加密后的指令(Cmd加密后长度会翻倍)
	//		2.Response: 未解密的服务器回应的数据(RecvFrom 所接收到的数据)
	unsigned short SizeOfBuffer = SizeOf_CR_Buffer*2;
	char *Buffer = (char *)malloc(SizeOfBuffer);
	if (!Buffer)
	{
		printf("WiPN_UnSubscribe - Malloc Buffer failed!!\n");
		free((void *)pSubscribeServerDID);
		free((void *)CR_Buffer);
		return WiPN_ERROR_MallocFailed;
	}
    	
    int ret = 0;
    int SubscribeServerHandle = -1;
	short retryCount_RecvFrom = 0;
    srand((UINT32)time(NULL));
	unsigned short PrintfFlag = 0;
	
    short index = 0;
    unsigned short j = abs(rand() % NumberOfSubscribeServer);
    while (1)
    {
        if (0 > SubscribeServerHandle)
        {
            for (i = 0; i < NumberOfSubscribeServer; i++)
            {
                j = (j + 1) % NumberOfSubscribeServer;
                index = SizeOfDID * j;
                
                // 计算 UTCT 时间
                UINT32 UTCT_UnSubscribe = time(NULL) - g_Time_ServerRet + strtol(pUTCTString, NULL, 16);
                
                // 格式化 UnSubscribe 指令到 CR_Buffer
                memset(CR_Buffer, 0, SizeOf_CR_Buffer);
                SNPRINTF(CR_Buffer, SizeOf_CR_Buffer, "%sUTCT=0x%X&ACT=UnSubscribe&", pSubCmd, (UINT32)UTCT_UnSubscribe); 
				if (0 == PrintfFlag)
				{
					PrintfFlag = 1;
					printf("UnSubscribeCmd= %s\n", CR_Buffer);
					printf("UnSubscribeCmdSize= %u byte (Not Encrypted Size)\n\n", (UINT32)strlen(CR_Buffer));	
				}		
				
                // 加密 UnSubscribe 指令到 Buffer           
                if (0 > iPN_StringEnc(STRING_ENC_DEC_KEY, CR_Buffer, Buffer, SizeOfBuffer))
                {
                    printf("WiPN_UnSubscribe - iPN_StringEnc failed!\n");
                    free((void *)pSubscribeServerDID);
                    free((void *)CR_Buffer);
                    free((void *)Buffer);
                    return WiPN_ERROR_iPNStringEncFailed;
                }
				             
				UINT16 SizeOfUnSubscribeCmd = strlen(Buffer);
                printf("send cmd to SubscribeServer, SubscribeServerDID[%d]= %s. SizeOfUnSubscribeCmd= %d byte (Encrypted Size). sending...\n", j, &pSubscribeServerDID[index], SizeOfUnSubscribeCmd);
				
                ret = NDT_PPCS_SendTo(&pSubscribeServerDID[index], Buffer, SizeOfUnSubscribeCmd, g_SendToMode);
                
				if (0 > ret)
                {
                    printf("send cmd to SubscribeServer failed! ret= %d [%s]\n", ret, getErrorCodeInfo(ret));
                    printf("retry to send ...\n\n");
                    continue;
                }
                else
                {
					SubscribeServerHandle = ret;
                    printf("send cmd to SubscribeServer success! \n");
                    break;
                }
            }
            if (0 > SubscribeServerHandle)
            {
                printf("WiPN_UnSubscribe - Get SubscribeServerHandle failed! SubscribeServerDID[%d]= %s. ret= %d [%s]\n", j, &pSubscribeServerDID[index], ret, getErrorCodeInfo(ret));
                break;
            }
        }
        else
        {
            printf("Waiting for SubscribeServer response, please wait ...\n");         
			UINT16 SizeToRead = SizeOfBuffer;
			memset(Buffer, 0, SizeOfBuffer);
			
            ret = NDT_PPCS_RecvFrom(SubscribeServerHandle, Buffer, &SizeToRead, 3000);
            
            // 记录 recvFrom 返回的当前时间
            time_t Time_ServerRet = time(NULL);          			
			struct tm *ptm = localtime((const time_t *)&Time_ServerRet);

            if (0 > ret)
            {
				printf("WiPN_UnSubscribe - NDT_PPCS_RecvFrom: SubscribeServerDID[%d]= %s. ret= %d. [%s]\n", j, &pSubscribeServerDID[index], ret, getErrorCodeInfo(ret));
				if (ptm)
				{
					printf("LocalTime: %d-%02d-%02d %02d:%02d:%02d\n", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
				} 
              
                // -26 -27 -3
                if ((NDT_ERROR_NoAckFromPushServer == ret 
					|| NDT_ERROR_NoAckFromDevice == ret 
					|| NDT_ERROR_TimeOut == ret) 
					&& 3 > retryCount_RecvFrom)
                {
                    retryCount_RecvFrom++;
                    continue;
                }
                else if (NDT_ERROR_RemoteHandleClosed == ret) // -36
				{
					printf("WiPN_UnSubscribe - SubscribeServer already call CloseHandle().  SubscribeServerDID[%d]= %s\n", j, &pSubscribeServerDID[index]);
				}  
                break;
            }
            else
            {
                // 解密接收到的数据, 输出到 CR_Buffer               
                if (0 > iPN_StringDnc(STRING_ENC_DEC_KEY, Buffer, CR_Buffer, SizeOf_CR_Buffer))
                {
                    printf("WiPN_UnSubscribe - NDT_PPCS_RecvFrom: iPN_StringDnc failed! SubscribeServerDID[%d]= %s.\n", j, &pSubscribeServerDID[index]);
                    ret = WiPN_ERROR_iPNStringDncFailed;
                    break ;
                }  
				printf("\nFrom SubscribeServer: \n");
				printf("SubscribeServerDID[%d]: %s\n", j, &pSubscribeServerDID[index]);
				printf("SubscribeServerHandle= %d\n", SubscribeServerHandle);
				//printf("RecvFromData: %s\nSizeOfRecvFromData: %u byte\n", Buffer, (UINT32)strlen(Buffer));
				printf("Data: %s\nSizeOfData: %u byte\n", CR_Buffer, (UINT32)strlen(CR_Buffer));             
                if (ptm)
				{
					printf("LocalTime: %d-%02d-%02d %02d:%02d:%02d\n", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
				}   

                if (0 > GetStringItem(CR_Buffer, "RET", '&', pRETString, SizeOfRETString))
                {
                    printf("WiPN_UnSubscribe - Get RETString failed!\n");
                    ret = WiPN_ERROR_GetRETStringItemFailed;
                    break;
                }
                if (0 > GetStringItem(CR_Buffer, "UTCT", '&', pUTCTString, SizeOfUTCTString))
                {
                    printf("WiPN_UnSubscribe - Get UTCTString failed!\n");
                    ret =  WiPN_ERROR_GetUTCTStringItemFailed;
                    break;
                }
                else // g_Time_ServerRet 必须要与 UTCTString 时间同步更新
				{
					g_Time_ServerRet = Time_ServerRet;
				}           
                break;
            } // ret > 0
        } // Handle > 0
    } // while (1)
    
    if (0 <= SubscribeServerHandle)
	{
		NDT_PPCS_CloseHandle(SubscribeServerHandle);
		printf("WiPN_UnSubscribe - NDT_PPCS_CloseHandle(%d)\n\n", SubscribeServerHandle);
	}
	if (pSubscribeServerDID)
		free((void *)pSubscribeServerDID);
	if (CR_Buffer)
		free((void *)CR_Buffer);
	if (Buffer)
		free((void *)Buffer);
    
    return ret;
} // WiPN_UnSubscribe

//// WiPN_UnSubscribe 取消订阅
INT32 WiPN_UnSubscribeByServer(	const CHAR *pServerString,
								const CHAR *pAES128Key,
								const CHAR *pSubscribeServerString,
								const CHAR *pSubCmd,
								CHAR *pRETString,
								UINT32 SizeOfRETString,
								CHAR *pUTCTString,
								UINT32 SizeOfUTCTString)
{
    if (   !pServerString
		|| !pAES128Key
		|| !pSubscribeServerString 
		|| !pSubCmd 
		|| !pRETString 
		|| !pUTCTString )
	{
		return WiPN_ERROR_InvalidParameter;
	}
    if (   0 == strlen(pServerString) 
		|| 0 == strlen(pAES128Key) 
		|| 0 == strlen(pSubscribeServerString) 
		|| 0 == strlen(pSubCmd) 
		|| 0 == strlen(pUTCTString)
		|| 0 == SizeOfRETString
		|| 0 == SizeOfUTCTString)
	{
		return WiPN_ERROR_InvalidParameter;
	}
	//// -------------------------- 获取 SubscribeServer DID -------------------------
    // 获取 SubscribeServerString 中 SubscribeServer 的个数
    unsigned short NumberOfSubscribeServer = 0;
    unsigned short SizeOfDID = 0;
    if (0 > pharse_number(pSubscribeServerString, &NumberOfSubscribeServer, &SizeOfDID) || 0 == NumberOfSubscribeServer)
	{
		return WiPN_ERROR_GetNumberFromSubscribeServerStringFailed;
	} 
    SizeOfDID = (SizeOfDID/4+1)*4; // DID 之间保持足够间隔
    
    // 根据 SubscribeServer DID 的个数分配内存空间
    CHAR *pSubscribeServerDID = (CHAR *)malloc(SizeOfDID*NumberOfSubscribeServer);
    if (!pSubscribeServerDID)
    {
        printf("WiPN_UnSubscribeByServer - Malloc SubscribeServerDID Buf failed!!\n");
        return WiPN_ERROR_MallocFailed;
    }
    memset(pSubscribeServerDID, '\0', SizeOfDID*NumberOfSubscribeServer);
    
    // 获取 SubscribeServerString 中的 SubscribeServer DID, 并保存
    const CHAR *pDID = NULL;
    int i = 0;
    for (; i < NumberOfSubscribeServer; i++)
    {
        pDID = pharse_DID(pSubscribeServerString, i);
        if (!pDID)
        {
            free((void *)pSubscribeServerDID);
            return WiPN_ERROR_GetDIDFromSubscribeServerStringFailed;
        }
        memcpy(&pSubscribeServerDID[SizeOfDID*i], pDID, strlen(pDID));
        //printf("SubscribeServerDID[%d]= %s\n", i, &pSubscribeServerDID[SizeOfDID*i]);
        free((void*)pDID);
		pDID = NULL;
    }
    //// ---------------------------------------------------------------------------
    // 计算 UnSubscribeCmd 指令大小
    UINT32 Length_UnSubscribeCmd = strlen("UTCT=0x&ACT=UnSubscribe&")
										+strlen(pSubCmd)
										+strlen(pUTCTString); 
	// NDT 一次最大只能发送1280字节数据，根据加密算法，加密之前的数据不能超过630字节
    UINT32 MaxSizeOfCmd = 630;
    if (MaxSizeOfCmd < Length_UnSubscribeCmd)
    {
        printf("WiPN_UnSubscribeByServer - Length Of UnSubscribeCmd is Exceed %d bytes!!\n", MaxSizeOfCmd);
        free((void *)pSubscribeServerDID);
        return WiPN_ERROR_ExceedMaxSize;
    }
	
	// 存放: 1.Cmd:加密前的指令
	//		2.Response: 解密后的服务器回应的数据
	UINT32 SizeOf_CR_Buffer = (Length_UnSubscribeCmd/4+1)*4;
	CHAR *CR_Buffer = (CHAR *)malloc(SizeOf_CR_Buffer);
	if (!CR_Buffer)
	{
		printf("WiPN_UnSubscribeByServer - Malloc UnSubscribeCmd Buf failed!!\n");
		free((void *)pSubscribeServerDID);
		return WiPN_ERROR_MallocFailed;
	}   
	
	// 存放: 1.Cmd: 加密后的指令(Cmd加密后长度会翻倍)
	//		2.Response: 未解密的服务器回应的数据(RecvFrom 所接收到的数据)
	unsigned short SizeOfBuffer = SizeOf_CR_Buffer*2;
	char *Buffer = (char *)malloc(SizeOfBuffer);
	if (!Buffer)
	{
		printf("WiPN_UnSubscribeByServer - Malloc Buffer failed!!\n");
		free((void *)pSubscribeServerDID);
		free((void *)CR_Buffer);
		return WiPN_ERROR_MallocFailed;
	}
	
    int ret = 0;
    int SubscribeServerHandle = -1;
    short retryCount_RecvFrom = 0;
	srand((UINT32)time(NULL));
	unsigned short PrintfFlag = 0;
	
    short index = 0;
    unsigned short j = abs(rand() % NumberOfSubscribeServer);
    while (1)
    {
        if (0 > SubscribeServerHandle)
        {
            for (i = 0; i < NumberOfSubscribeServer; i++)
            {
                j = (j + 1) % NumberOfSubscribeServer;
                index = SizeOfDID * j;
                
                // 计算 UTCT 时间
                UINT32 UTCT_UnSubscribe = time(NULL) - g_Time_ServerRet + strtol(pUTCTString, NULL, 16);
                
                // 格式化 UnSubscribe 指令到 CR_Buffer
                memset(CR_Buffer, 0, SizeOf_CR_Buffer);
                SNPRINTF(CR_Buffer, SizeOf_CR_Buffer, "%sUTCT=0x%X&ACT=UnSubscribe&", pSubCmd, (UINT32)UTCT_UnSubscribe);
				if (0 == PrintfFlag)
				{
					PrintfFlag = 1;
					printf("UnSubscribeCmd= %s\nUnSubscribeCmdSize= %u byte (Not Encrypted Size)\n\n", CR_Buffer, (UINT32)strlen(CR_Buffer));
				}
				
                // 加密 UnSubscribe 指令到 Buffer           
                if (0 > iPN_StringEnc(STRING_ENC_DEC_KEY, CR_Buffer, Buffer, SizeOfBuffer))
                {
                    printf("WiPN_UnSubscribeByServer - iPN_StringEnc: UnSubscribe Cmd Enc failed!\n");
                    free((void *)pSubscribeServerDID);
                    free((void *)CR_Buffer);
                    free((void *)Buffer);
                    return WiPN_ERROR_iPNStringEncFailed;
                }
				
				UINT16 SizeOfUnSubscribeCmd = strlen(Buffer);
                printf("send cmd to SubscribeServer, SubscribeServerDID[%d]= %s. SizeOfUnSubscribeCmd= %u byte (Encrypted Size). sending...\n", j, &pSubscribeServerDID[index], SizeOfUnSubscribeCmd);
                          
				// 发送加密之后的订阅指令到 SubscribeServer
				ret = NDT_PPCS_SendToByServer(&pSubscribeServerDID[index], Buffer, SizeOfUnSubscribeCmd, g_SendToMode, pServerString, pAES128Key);
                
				if (0 > ret)
                {
                    printf("send cmd to SubscribeServer failed! ret= %d [%s]\n", ret, getErrorCodeInfo(ret));
                    printf("retry to send ...\n\n");
                    continue;
                }
                else
                {
					SubscribeServerHandle = ret;
                    printf("send cmd to SubscribeServer success! \n");
                    break;
                }
            }
            if (0 > SubscribeServerHandle)
            {
                printf("WiPN_UnSubscribeByServer - Get SubscribeServerHandle failed! SubscribeServerDID[%d]= %s. ret= %d [%s]\n", j, &pSubscribeServerDID[index], ret, getErrorCodeInfo(ret));
                break;
            }
        }
        else
        {
            printf("Waiting for SubscribeServer response, please wait ...\n");        
            memset(Buffer, 0, SizeOfBuffer);
			UINT16 SizeToRead = SizeOfBuffer;
			
            ret = NDT_PPCS_RecvFrom(SubscribeServerHandle, Buffer, &SizeToRead, 3000);
            
            // 记录 recvFrom 返回的当前时间
            time_t Time_ServerRet = time(NULL);         
			struct tm *ptm = localtime((const time_t *)&Time_ServerRet);
            if (0 > ret)
            {
				printf("WiPN_UnSubscribeByServer - NDT_PPCS_RecvFrom: SubscribeServerDID[%d]= %s. ret= %d. [%s]\n", j, &pSubscribeServerDID[index], ret, getErrorCodeInfo(ret));             
				if (ptm)
				{
					printf("LocalTime: %d-%02d-%02d %02d:%02d:%02d\n", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
				}   
               
                // -26 -27 -3
                if ((NDT_ERROR_NoAckFromPushServer == ret 
					|| NDT_ERROR_NoAckFromDevice == ret 
					|| NDT_ERROR_TimeOut == ret) 
					&& 3 > retryCount_RecvFrom)
                {
                    retryCount_RecvFrom++;
                    continue;
                }
                else if (NDT_ERROR_RemoteHandleClosed == ret) // -36
				{
					printf("WiPN_UnSubscribeByServer - SubscribeServer already call CloseHandle(), SubscribeServerDID[%d]= %s.\n", j, &pSubscribeServerDID[index]);
				}  
                break;
            }
            else
            {
                // 解密接收到的数据, 输出到 CR_Buffer
                if (0 > iPN_StringDnc(STRING_ENC_DEC_KEY, Buffer, CR_Buffer, SizeOf_CR_Buffer))
                {
                    printf("WiPN_UnSubscribeByServer - NDT_PPCS_RecvFrom - iPN_StringDnc:  RecvFrom Data Dec failed! SubscribeServerDID[%d]= %s.\n", j, &pSubscribeServerDID[index]);
                    ret = WiPN_ERROR_iPNStringDncFailed;
                    break ;
                }            
				printf("\nFrom SubscribeServer: \n");
				printf("SubscribeServerDID[%d]: %s\n", j, &pSubscribeServerDID[index]);
				printf("SubscribeServerHandle= %d\n", SubscribeServerHandle);
				//printf("RecvFromData: %s\nSizeOfRecvFromData: %u byte\n", Buffer, (UINT32)strlen(Buffer));
				printf("Data: %s\nSizeOfData: %u byte\n", CR_Buffer, (UINT32)strlen(CR_Buffer));            
                if (ptm)
				{
					printf("LocalTime: %d-%02d-%02d %02d:%02d:%02d\n", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
				}  

                if (0 > GetStringItem(CR_Buffer, "RET", '&', pRETString, SizeOfRETString))
                {
                    printf("WiPN_UnSubscribeByServer - Get RETString failed!\n");
                    ret = WiPN_ERROR_GetRETStringItemFailed;
                    break;
                }
                if (0 > GetStringItem(CR_Buffer, "UTCT", '&', pUTCTString, SizeOfUTCTString))
                {
                    printf("WiPN_UnSubscribeByServer - Get UTCTString failed!\n");
                    ret =  WiPN_ERROR_GetUTCTStringItemFailed;
                    break;
                }
                else // g_Time_ServerRet 必须要与 UTCTString 时间同步更新
				{
					g_Time_ServerRet = Time_ServerRet;
				}           
                break;
            } // ret > 0
        } // Handle > 0
    } // while (1)
    
    if (0 <= SubscribeServerHandle)
	{
		NDT_PPCS_CloseHandle(SubscribeServerHandle);
		printf("WiPN_UnSubscribeByServer - NDT_PPCS_CloseHandle(%d)\n\n", SubscribeServerHandle);
	} 
	if (pSubscribeServerDID)
		free((void *)pSubscribeServerDID);
	if (CR_Buffer)
		free((void *)CR_Buffer);
	if (Buffer)
		free((void *)Buffer);

	
    return ret;
} // WiPN_UnSubscribe
#endif // #if SUBSCRIBE_FLAG

#if POST_FLAG
//// WiPN_Post 推送
INT32 WiPN_Post(const CHAR *pPostServerString, 
				const CHAR *pCmd, 
				CHAR *pRETString, 
				UINT32 SizeOfRETString, 
				CHAR *pUTCTString, 
				UINT32 SizeOfUTCTString)
{
    if (!pPostServerString || !pCmd || !pRETString || !pUTCTString)
	{
		return WiPN_ERROR_InvalidParameter;
	}
	if (   0 == strlen(pPostServerString) 
		|| 0 == strlen(pCmd)
		|| 0 == strlen(pUTCTString) 
		|| 0 == SizeOfRETString 
		|| 0 == SizeOfUTCTString)
	{
		return WiPN_ERROR_InvalidParameter;
	}
		
	//// -------------------------- 获取 PostServer DID -------------------------	
    // 获取 PostServerString 中 PostServer 的个数
    unsigned short NumberOfPostServer = 0;
	unsigned short SizeOfDID = 0;
    if (0 > pharse_number(pPostServerString, &NumberOfPostServer, &SizeOfDID) || 0 == NumberOfPostServer)
	{
		return WiPN_ERROR_GetNumberFromPostServerStringFailed;
	}
	SizeOfDID = (SizeOfDID/4+1)*4; // DID 之间保持足够间隔
	
	// 根据 PostServer DID 的个数分配内存空间
    CHAR *pPostServerDID = (CHAR *)malloc(SizeOfDID*NumberOfPostServer);
    if (!pPostServerDID)
	{
		printf("WiPN_Post - Malloc PostServerDID Buf failed!!\n");
		return WiPN_ERROR_MallocFailed;
	}   
    memset(pPostServerDID, '\0', SizeOfDID*NumberOfPostServer);
    
    // 获取 PostServerString 中的 PostServer DID, 并保存
    const CHAR *pDID = NULL;
    int i = 0;
    for ( ; i < NumberOfPostServer; i++)
    {
        pDID = pharse_DID(pPostServerString, i);
        if (!pDID)
		{
			free((void *)pPostServerDID);
			return WiPN_ERROR_GetDIDFromPostServerStringFailed;
		}   
        memcpy(&pPostServerDID[SizeOfDID*i], pDID, strlen(pDID));    
        //printf("PostServerDID[%d]=%s\n", i, &pPostServerDID[SizeOfDID*i]);
		free((void*)pDID);
		pDID = NULL;
    }
	//// -----------------------------------------------------------------------
	
	// 计算 PostCmd 所需空间大小
	UINT32 Length_PostCmd = strlen("UTCT=0x&")+strlen(pUTCTString)+strlen(pCmd);
	
	// NDT 一次最大只能发送1280字节数据，根据加密算法，加密之前的数据不能超过630字节
	int MaxSizeOfCmd = 630;
	if (MaxSizeOfCmd < Length_PostCmd) 
	{
		printf("WiPN_Post - Length Of PostCmd is Exceed %d bytes!!\n", MaxSizeOfCmd);
		free((void *)pPostServerDID);
		return WiPN_ERROR_ExceedMaxSize;
	}
	
	// 存放: 1.Cmd:加密前的指令
	//		2.Response: 解密后的服务器回应的数据
	unsigned short SizeOf_CR_Buffer = (Length_PostCmd/4+1)*4;
	char *CR_Buffer = (char *)malloc(SizeOf_CR_Buffer);
	if (!CR_Buffer)
	{
		printf("WiPN_Post - Malloc PostCmd Buf failed!!\n");
		free((void *)pPostServerDID);
		return WiPN_ERROR_MallocFailed;
	}   
	
	// 存放: 1.Cmd: 加密后的指令(Cmd加密后长度会翻倍)
	//		2.Response: 未解密的服务器回应的数据(RecvFrom 所接收到的数据)
	unsigned short SizeOfBuffer = SizeOf_CR_Buffer*2;
	char *Buffer = (char *)malloc(SizeOfBuffer);
	if (!Buffer)
	{
		printf("WiPN_Post - Malloc Buffer failed!!\n");
		free((void *)pPostServerDID);
		free((void *)CR_Buffer);
		return WiPN_ERROR_MallocFailed;
	}  
	
	int ret = 0;
    int PostServerHandle = -1;
    short retryCount_RecvFrom = 0;
	srand((UINT32)time(NULL));
	unsigned short PrintfFlag = 0;
	
	short index = 0;
	unsigned short j = abs(rand() % NumberOfPostServer);
    while (1)
    {
        if (0 > PostServerHandle)
        {
            for (i = 0; i < NumberOfPostServer; i++)
            {             
				j = (j + 1) % NumberOfPostServer;
				index = SizeOfDID * j;
				
				// 计算 UTCT 时间
				UINT32 UTCT_Post = time(NULL) - g_Time_ServerRet + strtol(pUTCTString, NULL, 16);
				
				// 格式化 PostCmd, 输出到 CR_Buffer
				memset(CR_Buffer, '\0', SizeOf_CR_Buffer);
				SNPRINTF(CR_Buffer, SizeOf_CR_Buffer, "UTCT=0x%X&%s", (UINT32)UTCT_Post, pCmd);
				if (0 == PrintfFlag)
				{
					PrintfFlag = 1;
					//printf("PostCmd= %s\nPostCmdSize= %u byte (Not Encrypted Size)\n\n", CR_Buffer, (UINT32)strlen(CR_Buffer));
				}
					
				// 加密 PostCmd, 输出到 Buffer 
				if (0 > iPN_StringEnc(STRING_ENC_DEC_KEY, CR_Buffer, Buffer, SizeOfBuffer))
				{
					//printf("WiPN_Post - iPN_StringEnc failed!\n");
					free((void *)pPostServerDID);
					free((void *)CR_Buffer);
					free((void *)Buffer);
					return WiPN_ERROR_iPNStringEncFailed;
				}
				
				UINT32 SizeOfPostCmd = strlen(Buffer);
				//printf("send cmd to PostServer, PostServerDID[%d]= %s. SizeOfPostCmd= %d byte (Encrypted Size). sending...\n", j, &pPostServerDID[index], SizeOfPostCmd);
				
				// 发送加密之后的推送指令到 PostServer
				ret = NDT_PPCS_SendTo(&pPostServerDID[index], Buffer, SizeOfPostCmd, g_SendToMode);
                
				if (0 > ret)
                {
                    printf("send cmd to PostServer failed!! ret= %d [%s]\n", ret, getErrorCodeInfo(ret));
                    printf("retry to send ...\n\n");
					continue;
                }
                else
                {
					PostServerHandle = ret;
                    printf("send cmd to PostServer success!! \n");
                    break;
                }
            }
            if (0 > PostServerHandle)
            {
                printf("WiPN_Post - Get PostServerHandle failed! PostServerDID[%d]= %s. ret= %d [%s]\n", j, &pPostServerDID[index], ret, getErrorCodeInfo(ret));
                break;
            }
        }
        else
        {
            printf("Waiting for PostServer response, please wait ...\n");      
			UINT16 SizeToRead = SizeOfBuffer;
			memset(Buffer, 0, SizeOfBuffer); 
			
			ret = NDT_PPCS_RecvFrom(PostServerHandle, Buffer, &SizeToRead, 3000);
			
			// 记录 recvFrom 返回的当前时间
			time_t Time_ServerRet = time(NULL); 
			struct tm *ptm = localtime((const time_t *)&Time_ServerRet);
            if (0 > ret)
            {
				//printf("WiPN_Post - NDT_PPCS_RecvFrom: PostServerDID[%d]= %s. ret= %d. [%s]\n", j, &pPostServerDID[index], ret, getErrorCodeInfo(ret));
                if (ptm)
				{
					//printf("LocalTime: %d-%02d-%02d %02d:%02d:%02d\n", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
				}  
        				             				
                // -26 -27 -3
                if ((NDT_ERROR_NoAckFromPushServer == ret || NDT_ERROR_NoAckFromDevice == ret || NDT_ERROR_TimeOut == ret) && 3 > retryCount_RecvFrom)
                {
                    retryCount_RecvFrom++;
                    continue;
                }
                else if (NDT_ERROR_RemoteHandleClosed == ret) // -36
				{
					//printf("WiPN_Post - PostServer already call CloseHandle(), PostServerDID[%d]= %s.\n", j, &pPostServerDID[index]);
				}  
                break;
            }
            else
            {
				// 解密接收到的数据                         
                if (0 > iPN_StringDnc(STRING_ENC_DEC_KEY, Buffer, CR_Buffer, SizeOf_CR_Buffer))
                {
                   // printf("WiPN_Post - NDT_PPCS_RecvFrom - iPN_StringDnc: RecvFrom Data Dec failed! PostServerDID[%d]: %s\n", j, &pPostServerDID[index]);
                    ret = WiPN_ERROR_iPNStringDncFailed;
                    break ;
                } 
				//printf("\nFrom PostServer: \n");
				//printf("PostServerDID[%d]: %s\n", j, &pPostServerDID[index]);
				//printf("PostServerHandle= %d\n", PostServerHandle);
				//printf("RecvFromData: %s\nSizeOfRecvFromData: %u byte\n", Buffer, (UINT32)strlen(Buffer));
				//printf("Data: %s\nSizeOfData: %u byte\n", CR_Buffer, (UINT32)strlen(CR_Buffer));
				if (ptm)
				{
					//printf("LocalTime: %d-%02d-%02d %02d:%02d:%02d\n", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
				}				
 			
				// 获取 RET 返回信息
                if (0 > GetStringItem(CR_Buffer, "RET", '&', pRETString, SizeOfRETString))
                {
                    //printf("WiPN_Post - Get RETString failed!\n");
                    ret = WiPN_ERROR_GetRETStringItemFailed;
                    break;
                }
				// 获取 UTCT 
                if (0 > GetStringItem(CR_Buffer, "UTCT", '&', pUTCTString, SizeOfUTCTString))
                {
                    //printf("WiPN_Post - Get UTCTString failed!\n");
                    ret =  WiPN_ERROR_GetUTCTStringItemFailed;
                    break;
                }
				else // g_Time_ServerRet 必须要与 UTCTString 时间同步更新!!!
				{
					g_Time_ServerRet = Time_ServerRet;
				}
                break;
            } // ret > 0
        } // Handle > 0
    } // while (1)
    if (0 <= PostServerHandle)
	{
		NDT_PPCS_CloseHandle(PostServerHandle);
		//printf("WiPN_Post - NDT_PPCS_CloseHandle(%d)\n\n", PostServerHandle);
	}
	if (pPostServerDID)
		free((void *)pPostServerDID);
	if (CR_Buffer)
		free((void *)CR_Buffer);
    if (Buffer)
		free((void *)Buffer);	
	
    return ret;
}

//// WiPN_PostByServer 推送
INT32 WiPN_PostByServer(const CHAR *pServerString,
						const CHAR *pAES128Key,
						const CHAR *pPostServerString, 
						const CHAR *pCmd, 
						CHAR *pRETString, 
						UINT32 SizeOfRETString, 
						CHAR *pUTCTString, 
						UINT32 SizeOfUTCTString)
{
    if (   !pServerString 
		|| !pAES128Key 
		|| !pPostServerString 
		|| !pCmd 
		|| !pRETString 
		|| !pUTCTString)
	{
		return WiPN_ERROR_InvalidParameter;
	}
	if (   0 == strlen(pServerString)
		|| 0 == strlen(pAES128Key)
		|| 0 == strlen(pPostServerString) 
		|| 0 == strlen(pCmd)
		|| 0 == strlen(pUTCTString) 
		|| 0 == SizeOfRETString 
		|| 0 == SizeOfUTCTString)
	{
		return WiPN_ERROR_InvalidParameter;
	}
		
	//// -------------------------- 获取 PostServer DID -------------------------	
    // 获取 PostServerString 中 PostServer 的个数
    unsigned short NumberOfPostServer = 0;
	unsigned short SizeOfDID = 0;
    if (0 > pharse_number(pPostServerString, &NumberOfPostServer, &SizeOfDID) || 0 == NumberOfPostServer)
	{
		return WiPN_ERROR_GetNumberFromPostServerStringFailed;
	}
	SizeOfDID = (SizeOfDID/4+1)*4; // DID 之间保持足够间隔
	
	// 根据 PostServer DID 的个数分配内存空间
    CHAR *pPostServerDID = (CHAR *)malloc(SizeOfDID*NumberOfPostServer);
    if (!pPostServerDID)
	{
		printf("WiPN_PostByServer - Malloc PostServerDID Buf failed!!\n");
		return WiPN_ERROR_MallocFailed;
	}   
    memset(pPostServerDID, '\0', SizeOfDID*NumberOfPostServer);
    
    // 获取 PostServerString 中的 PostServer DID, 并保存
    const CHAR *pDID = NULL;
    int i = 0;
    for ( ; i < NumberOfPostServer; i++)
    {
        pDID = pharse_DID(pPostServerString, i);
        if (!pDID)
		{
			free((void *)pPostServerDID);
			return WiPN_ERROR_GetDIDFromPostServerStringFailed;
		}   
        memcpy(&pPostServerDID[SizeOfDID*i], pDID, strlen(pDID));    
        //printf("PostServerDID[%d]=%s\n", i, &pPostServerDID[SizeOfDID*i]);
		free((void*)pDID);
		pDID = NULL;
    }
	//// -----------------------------------------------------------------------

	// 计算 PostCmd 所需空间大小
	UINT32 Length_PostCmd = strlen("UTCT=0x&")
							+strlen(pUTCTString)
							+strlen(pCmd);
	
	// NDT 一次最大只能发送1280字节数据，根据加密算法，加密之前的数据不能超过630字节
	int MaxSizeOfCmd = 630;
	if (MaxSizeOfCmd < Length_PostCmd) 
	{
		printf("WiPN_PostByServer - Length Of PostCmd is Exceed %d bytes!!\n", MaxSizeOfCmd);
		free((void *)pPostServerDID);
		return WiPN_ERROR_ExceedMaxSize;
	}
	
	// 存放: 1.Cmd:加密前的指令
	//		2.Response: 解密后的服务器回应的数据
	UINT32 SizeOf_CR_Buffer = (Length_PostCmd/4+1)*4;
	CHAR *CR_Buffer = (CHAR *)malloc(SizeOf_CR_Buffer);
	if (!CR_Buffer)
	{
		printf("WiPN_PostByServer - Malloc PostCmd Buf failed!!\n");
		free((void *)pPostServerDID);
		return WiPN_ERROR_MallocFailed;
	}   
		
	
	// 存放: 1.Cmd: 加密后的指令(Cmd加密后长度会翻倍)
	//		2.Response: 未解密的服务器回应的数据(RecvFrom 所接收到的数据)
	unsigned short SizeOfBuffer = SizeOf_CR_Buffer*2;
	char *Buffer = (char *)malloc(SizeOfBuffer);
	if (!Buffer)
	{
		printf("WiPN_WebAPI_Post - Malloc Buffer failed!!\n");
		free((void *)pPostServerDID);
		free((void *)CR_Buffer);
		return WiPN_ERROR_MallocFailed;
	}
	
	int ret = 0;
    int PostServerHandle = -1;
    short retryCount_RecvFrom = 0;
	srand((UINT32)time(NULL));
	unsigned short PrintfFlag = 0;
	
	short index = 0;
	unsigned short j = abs(rand() % NumberOfPostServer);
    while (1)
    {
        if (0 > PostServerHandle)
        {
            for (i = 0; i < NumberOfPostServer; i++)
            {             
				j = (j + 1) % NumberOfPostServer;
				index = SizeOfDID * j;
				// 计算 UTCT 时间
				UINT32 UTCT_Post = time(NULL) - g_Time_ServerRet + strtol(pUTCTString, NULL, 16);
				
				// 格式化 PostCmd, 输出到 CR_Buffer
				memset(CR_Buffer, '\0', SizeOf_CR_Buffer);
				SNPRINTF(CR_Buffer, SizeOf_CR_Buffer, "UTCT=0x%X&%s", (UINT32)UTCT_Post, pCmd);
				
				if (0 == PrintfFlag)
				{
					PrintfFlag = 1;
					printf("PostCmd= %s\nPostCmdSize= %u byte (Not Encrypted Size)\n\n", CR_Buffer, (UINT32)strlen(CR_Buffer));
				}
					
				// 加密 PostCmd, 输出到 Buffer				
				if (0 > iPN_StringEnc(STRING_ENC_DEC_KEY, CR_Buffer, Buffer, SizeOfBuffer))
				{
					printf("WiPN_PostByServer - iPN_StringEnc: PostCmd Enc failed!\n");
					free((void *)pPostServerDID);
					free((void *)CR_Buffer);
					free((void *)Buffer);
					return WiPN_ERROR_iPNStringEncFailed;
				}
				
				UINT32 SizeOfPostCmd = strlen(Buffer);
				printf("send cmd to PostServer, PostServerDID[%d]= %s. SizeOfPostCmd= %d byte (Encrypted Size). sending...\n", j, &pPostServerDID[index], SizeOfPostCmd);
				
				// 发送加密之后的推送指令到 PostServer
				ret = NDT_PPCS_SendToByServer(&pPostServerDID[index], Buffer, SizeOfPostCmd, g_SendToMode, pServerString, pAES128Key);
                
				if (0 > ret)
                {
                    printf("send cmd to PostServer failed!! ret= %d [%s]\n", ret, getErrorCodeInfo(ret));
                    printf("retry to send ...\n\n");
					continue;
                }
                else
                {
					PostServerHandle = ret;
                    printf("send cmd to PostServer success!! \n");
                    break;
                }
            } // for
            if (0 > PostServerHandle)
            {
                printf("WiPN_PostByServer - Get PostServerHandle failed! PostServerDID[%d]= %s. ret= %d [%s]\n", j, &pPostServerDID[index], ret, getErrorCodeInfo(ret));
                break;
            }
        }
        else
        {
            printf("Waiting for PostServer response, please wait ...\n");      
			memset(Buffer, 0, SizeOfBuffer); 
			UINT16 SizeToRead = SizeOfBuffer;
			
			ret = NDT_PPCS_RecvFrom(PostServerHandle, Buffer, &SizeToRead, 3000);
			
			// 记录 recvFrom 返回的当前时间
			time_t Time_ServerRet = time(NULL); 
			struct tm *ptm = localtime((const time_t *)&Time_ServerRet);			
            if (0 > ret)
            {
				printf("WiPN_PostByServer - NDT_PPCS_RecvFrom: PostServerDID[%d]= %s. ret= %d. [%s]\n", j, &pPostServerDID[index], ret, getErrorCodeInfo(ret));
				if (ptm)
				{
					printf("LocalTime: %d-%d-%d %02d:%02d:%02d\n", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
				}	
				
                // -26 -27 -3
                if ((NDT_ERROR_NoAckFromPushServer == ret || NDT_ERROR_NoAckFromDevice == ret || NDT_ERROR_TimeOut == ret) && 3 > retryCount_RecvFrom)
                {
                    retryCount_RecvFrom++;
                    continue;
                }
                else if (NDT_ERROR_RemoteHandleClosed == ret) // -36
				{
					printf("WiPN_PostByServer - PostServer already call CloseHandle().\n");
				}  
                break;
            }
            else
            {
				// 解密接收到的数据, 解密到 CR_Buffer                         
                if (0 > iPN_StringDnc(STRING_ENC_DEC_KEY, Buffer, CR_Buffer, SizeOf_CR_Buffer))
                {
                    printf("WiPN_PostByServer - NDT_PPCS_RecvFrom - iPN_StringDnc:  RecvFromData Dec failed! PostServerDID[%d]: %s\n", j, &pPostServerDID[index]);
                    ret = WiPN_ERROR_iPNStringDncFailed;
                    break ;
                }
				
				printf("\nFrom PostServer: \n");
				printf("PostServerDID[%d]: %s\n", j, &pPostServerDID[index]);
				printf("PostServerHandle= %d\n", PostServerHandle);
				//printf("RecvFromData: %s\nSizeOfRecvFromData: %u byte\n", Buffer, (UINT32)strlen(Buffer));
				printf("Data: %s\nSizeOfData: %u byte\n", CR_Buffer, (UINT32)strlen(CR_Buffer));
				if (ptm)
				{
					printf("LocalTime: %d-%02d-%02d %02d:%02d:%02d\n", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
				}
				
				// 获取 RET 返回信息
                if (0 > GetStringItem(CR_Buffer, "RET", '&', pRETString, SizeOfRETString))
                {
                    printf("WiPN_PostByServer - Get RETString failed!\n");
                    ret = WiPN_ERROR_GetRETStringItemFailed;
                    break;
                }
				// 获取 UTCT 
                if (0 > GetStringItem(CR_Buffer, "UTCT", '&', pUTCTString, SizeOfUTCTString))
                {
                    printf("WiPN_PostByServer - Get UTCTString failed!\n");
                    ret =  WiPN_ERROR_GetUTCTStringItemFailed;
                    break;
                }
				else // g_Time_ServerRet 必须要与 UTCTString 时间同步更新!!!
				{
					g_Time_ServerRet = Time_ServerRet;
				}
                break;
            } // ret > 0
        } // Handle > 0
    } // while (1)
    if (0 <= PostServerHandle)
	{
		NDT_PPCS_CloseHandle(PostServerHandle);
		printf("WiPN_PostByServer - NDT_PPCS_CloseHandle(%d)\n\n", PostServerHandle);
	}
	if (pPostServerDID)
		free((void *)pPostServerDID);
    if (CR_Buffer)
		free((void *)CR_Buffer);
	if (Buffer)
		free((void *)Buffer);
	
    return ret;
}
#endif // #if POST_FLAG


