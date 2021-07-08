/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : HI_IniPareser.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2006/12/29
  Description   : IniPareser interface
  History       :
  1.Date        : 20067/12/29
    Author      : guiliangping 60020692
    Modification: Created file

******************************************************************************/

#if !defined(AFX_HI_INIPARESER_H__499DCD85_B085_459A_9401_A136BECA0EB0__INCLUDED_)
#define AFX_HI_INIPARESER_H__499DCD85_B085_459A_9401_A136BECA0EB0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <io.h>
#include "iniparser.h"
#include "dictionary.h"
#include "strlib.h"

#define  HI_ERROR_INIP_NOTINI -3
#define  HI_ERROR_INIP_HASBEENINI -4 
#define  HI_ERROR_INIP_READEONLYFILE -5

//#define UNDEFINE_HRESULT

#ifdef  UNDEFINE_HRESULT

#define TRUE  1
#define FALSE 0
#define E_INVALIDARG 0x80000003L
typedef long HRESULT ;
typedef long BOOL ;

#endif
class CHI_IniPareser  
{
public:
	CHI_IniPareser();
	virtual ~CHI_IniPareser();

	HRESULT add_entry( char * sec,  char * key,  char * val);
	HRESULT getnsec(int& number);
	HRESULT getsecname(int n,char **Name);
	HRESULT dump_ini( char* pFileNmae);
//	HRESULT dump( char* pFileNmae);
	HRESULT getstr( char * key,char** ppStrReturn);
	HRESULT getstring( char * key, char** ppStrReturn);
	HRESULT getint( char * key,int& retValue);
	HRESULT getdouble( char * key, double& retValue);
	HRESULT getboolean( char * key, BOOL& returnValue);
	HRESULT setstr( char * entry,  char * val);
	HRESULT unset( char * entry);
	HRESULT find_entry( char * entry);
	HRESULT loadFile( char * pininame);
	HRESULT freedict();
private:
	FILE * m_pfile ;
	char*  m_pFileName ;
	dictionary* m_pDictionary ;
	BOOL   m_iniState;

};

#endif // !defined(AFX_HI_INIPARESER_H__499DCD85_B085_459A_9401_A136BECA0EB0__INCLUDED_)
