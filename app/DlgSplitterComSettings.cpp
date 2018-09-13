// DlgSplitterComSettings.cpp : implementation file
//

#include "stdafx.h"
#include "Hypoxio.h"
#include "DlgSplitterComSettings.h"
#include "afxdialogex.h"

// CDlgSplitterComSettings dialog

IMPLEMENT_DYNAMIC(CDlgSplitterComSettings, CDialogEx)

CDlgSplitterComSettings::CDlgSplitterComSettings(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDlgSplitterComSettings::IDD, pParent)
{

}

CDlgSplitterComSettings::~CDlgSplitterComSettings()
{
}

void CDlgSplitterComSettings::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//  DDX_Control(pDX, CMB_COMPORT, m_cmbComPort);
	DDX_Control(pDX, CMB_BAUDRATE, m_cmbBaudRate);
	DDX_Control(pDX, CMB_DATABITS, m_cmbDataBits);
	DDX_Control(pDX, CMB_PARITY, m_cmbParity);
	DDX_Control(pDX, CMB_STOPBITS, m_cmbStopBits);
	DDX_Control(pDX, TXT_COMPORT, m_edtComPort);
}


BEGIN_MESSAGE_MAP(CDlgSplitterComSettings, CDialogEx)
	ON_BN_CLICKED(BTN_ACCEPT, &CDlgSplitterComSettings::OnBnClickedAccept)
	ON_BN_CLICKED(BTN_CANCEL, &CDlgSplitterComSettings::OnBnClickedCancel)
END_MESSAGE_MAP()


// CDlgSplitterComSettings message handlers


BOOL CDlgSplitterComSettings::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Populate the comboboxes with the valid options
	InitControls();

	// Select the currect configuration
	CString txt;
	// Com Port
	txt.Format("%d", m_tSplitterComSettings.iComPort);
	m_edtComPort.SetWindowTextA(txt);
	// BaudRate
	txt.Format("%d", m_tSplitterComSettings.iBaudRate);
	m_cmbBaudRate.SelectString(-1, txt);
	// Parity
	switch (m_tSplitterComSettings.cParity)
	{
		case 'N':
			txt.Format("None");
			break;
		case 'O':
			txt.Format("Odd");
			break;
		case 'E':
			txt.Format("Even");
			break;
	}
	m_cmbParity.SelectString(-1, txt);
	// Data Bits
	txt.Format("%d", m_tSplitterComSettings.iDataBits);
	m_cmbDataBits.SelectString(-1, txt);
	// Stop Bits
	txt.Format("%d", m_tSplitterComSettings.iStopBits);
	m_cmbStopBits.SelectString(-1, txt);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void CDlgSplitterComSettings::InitControls(void)
{
	CString txt;

	// Com Ports
	m_edtComPort.SetWindowText("1");

	// Baud Rate
	//GetCommProperties()
	//	COMMPROP.dwSettableBaud
	m_cmbBaudRate.AddString("9600");
	m_cmbBaudRate.AddString("19200");
	m_cmbBaudRate.AddString("38400");
	m_cmbBaudRate.AddString("57600");
	m_cmbBaudRate.AddString("115200");

	// Parity
	m_cmbParity.AddString("None");
	m_cmbParity.AddString("Odd");
	m_cmbParity.AddString("Even");

	// Data Bits
	m_cmbDataBits.AddString("7");
	m_cmbDataBits.AddString("8");

	// Stop Bits
	m_cmbStopBits.AddString("1");
	m_cmbStopBits.AddString("2");

}


void CDlgSplitterComSettings::OnBnClickedAccept()
{
	CString txt;

	m_edtComPort.GetWindowText(txt);
	m_tSplitterComSettings.iComPort = atoi(txt);
	m_cmbBaudRate.GetWindowText(txt);
	m_tSplitterComSettings.iBaudRate = atoi(txt);
	m_cmbParity.GetWindowText(txt);
	if (txt.Compare("None") == 0)		{ txt = "N"; }
	else if (txt.Compare("Odd") == 0)	{ txt = "O"; }
	else if (txt.Compare("Even") == 0)	{ txt = "E"; }
	m_tSplitterComSettings.cParity = txt[0];
	m_cmbDataBits.GetWindowText(txt);
	m_tSplitterComSettings.iDataBits = atoi(txt);
	m_cmbStopBits.GetWindowText(txt);
	m_tSplitterComSettings.iStopBits = atoi(txt);

	EndDialog(0);
}


void CDlgSplitterComSettings::OnBnClickedCancel()
{
	EndDialog(-1);
}
