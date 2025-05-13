/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2025                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  ����CH347/CH339W GPIO�ӿں�������Ӧ��ʾ��

Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2025 Nanjing Qinheng Microelectronics Co., Ltd.
Revision History:
  4/3/2022: TECH30
--*/

#define WM_CH347Int       WM_USER+12         //�豸������Ӧ

//GPIO����
BOOL Gpio_Set();
//GPIO״̬��ȡ
BOOL Gpio_Get();
//ѡ��ȫ��GPIO��ƽ
VOID GpioSetDataAll();
//ʹ��ȫ��GPIO
VOID GpioEnableAll();
//ѡ��ȫ��GPIO����
VOID GpioSetDirAll();
//�����ж�
BOOL IntEnable();
//�ر��ж�
BOOL IntDisable();
//CH347�ж���Ӧ����
BOOL IntService(ULONG IntTrigGpioN);