/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2025                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  USB2.0 UART API Demo
Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2025 Nanjing Qinheng Microelectronics Co., Ltd.
Revision History:
  4/3/2022: TECH30
--*/

#include "Main.h"


#define WM_UartArrive WM_USER+10         //设备插入通知事件,窗体进程接收
#define WM_UartRemove WM_USER+11         //设备拔出通知事件,窗体进程接收

HWND UartDebugHwnd;     //主窗体句柄
extern BOOL g_isChinese;
extern HINSTANCE AfxMainIns; //进程实例
extern HWND AfxActiveHwnd;

BOOL UartDevIsOpened;  //设备是否打开
ULONG UartIndex;
ULONG TotalTxCnt=0,TotalRxCnt=0,TxFileSize;
BOOL StopTxThread,StopRxThread;
mDeviceInforS UartDevInfor[16] = {0};
BOOL UartAutoRecvIsStart = FALSE;
BOOL UartAutoRecvToFile = FALSE,UartAutoRecvShow=FALSE;
HANDLE hRxFile = INVALID_HANDLE_VALUE;

// USB设备插拔检测通知程序.因回调函数对函数操作有限制，通过窗体消息转移到消息处理函数内进行处理。
VOID	 CALLBACK	 Uart_UsbDevPnpNotify (ULONG iEventStatus ) 
{
	if(iEventStatus==CH347_DEVICE_ARRIVAL)// 设备插入事件,已经插入
		PostMessage(UartDebugHwnd,WM_UartArrive,0,0);
	else if(iEventStatus==CH347_DEVICE_REMOVE)// 设备拔出事件,已经拔出
		PostMessage(UartDebugHwnd,WM_UartRemove,0,0);
	return;
}

//显示设备信息
BOOL Uart_ShowDevInfor()
{
	ULONG ObjSel;
	CHAR  FmtStr[128]="";

	ObjSel = SendDlgItemMessage(UartDebugHwnd,IDC_ObjList,CB_GETCURSEL,0,0);
	if(ObjSel!=CB_ERR)
	{
		//sprintf(FmtStr,"**Chip Mode:%d,DevID:%s",UartDevInfor[ObjSel].ChipMode,UartDevInfor[ObjSel].DeviceID);
		sprintf(FmtStr,"**ChipMode:%d,%s,DevID:%s",UartDevInfor[ObjSel].ChipMode,UartDevInfor[ObjSel].UsbSpeedType?"HS":"FS",UartDevInfor[ObjSel].DeviceID);
		SetDlgItemText(UartDebugHwnd,IDC_DevInfor,FmtStr);
	}
	return (ObjSel!=CB_ERR);
}

//枚举设备
ULONG Uart_EnumDevice()
{
	ULONG i,oLen,DevCnt = 0;
	USB_DEVICE_DESCRIPTOR DevDesc = {0};
	CHAR tem[256] = "";
	mDeviceInforS DevInfor = {0};

	SendDlgItemMessage(UartDebugHwnd,IDC_ObjList,CB_RESETCONTENT,0,0);	
	for(i=0;i<16;i++)
	{
		if(CH347Uart_Open(i) != INVALID_HANDLE_VALUE)
		{
			oLen = sizeof(USB_DEVICE_DESCRIPTOR);
			CH347Uart_GetDeviceInfor(i,&DevInfor);			
			sprintf(tem,"%d# %s",i,DevInfor.FuncDescStr);
			SendDlgItemMessage(UartDebugHwnd,IDC_ObjList,CB_ADDSTRING,0,(LPARAM)(LPCTSTR)tem);		
			memcpy(&UartDevInfor[DevCnt],&DevInfor,sizeof(DevInfor));
			DevCnt++;
		}
		CH347Uart_Close(i);
	}
	if(DevCnt)
	{
		SendDlgItemMessage(UartDebugHwnd,IDC_ObjList,CB_SETCURSEL,DevCnt-1,0);
		SetFocus(GetDlgItem(UartDebugHwnd,IDC_ObjList));
	}
	return DevCnt;
}

