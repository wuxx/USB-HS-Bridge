USB-HS-Bridge
-----------
[中文](./README_cn.md)
* [USB-HS-Bridge Introduce](#usb-hs-bridge-introduce) 
* [Features](#Features)
* [Work Mode](#work-mode)
* [DIP Switch Config](#dip-switch-config)
* [How-to-Use](#how-to-use)
* [Product Link](#Product-Link)
* [Reference](#Reference)


# USB-HS-Bridge Introduce
USB-HS-Bridge v1.3 is a debug tool made by MuseLab based on WCH's CH346C With USB 2.0 High Speed, it support USB-to-UART/SPI Slave/UART/Passive parallel port/GPIO, can be used for a variety of high-speed transmission application
![1](https://github.com/wuxx/USB-HS-Bridge/blob/master/doc/CH346-1.jpg)

# Features
- 480Mbps High-Speed USB 2.0 Device
- support USB-to-UART/SPI Slave/UART/Passive parallel port/GPIO
- IO level is adjustable, supports 3.3V/2.5V/1.8V/VREF
- Support different working modes
 
# Work Mode
Mode 2 is directly determined by the DIP switch M2. When the DIP switch M2 is turned ON, it switches to Mode 2. When it is turned OFF, it switches to Mode 0 or Mode 1. Mode 0 and Mode 1 can be determined by the dynamic configuration of the host computer software.
mode|  M2  | detail |
----|------|--------|
0   |  OFF | UART0 + Passive parallel port |
1   |  OFF | UART0 + SPI Slave             |
2   |  ON  | UART0 + UART1                 |

# DIP Switch Config
DIP switch 1 is used to configure the work mode, and DIP switches 2-5 are used to configure the IO level. Note that DIP switches 2-5 are four-choice switches. When one switch is turned ON, the others must be turned OFF.
switch|  desc | detail |
------|-------|--------|
1     |  M2   | ON to switches to mode 2, OFF to switches to mode 0 or mode 1. Mode 0 and 1 can be dynamically configured by the host software |
2     |  VREF | IO level is configured as VREF and provided by an external pin|
3     |  1V8  | IO level is configured as 1V8|
4     |  2V5  | IO level is configured as 2V5|
5     |  3V3  | IO level is configured as 3V3|


# How to Use
there is a test demo under doc/CH346EVT/EVT/TOOLS/PC/CH346Demo/ for test with source code.

# Product Link
[USB-HS-Bridge Board](https://www.aliexpress.com/item/1005004685449797.html?spm=5261.ProductManageOnline.0.0.158c4edffnRuaN)

# Reference
### WCH
https://www.wch.cn/
### CH346 EVT & DRV
https://www.wch-ic.com/downloads/CH346EVT_ZIP.html  
https://www.wch-ic.com/downloads/CH346DRV_ZIP.html
