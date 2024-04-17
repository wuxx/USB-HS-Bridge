/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2023                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  CH347 SPI/I2C�ӿ�����������

Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
Revision History:
  4/3/2022: TECH30
--*/

#include "Main.h"

#define WM_CH347DevArrive WM_USER+10         //�豸����֪ͨ�¼�,������̽���
#define WM_CH347DevRemove WM_USER+11         //�豸�γ�֪ͨ�¼�,������̽���

#define CH347DevID "VID_1A86&PID_55D\0"  //����CH347 USB��ζ���,����ʱ���Զ����豸;�ر�ʱ�Զ��ر��豸

//����׼ȷ����ģʽ�´��ڲ�ζ�������д��������USBID����DEMO����������ģʽ������ֻ��ȡID��ͬ����
//����ָ���豸��USB��μ��: CH347SetDeviceNotify(,USBID_Mode???,)
//ȡ��ָ���豸��USB��μ��: CH347SetDeviceNotify(,USBID_Mode???,)
//#define USBID_VCP_Mode0_UART0       "VID_1A86&PID_55DA&MI_00\0"  //MODE0 UART0
//#define USBID_VCP_Mode0_UART1       "VID_1A86&PID_55DA&MI_01\0"  //MODE0 UART1
//#define USBID_VCP_Mode0_UART        "VID_1A86&PID_55DA\0"        //MODE0 UART
//#define USBID_VEN_Mode1_UART1       "VID_1A86&PID_55DB&MI_00\0"  //MODE1 UART
//#define USBID_HID_Mode2_UART1       "VID_1A86&PID_55DB&MI_00\0"  //MODE2 UART
//#define USBID_VEN_Mode3_UART1       "VID_1A86&PID_55DB&MI_00\0"  //MODE3 UART

//����׼ȷ����ģʽ�½ӿڲ�ζ�������д��������USBID����DEMO����������ģʽ������ֻ��ȡID��ͬ����
//����ָ���豸��USB��μ��: CH347Uart_SetDeviceNotify(,USBID_Mode???,)
//ȡ��ָ���豸��USB��μ��: CH347Uart_SetDeviceNotify(,USBID,)
//#define USBID_VEN_Mode1_SPI_I2C     "VID_1A86&PID_55DA&MI_00\0"  //MODE1 SPI/I2C
//#define USBID_HID_Mode2_SPI_I2C     "VID_1A86&PID_55DA&MI_00\0"  //MODE2 SPI/I2C
//#define USBID_VEN_Mode3_JTAG_I2C    "VID_1A86&PID_55DA&MI_00\0"  //MODE3 JTAG/I2C

extern BOOL g_isChinese;
extern HINSTANCE AfxMainIns; //����ʵ�� 
extern HWND AfxActiveHwnd;
extern HWND JtagDlgHwnd;
extern HWND FlashEepromDbgHwnd;
extern HWND UartDebugHwnd;
extern BOOL EnablePnPAutoOpen_Jtag; //���ò�κ��豸�Զ��򿪹رչ���
extern BOOL EnablePnPAutoOpen_Uart; //���ò�κ��豸�Զ��򿪹رչ���
extern BOOL EnablePnPAutoOpen_Flash; //���ò�κ��豸�Զ��򿪹رչ���


//ȫ�ֱ���
HWND SpiI2cGpioDebugHwnd;     //������
BOOL DevIsOpened;   //�豸�Ƿ��
BOOL SpiIsCfg;
BOOL I2CIsCfg;
ULONG SpiI2cGpioDevIndex;
mDeviceInforS SpiI2cDevInfor[16] = {0};
BOOL EnablePnPAutoOpen; //���ò�κ��豸�Զ��򿪹رչ���
BOOL  IntIsEnable=FALSE; //�ж�ʹ��
// CH347�豸��μ��֪ͨ����.��ص������Ժ������������ƣ�ͨ��������Ϣת�Ƶ���Ϣ�������ڽ��д���
VOID	 CALLBACK	 UsbDevPnpNotify (ULONG iEventStatus ) 
{
	if(iEventStatus==CH347_DEVICE_ARRIVAL)// �豸�����¼�,�Ѿ�����
		PostMessage(SpiI2cGpioDebugHwnd,WM_CH347DevArrive,0,0);
	else if(iEventStatus==CH347_DEVICE_REMOVE)// �豸�γ��¼�,�Ѿ��γ�
		PostMessage(SpiI2cGpioDebugHwnd,WM_CH347DevRemove,0,0);
	return;
}

