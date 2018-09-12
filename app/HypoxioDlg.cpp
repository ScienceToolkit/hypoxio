
// HypoxioDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Hypoxio.h"
#include "HypoxioDlg.h"
#include "afxdialogex.h"
#include "DlgSplitterComSettings.h"
#include <regex>
#include <hash_map>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
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
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CHypoxioDlg dialog




CHypoxioDlg::CHypoxioDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CHypoxioDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_iSpeedPct = 0;
	m_iCycles = 0;
	m_iDurationHours = 0;
	m_iDurationMinutes = 0;
	m_iDurationSeconds = 0;

	m_bHardwarePresent = false;
	m_iAmountPct = 0;

	m_hComm = NULL;
	m_bSplitterPresent = false;
	m_iRockStart = 0;
	m_iRockStop = 0;
	m_bRockingOn = false;
	m_iTableTiltCount = 0;

	m_pThread = NULL;
	m_pThreadUSBReceive = NULL;
}

void CHypoxioDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//  DDX_Text(pDX, TXT_CYCLES, m_iSpeedPct);
	DDX_Text(pDX, TXT_CYCLES, m_iCycles);
	DDX_Text(pDX, TXT_SPEEDPCT, m_iSpeedPct);
	DDX_Text(pDX, TXT_DURATION_HOURS, m_iDurationHours);
	DDX_Text(pDX, TXT_DURATION_MINUTES, m_iDurationMinutes);
	DDX_Text(pDX, TXT_DURATION_SECONDS, m_iDurationSeconds);
	DDX_Control(pDX, PCT_DURATION, m_pctDurationRemaining);
	DDX_Text(pDX, TXT_AMOUNTPCT, m_iAmountPct);
	DDX_Text(pDX, TXT_STARTROCK, m_iRockStart);
	DDX_Text(pDX, TXT_STOPROCK, m_iRockStop);
}

