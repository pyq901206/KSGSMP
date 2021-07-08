#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <string.h>
#include "author.h"
#include <time.h>

typedef struct _AuthorBook_T{
	char c; 
	int x;
	int y; 
}AuthorBook_T;

static AuthorBook_T g_book[64] = {
	{'0',0,0},{'1',0,1},{'2',0,2},{'3',0,3},{'4',0,4},{'5',0,5},{'6',0,6},{'7',0,7},	
	{'8',1,0},{'9',1,1},{'a',1,2},{'b',1,3},{'c',1,4},{'d',1,5},{'e',1,6},{'f',1,7},	
	{'g',2,0},{'h',2,1},{'i',2,2},{'j',2,3},{'k',2,4},{'l',2,5},{'m',2,6},{'n',2,7},	
	{'o',3,0},{'p',3,1},{'q',3,2},{'r',3,3},{'s',3,4},{'t',3,5},{'u',3,6},{'v',3,7},
	{'w',4,0},{'x',4,1},{'y',4,2},{'z',4,3},{'A',4,4},{'B',4,5},{'C',4,6},{'D',4,7},
	{'E',5,0},{'F',5,1},{'G',5,2},{'H',5,3},{'I',5,4},{'J',5,5},{'K',5,6},{'L',5,7},
	{'M',6,0},{'N',6,1},{'O',6,2},{'P',6,3},{'Q',6,4},{'R',6,5},{'S',6,6},{'T',6,7},
	{'U',7,0},{'V',7,1},{'W',7,2},{'X',7,3},{'Y',7,4},{'Z',7,5},{'_',7,6},{'-',7,7}
};

int ehobook(char book[][9])
{
	int i = 0;
	for(i = 0; i < 64;i++){
		if(i%8 == 0){
			printf("\n");
		}
		printf("%c ",book[i/8][i%8]);
	}
	printf("\n");
	return 0;
}

int ExchangeBookX(char book[][9],int x)
{	
	//printf("ExchangeBookX x = %d\r\n",x);
	int i = 0;
	for(i = 0;i < 8;i++){
		char c = book[x][i];
		book[x][i] = book[0][i]; 
		book[0][i] = c;
	}
	return 0;
}


int ExchangeBookY(char book[][9],int y)
{	
	int i = 0;
	//printf("ExchangeBookY y = %d\r\n",y);
	for(i = 0;i < 8;i++){
		char c = book[i][y];
		book[i][y] = book[i][0]; 
		book[i][0] = c;
	}
	return 0;
}

int encryoptionstring(char *name,char book[][9])
{
	int i = 0;
	int j = 0;
	for(i = 0;i < strlen(name);i++){
		for(j = 0; j < 64; j++){
			if(name[i] == g_book[j].c){
				name[i] = book[g_book[j].x][g_book[j].y];
				break;
			}
		}
	}
	return 0;
}

int decodeoptionstring(char *name,char book[][9])
{
	int i = 0;
	int j = 0;
	for(i = 0;i < strlen(name);i++){
		for(j = 0; j < 64; j++){
			if(name[i] == book[j/8][j%8]){
				name[i] = g_book[j].c;
				break;
			}
		}
	}
	return 0;
}

int ExBook2Port(char book[][9],int port)
{
	ExchangeBookX(book,(port/1000)%6 + 1);
	ExchangeBookY(book,(port/100)%6 + 1);
	ExchangeBookX(book,(port/10)%6 + 1);
	ExchangeBookY(book,port%6 + 1);
	ExchangeBookX(book,(port/7)%6 + 1);
	ExchangeBookY(book,(port/13)%6 + 1);
	ExchangeBookX(book,1);
	ExchangeBookY(book,2);
	ExchangeBookX(book,3);
	ExchangeBookY(book,4);
	ExchangeBookX(book,5);
	ExchangeBookY(book,6);
	ExchangeBookX(book,7);
	ExchangeBookY(book,(port/1000)%6 + 1);
	ExchangeBookX(book,(port/100)%6 + 1);
	ExchangeBookY(book,(port/10)%6 + 1);
	ExchangeBookX(book,port%6 + 1);
	ExchangeBookY(book,(port/7)%6 + 1);
	ExchangeBookX(book,(port/13)%6 + 1);
	return 0;
} 

