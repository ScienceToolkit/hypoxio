#pragma once

#include <vector>

// CDlgSplitterComSettings dialog

class CDlgSplitterComSettings : public CDialogEx
{
	DECLARE_DYNAMIC(CDlgSplitterComSettings)

public:
	CDlgSplitterComSettings(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgSplitterComSettings();

// Dialog Data
	enum { IDD = IDD_SPLITTERCOMSETTINGS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
//	CComboBox m_cmbComPort;
	CComboBox m_cmbBaudRate;
	CComboBox m_cmbDataBits;
	CComboBox m_cmbParity;
	CComboBox m_cmbStopBits;
	virtual BOOL OnInitDialog();
	void InitControls(void);
	afx_msg void OnBnClickedAccept();
	afx_msg void OnBnClickedCancel();
	tSPLITTERCOMSETTINGS m_tSplitterComSettings;
	CEdit m_edtComPort;
};
