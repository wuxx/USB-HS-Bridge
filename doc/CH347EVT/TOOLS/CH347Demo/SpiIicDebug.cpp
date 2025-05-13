/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2025                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  CH347/CH339W SPI/I2C接口数据流操作

Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2025 Nanjing Qinheng Microelectronics Co., Ltd.
Revision History:
  4/3/2022: TECH30
--*/

#include "Main.h"

#define WM_CH347DevArrive WM_USER+10         //设备插入通知事件,窗体进程接收
#define WM_CH347DevRemove WM_USER+11         //设备拔出通知事件,窗体进程接收

#define CH347DevID "VID_1A86&PID_55\0"  //监视CH347&CH339W USB插拔动作,插入时可自动打开设备;关闭时自动关闭设备

//如需准确监测各模式下串口插拔动作，可写如下完整USBID。因DEMO汇总了所有模式，所以只需取ID共同部分
//设置指定设备的USB插拔监测: CH347SetDeviceNotify(,USBID_Mode???,)
//取消指定设备的USB插拔监测: CH347SetDeviceNotify(,USBID_Mode???,)
//#define USBID_VCP_Mode0_UART0       "VID_1A86&PID_55DA&MI_00\0"  //MODE0 UART0
//#define USBID_VCP_Mode0_UART1       "VID_1A86&PID_55DA&MI_01\0"  //MODE0 UART1
//#define USBID_VCP_Mode0_UART        "VID_1A86&PID_55DA\0"        //MODE0 UART
//#define USBID_VEN_Mode1_UART1       "VID_1A86&PID_55DB&MI_00\0"  //MODE1 UART
//#define USBID_HID_Mode2_UART1       "VID_1A86&PID_55DB&MI_00\0"  //MODE2 UART
//#define USBID_VEN_Mode3_UART1       "VID_1A86&PID_55DB&MI_00\0"  //MODE3 UART

//如需准确监测各模式下接口插拔动作，可写如下完整USBID。因DEMO汇总了所有模式，所以只需取ID共同部分
//设置指定设备的USB插拔监测: CH347Uart_SetDeviceNotify(,USBID_Mode???,)
//取消指定设备的USB插拔监测: CH347Uart_SetDeviceNotify(,USBID,)
//#define USBID_VEN_Mode1_SPI_I2C     "VID_1A86&PID_55DA&MI_00\0"  //MODE1 SPI/I2C
//#define USBID_HID_Mode2_SPI_I2C     "VID_1A86&PID_55DA&MI_00\0"  //MODE2 SPI/I2C
//#define USBID_VEN_Mode3_JTAG_I2C    "VID_1A86&PID_55DA&MI_00\0"  //MODE3 JTAG/I2C

extern BOOL g_isChinese;
extern HINSTANCE AfxMainIns; //进程实例 
extern HWND AfxActiveHwnd;
extern HWND JtagDlgHwnd;
extern HWND FlashEepromDbgHwnd;
extern HWND UartDebugHwnd;
extern BOOL EnablePnPAutoOpen_Jtag; //启用插拔后设备自动打开关闭功能
extern BOOL EnablePnPAutoOpen_Uart; //启用插拔后设备自动打开关闭功能
extern BOOL EnablePnPAutoOpen_Flash; //启用插拔后设备自动打开关闭功能


//全局变量
HWND SpiI2cGpioDebugHwnd;     //窗体句柄
BOOL DevIsOpened;   //设备是否打开
BOOL SpiIsCfg;
BOOL I2CIsCfg;
ULONG SpiI2cGpioDevIndex;
mDeviceInforS SpiI2cDevInfor[16] = {0};
BOOL EnablePnPAutoOpen; //启用插拔后设备自动打开关闭功能
BOOL  IntIsEnable=FALSE; //中断使能
// CH347设备插拔检测通知程序.因回调函数对函数操作有限制，通过窗体消息转移到消息处理函数内进行处理。
VOID	 CALLBACK	 UsbDevPnpNotify (ULONG iEventStatus ) 
{
	if(iEventStatus==CH347_DEVICE_ARRIVAL)// 设备插入事件,已经插入
		PostMessage(SpiI2cGpioDebugHwnd,WM_CH347DevArrive,0,0);
	else if(iEventStatus==CH347_DEVICE_REMOVE)// 设备拔出事件,已经拔出
		PostMessage(SpiI2cGpioDebugHwnd,WM_CH347DevRemove,0,0);
	return;
}

//打开设备
BOOL OpenDevice()
{
	//获取设备序号
	SpiI2cGpioDevIndex = SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_ObjList,CB_GETCURSEL,0,0);
	if(SpiI2cGpioDevIndex==CB_ERR)
	{
		if (g_isChinese)
			DbgPrint("打开设备失败,请先选择设备");
		else 
			DbgPrint("Failed to open the device, please select the device first");
		goto Exit; //退出
	}	
	DevIsOpened = (CH347OpenDevice(SpiI2cGpioDevIndex) != INVALID_HANDLE_VALUE);
	CH347SetTimeout(SpiI2cGpioDevIndex,500,500);
	DbgPrint(">>Open the device...%s",DevIsOpened?"Success":"Failed");

	CH347InitSpi();
