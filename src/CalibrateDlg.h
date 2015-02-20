#pragma once

#include "afxwin.h"
#include "resource.h"
#include "ColorStatic.h"
#include "USBProbe.h"


// CCalibrateDlg dialog

class CCalibrateDlg : public CDialog
{
	DECLARE_DYNAMIC(CCalibrateDlg)
	CFont m_Font;

public:
	CCalibrateDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCalibrateDlg();
	enum { IDD = IDD_CALIBRATE };
	void PostNcDestroy(); 

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()

public:

	//Low calibration
	BOOL 		m_CalLowBusy;
	short		m_CalLowCount;
	double		m_CalValueLowSum;
	double		m_CalValueLow;
	BOOL		m_CalLowReadDone;
	//High calibration
	BOOL 		m_CalHighBusy;
	short		m_CalHighCount;
	double		m_CalValueHighSum;
	double		m_CalValueHigh;
	BOOL		m_CalHighReadDone;
	// Probe SW version
	float		m_ProbeSWVersion;

	//Mesage map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedReadProbe();
	afx_msg void OnBnClickedCancel();
	
	// Handlers for user defined messages
	afx_msg LRESULT OnMsgVolts( WPARAM, LPARAM ); 
	afx_msg LRESULT OnMsgSWVer( WPARAM, LPARAM ); 

	// Data
	CStatic m_ReadingStatic;
	CColorStatic m_CalLowValueStatic;
	CColorStatic m_CalHighValueStatic;
	CStatic m_LastCalibratedStatic;
	CStatic m_ProbeSWVersionStatic;

	double m_Reading;
	double m_RawVoltage;
	double m_CalSetpointLow;
	double m_CalSetpointHigh;
	Int64ByteUnion_type	m_TimeUnion;
	short m_UnitID;
	short m_SerialNumber;

	afx_msg void OnLbnSelchangeListLowCal();
	afx_msg void OnLbnSelchangeListHighCal();

	CListBox m_ListBox_LowCal;
	CListBox m_ListBox_HighCal;

	afx_msg void OnEnKillfocusEditLowCal();
	afx_msg void OnEnKillfocusEditHighCal();

	afx_msg void OnBnClickedStartLowCal();
	afx_msg void OnBnClickedStartHighCal();


	afx_msg void OnEnKillfocusEditUnitId();
	afx_msg void OnEnKillfocusEditSerNum();
	afx_msg void OnBnClickedSaveCalButton();

	CButton m_SaveCalDataButtonStatic;
	CButton m_ExitWithoutSavingButtonStatic;

	CButton m_StartLowCalButton;
	CButton m_StartHighCalButton;
	CEdit m_LowCalEditButton;
	CEdit m_HighCalEditButton;
	CEdit m_UnitIDEditButton;

};
