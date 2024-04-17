/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2023                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  ����CH347 SPI�ӿں�������FLASHӦ��ʾ����FLASH �ͺ�ʶ�𡢿������д���������FLASH���ݶ����ļ����ļ�
  д��FLASH���ٶȲ��ԵȲ���������SPI�����ٶȿɴ�2M�ֽ�/S

Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
Revision History:
  4/3/2022: TECH30
--*/

DWORD WINAPI FlashVerifyWithFile(LPVOID lpParameter);
DWORD WINAPI WriteFlashFromFile(LPVOID lpParameter);
DWORD WINAPI ReadFlashToFile(LPVOID lpParameter);
//FLASH���ԣ���д�ٶ�
DWORD WINAPI FlashRWSpeedTest(LPVOID lpParameter);

//BOOL InitFlashSPI();
BOOL CH347InitSpi();
BOOL CH347SpiCsCtrl();
//FLASH�����
BOOL FlashBlockErase();
//Flash������д
BOOL FlashBlockWrite();
//FLASH�ֽڶ�
BOOL FlashBlockRead();
//FLASH�ͺ�ʶ��
BOOL FlashIdentify();
//�ر��豸
BOOL CloseDevice();
//���豸
BOOL OpenDevice();
BOOL CH347SpiStream(ULONG CmdCode);

BOOL I2C_WriteRead();

//ö���豸
ULONG EnumDevice();

VOID InitWindows_Eeprom();

BOOL Eeprom_InitI2C();

BOOL EerpromRead();

//Flash������д
BOOL EepromWrite();

DWORD WINAPI WriteEepromFromFile(LPVOID lpParameter);

DWORD WINAPI EepromVerifyWithFile(LPVOID lpParameter);

DWORD WINAPI ReadEepromToFile(LPVOID lpParameter);