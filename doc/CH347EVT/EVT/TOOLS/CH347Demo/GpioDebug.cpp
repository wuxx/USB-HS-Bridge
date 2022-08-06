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


#include "Main.h"
#include "EepromDebug.h"

//全局变量
extern HWND SpiI2cGpioDebugHwnd;     //窗体句柄
extern BOOL DevIsOpened;   //设备是否打开
extern BOOL SpiIsCfg;
extern ULONG SpiI2cGpioDevIndex;

//GPIO使能控件ID
ULONG GpioEnCtrID[8] = {IDC_EnSet_Gpio0,IDC_EnSet_Gpio1,IDC_EnSet_Gpio2,IDC_EnSet_Gpio3,IDC_EnSet_Gpio4,IDC_EnSet_Gpio5,IDC_EnSet_Gpio6,IDC_EnSet_Gpio7};
//GPIO方向控件ID
ULONG GpioDirCtrID[8] = {IDC_Dir_Gpio0,IDC_Dir_Gpio1,IDC_Dir_Gpio2,IDC_Dir_Gpio3,IDC_Dir_Gpio4,IDC_Dir_Gpio5,IDC_Dir_Gpio6,IDC_Dir_Gpio7};
//GPIO电平控件ID
ULONG GpioStaCtrID[8] = {IDC_Val_Gpio0,IDC_Val_Gpio1,IDC_Val_Gpio2,IDC_Val_Gpio3,IDC_Val_Gpio4,IDC_Val_Gpio5,IDC_Val_Gpio6,IDC_Val_Gpio7};

//GPIO设置
BOOL Gpio_Set()
{
	ULONG i;
	UCHAR oEnable = 0;       //数据有效标志:对应位0-7,对应GPIO0-7.
	UCHAR oSetDirOut = 0;    //设置I/O方向,某位清0则对应引脚为输入,某位置1则对应引脚为输出.GPIO0-7对应位0-7.
	UCHAR oSetDataOut = 0;   //输出数据,如果I/O方向为输出,那么某位清0时对应引脚输出低电平,某位置1时对应引脚输出高电平
	BOOL  RetVal;

	for(i=0;i<8;i++)
	{
		//取使能位
		if( IsDlgButtonChecked(SpiI2cGpioDebugHwnd,GpioEnCtrID[i])==BST_CHECKED )
			oEnable |= (1<<i);
		//取方向
		if( IsDlgButtonChecked(SpiI2cGpioDebugHwnd,GpioDirCtrID[i])==BST_CHECKED )
			oSetDirOut |= (1<<i);
		//取电平值
		if( IsDlgButtonChecked(SpiI2cGpioDebugHwnd,GpioStaCtrID[i])==BST_CHECKED )
			oSetDataOut |= (1<<i);
	}
	RetVal = CH347GPIO_Set(SpiI2cGpioDevIndex,oEnable,oSetDirOut,oSetDataOut);
	DbgPrint("CH347GPIO_SetOutput %s,oEnable:%02X,oSetDirOut:%02X,oSetDataOut:%02X",RetVal?"Succ":"Fail",oEnable,oSetDirOut,oSetDataOut);

	return RetVal;
}

//GPIO状态获取
BOOL Gpio_Get()
{
	BOOL RetVal;
	UCHAR iDir = 0,iData = 0,i,Sel;

	RetVal = CH347GPIO_Get(SpiI2cGpioDevIndex,&iDir,&iData);
	DbgPrint("CH347GPIO_Get %s,iDir:%02X,iData:%02X",RetVal?"Succ":"Fail",iDir,iData);
	if(RetVal)
	{
		for(i=0;i<8;i++)
		{		
			//显示方向
			Sel = (iDir&(1<<i))?BST_CHECKED:BST_UNCHECKED;
			CheckDlgButton(SpiI2cGpioDebugHwnd,GpioDirCtrID[i],Sel);				
			//电平值
			Sel = (iData&(1<<i))?BST_CHECKED:BST_UNCHECKED;
			CheckDlgButton(SpiI2cGpioDebugHwnd,GpioStaCtrID[i],Sel);
		}
	}
	return RetVal;
}

//使能全部GPIO
VOID GpioEnableAll()
{
	BOOL Sel;
	UCHAR i;

	Sel = IsDlgButtonChecked(SpiI2cGpioDebugHwnd,IDC_GpioEnableAll);
	for(i=0;i<8;i++)
		CheckDlgButton(SpiI2cGpioDebugHwnd,GpioEnCtrID[i],Sel);

	return;
}

//选中全部GPIO方向
VOID GpioSetDirAll()
{
	BOOL Sel;
	UCHAR i;

	Sel = IsDlgButtonChecked(SpiI2cGpioDebugHwnd,IDC_GpioSetDirAll);
	for(i=0;i<8;i++)
		CheckDlgButton(SpiI2cGpioDebugHwnd,GpioDirCtrID[i],Sel);

	return;
}

//选中全部GPIO电平
VOID GpioSetDataAll()
{
	BOOL Sel;
	UCHAR i;

	Sel = IsDlgButtonChecked(SpiI2cGpioDebugHwnd,IDC_GpioSetDataAll);
	for(i=0;i<8;i++)
		CheckDlgButton(SpiI2cGpioDebugHwnd,GpioStaCtrID[i],Sel);

	return;
}