VOID Uart_InitParam()
{
	{
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Baudrate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"9600");
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Baudrate,CB_SETITEMDATA,0,(LPARAM)9600);
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Baudrate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"19200");
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Baudrate,CB_SETITEMDATA,1,(LPARAM)19200);
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Baudrate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"115200");
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Baudrate,CB_SETITEMDATA,2,(LPARAM)115200);
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Baudrate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"4000000");
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Baudrate,CB_SETITEMDATA,2,(LPARAM)4000000);
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Baudrate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"9000000");
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Baudrate,CB_SETITEMDATA,2,(LPARAM)9000000);
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Baudrate,CB_SETCURSEL,2,0);
	}		

	// 校验位(0：None; 1：Odd; 2：Even; 3：Mark; 4：Space)
	{
		if (g_isChinese)
		{
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"检验位:None");
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_SETITEMDATA,0,(LPARAM)0);
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"检验位:Odd");
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_SETITEMDATA,1,(LPARAM)1);
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"检验位:Even");
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_SETITEMDATA,2,(LPARAM)2);
		} else {
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Parity:None");
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_SETITEMDATA,0,(LPARAM)0);
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Parity:Odd");
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_SETITEMDATA,1,(LPARAM)1);
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Parity:Even");
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_SETITEMDATA,2,(LPARAM)2);
		}
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_SETCURSEL,0,0);
	}
	// 停止位数(0：1停止位; 1：1.5停止位; 2：2停止位)；
	{
		if (g_isChinese)
		{
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_StopBits,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"停止位:1");
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_StopBits,CB_SETITEMDATA,0,(LPARAM)0);
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_StopBits,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"停止位:2");
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_StopBits,CB_SETITEMDATA,1,(LPARAM)2);
		}
		else 
		{
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_StopBits,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Stop Bits:1");
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_StopBits,CB_SETITEMDATA,0,(LPARAM)0);
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_StopBits,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Stop Bits:2");
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_StopBits,CB_SETITEMDATA,1,(LPARAM)2);
		}

		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_StopBits,CB_SETCURSEL,0,0);
	}
	// 数据位数(5,6,7,8,16)
	{
		if (g_isChinese)
		{
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_DataSize,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"数据位:8");
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_DataSize,CB_SETITEMDATA,0,(LPARAM)8);
		}
		else 
		{
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_DataSize,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Data Bits:8");
			SendDlgItemMessage(UartDebugHwnd,IDC_Uart_DataSize,CB_SETITEMDATA,0,(LPARAM)8);
		}

		//SendDlgItemMessage(UartDebugHwnd,IDC_Uart_DataSize,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"数据位:16");
		//SendDlgItemMessage(UartDebugHwnd,IDC_Uart_DataSize,CB_SETITEMDATA,4,(LPARAM)16);
		SendDlgItemMessage(UartDebugHwnd,IDC_Uart_DataSize,CB_SETCURSEL,0,0);
	}
	//超时时间,单位100uS
	SetDlgItemInt(UartDebugHwnd,IDC_Uart_Timeout,0,FALSE);

	SendDlgItemMessage(UartDebugHwnd,IDC_RefreshObjList,BM_CLICK,0,0);	
	EnableButtonEnable_Uart();
}
//打开设备
BOOL Uart_OpenDevice()
{
	//获取设备序号
	UartIndex = SendDlgItemMessage(UartDebugHwnd,IDC_ObjList,CB_GETCURSEL,0,0);
	if(UartIndex==CB_ERR)
	{
		if (g_isChinese)
			DbgPrint("打开设备失败,请先选择设备");
		else 
			DbgPrint("Failed to open the device. Please select the device first");
		goto Exit; //退出
	}	
	UartDevIsOpened = (CH347Uart_Open(UartIndex) != INVALID_HANDLE_VALUE);
	CH347Uart_SetTimeout(UartIndex,500,500);
	StopTxThread = FALSE;
	StopRxThread = FALSE;

	DbgPrint(">>%d#Open Device...%s",UartIndex,UartDevIsOpened?"Success":"Failed");

	EnableButtonEnable_Uart();
Exit:
	return UartDevIsOpened;
}

//关闭设备
BOOL Uart_CloseDevice()
{
	StopTxThread = TRUE;
	StopRxThread = TRUE;
	Sleep(300);
	CH347Uart_Close(UartIndex);
	UartDevIsOpened = FALSE;
	DbgPrint(">>%d#Close device",UartIndex);

	UartIndex = CB_ERR;
	EnableButtonEnable_Uart();
	return TRUE;
}

