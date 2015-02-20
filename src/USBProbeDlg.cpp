/////////////////////////////////////////////////////////////////////////////
// USBProbeDlg.cpp : implementation file
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Includes
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "USBProbe.h"
#include "USBProbeDlg.h"
#include "CalibrateDlg.h"
#include "Math.h"
#include <dbt.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


/////////////////////////////////////////////////////////////////////////////
// Library Dependencies
/////////////////////////////////////////////////////////////////////////////

#pragma comment (lib, "SLABHIDDevice.lib")

// Define USB out data codes
#define OUT_DATA_CODE_READ_VOLTAGE		0x81
#define OUT_DATA_CODE_CALIBRATE			0x82
#define OUT_DATA_CODE_GET_CAL_DATA		0x83
#define OUT_DATA_CODE_GET_SW_VERSION	0x84

// Define USB in data codes
#define IN_DATA_A2D_DATA				0x41
#define IN_DATA_CAL_DATA				0x42	
#define IN_DATA_SW_VERSION_DATA			0x43	


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About
/////////////////////////////////////////////////////////////////////////////

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	BYTE major;
	BYTE minor;
	BOOL release;

	// Display the SLABHIDDevice.dll version on the about dialog
	if (HidDevice_GetHidLibraryVersion(&major, &minor, &release) == HID_DEVICE_SUCCESS)
	{
		CString version;
		version.Format(_T("%u.%u%s"), major, minor, (release) ? _T("") : _T(" (Debug)"));
		SetDlgItemText(IDC_STATIC_LIBRARY_VERSION, version);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// CUSBProbeDlg dialog
/////////////////////////////////////////////////////////////////////////////

CUSBProbeDlg::CUSBProbeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CUSBProbeDlg::IDD, pParent)
	, m_CalSetpointLow( 0.00)
	, m_CalSetpointHigh( 0.00)
	, m_CalValueLow ( 0.00)
	, m_CalValueHigh( 0.00)
	, m_Reading (0.00)
	, m_RawVoltage ( 0.00)
	, m_CheckSum (0)
	, m_SerialNumber (0)
	, m_UnitID (0)
	, m_ConfigBits(0)
	, m_VoltageReadActive (0)
	, m_CalDataReadActive (0)
	, m_ProbeSWVersion (0)
	, m_pCalDlg (0)
{

	// Initialize cal check value and counters
	m_CalCheckSetpoint = 100;
	m_CalCheckBusy = FALSE;
	m_CalCheckCount = 0;
	m_CalCheckReadDone = TRUE;	// initialize to done to allow first read

	m_hIcon			= AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_hSmallIcon	= (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 16, 16, 0);
}

void CUSBProbeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_DEVICE_LIST, m_comboDeviceList);
	DDX_Control(pDX, IDC_RICHEDIT_RECEIVE, m_richReceive);
	DDX_Control(pDX, IDC_VOLTAGE_STATIC, m_ReadingStatic);
	DDX_Control(pDX, IDC_CAL_DATE_TIME_STATIC, m_LastCalibratedStatic);
	DDX_Control(pDX, IDC_UNIT_ID_STATIC, m_UnitIDStatic);
	DDX_Control(pDX, IDC_DEBUG_TEXT_STATIC, m_DebugTextStatic);
	DDX_Control(pDX, IDC_BUTTON_CLEAR, m_DebugTextClearButton);
	DDX_Control(pDX, IDC_CAL_CHECK_BUTTON, m_CalCheckButton);
	DDX_Control(pDX, IDC_CAL_SETUP_BUTTON, m_CalSetupButton);
	DDX_Control(pDX, IDC_BUTTON_READ_VOLTAGE, m_ReadProbeButton);
	DDX_Text(pDX, IDC_CAL_CHECK_SETPOINT, m_CalCheckSetpoint);
	DDV_MinMaxDouble(pDX, m_CalCheckSetpoint, 0.0, 99999.0);			// TODO:  Limits for molar
	DDX_Control(pDX, IDC_CAL_CHECK_SETPOINT, m_CalCheckSetpointEdit);
}

BEGIN_MESSAGE_MAP(CUSBProbeDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_DEVICECHANGE()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_CHECK_CONNECT, &CUSBProbeDlg::OnBnClickedCheckConnect)
	ON_CBN_DROPDOWN(IDC_COMBO_DEVICE_LIST, &CUSBProbeDlg::OnCbnDropdownComboDeviceList)
	ON_CBN_CLOSEUP(IDC_COMBO_DEVICE_LIST, &CUSBProbeDlg::OnCbnCloseupComboDeviceList)
	ON_BN_CLICKED(IDC_BUTTON_READ_VOLTAGE, &CUSBProbeDlg::OnBnClickedButtonReadVoltage)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR, &CUSBProbeDlg::OnBnClickedButtonClear)
	ON_BN_CLICKED(IDC_CAL_SETUP_BUTTON, &CUSBProbeDlg::OnBnClickedCalSetup)
	ON_BN_CLICKED(IDC_CAL_CHECK_BUTTON, &CUSBProbeDlg::OnBnClickedCalCheckButton)
	ON_EN_KILLFOCUS(IDC_CAL_CHECK_SETPOINT, &CUSBProbeDlg::OnEnKillfocusCalCheckSetpoint)
	// Handle user defined messages	
	ON_REGISTERED_MESSAGE( theApp.m_MsgToMainWindow_ReadVoltage, &CUSBProbeDlg::OnMsg_ReadVoltage )
	ON_REGISTERED_MESSAGE( theApp.m_MsgToMainWindow_SaveProbeData, &CUSBProbeDlg::OnMsg_SaveProbeData )
	ON_REGISTERED_MESSAGE( theApp.m_MsgToMainWindow_DisplayDebugText, &CUSBProbeDlg::OnMsg_DisplayDebugText )
	ON_REGISTERED_MESSAGE( theApp.m_MsgToMainWindow_CalCheck, &CUSBProbeDlg::OnMsg_CalCheck )
END_MESSAGE_MAP()


// Handle User Defined messages
afx_msg LRESULT CUSBProbeDlg::OnMsg_ReadVoltage( WPARAM wParam, LPARAM lParam ) 
{
	OnBnClickedButtonReadVoltage();   
	return 0L;
}

afx_msg LRESULT CUSBProbeDlg::OnMsg_DisplayDebugText( WPARAM wParam, LPARAM lParam ) 
{
	// Show the debug text window, header & clear button
	m_richReceive.ShowWindow( SW_SHOW );
	m_DebugTextClearButton.ShowWindow( SW_SHOW );
	m_DebugTextStatic.ShowWindow( SW_SHOW );
	return 0L;
}

