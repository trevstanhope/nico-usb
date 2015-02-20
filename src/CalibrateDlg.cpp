// Calibrate.cpp : implementation file
//

#include "stdafx.h"
#include "CalibrateDlg.h"
#include "USBProbe.h"
#include "USBProbeDlg.h"


// Point to parent dialog that created this dialog
CUSBProbeDlg *m_pParent;

// CCalibrateDlg dialog
IMPLEMENT_DYNAMIC(CCalibrateDlg, CDialog)

CCalibrateDlg::CCalibrateDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCalibrateDlg::IDD, pParent)
{
	// Save point to parent dialog
	m_pParent = (CUSBProbeDlg*) pParent;

	// Initialize calibration counters
	m_CalLowBusy = FALSE;
	m_CalLowCount = 0;
	m_CalLowReadDone = TRUE;	// initialize to done to allow first read

	m_CalHighBusy = FALSE;
	m_CalHighCount = 0;
	m_CalHighReadDone = TRUE;	// initialize to done to allow first read
}

CCalibrateDlg::~CCalibrateDlg()
{
}

void CCalibrateDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_READING_STATIC, m_ReadingStatic);
	DDX_Text(pDX, IDC_EDIT_LOW_CAL, m_CalSetpointLow);
	DDX_Text(pDX, IDC_EDIT_HIGH_CAL, m_CalSetpointHigh);
	DDV_MinMaxDouble(pDX, m_CalSetpointLow, CAL_VOLTAGE_MIN, CAL_VOLTAGE_MAX);
	DDV_MinMaxDouble(pDX, m_CalSetpointHigh, CAL_VOLTAGE_MIN, CAL_VOLTAGE_MAX);

	DDX_Text(pDX, IDC_EDIT_UNIT_ID, m_UnitID);
	DDX_Text(pDX, IDC_EDIT_SER_NUM, m_SerialNumber);
	DDV_MinMaxShort(pDX, m_UnitID, 0, 10000);
	DDV_MinMaxShort(pDX, m_SerialNumber, 0, 10000);

	DDX_Control(pDX, IDC_LIST_LOW_CAL, m_ListBox_LowCal);
	DDX_Control(pDX, IDC_LIST_HIGH_CAL, m_ListBox_HighCal);
	DDX_Control(pDX, IDC_LOW_CAL_VALUE, m_CalLowValueStatic);
	DDX_Control(pDX, IDC_HIGH_CAL_VALUE, m_CalHighValueStatic);
	DDX_Control(pDX, IDC_CAL_DATE_TIME_STATIC, m_LastCalibratedStatic);
	DDX_Control(pDX, IDC_SAVE_CAL_BUTTON, m_SaveCalDataButtonStatic);
	DDX_Control(pDX, IDCANCEL, m_ExitWithoutSavingButtonStatic);
	DDX_Control(pDX, IDC_START_LOW_CAL, m_StartLowCalButton);
	DDX_Control(pDX, IDC_START_HIGH_CAL, m_StartHighCalButton);
	DDX_Control(pDX, IDC_EDIT_LOW_CAL, m_LowCalEditButton);
	DDX_Control(pDX, IDC_EDIT_HIGH_CAL, m_HighCalEditButton);
	DDX_Control(pDX, IDC_EDIT_UNIT_ID, m_UnitIDEditButton);
	DDX_Control(pDX, IDC_CAL_DATE_TIME_STATIC2, m_ProbeSWVersionStatic);
}


