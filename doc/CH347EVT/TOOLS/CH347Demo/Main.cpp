/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2023                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  CH347DLL API DEMO
  ����USB(480Mbps)ת��оƬCH347,����480Mbps����USB������չUART��SPI��I2C��JTAG��
  ƽ̨������ȫ������USBת˫���ڡ�USBתSPI��USBתI2C��USBתJTAG��CPU��������FPGA����������¼���ȡ�
Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
  SPDX-License-Identifier: Apache-2.0
Revision History:
  4/3/2022: TECH30
--*/
#include "Main.h"

//ȫ�ֱ���
HINSTANCE AfxMainIns; //����ʵ��
ULONG Flash_Sector_Count;   /* FLASHоƬ������ */
USHORT Flash_Sector_Size;   /* FLASHоƬ������С */
CHAR IsCloseNow;
CHAR CaptionBuf[128]="";
BOOL g_isChinese = TRUE;
HWND AfxMainHwnd;
int CALLBACK PropSheetProc(HWND hwndDlg,
						   UINT uMsg,
						   LPARAM lParam
						   )
{
	//ProSheetHandle = hwndDlg;
	switch(uMsg){
		case PSCB_INITIALIZED:
			{//��ӳ�������alt+tab�л�ʱ��ʾ��ͼ��
				HICON hicon;
				DWORD dwStyle;
				RECT rc;

				hicon = (HICON)LoadIcon(AfxMainIns,(LPCTSTR)IDI_Main);
				PostMessage(hwndDlg,WM_SETICON,ICON_BIG,(LPARAM)(HICON)hicon);
				PostMessage(hwndDlg,WM_SETICON,ICON_SMALL,(LPARAM)(HICON)hicon);

				dwStyle = GetWindowLong(hwndDlg, GWL_STYLE);
				dwStyle |= WS_MINIMIZEBOX;
				SetWindowLong(hwndDlg,GWL_STYLE,dwStyle);
				IsCloseNow = 1;
				LANGID lid=GetSystemDefaultLangID(); //�ж���Ӣ��ϵͳ
				if(lid==0x0804)
				{
					g_isChinese = TRUE;
				}
				else
				{
					g_isChinese = FALSE;
					SetThreadUILanguage( MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT));
				}
				{//��ȡϵͳ�汾����WIN10������ϵͳ����������С
					OSVERSIONINFO osvi;

					ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
					osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
					GetVersionEx(&osvi);
					if( (osvi.dwMajorVersion >= 6) && (osvi.dwMinorVersion >= 2) )
					{
						GetWindowRect (hwndDlg,&rc);
						if (g_isChinese) {
							// Increase the height of the CPropertySheet by 30
							rc.bottom = (LONG)(rc.bottom*0.1);
							// Increase the width of CPropertySheet by 50
							rc.right = (LONG)(rc.right*0.7);
						} else {
							// Increase the height of the CPropertySheet by 30
						    rc.bottom = (LONG)(rc.bottom*1);
						}
						// Resize the CPropertySheet
						MoveWindow (hwndDlg,rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,TRUE);          // Convert to client coordinates
						ScreenToClient (hwndDlg,(LPPOINT)&rc);
					}
				}
			}
		break;
	}
	return TRUE;
}

int CreatePropertySheet(HWND hwndOwner)//��������ҳ
{
    PROPSHEETPAGE psp[4]={0};//����ҳ�棺��׼��ʽ������ͨ��������ʽ������ǿ�ͷ�������ʽ��
	PROPSHEETHEADER psh={0};
	CHAR MainAddr[13]="tech@wch.cn\0";

    psp[0].dwSize = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags = PSP_USETITLE | PSP_HASHELP;;
    psp[0].hInstance = AfxMainIns;
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_SpiUartI2cDbg);
    psp[0].pszIcon = NULL;
    psp[0].pfnDlgProc = DlgProc_SpiUartI2cDbg;
	psp[0].pszTitle = "   SPI/I2C/GPIO Debug  ";
    psp[0].lParam = 0;

	psp[1].dwSize = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags = PSP_USETITLE | PSP_HASHELP;;
    psp[1].hInstance = AfxMainIns;
    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_JtagDebug);
    psp[1].pszIcon = NULL;
    psp[1].pfnDlgProc = DlgProc_JtagDebug;
	psp[1].pszTitle = "   Jtag Debug     ";
    psp[1].lParam = 0;	

	psp[2].dwSize = sizeof(PROPSHEETPAGE);
    psp[2].dwFlags = PSP_USETITLE | PSP_HASHELP;;
    psp[2].hInstance = AfxMainIns;
    psp[2].pszTemplate = MAKEINTRESOURCE(IDD_UartDbg);
    psp[2].pszIcon = NULL;
    psp[2].pfnDlgProc = DlgProc_UartDbg;
	psp[2].pszTitle = "    Uart Debug    ";
    psp[2].lParam = 0;	
	
	psp[3].dwSize = sizeof(PROPSHEETPAGE);
    psp[3].dwFlags = PSP_USETITLE | PSP_HASHELP;;
    psp[3].hInstance = AfxMainIns;
    psp[3].pszTemplate = MAKEINTRESOURCE(IDD_FlashEepromDbg);
    psp[3].pszIcon = NULL;
    psp[3].pfnDlgProc = DlgProc_FlashEepromDbg;
	psp[3].pszTitle = "Flash/Eeprom Debug";
    psp[3].lParam = 0;
    
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE|PSH_USEHICON|PSH_NOAPPLYNOW|PSH_USECALLBACK|PSH_HASHELP;
    psh.hwndParent = hwndOwner;
    psh.hInstance = AfxMainIns;
 
	strcpy(&CaptionBuf[0],"USB2.0(HS) TO SPI/I2C/UART Demo(For CH347T Mode0-Mode3 and CH347F)");
	psh.pszCaption = CaptionBuf;

    psh.nStartPage = 0;
	psh.hwndParent = hwndOwner;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;
	psh.hIcon = (HICON)LoadImage(AfxMainIns, MAKEINTRESOURCE(IDI_Main), IMAGE_ICON, LR_DEFAULTSIZE, LR_DEFAULTSIZE, LR_DEFAULTCOLOR); 
	psh.pfnCallback = PropSheetProc;
	do{
		PropertySheet(&psh);
	}while(!IsCloseNow);
    return 0;
}

//Ӧ�ó������
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	DbgPrint("------  CH347DEMO Build on %s-%s -------\n", __DATE__, __TIME__ );

	AfxMainIns = hInstance;

	srand( (unsigned)time( NULL ) );
	return CreatePropertySheet(NULL);
}
