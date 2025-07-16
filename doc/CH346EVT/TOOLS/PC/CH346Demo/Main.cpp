/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2024                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  CH346 SPI & Parallel port demo
Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2024 Nanjing Qinheng Microelectronics Co., Ltd.
Revision History:
  8/3/2024: TECH
--*/
#define _CRT_SECURE_NO_WARNINGS
#include "Main.h"

//#define CH375_IF 1

#define WM_UsbDevArrive WM_USER+10         //�豸����֪ͨ�¼�,������̽���
#define WM_UsbDevRemove WM_USER+11         //�豸�γ�֪ͨ�¼�,������̽���

HWND MainHwnd;     //��������
BOOL g_isChinese;
HINSTANCE AfxMainIns; //����ʵ��
extern HWND AfxActiveHwnd;
BOOL DevIsOpened;  //�豸�Ƿ��
ULONG AfxDevIndex;
ULONGLONG TotalTxCnt=0,TotalRxCnt=0;
BOOL StopTxThread,StopRxThread;
mDeviceInforS AfxDevInfor[16] = {0};
BOOL AfxAutoRecvIsStart = FALSE;
BOOL AfxtAutoRecvToFile = FALSE,AfxAutoRecvShow=FALSE;
BOOL AfxEnableUploadBuf = FALSE,AfxEnableDnloadBuf = FALSE;
CHAR AfxDevUsbID[64] = "VID_1A86&PID_55EB\0";
ULONG AfxPlugCnt = 0,AfxRemoveCnt = 0;
BOOL AfxEnableRxDataCheck;
CHAR AfxRxFileName[MAX_PATH] = "",AfxTxFileName[MAX_PATH] = "";
ULONG AfxInEndpN = 1,AfxOutEndpN = 1; //CH375 IFʹ��

