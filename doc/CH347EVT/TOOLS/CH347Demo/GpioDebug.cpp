/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2023                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  ����CH347 GPIO�ӿں�������Ӧ��ʾ��

Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
Revision History:
  4/3/2022: TECH30
--*/


#include "Main.h"
#include "EepromDebug.h"

//ȫ�ֱ���
extern HWND SpiI2cGpioDebugHwnd;     //������
extern BOOL DevIsOpened;   //�豸�Ƿ��
extern BOOL SpiIsCfg;
extern ULONG SpiI2cGpioDevIndex;

extern BOOL IntIsEnable;

//GPIOʹ�ܿؼ�ID
ULONG GpioEnCtrID[8] = {IDC_EnSet_Gpio0,IDC_EnSet_Gpio1,IDC_EnSet_Gpio2,IDC_EnSet_Gpio3,IDC_EnSet_Gpio4,IDC_EnSet_Gpio5,IDC_EnSet_Gpio6,IDC_EnSet_Gpio7};
//GPIO����ؼ�ID
ULONG GpioDirCtrID[8] = {IDC_Dir_Gpio0,IDC_Dir_Gpio1,IDC_Dir_Gpio2,IDC_Dir_Gpio3,IDC_Dir_Gpio4,IDC_Dir_Gpio5,IDC_Dir_Gpio6,IDC_Dir_Gpio7};
//GPIO��ƽ�ؼ�ID
ULONG GpioStaCtrID[8] = {IDC_Val_Gpio0,IDC_Val_Gpio1,IDC_Val_Gpio2,IDC_Val_Gpio3,IDC_Val_Gpio4,IDC_Val_Gpio5,IDC_Val_Gpio6,IDC_Val_Gpio7};

//GPIO����
BOOL Gpio_Set()
{
	ULONG i;
	UCHAR oEnable = 0;       //������Ч��־:��Ӧλ0-7,��ӦGPIO0-7.
	UCHAR oSetDirOut = 0;    //����I/O����,ĳλ��0���Ӧ����Ϊ����,ĳλ��1���Ӧ����Ϊ���.GPIO0-7��Ӧλ0-7.
	UCHAR oSetDataOut = 0;   //�������,���I/O����Ϊ���,��ôĳλ��0ʱ��Ӧ��������͵�ƽ,ĳλ��1ʱ��Ӧ��������ߵ�ƽ
	BOOL  RetVal;

	for(i=0;i<8;i++)
	{
		//ȡʹ��λ
		if( IsDlgButtonChecked(SpiI2cGpioDebugHwnd,GpioEnCtrID[i])==BST_CHECKED )
			oEnable |= (1<<i);
		//ȡ����
		if( IsDlgButtonChecked(SpiI2cGpioDebugHwnd,GpioDirCtrID[i])==BST_CHECKED )
			oSetDirOut |= (1<<i);
		//ȡ��ƽֵ
		if( IsDlgButtonChecked(SpiI2cGpioDebugHwnd,GpioStaCtrID[i])==BST_CHECKED )
			oSetDataOut |= (1<<i);
	}
	RetVal = CH347GPIO_Set(SpiI2cGpioDevIndex,oEnable,oSetDirOut,oSetDataOut);
	DbgPrint("CH347GPIO_SetOutput %s,oEnable:%02X,oSetDirOut:%02X,oSetDataOut:%02X",RetVal?"Succ":"Fail",oEnable,oSetDirOut,oSetDataOut);

	return RetVal;
}

//GPIO״̬��ȡ
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
			//��ʾ����
			Sel = (iDir&(1<<i))?BST_CHECKED:BST_UNCHECKED;
			CheckDlgButton(SpiI2cGpioDebugHwnd,GpioDirCtrID[i],Sel);				
			//��ƽֵ
			Sel = (iData&(1<<i))?BST_CHECKED:BST_UNCHECKED;
			CheckDlgButton(SpiI2cGpioDebugHwnd,GpioStaCtrID[i],Sel);
		}
	}
	return RetVal;
}

//ʹ��ȫ��GPIO
VOID GpioEnableAll()
{
	BOOL Sel;
	UCHAR i;

	Sel = IsDlgButtonChecked(SpiI2cGpioDebugHwnd,IDC_GpioEnableAll);
	for(i=0;i<8;i++)
		CheckDlgButton(SpiI2cGpioDebugHwnd,GpioEnCtrID[i],Sel);

	return;
}

//ѡ��ȫ��GPIO����
VOID GpioSetDirAll()
{
	BOOL Sel;
	UCHAR i;

	Sel = IsDlgButtonChecked(SpiI2cGpioDebugHwnd,IDC_GpioSetDirAll);
	for(i=0;i<8;i++)
		CheckDlgButton(SpiI2cGpioDebugHwnd,GpioDirCtrID[i],Sel);

	return;
}

