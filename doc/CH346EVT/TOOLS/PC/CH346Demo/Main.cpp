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

#define WM_UsbDevArrive WM_USER+10         //设备插入通知事件,窗体进程接收
#define WM_UsbDevRemove WM_USER+11         //设备拔出通知事件,窗体进程接收

HWND MainHwnd;     //主窗体句柄
BOOL g_isChinese;
HINSTANCE AfxMainIns; //进程实例
extern HWND AfxActiveHwnd;
BOOL DevIsOpened;  //设备是否打开
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
ULONG AfxInEndpN = 1,AfxOutEndpN = 1; //CH375 IF使用

//使能操作按钮
VOID EnableButtonEnable();
BOOL APIENTRY DlgProc_CH346(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


//初始化界面参数选择
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

//使能操作按钮，需先打开和配置JTAG，否则无法操作
VOID EnableButtonEnable()
{
	//更新打开/关闭设备按钮状态
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

// USB设备插拔检测通知程序.因回调函数对函数操作有限制，通过窗体消息转移到消息处理函数内进行处理。
VOID	 CALLBACK	 UsbDevPnpNotify (ULONG iEventStatus ) 
{
	if(iEventStatus==CH346_DEVICE_ARRIVAL)// 设备插入事件,已经插入
		PostMessage(MainHwnd,WM_UsbDevArrive,0,0);
	else if(iEventStatus==CH346_DEVICE_REMOVE)// 设备拔出事件,已经拔出
		PostMessage(MainHwnd,WM_UsbDevRemove,0,0);
	return;
}

//枚举设备
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
		//获取设备名称
#ifdef CH375_IF
		EnumDevPath = (PCHAR)CH375GetDeviceName(i);
		//可增加ID判断
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


//打开设备
BOOL OpenDevice()
{
	CHAR FmtStr[128] = "",SNString[128] = "";
	ULONG BufSize;

	//获取设备序号
	AfxDevIndex = SendDlgItemMessage(MainHwnd,IDC_DevList,CB_GETCURSEL,0,0);
	if(AfxDevIndex==CB_ERR)
	{
		if (g_isChinese)
			DbgPrint("打开设备失败,请先选择设备");
		else 
			DbgPrint("Failed to open the device. Please select the device first");
		goto Exit; //退出
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
	{//初始化设备插拔信息		
		sprintf(FmtStr,"*Plug.%d | Remove.%d",AfxPlugCnt,AfxRemoveCnt);
		SetDlgItemText(MainHwnd,IDC_PnPStatus,FmtStr);
	}
	{//显示设备信息
		sprintf(FmtStr,"**ChipMode:%d,%s,DevID:%s,%s.%s,SerialNumber:%s",
			AfxDevInfor[AfxDevIndex].ChipMode,AfxDevInfor[AfxDevIndex].UsbSpeedType?"HS":"FS",
			AfxDevInfor[AfxDevIndex].DeviceID,
			AfxDevInfor[AfxDevIndex].ManufacturerString,AfxDevInfor[AfxDevIndex].ProductString,SNString);
		SetDlgItemText(MainHwnd,IDC_DevInfor,FmtStr);
	}

	StopTxThread = FALSE;
	StopRxThread = FALSE;
	TotalTxCnt = TotalRxCnt = 0;
	EnableButtonEnable(); //使用操作按钮
Exit:
	return DevIsOpened;
}

//关闭设备
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

//模式0:写并口数据;模式1:写SPI数据
BOOL CH346SP_Write()
{
	ULONG OutLen,i,StrLen;
	UCHAR OutBuf[4096] = "";
	CHAR FmtStr[4096*3*6] = "",ValStr[16]="";
	double BT,UseT;
	BOOL RetVal = FALSE;

	StrLen = GetDlgItemText(MainHwnd,IDC_WriteData,FmtStr,sizeof(FmtStr));	
	if(StrLen > 4096*3) //写
		StrLen = 4096*3;
	//获取十六进制数据
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
			DbgPrint("长度不能为空,请输入有效长度.");
		else 
			DbgPrint("Unspecified length");
		return FALSE;
	}		
	BT = GetCurrentTimerVal();
	//模式0:写并口数据; 模式1:写SPI数据
#ifdef CH375_IF
	RetVal = CH375WriteEndP(AfxDevIndex,AfxOutEndpN,OutBuf,&OutLen);	
#else
	RetVal = CH346WriteData(AfxDevIndex,OutBuf,&OutLen);
#endif
	UseT = GetCurrentTimerVal()-BT;
	if(RetVal)
	{
		TotalTxCnt += OutLen;
		//显示发送统计信息
		sprintf(FmtStr,"Tx:%I64uB",TotalTxCnt);
		SetDlgItemText(MainHwnd,IDC_TxStatistics,FmtStr);
	}
	DbgPrint("Write %s,%dB Use time %.3fmS",RetVal?"succ":"failure",OutLen,UseT);
	
	return RetVal;
}

//模式0:读并口数据; 模式1:读SPI数据
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
			DbgPrint("长度不能为空,请输入有效长度.");
		else 
			DbgPrint("Unspecified length");
		goto Exit;
	}
	if(InLen>0x400000) //单次读最多4MB.
		InLen = 0x400000;	

	//申请读操作所需空间,最大4MB
	InBuf = (PUCHAR)malloc(InLen);
	FmtStr = (PCHAR)malloc(InLen*3+10);	
	if( (InBuf == NULL) || (FmtStr ==NULL) )//系统内存不足，重设定读长度
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
	//模式0:读并口数据; 模式1:读SPI数据
#ifdef CH375_IF	
	RetVal = CH375ReadEndP(AfxDevIndex,AfxInEndpN,InBuf,&InLen);
#else
	RetVal = CH346ReadData(AfxDevIndex,InBuf,&InLen);
#endif
	UseT = GetCurrentTimerVal()-BT;	

	//十六进制形式显示接收到的数据
	if(RetVal)
	{		
		if(InLen)
		{//显示读到的数据
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
		SetDlgItemInt(MainHwnd,IDC_ReadLen,InLen,FALSE); //更新读长度

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

//循环接收
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

	SetDlgItemText(MainHwnd,IDC_BeginAutoRx,"停止读");
	if(AfxEnableUploadBuf) //缓冲上传模式,应用层每次读取的长度，为CH346SetBufUpload中iPktSize的整数倍
		ReadUnitSize = GetDlgItemInt(MainHwnd,IDC_UploadBufPktSize,NULL,FALSE);				
	else
		ReadUnitSize = GetDlgItemInt(MainHwnd,IDC_ReadLen,NULL,FALSE);	
	if(ReadUnitSize>0x400000)
		ReadUnitSize = 0x400000;
	if(ReadUnitSize < 1)
		ReadUnitSize = 4096;

	if (g_isChinese)
		DbgPrint(">>**数据接收线程启动,%s,ReadUnitSize:%d...",AfxEnableUploadBuf?"缓冲上传方式读":"直接上传方式读",ReadUnitSize);
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
	//先清空缓冲上传区数据
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
				DbgPrint("中止读，退出线程");
			else
				DbgPrint("Stop Device reading and exit");
			break;
		}
		if(!DevIsOpened)
		{
			if (g_isChinese)
				DbgPrint("设备已关闭，退出线程");
			else 
				DbgPrint("Device is closed, exit");
			break;
		}
		RLen = ReadUnitSize;
		//模式0:读并口数据; 模式1:读SPI数据
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
			//校验上传数据.当前支持的数据校验方式为00-0xFF循环
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
			if(AfxtAutoRecvToFile) //接收数据保存至文件.如在测速度，建议不开启数据保存功能
			{
				if( !WriteFile(hRxFile,RBuf,RLen,&RLen,NULL) )
				{
					ShowLastError("AfxtAutoRecvToFile.WriteFile");
					break;
				}
			}
			if(AfxAutoRecvShow) //实时显示数据内容
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
			
			//平均速度统计和显示
			InverET = GetCurrentTimerVal();
			if( ( InverET - InverBT ) > 1000 ) //1秒后开始显示
			{
				InverBT = InverET;
				UseT1 = InverET - BT; //累计用时,单位ms
				AvaSpeed = TotalRxCnt/UseT1; //平均速度
				if(AvaSpeed>1024) //MB单位显示
					sprintf(&FmtStr[0],"Rx:%I64uB,RxSpeed:%.2fMB/S",TotalRxCnt,AvaSpeed/1024);
				else //KB单位显示
					sprintf(&FmtStr[0],"Rx:%I64uB,RxSpeed:%.2fKB/S",TotalRxCnt,AvaSpeed);

				SetDlgItemText(MainHwnd,IDC_RxStatistics,FmtStr);
				//if(TotalRxCnt>0x100000000) //累计到一段数据量后,清空计数
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
		DbgPrint("<<**<<串口数据接收线程退出,累计接收:%I64uB,UseTime:%.0fms,Speed:%.2fKB/S",TotalRxCnt,UseT,TotalRxCnt/UseT);
	else 
		DbgPrint("<<**<<RxThread end,total recieved %I64uB,Use time:%.0fms,Speed:%.2fKB/S",TotalRxCnt,UseT,TotalRxCnt/UseT);	
	SetDlgItemText(MainHwnd,IDC_BeginAutoRx,"自动读");

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

	SetDlgItemText(MainHwnd,IDC_BatchWrite,"停止发送");
	//缓冲下传模式,建议包长为端点的整数倍，效率高
	WriteUnitSize = GetDlgItemInt(MainHwnd,IDC_WriteLen,NULL,FALSE);
	if( ( WriteUnitSize < 1 ) || ( WriteUnitSize > (4*1024*1024) ) )
	{
		DbgPrint("TxFileThread.写长度值不能空，有效值范围为1-4MB");
		goto Exit;
	}
	if( WriteUnitSize > (4*1024*1024) ) //单次发送最大为4MB
		WriteUnitSize = 4*1024*1024;

	if (g_isChinese)
		DbgPrint("*>>*发送文件线程启动,%s,包大小:%dB...",AfxEnableDnloadBuf?"内部缓冲下传方式":"直接下传方式",WriteUnitSize);
	else 
		DbgPrint("*>>*Device send file thread start...");	

	if(strlen(AfxTxFileName) < 1)
	{
		if (g_isChinese)
			DbgPrint("文件无效，请重新选择");
		else 
			DbgPrint("Invalid file, please select again");
		goto Exit;
	}

	//打开文件
	hFile = CreateFile(AfxTxFileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);//打开文件
	if(hFile == INVALID_HANDLE_VALUE)
	{
		ShowLastError("TxFileThread.CreateFile");
		goto Exit;
	}	
	FileSize = GetFileSize(hFile,&RLen); 
	FileSize = FileSize + RLen*0x100000000; //高低32位合并为64位长度	
	DbgPrint("发送文件大小为%I64u, %s",FileSize,AfxTxFileName);
	//TxFileSize = FileSize;	
	TotalTxCnt = 0;
	RLen = 0;
	
	//申请数据内存
	WBuf = (PUCHAR)malloc(WriteUnitSize);
	if(WBuf==NULL)
	{
		if (g_isChinese)
			DbgPrint("申请文件内存%I64u字节失败",FileSize);
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
				DbgPrint("中止写,退出");
			else 
				DbgPrint("Stop Device writing, exit");
			break;
		}
		if(!DevIsOpened)
		{
			if (g_isChinese)
				DbgPrint("设备已关闭，退出");
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
		//计算发送大小
		if( (FileSize-RI) > WriteUnitSize )
			RLen = WriteUnitSize;
		else
			RLen = (ULONG)(FileSize-RI);
		//从文件里读数据,超过4G文件,长度需要拆分高低32位
		lDistanceToMove = (LONG)(RI&0xFFFFFFFF);
		lDistanceToMoveHigh = (LONG)((RI>>32)&0xFFFFFFFF) ;
		SetFilePointer(hFile,lDistanceToMove,&lDistanceToMoveHigh,FILE_BEGIN);
		//取数据
		if( !ReadFile(hFile,WBuf,RLen,&RLen,NULL) )
		{
			ShowLastError("TxFileThread.ReadFile");
			goto Exit;
		}
		if(RLen<1)
		{
			if (g_isChinese)
				DbgPrint("TxFileThread.已到文件末尾");
			else 
				DbgPrint("TxFileThread. End of file");
			break;
		}
		WLen = RLen;
		//模式0:向并口发送数据; 模式1:向SPI口发送数据
#ifdef CH375_IF
		RetVal = CH375WriteEndP(AfxDevIndex,AfxOutEndpN,WBuf,&WLen);		
#else
		RetVal = CH346WriteData(AfxDevIndex,WBuf,&WLen);		
#endif
		if( !RetVal )
		{
			if (g_isChinese)
				DbgPrint("TxFileThread.从%I64X处写%dB数据失败",RI,WLen);
			else 
				DbgPrint("TxFileThread.Failed to write %XB data at %I64X",RI,WLen);
			break;
		}

		TotalTxCnt += WLen;
		RI += WLen;
		if( ( RLen != WLen ) && (WLen) )//未发送完整
		{
			if (g_isChinese)
				DbgPrint("TxFileThread.写数据不完整(0x%X-0x%X)",RLen,WLen);
			else 
				DbgPrint("TxFileThread.Incomplete write data(0x%X-0x%X)",RLen,WLen);
			//break;
		}
		{//计算发送速度
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
		DbgPrint("****发送完成.累计发送%I64uB,平均速度:%.3fKB/S,用时%.3fS",RI,RI/UseT/1000,UseT);
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
		DbgPrint("*<<*发送文件线程退出");
	else
		DbgPrint("*<<*Send file thread exit");
	DbgPrint("\r\n");	

	SetDlgItemText(MainHwnd,IDC_BatchWrite,"发送文件");
	return RetVal;
}

//设备CH346 USB写模式:直接下传或内部缓冲下传
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
	DbgPrint("%s 缓冲下传模式%s,缓冲包数:%d.",WriteMode?"启用":"禁用",RetVal?"成功":"失败",10);

	return RetVal;
}

//设置设备CH346 USB读模式:直接上传或内部缓冲上传
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
	PktSize = PktSize/512*512; //缓冲上传包大小,必须为端点大小的整数包	

#ifdef CH375_IF
	CH375SetBufUploadEx(AfxDevIndex,ReadMode,AfxInEndpN,PktSize);
#else
	RetVal = CH346SetBufUpload(AfxDevIndex,ReadMode,PktSize);
#endif
	if(ReadMode) //如启用缓冲上传时，每次读取的大小，以包为单位进行读取，长度必须为PktSize的整数倍。
		SetDlgItemInt(MainHwnd,IDC_ReadLen,PktSize,FALSE); 
	AfxEnableUploadBuf = ( ReadMode && RetVal );
	DbgPrint("%s 内部缓冲上传模式%s,轮询包大小为:%d.",ReadMode?"启用":"禁用",RetVal?"成功":"失败",PktSize);
	return RetVal;
}

VOID ResetDevice()
{
	BOOL RetVal;
	RetVal = CH346ResetDevice(AfxDevIndex);
	DbgPrint("复位设备 %s.",RetVal?"成功":"失败");
	return;
}

//设备工作模式
VOID SwithChipWorkMode()
{
	UCHAR ChipMode;
	BOOL IsSave,RetVal;
	
	ChipMode = (UCHAR)SendDlgItemMessage(MainHwnd,IDC_WorkMode,CB_GETCURSEL,0,0);
	IsSave = (IsDlgButtonChecked(MainHwnd,IDC_SaveMode) == BST_CHECKED);

	RetVal = CH346SetChipMode(AfxDevIndex,ChipMode,IsSave);
	DbgPrint("CH346SetChipMode%d,%s %s",ChipMode,IsSave?"掉电保存":"掉电不保存",RetVal?"succ":"failure");	
	{//更新信息
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
	{//显示设备信息
		sprintf(FmtStr,"**ChipMode:%d,%s,DevID:%s,%s.%s,SerialNumber:%s",
			AfxDevInfor[AfxDevIndex].ChipMode,AfxDevInfor[AfxDevIndex].UsbSpeedType?"HS":"FS",
			AfxDevInfor[AfxDevIndex].DeviceID,
			AfxDevInfor[AfxDevIndex].ManufacturerString,AfxDevInfor[AfxDevIndex].ProductString,SNString);
		SetDlgItemText(MainHwnd,IDC_DevInfor,FmtStr);
	}
}
}

//主窗体
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
		//为USB2.0设备设置插入和拔出的通知.插入后自动打开设备,拔出后关闭设备
		if(CH346SetDeviceNotify(AfxDevIndex,AfxDevUsbID, UsbDevPnpNotify) )       //设备插拔通知回调函数
			DbgPrint("已开启USB设备插拔监视");
		break;	
	case WM_UsbDevArrive:
		AfxPlugCnt++;
		DbgPrint("****发现CH346设备插入U口,打开设备");
		//先枚举USB设备
		SendDlgItemMessage(MainHwnd,IDC_RefreshDevList,BM_CLICK,0,0);
		//打开设备
		SendDlgItemMessage(MainHwnd,IDC_OpenDevice,BM_CLICK,0,0);
		{
			CHAR FmtStr[128] = "";
			sprintf(FmtStr,"*Plug.%d |  Remove.%d",AfxPlugCnt,AfxRemoveCnt);
			SetDlgItemText(MainHwnd,IDC_PnPStatus,FmtStr);
		}
		break;
	case WM_UsbDevRemove:
		AfxRemoveCnt++;
		DbgPrint("****发现CH346已从USB口移除,关闭设备");
		//关闭设备
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
					mOpenFile.lpstrTitle        = "选择待写入的数据文件";
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
						DbgPrint("先打开设备");
					else
						DbgPrint("Open device first");
					break;
				}			
				GetDlgItemText(MainHwnd,IDC_BatchWrite,BtName,sizeof(BtName));
				if(strcmp(BtName,"发送文件")==0 )
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
				DbgPrint("数据检验",AfxEnableRxDataCheck?"启用":"禁用");
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
							mOpenFile.lpstrTitle    = "选择接收文件";
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
						DbgPrint("先打开设备");
					else
						DbgPrint("Open device first");
					break;
				}
				GetDlgItemText(MainHwnd,IDC_BeginAutoRx,BtName,sizeof(BtName));
				if(strcmp(BtName,"自动读")==0 )
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

//应用程序入口
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