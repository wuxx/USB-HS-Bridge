/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2022                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  基于CH347 SPI接口函数操作FLASH应用示例，FLASH 型号识别、块读、块写、块擦除、FLASH内容读至文件、文件
  写入FLASH、速度测试等操作函数。SPI传输速度可达2M字节/S

Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2022 Nanjing Qinheng Microelectronics Co., Ltd.
Revision History:
  4/3/2022: TECH30
--*/

DWORD WINAPI FlashVerifyWithFile(LPVOID lpParameter);
DWORD WINAPI WriteFlashFromFile(LPVOID lpParameter);
DWORD WINAPI ReadFlashToFile(LPVOID lpParameter);
//FLASH测试，先写再读
DWORD WINAPI FlashRWSpeedTest(LPVOID lpParameter);

//BOOL InitFlashSPI();
BOOL CH347InitSpi();
BOOL CH347SpiCsCtrl();
//FLASH块擦除
BOOL FlashBlockErase();
//Flash块数据写
BOOL FlashBlockWrite();
//FLASH字节读
BOOL FlashBlockRead();
//FLASH型号识别
BOOL FlashIdentify();
//关闭设备
BOOL CloseDevice();
//打开设备
BOOL OpenDevice();
BOOL CH347SpiStream(ULONG CmdCode);

BOOL I2C_WriteRead();

//枚举设备
ULONG EnumDevice();

VOID InitWindows_Eeprom();

BOOL Eeprom_InitI2C();

BOOL EerpromRead();

//Flash块数据写
BOOL EepromWrite();

DWORD WINAPI WriteEepromFromFile(LPVOID lpParameter);

DWORD WINAPI EepromVerifyWithFile(LPVOID lpParameter);

DWORD WINAPI ReadEepromToFile(LPVOID lpParameter);