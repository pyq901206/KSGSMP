/***********************************************************************************
*              Copyright 2004 - 2050, Hisilicon Tech. Co., Ltd.
*                           ALL RIGHTS RESERVED
* FileName: smtpc_send.c
* Description: send the E-mail.
*
* History:
* Version   Date              Author                                 DefectNum              Description
* main\1    2006-08-30   Ou Weiquan 60018927          NULL                       Create this file.
***********************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stddef.h>

//for xyssl
#if ((defined(CMP_POLARSSL)) && (CMP_POLARSSL == 1))
#include "polarssl/ssl.h"
#include "polarssl/havege.h"
#include "polarssl/certs.h"
#include "polarssl/x509.h"
#include "polarssl/net.h"
#endif

#include "os_types_interface.h"
#include "smtpc.h"
#include "smtpc_cm.h"
#include "smtpc_code.h"
#include "smtpc_send.h"
#include "smtpc_log.h"

#define RSPECT_CONTINUE	0x1
#define RSPECT_END			0x2

#define SEND_OK 0
#define SEND_FAILED -1

VOS_UINT32 g_u32SMTPC_SENDSocketFD = 0;

#if ((defined(CMP_POLARSSL)) && (CMP_POLARSSL == 1))
static havege_state hs;
static ssl_context ssl;
static ssl_session ssn;
static x509_cert cacert;
static x509_cert clicert;
static rsa_context rsa;
#endif

static int s_isSslConnected = 0;
static unsigned int s_smtpSendRespTimeout = SMTPC_SEND_RESP_TIMEOUT;
static unsigned int s_smtpSendConnTimeout = SMTPC_SEND_CONN_TIMEOUT;

extern char *strcasestr (__const char *__haystack, __const char *__needle);


#define DEBUG_LEVEL 0

void my_debug( void *ctx, int level, const char *str )
{
    if( level < DEBUG_LEVEL )
    {
        fprintf( (FILE *) ctx, "%s", str );
        fflush(  (FILE *) ctx  );
    }
}

/*
 * create the socket file.
 */
VOS_INT32 SMTPC_SENDSocketCreate()
{
  return(g_u32SMTPC_SENDSocketFD = socket(AF_INET, SOCK_STREAM, 0));
}


/*
 * create the tcp connection with socket.
 */
VOS_INT32 SMTPC_SENDSocketConnect(VOS_CHAR *hostName, VOS_UINT16 port)
{
  struct sockaddr_in serverAddr;
  struct hostent *host;
  int ret;
  struct timeval timeo = {0, 0};
  char **pptr;
  char str[32];

  if(g_u32SMTPC_SENDSocketFD == 0)
  {
    HI_ERR_Processor("SMTPC_SENDSocketConnect, g_u32SMTPC_SENDSocketFD == 0");
    return -1;
  }

  if((host = gethostbyname(hostName)) == VOS_NULL_PTR)
  {
    HI_ERR_Processor("SMTPC_SENDSocketConnect, get hostname error");
    return -1;	
  }

  for(pptr = host->h_aliases; *pptr != NULL; pptr++)
    printf(" alias:%s\n",*pptr);
    /* 根据地址类型，将地址打出来 */
    switch(host->h_addrtype)
    {
        case AF_INET:
        case AF_INET6:
        pptr=host->h_addr_list;
        /* 将刚才得到的所有地址都打出来。其中调用了inet_ntop()函数 */
        for(;*pptr!=NULL;pptr++)
        printf(" address:%s\n", inet_ntop(host->h_addrtype, *pptr, str, sizeof(str)));
        break;
        default:
        printf("unknown address type\n");
        break;
    }




  timeo.tv_sec = s_smtpSendConnTimeout;
  ret = setsockopt(g_u32SMTPC_SENDSocketFD, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo));
  if ( ret < 0 )
  {
      HI_ERR_Processor("SMTPC_SENDSocketConnect setsockopt err");      
      return -1;        
  }
  memset(&serverAddr, 0, sizeof(struct sockaddr_in));
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);
  serverAddr.sin_addr = *((struct in_addr *)host->h_addr);
  //serverAddr.sin_addr.s_addr = inet_addr(hostName);
  //printf("host:%s, port:%u, fd:%u\n", hostName, port, g_u32SMTPC_SENDSocketFD);
  ret = connect(g_u32SMTPC_SENDSocketFD, (struct sockaddr *)&serverAddr, (socklen_t)sizeof(struct sockaddr));  
  if ( ret < 0 )
  {
      printf("Connect to smtp server \"%s:%u\" timeout %u seconds!\n", 
        hostName, port, s_smtpSendConnTimeout);
      return ret;
  }
  return 0;
}


/*
 *send the message
 */
VOS_INT32 SMTPC_SENDSocketSend(VOS_CHAR *msg)
{
  if(g_u32SMTPC_SENDSocketFD == 0)
  {
    HI_ERR_Processor("SMTPC_SENDSocketSend, g_u32SMTPC_SENDSocketFD == 0");
    return -1;	
  }
  #if ((defined(CMP_POLARSSL)) && (CMP_POLARSSL == 1))
  if ( s_isSslConnected == 1 )
  {
      //printf("[SSL]:sll write\n");
      return ssl_write(&ssl, (VOS_UINT8 *)msg, strlen(msg));
  }
  #endif
  return send(g_u32SMTPC_SENDSocketFD, msg, strlen(msg), 0);
}

/*
 *receive the message
 */
VOS_INT32 SMTPC_SENDSocketReceive(VOS_CHAR *buf, VOS_INT32 receive_num)
{
  if(g_u32SMTPC_SENDSocketFD == 0)
  {
    HI_ERR_Processor("SMTPC_SENDSocketReceive, g_u32SMTPC_SENDSocketFD == 0");
    return -1;
  }
  #if ((defined(CMP_POLARSSL)) && (CMP_POLARSSL == 1))
  if ( s_isSslConnected == 1 )
  {
      //printf("[SSL]:sll read\n");
      return ssl_read(&ssl, (VOS_UINT8 *)buf, receive_num);
  }
  #endif
  return(recv(g_u32SMTPC_SENDSocketFD, buf, receive_num, 0));
}

/*
 *makesure the socket is useable
 */
VOS_INT32 SMTPC_SENDIsReadable()
{
  fd_set fds;
  struct timeval tv;
  tv.tv_sec = 2;
  tv.tv_usec = 0;
  
  FD_ZERO(&fds);
  FD_SET(g_u32SMTPC_SENDSocketFD, &fds);
  int nStatus = select(g_u32SMTPC_SENDSocketFD+1, &fds, NULL, NULL, &tv);
  //printf("~~~~~~nStatus = %d\n", nStatus);
  #if 0
  if ((nStatus == -1) || (nStatus == 0))
  {
    return -1;
  }
  else
  {
    return 0;
  }
  #endif
  return nStatus;
}

/*
 *close the socket
 */
VOS_VOID SMTPC_SENDSocketClose()
{
    if(g_u32SMTPC_SENDSocketFD == 0)
    {
        HI_ERR_Processor("SMTPC_SENDSocketClose, g_u32SMTPC_SENDSocketFD == 0");
        return;	
    }
    close(g_u32SMTPC_SENDSocketFD);
    g_u32SMTPC_SENDSocketFD = 0;
}

static void SMTPC_SEND_SslClose(void)
{
#if ((defined(CMP_POLARSSL)) && (CMP_POLARSSL == 1))
    x509_free( &clicert );
    x509_free( &cacert );
    rsa_free( &rsa );
    ssl_free( &ssl );
    memset( &ssl, 0, sizeof( ssl ));      
#endif
}

/*****************************************************************************
 Prototype       : SMTPC_SEND_SslClose
 Description     : ...
 Input           : None
 Output          : None
 Return Value    : int
 Global Variable   
    Read Only    : 
    Read & Write : 
  History         
  1.Date         : 2008/2/27
    Author       : qushen
    Modification : Created function

*****************************************************************************/
static void SMTPC_SEND_SslDisConnect( void )
{
    if ( s_isSslConnected == 0 )
    {
        return;
    }
    /*SSL断开连接需要发送notify通知连接断开*/
#if ((defined(CMP_POLARSSL)) && (CMP_POLARSSL == 1))
    ssl_close_notify( &ssl );
    
    SMTPC_SEND_SslClose();
#endif
    
    s_isSslConnected = 0;    
    return;
}