afx_msg LRESULT CUSBProbeDlg::OnMsg_SaveProbeData( WPARAM wParam, LPARAM lParam ) 
{
	// Transfer from dialog all data and save to probe
	if ( m_pCalDlg ) {
		// Transfer all data from cal dialog
		m_CalSetpointLow = m_pCalDlg->m_CalSetpointLow;
		m_CalSetpointHigh = m_pCalDlg->m_CalSetpointHigh;
		m_CalValueLow = m_pCalDlg->m_CalValueLow;
		m_CalValueHigh = m_pCalDlg->m_CalValueHigh;
		m_UnitID = m_pCalDlg->m_UnitID;
		m_SerialNumber = m_pCalDlg->m_SerialNumber;
		// Write data to the probe
		WriteProbeCalData();
	}
	return 0L;
}

afx_msg LRESULT CUSBProbeDlg::OnMsg_CalCheck( WPARAM wParam, LPARAM lParam ) 
{
	CString Text;

	// Display cal check results
	// Calculate error from setpoint.
	double PercentError = ((m_CalCheckValue - m_CalCheckSetpoint) / m_CalCheckSetpoint) * 100.0;
	if ( abs(PercentError) > 999.0 ) PercentError = 999.0;

	if ( abs(PercentError) <= 4.0 ) {
		Text.Format(_T("Check Passed. Error= %3.1f%%  \nLimit= +/-4%%"), PercentError);
		AfxMessageBox(	Text
						,MB_ICONEXCLAMATION |MB_OK);

	} else {
		Text.Format(_T("Check Failed.  Error= %3.1f %%  \nError is greater than +/-4%%  \nCalibration Required!"), PercentError);
		AfxMessageBox(	Text
						,MB_ICONEXCLAMATION | MB_OK);
	}

	// Enable controls
	m_CalCheckButton.SetWindowText( _T("Calibrate Check") );
	m_CalCheckButton.EnableWindow( TRUE );
	// Disable other controls when checking calibration
	m_CalSetupButton.EnableWindow( TRUE );
	m_ReadProbeButton.EnableWindow( TRUE );

	return 0L;
}



/////////////////////////////////////////////////////////////////////////////
// CUSBProbeDlg message handlers
/////////////////////////////////////////////////////////////////////////////

BOOL CUSBProbeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hSmallIcon, FALSE);	// Set small icon

	m_Font.CreatePointFont(200, _T("Tahoma"), NULL);

	//Initialize Voltage Text
	m_ReadingStatic.SetFont( &m_Font );
	m_ReadingStatic.SetWindowTextW(_T("Probe Reading"));
	m_ReadingStatic.SetTextColor( BLACK );
	//m_ReadingStatic.SetBkColor( GRAY );


	InitializeDialog();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CUSBProbeDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CUSBProbeDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CUSBProbeDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// Handle device change messages (ie a device is added or removed)
// - If an HID device is connected, then add the device to the device list
// - If the device we were connected to was removed, then disconnect from the device
BOOL CUSBProbeDlg::OnDeviceChange(UINT nEventType, DWORD_PTR dwData)
{
	// Device has been added or removed
	if (nEventType == DBT_DEVICEREMOVECOMPLETE ||
		nEventType == DBT_DEVICEARRIVAL)
	{
		if (dwData)
		{
			PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)dwData;

			if (pHdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
			{
				PDEV_BROADCAST_DEVICEINTERFACE pDevInf = (PDEV_BROADCAST_DEVICEINTERFACE)pHdr;

				// "\\?\hid#vid_10c4&pid_81ba..."
				CString deviceStr = pDevInf->dbcc_name;

				// If the device was removed
				if (nEventType == DBT_DEVICEREMOVECOMPLETE)
				{
					char devPath[MAX_PATH_LENGTH];

					// Check if our device was removed by device path
					if (HidDevice_GetString(m_hid, HID_PATH_STRING, devPath, MAX_PATH_LENGTH) == HID_DEVICE_SUCCESS)
					{
						// Our device was removed
						if (deviceStr.CompareNoCase(CString(devPath)) == 0)
						{
							// The device handle is stale
							// Disconnect from it
							Disconnect();
						}
					}
				}

				UpdateDeviceList();
			}
		}
	}

	return TRUE;
}

// Update the device list when the combobox is dropped down
void CUSBProbeDlg::OnCbnDropdownComboDeviceList()
{
	// Automatically update the device list when the
	// list is opened/dropped down
	UpdateDeviceList();
}

// Update the device list if the selected device has been
// removed when the combobox is closed
void CUSBProbeDlg::OnCbnCloseupComboDeviceList()
{
	CString		path;
	DWORD		deviceNum;

	if (GetSelectedDevice(path))
	{
		// If the selected device has been removed
		if (!FindDevice(path, deviceNum))
		{
			// Then update the device list
			UpdateDeviceList();		
		}
	}
}

// Toggle connection state
void CUSBProbeDlg::OnBnClickedCheckConnect()
{
	// Disconnecting
	if (IsDlgButtonChecked(IDC_CHECK_CONNECT))
	{
		Disconnect();
	}
	// Connecting
	else
	{
		Connect();
	}
}

// Send USB command to read probe calibration data
void CUSBProbeDlg::GetProbeCalData()
{
	// Allow only one read at a time.
	if ( m_VoltageReadActive || m_CalDataReadActive ) return;

	// Make sure that we are connected to a device
	if (HidDevice_IsOpened(m_hid))
	{
		DWORD transmitBufferSize;
		// Transmits:
		// (0) = Report ID
		// [1] = Size
		// [2] = transmitBuffer[0] = Command code to read calibration data
		transmitBufferSize = 3;
		BYTE* transmitBuffer = new BYTE[transmitBufferSize];
		transmitBuffer[0] = OUT_DATA_CODE_GET_CAL_DATA;
		m_CalDataReadActive = 1;

		if (!TransmitData(transmitBuffer, transmitBufferSize))
		{
			MessageBox(_T("Failed to get data"), 0, MB_ICONWARNING);
		}
		delete [] transmitBuffer;

	}
	else
	{
		AfxMessageBox(IDS_STRING_NOT_CONNECTED, MB_ICONWARNING);
	}	
}