BOOL Uart_Set()
{	
	BOOL RetVal = FALSE;
	ULONG Baudrate,Sel;
	UCHAR StopBits,Parity,DataSize,Timeout;
	CHAR FmtStr[64]="";

	GetDlgItemText(UartDebugHwnd,IDC_Uart_Baudrate,FmtStr,sizeof(FmtStr));
	Baudrate = atol(FmtStr);

	Sel = SendDlgItemMessage(UartDebugHwnd,IDC_Uart_StopBits,CB_GETCURSEL,0,0);
	StopBits = (UCHAR)SendDlgItemMessage(UartDebugHwnd,IDC_Uart_StopBits,CB_GETITEMDATA,Sel,0);

	Sel = SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_GETCURSEL,0,0);
	Parity = (UCHAR)SendDlgItemMessage(UartDebugHwnd,IDC_Uart_Parity,CB_GETITEMDATA,Sel,0);

	Sel = SendDlgItemMessage(UartDebugHwnd,IDC_Uart_DataSize,CB_GETCURSEL,0,0);
	DataSize = (UCHAR)SendDlgItemMessage(UartDebugHwnd,IDC_Uart_DataSize,CB_GETITEMDATA,Sel,0);

	Timeout = (UCHAR)GetDlgItemInt(UartDebugHwnd,IDC_Uart_Timeout,NULL,FALSE);

	RetVal = CH347Uart_Init(UartIndex,Baudrate,DataSize,Parity,StopBits,Timeout);
	DbgPrint("Uart_Set %s,Baudrate:%d,DataSize:%d,Parity:%d,StopBits:%d,Timeout:%",RetVal?"succ":"failure",
		Baudrate,DataSize,Parity,StopBits,Timeout);

	return RetVal;
}

VOID UpdateTRxCountShow()
{
	CHAR FmtStr[256]="";

	if(TxFileSize)
		sprintf(FmtStr,"Tx:%d,Rx:%d,%.1f%%",TotalTxCnt,TotalRxCnt,(float)TotalTxCnt*100/TxFileSize);
	else
		sprintf(FmtStr,"Tx:%d,Rx:%d",TotalTxCnt,TotalRxCnt);
	SetDlgItemText(UartDebugHwnd,IDC_TotalCntTRx,FmtStr);
	return;
}

BOOL Uart_Write()
{
	ULONG OutLen,i,StrLen;
	UCHAR OutBuf[4096] = "";
	CHAR FmtStr[4096*3*6] = "",ValStr[16]="";
	double BT,UseT;
	BOOL RetVal = FALSE;

	StrLen = GetDlgItemText(UartDebugHwnd,IDC_Uart_WriteData,FmtStr,sizeof(FmtStr));
	if(StrLen > 4096*3)
		StrLen = 4096*3;
	OutLen = 0;
	for(i=0;i<StrLen;i+=3)
	{		
		memcpy(&ValStr[0],&FmtStr[i],2);
		OutBuf[OutLen] = (UCHAR)mStrToHEX(ValStr);
		OutLen++;
	}
	SetDlgItemInt(UartDebugHwnd,IDC_Uart_WriteLen,OutLen,FALSE);	
	
	if(OutLen<1)
	{
		if (g_isChinese)
			DbgPrint("未指定长度");
		else 
			DbgPrint("Unspecified length");
		return FALSE;
	}		
	BT = GetCurrentTimerVal();		
	RetVal = CH347Uart_Write(UartIndex,OutBuf,&OutLen);
	UseT = GetCurrentTimerVal()-BT;
	if(RetVal)
	{
		TotalTxCnt += OutLen;
		UpdateTRxCountShow();
	}

	DbgPrint("Uart_Write %dB %s.Time %.3fS",OutLen,RetVal?"succ":"failure",UseT/1000);
	
	return RetVal;
}

