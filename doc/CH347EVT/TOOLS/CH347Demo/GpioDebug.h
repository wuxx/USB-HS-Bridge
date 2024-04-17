/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2023                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  基于CH347 GPIO接口函数操作应用示例

Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
Revision History:
  4/3/2022: TECH30
--*/

#define WM_CH347Int       WM_USER+12         //设备触发响应

//GPIO设置
BOOL Gpio_Set();
//GPIO状态获取
BOOL Gpio_Get();
//选中全部GPIO电平
VOID GpioSetDataAll();
//使能全部GPIO
VOID GpioEnableAll();
//选中全部GPIO方向
VOID GpioSetDirAll();
//开启中断
BOOL IntEnable();
//关闭中断
BOOL IntDisable();
//CH347中断响应程序
BOOL IntService(ULONG IntTrigGpioN);