// Send USB command to read probe calibration data
void CUSBProbeDlg::GetProbeSWVersion()
{
	// Allow only one read at a time.
	if ( m_VoltageReadActive || m_CalDataReadActive ) return;

	// Make sure that we are connected to a device
	if (HidDevice_IsOpened(m_hid))
	{
		DWORD transmitBufferSize;
		// Transmits:
		// (0) = Report ID
		// [1] = Size
		// [2] = transmitBuffer[0] = Command code to read sw version
		transmitBufferSize = 3;
		BYTE* transmitBuffer = new BYTE[transmitBufferSize];
		transmitBuffer[0] = OUT_DATA_CODE_GET_SW_VERSION;

		if (!TransmitData(transmitBuffer, transmitBufferSize))
		{
			MessageBox(_T("Failed to get sw version"), 0, MB_ICONWARNING);
		}
		delete [] transmitBuffer;

	}	
}

// Send USB command to read probe voltage
void CUSBProbeDlg::OnBnClickedButtonReadVoltage()
{

	// Allow only one read at a time.
	if ( m_VoltageReadActive || m_CalDataReadActive ) return;

	// Make sure that we are connected to a device
	if (HidDevice_IsOpened(m_hid))
	{
		DWORD transmitBufferSize;
		// Transmits:
		// (0) = Report ID
		// [1] = Size
		// [2] = transmitBuffer[0] = Command Code
		transmitBufferSize = 3;
		BYTE* transmitBuffer = new BYTE[transmitBufferSize];
		transmitBuffer[0] = OUT_DATA_CODE_READ_VOLTAGE;
		m_VoltageReadActive = 1;

		if (!TransmitData(transmitBuffer, transmitBufferSize))
		{
			MessageBox(_T("Failed to transmit data"), 0, MB_ICONWARNING);
		}
		delete [] transmitBuffer;

	}
	else
	{
		AfxMessageBox(IDS_STRING_NOT_CONNECTED, MB_ICONWARNING);
	}	
}

void CUSBProbeDlg::OnBnClickedCalCheckButton()
{

	if ( !m_pCalDlg ) {		// Don't allow if cal/setup dialog is open
		// Kick off read in timer
		m_CalCheckSum = 0;
		m_CalCheckBusy = TRUE;
		m_CalCheckCount = CALIBRATE_NUM_SAMPLES;
		// Change button text while checking calibration
		m_CalCheckButton.SetWindowText( _T("Checking...") );
		m_CalCheckButton.EnableWindow( FALSE );
		// Disable other controls when checking calibration
		m_CalSetupButton.EnableWindow( FALSE );
		m_ReadProbeButton.EnableWindow( FALSE );
	}
}

// Send calibration data to probe
void CUSBProbeDlg::WriteProbeCalData()
{
	ShortByteUnion_type TempShort;

	// Make sure that we are connected to a device
	if (HidDevice_IsOpened(m_hid))
	{
		DWORD transmitBufferSize;
		DoubleByteUnion_type TempDouble;

		// Transmits:
		// Fifo[0]		= Report ID
		// Fifo[1]		= Size
		// Fifo[2]		= transmitBuffer[0]			= Command Code
		// Fifo[3][50]  = transmitBuffer[1]..[48]	= 48 data bytes (CAL_DATA_SIZE)

		// Cal Data layout:
		// CalData[0][1]	= CheckSum (16-bit)
		// CalData[2][3] 	= Serial Number (16-bit)
		// CalData[4][5] 	= Config Bits (16-bit)
		// CalData[6][7] 	= Unit ID (16-bit)
		// CalData[8]...[39] = Cal Data (4x8bytes)
		// CalData[40]..[47] = Cal Data/time (8bytes)
		#define CAL_DATA_SIZE		48	// Indexes 0 to 47

		// This is number of bytes to take from transmitBuffer[], which is the
		// Command Code byte plus the number of data bytes (CAL_DATA_SIZE)
		transmitBufferSize = CAL_DATA_SIZE+1;

		BYTE* transmitBuffer = new BYTE[transmitBufferSize];
		transmitBuffer[0] = OUT_DATA_CODE_CALIBRATE;	// Command Code

		transmitBuffer[1] = 0;							// Checksum - Fill in later!
		transmitBuffer[2] = 0;

		TempShort.shortvalue = m_SerialNumber;
		transmitBuffer[3] = TempShort.bvalue[1];		// Serial Number
		transmitBuffer[4] = TempShort.bvalue[0];

		TempShort.shortvalue = m_ConfigBits;
		transmitBuffer[5] = TempShort.bvalue[1];		// ConfigBits
		transmitBuffer[6] = TempShort.bvalue[0];

		TempShort.shortvalue = m_UnitID;
		transmitBuffer[7] = TempShort.bvalue[1];		// Unit ID
		transmitBuffer[8] = TempShort.bvalue[0];

		// Put calibration data in tx buffer
		TempDouble.fvalue  = m_CalSetpointLow;
		transmitBuffer[9]  = TempDouble.bvalue[0];
		transmitBuffer[10]  = TempDouble.bvalue[1];
		transmitBuffer[11]  = TempDouble.bvalue[2];
		transmitBuffer[12] = TempDouble.bvalue[3];
		transmitBuffer[13] = TempDouble.bvalue[4];
		transmitBuffer[14] = TempDouble.bvalue[5];
		transmitBuffer[15] = TempDouble.bvalue[6];
		transmitBuffer[16] = TempDouble.bvalue[7];

		TempDouble.fvalue = m_CalSetpointHigh;
		transmitBuffer[17] = TempDouble.bvalue[0];
		transmitBuffer[18] = TempDouble.bvalue[1];
		transmitBuffer[19] = TempDouble.bvalue[2];
		transmitBuffer[20] = TempDouble.bvalue[3];
		transmitBuffer[21] = TempDouble.bvalue[4];
		transmitBuffer[22] = TempDouble.bvalue[5];
		transmitBuffer[23] = TempDouble.bvalue[6];
		transmitBuffer[24] = TempDouble.bvalue[7];

		TempDouble.fvalue = m_CalValueLow;
		transmitBuffer[25] = TempDouble.bvalue[0];
		transmitBuffer[26] = TempDouble.bvalue[1];
		transmitBuffer[27] = TempDouble.bvalue[2];
		transmitBuffer[28] = TempDouble.bvalue[3];
		transmitBuffer[29] = TempDouble.bvalue[4];
		transmitBuffer[30] = TempDouble.bvalue[5];
		transmitBuffer[31] = TempDouble.bvalue[6];
		transmitBuffer[32] = TempDouble.bvalue[7];

		TempDouble.fvalue = m_CalValueHigh;
		transmitBuffer[33] = TempDouble.bvalue[0];
		transmitBuffer[34] = TempDouble.bvalue[1];
		transmitBuffer[35] = TempDouble.bvalue[2];
		transmitBuffer[36] = TempDouble.bvalue[3];
		transmitBuffer[37] = TempDouble.bvalue[4];
		transmitBuffer[38] = TempDouble.bvalue[5];
		transmitBuffer[39] = TempDouble.bvalue[6];
		transmitBuffer[40] = TempDouble.bvalue[7];

		// Put current date/time in transmit buffer
		CTime theTime = CTime::GetCurrentTime();	//1330206395
		Int64ByteUnion_type TimeUnion;
		TimeUnion.int64= theTime.GetTime();
		transmitBuffer[41] = TimeUnion.bvalue[0];
		transmitBuffer[42] = TimeUnion.bvalue[1];
		transmitBuffer[43] = TimeUnion.bvalue[2];
		transmitBuffer[44] = TimeUnion.bvalue[3];
		transmitBuffer[45] = TimeUnion.bvalue[4];
		transmitBuffer[46] = TimeUnion.bvalue[5];
		transmitBuffer[47] = TimeUnion.bvalue[6];
		transmitBuffer[48] = TimeUnion.bvalue[7];

		//Calculate and insert the checksum
		unsigned short Checksum = 0x00;
		for ( int i=3; i<49; i++ ) {	// indexes 3 through 48 (skip cs bytes)
			Checksum -= transmitBuffer[i];
		}
		TempShort.shortvalue = Checksum;
		transmitBuffer[1] = TempShort.bvalue[1];
		transmitBuffer[2] = TempShort.bvalue[0];

		if (TransmitData(transmitBuffer, transmitBufferSize))
		{
			// Update debug text field
			CString DebugText("Calibration data written");
			WriteDebugText( DebugText);
			// Update controls on this dialog
			CString strCal  = theTime.Format ("Calibrated: %c");	// Calibrated: 02/25/12 13:41:20
			m_LastCalibratedStatic.SetWindowText( strCal );
			// Update UnitID on this dialog
			CString UnitID;
			UnitID.Format(_T("Unit ID: %4i"), m_UnitID);
			m_UnitIDStatic.SetWindowTextW( UnitID);

		} else {
			MessageBox(_T("Failed to send calibrate command"), 0, MB_ICONWARNING);
		}
		delete [] transmitBuffer;

	}
	else
	{
		AfxMessageBox(IDS_STRING_NOT_CONNECTED, MB_ICONWARNING);
	}
}

