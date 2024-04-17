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


#include "Main.h"
#include "EepromDebug.h"

//全局变量
extern HWND SpiI2cGpioDebugHwnd;     //窗体句柄
extern BOOL DevIsOpened;   //设备是否打开
extern BOOL SpiIsCfg;
extern ULONG SpiI2cGpioDevIndex;

extern BOOL IntIsEnable;

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

//CH347中断响应程序
BOOL IntService(ULONG IntTrigGpioN)
{
	ULONG i;
	for(i=0;i<8;i++)
	{
		if( IntTrigGpioN & (1<<i) )//中断GPIO脚号
			DbgPrint("Gpio%d interrupt(%X)",i,IntTrigGpioN);
	}
	return TRUE;
}

// 中断回调函数。
// 返回为8个字节，每个字节对应一个GPIO状态，位定义如下
//位7：当前的GPIO0方向，0：输入；1：输出，中断模式下必须为0；
//位6：当前的GPIO0中断边沿，0：下降沿；1：上升沿；
//位5：当前的GPIO0是否设置为中断，0：查询模式；1：中断模式；中断模式下必须为1；
//位4：保留；
//位3：当前的GPIO0中断状态，0：未触发；1：触发；
VOID	CALLBACK	CH347_INT_ROUTINE(PUCHAR			iStatus )  // 中断状态数据,参考下面的位说明
{
	ULONG IntTrigGpioN = 0,i;
	for(i=0;i<8;i++)
	{
		if( iStatus[i]&0x08 ) //中断GPIO脚号
			IntTrigGpioN |= (1<<i);			
	}
	PostMessage(SpiI2cGpioDebugHwnd,WM_CH347Int,IntTrigGpioN,0);
}

//开启中断
BOOL IntEnable()
{
	BOOL Int0Enable,Int1Enable;
	UCHAR Int0PinN = 8,Int1PinN = 8;
	UCHAR Int0TrigMode=0x03,Int1TrigMode=0x03;

	//不能重复设置	
	SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_DisableIntNotify,BM_CLICK,0,0);

	Int0Enable = (IsDlgButtonChecked(SpiI2cGpioDebugHwnd,IDC_Int0Enable)==BST_CHECKED);
	if(Int0Enable)
	{
		//中断0 GPIO引脚号,大于7:不启用此中断源; 为0-7对应gpio0-7
		Int0PinN = (UCHAR)SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_Int0PinSel,CB_GETCURSEL,0,0);
		//中断0类型: 00:下降沿触发; 01:上升沿触发; 02:双边沿触发; 03:保留;
		Int0TrigMode = (UCHAR)SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_Int0TrigMode,CB_GETCURSEL,0,0);
	}
	else
	{
		Int0PinN = 0xFF;
		Int0TrigMode = 0x03;
	}
	Int1Enable = (IsDlgButtonChecked(SpiI2cGpioDebugHwnd,IDC_Int1Enable)==BST_CHECKED);
	if(Int1Enable)
	{
		//中断1 GPIO引脚号,大于7则不启用此中断源,为0-7对应gpio0-7
		Int1PinN = (UCHAR)SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_Int1PinSel,CB_GETCURSEL,0,0);
		//中断1类型: 00:下降沿触发; 01:上升沿触发; 02:双边沿触发; 03:保留;
		Int1TrigMode = (UCHAR)SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_Int1TrigMode,CB_GETCURSEL,0,0);
	}
	else
	{
		Int1PinN = 0xFF;
		Int1TrigMode = 0x03;
	}
	//指定中断服务程序,为NULL则取消中断服务,否则在中断时调用该程序
	IntIsEnable = CH347SetIntRoutine(SpiI2cGpioDevIndex,Int0PinN,Int0TrigMode,Int1PinN,Int1TrigMode,CH347_INT_ROUTINE );
	DbgPrint("CH347 interrupt notify routine set %s ",IntIsEnable?"succ":"false");

	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_EnableIntNotify),!IntIsEnable);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_DisableIntNotify),IntIsEnable);

	return TRUE;
}

//关闭中断
BOOL IntDisable()
{
	{
		//指定中断服务程序,为NULL则取消中断服务,否则在中断时调用该程序
		IntIsEnable = CH347SetIntRoutine(SpiI2cGpioDevIndex,0xFF,0xFF,0xFF,0xFF,NULL );
		DbgPrint("CH347 interrupt notify routine cancell %s ",IntIsEnable?"succ":"false");

		IntIsEnable = FALSE;
	}
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_EnableIntNotify),TRUE);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_DisableIntNotify),FALSE);

	return TRUE;
}