BOOL Uart_Read()
{
	ULONG InLen,i;
	UCHAR InBuf[4096] = "";
	CHAR FmtStr[4096*3*6] = "",ValStr[16]="";
	double BT,UseT;
	BOOL RetVal = FALSE;	

	InLen = GetDlgItemInt(UartDebugHwnd,IDC_Uart_ReadLen,NULL,FALSE);	
	if(InLen>4096)
		InLen = 4096;	
	SetDlgItemText(UartDebugHwnd,IDC_Uart_ReadData,"");
	memset(FmtStr,0,sizeof(FmtStr));
	
	if(InLen<1)
	{
		if (g_isChinese)
			DbgPrint("未指定长度");
		else 
			DbgPrint("Unspecified length");
		return FALSE;
	}
	BT = GetCurrentTimerVal();
	RetVal = CH347Uart_Read(UartIndex,InBuf,&InLen);
	UseT = GetCurrentTimerVal()-BT;
	
	DbgPrint("CH347Uart_Read %dB %s.Time %.3fS.",InLen,RetVal?"succ":"failure",UseT/1000);
	
	if(RetVal)
	{		
		if(InLen)
		{//打印
			memset(FmtStr,0,sizeof(FmtStr));			
			for(i=0;i<InLen;i++)
			{
				sprintf(&FmtStr[strlen(FmtStr)],"%02X ",InBuf[i]);
			}
			SetDlgItemText(UartDebugHwnd,IDC_Uart_ReadData,FmtStr);		
			TotalRxCnt += InLen;
			UpdateTRxCountShow();
		}
		else
		{			
			SetDlgItemText(UartDebugHwnd,IDC_Uart_ReadData,"");
		}
		SetDlgItemInt(UartDebugHwnd,IDC_Uart_ReadLen,InLen,FALSE);		
	}
	return RetVal;
}

DWORD WINAPI UartAutoRecvToFileThread(LPVOID lpParameter)
{
	CHAR FileName[MAX_PATH] = "",FmtStr[4096]="";
	OPENFILENAME mOpenFile={0};
	ULONG RLen,i;
    UCHAR RBuf[4096]="";
	BOOL RetVal = FALSE;

	if (g_isChinese)
		DbgPrint(">>**>>串口数据接收线程启动.");
	else 
		DbgPrint(">>**>>Serial port data receiving thread start.");

	EnableWindow(GetDlgItem(UartDebugHwnd,IDC_Uart_StopRxThread),TRUE);
	UartAutoRecvIsStart = TRUE;
	
	while( 1 )
	{
		if(StopRxThread)
		{
			if (g_isChinese)
				DbgPrint("中止串口读，退出");
			else
				DbgPrint("Stop serial port reading and exit");
			break;
		}
		if(!UartDevIsOpened)
		{
			if (g_isChinese)
				DbgPrint("串口已关闭，退出");
			else 
				DbgPrint("Serial port is closed, exit");
			break;
		}		

		RLen = 4096;
		RetVal = CH347Uart_Read(UartIndex,RBuf,&RLen);
		if(!RetVal)
		{
			DbgPrint("UartAutoRecvToFIle.CH347Uart_Read err,break");
			break;
		}
		if(RLen)
		{
			if(UartAutoRecvToFile)
			{
				if( !WriteFile(hRxFile,RBuf,RLen,&RLen,NULL) )
				{
					ShowLastError("UartAutoRecvToFIle.WriteFile");
					break;
				}
			}
			if(UartAutoRecvShow)
			{
				FmtStr[0]=0;
				for(i=0;i<RLen;i++)
					sprintf(&FmtStr[strlen(FmtStr)],"%02X ",RBuf[i]);
				SendDlgItemMessage(UartDebugHwnd,IDC_Uart_ReadData,EM_SETSEL,0xFFFFFFFE,0xFFFFFFFE);
				SendDlgItemMessage(UartDebugHwnd,IDC_Uart_ReadData,EM_REPLACESEL,0,(LPARAM)FmtStr);
				SendDlgItemMessage(UartDebugHwnd,IDC_Uart_ReadData,EM_SETSEL,0xFFFFFFFE,0xFFFFFFFE);
			}
			SetDlgItemInt(UartDebugHwnd,IDC_Uart_ReadLen,RLen,FALSE);
			TotalRxCnt += RLen;
			UpdateTRxCountShow();
		}
		Sleep(0);
	}
//Exit:
	if(hRxFile!=INVALID_HANDLE_VALUE)
	{
		CloseHandle(hRxFile);
		hRxFile = INVALID_HANDLE_VALUE;
	}
	CheckDlgButton(UartDebugHwnd,IDC_EnAutoRecvToFile,BST_UNCHECKED);
	CheckDlgButton(UartDebugHwnd,IDC_EnAutoRecvShow,BST_UNCHECKED);	
	UartAutoRecvIsStart = FALSE;
	UartAutoRecvToFile = FALSE;
	if (g_isChinese)
		DbgPrint("<<**<<串口数据接收线程退出,累计接收:%dB",TotalRxCnt);
	else 
		DbgPrint("<<**<<The serial port data receiving thread exits and receives cumulatively:%dB",TotalRxCnt);
	EnableWindow(GetDlgItem(UartDebugHwnd,IDC_Uart_StopRxThread),FALSE);

	return 0;
}