//ѡ��ȫ��GPIO��ƽ
VOID GpioSetDataAll()
{
	BOOL Sel;
	UCHAR i;

	Sel = IsDlgButtonChecked(SpiI2cGpioDebugHwnd,IDC_GpioSetDataAll);
	for(i=0;i<8;i++)
		CheckDlgButton(SpiI2cGpioDebugHwnd,GpioStaCtrID[i],Sel);

	return;
}

//CH347�ж���Ӧ����
BOOL IntService(ULONG IntTrigGpioN)
{
	ULONG i;
	for(i=0;i<8;i++)
	{
		if( IntTrigGpioN & (1<<i) )//�ж�GPIO�ź�
			DbgPrint("Gpio%d interrupt(%X)",i,IntTrigGpioN);
	}
	return TRUE;
}

// �жϻص�������
// ����Ϊ8���ֽڣ�ÿ���ֽڶ�Ӧһ��GPIO״̬��λ��������
//λ7����ǰ��GPIO0����0�����룻1��������ж�ģʽ�±���Ϊ0��
//λ6����ǰ��GPIO0�жϱ��أ�0���½��أ�1�������أ�
//λ5����ǰ��GPIO0�Ƿ�����Ϊ�жϣ�0����ѯģʽ��1���ж�ģʽ���ж�ģʽ�±���Ϊ1��
//λ4��������
//λ3����ǰ��GPIO0�ж�״̬��0��δ������1��������
VOID	CALLBACK	CH347_INT_ROUTINE(PUCHAR			iStatus )  // �ж�״̬����,�ο������λ˵��
{
	ULONG IntTrigGpioN = 0,i;
	for(i=0;i<8;i++)
	{
		if( iStatus[i]&0x08 ) //�ж�GPIO�ź�
			IntTrigGpioN |= (1<<i);			
	}
	PostMessage(SpiI2cGpioDebugHwnd,WM_CH347Int,IntTrigGpioN,0);
}

//�����ж�
BOOL IntEnable()
{
	BOOL Int0Enable,Int1Enable;
	UCHAR Int0PinN = 8,Int1PinN = 8;
	UCHAR Int0TrigMode=0x03,Int1TrigMode=0x03;

	//�����ظ�����	
	SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_DisableIntNotify,BM_CLICK,0,0);

	Int0Enable = (IsDlgButtonChecked(SpiI2cGpioDebugHwnd,IDC_Int0Enable)==BST_CHECKED);
	if(Int0Enable)
	{
		//�ж�0 GPIO���ź�,����7:�����ô��ж�Դ; Ϊ0-7��Ӧgpio0-7
		Int0PinN = (UCHAR)SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_Int0PinSel,CB_GETCURSEL,0,0);
		//�ж�0����: 00:�½��ش���; 01:�����ش���; 02:˫���ش���; 03:����;
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
		//�ж�1 GPIO���ź�,����7�����ô��ж�Դ,Ϊ0-7��Ӧgpio0-7
		Int1PinN = (UCHAR)SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_Int1PinSel,CB_GETCURSEL,0,0);
		//�ж�1����: 00:�½��ش���; 01:�����ش���; 02:˫���ش���; 03:����;
		Int1TrigMode = (UCHAR)SendDlgItemMessage(SpiI2cGpioDebugHwnd,IDC_Int1TrigMode,CB_GETCURSEL,0,0);
	}
	else
	{
		Int1PinN = 0xFF;
		Int1TrigMode = 0x03;
	}
	//ָ���жϷ������,ΪNULL��ȡ���жϷ���,�������ж�ʱ���øó���
	IntIsEnable = CH347SetIntRoutine(SpiI2cGpioDevIndex,Int0PinN,Int0TrigMode,Int1PinN,Int1TrigMode,CH347_INT_ROUTINE );
	DbgPrint("CH347 interrupt notify routine set %s ",IntIsEnable?"succ":"false");

	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_EnableIntNotify),!IntIsEnable);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_DisableIntNotify),IntIsEnable);

	return TRUE;
}

//�ر��ж�
BOOL IntDisable()
{
	{
		//ָ���жϷ������,ΪNULL��ȡ���жϷ���,�������ж�ʱ���øó���
		IntIsEnable = CH347SetIntRoutine(SpiI2cGpioDevIndex,0xFF,0xFF,0xFF,0xFF,NULL );
		DbgPrint("CH347 interrupt notify routine cancell %s ",IntIsEnable?"succ":"false");

		IntIsEnable = FALSE;
	}
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_EnableIntNotify),TRUE);
	EnableWindow(GetDlgItem(SpiI2cGpioDebugHwnd,IDC_DisableIntNotify),FALSE);

	return TRUE;
}