#ifndef BIN2HEX_H
#define BIN2HEX_H
#include <stddef.h>
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

/***********************************
*********************************************
就是每次读写bin文件N个字节，然后再转化为hex格式流，hex格式流长度计算方式
: + 长度 + 地址 + 类型 + N个数据(N >= 0) + 校验
1 + 2	 + 4	+ 2	   + N * 2			 + 2
********************************************************************************/
#define NUMBER_OF_ONE_LINE		0x20
#define	MAX_BUFFER_OF_ONE_LINE	(NUMBER_OF_ONE_LINE * 2 + 11) 
typedef struct {
	uint8_t len;
	uint8_t addr[2];
	uint8_t type;
	uint8_t *data;
} HexFormat;
 
typedef enum {
	RES_OK = 0,						//操作完成
	RES_BIN_FILE_NOT_EXIST,			//相当于bin文件不存在，包括输入的路径可能存在不正确
	RES_HEX_FILE_PATH_ERROR			//目标文件路径可能输入有误			
} RESULT_STATUS;
 
RESULT_STATUS BinFile2HexFile(char *src, char *dest);
void bin2str(char *to, const unsigned char *p, size_t len);
void str2bin(const char* source, unsigned char * dest, size_t len);

#endif
