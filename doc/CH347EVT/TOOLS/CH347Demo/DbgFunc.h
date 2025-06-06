/**********************************************************
**  Copyright  (C)  WCH  2001-2023                       **
**  Web:  http://wch.cn                                  **
***********************************************************
Abstract:
    Auxiliary function
Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
  SPDX-License-Identifier: Apache-2.0
Revision History:
  3/1/2022: TECH30
--*/

#ifndef		_DEBUGFUNC_H
#define		_DEBUGFUNC_H

VOID  DbgPrint (LPCTSTR lpFormat,...); //显示格式化字符串
void ShowLastError(LPCTSTR lpFormat,...); //显示上次运行错误
double GetCurrentTimerVal(); //获取硬件计数器已运行时间,ms为单位,比GetTickCount更准确
ULONG mStrToHEX(PCHAR str); //将十六进制字符转换成十进制码,数字转换成字符用ltoa()函数
VOID  AddStrToEdit (HWND hDlg,ULONG EditID,const char * Format,...); //将格式化字符信息输出到文本框末尾
ULONG EndSwitch(ULONG dwVal);//大小端切换
VOID SetEditlInputMode(//设置EDIT控件文本输入和十进制输入
					    HWND hWnd,        // handle of window
						ULONG CtrlID,     //控件ID号
						UCHAR InputMode); //输入模式:0:文本模式,1:十六进制模式
					   
#endif