/*****************************************************************************
 Prototype       : SMTPC_SEND_SSL
 Description     : 
 Input           : None
 Output          : None
 Return Value    : 
 Global Variable   
    Read Only    : 
    Read & Write : 
  History         
  1.Date         : 2008/2/27
    Author       : qushen
    Modification : Created function

*****************************************************************************/
int  SMTPC_SEND_SslConnect( char *hostname, SMTP_SSL_TYPE_E ssltype )
{
  #if ((defined(CMP_POLARSSL)) && (CMP_POLARSSL == 0))
      printf("-------no polarssl\n");
	return 0;
  #else
    printf("=======has polarssl\n");
    int ret;
    
    /*support STARTTLS ssl type*/
    if ( ssltype == SSL_TYPE_NOSSL )
    {
        return 0;
    }
    
    if ( s_isSslConnected == 1 )
    {
        return 0;
    }
    /*
     * 0. Initialize the RNG and the session data
     */
    havege_init( &hs );
    memset( &ssn, 0, sizeof( ssl_session ) );

    /*
     * 1.1. Load the trusted CA
     */
    SMTP_INFO( "\n  . Loading the CA root certificate ..." );

    memset( &cacert, 0, sizeof( x509_cert ) );
    /*
     * 2. Start the connection, 连接已经建立,需要使用fd
     */    

    /*
     * 3. Setup stuff
     */
    SMTP_INFO( "  . Setting up the SSL/TLS structure..." );


    if( ( ret = ssl_init( &ssl ) ) != 0 )
    {
        SMTP_ERROR( " failed\n  ! ssl_init returned %d\n\n", ret );
        return -1;
    }

  //  ssl_set_debuglvl( &ssl, 0 );
    ssl_set_dbg( &ssl, my_debug, stdout );
    ssl_set_endpoint( &ssl, SSL_IS_CLIENT );
    ssl_set_authmode( &ssl, SSL_VERIFY_OPTIONAL );

    ssl_set_rng( &ssl, havege_rand, &hs );
    ssl_set_bio( &ssl, net_recv, &g_u32SMTPC_SENDSocketFD,
                       net_send, &g_u32SMTPC_SENDSocketFD );

    ssl_set_ciphersuites( &ssl, ssl_default_ciphersuites);
    ssl_set_session( &ssl, 1, 600, &ssn );


    ssl_set_ca_chain( &ssl, &cacert, NULL,hostname );
    ssl_set_own_cert( &ssl, &clicert, &rsa );

    /*
     * 4. Handshake
     */
    SMTP_INFO( "  . Performing the SSL/TLS handshake..." );

    while( ( ret = ssl_handshake( &ssl ) ) != 0 )
    {
        if( ret != POLARSSL_ERR_NET_WANT_READ && ret != POLARSSL_ERR_NET_WANT_WRITE )
        {
            SMTP_ERROR( " failed\n  ! ssl_handshake returned %d\n\n", ret );            
            return -1;
        }
    }
    s_isSslConnected = 1;
    SMTP_INFO( " ok,    [ Cipher is %s ]\n", ssl_get_cipher( &ssl ) );
    return 0;
#endif
}


/***********************************************************************************
* Function:      SMTPC_SEND_INF_DisConnect
* Description:   disconnect the smtp service
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:         none
* Output:       none
* Return:       none
* Others:       none
***********************************************************************************/
VOS_VOID SMTPC_SEND_INF_DisConnect()
{   
    SMTPC_SENDSocketClose();

    /*disconnect ssl*/
    SMTPC_SEND_SslDisConnect();    
}

/***********************************************************************************
* Function:      SMTPC_SEND_Send
* Description:   send the mail to the smtp server
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:         conf     the configure of SMTPC
*                   Mail     the mail to be send
* Output:       none
* Return:       0:  success
*                  -1: fail
* Others:       none
***********************************************************************************/
VOS_INT32 SMTPC_SEND_Send(PTR_SMTPC_CM_Mail_s Mail, PTR_SMTPC_CONF_Configure_s conf)
{
    SMTP_INFO("conf->Server = %s, conf->Port = %u, conf->RespTimeout = %u, conf->ConnTimeout = %u\n", 
        conf->Server, conf->Port, conf->RespTimeout, conf->ConnTimeout);
	//printf("conf->Server = %s, conf->Port = %u, conf->RespTimeout = %u, conf->ConnTimeout = %u\n", 
     //   conf->Server, conf->Port, conf->RespTimeout, conf->ConnTimeout);
    s_smtpSendRespTimeout = conf->RespTimeout;
    s_smtpSendConnTimeout = conf->ConnTimeout;
    if(SMTPC_SEND_CONN_Connect(conf) == -1) 
    {
      HI_ERR_Processor("SMTPC_SEND_Send, Connection error");
      return -1;
    }
   if(SMTPC_SEND_SM_SendMail (Mail) == -1)
   {
      SMTPC_SEND_INF_DisConnect();
      HI_ERR_Processor("SMTPC_SEND_Send, Send mail error");
      return -1;
   }
   
   return 0;
}

/***********************************************************************************
* Function:       SMTPC_SEND_CONN_Connect
* Description:   connect and login the smtp server
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:         config  the configure of SMTPC
* Output:       none
* Return:       0:   success
*                  -1:  fail
* Others:       none
***********************************************************************************/
VOS_INT32 SMTPC_SEND_CONN_Connect(PTR_SMTPC_CONF_Configure_s config)
{  
    VOS_CHAR *hostname = config->Server;
    VOS_CHAR *user = config->User;
    VOS_CHAR *passwd = config->Passwd;
    SMTPC_CONF_LoginType_E LoginType = config->LoginType;
    struct utsname localhost;
    VOS_INT32 IsConnect = 0;

    if(SMTPC_SENDSocketCreate() == -1)
    {
        HI_ERR_Processor("SMTPC_SEND_CONN_Connect, SMTPC_SENDSocketCreate, socket create error");
        return -1;
    }

    if(SMTPC_SENDSocketConnect(hostname, config->Port) == -1)
    {
        HI_ERR_Processor("SMTPC_SEND_CONN_Connect, SMTPC_SENDSocketConnect, socket connect error");
        SMTPC_SENDSocketClose();
        return -1;
    }

    if(config->SslType == 1)
	{
		/*建立SSL加密连接*/
	    IsConnect = SMTPC_SEND_SslConnect(localhost.nodename, config->SslType);
	    if ( IsConnect == -1 )
	    {
	        printf("[SSL]ssl connect error\n");
	        HI_ERR_Processor("SMTPC_SEND_CONN_Connect, SSL error");
	        /*SSL握手失败，只需关闭socket,并释放ssl资源*/
	        SMTPC_SENDSocketClose();
	        SMTPC_SEND_SslClose();
	        return -1;      
	    }  
	}
	
    /*读取SMTP服务器建立连接后的应答信息，是否必须放在SSL之后读取?*/ 
    if(SMTPC_SEND_RESP_ReadResponse(220, VOS_NULL_PTR, 0) == -1)
    {
        HI_ERR_Processor("SMTPC_SEND_CONN_Connect, read response error");
        SMTPC_SEND_INF_DisConnect();
        return -1;
    }
    /*get the localhost name*/
    if(uname(&localhost) < 0)
    {
        HI_ERR_Processor("SMTPC_SEND_CONN_Connect, get local name error");
        SMTPC_SEND_INF_DisConnect();
        return -1;	
    }
    /*设置登陆方式*/
    IsConnect = SMTPC_SEND_CONNLogin(localhost.nodename, user, passwd, LoginType);
    if(IsConnect == -1)
    {
        HI_ERR_Processor("SMTPC_SEND_CONN_Connect, login error");
        SMTPC_SEND_INF_DisConnect();
        return -1;
    }
    return 0;
}

