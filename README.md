USB-HS-Bridge
-----------
[中文](./README_cn.md)
* [USB-HS-Bridge Introduce](#usb-hs-bridge-introduce) 
* [Work Mode](#work-mode)
* [Features](#Features)
* [How-to-Use](#how-to-use)
* [Product Link](#Product-Link)
* [Reference](#Reference)


# USB-HS-Bridge Introduce
USB-HS-Bridge is a debug tool made by MuseLab based on WCH's CH347T/CH347F With USB 2.0 High Speed, it support USB-to-I2C/SPI/UART/JTAG/GPIO, can be used to operate various devices include MCU/DSP/FPGA/CPLD/EEPROM/SPI-Flash/LCD...
![1](https://github.com/wuxx/USB-HS-Bridge/blob/master/doc/1.jpg)

# CH347T
the work mode of CH347T is configured by DTR1 and RTS1  
mode| DTR1 | RTS1 | detail | 
----|------|------|--------|
0   |  1   |  1   | UART0 + UART1 |
1   |  1   |  0   | UART1 + I2C + SPI (VCP Mode) |
2   |  0   |  1   | UART1 + I2C + SPI (HID Mode) |
3   |  0   |  0   | UART1 + JTAG  |

# CH347F
The package of CH347F is QFN28, which has enough pins, so there is no need to switch the working mode to reuse the pins. UART0/UART1/I2C/SPI/JTAG can be used at the same time. and the IO level is adjustable by switch, the options are 3.3V/2.5V/1.8V/Vref.

# Features
## USB-to-UART
there are two UART peripherals on board, UART0 and UART1, both are enabled in mode-0, and only UART1 enabled in mode-1/2/3, UART0 support all modem signals and UART1 support some modem signals, both support hardware flow control.


## USB-to-I2C
the I2C(note: I2C Master) is enabled in mode-1/2, the difference between mode-1/2 is that mode-1 need to install the driver provided by WCH, and mode 2 does not require driver installation (act as USB HID Device).

## USB-to-SPI
the SPI(note: SPI Master with 4-line) is enabled in mode-1/2, there are two CS pins, so you can control two SPI devices at once(time-sharing).

## USB-to-JTAG
the JTAG is enabled in mode-3, include TCK/TMS/TDI/TDO/TRST, support fast-mode and bit-bang mode, uhe fastest speed up to 18Mbit/s.

# How to Use
there is a test demo under doc/CH347EVT/EVT/TOOLS/CH347Demo/ to test UART/I2C/SPI/JTAG/GPIO.
for openocd support,  please reference WCH official repo https://github.com/WCHSoftGroup/ch347


# Product Link
[USB-HS-Bridge Board](https://www.aliexpress.com/item/1005004685449797.html?spm=5261.ProductManageOnline.0.0.158c4edffnRuaN)

# Reference
### WCH
https://www.wch.cn/
### Openocd Patch for CH347
https://github.com/WCHSoftGroup/ch347
