// USBProbe.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

#define CALIBRATE_NUM_SAMPLES	50;

// CUSBProbeApp:
// See USBProbe.cpp for the implementation of this class
//

class CUSBProbeApp : public CWinApp
{
public:
	CUSBProbeApp();

// Overrides
	public:
	virtual BOOL InitInstance();

	// User defined messages
	UINT m_MsgToCalWindow_Volts;
	UINT m_MsgToCalWindow_SWVersion;
	UINT m_MsgToMainWindow_ReadVoltage;
	UINT m_MsgToMainWindow_SaveProbeData;
	UINT m_MsgToMainWindow_DisplayDebugText;
	UINT m_MsgToMainWindow_CalCheck;

	DECLARE_MESSAGE_MAP()
};

extern CUSBProbeApp theApp;

typedef union {
	float fvalue;
	unsigned char bvalue[4];
} FloatByteUnion_type;

typedef union {
	double fvalue;
	unsigned char bvalue[8];
} DoubleByteUnion_type;

typedef union {
	short shortvalue;
	unsigned char bvalue[2];
} ShortByteUnion_type;

typedef union {
	int longvalue;
	unsigned char bvalue[4];
} LongByteUnion_type;

typedef union {
	__int64 int64;
	unsigned char bvalue[8];
} Int64ByteUnion_type;