/***************************************************************************
                Gmail SMTP Information

Loading the CA root certificate ...  . Connecting to tcp/209.85.147.111/465
... ok
  . Setting up the SSL/TLS structure... ok
  . Performing the SSL/TLS handshake... ok
    [ Cipher is SSL_RSA_DES_168_SHA ]
Begin SSL data exchange
S: :220 mx.google.com ESMTP j31sm9633726waf
C: EHLO localhost
S: :250-mx.google.com at your service, [58.61.177.86]
250-SIZE 28311552
250-8BITMIME
250-AUTH LOGIN PLAIN
250 ENHANCEDSTATUSCODES
C: AUTH LOGIN
S: :334 VXNlcm5hbWU6
C: cXVxdXNzQGdtYWlsLmNvbQ==
S: :334 UGFzc3dvcmQ6
C: SGkzNTEwa2Nj
S: :235 2.7.0 Accepted
C: MAIL FROM:<ququss@gmail.com>
S: :250 2.1.0 OK
C: RCPT TO:<ququss@gmail.com>
S: :250 2.1.5 OK
C: DATA
S: :354 Go ahead
C: cmd len 261C: SGFoYSwgVGhpcyBpcyB4eXNzbCAuLi4=
C:
.
S: :250 2.0.0 OK 1194975154 j31sm9633726waf
C: QUIT
S: :221 2.0.0 mx.google.com closing connection j31sm9633726waf
  + Press Enter to exit this program.
  
***********************************************************************************/


/***********************************************************************************
* Function:       SMTPC_SEND_CONNLogin
* Description:   login the smtp server
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:         localhostname     the name of the local computer
*                   username           the username used to login the smtp server
*                   passwd              the password of the user 
* Output:       none
* Return:       0: success
*                 -1: fail
* Others:       none
***********************************************************************************/
VOS_INT32 SMTPC_SEND_CONNLogin(VOS_CHAR *localhostname, VOS_CHAR *username, VOS_CHAR *passwd, SMTPC_CONF_LoginType_E LoginType)
{
  VOS_CHAR *buf = VOS_NULL_PTR;
  VOS_INT32 IsLogin;
  
  buf = (VOS_CHAR *)SMTPC_Malloc(strlen(localhostname) + 8);
  /*modify l59217 check buf whether NULL 2008-06-24*/
  if (buf == VOS_NULL_PTR)
  {
    HI_ERR_Processor("SMTPC_SEND_CONNLogin, malloc buf error");
    return -1;
  }
  strcpy(buf, "EHLO ");
  strcat(buf, localhostname);
  strcat(buf, "\r\n");
  //printf("~~~~~~ login send: %s\n", buf);  
  if(SMTPC_SENDSocketSend(buf) == -1)
  {
    HI_ERR_Processor("SMTPC_SEND_CONNLogin, send EHLO error");
    /*modify l59217 mem leak 2008-06-24*/
    SMTPC_Free(buf);
    return -1;
  }
  SMTPC_Free(buf);
  buf = VOS_NULL_PTR;

  if(SMTPC_SEND_RESP_ReadResponse(250, VOS_NULL_PTR, 0) == -1)
  {
    HI_ERR_Processor("SMTPC_SEND_CONNLogin, Can not receive 250 after sending EHLO");
    return -1;
  }
  IsLogin = -1;
  switch(LoginType)
  {
    case CramLogin:
    {
      IsLogin = SMTPC_SEND_CONNCramLogin(username, passwd);
      break;
    }
    case AuthLogin:
    {
      IsLogin = SMTPC_SEND_CONNAuthLogin(username, passwd);
      break;
    }
    case AuthLoginPlain:
    {
      IsLogin = SMTPC_SEND_CONNAuthLoginPlain(username, passwd);
      break;
    }
    case NoLogin:
    {
      IsLogin = 0;
    }
    // TODO: Auto登陆方式，根据服务器返回login类型自动选择登陆方式
  }
  
  return IsLogin;
}

/***********************************************************************************
* Function:      SMTPC_SEND_CONNCramLogin
* Description:   login the smtp server by using the AUTH CRAM_MD5 method
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:         username          the username used to login the smtp server
*                   passwd              the password of the user 
* Output:       none
* Return:       0: success
*                 -1: fail
* Others:       none
***********************************************************************************/
VOS_INT32 SMTPC_SEND_CONNCramLogin(VOS_CHAR *username, VOS_CHAR *passwd)
{
  VOS_CHAR *buf = "AUTH CRAM-MD5\r\n";
  VOS_UINT32 RespBufSize = SMTPC_SEND_RESP_OutputBufferSize;
  VOS_CHAR RespBuffer[SMTPC_SEND_RESP_OutputBufferSize];
  VOS_INT32 RecvDataNum = 0;
  VOS_UINT8 digest[16];
  VOS_UINT32 NameLen;
  VOS_UINT32 DataSize;
  VOS_CHAR *Data;
  VOS_CHAR *tmpData;
  VOS_CHAR *key;
  VOS_CHAR *EncodedData;
  VOS_INT32 i;

  NameLen = strlen(username);
  DataSize = NameLen + 50;
  
  if(SMTPC_SENDSocketSend(buf) == -1)
  {
    HI_ERR_Processor("SMTPC_SEND_CONNCramLogin, Send AUTH command error");
    return -1;
  }
  
  if((RecvDataNum = SMTPC_SEND_RESP_ReadResponse(334, RespBuffer, RespBufSize)) == -1)
  {
    HI_ERR_Processor("SMTPC_SEND_CONNCramLogin, Can not receive 250 code after sending AUTH CRAM_MD5");
    return -1;
  }
  
  key = (VOS_CHAR *)SMTPC_Malloc(SMTPC_CODE_SetBDecodeOutputSize(RecvDataNum));
  /*modify l59217 check key whether NULL 2008-06-24*/
  if (key == VOS_NULL_PTR)
  {
    HI_ERR_Processor("SMTPC_SEND_CONNCramLogin, malloc key error");
    return -1;
  }
  SMTPC_CODE_Base64Decode(key, RespBuffer, RecvDataNum);
  
  SMTPC_CODE_MD5Digest((VOS_UINT8 *)key, strlen(key), (VOS_UINT8 *)passwd, strlen(passwd), digest);
  SMTPC_Free(key);
  
  Data = (VOS_CHAR *)SMTPC_Malloc(DataSize + 1);
  /*modify l59217 check Data whether NULL 2008-06-24*/
  if (Data == VOS_NULL_PTR)
  {
    HI_ERR_Processor("SMTPC_SEND_CONNCramLogin, malloc Data error");
    return -1;
  }
  tmpData = Data;
  strcpy(tmpData, username);
  strcat(tmpData, " ");
  tmpData += NameLen;
  tmpData ++;
  for(i = 0; i<16; i++)
  {
     sprintf(tmpData, "%02x", digest[i]);
     tmpData +=  2;
  }
  *(tmpData) = '\0';
  
  EncodedData = (VOS_CHAR *)SMTPC_Malloc(SMTPC_CODE_SetBEncodeOutputSize(DataSize) + 3);
  /*modify l59217 check EncodeData NULL 2008-06-24*/
  if (EncodedData == VOS_NULL_PTR)
  {
    HI_ERR_Processor("SMTPC_SEND_CONNCramLogin, malloc EncodedData error");
    SMTPC_Free(Data);
    return -1;
  }
  SMTPC_CODE_Base64Encode(EncodedData, Data, DataSize);
  SMTPC_Free(Data);
  strcat(EncodedData, "\r\n");
  
  if(SMTPC_SENDSocketSend(EncodedData) == -1)
  {
    HI_ERR_Processor("SMTPC_SEND_CONNCramLogin, Send username and password error");
    SMTPC_Free(EncodedData);
    return -1;
  }
  SMTPC_Free(EncodedData);
  
  if(SMTPC_SEND_RESP_ReadResponse(235, VOS_NULL_PTR, 0) == -1) 
  {
    HI_ERR_Processor("SMTPC_SEND_CONNCramLogin, Can not receive 235 code after sending username and password");
    return -1;
  }
  
  return 0;

}