BEGIN_MESSAGE_MAP(CHypoxioDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(BTN_START, &CHypoxioDlg::OnBnClickedStart)
	ON_BN_CLICKED(BTN_STOP, &CHypoxioDlg::OnBnClickedStop)
	ON_WM_TIMER()
	ON_BN_CLICKED(BTN_TESTSETTINGS, &CHypoxioDlg::OnBnClickedTestsettings)
	ON_EN_CHANGE(TXT_SPEEDPCT, &CHypoxioDlg::OnEnChangeSpeedpct)
	ON_EN_CHANGE(TXT_CYCLES, &CHypoxioDlg::OnEnChangeCycles)
	ON_EN_CHANGE(TXT_AMOUNTPCT, &CHypoxioDlg::OnEnChangeAmountpct)
	ON_EN_CHANGE(TXT_DURATION_HOURS, &CHypoxioDlg::OnEnChangeDurationHours)
	ON_EN_CHANGE(TXT_DURATION_MINUTES, &CHypoxioDlg::OnEnChangeDurationMinutes)
	ON_EN_CHANGE(TXT_DURATION_SECONDS, &CHypoxioDlg::OnEnChangeDurationSeconds)
	ON_BN_CLICKED(BTN_SPLITTERCOMSETTINGS, &CHypoxioDlg::OnBnClickedSplittercomsettings)
	ON_CONTROL_RANGE(BN_CLICKED, CHK_A1, CHK_D6, OnTrayWellClicked)
	ON_EN_CHANGE(TXT_STARTROCK, &CHypoxioDlg::OnEnChangeStartrock)
	ON_EN_CHANGE(TXT_STOPROCK, &CHypoxioDlg::OnEnChangeStoprock)
	ON_BN_CLICKED(BTN_TRAYSELECTALL, &CHypoxioDlg::OnBnClickedTrayselectall)
	ON_BN_CLICKED(BTN_TRAYSELECTNONE, &CHypoxioDlg::OnBnClickedTrayselectnone)
	ON_BN_CLICKED(BTN_ROCKING_STOP, &CHypoxioDlg::OnBnClickedRockingStop)
END_MESSAGE_MAP()


// CHypoxioDlg message handlers

BOOL CHypoxioDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	// Get the path of the user's AppData locations (added for Windows 7 support - not tested under WinXP)
	char strPath[_MAX_PATH];
	CString txtAppData, txtLogData;
	SHGetSpecialFolderPath(0, strPath, CSIDL_LOCAL_APPDATA, FALSE);
	txtAppData.Format("%s", strPath);
	m_txtPathConfigFiles = txtAppData+"\\"+INI_COMPANYNAME+"\\"+INI_APPNAME+"\\";

	// Make the tray data array the proper size
	m_TrayData.resize(NUM_CELLS);

	// Load configuration or create if doesn't exist
	CString txtKey;
	CString txtConfigFile(m_txtPathConfigFiles+FILENAME_CONFIGURATION);
	if (!FileExists(txtConfigFile))
	{
		// Create a default settings file
		WritePrivateProfileString(INI_APPNAME, INI_TILTSPEED, "85", txtConfigFile);
		WritePrivateProfileString(INI_APPNAME, INI_TILTCYCLES, "1", txtConfigFile);
		WritePrivateProfileString(INI_APPNAME, INI_TILTAMOUNT, "50", txtConfigFile);
		WritePrivateProfileString(INI_APPNAME, INI_DURATION_HOURS, "0", txtConfigFile);
		WritePrivateProfileString(INI_APPNAME, INI_DURATION_MINUTES, "10", txtConfigFile);
		WritePrivateProfileString(INI_APPNAME, INI_DURATION_SECONDS, "0", txtConfigFile);
		WritePrivateProfileString(INI_APPNAME, INI_ROCK_START, "30000", txtConfigFile);
		WritePrivateProfileString(INI_APPNAME, INI_ROCK_STOP, "25000", txtConfigFile);
		for (char cCellCol='A'; cCellCol<='D'; cCellCol++)
		{
			for (int iCellRow=1; iCellRow<=6; iCellRow++)
			{
				txtKey.Format(INI_CELL_ACTIVE, cCellCol, iCellRow);
				WritePrivateProfileString(INI_APPNAME, txtKey, "1", txtConfigFile);
			}
		}
		WritePrivateProfileString(INI_APPNAME, INI_SPLITTERCOM_COMPORT, "1", txtConfigFile);
		WritePrivateProfileString(INI_APPNAME, INI_SPLITTERCOM_BAUDRATE, "38400", txtConfigFile);
		WritePrivateProfileString(INI_APPNAME, INI_SPLITTERCOM_PARITY, "N", txtConfigFile);
		WritePrivateProfileString(INI_APPNAME, INI_SPLITTERCOM_DATABITS, "8", txtConfigFile);
		WritePrivateProfileString(INI_APPNAME, INI_SPLITTERCOM_STOPBITS, "1", txtConfigFile);
	}
	// Load settings
	m_iSpeedPct = GetPrivateProfileInt(INI_APPNAME, INI_TILTSPEED, 0, txtConfigFile);
	m_iCycles = GetPrivateProfileInt(INI_APPNAME, INI_TILTCYCLES, 0, txtConfigFile);
	m_iAmountPct = GetPrivateProfileInt(INI_APPNAME, INI_TILTAMOUNT, 0, txtConfigFile);
	m_iDurationHours = GetPrivateProfileInt(INI_APPNAME, INI_DURATION_HOURS, 0, txtConfigFile);
	m_iDurationMinutes = GetPrivateProfileInt(INI_APPNAME, INI_DURATION_MINUTES, 0, txtConfigFile);
	m_iDurationSeconds = GetPrivateProfileInt(INI_APPNAME, INI_DURATION_SECONDS, 0, txtConfigFile);
	m_iRockStart = GetPrivateProfileInt(INI_APPNAME, INI_ROCK_START, 0, txtConfigFile);
	m_iRockStop = GetPrivateProfileInt(INI_APPNAME, INI_ROCK_STOP, 0, txtConfigFile);
	for (char cCellCol='A'; cCellCol<='D'; cCellCol++)
	{
		for (int iCellRow=1; iCellRow<=6; iCellRow++)
		{
			txtKey.Format(INI_CELL_ACTIVE, cCellCol, iCellRow);
			int i = (((cCellCol-'A')*6)+iCellRow)-1;
			m_BCellActive[i] = GetPrivateProfileInt(INI_APPNAME, txtKey, 0, txtConfigFile);
		}
	}
	TrayViewUpdate();
	char szBuf[16];
	m_tSplitterComSettings.iComPort = GetPrivateProfileInt(INI_APPNAME, INI_SPLITTERCOM_COMPORT, 1, txtConfigFile);
	m_tSplitterComSettings.iBaudRate = GetPrivateProfileInt(INI_APPNAME, INI_SPLITTERCOM_BAUDRATE, 9600, txtConfigFile);
	GetPrivateProfileString(INI_APPNAME, INI_SPLITTERCOM_PARITY, "N", szBuf, 16, txtConfigFile);
	m_tSplitterComSettings.cParity = szBuf[0];
	m_tSplitterComSettings.iDataBits = GetPrivateProfileInt(INI_APPNAME, INI_SPLITTERCOM_DATABITS, 8, txtConfigFile);
	m_tSplitterComSettings.iStopBits = GetPrivateProfileInt(INI_APPNAME, INI_SPLITTERCOM_STOPBITS, 1, txtConfigFile);
		
	EnableInputControls();

	m_pctDurationRemaining.SetMin(0);
	m_pctDurationRemaining.SetMax(100);
	m_pctDurationRemaining.SetPortionPercent(0);
	m_pctDurationRemaining.SetDigitalFormat("0 hrs 0 mins 0 secs");

	TableTiltCountUpdate(0);

	EnableStartButton();

	// Load the USB hardware
	m_bHardwarePresent = LoadUSBHardware();
	m_bSplitterPresent = SplitterConnect();
	if (m_bHardwarePresent == true && m_bSplitterPresent == true)
	{
		// The USB hardware as well as the com splitter are present
		TiltReset();
		EnableInputControls();
		EnableStartButton();
	}
	else
	{
		// There is no hardware connected or the com splitter isn't connected
		DisableInputControls();
		DisableButtons();
	}

	UpdateData(FALSE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CHypoxioDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CHypoxioDlg::OnPaint()
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
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CHypoxioDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

bool CHypoxioDlg::LoadUSBHardware(void)
{
	CString szHardwareName,txt;
	FT_STATUS status;

	szHardwareName.LoadString(STR_USB_HARDWAREIDENTIFIER);		// Load the name of the USB device
	
	// Try to open the device - will fail if device is not connected
	status = FT_OpenEx((PVOID)(LPCTSTR)szHardwareName, FT_OPEN_BY_DESCRIPTION, &m_ftHandle);
	if (status != FT_OK)
	{
		txt.Format("%s is not responding.  Please make sure the device is plugged in to your USB port and turned on.", szHardwareName);
		MessageBox(txt, "Rocker Table Hardware Error", MB_OK|MB_ICONERROR);
		return false;
	}
	// Reset the device
	status = FT_ResetDevice(m_ftHandle);
	if (status != FT_OK)
	{
		txt.Format("Unable to reset the %s. The device is present but unable to reset - operation cannot be guaranteed.  Try the following:\n\n1) Exit the program\n2) Unplug the USB from the device and plug it back in\n3) Start the software", szHardwareName);
		MessageBox(txt, "Rocker Table Hardware Error", MB_OK|MB_ICONERROR);
		return false;
	}
	// Set the communication baud rate
	status = FT_SetBaudRate(m_ftHandle,38400);
	if (status != FT_OK)
	{
		txt.Format("Unable to configure the %s. The device is having a problem being configured - operation cannot be guaranteed.  Try the following:\n\n1) Exit the program\n2) Unplug the USB from the device and plug it back in\n3) Start the software", szHardwareName);
		MessageBox(txt, "Rocker Table Hardware Error", MB_OK|MB_ICONERROR);
		return false;
	}
	// Set the communication parameters
	status = FT_SetDataCharacteristics(m_ftHandle,FT_BITS_8,FT_STOP_BITS_1,FT_PARITY_NONE);
	if (status != FT_OK)
	{
		txt.Format("Unable to configure the %s. The device is having a problem being configured - operation cannot be guaranteed.  Try the following:\n\n1) Exit the program\n2) Unplug the USB from the device and plug it back in\n3) Start the software", szHardwareName);
		MessageBox(txt, "Rocker Table Hardware Error", MB_OK|MB_ICONERROR);
		return false;
	}

	FT_Purge(m_ftHandle, FT_PURGE_RX|FT_PURGE_TX);

	return true;	// It passed everything, the hardware is present and ready
}


void CHypoxioDlg::OnBnClickedStart()
{
	CString txt;

	UpdateData(TRUE);

	if (m_iRockStart < m_iRockStop)
	{
		MessageBox("The rocking start value must be higher than the rocking stop value.","Invalid Rocking Values",MB_OK|MB_ICONERROR);
		return;
	}

	int iActiveCells = 0;
	for (int iCell=0; iCell<NUM_CELLS; iCell++)
	{
		if (m_BCellActive[iCell] == TRUE)	{ iActiveCells++; }
	}
	if (iActiveCells <= 0)
	{
		MessageBox("There are no cells selected to be active.  Please tag the active cells and try again.","No Active Cells",MB_OK|MB_ICONERROR);
		return;
	}

	if (m_iSpeedPct <= 0 || m_iSpeedPct > 100)	// 1 -> 100
	{
		MessageBox("Please enter a valid speed percentage (1-100).","Invalid Speed Percentage",MB_OK|MB_ICONERROR);
		return;
	}

	if (m_iCycles <= 0 || m_iCycles >= 100)		// 1 -> 99
	{
		MessageBox("Please enter a valid cycle count (1-99).","Invalid Cycle Count",MB_OK|MB_ICONERROR);
		return;
	}

	if (m_iAmountPct <= 0 || m_iAmountPct > 100)	// 1 -> 100
	{

		MessageBox("Please enter a valid tilt amount percentage (1-100).","Invalid Tilt Percentage",MB_OK|MB_ICONERROR);
		return;
	}

	m_iDurationCount = (m_iDurationHours*3600)+(m_iDurationMinutes*60)+m_iDurationSeconds;
	if (m_iDurationCount <= 0)
	{
		MessageBox("Please enter a valid duration.","Invalid Duration",MB_OK|MB_ICONERROR);
		return;
	}

	DisableInputControls();
	
	txt.Format("%d hrs %d mins %d secs", m_iDurationHours, m_iDurationMinutes, m_iDurationSeconds);
	m_pctDurationRemaining.SetDigitalFormat(txt);
	m_pctDurationRemaining.SetMax(m_iDurationCount);
	m_pctDurationRemaining.SetPortionValue(m_iDurationCount);

	SetTimer(TMR_ONESECOND, 1000, NULL);

	// Clear the list so we're prepared for the next burst of cell data
	m_vTrayDataRefreshed.clear();

	m_bRockingOn = false;

	// Reset the table tilt count statistic
	m_iTableTiltCount = 0;
	TableTiltCountUpdate(m_iTableTiltCount);

	// Start the Serial Port Splitter data receive thread
	setSplitterThreadExit(false);
	m_pThread = AfxBeginThread(threadDataReceive, this);

	// Start the USB Receive Handler
	m_hThreadUSBDataReceiveExit = CreateEvent(NULL, TRUE, FALSE, NULL);	// Manual Reset, Initially Non-Signaled
	m_pThreadUSBReceive = AfxBeginThread(threadDataReceiveUSBHardware, this);

	EnableStopButton();
}


void CHypoxioDlg::OnBnClickedStop()
{
	// Signal thread to exit
	setSplitterThreadExit(true);
	setUSBReceiveThreadExit();
	WaitForSingleObject(m_pThread->m_hThread, 5000);
	WaitForSingleObject(m_pThreadUSBReceive->m_hThread, 5000);

	KillTimer(TMR_ONESECOND);
	EnableInputControls();
	EnableStartButton();
}

void CHypoxioDlg::EnableInputControls(void)
{
	// Tray Configuration
	((CStatic*)GetDlgItem(LBL_STARTROCK))->EnableWindow(TRUE);
	((CEdit*)GetDlgItem(TXT_STARTROCK))->EnableWindow(TRUE);
	((CStatic*)GetDlgItem(LBL_STOPROCK))->EnableWindow(TRUE);
	((CEdit*)GetDlgItem(TXT_STOPROCK))->EnableWindow(TRUE);
	for (int iCell=0; iCell<NUM_CELLS; iCell++)
	{
		((CButton*)GetDlgItem(CHK_A1+iCell))->EnableWindow(TRUE);
	}
	((CButton*)GetDlgItem(BTN_TRAYSELECTALL))->EnableWindow(TRUE);
	((CButton*)GetDlgItem(BTN_TRAYSELECTNONE))->EnableWindow(TRUE);
	// Speed/Cycles/Amount
	((CStatic*)GetDlgItem(LBL_SPEEDPCT))->EnableWindow(TRUE);
	((CEdit*)GetDlgItem(TXT_SPEEDPCT))->EnableWindow(TRUE);
	((CStatic*)GetDlgItem(LBL_SPEEDPCT_SYM))->EnableWindow(TRUE);
	((CEdit*)GetDlgItem(TXT_CYCLES))->EnableWindow(TRUE);
	((CStatic*)GetDlgItem(LBL_CYCLES))->EnableWindow(TRUE);
	((CStatic*)GetDlgItem(LBL_AMOUNTPCT))->EnableWindow(TRUE);
	((CEdit*)GetDlgItem(TXT_AMOUNTPCT))->EnableWindow(TRUE);
	((CStatic*)GetDlgItem(LBL_AMOUNTPCT_SYM))->EnableWindow(TRUE);
	// Duration Time
	((CStatic*)GetDlgItem(LBL_DURATION))->EnableWindow(TRUE);
	((CEdit*)GetDlgItem(TXT_DURATION_HOURS))->EnableWindow(TRUE);
	((CStatic*)GetDlgItem(LBL_DURATION_HOURS))->EnableWindow(TRUE);
	((CEdit*)GetDlgItem(TXT_DURATION_MINUTES))->EnableWindow(TRUE);
	((CStatic*)GetDlgItem(LBL_DURATION_MINUTES))->EnableWindow(TRUE);
	((CEdit*)GetDlgItem(TXT_DURATION_SECONDS))->EnableWindow(TRUE);
	((CStatic*)GetDlgItem(LBL_DURATION_SECONDS))->EnableWindow(TRUE);
}


void CHypoxioDlg::DisableInputControls(void)
{
	// Tray Configuration
	((CStatic*)GetDlgItem(LBL_STARTROCK))->EnableWindow(FALSE);
	((CEdit*)GetDlgItem(TXT_STARTROCK))->EnableWindow(FALSE);
	((CStatic*)GetDlgItem(LBL_STOPROCK))->EnableWindow(FALSE);
	((CEdit*)GetDlgItem(TXT_STOPROCK))->EnableWindow(FALSE);
	for (int iCell=0; iCell<NUM_CELLS; iCell++)
	{
		((CButton*)GetDlgItem(CHK_A1+iCell))->EnableWindow(FALSE);
	}
	((CButton*)GetDlgItem(BTN_TRAYSELECTALL))->EnableWindow(FALSE);
	((CButton*)GetDlgItem(BTN_TRAYSELECTNONE))->EnableWindow(FALSE);
	// Speed/Cycles/Amount
	((CStatic*)GetDlgItem(LBL_SPEEDPCT))->EnableWindow(FALSE);
	((CEdit*)GetDlgItem(TXT_SPEEDPCT))->EnableWindow(FALSE);
	((CStatic*)GetDlgItem(LBL_SPEEDPCT_SYM))->EnableWindow(FALSE);
	((CEdit*)GetDlgItem(TXT_CYCLES))->EnableWindow(FALSE);
	((CStatic*)GetDlgItem(LBL_CYCLES))->EnableWindow(FALSE);
	((CStatic*)GetDlgItem(LBL_AMOUNTPCT))->EnableWindow(FALSE);
	((CEdit*)GetDlgItem(TXT_AMOUNTPCT))->EnableWindow(FALSE);
	((CStatic*)GetDlgItem(LBL_AMOUNTPCT_SYM))->EnableWindow(FALSE);
	// Duration Time
	((CStatic*)GetDlgItem(LBL_DURATION))->EnableWindow(FALSE);
	((CEdit*)GetDlgItem(TXT_DURATION_HOURS))->EnableWindow(FALSE);
	((CStatic*)GetDlgItem(LBL_DURATION_HOURS))->EnableWindow(FALSE);
	((CEdit*)GetDlgItem(TXT_DURATION_MINUTES))->EnableWindow(FALSE);
	((CStatic*)GetDlgItem(LBL_DURATION_MINUTES))->EnableWindow(FALSE);
	((CEdit*)GetDlgItem(TXT_DURATION_SECONDS))->EnableWindow(FALSE);
	((CStatic*)GetDlgItem(LBL_DURATION_SECONDS))->EnableWindow(FALSE);
}


void CHypoxioDlg::EnableStartButton(void)
{
	((CButton*)GetDlgItem(BTN_START))->EnableWindow(TRUE);
	((CButton*)GetDlgItem(BTN_STOP))->EnableWindow(FALSE);
	((CButton*)GetDlgItem(BTN_ROCKING_STOP))->EnableWindow(TRUE);
	((CButton*)GetDlgItem(BTN_TESTSETTINGS))->EnableWindow(TRUE);
	((CButton*)GetDlgItem(BTN_SPLITTERCOMSETTINGS))->EnableWindow(TRUE);
}


void CHypoxioDlg::EnableStopButton(void)
{
	((CButton*)GetDlgItem(BTN_START))->EnableWindow(FALSE);
	((CButton*)GetDlgItem(BTN_STOP))->EnableWindow(TRUE);
	((CButton*)GetDlgItem(BTN_ROCKING_STOP))->EnableWindow(TRUE);
	((CButton*)GetDlgItem(BTN_TESTSETTINGS))->EnableWindow(FALSE);
	((CButton*)GetDlgItem(BTN_SPLITTERCOMSETTINGS))->EnableWindow(FALSE);
}

void CHypoxioDlg::DisableButtons(void)
{
	((CButton*)GetDlgItem(BTN_START))->EnableWindow(FALSE);
	((CButton*)GetDlgItem(BTN_STOP))->EnableWindow(FALSE);
	((CButton*)GetDlgItem(BTN_ROCKING_STOP))->EnableWindow(FALSE);
	((CButton*)GetDlgItem(BTN_TESTSETTINGS))->EnableWindow(FALSE);
	((CButton*)GetDlgItem(BTN_SPLITTERCOMSETTINGS))->EnableWindow(TRUE);
}


void CHypoxioDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	CString txt;
	int iMins, iHrs;

	switch (nIDEvent)
	{
		case TMR_ONESECOND:
			m_iDurationCount--;
			// Duration
			iMins = m_iDurationCount / 60;
			iHrs = iMins / 60;
			txt.Format("%d hrs %d mins %d secs", iHrs, iMins%60, m_iDurationCount%60);
			m_pctDurationRemaining.SetDigitalFormat(txt);
			m_pctDurationRemaining.SetPortionValue(m_iDurationCount);

			if (m_iDurationCount <= 0)
			{
				// The specified duration has elapsed, auto stop.
				OnBnClickedStop();
			}

			break;
	}

	CDialogEx::OnTimer(nIDEvent);
}


void CHypoxioDlg::Tilt(int iSpeedPct, int iCycles, int iAmountPct)
{
	CString cmd;
	DWORD dwBytesWritten;

	cmd.Format("<T%03d%03d%02d>", iAmountPct, iSpeedPct, iCycles);
	FT_Write(m_ftHandle, cmd.GetBuffer(cmd.GetLength()), cmd.GetLength(), &dwBytesWritten);
}

void CHypoxioDlg::TiltReset()
{
	CString cmd;
	DWORD dwBytesWritten;

	cmd.Format("<R>");
	FT_Write(m_ftHandle, cmd.GetBuffer(cmd.GetLength()), cmd.GetLength(), &dwBytesWritten);
}


void CHypoxioDlg::OnBnClickedTestsettings()
{
	UpdateData(TRUE);

	if (m_iSpeedPct <= 0 || m_iSpeedPct > 100)	// 1 -> 100
	{
		MessageBox("Please enter a valid speed percentage (1-100).","Invalid Speed Percentage",MB_OK|MB_ICONERROR);
		return;
	}

	if (m_iCycles <= 0 || m_iCycles >= 100)		// 1 -> 99
	{
		MessageBox("Please enter a valid cycle count (1-99).","Invalid Cycle Count",MB_OK|MB_ICONERROR);
		return;
	}

	if (m_iAmountPct <= 0 || m_iAmountPct > 100)	// 1 -> 100
	{
		MessageBox("Please enter a valid tilt amount percentage (1-100).","Invalid Tilt Percentage",MB_OK|MB_ICONERROR);
		return;
	}

	// Send the command to tilt the table...
	Tilt(m_iSpeedPct, m_iCycles, m_iAmountPct);
}

BOOL CHypoxioDlg::FileExists(CString txtFilename)
{
	DWORD attr = GetFileAttributes(txtFilename);
    return DWORD(-1) != attr && !(FILE_ATTRIBUTE_DIRECTORY & attr);
}


void CHypoxioDlg::OnEnChangeSpeedpct()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
	CString txtValue;
	CString txtConfigFile(m_txtPathConfigFiles+FILENAME_CONFIGURATION);
	((CEdit*)GetDlgItem(TXT_SPEEDPCT))->GetWindowTextA(txtValue);
	WritePrivateProfileString(INI_APPNAME, INI_TILTSPEED, txtValue, txtConfigFile);
}


