
// Hypoxio.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

struct tSPLITTERCOMSETTINGS
{
	int iComPort;
	int iBaudRate;
	char cParity;
	int iDataBits;
	int iStopBits;
};

// CHypoxioApp:
// See Hypoxio.cpp for the implementation of this class
//

class CHypoxioApp : public CWinApp
{
public:
	CHypoxioApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CHypoxioApp theApp;