/***********************************************************************************
* Function:      SMTPC_SEND_CONNAuthLogin
* Description:   login the smtp server by using the AUTH LOGIN method
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:          username          the username used to login the smtp server
*                    passwd              the password of the user 
* Output:        none
* Return:        0: success
*                  -1: fail
* Others:       none
***********************************************************************************/
VOS_INT32 SMTPC_SEND_CONNAuthLogin(VOS_CHAR *username, VOS_CHAR *passwd)
{
  VOS_CHAR *buf = "AUTH LOGIN\r\n";
  VOS_INT32 RecvDataNum = 0;
  VOS_CHAR RespBuf[50];
  VOS_CHAR *usernameData;
  VOS_CHAR *passwordData;
  VOS_CHAR *RespForUser;
  VOS_CHAR *RespForPasswd;

  if(SMTPC_SENDSocketSend(buf) == -1)
  {
    HI_ERR_Processor("SMTPC_SEND_CONNAuthLogin, Send AUTH LOGIN command error");
    return -1;
  }
  //printf("~~~~~~buf=%s\n", buf);
  if((RecvDataNum= SMTPC_SEND_RESP_ReadResponse(334, RespBuf, sizeof(RespBuf))) == -1) 
  {
    HI_ERR_Processor("SMTPC_SEND_CONNAuthLogin, Can not receive 334 code after sending AUTH LOGIN command");
    return -1;
  }

  RespForUser =  (VOS_CHAR *)SMTPC_Malloc(SMTPC_CODE_SetBDecodeOutputSize(RecvDataNum));
  /*modify l59217 check RespForUser whether NULL 2008-06-24*/
  if (RespForUser == VOS_NULL_PTR)
  {
    HI_ERR_Processor("SMTPC_SEND_CONNAuthLogin, malloc RespForUser error");
    return -1;
  }
  SMTPC_CODE_Base64Decode(RespForUser, RespBuf, RecvDataNum);

  //printf("-----RespForUser is %s---------------\n", RespForUser);
  /*modify l59217 2008-06-13 by 163 smtp err*/
  /*if(strcmp(RespForUser, "Username:") != 0)*/
  if (strcasecmp(RespForUser, "Username:") != 0)
  {
    HI_ERR_Processor("SMTPC_SEND_CONNAuthLogin, Can not receive 'Username:' String");
    SMTPC_Free(RespForUser);
    return -1;
  }
  SMTPC_Free(RespForUser);
  
  usernameData = (VOS_CHAR *)SMTPC_Malloc(SMTPC_CODE_SetBEncodeOutputSize(strlen(username)) + 3);
  /*modify l59217 check usernameData whether NULL 2008-06-24*/
  if (usernameData == VOS_NULL_PTR)
  {
    HI_ERR_Processor("SMTPC_SEND_CONNAuthLogin, malloc usernameData error");
    return -1;
  }
  SMTPC_CODE_Base64Encode(usernameData, username, strlen(username));
  strcat(usernameData, "\r\n");
  //printf("@@@@@@@@@@@@@ username=%s, passwd=%s\n", username, passwd);
  if(SMTPC_SENDSocketSend(usernameData) == -1)
  {
    HI_ERR_Processor("SMTPC_SEND_CONNAuthLogin, send username error");
    SMTPC_Free(usernameData);
    return -1;
  }
  SMTPC_Free(usernameData);
  
  if((RecvDataNum= SMTPC_SEND_RESP_ReadResponse(334, RespBuf, sizeof(RespBuf))) == -1)
  {
     HI_ERR_Processor("SMTPC_SEND_CONNAuthLogin, Can not receive 334 code after sending username");
     return -1;
  }
  
  RespForPasswd = (VOS_CHAR *)SMTPC_Malloc(SMTPC_CODE_SetBDecodeOutputSize(RecvDataNum));
  /*modify l59217 check RespForPasswd whether NULl 2008-06-24*/
  if (RespForPasswd == VOS_NULL_PTR)
  {
     HI_ERR_Processor("SMTPC_SEND_CONNAuthLogin, malloc RespForPasswd error");
     return -1;
  }
  SMTPC_CODE_Base64Decode(RespForPasswd, RespBuf, RecvDataNum);
  /*modify l59217 2008-06-13 by 163 smtp err*/
  /*if(strcmp(RespForPasswd, "Password:") != 0)*/
  if (strcasecmp(RespForPasswd, "Password:") != 0)
  {
     HI_ERR_Processor("SMTPC_SEND_CONNAuthLogin, Can not receive username String");
     SMTPC_Free(RespForPasswd);
     return -1;
  }
  SMTPC_Free(RespForPasswd);
  
  passwordData =  (VOS_CHAR *)SMTPC_Malloc(SMTPC_CODE_SetBEncodeOutputSize(strlen(passwd)) + 3);
  /*modify l59217 check passwordData whether NULL 2008-06-24*/
  if (passwordData == VOS_NULL_PTR)
  {
     HI_ERR_Processor("SMTPC_SEND_CONNAuthLogin, malloc passwordData error");
     return -1;
  }
  SMTPC_CODE_Base64Encode(passwordData, passwd, strlen(passwd));
  strcat(passwordData, "\r\n");
  if(SMTPC_SENDSocketSend(passwordData) == -1)
  {
     HI_ERR_Processor("SMTPC_SEND_CONNAuthLogin, send password error");
     SMTPC_Free(passwordData);
     return -1;
  }
  SMTPC_Free(passwordData);

  if(SMTPC_SEND_RESP_ReadResponse(235, VOS_NULL_PTR, 0) == -1)
  {
     HI_ERR_Processor("SMTPC_SEND_CONNAuthLogin, Can not receive 235 code after sending password");
     return -1;
  }
  
  return 0;
}

/***********************************************************************************
* Function:      SMTPC_SEND_CONNAuthLoginPlain
* Description:   login the smtp server by using the AUTH LOGIN PLAIN method
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:          username          the username used to login the smtp server
*                    passwd              the password of the user 
* Output:        none
* Return:        0: success
*                  -1: fail
* Others:       none
***********************************************************************************/
VOS_INT32 SMTPC_SEND_CONNAuthLoginPlain(VOS_CHAR *username, VOS_CHAR *passwd)
{
  VOS_CHAR *buf;
  VOS_UINT32 bufSize;
  VOS_CHAR *Data;
  VOS_CHAR *TmpData;
  VOS_UINT32 DataSize;
  VOS_UINT32 EncodedDataSize;
  VOS_CHAR *EncodedData;
  VOS_UINT32 NameLen, PasswdLen;
  
  NameLen = strlen(username);
  PasswdLen = strlen(passwd);
  
  DataSize = NameLen + PasswdLen + 2;
  Data = (VOS_CHAR *)SMTPC_Malloc(DataSize);
  /*modify l59217 check Data whether NULL 2008-06-24*/
  if (Data == VOS_NULL_PTR)
  {
    HI_ERR_Processor("SMTPC_SEND_CONNAuthLoginPlain, malloc Data error");
    return -1;
  }
  
  TmpData = Data;
  strcat(TmpData, "\0");
  TmpData ++;
  strcpy(TmpData, username);
  TmpData += NameLen;
  
  strcat(TmpData, "\0");
  TmpData ++;
  strcpy(TmpData, passwd);
  TmpData += PasswdLen;
  
  EncodedDataSize = SMTPC_CODE_SetBEncodeOutputSize(DataSize);
  EncodedData = (VOS_CHAR *)SMTPC_Malloc(EncodedDataSize);
  /*modify l59217 check EncodedData whether NULL 2008-06-24*/
  if (EncodedData == VOS_NULL_PTR)
  {
     HI_ERR_Processor("SMTPC_SEND_CONNAuthLoginPlain, malloc EncodedData error");
     SMTPC_Free(Data);
     return -1;
  }
  SMTPC_CODE_Base64Encode(EncodedData, Data, DataSize);
  SMTPC_Free(Data);
  
  bufSize = EncodedDataSize + 14;
  buf = (VOS_CHAR *)SMTPC_Malloc(bufSize);
  /*modify l59217 check buf whether NULL 2008-06-24*/
  if (buf == VOS_NULL_PTR)
  {
     HI_ERR_Processor("SMTPC_SEND_CONNAuthLoginPlain, malloc buf error");
     SMTPC_Free(EncodedData);
     return -1;
  }
  strcpy(buf, "AUTH PLAIN ");
  strcat(buf, "\r\n");

  if(SMTPC_SENDSocketSend(buf) == -1)
  {
      HI_ERR_Processor("SMTPC_SEND_CONNAuthLoginPlain, send auth data error");
      SMTPC_Free(EncodedData);
      SMTPC_Free(buf);
      return -1;
  }
 
  if(SMTPC_SEND_RESP_ReadResponse(334, VOS_NULL_PTR, 0) == -1)
  {
    HI_ERR_Processor("SMTPC_SEND_CONNAuthLoginPlain, Can not receive 235 code after sending password");
    SMTPC_Free(EncodedData);
    SMTPC_Free(buf);
    return -1;
   }
  memset(buf,0,bufSize);
  strcpy(buf, EncodedData);
  
  strcat(buf, "\r\n");
  SMTPC_Free(EncodedData);
  if(SMTPC_SENDSocketSend(buf) == -1)
  {
     HI_ERR_Processor("SMTPC_SEND_CONNAuthLoginPlain, send auth data error");
     SMTPC_Free(buf);
     return -1;
  }
  SMTPC_Free(buf);
  
  if(SMTPC_SEND_RESP_ReadResponse(235, VOS_NULL_PTR, 0) == -1)
  {
      HI_ERR_Processor("SMTPC_SEND_CONNAuthLoginPlain, Can not receive 235 code after sending password");
      return -1;
  }
  
  return 0;
}