void CHypoxioDlg::OnEnChangeCycles()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
	CString txtValue;
	CString txtConfigFile(m_txtPathConfigFiles+FILENAME_CONFIGURATION);
	((CEdit*)GetDlgItem(TXT_CYCLES))->GetWindowTextA(txtValue);
	WritePrivateProfileString(INI_APPNAME, INI_TILTCYCLES, txtValue, txtConfigFile);
}


void CHypoxioDlg::OnEnChangeAmountpct()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
	CString txtValue;
	CString txtConfigFile(m_txtPathConfigFiles+FILENAME_CONFIGURATION);
	((CEdit*)GetDlgItem(TXT_AMOUNTPCT))->GetWindowTextA(txtValue);
	WritePrivateProfileString(INI_APPNAME, INI_TILTAMOUNT, txtValue, txtConfigFile);
}


void CHypoxioDlg::OnEnChangeDurationHours()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
	CString txtValue;
	CString txtConfigFile(m_txtPathConfigFiles+FILENAME_CONFIGURATION);
	((CEdit*)GetDlgItem(TXT_DURATION_HOURS))->GetWindowTextA(txtValue);
	WritePrivateProfileString(INI_APPNAME, INI_DURATION_HOURS, txtValue, txtConfigFile);
}


void CHypoxioDlg::OnEnChangeDurationMinutes()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
	CString txtValue;
	CString txtConfigFile(m_txtPathConfigFiles+FILENAME_CONFIGURATION);
	((CEdit*)GetDlgItem(TXT_DURATION_MINUTES))->GetWindowTextA(txtValue);
	WritePrivateProfileString(INI_APPNAME, INI_DURATION_MINUTES, txtValue, txtConfigFile);
}


