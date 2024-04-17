/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2023                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  USB2.0  UART API Demo
Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
Revision History:
  4/3/2022: TECH30
--*/

VOID Uart_InitParam();

//ö���豸
ULONG Uart_EnumDevice();

//�ر��豸
BOOL Uart_CloseDevice();

//���豸
BOOL Uart_OpenDevice();

BOOL Uart_Set();

BOOL Uart_Read();

BOOL Uart_Write();

//ʹ�ܲ�����ť
VOID EnableButtonEnable_Uart();

BOOL APIENTRY DlgProc_UartDbg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);