/***********************************************************************************
* Function:      SMTPC_SEND_RESP_ReadResponse
* Description:   Read the response of the smtp server
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:          ExpectCmd    parameter description
* Output:        none
* Return:        the response string from the smtp server:  success
*                    VOS_NULL_PTR:                                      fail
* Others:        none
***********************************************************************************/
VOS_INT32 SMTPC_SEND_RESP_ReadResponse(VOS_UINT32 ExpectCmd, VOS_CHAR *Output, VOS_UINT32 OutLen)
{
  VOS_CHAR *End = "\r\n";
  VOS_CHAR *Pos1 = NULL;
  VOS_CHAR *Pos2 = NULL;
  VOS_INT32  Posexpect = 0;

  /*modify by l59217, 2008-06-13,chang uint to int*/
  VOS_INT32 ReceivedDataNum = 0;
  VOS_UINT32 DataNum = 0;
  VOS_INT32 IsFoundEnd = 0;
  VOS_CHAR rsp[5];
  VOS_CHAR rspend[5];
  VOS_CHAR *Sentence = NULL;
  VOS_CHAR  *RecvBuffer;
  VOS_UINT32 SentenceSize;

  VOS_UINT32 StartTime;
  VOS_UINT32 TimeNow = 0;

  VOS_UINT32 SentenceLen = 0;
  VOS_INT32 RetValue = 0;
  VOS_UINT32 SentenceDataLen = 0;
  VOS_UINT32 i = 1;
  int ret;

  /*modify by q46326, 2007-04-29*/
  Sentence = (VOS_CHAR *)SMTPC_Malloc(SMTPC_SEND_RESP_BUFSIZE);
  if ( Sentence == NULL)
  {
      printf("no mem!\n");
      return -1;
  }  
  RecvBuffer = (VOS_CHAR *)SMTPC_Malloc(SMTPC_SEND_RESP_BUFSIZE);
  if ( RecvBuffer == NULL )
  {
      printf("no mem!\n");
      SMTPC_Free(Sentence);
      return -1;
  }
  /*modify by l59217, 2008-06-06*/
  memset(Sentence, 0, SMTPC_SEND_RESP_BUFSIZE);
  memset(RecvBuffer, 0, SMTPC_SEND_RESP_BUFSIZE);
  
  SentenceSize = SMTPC_SEND_RESP_BUFSIZE;
  
  sprintf(rsp,"%d-",ExpectCmd);
  rsp[4] = '\0';
  
  sprintf(rspend,"%d ",ExpectCmd);
  rspend[4] = '\0';  
  
  StartTime = time((time_t *)VOS_NULL_PTR);

  while(!IsFoundEnd)
  {
	TimeNow = time((time_t *)VOS_NULL_PTR);
	//if(TimeNow - StartTime > SMTPC_SEND_RESP_TIMEOUT)
	if(TimeNow - StartTime > s_smtpSendRespTimeout)
	{
		HI_ERR_Processor("SMTPC_SEND_RESP_ReadResponse,responese time out");
		/*HI_ERR_Processor(Sentence);*/
		SMTPC_Free(Sentence);
		SMTPC_Free(RecvBuffer);
		return -1;
	}

    ret = SMTPC_SENDIsReadable();

    if( ret == -1)
    {
        HI_ERR_Processor("SMTPC_SEND_RESP_ReadResponse, time out from smtp server");
        /*HI_ERR_Processor(Sentence);*/
        SMTPC_Free(Sentence);
        SMTPC_Free(RecvBuffer);
        return -1;
    }
    else if ( ret == 0 )
    {
        continue;
    }

	if((ReceivedDataNum = SMTPC_SENDSocketReceive(RecvBuffer , SMTPC_SEND_RESP_BUFSIZE - 1)) >= 0)
	{
    		if(0 == ReceivedDataNum){

			continue;
		}

		*(RecvBuffer + ReceivedDataNum) = '\0';
	    /*modify by q46326, 2007-04-29*/
             //printf("[S]: %s\n", RecvBuffer);

		/* find the \r\n position */
		Pos1 = strstr(RecvBuffer,End);
		if(!Pos1)
		{

			memcpy(&Sentence[SentenceDataLen],RecvBuffer,ReceivedDataNum);
			SentenceDataLen += ReceivedDataNum;
			i++;
			Sentence = (VOS_CHAR *) SMTPC_Realloc(Sentence,i * SMTPC_SEND_RESP_BUFSIZE);
            if (Sentence == NULL)
            {
                HI_ERR_Processor("SMTPC_SEND_RESP_ReadResponse,SMTPC_Realloc no mem!\n");
                SMTPC_Free(RecvBuffer);
                return -1;
            }
            SentenceSize = i*SMTPC_SEND_RESP_BUFSIZE;
            /*modified by l59217 2008-06-06*/
            memset(&Sentence[SentenceDataLen], 0, SentenceSize - SentenceDataLen);
			continue;
		}

		Pos2 = RecvBuffer;


	  	while(1)
        {

    			/*
			TimeNow = time((time_t *)VOS_NULL_PTR);
			if(TimeNow - StartTime > SMTPC_SEND_RESP_TIMEOUT)
			{
				HI_ERR_Processor("SMTPC_SEND_RESP_ReadResponse, time out from smtp server");
				HI_ERR_Processor(Sentence);
				SMTPC_Free(Sentence);
				return -1;
			}*/

    			/* copy to Sentence from RecvBuffer head to \r\n */
			SentenceLen = (Pos1 - Pos2) + 2;
			memcpy((Sentence+SentenceDataLen), Pos2, SentenceLen);
  			Posexpect = 0;
			Pos2 = Pos1 + 2;

			Sentence[SentenceDataLen + SentenceLen] = '\0';

			/* find the expectcmd answer */
			// "250-"
			if(strcasestr(Sentence,rsp) != NULL)
			{
				Posexpect |= RSPECT_CONTINUE;
			}
			if((Posexpect & RSPECT_CONTINUE) == 0)
			{
				if(strcasestr(Sentence,rspend) != NULL)
				{
					Posexpect |= RSPECT_END;
				}
			}
            /* can not find expectcmd answer end */
			if(!(Posexpect & RSPECT_END))
			{
				// not the respect end
				// ??
				if((Pos2 -RecvBuffer) >= ReceivedDataNum)
				{

                    break;
				}

				/* find the \r\n position */
				Pos1 = strstr(Pos2,End);
                if(!Pos1)
				{
					/* remain words not in one sentence,copy them to Sentence buffer */
					memset(Sentence, 0 , SentenceSize);
                    /*HI_ERR_Processor(Pos2);*/
                    /*modify l59217 2008-06-13 follow two lines*/
                    /*memcpy(Sentence, Pos2, (Pos2 - Pos1 + 2));*/
                    /* record the number of data in sentence */
					/*SentenceDataLen = Pos1 - Pos2 + 2;*/
                    memcpy(Sentence, Pos2, (ReceivedDataNum - SentenceLen));
                    /* record the number of data in sentence */
                    SentenceDataLen = SentenceLen;

                    break;
				}
				else
				{
					/*HI_ERR_Processor(Sentence);*/
					memset(Sentence, 0 , SentenceSize);
					SentenceDataLen = 0;
                }
			}
			else
			{
				/* find the sentence with expect word and \r\n*/
				IsFoundEnd = 1;
				RetValue = 0;
				SentenceDataLen += SentenceLen;

                break;
			}
		}
	}
	else
	{

        *(RecvBuffer + DataNum) = '\0';
		HI_ERR_Processor("SMTPC_SEND_RESP_ReadResponse, received data error, no data received");
		/*HI_ERR_Processor(Sentence);*/
		SMTPC_Free(Sentence);
		SMTPC_Free(RecvBuffer);
        return -1;
	}
  }
   // Output

	if((Output != VOS_NULL_PTR) && (SentenceDataLen > 4))
	{

      		if(OutLen < SentenceDataLen)
      		{
			HI_ERR_Processor("SMTPC_SEND_RESP_ReadResponse, the length of output is less then the buffer size");
			SMTPC_Free(Sentence);
			SMTPC_Free(RecvBuffer);
			return -1;
		}
		// Output buffer not need cmd
		memcpy(Output, Sentence+4,SentenceDataLen - 4);

		*(Output + SentenceDataLen - 4) = '\0';

		RetValue = SentenceDataLen - 4;
	}

	// just printf message from serve not error!
	/*modify by q46326, 2007-04-29*/
	//HI_ERR_Processor(Sentence);
	//printf("[Sentence]: %s\n", Sentence);
	SMTPC_Free(Sentence);
	SMTPC_Free(RecvBuffer);
	return RetValue;
  }