void CHypoxioDlg::OnEnChangeDurationSeconds()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
	CString txtValue;
	CString txtConfigFile(m_txtPathConfigFiles+FILENAME_CONFIGURATION);
	((CEdit*)GetDlgItem(TXT_DURATION_SECONDS))->GetWindowTextA(txtValue);
	WritePrivateProfileString(INI_APPNAME, INI_DURATION_SECONDS, txtValue, txtConfigFile);
}

UINT CHypoxioDlg::threadDataReceive(LPVOID pParam)
{
	CHypoxioDlg* pParent= (CHypoxioDlg*) pParam;
	bool bDone = false;
	CString txtSegment = "";
	std::string txtBuffer = "";
	BOOL BRet;
	DWORD dwBytesRead;
	char buf[1];
	
	TRACE("threadDataReceive starting\n");

	// Regex Routines
	//
	// Matches the entire cell line
	//M01;C01;S00001F;PECEF;R00CD4F;FEE51;V162E;E00;
	std::regex txt_regex_cellline("M01;C[01][0-9A-F];S[0-9A-F]{6};P[0-9A-F]{4};R[0-9A-F]{6};F[0-9A-F]{4};V[0-9A-F]{4};E[0-9A-F]{2};");	
	std::regex txt_regex_cellid(";C[01][0-9A-F];");

	while (pParent->getSplitterThreadExit() == false)
	{
		// Read the incoming data and add it to the buffer
		BRet = ReadFile(pParent->m_hComm, buf, 1, &dwBytesRead, NULL);
		if (BRet && dwBytesRead)
		{
			// Append the new character to the buffer
			txtBuffer += buf[0];
			// Regex the current data to see if there's anything to process
			std::sregex_iterator it(txtBuffer.begin(), txtBuffer.end(), txt_regex_cellline);
			std::sregex_iterator it_end;
			while (it != it_end)
			{
				std::smatch m = *it;
				std::string txt = m.str();
				// Get cell id
				std::sregex_iterator cellid_it(txt.begin(), txt.end(), txt_regex_cellid);
				std::sregex_iterator cellid_it_end;
				while (cellid_it != cellid_it_end)
				{
					std::smatch cellid_m = *cellid_it;
					std::string cellid_txt = cellid_m.str();
					// Remove ;'s
					cellid_txt.erase(std::remove(cellid_txt.begin(), cellid_txt.end(), ';'), cellid_txt.end());
					// Remove C
					cellid_txt.erase(0, 1);
					int iCell = std::stoi(cellid_txt, nullptr, 16);

					// Update the received cell's data line
					pParent->SetTrayData(iCell, txt);

					++cellid_it;
					// Erase the buffer since we've extracted what we needed
					txtBuffer.erase(0, txtBuffer.length());
				}
				++it;
			}
		}
	}

	TRACE("threadDataReceive exiting\n");

	return 0;
}