//���豸
BOOL OpenDevice()
{
	//��ȡ�豸���
	SpiI2cGpioDevIndex = SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_ObjList,CB_GETCURSEL,0,0);
	if(SpiI2cGpioDevIndex==CB_ERR)
	{
		if (g_isChinese)
			DbgPrint("���豸ʧ��,����ѡ���豸");
		else 
			DbgPrint("Failed to open the device, please select the device first");
		goto Exit; //�˳�
	}	
	DevIsOpened = (CH347OpenDevice(SpiI2cGpioDevIndex) != INVALID_HANDLE_VALUE);
	CH347SetTimeout(SpiI2cGpioDevIndex,500,500);
	DbgPrint(">>Open the device...%s",DevIsOpened?"Success":"Failed");
Exit:
	return DevIsOpened;
}

//�ر��豸
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
	DOUBLE SpiFrequency = 0;	// ���õ�Ƶ��
	UCHAR SpiDatabits = 0;	// ���õ�����λ Ĭ��0
	
	
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

	// ��ʹ�������������ʣ������CH347SPISetFrequency
	if (IsDlgButtonChecked(SpiI2cGpioDebugHwnd,IDC_SpiCfg_SetFreq) == BST_CHECKED)
	{
		// ����SPIƵ��
		
		GetDlgItemText(SpiI2cGpioDebugHwnd, IDC_SpiCfg_Frequency, Frequency, 10);
		SpiFrequency = atof(Frequency);
		DbgPrint("SpiFrequency %.2f\n", SpiFrequency);
		
		// ��ȡ���õ�λ
		if (SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_SpiCfg_frequencyUnit,CB_GETCURSEL,0,0))
			RetVal = CH347SPI_SetFrequency(SpiI2cGpioDevIndex, KHZ(SpiFrequency));
		else 
			RetVal = CH347SPI_SetFrequency(SpiI2cGpioDevIndex, MHZ(SpiFrequency));
	}

	// ����SPI ����λ
	SpiDatabits = (UCHAR)SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_SpiCfg_Databits,CB_GETCURSEL,0,0);
	RetVal = CH347SPI_SetDataBits(SpiI2cGpioDevIndex, SpiDatabits);

	RetVal = CH347SPI_Init(SpiI2cGpioDevIndex,&SpiCfg);

	DbgPrint("CH347SPI_Init %s",RetVal?"succ":"failure");
	
	return RetVal;
}

// ��Ӵ˴�����SPI��ʼ������
BOOL CH347InitI2C()
{
	BOOL RetVal = FALSE;
	BOOL isStrentch = FALSE;
	ULONG iMode;	// I2Cģʽ
	ULONG I2CDelayMs = 0;
	
	iMode = SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_I2CCfg_Clock,CB_GETCURSEL,0,0);
	isStrentch = SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_I2CCfg_SclStretch,CB_GETCURSEL,0,0) ? FALSE : TRUE ;
	I2CDelayMs = GetDlgItemInt(SpiI2cGpioDebugHwnd,IDC_I2CCfg_Delay,NULL,FALSE);
	
	RetVal = CH347I2C_Set(SpiI2cGpioDevIndex, iMode);
	DbgPrint("CH347I2C Set clock %s",RetVal ? "succ" : "failure");

	RetVal = CH347I2C_SetStretch(SpiI2cGpioDevIndex, isStrentch);
	DbgPrint("CH347 I2C set stetching %s",RetVal ? "succ" : "failure");
	
	if (I2CDelayMs > 0)
		RetVal = CH347I2C_SetDelaymS(SpiI2cGpioDevIndex, I2CDelayMs);
	
	DbgPrint("CH347InitI2C %s",RetVal ? "succ" : "failure");

	return RetVal;
}