// Clear the receive rich edit control
void CUSBProbeDlg::OnBnClickedButtonClear()
{
	m_richReceive.SetWindowText(_T(""));
}

// Handle timer messages
void CUSBProbeDlg::OnTimer(UINT_PTR nIDEvent)
{

	unsigned char i=0;
	double A2DAverage = 0;
	ShortByteUnion_type A2DNumReads;
	ShortByteUnion_type TempShort;
	LongByteUnion_type	A2DSum;
	DoubleByteUnion_type TempDouble;
	
	double CalDecades;
	double CalSlope;
	double ppm;

	// Periodically check for input reports over
	// the interrupt endpoint
	if (nIDEvent == TIMER_READ_ID)
	{
		// Make sure that we are connected to a device
		if (HidDevice_IsOpened(m_hid))
		{
			DWORD bufferSize	= HidDevice_GetMaxReportRequest(m_hid) * SIZE_MAX_READ;
			BYTE* buffer		= new BYTE[bufferSize];
			DWORD bytesRead;

			// Receive the max number of UART input data bytes
			if (ReceiveData(buffer, bufferSize, bytesRead))
			{
				if (bytesRead > 0)
				{
					//------------------------------------------------------
					// Get A2D Data
					//------------------------------------------------------
					if ( buffer[0] == IN_DATA_A2D_DATA )
					{
						// Sum & Number of reads
						A2DSum.bvalue[3] = buffer[1];
						A2DSum.bvalue[2] = buffer[2];
						A2DSum.bvalue[1] = buffer[3];
						A2DSum.bvalue[0] = buffer[4];
	
						A2DNumReads.bvalue[1] = buffer[5];
						A2DNumReads.bvalue[0] = buffer[6];
						
						// Calculate raw voltage
						A2DAverage = (A2DSum.longvalue + (A2DNumReads.shortvalue/2 )) / A2DNumReads.shortvalue;
						m_RawVoltage = A2DAverage / 32000;	//  /32768.0 * 1.024;
						m_RawVoltage *= -1.0;
	
						// Calculate ppm
						// Volts = Vo - M[log (ppm/Po)
						// Where:	Vo = low cal point voltage
						//			M  = volts/decade = (high cal volts - low cal volts)/(high cal ppm / low cal ppm)
						//			Po = low cal point ppm
						// Solve for ppm:
						// ppm = [Inverse Log[(Volts-Vo)/m]*Po
						CalDecades = log10(m_CalSetpointHigh/m_CalSetpointLow);		// # of decades
						CalSlope  = (m_CalValueHigh-m_CalValueLow)/ CalDecades;		// Slope
						ppm = m_RawVoltage - m_CalValueLow;							// Volts - Vo
						ppm = ppm  / CalSlope;										// (Volts - Vo)/ m
						m_Reading = pow(10,ppm);									// Inverse log
						m_Reading *= m_CalSetpointLow;								// x Po

						// Bounds check for ppm
						//TODO: will be different for molar
						CString Data;
						if ( m_Reading > 99999.0 ) {
							m_Reading = 99999.0;
							Data.Format(_T("Reading = Overrange"));
						} else if ( m_Reading < 0.0 ) {
							m_Reading = 0.0;
							Data.Format(_T("Reading = Underrange"));
						} else {
							// Update dialog static text
							Data.Format(_T("Reading = %3.1f ppm"), m_Reading);
						}

						// If cal check is active
						if ( m_CalCheckBusy ) {
							m_CalCheckSum += m_Reading;
							m_CalCheckCount--;
							if ( m_CalCheckCount == 0 ) {
								m_CalCheckValue = m_CalCheckSum / CALIBRATE_NUM_SAMPLES;
								m_CalCheckBusy = FALSE;
								// Send message to ourself to display results
								SendMessage( theApp.m_MsgToMainWindow_CalCheck, 0, 0 );
							}
							m_CalCheckReadDone = TRUE;
						} else {
							// Only updates reading text if not doing a calibration check
							m_ReadingStatic.SetWindowTextW( Data);
						}

						//Signal Calibration/Setup task that voltage changed
						if ( m_pCalDlg ) {
							m_pCalDlg->SendMessage( theApp.m_MsgToCalWindow_Volts, 0, 0 );
						}

						// Update debug text field with raw voltage
						CString DebugText;
						DebugText.Format( _T("Raw voltage %2.6f"), m_RawVoltage);
						WriteDebugText( DebugText);

						// Clear read flag
						m_VoltageReadActive = 0;

					}

					//------------------------------------------------------
					// Get calibration data
					//
					// Cal Data layout:
					// CalData[0][1]	= CheckSum (16-bit)
					// CalData[2][3] 	= Serial Number (16-bit)
					// CalData[4][5]	= Config bits (16-bits)
					// CalData[6][7] 	= Unit ID (16-bit)
					// CalData[8]...[39] = Cal Data (4x8bytes)
					// CalData[40]..[47] = Cal Data/time (8bytes)
					//------------------------------------------------------
					if ( buffer[0] == IN_DATA_CAL_DATA )
					{
						// Get Check Sum
						TempShort.bvalue[1] = buffer[1];
						TempShort.bvalue[0] = buffer[2];
						m_CheckSum = TempShort.shortvalue;

						//Calculate and compare the check sum.
						unsigned short Checksum = 0x00;
						for ( int i=3; i<49; i++ ) {	// indexes 3 through 48 (skip cs bytes)
							Checksum -= buffer[i];
						}
	
						if ( Checksum != m_CheckSum ) {
							// Checksum error.
							// Set default values
							m_SerialNumber = -1;   // Allows resetting
							m_ConfigBits = 0x00;
							m_UnitID = 1;
							m_CalSetpointLow = 10.0;
							m_CalSetpointHigh = 100.0;
							m_CalValueLow = -0.40;		// TODO:  Better default
							m_CalValueHigh = -0.80;		// TODO:  Better default
							m_TimeUnion.int64 = 0x00;	// TODO:  Better default
							// Display an error message
							AfxMessageBox(	_T("Probe data error!")
											_T("\n  ")
							                _T("\nCalibration and setup data stored in probe is corrupt.")
											_T("\nAll data was set to default values.")
											_T("\nYou must recalibrate and setup this probe.")
											_T("\n")
											_T("\nIf this problem persists the USB dongle hardware is faulty,")
											_T("\nand must be repaired or replaced.")
											,MB_ICONEXCLAMATION | MB_OK);
						} else {
							// Checksum is OK so get date from buffer
							// Get Serial Number
							TempShort.bvalue[1] = buffer[3];
							TempShort.bvalue[0] = buffer[4];
							m_SerialNumber = TempShort.shortvalue;

							// Get Config Bits
							TempShort.bvalue[1] = buffer[5];
							TempShort.bvalue[0] = buffer[6];
							m_ConfigBits = TempShort.shortvalue;

							// Get Unit ID
							TempShort.bvalue[1] = buffer[7];
							TempShort.bvalue[0] = buffer[8];
							m_UnitID = TempShort.shortvalue;

							// Get Cal setpoint low
							TempDouble.bvalue[0] = buffer[9];
							TempDouble.bvalue[1] = buffer[10];
							TempDouble.bvalue[2] = buffer[11];
							TempDouble.bvalue[3] = buffer[12];
							TempDouble.bvalue[4] = buffer[13];
							TempDouble.bvalue[5] = buffer[14];
							TempDouble.bvalue[6] = buffer[15];
							TempDouble.bvalue[7] = buffer[16];
							m_CalSetpointLow = TempDouble.fvalue;

							// Get Cal setpoint high
							TempDouble.bvalue[0] = buffer[17];
							TempDouble.bvalue[1] = buffer[18];
							TempDouble.bvalue[2] = buffer[19];
							TempDouble.bvalue[3] = buffer[20];
							TempDouble.bvalue[4] = buffer[21];
							TempDouble.bvalue[5] = buffer[22];
							TempDouble.bvalue[6] = buffer[23];
							TempDouble.bvalue[7] = buffer[24];
							m_CalSetpointHigh = TempDouble.fvalue;

							// Get Cal value low
							TempDouble.bvalue[0] = buffer[25];
							TempDouble.bvalue[1] = buffer[26];
							TempDouble.bvalue[2] = buffer[27];
							TempDouble.bvalue[3] = buffer[28];
							TempDouble.bvalue[4] = buffer[29];
							TempDouble.bvalue[5] = buffer[30];
							TempDouble.bvalue[6] = buffer[31];
							TempDouble.bvalue[7] = buffer[32];
							m_CalValueLow = TempDouble.fvalue;

							// Get Cal value high
							TempDouble.bvalue[0] = buffer[33];
							TempDouble.bvalue[1] = buffer[34];
							TempDouble.bvalue[2] = buffer[35];
							TempDouble.bvalue[3] = buffer[36];
							TempDouble.bvalue[4] = buffer[37];
							TempDouble.bvalue[5] = buffer[38];
							TempDouble.bvalue[6] = buffer[39];
							TempDouble.bvalue[7] = buffer[40];
							m_CalValueHigh = TempDouble.fvalue;

							// Get calibration data and time
							//	Int64ByteUnion_type TimeUnion;
							m_TimeUnion.bvalue[0] = buffer[41];
							m_TimeUnion.bvalue[1] = buffer[42];
							m_TimeUnion.bvalue[2] = buffer[43];
							m_TimeUnion.bvalue[3] = buffer[44];
							m_TimeUnion.bvalue[4] = buffer[45];
							m_TimeUnion.bvalue[5] = buffer[46];
							m_TimeUnion.bvalue[6] = buffer[47];
							m_TimeUnion.bvalue[7] = buffer[48];
						}

						// Update debug text field
						CString DebugText("Calibration data read");
						WriteDebugText( DebugText);

						// Print data/time in debug text field
						CTime theTime( m_TimeUnion.int64 );
						CString strCal  = theTime.Format ("Calibrated: %c");	// Calibrated: 02/25/12 13:41:20
						WriteDebugText( strCal);
	
						// Update date/time on this dialog
						m_LastCalibratedStatic.SetWindowText( strCal );

						// Update UnitID on this dialog
						CString UnitID;
						UnitID.Format(_T("Unit ID: %4i"), m_UnitID);
						m_UnitIDStatic.SetWindowTextW( UnitID);

						// Clear read flag
						m_CalDataReadActive = 0;
						UpdateData(FALSE);
					}

					//------------------------------------------------------
					// Get the probes software version
					//------------------------------------------------------
					if ( buffer[0] == IN_DATA_SW_VERSION_DATA )
					{
						FloatByteUnion_type TempFloat;
						TempFloat.bvalue[0] = buffer[4];
						TempFloat.bvalue[1] = buffer[3];
						TempFloat.bvalue[2] = buffer[2];
						TempFloat.bvalue[3] = buffer[1];
						m_ProbeSWVersion = TempFloat.fvalue;
						//Signal Calibration/Setup task that sw changed
						if ( m_pCalDlg ) {
							m_pCalDlg->SendMessage( theApp.m_MsgToCalWindow_SWVersion, 0, 0 );
						}
					}
				}
			}

			delete [] buffer;
		}

		// Process Cal Check
		if ( m_CalCheckCount && m_CalCheckReadDone) {
			// Read probe
			m_CalCheckReadDone = FALSE;
			SendMessage ( theApp.m_MsgToMainWindow_ReadVoltage );
		}
	}

	CDialog::OnTimer(nIDEvent);
}

