/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2022                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  USB2.0  UART API Demo
Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
Revision History:
  4/3/2022: TECH30
--*/

VOID Uart_InitParam();

//枚举设备
ULONG Uart_EnumDevice();

//关闭设备
BOOL Uart_CloseDevice();

//打开设备
BOOL Uart_OpenDevice();

BOOL Uart_Set();

BOOL Uart_Read();

BOOL Uart_Write();

//使能操作按钮
VOID EnableButtonEnable_Uart();

BOOL APIENTRY DlgProc_UartDbg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);