DWORD WINAPI UartTxFileThread(LPVOID lpParameter)
{
	ULONG FlashAddr = 0;
	double BT = 0;
	BOOL RetVal = FALSE;
	CHAR FileName[MAX_PATH]="";
	HANDLE hFile=INVALID_HANDLE_VALUE;
	PUCHAR FileBuf=NULL,RBuf=NULL;
	ULONG FileSize,RLen,UnitSize,RI,WLen;
	double UsedT = 0;
	OPENFILENAME mOpenFile={0};

	if (g_isChinese)
		DbgPrint("*>>*串口发送文件线程启动...");
	else 
		DbgPrint("*>>*Serial port send file thread start...");
	// Fill in the OPENFILENAME structure to support a template and hook.
	mOpenFile.lStructSize = sizeof(OPENFILENAME);
	mOpenFile.hwndOwner         = UartDebugHwnd;
	mOpenFile.hInstance         = AfxMainIns;		
	mOpenFile.lpstrFilter       = "*.*\x0\x0";		
	mOpenFile.lpstrCustomFilter = NULL;
	mOpenFile.nMaxCustFilter    = 0;
	mOpenFile.nFilterIndex      = 0;
	mOpenFile.lpstrFile         = FileName;
	mOpenFile.nMaxFile          = sizeof(FileName);
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
	mOpenFile.lpfnHook 		   = NULL;
	mOpenFile.lpTemplateName    = NULL;
	mOpenFile.Flags             = OFN_SHOWHELP | OFN_EXPLORER | OFN_READONLY | OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&mOpenFile))
	{ 	
		DbgPrint("Uart send data from file:%s",FileName);
	}
	else
		goto Exit;

	if(strlen(FileName) < 1)
	{
		if (g_isChinese)
			DbgPrint("文件无效，请重新选择");
		else 
			DbgPrint("Invalid file, please select again");
		goto Exit;
	}	
	hFile = CreateFile(FileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);//打开文件
	if(hFile == INVALID_HANDLE_VALUE)
	{
		ShowLastError("UartTxFileThread.CreateFile");
		goto Exit;
	}
	FileSize = GetFileSize(hFile,NULL);
	DbgPrint("UartTxFileThread filesize:%d",FileSize);
	TxFileSize = FileSize;
	
	FileBuf = (PUCHAR)malloc(4096);
	if(FileBuf==NULL)
	{
		if (g_isChinese)
			DbgPrint("申请文件内存%dB失败",FileSize);
		else
			DbgPrint("failed to apply for file memory%dB", FileSize);
		goto Exit;
	}
	RBuf = (PUCHAR)malloc(FileSize+128);
	if(RBuf==NULL)
	{
		if (g_isChinese)
			DbgPrint("申请文件内存%dB失败",FileSize);
		else
			DbgPrint("failed to apply for file memory%dB", FileSize);
		goto Exit;
	}

	BT = GetCurrentTimerVal();
	UnitSize = 512;
	RI = 0;
	while(RI<FileSize)
	{		
		if(StopTxThread)
		{
			if (g_isChinese)
				DbgPrint("中止串口写,退出");
			else 
				DbgPrint("Stop serial port writing, exit");
			break;
		}
		if(!UartDevIsOpened)
		{
			if (g_isChinese)
				DbgPrint("串口已关闭，退出");
			else
				DbgPrint("Serial port is closed, exit");
			break;
		}

		if( (FileSize-RI) > UnitSize )
			RLen = UnitSize;
		else
			RLen = FileSize-RI;
		
		SetFilePointer(hFile,RI,NULL,FILE_BEGIN);
		if( !ReadFile(hFile,FileBuf,RLen,&RLen,NULL) )
		{
			ShowLastError("UartTxFileThread.ReadFile");
			goto Exit;
		}
		if(RLen<1)
		{
			if (g_isChinese)
				DbgPrint("UartTxFileThread.已到文件末尾,退出");
			else 
				DbgPrint("UartTxFileThread. End of file, exit");
			break;
		}
		WLen = RLen;
		RetVal = CH347Uart_Write(UartIndex,FileBuf,&WLen);		
		if( !RetVal )
		{
			if (g_isChinese)
				DbgPrint("UartTxFileThread.从%X处写%dB数据失败",RI,WLen);
			else 
				DbgPrint("UartTxFileThread.Failed to write %dB data from%X",RI,WLen);
			break;
		}
		TotalTxCnt += WLen;
		RI += WLen;
		if( ( RLen != WLen ) && (WLen) )
		{
			if (g_isChinese)
				DbgPrint("UartTxFileThread.写数据不完整(0x%X-0x%X)",RLen,WLen);
			else 
				DbgPrint("UartTxFileThread.Incomplete write data(0x%X-0x%X)",RLen,WLen);
			//break;
		}
		UpdateTRxCountShow();
		Sleep(0);
	}
	RetVal = TRUE;
	UsedT = (GetCurrentTimerVal()-BT)/1000;	
	if (g_isChinese)
		DbgPrint("*<<*累计发送%dB,平均速度:%.3fKB/S,用时%.3fS",RI,RI/UsedT/1000,UsedT);
	else 
		DbgPrint("*<<*Cumulative send%dB,Average speed:%.3fKB/S,Time %.3fS",RI,RI/UsedT/1000,UsedT);