/***********************************************************************************
* Function:      SMTPC_SEND_RESP_ReadResponse
* Description:   Read the response of the smtp server,when not response repectly,copy that resp
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:          ExpectCmd    parameter description
* Output:        the unexpected response string
* Return:        the response string from the smtp server:  fail
*                    -1:                                    fail
*				    0:								success
* Others:        none
***********************************************************************************/
VOS_INT32 SMTPC_SEND_RESP_ReadResponse2(VOS_UINT32 ExpectCmd, VOS_CHAR *Output, VOS_UINT32 OutLen)
{
  VOS_CHAR *End = "\r\n";
  VOS_CHAR *Pos1 = NULL;
  VOS_CHAR *Pos2 = NULL;
  VOS_INT32  Posexpect = 0;

  /*modify by l59217, 2008-06-13,chang uint to int*/
  VOS_INT32 ReceivedDataNum = 0;
  VOS_UINT32 DataNum = 0;
  VOS_INT32 Responsed = 0;
  VOS_CHAR rsp[5];
  VOS_CHAR rspend[5];
  VOS_CHAR *Sentence = NULL;
  VOS_CHAR  *RecvBuffer;
  VOS_UINT32 SentenceSize;

  VOS_UINT32 StartTime;
  VOS_UINT32 TimeNow = 0;

  VOS_UINT32 SentenceLen = 0;
  VOS_INT32 RetValue = 0;
  VOS_UINT32 SentenceDataLen = 0;
  VOS_UINT32 i = 1;
  int ret;

  /*modify by q46326, 2007-04-29*/
  Sentence = (VOS_CHAR *)SMTPC_Malloc(SMTPC_SEND_RESP_BUFSIZE);
  if ( Sentence == NULL)
  {
      printf("no mem!\n");
      return -1;
  }
  RecvBuffer = (VOS_CHAR *)SMTPC_Malloc(SMTPC_SEND_RESP_BUFSIZE);
  if ( RecvBuffer == NULL )
  {
      printf("no mem!\n");
      SMTPC_Free(Sentence);
      return -1;
  }
  /*modify by l59217, 2008-06-06*/
  memset(Sentence, 0, SMTPC_SEND_RESP_BUFSIZE);
  memset(RecvBuffer, 0, SMTPC_SEND_RESP_BUFSIZE);

  SentenceSize = SMTPC_SEND_RESP_BUFSIZE;

  sprintf(rsp,"%d-",ExpectCmd);
  rsp[4] = '\0';

  sprintf(rspend,"%d ",ExpectCmd);
  rspend[4] = '\0';

  StartTime = time((time_t *)VOS_NULL_PTR);

  while(!Responsed)
  {
	TimeNow = time((time_t *)VOS_NULL_PTR);
	//if(TimeNow - StartTime > SMTPC_SEND_RESP_TIMEOUT)
	if(TimeNow - StartTime > s_smtpSendRespTimeout)
	{
		HI_ERR_Processor("SMTPC_SEND_RESP_ReadResponse,responese time out");
		/*HI_ERR_Processor(Sentence);*/
		SMTPC_Free(Sentence);
		SMTPC_Free(RecvBuffer);
		return -1;	
	}

    ret = SMTPC_SENDIsReadable();
    if( ret == -1)
    {
        HI_ERR_Processor("SMTPC_SEND_RESP_ReadResponse, time out from smtp server");
        /*HI_ERR_Processor(Sentence);*/
        SMTPC_Free(Sentence);
        SMTPC_Free(RecvBuffer);
        return -1;	
    }
    else if ( ret == 0 )
    {
        continue;
    }
    

	if((ReceivedDataNum = SMTPC_SENDSocketReceive(RecvBuffer , SMTPC_SEND_RESP_BUFSIZE - 1)) >= 0)
	{
    		if(0 == ReceivedDataNum){
			continue;
		}
			
		*(RecvBuffer + ReceivedDataNum) = '\0';
	    /*modify by q46326, 2007-04-29*/        
             //printf("[S]: %s\n", RecvBuffer);
		
		/* find the \r\n position */
		Pos1 = strstr(RecvBuffer,End);
		if(!Pos1)
		{
			memcpy(&Sentence[SentenceDataLen],RecvBuffer,ReceivedDataNum);
			SentenceDataLen += ReceivedDataNum;
			i++;
			Sentence = (VOS_CHAR *) SMTPC_Realloc(Sentence,i * SMTPC_SEND_RESP_BUFSIZE);
            if (Sentence == NULL)
            {
                HI_ERR_Processor("SMTPC_SEND_RESP_ReadResponse,SMTPC_Realloc no mem!\n");
                SMTPC_Free(RecvBuffer);
                return -1;	
            }
            SentenceSize = i*SMTPC_SEND_RESP_BUFSIZE;
            /*modified by l59217 2008-06-06*/
            memset(&Sentence[SentenceDataLen], 0, SentenceSize - SentenceDataLen);
			continue;
		}
		
		Pos2 = RecvBuffer;
		
	  	while(1)
        {
    			/*
			TimeNow = time((time_t *)VOS_NULL_PTR);
			if(TimeNow - StartTime > SMTPC_SEND_RESP_TIMEOUT)
			{
				HI_ERR_Processor("SMTPC_SEND_RESP_ReadResponse, time out from smtp server");
				HI_ERR_Processor(Sentence);
				SMTPC_Free(Sentence);
				return -1;	
			}*/
			
    			/* copy to Sentence from RecvBuffer head to \r\n */
			SentenceLen = (Pos1 - Pos2) + 2;
			memcpy((Sentence+SentenceDataLen), Pos2, SentenceLen);
  			Posexpect = 0;
			Pos2 = Pos1 + 2;

			Sentence[SentenceDataLen + SentenceLen] = '\0';
            
			/* find the expectcmd answer */
			// "250-"
			if(strcasestr(Sentence,rsp) != NULL)
			{
				Posexpect |= RSPECT_CONTINUE;
			}
			if((Posexpect & RSPECT_CONTINUE) == 0)
			{
				if(strcasestr(Sentence,rspend) != NULL)
				{
					Posexpect |= RSPECT_END;
				}
			}
            /* can not find expectcmd answer end */
			if(!(Posexpect & RSPECT_END))
			{
				// Output
				 if((Output != VOS_NULL_PTR) && (ReceivedDataNum > 4))
				 {
						 if(OutLen < SentenceDataLen)
						 {
						 HI_ERR_Processor("SMTPC_SEND_RESP_ReadResponse, the length of output is less then the buffer size");
						 SMTPC_Free(Sentence);
						 SMTPC_Free(RecvBuffer);
						 return -1;
					 }
					 // copy the received response temenated with '\0'
					 memcpy(Output, RecvBuffer, ReceivedDataNum + 1);

					 RetValue = ReceivedDataNum + 1;
					 Responsed = 1;
				 }
				 break;
			}
			else
			{
				/* find the sentence with expect word and \r\n*/
				Responsed = 1;
				RetValue = 0;
				SentenceDataLen += SentenceLen;
                break;
			}
		}
	}
	else
	{
        *(RecvBuffer + DataNum) = '\0';
		HI_ERR_Processor("SMTPC_SEND_RESP_ReadResponse, received data error, no data received");
		/*HI_ERR_Processor(Sentence);*/
		SMTPC_Free(Sentence);
		SMTPC_Free(RecvBuffer);
        return -1;
	}
  }

	// just printf message from serve not error!
	/*modify by q46326, 2007-04-29*/
	//HI_ERR_Processor(Sentence);
	//printf("[Sentence]: %s\n", Sentence);
	SMTPC_Free(Sentence);
	SMTPC_Free(RecvBuffer);	
	return RetValue; 
  }