Exit:
	return DevIsOpened;
}

//关闭设备
BOOL CloseDevice()
{
	CH347CloseDevice(SpiI2cGpioDevIndex);	
	if(IntIsEnable)
		SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_DisableIntNotify,BM_CLICK,0,0);
	IntIsEnable = FALSE;
	DevIsOpened = FALSE;
	DbgPrint(">>Close the Device");

	return TRUE;
}

BOOL CH347InitSpi()
{	
	BOOL RetVal = FALSE;
	mSpiCfgS SpiCfg = {0};
	mSpiCfgS TestSpiCfg = {0};
	UCHAR HwVer = 0;
	CHAR  Frequency[32]="";
	DOUBLE SpiFrequency = 0;	// 设置的频率
	UCHAR SpiDatabits = 0;	// 设置的数据位 默认0
	
	
	SpiCfg.iMode = (UCHAR)SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_SpiCfg_Mode,CB_GETCURSEL,0,0);
	SpiCfg.iClock = (UCHAR)SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_SpiCfg_Clock,CB_GETCURSEL,0,0);
	SpiCfg.iByteOrder = (UCHAR)SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_SpiCfg_ByteBitOrder,CB_GETCURSEL,0,0);
	SpiCfg.iSpiWriteReadInterval = GetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiCfg_OutInIntervalT,NULL,FALSE);
	SpiCfg.iSpiOutDefaultData = 0xFF;
	SpiCfg.iChipSelect = SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_SpiCfg_ChipIndex,CB_GETCURSEL,0,0);	
	SpiCfg.CS1Polarity = (UCHAR)SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_SpiCfg_CS1Polarity,CB_GETCURSEL,0,0);	
	SpiCfg.CS2Polarity = (UCHAR)SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_SpiCfg_CS2Polarity,CB_GETCURSEL,0,0);
	if(IsDlgButtonChecked(SpiI2cGpioDebugHwnd,IDC_EnableCS)==BST_CHECKED)
		SpiCfg.iChipSelect |= 0x80;
	SpiCfg.iIsAutoDeativeCS = (IsDlgButtonChecked(SpiI2cGpioDebugHwnd,IDC_AutoDeativeCS)==BST_CHECKED);
	SpiCfg.iActiveDelay = GetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiCfg_ActiveDelay,NULL,FALSE);
	SpiCfg.iDelayDeactive = GetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiCfg_DelayDeactive,NULL,FALSE);

	// 若使能设置其他速率，则调用CH347SPISetFrequency
	if (IsDlgButtonChecked(SpiI2cGpioDebugHwnd,IDC_SpiCfg_SetFreq) == BST_CHECKED)
	{
		// 设置SPI频率
		
		GetDlgItemText(SpiI2cGpioDebugHwnd, IDC_SpiCfg_Frequency, Frequency, 10);
		SpiFrequency = atof(Frequency);
		DbgPrint("SpiFrequency %.2f\n", SpiFrequency);
		
		// 获取设置单位
		if (SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_SpiCfg_frequencyUnit,CB_GETCURSEL,0,0))
			RetVal = CH347SPI_SetFrequency(SpiI2cGpioDevIndex, KHZ(SpiFrequency));
		else 
			RetVal = CH347SPI_SetFrequency(SpiI2cGpioDevIndex, MHZ(SpiFrequency));
	} 
	else 
	{
		RetVal = CH347SPI_SetFrequency(SpiI2cGpioDevIndex, 0);
	}

	// 设置SPI 数据位
	SpiDatabits = (UCHAR)SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_SpiCfg_Databits,CB_GETCURSEL,0,0);
	RetVal = CH347SPI_SetDataBits(SpiI2cGpioDevIndex, SpiDatabits);

	RetVal = CH347SPI_Init(SpiI2cGpioDevIndex,&SpiCfg);

	CH347SPI_GetCfg(SpiI2cGpioDevIndex, &TestSpiCfg);
	DbgPrint("iByteOrder:%02x.\n", TestSpiCfg.iByteOrder);
	DbgPrint("CH347SPI_Init %s",RetVal?"succ":"failure");
	
	return RetVal;
}

