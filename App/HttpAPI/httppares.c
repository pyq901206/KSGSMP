#include "common.h"
#include "httppares.h"
#include "userdefine.h"
#include "timemanage.h"
#include "jsonswitch.h"
 
static char* memstr(char* full_data, int full_data_len, char* substr)
{
    if (full_data == NULL || full_data_len <= 0 || substr == NULL) {
        return NULL;
    }
    if (*substr == '\0') {
        return NULL;
    }
    int sublen = strlen(substr);
 
    int i;
    char* cur = full_data;
    int last_possible = full_data_len - sublen + 1;
    for (i = 0; i < last_possible; i++) {
        if (*cur == *substr) {
            //assert(full_data_len - i >= sublen);
            if (memcmp(cur, substr, sublen) == 0) {
                //found
                return cur;
            }
        }
        cur++;
    }
    return NULL;
}

static HTTP_METHOD_E HttpMethodStrToInt(char *str)
{
	if( 0 == STRCASECMP(str,"POST")){
		return  HTTP_POST;
	}else if(0 == STRCASECMP(str,"GET")){
		return HTTP_GET;
	}
	return  HTTP_NULL;
}


static int GetHttpUrl(char *str,char *url)
{
	if(NULL == str || NULL == url) return KEY_FALSE;
	char *pHead = NULL;
	pHead = strstr(str," ");
	if(NULL == pHead) return KEY_FALSE;
	STR_GETVALUE_BTPOINT(pHead+1,str+strlen(str),url);
	return KEY_TRUE;
}


static int GetHttpHost(char *str,char *host)
{
	if(NULL == str || NULL == host) return KEY_FALSE;
	char *pHead = strstr(str, "Host:");
	if(NULL == pHead) return KEY_FALSE;
	STR_GETVALUE_BTPOINT(pHead+strlen("Host:")+1,str+strlen(str),host);
	return KEY_TRUE;
}

static int GetHttpBoundary(char *str,char *boundary)
{
	if(NULL == str || NULL == boundary) return KEY_FALSE;
	char *pHead = strstr(str, "boundary=");
	if(NULL == pHead) return KEY_FALSE;
	STR_GETVALUE_BTPOINT(pHead+strlen("boundary=")+1,str+strlen(str),boundary);
	return KEY_TRUE;
}

static int GetHttpContentLenth(char *str)
{
	if(NULL == str) return KEY_FALSE;
	char *ps =  strstr(str, "Content-Length:");
	char tmp[16] = {0};
	if(NULL == ps){
		return KEY_FALSE;
	}
	STR_GETVALUE_BTPOINT(ps+strlen("Content-Length:")+1,str+strlen(str),tmp);
	//DF_DEBUG("%s",tmp);
	return  atoi(tmp);	
}

static int GetHttpFormDataName(char *str,char *name)
{
    char temp[1024] = {0};
	if(NULL == str || NULL == name) return KEY_FALSE;
	char *pHead = strstr(str, "name=");
	if(NULL == pHead) {
		return KEY_FALSE;}
	STR_GETVALUE_BTPOINT(pHead+strlen("name=")+1,str+strlen(str),temp);
	sscanf(temp, "%[^\"]", name);
	return KEY_TRUE;
}

static int GetHttpRequstHead(HttpReqHead_T *head,char *msg,int len)
{	
	char method[8] = {0};
	STR_GETVALUE_BTPOINT(msg,msg+5,method);
	head->method = HttpMethodStrToInt(method);
	GetHttpUrl(msg,head->url);
	GetHttpHost(msg,head->host);
	head->contentLength = GetHttpContentLenth(msg);
	GetHttpBoundary(msg,head->boundary);
	return 0;
}

typedef struct _HttpCode_T{
	char *str;
	int  code;
}HttpCode_T;

static int GroupRespHttpMsg(int code,char *buff,int buffLen,char *out)
{
	static HttpCode_T name[] = {
		{"OK",200},
		{"Bad Request",400},
		{"Unauthorized",401},
		{"Forbidden",403},
		{"Not Found",404},
		{"Method Not Allowed",405},
		{NULL,-1}
	};

	if( NULL == buff ||
		0 > buffLen  ||
		NULL == out){
		return KEY_FALSE;
	}
	char date[64] = {0};	
	GetUTCTime(date);
	
	int i = 0;
	for(i = 0; NULL != name[i].str; i++){
		if(code == name[i].code)	break;
	}
	if(NULL == name[i].str) return KEY_FALSE;
	
	sprintf(out,"HTTP/1.1 %d %s\r\n"\
	"Server: %s\r\n"\
	"Date: %s\r\n"\
	"Content-Type: application/json; charset=utf-8;\r\n"\
	"Content-Length: %d\r\n"\
	"Connection: close\r\n\r\n",name[i].code,name[i].str,HTTPMSG_SERVER,date,buffLen);
	strcat(out,buff);
	return KEY_TRUE;
	
}

int ParesHttpMsg(char *msg,int len,char *out,int *outlen)
{
	int iret = KEY_FALSE;
	char buff[HTTPMSG_MAXLEN] = {0};
	char *bodyMsg = NULL;
	HttpReqHead_T head = {0};
	GetHttpRequstHead(&head,msg,len);
	bodyMsg = strstr(msg,"\r\n\r\n");
	if(bodyMsg != NULL){
		bodyMsg += strlen("\r\n\r\n");
		if(0 != strlen(bodyMsg))
		{
			iret = JsonSwitch(bodyMsg, buff, HTTPMSG_MAXLEN);
			//DF_DEBUG("cd test iret = %d",iret);
		}
	}
	
	if(iret == KEY_TRUE){
		  GroupRespHttpMsg(200,buff,strlen(buff),out);
		  *outlen = strlen(out);
	}
	else if(iret == 401)
	{
		  GroupRespHttpMsg(401,buff,strlen(buff),out);
		  *outlen = strlen(out);	 
	}
	else if(iret == 403)
	{
		  GroupRespHttpMsg(403,buff,strlen(buff),out);
		  *outlen = strlen(out);
	 
	}
	else if(iret == 404)
	{
		  GroupRespHttpMsg(404,buff,strlen(buff),out);
		  *outlen = strlen(out);
	 
	}
	else
	{
		  GroupRespHttpMsg(400,buff,strlen(buff),out);
		  *outlen = strlen(out);
	}
	return iret;
}