Exit:
	if(hFile != INVALID_HANDLE_VALUE) 
	{
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}
	if(FileBuf!=NULL)
		free(FileBuf);

	if (g_isChinese)
		DbgPrint("*<<*串口发送文件线程退出");
	else
		DbgPrint("*<<*Serial port send file thread exit");
	DbgPrint("\r\n");	

	return RetVal;
}

//使能操作按钮，需先打开和配置JTAG，否则无法操作
VOID EnableButtonEnable_Uart()
{
	//更新打开/关闭设备按钮状态
	EnableWindow(GetDlgItem(UartDebugHwnd,IDC_CloseDevice),UartDevIsOpened);
	EnableWindow(GetDlgItem(UartDebugHwnd,IDC_OpenDevice),!UartDevIsOpened);

	EnableWindow(GetDlgItem(UartDebugHwnd,IDC_ObjList),!UartDevIsOpened);
	EnableWindow(GetDlgItem(UartDebugHwnd,IDC_RefreshObjList),!UartDevIsOpened);

	EnableWindow(GetDlgItem(UartDebugHwnd,IDC_Uart_Set),UartDevIsOpened);
	EnableWindow(GetDlgItem(UartDebugHwnd,IDC_Uart_Write),UartDevIsOpened);
	EnableWindow(GetDlgItem(UartDebugHwnd,IDC_Uart_Read),UartDevIsOpened);
}