// 添加此处对齐SPI初始化操作
BOOL CH347InitI2C()
{
	BOOL RetVal = FALSE;
	BOOL isStrentch = FALSE;
	ULONG iMode;	// I2C模式
	ULONG I2CDelayMs = 0;
	
	iMode = SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_I2CCfg_Clock,CB_GETCURSEL,0,0);
	isStrentch = SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_I2CCfg_SclStretch,CB_GETCURSEL,0,0) ? FALSE : TRUE ;
	I2CDelayMs = GetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_I2CCfg_Delay,NULL,FALSE);
	
	RetVal = CH347I2C_Set(SpiI2cGpioDevIndex, iMode);
	DbgPrint("CH347I2C Set clock %s",RetVal ? "succ" : "failure");

	DbgPrint("CH347 I2C set stetching %s",RetVal ? "succ" : "failure");

	if (I2CDelayMs > 0)
		RetVal = CH347I2C_SetDelaymS(SpiI2cGpioDevIndex, I2CDelayMs);
	
	DbgPrint("CH347InitI2C %s",RetVal ? "succ" : "failure");

	return RetVal;
}

BOOL CH347SpiCsCtrl()
{
	USHORT          iEnableSelect;      // 低八位为CS1，高八位为CS2; 字节值为1=设置CS,为0=忽略此CS设置
	USHORT          iChipSelect;		// 低八位为CS1，高八位为CS2;片选输出,0=L,1=H
	USHORT          iIsAutoDeativeCS;   // 低八位为CS1，高八位为CS2;操作完成后是否自动撤消片选
    ULONG           iActiveDelay;		// 低八位为CS1，高八位为CS2;设置片选后执行读写操作的延时时间
	ULONG           iDelayDeactive;		// 低八位为CS1，高八位为CS2;撤消片选后执行读写操作的延时时间
	UCHAR           CsSel;
	BOOL RetVal;
	mSpiCfgS SpiCfg = {0};

	CsSel = (UCHAR)SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_SpiCfg_ChipIndex,CB_GETCURSEL,0,0);	
	iEnableSelect = ( IsDlgButtonChecked(SpiI2cGpioDebugHwnd,IDC_EnableCS)==BST_CHECKED );
	iChipSelect = (UCHAR)SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_SpiCsStatus,CB_GETCURSEL,0,0);	
	iIsAutoDeativeCS = (IsDlgButtonChecked(SpiI2cGpioDebugHwnd,IDC_AutoDeativeCS)==BST_CHECKED);
	iActiveDelay = GetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiCfg_ActiveDelay,NULL,FALSE)&0xFF;
	iDelayDeactive = GetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiCfg_DelayDeactive,NULL,FALSE)&0xFF;	
	if(iEnableSelect)
	{
		if( CsSel )
		{
			iEnableSelect = 0x0100;		
			iChipSelect = (iChipSelect<<8)&0xFF00;
			iIsAutoDeativeCS = (iIsAutoDeativeCS<<8)&0xFF00;
			iActiveDelay = (iActiveDelay<<16)&0xFFFF0000;
			iDelayDeactive = (iDelayDeactive<<16)&0xFFFF0000;
		}
		else 
		{
			iEnableSelect = 0x01;
		}
	}
	else
		iEnableSelect = 0;

	RetVal = CH347SPI_SetChipSelect(SpiI2cGpioDevIndex,iEnableSelect,iChipSelect,iIsAutoDeativeCS,iActiveDelay,iDelayDeactive);
	
	DbgPrint("CH347SPI_ConfigCS %s",RetVal?"succ":"failure");

	return RetVal;
}