UINT CHypoxioDlg::threadDataReceiveUSBHardware(LPVOID pParam)
{
	CHypoxioDlg* pParent= (CHypoxioDlg*) pParam;
	bool bDone = false;
	DWORD dwObj;
	DWORD dwRxBytes, dwTxBytes, dwEventDWord;
	char RxBuf[100];
	DWORD dwBytesRead;
	HANDLE hEvents[2];
	HANDLE hRXData = 0;
	FT_STATUS ftStatus;
	CString txtRxBuffer = "";
	CString txtCmd;
	int iStart = 0;
	int iEnd = 0;

	TRACE("threadDataReceiveUSBHardware starting\n");

	hRXData = CreateEvent(NULL,FALSE,FALSE,NULL);		// Auto Reset, Initially Non-Signaled

	hEvents[0] = pParent->m_hThreadUSBDataReceiveExit;
	hEvents[1] = hRXData;

	ftStatus = FT_SetEventNotification(pParent->m_ftHandle, FT_EVENT_RXCHAR, hRXData);
	if (ftStatus != FT_OK)
	{
		pParent->PostMessageA(WM_RXTHREAD_SETEVENTNOTIFICATION_ERROR, 0, 0);
		bDone = true;				// Exit the thread
	}

	while (!bDone)
	{
		dwObj = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);
		switch (dwObj)
		{
			case WAIT_OBJECT_0:
				bDone = true;
				break;
			case WAIT_OBJECT_0+1:
				// Get the number of characters waiting in the buffer
  		  	    FT_GetStatus(pParent->m_ftHandle, &dwRxBytes, &dwTxBytes, &dwEventDWord);
				ftStatus = FT_Read(pParent->m_ftHandle, RxBuf, dwRxBytes, &dwBytesRead);
				for (unsigned int iVal=0; iVal<dwRxBytes; iVal++)
				{
					txtRxBuffer += RxBuf[iVal];
				}
				// Process received buffer
				iStart = txtRxBuffer.Find('[');
				iEnd = txtRxBuffer.Find(']');
				while (iStart >= 0 && iEnd >= 0)
				{
					if (iStart < iEnd)
					{
						txtCmd = txtRxBuffer.Mid(iStart, iEnd-iStart+1);
						if (txtCmd.Find("LIMITSWITCH") >= 0)
						{
							if (txtCmd.Find("LEFT") >= 0)
							{
								// Hit the left limit switch
								pParent->PostMessageA(WM_RXTHREAD_LIMITSWITCHHIT, 0, 0);
							}
							else if (txtCmd.Find("RIGHT") >= 0)
							{
								// Hit the right limit switch
								pParent->PostMessageA(WM_RXTHREAD_LIMITSWITCHHIT, 0, 1);
							}
						}
						txtRxBuffer = txtRxBuffer.Right(txtRxBuffer.GetLength()-iEnd-1);
					}
					iStart = txtRxBuffer.Find('[');
					iEnd = txtRxBuffer.Find(']');
				}
				break;
		}
	}

	TRACE("threadDataReceiveUSBHardware exiting\n");

	return 0;
}