BOOL CH347SpiCsCtrl()
{
	USHORT          iEnableSelect;      // �Ͱ�λΪCS1���߰�λΪCS2; �ֽ�ֵΪ1=����CS,Ϊ0=���Դ�CS����
	USHORT          iChipSelect;		// �Ͱ�λΪCS1���߰�λΪCS2;Ƭѡ���,0=L,1=H
	USHORT          iIsAutoDeativeCS;   // �Ͱ�λΪCS1���߰�λΪCS2;������ɺ��Ƿ��Զ�����Ƭѡ
    ULONG           iActiveDelay;		// �Ͱ�λΪCS1���߰�λΪCS2;����Ƭѡ��ִ�ж�д��������ʱʱ��
	ULONG           iDelayDeactive;		// �Ͱ�λΪCS1���߰�λΪCS2;����Ƭѡ��ִ�ж�д��������ʱʱ��
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
	CHAR FmtStr[4096*3*6] = "",ValStr[16]="",FmtStr2[4096*3];
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
		SpiInLen = SpiOutLen; //����ֽ����������ֽ������
		if(SpiOutLen<1)
		{
			DbgPrint("SPI read length not specified");
			return FALSE;
		}
		memcpy(InBuf,OutBuf,SpiOutLen);
		RetVal = CH347StreamSPI4(SpiI2cGpioDevIndex,ChipSelect,SpiOutLen,InBuf);
		sprintf(FmtStr,"Cmd%X(StreamSpi) %s.",CmdCode,RetVal?"succ":"failure");
	}
	else if(CmdCode == 0xC3)
	{
		if(SpiInLen<1)
		{
			DbgPrint("SPI read length not specified");
			return FALSE;
		}
		SpiOutLen = 0;
		RetVal = CH347SPI_Read(SpiI2cGpioDevIndex,ChipSelect,4,&SpiInLen,InBuf);
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
		{//��ӡ
			sprintf(&FmtStr[strlen(FmtStr)],"\r\n                    OutData(%d):",SpiOutLen);
			//16������ʾ
			for(i=0;i<SpiOutLen;i++)
			{			
				sprintf(&FmtStr[strlen(FmtStr)],"%02X ",OutBuf[i]);						
			}		
		}
		if(SpiInLen)
		{//��ӡ
			//memset(FmtStr,0,sizeof(FmtStr));
			sprintf(&FmtStr[strlen(FmtStr)],"\r\n                    InData (%d):",SpiInLen);
			//16������ʾ
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
		{//��ӡ
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

//ʹ�ܲ�����ť�����ȴ򿪺�����JTAG�������޷�����
VOID EnableButtonEnable()
{
	if(!DevIsOpened)
		SpiIsCfg = FALSE;

	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_CMD_InitSPI),DevIsOpened);

	//���´�/�ر��豸��ť״̬
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
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_CMD_I2C_RW),SpiIsCfg);

	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_SetGpio),DevIsOpened);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_GetGpio),DevIsOpened);

	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_EnableIntNotify),DevIsOpened);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_DisableIntNotify),DevIsOpened);
}

//��ʾ�豸��Ϣ
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

