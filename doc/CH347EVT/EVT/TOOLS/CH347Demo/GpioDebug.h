/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2022                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  基于CH347 GPIO接口函数操作应用示例

Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2022 Nanjing Qinheng Microelectronics Co., Ltd.
Revision History:
  4/3/2022: TECH30
--*/


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