void CUSBProbeDlg::OnDestroy()
{
	CDialog::OnDestroy();

	// Close the device
	if (HidDevice_IsOpened(m_hid))
	{
		// Disable the read timer before we disconnect
		// from the device
		StopReadTimer();

		HidDevice_Close(m_hid);
		m_hid = NULL;
	}

	// Disable WM_DEVICECHANGE messages
	UnregisterDeviceChange();
}

/////////////////////////////////////////////////////////////////////////////
// CUSBProbeDlg Class - Protected Methods
/////////////////////////////////////////////////////////////////////////////

void CUSBProbeDlg::InitializeDialog()
{
	// Initially device controls are disabled
	// until connected to a device
	EnableDeviceCtrls(FALSE);

	// Populate the device combobox list
	UpdateDeviceList();

	// Register for WM_DEVICECHANGE messages
	// whenever an HID device is attached or removed
	RegisterDeviceChange();
}

// Register for device change notification for USB HID devices
// OnDeviceChange() will handle device arrival and removal
void CUSBProbeDlg::RegisterDeviceChange()
{
	DEV_BROADCAST_DEVICEINTERFACE devIF = {0};

	devIF.dbcc_size			= sizeof(devIF);    
	devIF.dbcc_devicetype	= DBT_DEVTYP_DEVICEINTERFACE;    
	
	HidDevice_GetHidGuid(&devIF.dbcc_classguid);
	
	m_hNotifyDevNode = RegisterDeviceNotification(GetSafeHwnd(), &devIF, DEVICE_NOTIFY_WINDOW_HANDLE);
}