//ö���豸
ULONG EnumDevice()
{
	ULONG i,oLen,DevCnt = 0;
	USB_DEVICE_DESCRIPTOR DevDesc = {0};
	CHAR tem[256] = "";
	mDeviceInforS DevInfor = {0};

	SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_ObjList,CB_RESETCONTENT,0,0);
	for(i=0;i<16;i++)
	{
		if(CH347OpenDevice(i) != INVALID_HANDLE_VALUE)
		{
			oLen = sizeof(USB_DEVICE_DESCRIPTOR);
			CH347GetDeviceInfor(i,&DevInfor);

			if(DevInfor.ChipMode == 3) //ģʽ3�˽ӿ�ΪJTAG/I2C
				continue;

			sprintf(tem,"%d# %s",i,DevInfor.FuncDescStr);
			SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_ObjList,CB_ADDSTRING,0,(LPARAM)(LPCTSTR)tem);		
			memcpy(&SpiI2cDevInfor[DevCnt],&DevInfor,sizeof(DevInfor));
			DevCnt++;
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

//��ʼ������
VOID InitWindows()
{	
	//���Ҳ���ʾ�豸�б� 
	SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_RefreshObjList,BM_CLICK,0,0);	
	
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_OpenDevice),!DevIsOpened);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_CloseDevice),DevIsOpened);
	//Flash��ַ���ֵ
	SetDlgItemText(SpiI2cGpioDebugHwnd,IDC_FlashStartAddr,"0");
	//Flash���������ֵ
	SetDlgItemText(SpiI2cGpioDebugHwnd,IDC_FlashDataSize,"100");
	//���Flash����ģʽ
	SetDlgItemText(SpiI2cGpioDebugHwnd,IDC_FlashData,"");
	//�����������ʾ������ַ���
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
			{//���alt+tab�л�ʱ��ʾ��ͼ��
				HICON hicon;
				hicon = (HICON)LoadIcon(AfxMainIns,(LPCTSTR)IDI_Main);
				PostMessage(SpiI2cGpioDebugHwnd,WM_SETICON,ICON_BIG,(LPARAM)(HICON)hicon);
				PostMessage(SpiI2cGpioDebugHwnd,WM_SETICON,ICON_SMALL,(LPARAM)(HICON)hicon);
			}
			DevIsOpened = FALSE;		
			InitWindows(); //��ʼ������
			{
				SendDlgItemMessage(hWnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode0");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode1");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode2");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode3");
				SendDlgItemMessage(hWnd,IDC_SpiCfg_Mode,CB_SETCURSEL,3,0);
			}
			{//0=60MHz, 1=30MHz, 2=15MHz, 3=7.5MHz, 4=3.75MHz, 5=1.875MHz, 6=937.5KHz��7=468.75KHz
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
			{//0=��λ��ǰ(LSB), 1=��λ��ǰ(MSB)
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

			if (g_isChinese) 
			{
				//00:�½��ش���; 01:�����ش���; 02:˫���ش���;
				SendDlgItemMessage(hWnd,IDC_Int0TrigMode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"�½��ش���");
				SendDlgItemMessage(hWnd,IDC_Int0TrigMode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"�����ش���");
				SendDlgItemMessage(hWnd,IDC_Int0TrigMode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"˫���ش���");
				SendDlgItemMessage(hWnd,IDC_Int0TrigMode,CB_SETCURSEL,0,0);
				SendDlgItemMessage(hWnd,IDC_Int1TrigMode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"�½��ش���");
				SendDlgItemMessage(hWnd,IDC_Int1TrigMode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"�����ش���");
				SendDlgItemMessage(hWnd,IDC_Int1TrigMode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"˫���ش���");
				SendDlgItemMessage(hWnd,IDC_Int1TrigMode,CB_SETCURSEL,1,0);	
			} else
			{
				//00:Falling edge trigger; 01:Rising edge trigger; 02:Double edge trigger;
				SendDlgItemMessage(hWnd,IDC_Int0TrigMode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Falling edge trigger");
				SendDlgItemMessage(hWnd,IDC_Int0TrigMode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Rising edge trigger");
				SendDlgItemMessage(hWnd,IDC_Int0TrigMode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Double edge trigger");
				SendDlgItemMessage(hWnd,IDC_Int0TrigMode,CB_SETCURSEL,0,0);
				SendDlgItemMessage(hWnd,IDC_Int1TrigMode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Falling edge trigger");
				SendDlgItemMessage(hWnd,IDC_Int1TrigMode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Rising edge trigger");
				SendDlgItemMessage(hWnd,IDC_Int1TrigMode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Double edge trigger");
				SendDlgItemMessage(hWnd,IDC_Int1TrigMode,CB_SETCURSEL,1,0);	
			}		

			SetEditlInputMode(hWnd,IDC_SpiOut,1);		
			SetEditlInputMode(hWnd,IDC_SpiIn,1);	
			SetEditlInputMode(hWnd,IDC_I2COut,1);		
			SetEditlInputMode(hWnd,IDC_I2CIn,1);
			
			// ��ʼ��I2C��ؿؼ�����
			{
			SendDlgItemMessage(hWnd,IDC_I2CCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"20KHz");
			SendDlgItemMessage(hWnd,IDC_I2CCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"100KHz");
			SendDlgItemMessage(hWnd,IDC_I2CCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"400KHz");
			SendDlgItemMessage(hWnd,IDC_I2CCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"750KHz");
			SendDlgItemMessage(hWnd,IDC_I2CCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"50KHz");
			SendDlgItemMessage(hWnd,IDC_I2CCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"200KHz");
			SendDlgItemMessage(hWnd,IDC_I2CCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"1MHz");
			SendDlgItemMessage(hWnd,IDC_I2CCfg_Clock,CB_SETCURSEL,0,3);

			SendDlgItemMessage(hWnd,IDC_I2CCfg_SclStretch,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Enable");
			SendDlgItemMessage(hWnd,IDC_I2CCfg_SclStretch,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Disable");
			SendDlgItemMessage(hWnd,IDC_I2CCfg_SclStretch,CB_SETCURSEL,0,1);
			}

			// ��������SPIʱ��Ƶ��
			{
			SendDlgItemMessage(hWnd,IDC_SpiCfg_frequencyUnit,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"MHz");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_frequencyUnit,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"KHz");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_frequencyUnit,CB_SETCURSEL,0,0);
			}
			
			// ����SPI����λ 8bit : 16bit
			{
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Databits,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"8bit");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Databits,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"16bit");
			SendDlgItemMessage(hWnd,IDC_SpiCfg_Databits,CB_SETCURSEL,0,0);
			}

			EnableButtonEnable();	
			//ΪUSB2.0JTAG�豸���ò���Ͱγ���֪ͨ.������Զ����豸,�γ���ر��豸
			if(CH347SetDeviceNotify(SpiI2cGpioDevIndex,CH347DevID, UsbDevPnpNotify) )       //�豸���֪ͨ�ص�����
			{
				if (g_isChinese)
					DbgPrint("�ѿ���USB�豸��μ���");
				else 
					DbgPrint("USB device plug monitoring has been turned on");
			}
		}
			break;	
	case WM_CH347DevArrive:
		{
			if (g_isChinese)
				DbgPrint("****����CH347�豸����USB��,���豸");			
			else 
				DbgPrint("****Found that the ch347 device was inserted into the USB port, and opened the device");
			Sleep(100);	//��⵽�豸����״̬֮���豸ö�ٻ���һ�����ͺ󣬲�ͬϵͳ�������ڲ��죬�˴���ʱ100ms
			//SPI/I2C Debug����		
			if(AfxActiveHwnd==SpiI2cGpioDebugHwnd)
			{				
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //��ö��USB�豸		
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_OpenDevice,BM_CLICK,0,0); //���豸
				if (g_isChinese)
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus,"�豸����");
				else 
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus,"Device insertion");
			}
			//Flash/Eeprom Debug����			
			if(AfxActiveHwnd==FlashEepromDbgHwnd)
			{				
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //��ö��USB�豸		
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen_Flash)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_OpenDevice,BM_CLICK,0,0); //���豸
				if (g_isChinese)
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Flash,"�豸����");
				else 
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Flash,"Device insertion");
			}
			//Jtag Debug����
			if(AfxActiveHwnd==JtagDlgHwnd)
			{				
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //��ö��USB�豸		
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen_Jtag)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_OpenDevice,BM_CLICK,0,0); //���豸
				if (g_isChinese)
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Jtag,"�豸����");
				else 
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Jtag,"Device insertion");
			}
			//Uart Debug����
			if(AfxActiveHwnd==UartDebugHwnd)
			{				
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //��ö��USB�豸		
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen_Uart)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_OpenDevice,BM_CLICK,0,0); //���豸
				if (g_isChinese)
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Uart,"�豸����");
				else 
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Uart,"Device insertion");
			}
		}
		break;
	case WM_CH347DevRemove:	
		{
			//�ر��豸
			if (g_isChinese)
				DbgPrint("****����CH347�Ѵ�USB���Ƴ�,�ر��豸");			
			else 
				DbgPrint("****It is found that ch347 has been removed from the USB port. Close the device");
			//SPI/I2C Debug����		
			if(AfxActiveHwnd==SpiI2cGpioDebugHwnd)
			{
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_CloseDevice,BM_CLICK,0,0); //���豸
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //��ö��USB�豸
				if (g_isChinese)
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus,"�豸�Ƴ�");
				else
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus,"Device removal");
			}
			//Flash/Eeprom Debug����		
			if(AfxActiveHwnd==FlashEepromDbgHwnd)
			{
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen_Flash)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_CloseDevice,BM_CLICK,0,0); //���豸
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //��ö��USB�豸		
				if (g_isChinese)
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Flash,"�豸�Ƴ�");
				else
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Flash,"Device removal");
			}
			//Jtag Debug����
			if(AfxActiveHwnd==JtagDlgHwnd)
			{
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen_Jtag)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_CloseDevice,BM_CLICK,0,0); //���豸
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //��ö��USB�豸
				if (g_isChinese)
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Jtag,"�豸�Ƴ�");
				else
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Jtag,"Device removal");
			}
			//Uart Debug����
			if(AfxActiveHwnd==UartDebugHwnd)
			{
				if(IsDlgButtonChecked(AfxActiveHwnd,IDC_EnablePnPAutoOpen_Uart)==BST_CHECKED)
					SendDlgItemMessage(AfxActiveHwnd,IDC_CloseDevice,BM_CLICK,0,0); //���豸
				SendDlgItemMessage(AfxActiveHwnd,IDC_RefreshObjList,BM_CLICK,0,0); //��ö��USB�豸
				if (g_isChinese)
					SetDlgItemText(AfxActiveHwnd,IDC_PnPStatus_Uart,"�豸�Ƴ�");
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
			EnumDevice();   //ö�ٲ���ʾ�豸
			break;
		case IDC_ObjList:
			ShowDevInfor();
			break;
		case IDC_EnablePnPAutoOpen:
			EnablePnPAutoOpen = (IsDlgButtonChecked(hWnd,IDC_EnablePnPAutoOpen)==BST_CHECKED);
			break;
		case IDC_OpenDevice://���豸
			OpenDevice();
			EnableButtonEnable();	//���°�ť״̬		
			break;
		case IDC_CloseDevice:			
			CloseDevice();				
			EnableButtonEnable();	//���°�ť״̬			
			break;	
		case IDC_FlashIdentify:
			FlashIdentify();
			break;
		case IDC_FlashRead:
			FlashBlockRead();
			break;
		case IDC_FlashWrite://��IDC_FLASHDATA��������д��FLASH
			FlashBlockWrite();
			break;
		case IDC_FlashErase:
			FlashBlockErase();
			break;	
		case IDC_CMD_InitSPI:			
			SpiIsCfg = CH347InitSpi();
			// CH347I2C_Set(SpiI2cGpioDevIndex,3); //����I2C�ٶ�Ϊ����750K
			EnableButtonEnable();
			break;
		case IDC_I2CInit:
			I2CIsCfg = CH347InitI2C();			// �����ĳ�ʼ��I2C����
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
			CloseHandle(CreateThread(NULL,0,FlashVerifyWithFile,NULL,0,&ThreadID)); //��ʼUSB����
			break;
		case IDC_WriteFormFile:
			CloseHandle(CreateThread(NULL,0,WriteFlashFromFile,NULL,0,&ThreadID)); //��ʼUSB����
			break;
		case IDC_ReadToFile:
			CloseHandle(CreateThread(NULL,0,ReadFlashToFile,NULL,0,&ThreadID)); //��ʼUSB����
			break;
		case IDC_FlashRWSpeedTest://��д����
			CloseHandle(CreateThread(NULL,0,FlashRWSpeedTest,NULL,0,&ThreadID)); //��ʼUSB����
			break;		
		case IDC_CMD_I2C_RW:
			I2C_WriteRead();
			break;
		case IDC_SetGpio://GPIO����
			Gpio_Set();
			break;
		case IDC_GetGpio://GPIO״̬��ȡ
			Gpio_Get();
			break;
		case IDC_GpioSetDataAll://ѡ��ȫ��GPIO��ƽ
			GpioSetDataAll();
			break;
		case IDC_GpioEnableAll://ʹ��ȫ��GPIO
			GpioEnableAll();
			break;
		case IDC_GpioSetDirAll://ѡ��ȫ��GPIO����
			GpioSetDirAll();
			break;
		case IDC_EnableIntNotify:
			IntEnable(); //�����ж�
			break;
		case IDC_DisableIntNotify:
			IntDisable(); //�ر��ж�
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