BOOL APIENTRY DlgProc_UartDbg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case WM_INITDIALOG:
		UartDebugHwnd = hWnd;
		AfxActiveHwnd = hWnd;
		CheckDlgButton(hWnd,IDC_EnablePnPAutoOpen_Uart,BST_CHECKED);
		EnableWindow(GetDlgItem(UartDebugHwnd,IDC_Uart_StopRxThread),FALSE);
		SetEditlInputMode(UartDebugHwnd,IDC_Uart_WriteData,1);
		SetEditlInputMode(UartDebugHwnd,IDC_Uart_ReadData,1);
		Uart_InitParam();		
		//为USB2.0串口设备设置插入和拔出的通知.插入后自动打开设备,拔出后关闭设备
		//if(CH347Uart_SetDeviceNotify(UartIndex,UartUsbID, Uart_UsbDevPnpNotify) )       //设备插拔通知回调函数
		//	DbgPrint("已开启USB设备插拔监视");
		break;	
	//case WM_UartArrive:
	//	DbgPrint("****发现CH347设备插入U口,打开设备");
	//	//先枚举USB设备
	//	SendDlgItemMessage(UartDebugHwnd,IDC_RefreshObjList,BM_CLICK,0,0);
	//	//打开设备
	//	SendDlgItemMessage(UartDebugHwnd,IDC_OpenDevice,BM_CLICK,0,0);
	//	break;
	//case WM_UartRemove:
	//	DbgPrint("****发现CH347已从USB口移除,关闭设备");
	//	//关闭设备
	//	SendDlgItemMessage(UartDebugHwnd,IDC_CloseDevice,BM_CLICK,0,0);
	//	SendDlgItemMessage(UartDebugHwnd,IDC_RefreshObjList,BM_CLICK,0,0);
	//	break;
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
		case IDC_ObjList:
			Uart_ShowDevInfor();
			break;
		case IDC_UartSendFile:
			if(!UartDevIsOpened)
			{
				CheckDlgButton(UartDebugHwnd,IDC_EnAutoRecvToFile,BST_UNCHECKED);
				CheckDlgButton(UartDebugHwnd,IDC_EnAutoRecvShow,BST_UNCHECKED);		
				if (g_isChinese)
					DbgPrint("先打开设备");
				else
					DbgPrint("Open device first");
				break;
			}
			TotalTxCnt = TotalRxCnt = 0;
			TxFileSize = 0;
			StopTxThread = FALSE;			
			CloseHandle(CreateThread(NULL,0,UartTxFileThread,(PVOID)0,0,NULL));
			break;
		case IDC_Uart_StopTxThread:
			StopTxThread = TRUE;
			break;
		case IDC_EnAutoRecvToFile:
		case IDC_EnAutoRecvShow:
			if(wmEvent==BN_CLICKED)
			{
				BOOL UartAutoRecvToFile2;
				if(!UartDevIsOpened)
				{
					CheckDlgButton(UartDebugHwnd,IDC_EnAutoRecvToFile,BST_UNCHECKED);
					CheckDlgButton(UartDebugHwnd,IDC_EnAutoRecvShow,BST_UNCHECKED);					
					if (g_isChinese)
						DbgPrint("先打开设备");
					else
						DbgPrint("Open device first");
					break;
				}			
				UartAutoRecvShow = (IsDlgButtonChecked(UartDebugHwnd,IDC_EnAutoRecvShow)==BST_CHECKED);
				UartAutoRecvToFile2 = (IsDlgButtonChecked(UartDebugHwnd,IDC_EnAutoRecvToFile)==BST_CHECKED);
				if( UartAutoRecvShow | UartAutoRecvToFile2 )
				{
					if(UartAutoRecvToFile2 && (UartAutoRecvToFile2!=UartAutoRecvToFile))
					{
						CHAR FileName[MAX_PATH] = "";
						OPENFILENAME mOpenFile={0};

						UartAutoRecvToFile = UartAutoRecvToFile2;
						// Fill in the OPENFILENAME structure to support a template and hook.
						mOpenFile.lStructSize = sizeof(OPENFILENAME);
						mOpenFile.hwndOwner         = UartDebugHwnd;
						mOpenFile.hInstance         = AfxMainIns;		
						mOpenFile.lpstrFilter       = "*.*\x0\x0";		
						mOpenFile.lpstrCustomFilter = NULL;
						mOpenFile.nMaxCustFilter    = 0;
						mOpenFile.nFilterIndex      = 0;
						mOpenFile.lpstrFile         = FileName;
						mOpenFile.nMaxFile          = sizeof(FileName);
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
							DbgPrint("*>>*UartData will save to %s",FileName);

							hRxFile = CreateFile(FileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_WRITE|FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_ARCHIVE,NULL);
							if(hRxFile==INVALID_HANDLE_VALUE)
							{
								ShowLastError("UartAutoRecvToFIle.CreateFile");
								//goto Exit;
							}
						}
						else
						{
							if( !UartAutoRecvShow )
								break;
						}
					}
					if(!UartAutoRecvIsStart)
					{
						StopRxThread = FALSE;
						CloseHandle(CreateThread(NULL,0,UartAutoRecvToFileThread,(PVOID)0,0,NULL));
					}
				}
				else
					StopRxThread = TRUE;
			}
			break;
		case IDC_Uart_StopRxThread:
			StopRxThread = TRUE;
			break;
		case IDC_RefreshObjList:
			Uart_EnumDevice();
			break;
		case IDC_CloseDevice:
			Uart_CloseDevice();
			break;
		case IDC_OpenDevice:
			Uart_OpenDevice();
			break;
		case IDC_Uart_Set:			
			Uart_Set();
			break;
		case IDC_Uart_Write:
			Uart_Write();
			break;
		case IDC_Uart_Read:
			Uart_Read();
			break;
		case IDC_ClearInfor:
			SetDlgItemText(hWnd,IDC_InforShow,"");
			break;
		case IDC_ResetCnt:
			TotalTxCnt=TotalRxCnt=0;
			SetDlgItemText(hWnd,IDC_TotalCntTRx,"Tx:0,Rx:0");
			break;
		case WM_DESTROY:
			SendDlgItemMessage(hWnd,IDC_CloseDevice,BM_CLICK,0,0);
			//CH347Uart_SetDeviceNotify(UartIndex,UartUsbID,NULL);
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