// Unregister for device change notification for USB HID devices
void CUSBProbeDlg::UnregisterDeviceChange()
{
	if (m_hNotifyDevNode)
	{
		UnregisterDeviceNotification(m_hNotifyDevNode);
		m_hNotifyDevNode = NULL;
	}
}

// Connect to the device with the path string selected
// in the device list
// - Connect to the device specified in the device list
// - Set Connect checkbox/button caption and pressed state
// - Enable/disable device combobox
BOOL CUSBProbeDlg::Connect()
{
	BOOL		connected = FALSE;
	CString		path;
	DWORD		deviceNum;

	// Get selected device path string
	if (GetSelectedDevice(path))
	{
		// Find the selected device number
		if (FindDevice(path, deviceNum))
		{
			BYTE status = HidDevice_Open(&m_hid, deviceNum, VID, PID, MAX_REPORT_REQUEST_XP);

			// Attempt to open the device
			if (status == HID_DEVICE_SUCCESS)
			{
				connected = TRUE;
			}
			else
			{
				CString msg;
				msg.Format(_T("Failed to connect to %s."), path);
				MessageBox(msg, 0, MB_ICONWARNING);
			}
		}
	}

	// Connected
	if (connected)
	{
		// Set read/write timeouts
		// Read timeouts should be set very low since we are periodically
		// reading for input reports over the interrupt endpoint in the
		// selector timer
		HidDevice_SetTimeouts(m_hid, HID_READ_TIMEOUT, HID_WRITE_TIMEOUT);

		// Check/press the connect button
		CheckDlgButton(IDC_CHECK_CONNECT, TRUE);

		// Update Connect/Disconnect button caption
		SetDlgItemText(IDC_CHECK_CONNECT, _T("&Disconnect"));

		// Disable the device combobox
		m_comboDeviceList.EnableWindow(FALSE);

		// Enable the device controls
		EnableDeviceCtrls(TRUE);

		// Enable the read timer to check for input reports
		// over the interrupt endpoint
		StartReadTimer();

		// On startup trigger read of the calibration data
		GetProbeCalData();
	}
	// Disconnected
	else
	{
		// Uncheck/unpress the connect button
		CheckDlgButton(IDC_CHECK_CONNECT, FALSE);

		// Update Connect/Disconnect button caption
		SetDlgItemText(IDC_CHECK_CONNECT, _T("&Connect"));

		// Enable the device combobox
		m_comboDeviceList.EnableWindow(TRUE);

		// Disable the device controls
		EnableDeviceCtrls(FALSE);
	}

	return connected;
}

// Disconnect from the currently connected device
// - Disconnect from the current device
// - Output any error messages
// - Set Connect checkbox/button caption and pressed state
// - Enable/disable device combobox
BOOL CUSBProbeDlg::Disconnect()
{
	BOOL disconnected = FALSE;

	// Disable the read timer before we disconnect
	// from the device
	StopReadTimer();

	// Disconnect from the current device
	BYTE status = HidDevice_Close(m_hid);
	m_hid = NULL;

	// Output an error message if the close failed
	if (status != HID_DEVICE_SUCCESS)
	{
		CString msg;
		msg.Format(_T("Failed to disconnect."));
		MessageBox(msg, 0, MB_ICONWARNING);
	}
	else
	{
		disconnected = TRUE;
	}

	// Uncheck/unpress the connect button
	CheckDlgButton(IDC_CHECK_CONNECT, FALSE);

	// Update Connect/Disconnect button caption
	SetDlgItemText(IDC_CHECK_CONNECT, _T("&Connect"));

	// Enable the device combobox
	m_comboDeviceList.EnableWindow(TRUE);

	// Disable the device controls
	EnableDeviceCtrls(FALSE);

	return disconnected;
}

