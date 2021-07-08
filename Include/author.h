#ifndef __AUTHOR_H__
#define __AUTHOR_H__

int GetTopic(char *uuid,char *topic);
int GetLicense(char *uuid,char *license);
int GetEcryPublicKey(char *key,char *uuid,char *eryKey);
int GetDeCodePublicKey(char *eryKey,char *uuid,char *key);
int GetRandString(int uuidcont, int uuidLen, char outUuid[][64]);

#endif
