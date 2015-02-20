/////////////////////////////////////////////////////////////////////////////
// USBProbeDlg.h : header file
/////////////////////////////////////////////////////////////////////////////

#pragma once

/////////////////////////////////////////////////////////////////////////////
// Includes
/////////////////////////////////////////////////////////////////////////////

#include "SLABHIDDevice.h"
#include "afxwin.h"
#include "afxcmn.h"
#include "CalibrateDlg.h"

/////////////////////////////////////////////////////////////////////////////
// Definitions
/////////////////////////////////////////////////////////////////////////////

// USB Parameters
#define VID								0x10C4
#define PID								0x8468
#define HID_READ_TIMEOUT				0
#define HID_WRITE_TIMEOUT				1000

// Calibration limits
//#define CAL_VOLTAGE_MIN				-1.024
//#define CAL_VOLTAGE_MAX				+1.024
#define CAL_VOLTAGE_MIN					1
#define CAL_VOLTAGE_MAX					10000

// HID Report IDs
#define ID_IN_CONTROL					0xFE
#define ID_OUT_CONTROL					0xFD
#define ID_IN_DATA						0x01
#define ID_OUT_DATA						0x02

// HID Report Sizes
// For testing. Make same as SL + 1
#define SIZE_IN_CONTROL					9	// Was 5
#define SIZE_OUT_CONTROL				9	// Was 5

#define SIZE_IN_DATA					61
#define SIZE_OUT_DATA					61
#define SIZE_MAX_WRITE					59
#define SIZE_MAX_READ					59

// Read Timer Definitions
#define TIMER_READ_ID					10		// Was zero
#define TIMER_READ_ELAPSE				15

/////////////////////////////////////////////////////////////////////////////
// CUSBProbeDlg dialog
/////////////////////////////////////////////////////////////////////////////

class CUSBProbeDlg : public CDialog
{
// Construction
public:
	CUSBProbeDlg(CWnd* pParent = NULL);	// standard constructor
	CFont m_Font;
	enum { IDD = IDD_USBPROBE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Protected Members
protected:
	HICON		m_hIcon;
	HICON		m_hSmallIcon;
	HID_DEVICE	m_hid;
	HDEVNOTIFY	m_hNotifyDevNode;

// Protected Methods
protected:
	void InitializeDialog();
	void RegisterDeviceChange();
	void UnregisterDeviceChange();
	BOOL Connect();
	BOOL Disconnect();
	BOOL GetSelectedDevice(CString& path);
	BOOL FindDevice(CString path, DWORD& deviceNum);
	void UpdateDeviceList();
	void EnableDeviceCtrls(BOOL bEnable);
	void StartReadTimer();
	void StopReadTimer();
	void AppendReceiveText(const CString& text);
	DECLARE_MESSAGE_MAP()

	// HID Report Methods
protected:
	BOOL GetCalData();
	void WriteDebugText( CString );
	BOOL TransmitData(const BYTE* buffer, DWORD bufferSize);
	BOOL ReceiveData(BYTE* buffer, DWORD bufferSize, DWORD& bytesRead);

public:
	//Mesage map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg BOOL OnDeviceChange(UINT nEventType, DWORD_PTR dwData);
	afx_msg void OnBnClickedCheckConnect();
	afx_msg void OnCbnDropdownComboDeviceList();
	afx_msg void OnCbnCloseupComboDeviceList();
	void GetProbeCalData();
	void GetProbeSWVersion();
	void WriteProbeCalData();
	afx_msg void OnBnClickedButtonReadVoltage();
	afx_msg void OnBnClickedButtonClear();
	afx_msg void OnBnClickedCalSetup();

	// Handlers for user defined messages
	afx_msg LRESULT OnMsg_ReadVoltage( WPARAM, LPARAM);
	afx_msg LRESULT OnMsg_SaveProbeData( WPARAM, LPARAM); 
	afx_msg LRESULT OnMsg_DisplayDebugText( WPARAM, LPARAM); 
	afx_msg LRESULT OnMsg_CalCheck( WPARAM, LPARAM); 

	// Data
	CRichEditCtrl	m_richReceive;
	CComboBox	m_comboDeviceList;
	CCalibrateDlg *m_pCalDlg;				// Pointer to calibration dialog window
	BOOL		m_VoltageReadActive;
	BOOL		m_CalDataReadActive;

	double		m_CalCheckSetpoint;
	BOOL		m_CalCheckBusy;
	short		m_CalCheckCount;
	double		m_CalCheckSum;
	double		m_CalCheckValue;
	BOOL		m_CalCheckReadDone;

	CColorStatic m_ReadingStatic;			// Shows reading (ppm or molar)
	CStatic m_LastCalibratedStatic;			// Shows calibrated date/time
	CStatic m_UnitIDStatic;

	double		m_Reading;
	double		m_RawVoltage;

	// Calibration data to/from probe
	unsigned short	m_CheckSum;
	short		m_SerialNumber;
	short		m_ConfigBits;
	short		m_UnitID;
	double		m_CalSetpointLow;
	double		m_CalSetpointHigh;
	double		m_CalValueLow;
	double		m_CalValueHigh;
	Int64ByteUnion_type	m_TimeUnion;

	//Probe SW Version
	float m_ProbeSWVersion;

	CStatic m_DebugTextStatic;
	CButton m_DebugTextClearButton;
	CButton m_CalCheckButton;
	CButton m_CalSetupButton;
	CButton m_ReadProbeButton;
	afx_msg void OnBnClickedCalCheckButton();
	CEdit m_CalCheckSetpointEdit;
	afx_msg void OnEnKillfocusCalCheckSetpoint();
};