// Get the combobox device selection
// If a device is selected, return TRUE and return the path string
// Otherwise, return FALSE
BOOL CUSBProbeDlg::GetSelectedDevice(CString& path)
{
	BOOL selected = FALSE;

	int			sel;
	CString		selText;

	// Get current selection index or CB_ERR(-1)
	// if no device is selected
	sel = m_comboDeviceList.GetCurSel();

	if (sel != CB_ERR)
	{
		// Get the selected device string
		m_comboDeviceList.GetLBText(sel, selText);
		selected	= TRUE;
		path		= selText;
	}

	return selected;
}

// Search for an HID device with a matching device path string
// If the device was found return TRUE and return the device number
// in deviceNumber
// Otherwise return FALSE
BOOL CUSBProbeDlg::FindDevice(CString path, DWORD& deviceNum)
{
	BOOL found = FALSE;
	char deviceString[MAX_PATH_LENGTH];

	// Iterate through all connected HID devices
	for (DWORD i = 0; i < HidDevice_GetNumHidDevices(VID, PID); i++)
	{
		// Get the device path string
		if (HidDevice_GetHidString(i, VID, PID, HID_PATH_STRING, deviceString, MAX_PATH_LENGTH) == HID_DEVICE_SUCCESS)
		{
			if (path.CompareNoCase(CString(deviceString)) == 0)
			{
				found		= TRUE;
				deviceNum	= i;
				break;
			}
		}
	}

	return found;
}

// Populate the device list combo box with connected device path strings
// - Save previous device path string selection
// - Fill the device list with connected device path strings
// - Restore previous device selection
void CUSBProbeDlg::UpdateDeviceList()
{
	// Only update the combo list when the drop down list is closed
	if (!m_comboDeviceList.GetDroppedState())
	{
		int			sel;
		CString		path;
		char		deviceString[MAX_PATH_LENGTH];

		// Get previous combobox string selection
		GetSelectedDevice(path);

		// Remove all strings from the combobox
		m_comboDeviceList.ResetContent();

		// Iterate through each HID device with matching VID/PID
		for (DWORD i = 0; i < HidDevice_GetNumHidDevices(VID, PID); i++)
		{
			// Add path strings to the combobox
			if (HidDevice_GetHidString(i, VID, PID, HID_PATH_STRING, deviceString, MAX_PATH_LENGTH) == HID_DEVICE_SUCCESS)
			{
				m_comboDeviceList.AddString(CString(deviceString));
			}
		}

		sel = m_comboDeviceList.FindStringExact(-1, path);

		// Select first combobox string
		if (sel == CB_ERR)
		{
			m_comboDeviceList.SetCurSel(0);
		}
		// Restore previous combobox selection
		else
		{
			m_comboDeviceList.SetCurSel(sel);
		}
	}
}

// Enable/disable controls that should only be enabled
// when connected to a device
void CUSBProbeDlg::EnableDeviceCtrls(BOOL bEnable)
{
	int nIDs[] = 
	{
		IDC_BUTTON_READ_VOLTAGE,
		IDC_VOLTAGE_STATIC,
		IDC_CAL_SETUP_BUTTON,
		IDC_CAL_CHECK_BUTTON,
		IDC_UNIT_ID_STATIC,
		IDC_CAL_DATE_TIME_STATIC
	};

	for (int i = 0; i < sizeof(nIDs) / sizeof(nIDs[0]); i++)
	{
		GetDlgItem(nIDs[i])->EnableWindow(bEnable);
	}
}

// Start a timer to periodically check for input reports
// over the interrupt endpoint
void CUSBProbeDlg::StartReadTimer()
{
	SetTimer(TIMER_READ_ID, TIMER_READ_ELAPSE, NULL);
}

// Stop the read timer
void CUSBProbeDlg::StopReadTimer()
{
	KillTimer(TIMER_READ_ID);
}

void CUSBProbeDlg::WriteDebugText( CString InString ) {
	static unsigned char DebugCounter = 0;
	CString DebugString;
	DebugString.Format( _T("%2d: "), DebugCounter);
	DebugString += InString;
	DebugString += '\r';
	DebugString += '\n';
	AppendReceiveText(DebugString);
	if (DebugCounter++ >= 10) DebugCounter = 0;
}


// Append text to the end of the receive rich edit control
void CUSBProbeDlg::AppendReceiveText(const CString& text)
{
	long len = m_richReceive.GetTextLength();

	m_richReceive.SetSel(len, len);
	m_richReceive.ReplaceSel(text);
}


// Fragment and transmit data by sending output reports over
// the interrupt endpoint
BOOL CUSBProbeDlg::TransmitData(const BYTE* buffer, DWORD bufferSize)
{
	BOOL	success		= FALSE;
	WORD	reportSize	= HidDevice_GetOutputReportBufferLength(m_hid);
	BYTE*	report		= new BYTE[reportSize];

	// Make sure that the device report size is adequate
	if (reportSize >= SIZE_OUT_DATA)
	{
		DWORD bytesWritten = 0;
		DWORD bytesToWrite = bufferSize;

		// Fragment the buffer into several writes of up to SIZE_MAX_WRITE(61)
		// bytes
		while (bytesWritten < bytesToWrite)
		{
			DWORD transferSize = min(bytesToWrite - bytesWritten, SIZE_MAX_WRITE);

			report[0] = ID_OUT_DATA;
			report[1] = (BYTE)transferSize;
			memcpy(&report[2], &buffer[bytesWritten], transferSize);

			// Send an output report over the interrupt endpoint
			if (HidDevice_SetOutputReport_Interrupt(m_hid, report, reportSize) != HID_DEVICE_SUCCESS)
			{
				// Stop transmitting if there was an error
				break;
			}

			bytesWritten += transferSize;
		}

		// Write completed successfully
		if (bytesWritten == bytesToWrite)
		{
			success = TRUE;
		}
	}

	delete [] report;

	return success;
}

