/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2023                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  CH347DLL API DEMO
  高速USB(480Mbps)转接芯片CH347,基于480Mbps高速USB总线扩展UART、SPI、I2C、JTAG，
  平台驱动齐全，用于USB转双串口、USB转SPI、USB转I2C、USB转JTAG、CPU调试器、FPGA下载器、烧录器等。
Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
  SPDX-License-Identifier: Apache-2.0
Revision History:
  4/3/2022: TECH30
*/

#ifndef __MAIN_H
#define __MAIN_H

//To disable deprecation, use _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

// Windows Header Files:
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#pragma comment(lib,"winmm")
#include <time.h>
#include <stdio.h>
#include "setupapi.h"
#pragma comment(lib,"setupapi")
#pragma comment(lib,"comctl32.lib")

#include "DbgFunc.h"
#include "resource.h"

#include "ExternalLib\\CH347DLL.H"
#pragma comment(lib,"ExternalLib\\I386\\CH347DLL.lib")

#include "SPI_FLASH.h"
#include "FlashDebug.h"
#include "UartDebug.h"
#include "GpioDebug.h"

typedef struct _USB_DEVICE_DESCRIPTOR {
    UCHAR bLength;
    UCHAR bDescriptorType;
    USHORT bcdUSB;
    UCHAR bDeviceClass;
    UCHAR bDeviceSubClass;
    UCHAR bDeviceProtocol;
    UCHAR bMaxPacketSize0;
    USHORT idVendor;
    USHORT idProduct;
    USHORT bcdDevice;
    UCHAR iManufacturer;
    UCHAR iProduct;
    UCHAR iSerialNumber;
    UCHAR bNumConfigurations;
} USB_DEVICE_DESCRIPTOR, *PUSB_DEVICE_DESCRIPTOR;
#pragma pack() 

// 添加用于设置SPI速率
#define KHZ(n) ((n)*ULONG(1000))
#define MHZ(n) ((n)*ULONG(1000000))

BOOL APIENTRY DlgProc_SpiUartI2cDbg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL APIENTRY DlgProc_FlashEepromDbg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL APIENTRY DlgProc_JtagDebug(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#endif