BOOL CH347SpiStream(ULONG CmdCode)
{
	ULONG SpiOutLen,SpiInLen,FlashAddr=0,i,StrLen;
	UCHAR InBuf[4096] = "",OutBuf[4096] = "";
	CHAR FmtStr[4096*3*6] = "",ValStr[16]="",FmtStr2[4096*3*6];
	double BT,UseT;
	UCHAR ChipSelect;
	BOOL RetVal = FALSE;

	if(IsDlgButtonChecked(SpiI2cGpioDebugHwnd,IDC_EnableCS)==BST_CHECKED )
		ChipSelect = 0x80;
	else
		ChipSelect = 0x00;

	StrLen = GetDlgItemText(SpiI2cGpioDebugHwnd,IDC_SpiOut,FmtStr,sizeof(FmtStr));	
	if(StrLen > 4096*3)
		StrLen = 4096*3;
	SpiOutLen = 0;
	for(i=0;i<StrLen;i+=3)
	{		
		memcpy(&ValStr[0],&FmtStr[i],2);
		OutBuf[SpiOutLen] = (UCHAR)mStrToHEX(ValStr);
		SpiOutLen++;
	}
	SetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiOutLen,SpiOutLen,FALSE);
	SpiInLen = GetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiInLen,NULL,FALSE);	
	if(SpiInLen>4096)
		SpiInLen = 4096;	
	SetDlgItemText(SpiI2cGpioDebugHwnd,IDC_SpiIn,"");
	memset(FmtStr,0,sizeof(FmtStr));

	BT = GetCurrentTimerVal();
	if(CmdCode == 0xC2)
	{
		SpiInLen = SpiOutLen; //输出字节数与输入字节数相等
		if(SpiOutLen<1)
		{
			DbgPrint("SPI read length not specified");
			return FALSE;
		}
		memcpy(InBuf,OutBuf,SpiOutLen);
		RetVal = CH347StreamSPI4(SpiI2cGpioDevIndex,0x80,SpiOutLen,InBuf);
		sprintf(FmtStr,"Cmd%X(StreamSpi) %s.",CmdCode,RetVal?"succ":"failure");
	}
	else if(CmdCode == 0xC3)
	{
		if(SpiInLen<1)
		{
			DbgPrint("SPI read length not specified");
			return FALSE;
		}
		if (SpiOutLen < 1)
			SpiOutLen = 0;
		memcpy(InBuf, OutBuf, SpiOutLen);
		RetVal = CH347SPI_Read(SpiI2cGpioDevIndex,ChipSelect,SpiOutLen,&SpiInLen,InBuf);
		sprintf(FmtStr,"Cmd%X(StreamSpiBulkRead) %s.",CmdCode,RetVal?"succ":"failure");
	}
	else if(CmdCode == 0xC4)
	{
		if(SpiOutLen<1)
		{
			DbgPrint("SPI read length not specified");
			return FALSE;
		}
		SpiInLen = 0;
		RetVal = CH347SPI_Write(SpiI2cGpioDevIndex,ChipSelect,SpiOutLen,512,OutBuf);
		sprintf(FmtStr,"Cmd%X(StreamSpiBulkWrite) %s.",CmdCode,RetVal?"succ":"failure");
	}
	else
		return FALSE;
	UseT = GetCurrentTimerVal()-BT;
	sprintf(&FmtStr[strlen(FmtStr)],",Time %.3fS.",UseT/1000);
	
	if(RetVal)
	{
		if(SpiOutLen)
		{//打印
			sprintf(&FmtStr[strlen(FmtStr)],"\r\n                    OutData(%d):",SpiOutLen);
			//16进制显示
			for(i=0;i<SpiOutLen;i++)
			{			
				sprintf(&FmtStr[strlen(FmtStr)],"%02X ",OutBuf[i]);						
			}		
		}
		if(SpiInLen)
		{//打印
			//memset(FmtStr,0,sizeof(FmtStr));
			sprintf(&FmtStr[strlen(FmtStr)],"\r\n                    InData (%d):",SpiInLen);
			//16进制显示
			FmtStr2[0]=0;
			for(i=0;i<SpiInLen;i++)
			{
				sprintf(&FmtStr[strlen(FmtStr)],"%02X ",InBuf[i]);
				sprintf(&FmtStr2[strlen(FmtStr2)],"%02X ",InBuf[i]);
			}
			SetDlgItemText(SpiI2cGpioDebugHwnd,IDC_SpiIn,FmtStr2);		
			SetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiInLen,SpiInLen,FALSE);		
		}
	}
	else
	{
		SetDlgItemText(SpiI2cGpioDebugHwnd,IDC_SpiIn,"");		
		SetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiInLen,0,FALSE);
	}
	DbgPrint("%s",FmtStr);
	return RetVal;
}

BOOL I2C_WriteRead()
{
	ULONG OutLen,InLen,i,StrLen;
	UCHAR OutBuf[4096] = "",InBuf[4096] = "";
	CHAR FmtStr[4096*3*6] = "",ValStr[16]="";
	double BT,UseT;
	BOOL RetVal = FALSE;

	StrLen = GetDlgItemText(SpiI2cGpioDebugHwnd,IDC_I2COut,FmtStr,sizeof(FmtStr));
	if(StrLen > 4096*3)
		StrLen = 4096*3;
	OutLen = 0;
	for(i=0;i<StrLen;i+=3)
	{		
		memcpy(&ValStr[0],&FmtStr[i],2);
		OutBuf[OutLen] = (UCHAR)mStrToHEX(ValStr);
		OutLen++;
	}
	SetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_I2COutLen,OutLen,FALSE);	

	InLen = GetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_I2CInLen,NULL,FALSE);
	if(InLen>4096)
		InLen = 4096;

	if((OutLen+InLen)<1)
	{
		DbgPrint("Read length not specified");
		return FALSE;
	}		
	BT = GetCurrentTimerVal();		
	RetVal = CH347StreamI2C(SpiI2cGpioDevIndex,OutLen,OutBuf,InLen,InBuf);
	UseT = GetCurrentTimerVal()-BT;
	if(RetVal)
	{		
		if(InLen)
		{//打印
			memset(FmtStr,0,sizeof(FmtStr));			
			for(i=0;i<InLen;i++)
			{
				sprintf(&FmtStr[strlen(FmtStr)],"%02X ",InBuf[i]);
			}
			SetDlgItemText(SpiI2cGpioDebugHwnd,IDC_I2CIn,FmtStr);		
			SetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_I2CInLen,InLen,FALSE);		
		}
	}

	DbgPrint("I2C_WriteRead %s.Write:%dB,Read:%dB ,Time %.3fS",RetVal?"succ":"failure",OutLen,InLen,UseT/1000);

	return RetVal;
}