int author(char *name,int port,char *out)
{
	char book[8][9]={
		"01234567",
		"89abcdef",
		"ghijklmn",
		"opqrtsuv",
		"wxyzABCD",
		"EFGHIJKL",
		"MNOPQRST",
		"UVWXYZ_-"
	};
	int crc = 0;
	int i = 0;
	for(i = 0; i < strlen(name);i++){
		crc += name[i];
	}
	sprintf(out,"%s_crc_%d",name,crc);
	ExBook2Port(book,port);
	encryoptionstring(out,book);
	return 0;
}



int paresAuthor(char *authorstr,int port,char *name)
{
	char book[8][9]={
		"01234567",
		"89abcdef",
		"ghijklmn",
		"opqrtsuv",
		"wxyzABCD",
		"EFGHIJKL",
		"MNOPQRST",
		"UVWXYZ_-"
	};

	ExBook2Port(book,port);
	
	char out[256] = {0};
	sprintf(out,"%s",authorstr);
	decodeoptionstring(out,book);
	char *p = NULL;
	p = strstr(out,"_crc_");
	if(NULL == p){
		return -1;
	}
	memcpy(name,out,p - out);
	int i = 0;
	int crc = 0;
	for(i = 0; i < strlen(name);i++){
		crc += name[i];
	}
	if(crc != atoi(p+strlen("_crc_"))){
		return -1;
	}
	return 0;
}

static int GetUUIDNonce(char *uuid)
{
	char uuidNumber[8] = {0};
	char *ps = strstr(uuid,"-") + 1;
	char *pe = strstr(ps,"-");
	memcpy(uuidNumber,ps,pe-ps);
	return atoi(uuidNumber);
}

int GetLicense(char *uuid,char *license)
{
	author(uuid,GetUUIDNonce(uuid)*2,license);
	return 0;
}


int GetTopic(char *uuid,char *topic)
{
	author(uuid,GetUUIDNonce(uuid)*4,topic);
}

int GetEcryPublicKey(char *key,char *uuid,char *eryKey)
{
	author(key,GetUUIDNonce(uuid)*2,eryKey);
	return 0;
}

int GetDeCodePublicKey(char *eryKey,char *uuid,char *key)
{
	paresAuthor(eryKey,GetUUIDNonce(uuid)*2,key);
	return 0;
}

int GetRandString(int uuidcont, int uuidLen, char outUuid[][64])
{	
	char randstr[62] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z'};
	int i = 0, j = 0;
	int num = 0, count = 0;
	char uuid[64] = {0};
	//srand(time(NULL));//设置随机数种子
	for (j = 0; j < uuidcont; j++)
	{
		for (i = 0; i < uuidLen; i++)
		{
			num = rand()%62;
			sprintf(uuid + i, "%c", randstr[num]);
		}
		snprintf(outUuid[j], 64, "%s", uuid);
	}
	return 0;
}


static int LoadConfig(const char *path,char **str)
{
	if(NULL == path){
		return -1;
	}
	//DF_DEBUG("path = %s",path);
	FILE *fp = fopen(path, "r"); /* 只供读取 */

    if (NULL == fp) /* 如果失败了 */
    {
        perror("fopen:");
		return -1;
	}
	fseek(fp,0,SEEK_END);
	int outLen = ftell(fp);
	fseek(fp,0,SEEK_SET);
	*str = (char *)malloc(outLen+1);
	memset(*str,0,outLen+1);
	fread(*str,outLen,1,fp);
	fclose(fp);
	return outLen;
}


static int WirteConf(const char *path,char *str)
{
	if(NULL == path){
		return -1;
	}
	FILE *fp = fopen(path, "w+"); 
    if (NULL == fp) /* 如果失败了 */
    {
        perror("fopen:");
		return -1;
	}
	fwrite(str,1,strlen(str),fp);
	fclose(fp);
	return 0;
}


#if 0

int main()
{	
	
	char *uuidfile = NULL; 
	char licenseTxt[1024*1024] = {0};
	char uuid[]={"INEW-001234-UUYTG"};
	char license[128] = {0};
	char topic[128] = {0};
	
	LoadConfig("./uuid.txt",&uuidfile);
	if(uuidfile == NULL){
		return -1;
	}
	char *tokenPtr=strtok(uuidfile,"\r\n");
    while(tokenPtr != NULL){
		GetLicense(tokenPtr,license);
		strcat(licenseTxt,license);
		strcat(licenseTxt,"\r\n");
		//printf("license = %s\r\n",license);
        tokenPtr=strtok(NULL,"\r\n");
    }
	WirteConf("./license.txt",licenseTxt);
//GetTopic(uuid,topic);
//	printf("license = %s\r\n",license);
//	printf("topic   = %s\r\n",topic);
	free(uuidfile);
	return 0;
}

#endif