bool CHypoxioDlg::SplitterConnect()
{
	CString txtComPort;
	CString txtComParams;
	bool bReturn = true;

	// Check to see if we're connected already.  If we are, disconnect us so we can reconnect.
	if (m_bSplitterPresent)
	{
		SplitterDisconnect();
	}

	txtComPort.Format("\\\\.\\COM%d", m_tSplitterComSettings.iComPort);
	txtComParams.Format("baud=%d parity=%c data=%d stop=%d", m_tSplitterComSettings.iBaudRate, m_tSplitterComSettings.cParity, m_tSplitterComSettings.iDataBits, m_tSplitterComSettings.iStopBits);

	m_hComm = CreateFile(txtComPort,					// communication port string (COMX)
					     GENERIC_READ | GENERIC_WRITE,	// read/write types
					     0,								// comm devices must be opened with exclusive access
					     NULL,							// no security attributes
					     OPEN_EXISTING,					// comm devices must use OPEN_EXISTING
					     FILE_ATTRIBUTE_NORMAL,			//FILE_FLAG_OVERLAPPED
					     0);
	if (m_hComm != INVALID_HANDLE_VALUE)
	{
		// Set up some timeout values for reading from the com port.
		m_comTimeouts.ReadIntervalTimeout = MAXDWORD;
		m_comTimeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
		m_comTimeouts.ReadTotalTimeoutConstant = 5000;
		m_comTimeouts.WriteTotalTimeoutMultiplier = 1000;
		m_comTimeouts.WriteTotalTimeoutConstant = 1000;
	
		if (SetCommTimeouts(m_hComm, &m_comTimeouts))
		{
			DCB tdcb;
			memset(&tdcb, 0, sizeof(DCB));
			if (BuildCommDCB(txtComParams, &tdcb))
			{
				// DCB structure initialized with our settings, now enable Xon/Xoff flow control.
				//tdcb.fInX = TRUE;
				//tdcb.XoffChar = 0x13;
				//tdcb.XoffLim = 10;
				//tdcb.XonChar = 0x11;
				//tdcb.XonLim = 10;
				//tdcb.fOutX = TRUE;
				if (SetCommState(m_hComm, &tdcb))
				{
					// Clear the buffers so we're getting a fresh start
					PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

					// We could just set m_bHardwarePresent to true now, but realistically we don't know who or what is connected to us.  We need to get some 
					// information from the connected device to verify we have the proper hardware connected.  This thread only determines if the correct device 
					// is connected to the com port or not.
	//				m_pThread = AfxBeginThread(threadident, this);
					bReturn = true;
				}
				else
				{
					MessageBox("There was a problem setting the com port parameters.  Please restart the program and try again.","Comm Splitter Error", MB_OK|MB_ICONERROR);
					// Disconnect - since we had problems configuring the com port there's a pretty good chance we won't be able to talk to it anyway.
					SplitterDisconnect();
					bReturn = false;
				}
			}
			else
			{
				MessageBox("There was a problem configuring the parameters for the com port.  Please restart the program and try again.","Comm Splitter Error", MB_OK|MB_ICONERROR);
				// Disconnect - since we had problems configuring the com port there's a pretty good chance we won't be able to talk to it anyway.
				SplitterDisconnect();
				bReturn = false;
			}
		}
		else
		{
			MessageBox("There was a problem configuring the timeout values for the com port.  Please restart the program and try again.","Comm Splitter Error", MB_OK|MB_ICONERROR);
			// Disconnect - since we had problems configuring the com port there's a pretty good chance we won't be able to talk to it anyway.
			SplitterDisconnect();
			bReturn = false;
		}
	}
	else
	{
		MessageBox("Unable to open the selected com port.  Make sure you have the correct port configured in settings and restart the program.","Comm Splitter Error", MB_OK|MB_ICONERROR);
		// Disconnect - since we had problems configuring the com port there's a pretty good chance we won't be able to talk to it anyway.
		SplitterDisconnect();
		bReturn = false;
	}

	return bReturn;

}