//使能操作按钮，需先打开和配置JTAG，否则无法操作
VOID EnableButtonEnable()
{
	if(!DevIsOpened)
	{
		SpiIsCfg = FALSE;
		I2CIsCfg = FALSE;
	}
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_CMD_InitSPI),DevIsOpened);

	//更新打开/关闭设备按钮状态
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_OpenDevice),!DevIsOpened);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_CloseDevice),DevIsOpened);

	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_ObjList),!DevIsOpened);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_RefreshObjList),!DevIsOpened);

	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_CMD_SPICsCtrl),SpiIsCfg);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_FlashIdentify),SpiIsCfg);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_FlashRead),SpiIsCfg);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_FlashWrite),SpiIsCfg);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_FlashErase),SpiIsCfg);

	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_ReadToFile),SpiIsCfg);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_WriteFormFile),SpiIsCfg);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_FlashVerify),SpiIsCfg);
	
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_CMD_SPIStream),SpiIsCfg);	
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_CMD_BulkSpiIn),SpiIsCfg);	
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_CMD_BulkSpiOut),SpiIsCfg);	
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_CMD_I2C_RW),I2CIsCfg);

	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_SetGpio),DevIsOpened);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_GetGpio),DevIsOpened);

	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_EnableIntNotify),DevIsOpened);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_DisableIntNotify),DevIsOpened);
}

//显示设备信息
BOOL ShowDevInfor()
{
	ULONG ObjSel;
	CHAR  FmtStr[128]="";

	ObjSel = SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_ObjList,CB_GETCURSEL,0,0);
	if(ObjSel!=CB_ERR)
	{
		sprintf(FmtStr,"**ChipMode:%d,%s,Ver:%02X,DevID:%s",SpiI2cDevInfor[ObjSel].ChipMode,SpiI2cDevInfor[ObjSel].UsbSpeedType?"HS":"FS",SpiI2cDevInfor[ObjSel].FirewareVer,SpiI2cDevInfor[ObjSel].DeviceID);
		SetDlgItemText(SpiI2cGpioDebugHwnd,IDC_DevInfor,FmtStr);
	}	

	return (ObjSel!=CB_ERR);
}

//枚举设备
ULONG EnumDevice()
{
	ULONG i,oLen,DevCnt = 0;
	USB_DEVICE_DESCRIPTOR DevDesc = {0};
	CHAR tem[256] = "";
	mDeviceInforS DevInfor = {0};

	UCHAR iDriverVer = 0;
	UCHAR iDLLVer = 0;
	UCHAR ibcdDevice = 0;
	UCHAR iChipType = 0;
	SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_ObjList,CB_RESETCONTENT,0,0);
	for(i=0;i<16;i++)
	{
		if(CH347OpenDevice(i) != INVALID_HANDLE_VALUE)
		{
			oLen = sizeof(USB_DEVICE_DESCRIPTOR);
			CH347GetDeviceInfor(i,&DevInfor);
			
			DbgPrint("M:%s\nP:%s\n", DevInfor.ManufacturerString, DevInfor.ProductString);//SN:%s\n

			if(DevInfor.ChipMode == 3) //模式3此接口为JTAG/I2C
				continue;

			sprintf(tem,"%d# %s",i,DevInfor.FuncDescStr);
			SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_ObjList,CB_ADDSTRING,0,(LPARAM)(LPCTSTR)tem);		
			memcpy(&SpiI2cDevInfor[DevCnt],&DevInfor,sizeof(DevInfor));
			DevCnt++;

			CH347GetVersion(i, &iDriverVer, &iDLLVer, &ibcdDevice, &iChipType);
			DbgPrint("DV:%02x  DLLV:%02x BCD:%02x CT:%02x.\n", iDriverVer, iDLLVer, ibcdDevice, iChipType);
		} 
		CH347CloseDevice(i);
	}
	if(DevCnt)
	{
		SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_ObjList,CB_SETCURSEL,0,0);
		SetFocus(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_ObjList));
	}
	return DevCnt;
}

//初始化窗体
VOID InitWindows()
{	
	//查找并显示设备列表 
	SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_RefreshObjList,BM_CLICK,0,0);	
	
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_OpenDevice),!DevIsOpened);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_CloseDevice),DevIsOpened);
	//Flash地址斌初值
	SetDlgItemText(SpiI2cGpioDebugHwnd,IDC_FlashStartAddr,"0");
	//Flash操作数斌初值
	SetDlgItemText(SpiI2cGpioDebugHwnd,IDC_FlashDataSize,"100");
	//清空Flash数据模式
	SetDlgItemText(SpiI2cGpioDebugHwnd,IDC_FlashData,"");
	//输出框设置显示的最大字符数
	SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_InforShow,EM_LIMITTEXT,0xFFFFFFFF,0);

	return;
}