BEGIN_MESSAGE_MAP(CCalibrateDlg, CDialog)
	ON_BN_CLICKED(IDC_READ_PROBE_BUTTON, &CCalibrateDlg::OnBnClickedReadProbe)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDCANCEL, &CCalibrateDlg::OnBnClickedCancel)
	ON_LBN_SELCHANGE(IDC_LIST_LOW_CAL, &CCalibrateDlg::OnLbnSelchangeListLowCal)
	ON_LBN_SELCHANGE(IDC_LIST_HIGH_CAL, &CCalibrateDlg::OnLbnSelchangeListHighCal)
	ON_EN_KILLFOCUS(IDC_EDIT_LOW_CAL, &CCalibrateDlg::OnEnKillfocusEditLowCal)
	ON_EN_KILLFOCUS(IDC_EDIT_HIGH_CAL, &CCalibrateDlg::OnEnKillfocusEditHighCal)
	ON_BN_CLICKED(IDC_START_LOW_CAL, &CCalibrateDlg::OnBnClickedStartLowCal)
	ON_BN_CLICKED(IDC_START_HIGH_CAL, &CCalibrateDlg::OnBnClickedStartHighCal)
	ON_EN_KILLFOCUS(IDC_EDIT_UNIT_ID, &CCalibrateDlg::OnEnKillfocusEditUnitId)
	ON_EN_KILLFOCUS(IDC_EDIT_SER_NUM, &CCalibrateDlg::OnEnKillfocusEditSerNum)
	ON_BN_CLICKED(IDC_SAVE_CAL_BUTTON, &CCalibrateDlg::OnBnClickedSaveCalButton)
	// Handle user defined messages	
	ON_REGISTERED_MESSAGE( theApp.m_MsgToCalWindow_Volts, &CCalibrateDlg::OnMsgVolts)
	ON_REGISTERED_MESSAGE( theApp.m_MsgToCalWindow_SWVersion, &CCalibrateDlg::OnMsgSWVer)

END_MESSAGE_MAP()


// CCalibrate message handlers
afx_msg LRESULT CCalibrateDlg::OnMsgVolts( WPARAM wParam, LPARAM lParam ) 
{
	if ( m_CalLowBusy ) {
		// Get raw voltage from parent
		m_CalValueLowSum += m_pParent->m_RawVoltage;
		m_CalLowCount--;
		if ( m_CalLowCount == 0 ) {
			m_CalValueLow = m_CalValueLowSum / CALIBRATE_NUM_SAMPLES;
			m_CalLowBusy = FALSE;
			// Save and display value
			CString CalLow;
			CalLow.Format(_T("Low Cal = %2.6f volts"), m_CalValueLow);
			m_CalLowValueStatic.SetWindowTextW( CalLow);
			m_CalLowValueStatic.SetTextColor( BLACK );
		}
		m_CalLowReadDone = TRUE;
	}
	else if ( m_CalHighBusy ) {
		// Get raw voltage from parent
		m_CalValueHighSum += m_pParent->m_RawVoltage;
		m_CalHighCount--;
		if ( m_CalHighCount == 0 ) {
			m_CalValueHigh = m_CalValueHighSum / CALIBRATE_NUM_SAMPLES;
			m_CalHighBusy = FALSE;
			// Save and display value
			CString CalHigh;
			CalHigh.Format(_T("High Cal = %2.6f volts"), m_CalValueHigh);
			m_CalHighValueStatic.SetWindowTextW( CalHigh);
			m_CalHighValueStatic.SetTextColor( BLACK );
		}
		m_CalHighReadDone = TRUE;
	} else {
		// Only update text if not calibrating
		CString Data;
		m_Reading = m_pParent->m_Reading;				// This is ppm or molar
		// m_Reading is already bounds cheked in parent
		Data.Format(_T("Reading = %3.1f ppm"), m_Reading);
		m_ReadingStatic.SetWindowTextW( Data );
	}

	UpdateData( FALSE );
	return 0L;
}

afx_msg LRESULT CCalibrateDlg::OnMsgSWVer( WPARAM wParam, LPARAM lParam ) 
{
	CString TempString;
	m_ProbeSWVersion = m_pParent->m_ProbeSWVersion;
	TempString.Format(_T("Probe SW Ver:  %03.2f "), m_ProbeSWVersion);
	m_ProbeSWVersionStatic.SetWindowTextW( TempString);
	UpdateData( FALSE );
	return 0L;
}