//ʹ�ܲ�����ť
VOID EnableButtonEnable();
BOOL APIENTRY DlgProc_CH346(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


//��ʼ���������ѡ��
VOID InitDlg()
{
	{
		SendDlgItemMessage(MainHwnd,IDC_WorkMode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode0: Uart+Parallel Slave");
		SendDlgItemMessage(MainHwnd,IDC_WorkMode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode1: Uart+SPI Slave");
		//SendDlgItemMessage(MainHwnd,IDC_WorkMode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode2: Uart0+Uart1");
		SendDlgItemMessage(MainHwnd,IDC_WorkMode,CB_SETCURSEL,0,0);
	}
	SetDlgItemText(MainHwnd,IDC_ReadLen,"4096");
	SetDlgItemText(MainHwnd,IDC_WriteLen,"512");
	SetDlgItemText(MainHwnd,IDC_UploadBufPktSize,"40960");
	SetDlgItemInt(MainHwnd,IDC_DnPktCnt,10,FALSE);
	EnableButtonEnable();
	{
		CHAR FmtStr[256*3+16] = "";
		ULONG i;
		for(i=0;i<256;i++)
			sprintf(&FmtStr[strlen(FmtStr)],"%02X ",i);
		SetDlgItemText(MainHwnd,IDC_WriteData,FmtStr);

		sprintf(FmtStr,"Tx:%I64u B",TotalTxCnt);
		SetDlgItemText(MainHwnd,IDC_TxStatistics,FmtStr);

		sprintf(FmtStr,"Rx:%I64u B",TotalRxCnt);
		SetDlgItemText(MainHwnd,IDC_RxStatistics,FmtStr);
	}
}

//ʹ�ܲ�����ť�����ȴ򿪺�����JTAG�������޷�����
VOID EnableButtonEnable()
{
	//���´�/�ر��豸��ť״̬
	EnableWindow(GetDlgItem(MainHwnd,IDC_CloseDevice),DevIsOpened);
	EnableWindow(GetDlgItem(MainHwnd,IDC_OpenDevice),!DevIsOpened);

	EnableWindow(GetDlgItem(MainHwnd,IDC_DevList),!DevIsOpened);
	EnableWindow(GetDlgItem(MainHwnd,IDC_RefreshDevList),!DevIsOpened);

	EnableWindow(GetDlgItem(MainHwnd,IDC_Write),DevIsOpened);
	EnableWindow(GetDlgItem(MainHwnd,IDC_BatchWrite),DevIsOpened);
	EnableWindow(GetDlgItem(MainHwnd,IDC_Read),DevIsOpened);

	EnableWindow(GetDlgItem(MainHwnd,IDC_ResetDevice),DevIsOpened);
	EnableWindow(GetDlgItem(MainHwnd,IDC_BeginAutoRx),DevIsOpened);	
}

// USB�豸��μ��֪ͨ����.��ص������Ժ������������ƣ�ͨ��������Ϣת�Ƶ���Ϣ�������ڽ��д���
VOID	 CALLBACK	 UsbDevPnpNotify (ULONG iEventStatus ) 
{
	if(iEventStatus==CH346_DEVICE_ARRIVAL)// �豸�����¼�,�Ѿ�����
		PostMessage(MainHwnd,WM_UsbDevArrive,0,0);
	else if(iEventStatus==CH346_DEVICE_REMOVE)// �豸�γ��¼�,�Ѿ��γ�
		PostMessage(MainHwnd,WM_UsbDevRemove,0,0);
	return;
}

//ö���豸
ULONG CH346Enum()
{
	ULONG i,DevCnt = 0;
	USB_DEVICE_DESCRIPTOR DevDesc = {0};
	CHAR tem[256] = "";
	mDeviceInforS DevInfor = {0};
	PCHAR EnumDevPath = NULL;

	SendDlgItemMessage(MainHwnd,IDC_DevList,CB_RESETCONTENT,0,0);	
	DbgPrint("");
	DbgPrint(">>Enum CH346 Device");
	for(i=0;i<16;i++)
	{
		//��ȡ�豸����
#ifdef CH375_IF
		EnumDevPath = (PCHAR)CH375GetDeviceName(i);
		//������ID�ж�
#else		
		EnumDevPath = (PCHAR)CH346GetDeviceName(i);		
#endif
		if(EnumDevPath == NULL )
			break;
		if( strlen(EnumDevPath) )
		{
			sprintf(tem,"  %d# %s",i,EnumDevPath);			
			SendDlgItemMessage(MainHwnd,IDC_DevList,CB_ADDSTRING,0,(LPARAM)(LPCTSTR)tem);
			DevCnt++;

			DbgPrint("%s",tem);
		}
	}
	if(DevCnt)
	{
		SendDlgItemMessage(MainHwnd,IDC_DevList,CB_SETCURSEL,DevCnt-1,0);
		SetFocus(GetDlgItem(MainHwnd,IDC_DevList));
	}
	DbgPrint("<<Total found %d CH346.",DevCnt);
	return DevCnt;
}


//���豸
BOOL OpenDevice()
{
	CHAR FmtStr[128] = "",SNString[128] = "";
	ULONG BufSize;

	//��ȡ�豸���
	AfxDevIndex = SendDlgItemMessage(MainHwnd,IDC_DevList,CB_GETCURSEL,0,0);
	if(AfxDevIndex==CB_ERR)
	{
		if (g_isChinese)
			DbgPrint("���豸ʧ��,����ѡ���豸");
		else 
			DbgPrint("Failed to open the device. Please select the device first");
		goto Exit; //�˳�
	}	
	if(DevIsOpened)
	{
		DbgPrint("Device is already opened.");
		goto Exit;
	}
#ifdef CH375_IF
	DevIsOpened = (CH375OpenDevice(AfxDevIndex) != INVALID_HANDLE_VALUE);
	CH375SetTimeoutEx(AfxDevIndex,1000,1000,1000,1000);	
	AfxInEndpN = 0x06;
	AfxOutEndpN = 0x06;
	DbgPrint("**>>**%d#Device opene %s...UploadEndp:%d,DownloadEndp:%d,ReadTimeout:%dms,WriteTimeout:%dms",
		AfxDevIndex,DevIsOpened?"Success":"Failed",
		AfxInEndpN,AfxOutEndpN,1000,1000);
#else
	DevIsOpened = (CH346OpenDevice(AfxDevIndex) != INVALID_HANDLE_VALUE);
	CH346SetTimeout(AfxDevIndex,1000,1000);
	CH346GetDeviceInfor(AfxDevIndex,&AfxDevInfor[AfxDevIndex]);		
	BufSize = sizeof(SNString);
	CH346GetSnString(AfxDevIndex,SNString,&BufSize);
	DbgPrint("**>>**%d#Device opene %s...UploadEndp:%d,DownloadEndp:%d,ReadTimeout:%dms,WriteTimeout:%dms,SN:%s",
		AfxDevIndex,DevIsOpened?"Success":"Failed",
		AfxDevInfor[AfxDevIndex].DataUpEndp,AfxDevInfor[AfxDevIndex].DataDnEndp,500,500,SNString);
#endif
	{//��ʼ���豸�����Ϣ		
		sprintf(FmtStr,"*Plug.%d | Remove.%d",AfxPlugCnt,AfxRemoveCnt);
		SetDlgItemText(MainHwnd,IDC_PnPStatus,FmtStr);
	}
	{//��ʾ�豸��Ϣ
		sprintf(FmtStr,"**ChipMode:%d,%s,DevID:%s,%s.%s,SerialNumber:%s",
			AfxDevInfor[AfxDevIndex].ChipMode,AfxDevInfor[AfxDevIndex].UsbSpeedType?"HS":"FS",
			AfxDevInfor[AfxDevIndex].DeviceID,
			AfxDevInfor[AfxDevIndex].ManufacturerString,AfxDevInfor[AfxDevIndex].ProductString,SNString);
		SetDlgItemText(MainHwnd,IDC_DevInfor,FmtStr);
	}

	StopTxThread = FALSE;
	StopRxThread = FALSE;
	TotalTxCnt = TotalRxCnt = 0;
	EnableButtonEnable(); //ʹ�ò�����ť
Exit:
	return DevIsOpened;
}

//�ر��豸
BOOL CloseDevice()
{
	StopTxThread = TRUE;
	StopRxThread = TRUE;
#ifdef CH375_IF
	if(AfxEnableDnloadBuf)
	{
		CH375SetBufDownloadEx(AfxDevIndex,0,AfxInEndpN,0);
		AfxEnableDnloadBuf = FALSE;
	}
	if(AfxEnableUploadBuf)
	{
		CH375SetBufUploadEx(AfxDevIndex,0,AfxInEndpN,0);
		AfxEnableUploadBuf = FALSE;
	}
	CH375CloseDevice(AfxDevIndex);
#else
	if(AfxEnableDnloadBuf)
	{
		CH346SetBufDownload(AfxDevIndex,0,0);
		AfxEnableDnloadBuf = FALSE;
	}
	if(AfxEnableUploadBuf)
	{
		CH346SetBufUpload(AfxDevIndex,0,0);
		AfxEnableUploadBuf = FALSE;
	}
	CH346CloseDevice(AfxDevIndex);
#endif
	DbgPrint("**<<**%d#Device is closed",AfxDevIndex);

	CheckDlgButton(MainHwnd,IDC_EnableBufDown,BST_UNCHECKED);
	CheckDlgButton(MainHwnd,IDC_EnableBufUpload,BST_UNCHECKED);	
	DevIsOpened = FALSE;
	AfxDevIndex = CB_ERR;
	EnableButtonEnable();

	return TRUE;
}

//ģʽ0:д��������;ģʽ1:дSPI����
BOOL CH346SP_Write()
{
	ULONG OutLen,i,StrLen;
	UCHAR OutBuf[4096] = "";
	CHAR FmtStr[4096*3*6] = "",ValStr[16]="";
	double BT,UseT;
	BOOL RetVal = FALSE;

	StrLen = GetDlgItemText(MainHwnd,IDC_WriteData,FmtStr,sizeof(FmtStr));	
	if(StrLen > 4096*3) //д
		StrLen = 4096*3;
	//��ȡʮ����������
	OutLen = 0;
	for(i=0;i<StrLen;i+=3)
	{		
		memcpy(&ValStr[0],&FmtStr[i],2);
		OutBuf[OutLen] = (UCHAR)mStrToHEX(ValStr);
		OutLen++;
	}
	SetDlgItemInt(MainHwnd,IDC_WriteLen,OutLen,FALSE);	
	
	if(OutLen<1)
	{
		if (g_isChinese)
			DbgPrint("���Ȳ���Ϊ��,��������Ч����.");
		else 
			DbgPrint("Unspecified length");
		return FALSE;
	}		
	BT = GetCurrentTimerVal();
	//ģʽ0:д��������; ģʽ1:дSPI����
#ifdef CH375_IF
	RetVal = CH375WriteEndP(AfxDevIndex,AfxOutEndpN,OutBuf,&OutLen);	
#else
	RetVal = CH346WriteData(AfxDevIndex,OutBuf,&OutLen);
#endif
	UseT = GetCurrentTimerVal()-BT;
	if(RetVal)
	{
		TotalTxCnt += OutLen;
		//��ʾ����ͳ����Ϣ
		sprintf(FmtStr,"Tx:%I64uB",TotalTxCnt);
		SetDlgItemText(MainHwnd,IDC_TxStatistics,FmtStr);
	}
	DbgPrint("Write %s,%dB Use time %.3fmS",RetVal?"succ":"failure",OutLen,UseT);
	
	return RetVal;
}

//ģʽ0:����������; ģʽ1:��SPI����
BOOL CH346SP_Read()
{
	ULONG InLen,i;	
	CHAR ValStr[16]="";
	double BT,UseT;
	BOOL RetVal = FALSE;	
	PUCHAR InBuf = NULL;
	PCHAR FmtStr = NULL;

	InLen = GetDlgItemInt(MainHwnd,IDC_ReadLen,NULL,FALSE);	
	if(InLen<1)
	{
		if (g_isChinese)
			DbgPrint("���Ȳ���Ϊ��,��������Ч����.");
		else 
			DbgPrint("Unspecified length");
		goto Exit;
	}
	if(InLen>0x400000) //���ζ����4MB.
		InLen = 0x400000;	

	//�������������ռ�,���4MB
	InBuf = (PUCHAR)malloc(InLen);
	FmtStr = (PCHAR)malloc(InLen*3+10);	
	if( (InBuf == NULL) || (FmtStr ==NULL) )//ϵͳ�ڴ治�㣬���趨������
	{
		InLen = 10240;
		SetDlgItemInt(MainHwnd,IDC_ReadLen,InLen,FALSE);	

		if(InBuf)
		{
			free(InBuf);
			InBuf = (PUCHAR)malloc(InLen);
		}
		if(FmtStr)
		{
			free(FmtStr);
			FmtStr = (PCHAR)malloc(InLen);
		}	
	}
	SetDlgItemText(MainHwnd,IDC_ReadData,"");
	memset(FmtStr,0,sizeof(FmtStr));	
	
	BT = GetCurrentTimerVal();
	//ģʽ0:����������; ģʽ1:��SPI����
#ifdef CH375_IF	
	RetVal = CH375ReadEndP(AfxDevIndex,AfxInEndpN,InBuf,&InLen);
#else
	RetVal = CH346ReadData(AfxDevIndex,InBuf,&InLen);
#endif
	UseT = GetCurrentTimerVal()-BT;	

	//ʮ��������ʽ��ʾ���յ�������
	if(RetVal)
	{		
		if(InLen)
		{//��ʾ����������
			for(i=0;i<InLen;i++)
			{
				sprintf(&FmtStr[strlen(FmtStr)],"%02X ",InBuf[i]);
			}
			SetDlgItemText(MainHwnd,IDC_ReadData,FmtStr);		
			TotalRxCnt += InLen;
		}
		else
		{			
			SetDlgItemText(MainHwnd,IDC_ReadData,"");
		}
		SetDlgItemInt(MainHwnd,IDC_ReadLen,InLen,FALSE); //���¶�����

		sprintf(FmtStr,"Rx:%I64u B",TotalRxCnt);
		SetDlgItemText(MainHwnd,IDC_RxStatistics,FmtStr);
	}
	DbgPrint("CH346Read %s,%dB Use time:%.3fms.TotalRxCnt:%d B",RetVal?"succ":"failure",InLen,UseT,TotalRxCnt);

Exit:
	if(InBuf)
		free(InBuf);
	if(FmtStr)
		free(FmtStr);
	return RetVal;
}

//ѭ������
DWORD WINAPI AfxtAutoRecvThread(LPVOID lpParameter)
{
	CHAR FileName[MAX_PATH] = "",FmtStr[4096]="";
	OPENFILENAME mOpenFile={0};
	ULONG RLen,i,ReadUnitSize;
	BOOL RetVal = FALSE;
	double UseT,BT,UseT1,AvaSpeed,InverBT,InverET;
	UCHAR LastDataI = 0;
	PUCHAR RBuf = NULL;
	HANDLE hRxFile = INVALID_HANDLE_VALUE;

	SetDlgItemText(MainHwnd,IDC_BeginAutoRx,"ֹͣ��");
	if(AfxEnableUploadBuf) //�����ϴ�ģʽ,Ӧ�ò�ÿ�ζ�ȡ�ĳ��ȣ�ΪCH346SetBufUpload��iPktSize��������
		ReadUnitSize = GetDlgItemInt(MainHwnd,IDC_UploadBufPktSize,NULL,FALSE);				
	else
		ReadUnitSize = GetDlgItemInt(MainHwnd,IDC_ReadLen,NULL,FALSE);	
	if(ReadUnitSize>0x400000)
		ReadUnitSize = 0x400000;
	if(ReadUnitSize < 1)
		ReadUnitSize = 4096;

	if (g_isChinese)
		DbgPrint(">>**���ݽ����߳�����,%s,ReadUnitSize:%d...",AfxEnableUploadBuf?"�����ϴ���ʽ��":"ֱ���ϴ���ʽ��",ReadUnitSize);
	else 
		DbgPrint(">>**Device data receiving thread start.");

	RBuf = (PUCHAR)malloc(ReadUnitSize);
	if(RBuf==NULL)
	{
		DbgPrint("ReadBuf alloc mem failure.");
		goto Exit;
	}	
	if(strlen(AfxRxFileName))
	{
		hRxFile = CreateFile(AfxRxFileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_WRITE|FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_ARCHIVE,NULL);
		if(hRxFile==INVALID_HANDLE_VALUE)
		{
			ShowLastError("AfxtAutoRecvToFile.CreateFile");
			goto Exit;
		}
	}
	EnableWindow(GetDlgItem(MainHwnd,IDC_Read),FALSE);
	AfxAutoRecvIsStart = TRUE;
	
	LastDataI = 0;
	TotalRxCnt = 0;
	//����ջ����ϴ�������
#ifdef CH375_IF
	CH375ClearBufUpload(AfxDevIndex,AfxInEndpN);
#else
	CH346ClearBufUpload(AfxDevIndex);
#endif
	InverBT = BT = GetCurrentTimerVal();	
	while( 1 )
	{
		if(StopRxThread)
		{
			if (g_isChinese)
				DbgPrint("��ֹ�����˳��߳�");
			else
				DbgPrint("Stop Device reading and exit");
			break;
		}
		if(!DevIsOpened)
		{
			if (g_isChinese)
				DbgPrint("�豸�ѹرգ��˳��߳�");
			else 
				DbgPrint("Device is closed, exit");
			break;
		}
		RLen = ReadUnitSize;
		//ģʽ0:����������; ģʽ1:��SPI����
#ifdef CH375_IF
		RetVal = CH375ReadEndP(AfxDevIndex,AfxInEndpN,RBuf,&RLen);
#else
		RetVal = CH346ReadData(AfxDevIndex,RBuf,&RLen);
#endif
		if(!RetVal)
		{
			DbgPrint("AfxtAutoRecvToFile.CH346Read err,break");
			break;
		}
		if(RLen)
		{
			//У���ϴ�����.��ǰ֧�ֵ�����У�鷽ʽΪ00-0xFFѭ��
			if(AfxEnableRxDataCheck)
			{
				for(i=0;i<RLen;i++)
				{
					if( RBuf[i] != LastDataI )
					{
						DbgPrint("  Upload data error:%02X-%02X,offset:%I64u",RBuf[i],LastDataI,TotalRxCnt+i);
						goto Exit;
					}
					LastDataI++;
				}
			}
			if(AfxtAutoRecvToFile) //�������ݱ������ļ�.���ڲ��ٶȣ����鲻�������ݱ��湦��
			{
				if( !WriteFile(hRxFile,RBuf,RLen,&RLen,NULL) )
				{
					ShowLastError("AfxtAutoRecvToFile.WriteFile");
					break;
				}
			}
			if(AfxAutoRecvShow) //ʵʱ��ʾ��������
			{
				FmtStr[0]=0;
				for(i=0;i<RLen;i++)
					sprintf(&FmtStr[strlen(FmtStr)],"%02X ",RBuf[i]);
				SendDlgItemMessage(MainHwnd,IDC_ReadData,EM_SETSEL,0xFFFFFFFE,0xFFFFFFFE);
				SendDlgItemMessage(MainHwnd,IDC_ReadData,EM_REPLACESEL,0,(LPARAM)FmtStr);
				SendDlgItemMessage(MainHwnd,IDC_ReadData,EM_SETSEL,0xFFFFFFFE,0xFFFFFFFE);
			}
			SetDlgItemInt(MainHwnd,IDC_ReadLen,RLen,FALSE);
			TotalRxCnt += RLen;
			
			//ƽ���ٶ�ͳ�ƺ���ʾ
			InverET = GetCurrentTimerVal();
			if( ( InverET - InverBT ) > 1000 ) //1���ʼ��ʾ
			{
				InverBT = InverET;
				UseT1 = InverET - BT; //�ۼ���ʱ,��λms
				AvaSpeed = TotalRxCnt/UseT1; //ƽ���ٶ�
				if(AvaSpeed>1024) //MB��λ��ʾ
					sprintf(&FmtStr[0],"Rx:%I64uB,RxSpeed:%.2fMB/S",TotalRxCnt,AvaSpeed/1024);
				else //KB��λ��ʾ
					sprintf(&FmtStr[0],"Rx:%I64uB,RxSpeed:%.2fKB/S",TotalRxCnt,AvaSpeed);

				SetDlgItemText(MainHwnd,IDC_RxStatistics,FmtStr);
				//if(TotalRxCnt>0x100000000) //�ۼƵ�һ����������,��ռ���
				//{
				//	TotalRxCnt = 0;
				//	InverBT = BT = GetCurrentTimerVal();	
				//}
			}
		}
		Sleep(0);
	}

Exit:
	UseT = GetCurrentTimerVal()-BT;

	if(RBuf)
		free(RBuf);
	if(hRxFile!=INVALID_HANDLE_VALUE)
	{
		CloseHandle(hRxFile);
		hRxFile = INVALID_HANDLE_VALUE;
	}
	CheckDlgButton(MainHwnd,IDC_EnAutoRecvToFile,BST_UNCHECKED);
	CheckDlgButton(MainHwnd,IDC_EnAutoRecvShow,BST_UNCHECKED);		
	EnableWindow(GetDlgItem(MainHwnd,IDC_Read),TRUE);
	AfxAutoRecvIsStart = FALSE;
	AfxtAutoRecvToFile = FALSE;
	if (g_isChinese)
		DbgPrint("<<**<<�������ݽ����߳��˳�,�ۼƽ���:%I64uB,UseTime:%.0fms,Speed:%.2fKB/S",TotalRxCnt,UseT,TotalRxCnt/UseT);
	else 
		DbgPrint("<<**<<RxThread end,total recieved %I64uB,Use time:%.0fms,Speed:%.2fKB/S",TotalRxCnt,UseT,TotalRxCnt/UseT);	
	SetDlgItemText(MainHwnd,IDC_BeginAutoRx,"�Զ���");

	return 0;
}

DWORD WINAPI TxFileThread(LPVOID lpParameter)
{
	BOOL RetVal = FALSE;	
	HANDLE hFile=INVALID_HANDLE_VALUE;
	PUCHAR WBuf=NULL;
	ULONG RLen,WLen;
	double UseT,BT,UseT1,AvaSpeed,InverBT,InverET;	
	CHAR FmtStr[256] = "";
	ULONGLONG FileSize = 0,RI = 0;	
	ULONG WriteUnitSize;	
	LONG lDistanceToMove,lDistanceToMoveHigh;

	SetDlgItemText(MainHwnd,IDC_BatchWrite,"ֹͣ����");
	//�����´�ģʽ,�������Ϊ�˵����������Ч�ʸ�
	WriteUnitSize = GetDlgItemInt(MainHwnd,IDC_WriteLen,NULL,FALSE);
	if( ( WriteUnitSize < 1 ) || ( WriteUnitSize > (4*1024*1024) ) )
	{
		DbgPrint("TxFileThread.д����ֵ���ܿգ���Чֵ��ΧΪ1-4MB");
		goto Exit;
	}
	if( WriteUnitSize > (4*1024*1024) ) //���η������Ϊ4MB
		WriteUnitSize = 4*1024*1024;

	if (g_isChinese)
		DbgPrint("*>>*�����ļ��߳�����,%s,����С:%dB...",AfxEnableDnloadBuf?"�ڲ������´���ʽ":"ֱ���´���ʽ",WriteUnitSize);
	else 
		DbgPrint("*>>*Device send file thread start...");	

	if(strlen(AfxTxFileName) < 1)
	{
		if (g_isChinese)
			DbgPrint("�ļ���Ч��������ѡ��");
		else 
			DbgPrint("Invalid file, please select again");
		goto Exit;
	}

	//���ļ�
	hFile = CreateFile(AfxTxFileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);//���ļ�
	if(hFile == INVALID_HANDLE_VALUE)
	{
		ShowLastError("TxFileThread.CreateFile");
		goto Exit;
	}	
	FileSize = GetFileSize(hFile,&RLen); 
	FileSize = FileSize + RLen*0x100000000; //�ߵ�32λ�ϲ�Ϊ64λ����	
	DbgPrint("�����ļ���СΪ%I64u, %s",FileSize,AfxTxFileName);
	//TxFileSize = FileSize;	
	TotalTxCnt = 0;
	RLen = 0;
	
	//���������ڴ�
	WBuf = (PUCHAR)malloc(WriteUnitSize);
	if(WBuf==NULL)
	{
		if (g_isChinese)
			DbgPrint("�����ļ��ڴ�%I64u�ֽ�ʧ��",FileSize);
		else
			DbgPrint("failed to apply for file memory%I64uB", FileSize);
		goto Exit;
	}

	InverBT = BT = GetCurrentTimerVal();	
	RI = 0;
	while(RI<FileSize)
	{		
		if(StopTxThread)
		{
			if (g_isChinese)
				DbgPrint("��ֹд,�˳�");
			else 
				DbgPrint("Stop Device writing, exit");
			break;
		}
		if(!DevIsOpened)
		{
			if (g_isChinese)
				DbgPrint("�豸�ѹرգ��˳�");
			else
				DbgPrint("Device is closed, exit");
			break;
		}
		{
			ULONG PktCnt = 0,OutBufRemainSize = 0;
#ifdef CH375_IF			
			CH375QueryBufDownloadEx(AfxDevIndex,AfxOutEndpN,&PktCnt,&OutBufRemainSize);
			//if(PktCnt)
			//	DbgPrint("OutBuf remain PktCnt:%d,OutBufRemainSize:%d",PktCnt,OutBufRemainSize);		
#else			
			CH346QueryBufDownload(AfxDevIndex,&PktCnt,&OutBufRemainSize);
			//if(PktCnt)
			//	DbgPrint("OutBuf remain PktCnt:%d,OutBufRemainSize:%d",PktCnt,OutBufRemainSize);
#endif
		}
		//���㷢�ʹ�С
		if( (FileSize-RI) > WriteUnitSize )
			RLen = WriteUnitSize;
		else
			RLen = (ULONG)(FileSize-RI);
		//���ļ��������,����4G�ļ�,������Ҫ��ָߵ�32λ
		lDistanceToMove = (LONG)(RI&0xFFFFFFFF);
		lDistanceToMoveHigh = (LONG)((RI>>32)&0xFFFFFFFF) ;
		SetFilePointer(hFile,lDistanceToMove,&lDistanceToMoveHigh,FILE_BEGIN);
		//ȡ����
		if( !ReadFile(hFile,WBuf,RLen,&RLen,NULL) )
		{
			ShowLastError("TxFileThread.ReadFile");
			goto Exit;
		}
		if(RLen<1)
		{
			if (g_isChinese)
				DbgPrint("TxFileThread.�ѵ��ļ�ĩβ");
			else 
				DbgPrint("TxFileThread. End of file");
			break;
		}
		WLen = RLen;
		//ģʽ0:�򲢿ڷ�������; ģʽ1:��SPI�ڷ�������
#ifdef CH375_IF
		RetVal = CH375WriteEndP(AfxDevIndex,AfxOutEndpN,WBuf,&WLen);		
#else
		RetVal = CH346WriteData(AfxDevIndex,WBuf,&WLen);		
#endif
		if( !RetVal )
		{
			if (g_isChinese)
				DbgPrint("TxFileThread.��%I64X��д%dB����ʧ��",RI,WLen);
			else 
				DbgPrint("TxFileThread.Failed to write %XB data at %I64X",RI,WLen);
			break;
		}

		TotalTxCnt += WLen;
		RI += WLen;
		if( ( RLen != WLen ) && (WLen) )//δ��������
		{
			if (g_isChinese)
				DbgPrint("TxFileThread.д���ݲ�����(0x%X-0x%X)",RLen,WLen);
			else 
				DbgPrint("TxFileThread.Incomplete write data(0x%X-0x%X)",RLen,WLen);
			//break;
		}
		{//���㷢���ٶ�
			InverET = GetCurrentTimerVal();
			if( ( InverET - InverBT ) > 1000 )
			{
				InverBT = InverET;
				UseT1 = InverET - BT;
				AvaSpeed = TotalTxCnt/UseT1;
				if(AvaSpeed>1024) //MB
					sprintf(&FmtStr[0],"Tx:%I64uB,TxSpeed:%.2fMB/S",TotalTxCnt,AvaSpeed/1024);
				else //KB
					sprintf(&FmtStr[0],"Tx:%I64uB,TxSpeed:%.0fKB/S",TotalTxCnt,AvaSpeed);
				SetDlgItemText(MainHwnd,IDC_TxStatistics,FmtStr);
			}
		}
		Sleep(0);
	}
	RetVal = TRUE;
	UseT = (GetCurrentTimerVal()-BT)/1000;	
	if (g_isChinese)
		DbgPrint("****�������.�ۼƷ���%I64uB,ƽ���ٶ�:%.3fKB/S,��ʱ%.3fS",RI,RI/UseT/1000,UseT);
	else 
		DbgPrint("****Cumulative send %I64uB,Average speed:%.3fKB/S,Time %.3fS",RI,RI/UseT/1000,UseT);
Exit:
	if(hFile != INVALID_HANDLE_VALUE) 
	{
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}
	if(WBuf!=NULL)
		free(WBuf);

	if (g_isChinese)
		DbgPrint("*<<*�����ļ��߳��˳�");
	else
		DbgPrint("*<<*Send file thread exit");
	DbgPrint("\r\n");	

	SetDlgItemText(MainHwnd,IDC_BatchWrite,"�����ļ�");
	return RetVal;
}

//�豸CH346 USBдģʽ:ֱ���´����ڲ������´�
BOOL SetWriteMode()
{
	UCHAR WriteMode = 0;
	BOOL RetVal;
	ULONG PktCnt;

	WriteMode = (IsDlgButtonChecked(MainHwnd,IDC_EnableBufDown) == BST_CHECKED);
	PktCnt = GetDlgItemInt(MainHwnd,IDC_DnPktCnt,NULL,FALSE);
	if(PktCnt > 10)
		PktCnt = 10;
#ifdef CH375_IF
	CH375SetBufDownloadEx(AfxDevIndex,WriteMode,AfxOutEndpN,PktCnt);
#else
	RetVal = CH346SetBufDownload(AfxDevIndex,WriteMode,PktCnt);
#endif	
	DbgPrint("%s �����´�ģʽ%s,�������:%d.",WriteMode?"����":"����",RetVal?"�ɹ�":"ʧ��",10);

	return RetVal;
}

//�����豸CH346 USB��ģʽ:ֱ���ϴ����ڲ������ϴ�
BOOL SetReadMode()
{
	UCHAR ReadMode = 0;
	BOOL RetVal;
	ULONG PktSize;

	ReadMode = (IsDlgButtonChecked(MainHwnd,IDC_EnableBufUpload) == BST_CHECKED);
	PktSize = GetDlgItemInt(MainHwnd,IDC_UploadBufPktSize,NULL,FALSE);
	if(PktSize>0x400000)
		PktSize = 0x400000;
	if(PktSize<512)
		PktSize = 512;
	PktSize = PktSize/512*512; //�����ϴ�����С,����Ϊ�˵��С��������	

#ifdef CH375_IF
	CH375SetBufUploadEx(AfxDevIndex,ReadMode,AfxInEndpN,PktSize);
#else
	RetVal = CH346SetBufUpload(AfxDevIndex,ReadMode,PktSize);
#endif
	if(ReadMode) //�����û����ϴ�ʱ��ÿ�ζ�ȡ�Ĵ�С���԰�Ϊ��λ���ж�ȡ�����ȱ���ΪPktSize����������
		SetDlgItemInt(MainHwnd,IDC_ReadLen,PktSize,FALSE); 
	AfxEnableUploadBuf = ( ReadMode && RetVal );
	DbgPrint("%s �ڲ������ϴ�ģʽ%s,��ѯ����СΪ:%d.",ReadMode?"����":"����",RetVal?"�ɹ�":"ʧ��",PktSize);
	return RetVal;
}

VOID ResetDevice()
{
	BOOL RetVal;
	RetVal = CH346ResetDevice(AfxDevIndex);
	DbgPrint("��λ�豸 %s.",RetVal?"�ɹ�":"ʧ��");
	return;
}

//�豸����ģʽ
VOID SwithChipWorkMode()
{
	UCHAR ChipMode;
	BOOL IsSave,RetVal;
	
	ChipMode = (UCHAR)SendDlgItemMessage(MainHwnd,IDC_WorkMode,CB_GETCURSEL,0,0);
	IsSave = (IsDlgButtonChecked(MainHwnd,IDC_SaveMode) == BST_CHECKED);

	RetVal = CH346SetChipMode(AfxDevIndex,ChipMode,IsSave);
	DbgPrint("CH346SetChipMode%d,%s %s",ChipMode,IsSave?"���籣��":"���粻����",RetVal?"succ":"failure");	
	{//������Ϣ
		ULONG BufSize;
		CHAR SNString[64] = "" ,FmtStr[256] = "";
#ifdef CH375_IF
#else
	CH346GetDeviceInfor(AfxDevIndex,&AfxDevInfor[AfxDevIndex]);		
	BufSize = sizeof(SNString);
	CH346GetSnString(AfxDevIndex,SNString,&BufSize);
	DbgPrint("**>>**%d#Device opene %s...UploadEndp:%d,DownloadEndp:%d,ReadTimeout:%dms,WriteTimeout:%dms,SN:%s",
		AfxDevIndex,DevIsOpened?"Success":"Failed",
		AfxDevInfor[AfxDevIndex].DataUpEndp,AfxDevInfor[AfxDevIndex].DataDnEndp,500,500,SNString);
#endif	
	{//��ʾ�豸��Ϣ
		sprintf(FmtStr,"**ChipMode:%d,%s,DevID:%s,%s.%s,SerialNumber:%s",
			AfxDevInfor[AfxDevIndex].ChipMode,AfxDevInfor[AfxDevIndex].UsbSpeedType?"HS":"FS",
			AfxDevInfor[AfxDevIndex].DeviceID,
			AfxDevInfor[AfxDevIndex].ManufacturerString,AfxDevInfor[AfxDevIndex].ProductString,SNString);
		SetDlgItemText(MainHwnd,IDC_DevInfor,FmtStr);
	}
}
}

//������
BOOL APIENTRY DlgProc_CH346(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case WM_INITDIALOG:
		MainHwnd = hWnd;
		AfxActiveHwnd = hWnd;
		CheckDlgButton(hWnd,IDC_EnablePnPAutoOpen,BST_CHECKED);		
		SetEditlInputMode(MainHwnd,IDC_WriteData,1);
		SetEditlInputMode(MainHwnd,IDC_ReadData,1);
		InitDlg();
		SendDlgItemMessage(MainHwnd,IDC_RefreshDevList,BM_CLICK,0,0);
		//ΪUSB2.0�豸���ò���Ͱγ���֪ͨ.������Զ����豸,�γ���ر��豸
		if(CH346SetDeviceNotify(AfxDevIndex,AfxDevUsbID, UsbDevPnpNotify) )       //�豸���֪ͨ�ص�����
			DbgPrint("�ѿ���USB�豸��μ���");
		break;	
	case WM_UsbDevArrive:
		AfxPlugCnt++;
		DbgPrint("****����CH346�豸����U��,���豸");
		//��ö��USB�豸
		SendDlgItemMessage(MainHwnd,IDC_RefreshDevList,BM_CLICK,0,0);
		//���豸
		SendDlgItemMessage(MainHwnd,IDC_OpenDevice,BM_CLICK,0,0);
		{
			CHAR FmtStr[128] = "";
			sprintf(FmtStr,"*Plug.%d |  Remove.%d",AfxPlugCnt,AfxRemoveCnt);
			SetDlgItemText(MainHwnd,IDC_PnPStatus,FmtStr);
		}
		break;
	case WM_UsbDevRemove:
		AfxRemoveCnt++;
		DbgPrint("****����CH346�Ѵ�USB���Ƴ�,�ر��豸");
		//�ر��豸
		SendDlgItemMessage(MainHwnd,IDC_CloseDevice,BM_CLICK,0,0);
		SendDlgItemMessage(MainHwnd,IDC_RefreshDevList,BM_CLICK,0,0);
		{
			CHAR FmtStr[128] = "";
			sprintf(FmtStr," Plug.%d | *Remove.%d",AfxPlugCnt,AfxRemoveCnt);
			SetDlgItemText(MainHwnd,IDC_PnPStatus,FmtStr);
		}
		break;	
	case WM_COMMAND:
		wmId    = LOWORD(wParam); 
		wmEvent = HIWORD(wParam); 
		// Parse the menu selections:
		switch (wmId)
		{
		case IDC_SelTxFile:
			{
				OPENFILENAME mOpenFile={0};
				// Fill in the OPENFILENAME structure to support a template and hook.
				mOpenFile.lStructSize = sizeof(OPENFILENAME);
				mOpenFile.hwndOwner         = MainHwnd;
				mOpenFile.hInstance         = AfxMainIns;		
				mOpenFile.lpstrFilter       = "*.*\x0\x0";		
				mOpenFile.lpstrCustomFilter = NULL;
				mOpenFile.nMaxCustFilter    = 0;
				mOpenFile.nFilterIndex      = 0;
				mOpenFile.lpstrFile         = AfxTxFileName;
				mOpenFile.nMaxFile          = sizeof(AfxTxFileName);
				mOpenFile.lpstrFileTitle    = NULL;
				mOpenFile.nMaxFileTitle     = 0;
				mOpenFile.lpstrInitialDir   = NULL;
				if (g_isChinese)
					mOpenFile.lpstrTitle        = "ѡ���д��������ļ�";
				else 
					mOpenFile.lpstrTitle        = "Select the data file to be written";
				mOpenFile.nFileOffset       = 0;
				mOpenFile.nFileExtension    = 0;
				mOpenFile.lpstrDefExt       = NULL;
				mOpenFile.lCustData         = 0;
				mOpenFile.lpfnHook 		    = NULL;
				mOpenFile.lpTemplateName    = NULL;
				mOpenFile.Flags             = OFN_SHOWHELP | OFN_EXPLORER | OFN_READONLY | OFN_FILEMUSTEXIST;
				if (!GetOpenFileName(&mOpenFile))
					AfxTxFileName[0] = 0;
				SetDlgItemText(hWnd,IDC_TxFilePath,AfxTxFileName);
			}
			break;
		case IDC_ResetDevice:
			ResetDevice();
			break;
		case IDC_SwithWorkMode:
			SwithChipWorkMode();
			break;
		case IDC_EnableBufDown:
			SetWriteMode();
			break;
		case IDC_EnableBufUpload:
			SetReadMode();
			break;
		case IDC_BatchWrite:
			{
				CHAR BtName[64]="";
				if(!DevIsOpened)
				{				
					if (g_isChinese)
						DbgPrint("�ȴ��豸");
					else
						DbgPrint("Open device first");
					break;
				}			
				GetDlgItemText(MainHwnd,IDC_BatchWrite,BtName,sizeof(BtName));
				if(strcmp(BtName,"�����ļ�")==0 )
				{
					StopTxThread = FALSE;
					CloseHandle(CreateThread(NULL,0,TxFileThread,(PVOID)0,0,NULL));
				}
				else
					StopTxThread = TRUE;
			}
			break;
		case IDC_EnableRxDataCheck0:
			if(wmEvent==BM_CLICK)
			{
				AfxEnableRxDataCheck = (IsDlgButtonChecked(MainHwnd,IDC_EnableRxDataCheck0) == BST_CHECKED);
				DbgPrint("���ݼ���",AfxEnableRxDataCheck?"����":"����");
			}
			break;
		case IDC_StopTxThread:
			StopTxThread = TRUE;
			break;
		case IDC_EnAutoRecvShow:
			AfxAutoRecvShow = (IsDlgButtonChecked(MainHwnd,IDC_EnAutoRecvShow)==BST_CHECKED);
			break;
		case IDC_EnAutoRecvToFile:
			{
				if(wmEvent==BN_CLICKED)
				{
					if( (IsDlgButtonChecked(MainHwnd,IDC_EnAutoRecvToFile)==BST_CHECKED) )
					{				
						CHAR AfxRxFileName[MAX_PATH] = "";
						OPENFILENAME mOpenFile={0};
						
						// Fill in the OPENFILENAME structure to support a template and hook.
						mOpenFile.lStructSize = sizeof(OPENFILENAME);
						mOpenFile.hwndOwner         = MainHwnd;
						mOpenFile.hInstance         = AfxMainIns;		
						mOpenFile.lpstrFilter       = "*.*\x0\x0";		
						mOpenFile.lpstrCustomFilter = NULL;
						mOpenFile.nMaxCustFilter    = 0;
						mOpenFile.nFilterIndex      = 0;
						mOpenFile.lpstrFile         = AfxRxFileName;
						mOpenFile.nMaxFile          = sizeof(AfxRxFileName);
						mOpenFile.lpstrFileTitle    = NULL;
						mOpenFile.nMaxFileTitle     = 0;
						mOpenFile.lpstrInitialDir   = NULL;
						if (g_isChinese)
							mOpenFile.lpstrTitle    = "ѡ������ļ�";
						else
							mOpenFile.lpstrTitle    = "Select receive file";

						mOpenFile.nFileOffset       = 0;
						mOpenFile.nFileExtension    = 0;
						mOpenFile.lpstrDefExt       = NULL;
						mOpenFile.lCustData         = 0;
						mOpenFile.lpfnHook 		   = NULL;
						mOpenFile.lpTemplateName    = NULL;
						mOpenFile.Flags             = OFN_SHOWHELP | OFN_EXPLORER | OFN_READONLY | OFN_FILEMUSTEXIST;
						if (GetSaveFileName(&mOpenFile))
						{
							DbgPrint("*>>*RxData will save to %s",AfxRxFileName);
							SetDlgItemText(MainHwnd,IDC_RxFilePath,AfxRxFileName);
						}
						else
							AfxRxFileName[0] = 0;
					}
				}
			}
			break;
		case IDC_BeginAutoRx:
			{
				CHAR BtName[64] = "";

				if(!DevIsOpened)
				{				
					if (g_isChinese)
						DbgPrint("�ȴ��豸");
					else
						DbgPrint("Open device first");
					break;
				}
				GetDlgItemText(MainHwnd,IDC_BeginAutoRx,BtName,sizeof(BtName));
				if(strcmp(BtName,"�Զ���")==0 )
				{
					GetDlgItemText(MainHwnd,IDC_RxFilePath,AfxRxFileName,sizeof(AfxRxFileName));
					StopRxThread = FALSE;
					CloseHandle(CreateThread(NULL,0,AfxtAutoRecvThread,(PVOID)0,0,NULL));
				}
				else
				{
					StopRxThread = TRUE;
				}
			}
			break;		
		case IDC_RefreshDevList:
			//EnumDevice();
			CH346Enum();
			break;
		case IDC_CloseDevice:
			CloseDevice();
			break;
		case IDC_OpenDevice:
			OpenDevice();
			break;
		case IDC_Write:
			CH346SP_Write();
			break;
		case IDC_Read:
			CH346SP_Read();
			break;
		case IDC_ClearDbgShow:
			SetDlgItemText(hWnd,IDC_DbgShow,"");
			break;
		case IDC_ResetTRxCnt:
			TotalTxCnt = TotalRxCnt = 0;
			AfxPlugCnt = AfxRemoveCnt = 0;
			SetDlgItemText(hWnd,IDC_TxStatistics,"Tx:0 B");
			SetDlgItemText(hWnd,IDC_RxStatistics,"Rx:0 B");
			break;
		case WM_DESTROY:
			SendDlgItemMessage(hWnd,IDC_CloseDevice,BM_CLICK,0,0);
			CH346SetDeviceNotify(AfxDevIndex,AfxDevUsbID,NULL);
			DestroyWindow(hWnd);
			break;
		default:
			DefWindowProc(hWnd, message, wParam, lParam);
			break;
		}
		break;		
		case WM_DESTROY:
			PostQuitMessage(0);
			break;		
	}
	return 0;
}

//Ӧ�ó������
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	DbgPrint("------  CH346DEMO Build on %s-%s -------\n", __DATE__, __TIME__ );
	
	srand( (unsigned)time( NULL ) );
	AfxMainIns = hInstance;
	return DialogBox(hInstance, (LPCTSTR)IDD_SPI_Parallel_Demo, 0, (DLGPROC)DlgProc_CH346);
}