// Receive several data input reports over the interrupt endpoint
BOOL CUSBProbeDlg::ReceiveData(BYTE* buffer, DWORD bufferSize, DWORD& bytesRead)
{
	BOOL success		= FALSE;
	WORD reportSize		= HidDevice_GetOutputReportBufferLength(m_hid);

	// Make sure that the device report size is adequate
	if (reportSize >= SIZE_IN_DATA)
	{
		// Make sure that the buffer is at least big enough to hold the maximum
		// number of input data bytes from a single report
		if (bufferSize >= SIZE_MAX_READ)
		{
			// Determine the worst-case number of reports that will fit in the
			// user buffer
			DWORD	numReports			= bufferSize / SIZE_MAX_READ;
			DWORD	reportBufferSize	= numReports * reportSize;
			BYTE*	reportBuffer		= new BYTE[reportBufferSize];
			DWORD	reportBufferRead	= 0;
			BYTE	status;

			// Receive as many input reports as possible
			// (resulting data bytes must be able to fit in the user buffer)
			status = HidDevice_GetInputReport_Interrupt(m_hid, reportBuffer, reportBufferSize, numReports, &reportBufferRead);

			// Success indicates that numReports were read
			// Transfer timeout may have returned less data
			if (status == HID_DEVICE_SUCCESS ||
				status == HID_DEVICE_TRANSFER_TIMEOUT)
			{
				bytesRead = 0;

				// Iterate through each report in the report buffer
				for (DWORD i = 0; i < reportBufferRead; i += reportSize)
				{
					// Determine the number of valid data bytes in the current report
					BYTE bytesInReport = reportBuffer[i + 1];

					// Copy the data bytes into the user buffer
					memcpy(&buffer[bytesRead], &reportBuffer[i + 2], bytesInReport);

					// Keep track of how many valid bytes are being returned in the user buffer
					bytesRead += bytesInReport;
				}

				success = TRUE;
			}

			delete [] reportBuffer;
		}
	}

	return success;
}

void CUSBProbeDlg::OnBnClickedCalSetup()
{
	if( m_pCalDlg) {
		m_pCalDlg->SetForegroundWindow();
	} else {
		// First get send probe command to read the software version.
		GetProbeSWVersion();
		// Create modeless calibrate dialog
		m_pCalDlg = new CCalibrateDlg(this);
		m_pCalDlg->Create( CCalibrateDlg::IDD);
		m_pCalDlg->ShowWindow(SW_SHOW);
	}
}

void CUSBProbeDlg::OnEnKillfocusCalCheckSetpoint()
{
	// Call update to test limits
	if ( UpdateData(TRUE) ) {
		// User entered a value in edit box that was excepted
	} else {
		// Range error
		CEdit* edit = (CEdit*)GetDlgItem(IDC_CAL_CHECK_SETPOINT);
		edit->SetFocus();
	}
}


// For testing
#if 0
BOOL CKVisionTestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_Font.CreatePointFont(120, _T("Tahoma"), NULL);

	//Initialize Option board 1 button and static
	m_OptionBoard1Button.SetFont( &m_Font );
	m_OptionBd1StateStatic.SetFont( &m_Font );
	m_OptionBd1StateStatic.SetTextColor( BLACK );
	m_OptionBd1StateStatic.SetBkColor( GRAY );
	m_OptionBd1StateStatic.SetWindowTextW( _T("Off"));

	//Initialize Option board 2 button and static
	m_OptionBoard2Button.SetFont( &m_Font );
	m_OptionBd2StateStatic.SetFont( &m_Font );
	m_OptionBd2StateStatic.SetTextColor( BLACK );
	m_OptionBd2StateStatic.SetBkColor( GRAY );
	m_OptionBd2StateStatic.SetWindowTextW(_T("Off"));
	
	//Initialize COM1 COM2 button and state
	m_Com1Com2StateStatic.SetFont( &m_Font );
	m_Com1Com2StateStatic.SetTextColor( BLACK );
	m_Com1Com2StateStatic.SetBkColor( GRAY );
	m_Com1Com2StateStatic.SetWindowTextW(_T("Testing"));
	m_Com1Com2Static.SetFont( &m_Font );

	//Initialize RS485 button and static 
	m_RS485StateStatic.SetFont( &m_Font );
	m_RS485StateStatic.SetTextColor( BLACK );
	m_RS485StateStatic.SetBkColor( GRAY );
	m_RS485StateStatic.SetWindowTextW(_T("Testing"));
	m_RS485Static.SetFont( &m_Font );

	//Temperature data and static
	m_TemperatureStateStatic.SetFont( &m_Font );
	m_TemperatureStateStatic.SetWindowTextW(_T("Wait"));
	m_TemperatureStatic.SetFont( &m_Font );

	//Initialize Fan button and static 
	m_FanButton.SetFont( &m_Font );
	if ( m_FanState == true ) m_FanStateStatic.SetWindowTextW(_T("Fan On"));
	else                      m_FanStateStatic.SetWindowTextW(_T("Fan Off"));
	m_FanStateStatic.SetFont( &m_Font );

	//Touch screen test button and static
	m_TouchTestButton.SetFont( &m_Font );
	m_TouchTestStateStatic.SetFont( &m_Font );
	m_TouchTestStateStatic.SetTextColor( BLACK );
	m_TouchTestStateStatic.SetBkColor( GRAY );
	m_TouchTestStateStatic.SetWindowTextW(_T("Off"));

	//Backlight static and button
	m_BacklightStatic.SetFont( &m_Font );
	m_RadioButton5.SetCheck( true);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
void CKVisionTestDlg::OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/)
{
	DRA::RelayoutDialog(
		AfxGetInstanceHandle(), 
		this->m_hWnd, 
		DRA::GetDisplayMode() != DRA::Portrait ? 
			MAKEINTRESOURCE(IDD_KVISIONTEST_DIALOG_WIDE) : 
			MAKEINTRESOURCE(IDD_KVISIONTEST_DIALOG));
}
#endif


void CKVisionTestDlg::OnBnClickedOptionboard1button()
{
	m_OptionBd1Test = true;
}

void CKVisionTestDlg::OnBnClickedOptionboard2button()
{
	m_OptionBd2Test = true;
}

void CKVisionTestDlg::OnBnClickedFanbutton()
{
	m_FanState = !m_FanState;
	if ( m_FanState == true ) m_FanStateStatic.SetWindowTextW(_T("Fan On"));
	else                      m_FanStateStatic.SetWindowTextW(_T("Fan Off"));
	//Invalidate();
}

void CKVisionTestDlg::OnBnClickedTouchTestbutton()
{
	CKVisionTouchTestDlg dlg;
	INT_PTR nResponse = dlg.DoModal();

	if (nResponse == IDOK)	{
		m_TouchTestStateStatic.SetBkColor( LIGHTGREEN );
		m_TouchTestStateStatic.SetWindowTextW(_T("Pass"));
	}
	if (nResponse == IDCANCEL)	{
		m_TouchTestStateStatic.SetBkColor( LIGHTRED );
		m_TouchTestStateStatic.SetWindowTextW(_T("Fail"));
	}
}

#endif




