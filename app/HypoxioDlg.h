
// HypoxioDlg.h : header file
//

#pragma once

#include "ftd2xx.h"
#include "percent.h"
#include <vector>
#include <string> 

const int NUM_CELLS = 24;

///////////////////////////////////////////////////////////////////////////////
// Configuration                                                             //
///////////////////////////////////////////////////////////////////////////////
const CString FILENAME_CONFIGURATION = "config.ini";
const CString INI_COMPANYNAME = "Brock University";
const CString INI_APPNAME = "Hypoxio";

const CString INI_TILTSPEED = "TiltSpeed";
const CString INI_TILTCYCLES = "TiltCycles";
const CString INI_TILTAMOUNT = "TiltAmount";
const CString INI_DURATION_HOURS = "DurationHours";
const CString INI_DURATION_MINUTES = "DurationMinutes";
const CString INI_DURATION_SECONDS = "DurationSeconds";

const CString INI_ROCK_START = "RockStart";
const CString INI_ROCK_STOP = "RockStop";
const CString INI_CELL_ACTIVE = "Cell%c%dActive";

const CString INI_SPLITTERCOM_COMPORT = "SplitterComPort";
const CString INI_SPLITTERCOM_BAUDRATE = "SplitterBaudRate";
const CString INI_SPLITTERCOM_PARITY = "SplitterParity";
const CString INI_SPLITTERCOM_DATABITS = "SplitterDataBits";
const CString INI_SPLITTERCOM_STOPBITS = "SplitterStopBits";

const int TMR_ONESECOND = 0;

///////////////////////////////////////////////////////////////////////////////
// Windows Messages                                                          //
///////////////////////////////////////////////////////////////////////////////
const int WM_ALLCELLSREFRESHED						= WM_APP + 0;
const int WM_RXTHREAD_SETEVENTNOTIFICATION_ERROR	= WM_APP + 1;
const int WM_RXTHREAD_LIMITSWITCHHIT				= WM_APP + 2;

// CHypoxioDlg dialog
class CHypoxioDlg : public CDialogEx
{
// Construction
public:
	CHypoxioDlg(CWnd* pParent = NULL);	// standard constructor
	
// Dialog Data
	enum { IDD = IDD_HYPOXIO_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

private:
	FT_HANDLE m_ftHandle;
	int m_iDurationCount;
	bool m_bHardwarePresent;
	CWinThread* m_pThread;
	CWinThread* m_pThreadUSBReceive;
	static UINT threadDataReceive(LPVOID pParam);
	static UINT threadDataReceiveUSBHardware(LPVOID pParam);
	bool SplitterConnect();
	void SplitterDisconnect();
	HANDLE m_hComm;
	bool m_bSplitterPresent;
	tSPLITTERCOMSETTINGS m_tSplitterComSettings;
	COMMTIMEOUTS m_comTimeouts;
	bool ReadDeviceData(CString * pData);

	HANDLE m_hThreadUSBDataReceiveExit;
	void setUSBReceiveThreadExit();
	CCriticalSection m_csUSBReceiveExit;

public:
	int m_iCycles;
	int m_iSpeedPct;
	int m_iDurationHours;
	int m_iDurationMinutes;
	int m_iDurationSeconds;
	BOOL m_BCellActive[NUM_CELLS];
	CPercent m_pctDurationRemaining;
	bool LoadUSBHardware(void);
	void EnableInputControls(void);
	void DisableInputControls(void);
	void EnableStartButton(void);
	void EnableStopButton(void);
	void DisableButtons(void);
	bool m_bSplitterThreadExit;
	CCriticalSection m_csSplitterThreadExit;

	afx_msg void OnBnClickedStart();
	afx_msg void OnBnClickedStop();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	void Tilt(int iSpeedPct, int iCycles, int iAmountPct);
	void TiltReset();
	int m_iAmountPct;
	afx_msg void OnBnClickedTestsettings();
	CString m_txtPathConfigFiles;
	BOOL FileExists(CString txtFilename);
	afx_msg void OnEnChangeSpeedpct();
	afx_msg void OnEnChangeCycles();
	afx_msg void OnEnChangeAmountpct();
	afx_msg void OnEnChangeDurationHours();
	afx_msg void OnEnChangeDurationMinutes();
	afx_msg void OnEnChangeDurationSeconds();

	afx_msg void OnBnClickedSplittercomsettings();
	afx_msg void OnTrayWellClicked(UINT nID);

	bool getSplitterThreadExit(void);
	void setSplitterThreadExit(bool bExit);
	int m_iRockStart;
	int m_iRockStop;
	afx_msg void OnEnChangeStartrock();
	afx_msg void OnEnChangeStoprock();
	void TrayViewUpdate(void);

	std::vector <std::string> m_TrayData;
	std::vector <int> m_vTrayDataRefreshed;
	void SetTrayData(int iCell, std::string strData);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	void CurrentAverageValueUpdate(double dValue);
	afx_msg void OnBnClickedTrayselectall();
	afx_msg void OnBnClickedTrayselectnone();
	bool m_bRockingOn;
	void TableTiltCountUpdate(int iNewCount);
private:
	int m_iTableTiltCount;
public:
	afx_msg void OnBnClickedRockingStop();
};