void CHypoxioDlg::SplitterDisconnect()
{
	CloseHandle(m_hComm);
	m_hComm = NULL;
	m_bSplitterPresent = false;
}

void CHypoxioDlg::OnBnClickedSplittercomsettings()
{
	CDlgSplitterComSettings dlgSplitterSettings;
	dlgSplitterSettings.m_tSplitterComSettings = m_tSplitterComSettings;
	int iDlgRet = dlgSplitterSettings.DoModal();
	if (iDlgRet == 0)
	{
		// Accept
		m_tSplitterComSettings = dlgSplitterSettings.m_tSplitterComSettings;

		CString txtConfigFile(m_txtPathConfigFiles+FILENAME_CONFIGURATION);
		CString txt;
		txt.Format("%d", dlgSplitterSettings.m_tSplitterComSettings.iComPort);
		WritePrivateProfileString(INI_APPNAME, INI_SPLITTERCOM_COMPORT, txt, txtConfigFile);
		txt.Format("%d", dlgSplitterSettings.m_tSplitterComSettings.iBaudRate);
		WritePrivateProfileString(INI_APPNAME, INI_SPLITTERCOM_BAUDRATE, txt, txtConfigFile);
		txt.Format("%c", dlgSplitterSettings.m_tSplitterComSettings.cParity);
		WritePrivateProfileString(INI_APPNAME, INI_SPLITTERCOM_PARITY, txt, txtConfigFile);
		txt.Format("%d", dlgSplitterSettings.m_tSplitterComSettings.iDataBits);
		WritePrivateProfileString(INI_APPNAME, INI_SPLITTERCOM_DATABITS, txt, txtConfigFile);
		txt.Format("%d", dlgSplitterSettings.m_tSplitterComSettings.iStopBits);
		WritePrivateProfileString(INI_APPNAME, INI_SPLITTERCOM_STOPBITS, txt, txtConfigFile);

		MessageBox("You must restart the program for the new comm settings to take effect.","Changed Com Settings", MB_ICONEXCLAMATION|MB_OK);
	}
}

bool CHypoxioDlg::ReadDeviceData(CString * pData)
{
	char	buf[1];
	DWORD	dwBytesRead;
	CString	txtData;
	BOOL	BRet;
	bool	bSuccess = false;
	int		iRetries = 4;	// ReadFile will timeout after 5 seconds if there is no data to read in from the buffer.  To prevent 
							// the system hanging, we'll allow it to timeout a few times before calling it quits.

	// If there are any characters in the input buffer, ReadFile returns immediately with the characters in the buffer. 
	// If there are no characters in the input buffer, ReadFile waits until a character arrives and then returns immediately. 
	// If no character arrives within the time specified by ReadTotalTimeoutConstant, ReadFile times out.

	buf[0] = 0;
	txtData = "";
	while (iRetries >= 0)
	{
		BRet = ReadFile(m_hComm, buf, 1, &dwBytesRead, NULL);
		if (BRet && dwBytesRead)
		{
			txtData += buf[0];
		}
		else
		{
			iRetries--;
		}
	}

	if (iRetries >= 0)
	{
		*pData = txtData;
		bSuccess = true;
	}
	else
	{
		*pData = "";
		bSuccess = false;
	}
	return bSuccess;
}



bool CHypoxioDlg::getSplitterThreadExit(void)
{
	CSingleLock lock(&m_csSplitterThreadExit, TRUE);
	return m_bSplitterThreadExit;
}


void CHypoxioDlg::setSplitterThreadExit(bool bExit)
{
	CSingleLock lock(&m_csSplitterThreadExit, TRUE);
	m_bSplitterThreadExit = bExit;
}

void CHypoxioDlg::setUSBReceiveThreadExit()
{
	CSingleLock lock(&m_csUSBReceiveExit, TRUE);
	SetEvent(m_hThreadUSBDataReceiveExit);
}

void CHypoxioDlg::OnTrayWellClicked(UINT nID)
{
	CString txtValue, txtKey;
	CButton *pButton = (CButton*)GetDlgItem(nID);
	CString txtConfigFile(m_txtPathConfigFiles+FILENAME_CONFIGURATION);

	int iIndex = nID-CHK_A1;
	m_BCellActive[iIndex] = pButton->GetCheck();

	char cCol = 'A' + (iIndex / 6);
	int iRow;
	switch (cCol)
	{
		case 'A':
			iRow = iIndex + 1;
			break;
		case 'B':
			iRow = iIndex - 5;
			break;
		case 'C':
			iRow = iIndex - 11;
			break;
		case 'D':
			iRow = iIndex - 17;
			break;
	}

	txtKey.Format(INI_CELL_ACTIVE, cCol, iRow);
	txtValue.Format("%d", m_BCellActive[iIndex]);
	WritePrivateProfileString(INI_APPNAME, txtKey, txtValue, txtConfigFile);
}


void CHypoxioDlg::OnEnChangeStartrock()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
	CString txtValue;
	CString txtConfigFile(m_txtPathConfigFiles+FILENAME_CONFIGURATION);
	((CEdit*)GetDlgItem(TXT_STARTROCK))->GetWindowTextA(txtValue);
	WritePrivateProfileString(INI_APPNAME, INI_ROCK_START, txtValue, txtConfigFile);
}