/***********************************************************************************
* Function:       SMTPC_SEND_SM_SendMail
* Description:   Send the mail to the smtp server
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:         Mail     the mail to be send
* Output:       none
* Return:       0: success
*                 -1: fail
* Others:       none
***********************************************************************************/
VOS_INT32 SMTPC_SEND_SM_SendMail(PTR_SMTPC_CM_Mail_s Mail)
{
   PTR_SMTPC_CM_MailBody_s Mptr,MTmpPtr;
   
   /*send mail command*/
   VOS_CHAR *Sender = Mail->Sender->Next->Addr;

   VOS_CHAR *buf1 = "MAIL FROM:<";
   VOS_CHAR *buf2 = ">\r\n\0";
   VOS_UINT32 MailCmdSize = strlen(Sender) + strlen(buf1) + strlen(buf2) + 1;
   VOS_CHAR *MailCmd;
   MailCmd = (VOS_CHAR *)SMTPC_Malloc(MailCmdSize);
   /*modify l59217 check MailCmd whether NULL 2008-06-23*/
   if (MailCmd == VOS_NULL_PTR)
   {
      HI_ERR_Processor("SMTPC_SEND_SM_SendMail: malloc MailCmd error");
      return -1;
   }
   sprintf(MailCmd, "%s%s%s", buf1, Sender,buf2);
   
   if(SMTPC_SENDSocketSend(MailCmd) == -1)
   {
      HI_ERR_Processor("SMTPC_SEND_SM_SendMail: send mail command error");
	goto SMTPC_SEND_SM_SendMail_out;
   }

  if(SMTPC_SEND_RESP_ReadResponse(250, VOS_NULL_PTR, 0) == -1)
  {
     HI_ERR_Processor("SMTPC_SEND_SM_SendMail: Can not receive 250 code after sending MAIL comand");
	goto SMTPC_SEND_SM_SendMail_out;
  }
  
  /*end of sending mail command*/
  
  /*send rcpt command*/
  
  /* ToRecipient*/
  if(SMTPC_SEND_SMSendRecipients(Mail->ToRecipientsHead) == -1)
  {
     HI_ERR_Processor("SMTPC_SEND_SM_SendMail: send ToRecipients error");
     goto SMTPC_SEND_SM_SendMail_out;
  }
  /*end of ToReciient*/

   /* CcRecipient*/
  if(Mail->CcRecipientsHead != VOS_NULL_PTR)
  {
     if(SMTPC_SEND_SMSendRecipients(Mail->CcRecipientsHead) == -1)
     {
         HI_ERR_Processor("SMTPC_SEND_SM_SendMail: send CcRecipients error");
     }
  }
  /*end of CcReciient*/
  
/* BcRecipient*/
  if(Mail->BcRecipientsHead != VOS_NULL_PTR)
  {
     if(SMTPC_SEND_SMSendRecipients(Mail->BcRecipientsHead) == -1)
     {
         HI_ERR_Processor("SMTPC_SEND_SM_SendMail: send BcRecipients error");
     }
  }
  /*end of BcReciient*/
  /*end of sending rcpt command*/
  
  if(SMTPC_SENDSocketSend("DATA\r\n") == -1)
  {
      HI_ERR_Processor("SMTPC_SEND_SM_SendMail: send DATA command error");
	goto SMTPC_SEND_SM_SendMail_out;
  }

  if(SMTPC_SEND_RESP_ReadResponse(354, VOS_NULL_PTR, 0) == -1)
  {
     HI_ERR_Processor("SMTPC_SEND_SM_SendMail: Can not receive 354 code after sending DATA command");
	goto SMTPC_SEND_SM_SendMail_out;
  }
  
  /* send mail header*/
  if(SMTPC_SENDSocketSend(Mail->Header) == -1)
  {
     HI_ERR_Processor("SMTPC_SEND_SM_SendMail: send mail header error");
	goto SMTPC_SEND_SM_SendMail_out;
  } 
  /*end of  header*/
  
  if(Mail->MailBodyHead != VOS_NULL_PTR)
  {
     Mptr = Mail->MailBodyHead->Next;
     while (Mptr != NULL)
     {
        if(SMTPC_SEND_SMSendMailBody(Mptr) == -1)
        {
            HI_ERR_Processor("SMTPC_SEND_SM_SendMail: send mail body error");
            /*modify by q46326, 2007-04-29*/
            //SMTPC_Free((VOS_CHAR *)Mptr);
		goto SMTPC_SEND_SM_SendMail_out;
        }
        MTmpPtr = Mptr;
	 if(Mptr->Next == NULL)
	 {
	 	if(SMTPC_SENDSocketSend(Mptr->MailBodyFooter) == -1)
	  	{
	     		HI_ERR_Processor("SMTPC_SEND_SMSendMailBody: send mail body footer error");
	     		goto SMTPC_SEND_SM_SendMail_out;
  	 	}
	 }
        Mptr = Mptr->Next;
    }
//    SMTPC_Free((VOS_CHAR *)Mail->MailBodyHead);
  }

  if(SMTPC_SENDSocketSend("\r\n.\r\n") == -1)
  {
      HI_ERR_Processor("SMTPC_SEND_SM_SendMail: send '\r\n.\r\n' error");
	goto SMTPC_SEND_SM_SendMail_out;
  }

  if(SMTPC_SEND_RESP_ReadResponse(250, VOS_NULL_PTR, 0) == -1)
  {
      HI_ERR_Processor("SMTPC_SEND_SM_SendMail: Can not receive 250 code after sending '\r\n.\r\n'");
	goto SMTPC_SEND_SM_SendMail_out;
  }

   if(SMTPC_SENDSocketSend("QUIT\r\n") == -1)
  {
      HI_ERR_Processor("SMTPC_SEND_INF_DisConnect, send QUIT command error");
	goto SMTPC_SEND_SM_SendMail_out;
  }
  
  if(SMTPC_SEND_RESP_ReadResponse(221, VOS_NULL_PTR, 0) == -1)
  {
     HI_ERR_Processor("SMTPC_SEND_INF_DisConnect, Can not receive 221 code after sending QUIT");
     goto SMTPC_SEND_SM_SendMail_out;
  }

  SMTPC_SEND_INF_DisConnect();

  SMTPC_Free(MailCmd);
  return 0;
  
SMTPC_SEND_SM_SendMail_out:
	SMTPC_Free(MailCmd);	
	return -1;
}

