// USBProbe.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "USBProbe.h"
#include "USBProbeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CUSBProbeApp
BEGIN_MESSAGE_MAP(CUSBProbeApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CUSBProbeApp construction
CUSBProbeApp::CUSBProbeApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CUSBProbeApp object
CUSBProbeApp theApp;


// CUSBProbeApp initialization
BOOL CUSBProbeApp::InitInstance()
{
	// Initialize richedit2 library
	AfxInitRichEdit2();

	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	AfxEnableControlContainer();

	// Standard initialization
	SetRegistryKey(_T("Nico Scientific - detectIon Application"));

	//Register user defined messages
	m_MsgToCalWindow_Volts				= RegisterWindowMessage( _T("Volts")		);
	m_MsgToCalWindow_SWVersion			= RegisterWindowMessage( _T("SWVersion")	);
	m_MsgToMainWindow_ReadVoltage		= RegisterWindowMessage( _T("ReadVoltage")	);
	m_MsgToMainWindow_SaveProbeData		= RegisterWindowMessage( _T("SaveData")		);
	m_MsgToMainWindow_DisplayDebugText	= RegisterWindowMessage( _T("DisplayDebug")	);
	m_MsgToMainWindow_CalCheck			= RegisterWindowMessage( _T("CalCheck")	);

	CUSBProbeDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Handle dialog OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Handle dialog Cancel
	}

	// Return false to exit program
	return FALSE;
}