void CHypoxioDlg::OnEnChangeStoprock()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
	CString txtValue;
	CString txtConfigFile(m_txtPathConfigFiles+FILENAME_CONFIGURATION);
	((CEdit*)GetDlgItem(TXT_STOPROCK))->GetWindowTextA(txtValue);
	WritePrivateProfileString(INI_APPNAME, INI_ROCK_STOP, txtValue, txtConfigFile);
}


void CHypoxioDlg::TrayViewUpdate(void)
{
	int iControl;
	
	for (iControl=CHK_A1; iControl<CHK_A1+NUM_CELLS; iControl++)
	{
		CButton *pButton = (CButton*)GetDlgItem(iControl);
		pButton->SetCheck(m_BCellActive[iControl-CHK_A1]);
	}
}


void CHypoxioDlg::SetTrayData(int iCell, std::string strData)
{
	m_TrayData[iCell-1] = strData;

	m_vTrayDataRefreshed.push_back(iCell);
	std::sort(m_vTrayDataRefreshed.rbegin(), m_vTrayDataRefreshed.rend());	// Sort descending so [0] holds the highest cell number last refreshed
	if (m_vTrayDataRefreshed.size() == NUM_CELLS && m_vTrayDataRefreshed[0] == NUM_CELLS)
	{
		// All of the cells have been refreshed
		PostMessage(WM_ALLCELLSREFRESHED, 0, 0);

		// Clear the list so we're prepared for the next burst of cell data
		m_vTrayDataRefreshed.clear();
	}
}


BOOL CHypoxioDlg::PreTranslateMessage(MSG* pMsg)
{
	CString txtMsg, txtSide;

	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == WM_ALLCELLSREFRESHED)
	{
		double dCellAverage = 0.0;
		int iNumCellsActive = 0;
		std::regex txt_regex_value(";V[0-9A-F]{4};");

		// Calculate the average of the enabled cells
		for (int iCell=0; iCell<NUM_CELLS; iCell++) {
			if (m_BCellActive[iCell]) {
				iNumCellsActive++;
				// Regex the current data to see if there's anything to process
				std::sregex_iterator it(m_TrayData[iCell].begin(), m_TrayData[iCell].end(), txt_regex_value);
				std::sregex_iterator it_end;
				while (it != it_end) {
					std::smatch m = *it;
					std::string txtValue = m.str();
					// Remove ;'s
					txtValue.erase(std::remove(txtValue.begin(), txtValue.end(), ';'), txtValue.end());
					// Remove V
					txtValue.erase(0, 1);
					int iCellValue = std::stoi(txtValue, nullptr, 16);
					dCellAverage += iCellValue;
					it++;
				}
			}
		}
		dCellAverage /= (double)iNumCellsActive;

		// Update the average value displayed on the dialog box
		CurrentAverageValueUpdate(dCellAverage);

		if (dCellAverage > m_iRockStart) {
			m_bRockingOn = true;
		} else if (dCellAverage < m_iRockStop) {
			m_bRockingOn = false;
		}

		if (m_bRockingOn == true) {
			// Send the tilt command to the hardware
			Tilt(m_iSpeedPct, m_iCycles, m_iAmountPct);
			m_iTableTiltCount += m_iCycles;
			TableTiltCountUpdate(m_iTableTiltCount);
		}

		return TRUE;		
	}
	else if (pMsg->message == WM_RXTHREAD_SETEVENTNOTIFICATION_ERROR)
	{
		txtMsg.Format("There was a problem configuring the event notifications for your USB hardware.  Please try closing the software and cycling the power on the hardware.");
		MessageBox(txtMsg, "Hardware Error", MB_OK|MB_ICONERROR);
		return TRUE;
	}
	else if (pMsg->message == WM_RXTHREAD_LIMITSWITCHHIT)
	{
		OnBnClickedStop();
		if (pMsg->lParam == 0)		{ txtSide = "left"; }
		else if (pMsg->lParam == 1)	{ txtSide = "right"; }
		txtMsg.Format("The table has hit the %s limit switch so operation has stopped:\n\n1. Remove power from the controller unit.\n2. Using the shaft extending from the rear of the motor, hand spin \n    the motor until the table is no longer touching the switch.\n3. Check the table to make sure it's securely fastened to the motor.\n4. Plug the power adapter back into the controller unit.\n\nThe table should recalibrate and recenter itself automatically.  Test the tilt settings to make sure the table responds properly.  If the table hits one of the limits switches during the test, contact the Electronics Shop for assistance.", txtSide);
		MessageBox(txtMsg, "Hardware Error", MB_OK|MB_ICONERROR);

		return TRUE;
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}


void CHypoxioDlg::CurrentAverageValueUpdate(double dValue)
{
	CString txtNewValue;
	txtNewValue.Format("%.1f", dValue);
	((CStatic*)GetDlgItem(LBL_CURRENTAVERAGE))->SetWindowText(txtNewValue);
}


void CHypoxioDlg::OnBnClickedTrayselectall()
{
	for (int iChk=CHK_A1; iChk<=CHK_D6; iChk++) {
		CButton *pButton = (CButton*)GetDlgItem(iChk);
		pButton->SetCheck(TRUE);
		OnTrayWellClicked(iChk);
	}
}


void CHypoxioDlg::OnBnClickedTrayselectnone()
{
	for (int iChk=CHK_A1; iChk<=CHK_D6; iChk++) {
		CButton *pButton = (CButton*)GetDlgItem(iChk);
		pButton->SetCheck(FALSE);
		OnTrayWellClicked(iChk);
	}
}


void CHypoxioDlg::TableTiltCountUpdate(int iNewCount)
{
	CString txtNewValue;
	txtNewValue.Format("%d", iNewCount);
	((CStatic*)GetDlgItem(LBL_STATS_TABLETILTCOUNT))->SetWindowText(txtNewValue);
}


void CHypoxioDlg::OnBnClickedRockingStop()
{
	TiltReset();
}
