/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : smtp_log.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/02/28
  Description   : smtp_log.c header file
  History       :
  1.Date        : 2008/02/28
    Author      : qushen
    Modification: Created file

******************************************************************************/

#ifndef __SMTP_LOG_H__
#define __SMTP_LOG_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#ifdef __SMTP_DEBUG__
#define DEBUG_POS 	printf("=======>%s:%d(\"%s\")\n", __FILE__, __LINE__, __FUNCTION__)

#define SMTP_INFO(args...)   \
do{   \
    printf("[info]%s(%s: %d) : ", __FUNCTION__, __FILE__, __LINE__);  \
    printf(args);    \
}while(0)

#define SMTP_DEBUG(args...)   \
do{   \
    printf("[debug]%s(%s: %d) : ", __FUNCTION__, __FILE__, __LINE__);  \
    printf(args);    \
}while(0)

#else

#define DEBUG_POS 	         do{}while(0)

#define SMTP_INFO(args...)   do{}while(0)

#define SMTP_DEBUG(args...)  do{}while(0)

#endif

#define SMTP_ERROR(args...)   \
do{   \
    printf("[error]%s(%s: %d) : ", __FUNCTION__, __FILE__, __LINE__);  \
    printf(args);    \
}while(0)


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* End of #ifndef __SMTP_LOG_H__ */