BOOL CCalibrateDlg::OnInitDialog()
{
	CString TempString;

	CDialog::OnInitDialog();
	m_Font.CreatePointFont(150, _T("Tahoma"), NULL);

	//Initialize Reading Text
	m_ReadingStatic.SetFont( &m_Font );
	m_ReadingStatic.SetWindowTextW(_T("Reading = "));

	// Initialize dialog data
	// m_Reading is already bounds cheked in parent
	m_Reading = m_pParent->m_Reading;				// This is ppm or molar
	TempString.Format(_T("Reading = %3.1f ppm"), m_Reading);
	m_ReadingStatic.SetFont( &m_Font );
	m_ReadingStatic.SetWindowTextW( TempString);
	
	m_CalSetpointLow  = m_pParent->m_CalSetpointLow;
	if (m_CalSetpointLow != m_CalSetpointLow ) {	// NAN check for new probe
		m_CalSetpointLow = 10;
	}
	
	m_CalSetpointHigh = m_pParent->m_CalSetpointHigh;
	if (m_CalSetpointHigh != m_CalSetpointHigh ) {	// NAN check for new probe
		m_CalSetpointHigh = 100;
	}

	m_CalValueLow = m_pParent->m_CalValueLow;
	if (m_CalValueLow != m_CalValueLow ) {			// NAN check for new probe
		m_CalValueLow = -0.350;
	}
	TempString.Format(_T("Low Cal = %2.6f volts"), m_CalValueLow);
	m_CalLowValueStatic.SetWindowTextW( TempString);
	m_CalLowValueStatic.SetTextColor( BLACK );

	m_CalValueHigh = m_pParent->m_CalValueHigh;
	if (m_CalValueHigh != m_CalValueHigh ) {		// NAN check for new probe
		m_CalValueHigh = -0.400;
	}
	TempString.Format(_T("High Cal = %2.6f volts"), m_CalValueHigh);
	m_CalHighValueStatic.SetWindowTextW( TempString);
	m_CalHighValueStatic.SetTextColor( BLACK );

	m_UnitID = m_pParent->m_UnitID;
	if ( m_UnitID == -1 ) {		// Eliminate range error for new probe
		m_UnitID = 0;
	}

	m_SerialNumber = m_pParent->m_SerialNumber;
	// If a new probe the serial number is 0xFFFF = -1
	if ( m_SerialNumber != -1 ) {
		// Disable user entry of serial number
		GetDlgItem( IDC_EDIT_SER_NUM)->EnableWindow(FALSE);
	}

	m_TimeUnion.int64 = m_pParent->m_TimeUnion.int64;
	CTime theTime( m_TimeUnion.int64 );
	CString strCal  = theTime.Format (" %c");	// Calibrated: 02/25/12 13:41:20
	m_LastCalibratedStatic.SetWindowText( strCal );

	// Set color
	m_CalLowValueStatic.SetTextColor( BLACK );
	//m_CalLowValueStatic.SetBkColor( GRAY );
	m_CalHighValueStatic.SetTextColor( BLACK );
	//m_CalLowValueStatic.SetBkColor( GRAY );

	m_ListBox_LowCal.AddString( _T("0") );
	m_ListBox_LowCal.AddString( _T("1") );
	m_ListBox_LowCal.AddString( _T("10") );
	m_ListBox_LowCal.AddString( _T("100") );
	m_ListBox_LowCal.AddString( _T("1000") );
	m_ListBox_LowCal.AddString( _T("Other") );
	m_ListBox_LowCal.SetCurSel(0);

	m_ListBox_HighCal.AddString( _T("0") );
	m_ListBox_HighCal.AddString( _T("1") );
	m_ListBox_HighCal.AddString( _T("10") );
	m_ListBox_HighCal.AddString( _T("100") );
	m_ListBox_HighCal.AddString( _T("1000") );
	m_ListBox_HighCal.AddString( _T("Other") );
	m_ListBox_HighCal.SetCurSel(0);

	// Start our timer
	#define TIMER_CAL_ID	11
	SetTimer(TIMER_CAL_ID, 50, NULL);	// 50 = 50 msec

	UpdateData( FALSE );
	return TRUE;
}