BOOL APIENTRY DlgProc_SpiUartI2cDbg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	ULONG ThreadID;
	CHAR tem[64]="",i;

	switch (message)
	{
		case WM_INITDIALOG:
		{
			SpiI2cGpioDebugHwnd = hWnd;
			AfxActiveHwnd = hWnd;	
			// Seed the random-number generator with current time so that
			// the numbers will be different every time we run.		
			{//添加alt+tab切换时显示的图标
				HICON hicon;
				hicon = (HICON)LoadIcon(AfxMainIns,(LPCTSTR)IDI_Main);
				PostMessage(SpiI2cGpioDebugHwnd,WM_SETICON,ICON_BIG,(LPARAM)(HICON)hicon);
				PostMessage(SpiI2cGpioDebugHwnd,WM_SETICON,ICON_SMALL,(LPARAM)(HICON)hicon);
			}
			DevIsOpened = FALSE;		
			InitWindows(); //初始化窗体
			{
				SendDlgItemMessage(hWnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode0");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode1");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode2");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode3");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_Mode,CB_SETCURSEL,3,0);
			}
			{//0=60MHz, 1=30MHz, 2=15MHz, 3=7.5MHz, 4=3.75MHz, 5=1.875MHz, 6=937.5KHz，7=468.75KHz
				SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"60MHz");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"30MHz");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"15MHz");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"7.5MHz");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"3.75MHz");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"1.875MHz");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"937.5KHz");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"468.75KHz");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_Clock,CB_SETCURSEL,1,0);
			}
			{//0=低位在前(LSB), 1=高位在前(MSB)
				SendDlgItemMessage(hWnd,IDC_SpiCfg_ByteBitOrder,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"LSB");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_ByteBitOrder,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"MSB");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_ByteBitOrder,CB_SETCURSEL,1,0);
			}
			{
				SendDlgItemMessage(hWnd,IDC_SpiCfg_ChipIndex,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"CS1");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_ChipIndex,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"CS2");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_ChipIndex,CB_SETCURSEL,0,0);
			}
			{
				SendDlgItemMessage(hWnd,IDC_SpiCsStatus,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"CS Active");
				SendDlgItemMessage(hWnd,IDC_SpiCsStatus,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"CS Deactive");
				SendDlgItemMessage(hWnd,IDC_SpiCsStatus,CB_SETCURSEL,0,0);
			}
			{
				SendDlgItemMessage(hWnd,IDC_SpiCfg_CS1Polarity,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"CS1 POLA_LOW");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_CS1Polarity,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"CS1 POLA_HIGH");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_CS1Polarity,CB_SETCURSEL,0,0);
			}
			{
				SendDlgItemMessage(hWnd,IDC_SpiCfg_CS2Polarity,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"CS2 POLA_LOW");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_CS2Polarity,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"CS2 POLA_HIGH");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_CS2Polarity,CB_SETCURSEL,0,0);
			}
			SetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiCfg_OutInIntervalT,0,FALSE);
			SetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiCfg_ActiveDelay,0,FALSE);
			SetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_SpiCfg_DelayDeactive,0,FALSE);
			CheckDlgButton(SpiI2cGpioDebugHwnd,IDC_EnableCS,BST_CHECKED);	
			CheckDlgButton(SpiI2cGpioDebugHwnd,IDC_EnablePnPAutoOpen,BST_CHECKED);	

			GpioEnableAll();

			//int0 pin sel
			for(i=0;i<8;i++)
			{
				sprintf(tem,"Gpio%d",i);
				SendDlgItemMessage(hWnd,IDC_Int0PinSel,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)tem);
				SendDlgItemMessage(hWnd,IDC_Int1PinSel,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)tem);
			}
			SendDlgItemMessage(hWnd,IDC_Int0PinSel,CB_SETCURSEL,0,0);
			SendDlgItemMessage(hWnd,IDC_Int1PinSel,CB_SETCURSEL,1,0);
			CheckDlgButton(hWnd,IDC_Int0Enable,BST_CHECKED);

			//00:下降沿触发; 01:上升沿触发; 02:双边沿触发;
			SendDlgItemMessage(hWnd,IDC_Int0TrigMode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"下降沿触发");
			SendDlgItemMessage(hWnd,IDC_Int0TrigMode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"上升沿触发");
			SendDlgItemMessage(hWnd,IDC_Int0TrigMode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"双边沿触发");
			SendDlgItemMessage(hWnd,IDC_Int0TrigMode,CB_SETCURSEL,0,0);
			SendDlgItemMessage(hWnd,IDC_Int1TrigMode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"下降沿触发");
			SendDlgItemMessage(hWnd,IDC_Int1TrigMode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"上升沿触发");
			SendDlgItemMessage(hWnd,IDC_Int1TrigMode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"双边沿触发");
			SendDlgItemMessage(hWnd,IDC_Int1TrigMode,CB_SETCURSEL,1,0);			

			SetEditlInputMode(hWnd,IDC_SpiOut,1);		
			SetEditlInputMode(hWnd,IDC_SpiIn,1);	
			SetEditlInputMode(hWnd,IDC_I2COut,1);		
			SetEditlInputMode(hWnd,IDC_I2CIn,1);
			
			// 初始化I2C相关控件内容
			{
			SendDlgItemMessage(hWnd,IDC_I2CCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"20KHz");
			SendDlgItemMessage(hWnd,IDC_I2CCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"100KHz");
			SendDlgItemMessage(hWnd,IDC_I2CCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"400KHz");
			SendDlgItemMessage(hWnd,IDC_I2CCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"750KHz");
			SendDlgItemMessage(hWnd,IDC_I2CCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"50KHz");
			SendDlgItemMessage(hWnd,IDC_I2CCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"200KHz");
			SendDlgItemMessage(hWnd,IDC_I2CCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"1MHz");
			SendDlgItemMessage(hWnd,IDC_I2CCfg_Clock,CB_SETCURSEL,1,0);

			SendDlgItemMessage(hWnd,IDC_I2CCfg_SclStretch,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Enable");
			SendDlgItemMessage(hWnd,IDC_I2CCfg_SclStretch,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Disable");
			SendDlgItemMessage(hWnd,IDC_I2CCfg_SclStretch,CB_SETCURSEL,1,0);

			SetDlgItemInt(hWnd,IDC_I2CCfg_Delay, 0, 0);
			}

			// 独立设置SPI时钟频率
			{
			SendDlgItemMessage(hWnd,IDC_SpiCfg_frequencyUnit,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"MHz");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_frequencyUnit,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"KHz");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_frequencyUnit,CB_SETCURSEL,0,0);
			}
			
			// 设置SPI数据位 8bit : 16bit
			{
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Databits,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"8bit");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Databits,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"16bit");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Databits,CB_SETCURSEL,0,0);
			}

			EnableButtonEnable();	
			//为USB2.0JTAG设备设置插入和拔出的通知.插入后自动打开设备,拔出后关闭设备
			if(CH347SetDeviceNotify(SpiI2cGpioDevIndex,CH347DevID, UsbDevPnpNotify) )       //设备插拔通知回调函数
			{
				if (g_isChinese)
					DbgPrint("已开启USB设备插拔监视");
				else 
					DbgPrint("USB device plug monitoring has been turned on");
			}
		}
			break;	
	case WM_CH347DevArrive:
		{
			if (g_isChinese)
				DbgPrint("****发现CH347设备插入USB口,打开设备");			
			else 
				DbgPrint("****Found that the ch347 device was inserted into the USB port, and opened the device");
			Sleep(100);	//检测到设备插入状态之后到设备枚举会有一定的滞后，不同系统环境存在差异，此处延时100ms
			//SPI/I2C Debug窗体		
			if(AfxActiveHwnd==SpiI2cGpioDebugHwnd)
			{				
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //先枚举USB设备		
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_OpenDevice,BM_CLICK,0,0); //打开设备
				if (g_isChinese)
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus,"设备插入");
				else 
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus,"Device insertion");
			}
			//Flash/Eeprom Debug窗体			
			if(AfxActiveHwnd==FlashEepromDbgHwnd)
			{				
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //先枚举USB设备		
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen_Flash)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_OpenDevice,BM_CLICK,0,0); //打开设备
				if (g_isChinese)
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Flash,"设备插入");
				else 
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Flash,"Device insertion");
			}
			//Jtag Debug窗体
			if(AfxActiveHwnd==JtagDlgHwnd)
			{				
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //先枚举USB设备		
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen_Jtag)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_OpenDevice,BM_CLICK,0,0); //打开设备
				if (g_isChinese)
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Jtag,"设备插入");
				else 
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Jtag,"Device insertion");
			}
			//Uart Debug窗体
			if(AfxActiveHwnd==UartDebugHwnd)
			{				
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //先枚举USB设备		
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen_Uart)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_OpenDevice,BM_CLICK,0,0); //打开设备
				if (g_isChinese)
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Uart,"设备插入");
				else 
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Uart,"Device insertion");
			}
		}
		break;
	case WM_CH347DevRemove:	
		{
			//关闭设备
			if (g_isChinese)
				DbgPrint("****发现CH347已从USB口移除,关闭设备");			
			else 
				DbgPrint("****It is found that ch347 has been removed from the USB port. Close the device");
			//SPI/I2C Debug窗体		
			if(AfxActiveHwnd==SpiI2cGpioDebugHwnd)
			{
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_CloseDevice,BM_CLICK,0,0); //打开设备
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //先枚举USB设备
				if (g_isChinese)
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus,"设备移除");
				else
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus,"Device removal");
			}
			//Flash/Eeprom Debug窗体		
			if(AfxActiveHwnd==FlashEepromDbgHwnd)
			{
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen_Flash)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_CloseDevice,BM_CLICK,0,0); //打开设备
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //先枚举USB设备		
				if (g_isChinese)
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Flash,"设备移除");
				else
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Flash,"Device removal");
			}
			//Jtag Debug窗体
			if(AfxActiveHwnd==JtagDlgHwnd)
			{
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen_Jtag)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_CloseDevice,BM_CLICK,0,0); //打开设备
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //先枚举USB设备
				if (g_isChinese)
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Jtag,"设备移除");
				else
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Jtag,"Device removal");
			}
			//Uart Debug窗体
			if(AfxActiveHwnd==UartDebugHwnd)
			{
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen_Uart)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_CloseDevice,BM_CLICK,0,0); //打开设备
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //先枚举USB设备
				if (g_isChinese)
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Uart,"设备移除");
				else
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Uart,"Device removal");
			}
		}
		break;
	case WM_CH347Int:
		IntService(wParam);
		break;
	case WM_NOTIFY:
		{
			if(((LPNMHDR)lParam)->code == PSN_SETACTIVE)
				AfxActiveHwnd = hWnd;
		}
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		// Parse the menu selections:
		switch (wmId)
		{
		case IDC_RefreshObjList:
			EnumDevice();   //枚举并显示设备
			break;
		case IDC_ObjList:
			ShowDevInfor();
			break;
		case IDC_EnablePnPAutoOpen:
			EnablePnPAutoOpen = (IsDlgButtonChecked(hWnd,IDC_EnablePnPAutoOpen)==BST_CHECKED);
			break;
		case IDC_OpenDevice://打开设备
			OpenDevice();
			EnableButtonEnable();	//更新按钮状态		
			break;
		case IDC_CloseDevice:			
			CloseDevice();				
			EnableButtonEnable();	//更新按钮状态			
			break;	
		case IDC_FlashIdentify:
			FlashIdentify();
			break;
		case IDC_FlashRead:
			FlashBlockRead();
			break;
		case IDC_FlashWrite://将IDC_FLASHDATA框内数据写入FLASH
			FlashBlockWrite();
			break;
		case IDC_FlashErase:
			FlashBlockErase();
			break;	
		case IDC_CMD_InitSPI:			
			SpiIsCfg = CH347InitSpi();
			// CH347I2C_Set(SpiI2cGpioDevIndex,3); //配置I2C速度为快速750K
			EnableButtonEnable();
			break;
		case IDC_I2CInit:
			I2CIsCfg = CH347InitI2C();			// 单独的初始化I2C操作
			EnableButtonEnable();
			break;
		case IDC_CMD_SPICsCtrl:
			CH347SpiCsCtrl();
			EnableButtonEnable();
			break;
		case IDC_CMD_SPIStream:
			CH347SpiStream(0xC2);
			break;
		case IDC_CMD_BulkSpiIn:
			CH347SpiStream(0xC3);
			break;
		case IDC_CMD_BulkSpiOut:
			CH347SpiStream(0xC4);
			break;
		case IDC_FlashVerify:
			CloseHandle(CreateThread(NULL,0,FlashVerifyWithFile,NULL,0,&ThreadID)); //开始USB下载
			break;
		case IDC_WriteFormFile:
			CloseHandle(CreateThread(NULL,0,WriteFlashFromFile,NULL,0,&ThreadID)); //开始USB下载
			break;
		case IDC_ReadToFile:
			CloseHandle(CreateThread(NULL,0,ReadFlashToFile,NULL,0,&ThreadID)); //开始USB下载
			break;
		case IDC_FlashRWSpeedTest://读写测速
			CloseHandle(CreateThread(NULL,0,FlashRWSpeedTest,NULL,0,&ThreadID)); //开始USB下载
			break;		
		case IDC_CMD_I2C_RW:
			I2C_WriteRead();
			break;
		case IDC_SetGpio://GPIO设置
			Gpio_Set();
			break;
		case IDC_GetGpio://GPIO状态获取
			Gpio_Get();
			break;
		case IDC_GpioSetDataAll://选中全部GPIO电平
			GpioSetDataAll();
			break;
		case IDC_GpioEnableAll://使能全部GPIO
			GpioEnableAll();
			break;
		case IDC_GpioSetDirAll://选中全部GPIO方向
			GpioSetDirAll();
			break;
		case IDC_EnableIntNotify:
			IntEnable(); //开启中断
			break;
		case IDC_DisableIntNotify:
			IntDisable(); //关闭中断
			break;
		case IDC_ClearInfor:
			SetDlgItemText(hWnd,IDC_InforShow,"");
			break;
		case WM_DESTROY:			
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;		
		case WM_DESTROY:
			SendDlgItemMessage(hWnd,IDC_CloseDevice,BM_CLICK,0,0);
			CH347SetDeviceNotify(SpiI2cGpioDevIndex,CH347DevID,NULL);
			PostQuitMessage(0);
			break;		
	}
	return 0;
}
