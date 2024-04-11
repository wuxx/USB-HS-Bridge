USB-HS-Bridge
-----------
[English](./README.md)
* [介绍](#介绍) 
* [工作模式](#工作模式)
* [特性](#特性)
* [如何使用](#如何使用)
* [产品链接](#产品链接)
* [参考](#参考)


# 介绍
USB-HS-Bridge 是MuseLab基于沁恒CH347T/CH347F制作的调试工具，USB 2.0高速设备，支持 USB转I2C/SPI/UART/JTAG/GPIO, 可用于调试操作多种外设传感器以及MCU、FPGA的调试下载等。
![1](https://github.com/wuxx/USB-HS-Bridge/blob/master/doc/1.jpg)

# CH347T
由于CH347T没有足够的引脚，所以在上电启动的时候需要使用DTR1和RTS1进行模式的配置
mode| DTR1 | RTS1 | detail | 
----|------|------|--------|
0   |  1   |  1   | UART0 + UART1 |
1   |  1   |  0   | UART1 + I2C + SPI (VCP Mode) |
2   |  0   |  1   | UART1 + I2C + SPI (HID Mode) |
3   |  0   |  0   | UART1 + JTAG  |

# CH347F
CH347F的封装为QFN28，其有足够的引脚，所以不需要工作模式的切换来复用引脚，所以可以同时使用UART0+UART1+I2C+SPI+JTAG，而且IO电平可调，可通过拨码开关来选择IO电平，可选择的为3.3V/2.5V/1.8V/Vref

# 特性
## USB-to-UART
CH347支持2路UART，单路最高波特率可达9Mbps，其中UART0支持所有的modem控制信号和流控，UART1支持部分modem信号和流控。

## USB-to-I2C
I2C工作在Master主机模式，支持4种传输速度，可用于操作EEPROM和传感器等器件

## USB-to-SPI
SPI工作在Master主机模式，最高频率可达36MHz，为标准四线SPI，支持2路片选，可分时操作2个SPI从设备。

## USB-to-JTAG
JTAG为标准4线JTAG协议，最高频率可达18Mbit/s, 包括TCK/TMS/TDI/TDO以及TRST，可用于实现MCU调试下载以及FPGA下载等。

# 如何使用
官方的demo程序位于doc/CH347EVT/EVT/TOOLS/CH347Demo/, 可用于测试验证各个接口的基本功能，另外也可参考手册实现自定义的应用程序。
openocd 的支持请参考 https://github.com/WCHSoftGroup/ch347 。

# 产品链接
[USB-HS-Bridge Board](https://item.taobao.com/item.htm?spm=a1z10.3-c.w4002-21349689064.10.72f8773dlRkUhZ&id=682518391864)

# Reference
### WCH
https://www.wch.cn/
### Openocd patch for CH347
https://github.com/WCHSoftGroup/ch347