void CCalibrateDlg::OnTimer(UINT_PTR nIDEvent)
{
	// MessageBeep(0xFFFFFFFF);

	// Low calibration.  Periodically read value
	if ( m_CalLowCount && m_CalLowReadDone) {
		// Read probe
		m_CalLowReadDone = FALSE;
		GetParent()->SendMessage ( theApp.m_MsgToMainWindow_ReadVoltage );
	}

	// High calibration.  Periodically read value
	if ( m_CalHighCount && m_CalHighReadDone) {
		// Read probe
		m_CalHighReadDone = FALSE;
		GetParent()->SendMessage ( theApp.m_MsgToMainWindow_ReadVoltage );
	}

	CDialog::OnTimer(nIDEvent);
}


void CCalibrateDlg::OnBnClickedReadProbe()
{
	// Send message to read voltage
	GetParent()->SendMessage ( theApp.m_MsgToMainWindow_ReadVoltage );
}

void CCalibrateDlg::OnBnClickedSaveCalButton()
{
	// Send message to save all calibration & setup data to probe
	GetParent()->SendMessage ( theApp.m_MsgToMainWindow_SaveProbeData );
	// Change this buttons text and disable the button
	m_SaveCalDataButtonStatic.SetWindowText( _T("Data Saved") );
	m_SaveCalDataButtonStatic.EnableWindow( FALSE );
	// Since we already saved the data, change the text on the "Exit Without Saving" button
	m_ExitWithoutSavingButtonStatic.SetWindowText( _T("Exit") );
	//Also disable all the other controls, except exit
	m_StartLowCalButton.EnableWindow( FALSE );
	m_StartHighCalButton.EnableWindow( FALSE );
	m_LowCalEditButton.EnableWindow( FALSE );
	m_HighCalEditButton.EnableWindow( FALSE );
	m_UnitIDEditButton.EnableWindow( FALSE );
	m_ListBox_LowCal.EnableWindow( FALSE);
	m_ListBox_HighCal.EnableWindow( FALSE);
	m_CalLowValueStatic.EnableWindow( FALSE);
	m_CalHighValueStatic.EnableWindow( FALSE);
	m_LastCalibratedStatic.EnableWindow( FALSE);
}

void CCalibrateDlg::OnBnClickedCancel()
{
	// Goes here for cancel and ESC
	// See: http://www.codeproject.com/Articles/1651/Tutorial-Modeless-Dialogs-with-MFC
	DestroyWindow();
}


void CCalibrateDlg::PostNcDestroy() 
{	
    CDialog::PostNcDestroy();
	m_pParent->m_pCalDlg = 0;	// Delete parents point to us
    delete this;
}


// On Low Calibration Setpoint list box change
void CCalibrateDlg::OnLbnSelchangeListLowCal()
{
	int Index = m_ListBox_LowCal.GetCurSel();
	
	switch ( Index ) {
		default:
		case 0:
			m_CalSetpointLow = 0.0;
			break;
		case 1:
			m_CalSetpointLow = 1.0;
			break;
		case 2:
			m_CalSetpointLow = 10.0;
			break;
		case 3:
			m_CalSetpointLow = 100.0;
			break;
		case 4:
			m_CalSetpointLow = 1000.0;
			break;
		case 5:
			//m_CalSetpointLow = 1234.0;		// Other
			break;
	}
	UpdateData( FALSE );

}

// On High Calibration Setpoint list box change
void CCalibrateDlg::OnLbnSelchangeListHighCal()
{
	int Index = m_ListBox_HighCal.GetCurSel();
	
	switch ( Index ) {
		default:
		case 0:
			m_CalSetpointHigh = 0.0;
			break;
		case 1:
			m_CalSetpointHigh = 1.0;
			break;
		case 2:
			m_CalSetpointHigh = 10.0;
			break;
		case 3:
			m_CalSetpointHigh = 100.0;
			break;
		case 4:
			m_CalSetpointHigh = 1000.0;
			break;
		case 5:
			//m_CalSetpointHigh = 1234.0;		// Other
			break;
	}
	UpdateData( FALSE );
}