/***********************************************************************************
* Function:       SMTPC_SEND_SMSendMailBody
* Description:   Send body of the mail
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:         mb     the MailBody node which is to be send
* Output:       none
* Return:       0: success
*                 -1: fail
* Others:       none
***********************************************************************************/
VOS_INT32 SMTPC_SEND_SMSendMailBody(PTR_SMTPC_CM_MailBody_s mb)
{
   VOS_CHAR buf[SMTPC_SEND_SM_FILEBUFFERSIZE];
   VOS_CHAR EncodeFileData[SMTPC_CODE_SetBEncodeOutputSize(SMTPC_SEND_SM_FILEBUFFERSIZE)];
   //VOS_CHAR EncodeFileData[]
   FILE *file;
   VOS_INT32 FileFD;
   VOS_UINT32 readlen = 0;
   VOS_UINT32 i = 0;
   fd_set fd;
          
   if(SMTPC_SENDSocketSend(mb->MailBodyHeader) == -1)
   {
      HI_ERR_Processor("SMTPC_SEND_SMSendMailBody: send mail body header error");
      return -1;
   }
//   SMTPC_Free(mb->MailBodyHeader);
   memset(EncodeFileData, 0, sizeof(EncodeFileData)); 
   if(mb->Text != VOS_NULL_PTR)
   {
      /*modify l59217 2008-06-18 align base64 76byte,mailtext also based64*/
      VOS_CHAR* ptmp = mb->Text;
      VOS_INT   ilen = 0;
      
      readlen = strlen(mb->Text);
      ilen = readlen + 1;
      do
      {
           if (ilen > (SMTPC_SEND_SM_FILEBUFFERSIZE - 1))
           {
              SMTPC_CODE_Base64Encode(EncodeFileData, ptmp, SMTPC_SEND_SM_FILEBUFFERSIZE - 1);
              ptmp += (SMTPC_SEND_SM_FILEBUFFERSIZE - 1);
           }
           else
           {
              SMTPC_CODE_Base64Encode(EncodeFileData, ptmp, ilen-1);
           }
           if(SMTPC_SENDSocketSend(EncodeFileData) == -1)
           {
              HI_ERR_Processor("SMTPC_SEND_SMSendMailBody: send mail body header error");
              return -1;
           }
           if(SMTPC_SENDSocketSend("\r\n") == -1)
           {
              HI_ERR_Processor("SMTPC_SEND_SMSendMailBody: send mail body header \r\n error");
              return -1;
           }
           ilen -= (SMTPC_SEND_SM_FILEBUFFERSIZE - 1);
           
      } while(ilen > 0);
//      SMTPC_Free(mb->Text);
   }
   else if(mb->Attachment != VOS_NULL_PTR)
   {
        //printf("#### open file %s\n", mb->Attachment);
          if((file = fopen(mb->Attachment, "r+")) == VOS_NULL_PTR)
          //if((file = fopen(mb->Attachment, "rb")) == VOS_NULL_PTR)          
          {
              HI_ERR_Processor("SMTPC_SEND_SMSendMailBody: open attachment error");
              //SMTPC_Free(mb->Attachment);
              return -1;
          }
//          SMTPC_Free(mb->Attachment);
          FileFD = fileno(file);
          FD_ZERO(&fd);
          FD_SET(FileFD, &fd);
          if(select(FileFD+1, &fd, NULL, NULL, 0) == -1)
          {
              HI_ERR_Processor("SMTPC_SEND_SMSendMailBody: The attachment is not ready");
              return -1;
          }
          memset(buf, 0, SMTPC_SEND_SM_FILEBUFFERSIZE);
          while(!feof(file))
          {
              readlen = fread(buf, 1, SMTPC_SEND_SM_FILEBUFFERSIZE - 1, file);
              SMTPC_CODE_Base64Encode(EncodeFileData, buf, readlen);
              //SMTP_DEBUG("readlen = %d, baselen=%d, i=%d\n", readlen, strlen(EncodeFileData), i);             
	 	      i++;
              if(SMTPC_SENDSocketSend(EncodeFileData) == -1)
              {
                  HI_ERR_Processor("SMTPC_SEND_SMSendMailBody: send file data error");
                  /*modify by q46326, 2007-04-29*/
                  //SMTPC_Free(EncodeFileData);
                  return -1;
              }
              if(SMTPC_SENDSocketSend("\r\n") == -1)
              {
                  HI_ERR_Processor("SMTPC_SEND_SMSendMailBody: send file data \r\nerror");
                  /*modify by q46326, 2007-04-29*/
                  //SMTPC_Free(EncodeFileData);
                  return -1;
              }
              memset(buf, 0, SMTPC_SEND_SM_FILEBUFFERSIZE);
   		      memset(EncodeFileData,0,SMTPC_CODE_SetBEncodeOutputSize(SMTPC_SEND_SM_FILEBUFFERSIZE));
          }          
          fclose(file);
  }
  else
  {
     HI_ERR_Processor("SMTPC_SEND_SMSendMailBody: the body has no text or attachment");
     return -1;
  }
  
  /*if(SMTPC_SENDSocketSend(mb->MailBodyFooter) == -1)
  {
     HI_ERR_Processor("SMTPC_SEND_SMSendMailBody: send mail body footer error");
     return -1;
  }*/
  
  return 0;
}

/***********************************************************************************
* Function:       SMTPC_SEND_SMSendRecipients
* Description:   Send Recipients addresses
* Data Accessed: (Optional)
* Data Updated:  (Optional)
* Input:         Head     address head
* Output:       none
* Return:       0: success
*                 -1: fail
* Others:       none
***********************************************************************************/
VOS_INT32 SMTPC_SEND_SMSendRecipients(PTR_SMTPC_CM_Address_s Head)
{
  PTR_SMTPC_CM_Address_s ptr, TmpPtr;
  VOS_CHAR *RecipientBuf;
  VOS_CHAR *Recipient;
  int sendOk = SEND_FAILED;
  VOS_CHAR RespBuf[512] = {0};

  if(Head == NULL) {
  	return 0;
  }
  
  ptr = Head->Next;
  while (ptr != NULL)
  {
     Recipient = ptr->Addr;
     RecipientBuf = (VOS_CHAR *)SMTPC_Malloc(strlen(Recipient) + 15);
     /*modify l59217 check RecipientBuf whether NULL 2008-06-24*/
     if (RecipientBuf == VOS_NULL_PTR)
     {
        HI_ERR_Processor("SMTPC_SEND_SMSendRecipients: malloc recipientbuf error");
        return -1;
     }
     sprintf(RecipientBuf, "RCPT TO:<%s>\r\n", Recipient);
     if(SMTPC_SENDSocketSend(RecipientBuf) == -1)
     {
        HI_ERR_Processor("SMTPC_SEND_SMSendRecipients: send to recipient error");
        /*HI_ERR_Processor(RecipientBuf);*/ 
        SMTPC_Free(RecipientBuf);
        return -1;
     }
     SMTPC_Free(RecipientBuf);

	int ret = SMTPC_SEND_RESP_ReadResponse2(250, RespBuf, 512);

     if(ret == -1)
     {
         HI_ERR_Processor("SMTPC_SEND_SMSendRecipients: Can not receive 250 code after sending a to recipient");
	     return -1;
     } else if (ret != 0) {
		if (strstr(RespBuf, "550")) {
			HI_ERR_Processor("SMTPC_SEND_SMSendRecipients: Unknown User!\n");
			TmpPtr = ptr;
			ptr = ptr->Next;
			continue;
		}
     }

	//when success one time,it will be set
	 sendOk = SEND_OK;

     TmpPtr = ptr;
     ptr = ptr->Next;
  }
  return sendOk;
}