void CCalibrateDlg::OnEnKillfocusEditLowCal()
{
	// Call update to test limits
	if ( UpdateData(TRUE) ) {
		// User entered a value in edit box that was excepted
		m_ListBox_LowCal.SetCurSel(5);		// Set list box to other
		UpdateData( FALSE );				// Refresh list box
	} else {
		// Range error
		m_CalSetpointLow = m_pParent->m_CalSetpointLow;
		UpdateData( FALSE );
		CEdit* edit = (CEdit*)GetDlgItem(IDC_EDIT_LOW_CAL);
		edit->SetFocus();
	}
		
}

void CCalibrateDlg::OnEnKillfocusEditHighCal()
{
	// Call update to test limits
	if ( UpdateData(TRUE) ) {
		// User entered a value in edit box that was excepted
		m_ListBox_HighCal.SetCurSel(5);		// Set list box to other
		UpdateData( FALSE );				// Refresh list box

		// Check special debug code to enable degug text on main dialog
		if ( (m_CalSetpointLow == 1234) && (m_CalSetpointHigh == 5678) ) {
			GetParent()->SendMessage ( theApp.m_MsgToMainWindow_DisplayDebugText );
		}
	} else {
		// Range error
		m_CalSetpointHigh = m_pParent->m_CalSetpointHigh;
		UpdateData( FALSE );
		CEdit* edit = (CEdit*)GetDlgItem(IDC_EDIT_HIGH_CAL);
		edit->SetFocus();
	}
}

void CCalibrateDlg::OnEnKillfocusEditUnitId()
{
	// Call update to test limits
	if ( UpdateData(TRUE) ) {
		// User entered a value in edit box that was excepted
	} else {
		// Range error
		m_UnitID = m_pParent->m_UnitID;
		UpdateData( FALSE );
		CEdit* edit = (CEdit*)GetDlgItem(IDC_EDIT_UNIT_ID);
		edit->SetFocus();
	}
}

void CCalibrateDlg::OnEnKillfocusEditSerNum()
{
	// Call update to test limits
	if ( UpdateData(TRUE) ) {
		// User entered a value in edit box that was excepted
	} else {
		// Range error
		m_SerialNumber = m_pParent->m_SerialNumber;
		UpdateData( FALSE );
		CEdit* edit = (CEdit*)GetDlgItem(IDC_EDIT_SER_NUM);
		edit->SetFocus();
	}
}

void CCalibrateDlg::OnBnClickedStartLowCal()
{
	if ( !m_CalLowBusy && !m_CalHighBusy ) {
		// Kick off read in timer
		m_CalValueLowSum = 0;
		m_CalLowBusy = TRUE;
		m_CalLowCount = CALIBRATE_NUM_SAMPLES;
		// Clear text
		CString CalLow;
		CalLow.Format(_T("Updating..."));
		m_CalLowValueStatic.SetWindowTextW( CalLow);
		m_CalLowValueStatic.SetTextColor( LIGHTRED );
		UpdateData( FALSE );
	}
}

void CCalibrateDlg::OnBnClickedStartHighCal()
{
	if ( !m_CalLowBusy && !m_CalHighBusy ) {
		// Kick off read in timer
		m_CalValueHighSum = 0;
		m_CalHighBusy = TRUE;
		m_CalHighCount = CALIBRATE_NUM_SAMPLES;
		// Clear text
		CString CalHigh;
		CalHigh.Format(_T("Updating..."));
		m_CalHighValueStatic.SetWindowTextW( CalHigh);
		m_CalHighValueStatic.SetTextColor( LIGHTRED );
		UpdateData( FALSE );
	}
}