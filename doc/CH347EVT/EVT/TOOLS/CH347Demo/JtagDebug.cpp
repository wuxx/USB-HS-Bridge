/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2022                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  CH347 JTAG API Demo
Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2022 Nanjing Qinheng Microelectronics Co., Ltd.
--*/

#include "Main.H"

extern HINSTANCE AfxMainIns; //进程实例
extern HWND AfxActiveHwnd;

UCHAR AfxDataTransType;
HWND JtagDlgHwnd; //主窗体句柄
BOOL JtagDevIsOpened;  //设备是否打开
BOOL JtagIsCfg;
ULONG JtagDevIndex;

mDeviceInforS JtagDevInfor[16] = {0};

#define WM_JtagDevArrive WM_USER+10         //设备插入通知事件,窗体进程接收
#define WM_JtagDevRemove WM_USER+11         //设备拔出通知事件,窗体进程接收

#define JtagUSBID "VID_1A86&PID_55DD&MI_02\0"

/* Brings target into the Debug mode and get device id */
VOID Jtag_InitTarget (VOID)
{
	UCHAR DevID[512] = "0xFF";
	ULONG Len;
	BOOL  RetVal;
	
	CH347Jtag_SwitchTapState(0); //复位 
	//After the reset, the target DEVICE is switched to DEVICE ID DR.
	Len = 4; //4字节
	memset(DevID, 0xFF, sizeof(DevID));
	RetVal = CH347Jtag_ByteReadDR(JtagDevIndex, &Len, &DevID);
	DbgPrint("CH347Jtag_ByteWriteDR %s,read jtag ID: %08X\r\n",RetVal?"succ":"failure",DevID);

	Sleep(100); 

	CH347Jtag_SwitchTapState(0); //复位
	Len = 4*8; //32位
	memset(DevID, 0xFF, sizeof(DevID));
	RetVal = CH347Jtag_BitReadDR(JtagDevIndex, &Len, &DevID);
	DbgPrint("CH347Jtag_BitWriteDR  %s,read jtag ID: %08X\r\n",RetVal?"succ":"failure",DevID);
}

//JTAT IR/DR data read and write. Read and write by byte. State machine from run-test-idle->; IR/DR-&gt; Exit IR/DR -&gt; Run-Test-Idle
BOOL Jtag_DataRW_Byte(BOOL IsRead)
{
	BOOL RetVal = FALSE;
	BOOL IsDr;
	ULONG StrLen,i,OutLen=0,DLen=0,InLen=0;
	CHAR ValStr[64]="";
	PCHAR FmtStr=NULL;
	PUCHAR OutBuf=NULL,InBuf=NULL;
	double UsedT = 0,BT = 0;

	IsDr = (SendDlgItemMessage(JtagDlgHwnd,IDC_JtaShiftgChannel,CB_GETCURSEL,0,0) == 1);

	DbgPrint("**>>**Jtag_DataRW_Byte");
	if(!JtagDevIsOpened)
	{
		DbgPrint("Deivce is not opened.");
		goto Exit;
	}
	if(IsRead) //JTAG read, apply for read cache
	{
		InLen = GetDlgItemInt(JtagDlgHwnd,IDC_JtagInLen,NULL,FALSE);	
		if(InLen<1)
		{
			DbgPrint("No read length is specified");
			goto Exit;
		}		
		InBuf = (PUCHAR)malloc(InLen+64);
		if(InBuf==NULL)
		{
			DbgPrint("Failed to apply for the read cache. Procedure");
			goto Exit;
		}
	}
	else //JTAG write, get write data length
	{	
		StrLen = GetWindowTextLength(GetDlgItem(JtagDlgHwnd,IDC_JtagOut))+1;
		if(StrLen<2)
		{
			DbgPrint("No write data is entered");
			goto Exit;
		}
		OutLen = (StrLen+2)/3; //Three characters make a byte

		OutBuf = (PUCHAR)malloc(OutLen+64);
		if(OutBuf==NULL)
		{
			DbgPrint("Failed to apply for write cache. Procedure");
			goto Exit;
		}
	}		
	{//申请字符转数据缓存
		if(!IsRead)
		{
			FmtStr = (PCHAR)malloc(OutLen*3+64);
			memset(FmtStr,0,OutLen*3+64);
		}
		else
		{
			FmtStr = (PCHAR)malloc(InLen*3+64);
			memset(FmtStr,0,InLen*3+64);
		}
		if(FmtStr==NULL)
		{
			DbgPrint("Failed to apply for data conversion cache");
			goto Exit;
		}
	}
	if(!IsRead)//Obtain data to be written from the interface. Converts hexadecimal characters to hexadecimal data
	{
		StrLen = GetDlgItemText(JtagDlgHwnd,IDC_JtagOut,FmtStr,OutLen*3+1);
		if((StrLen%3)==1)
		{
			FmtStr[StrLen] = '0';
			FmtStr[StrLen+1] = ' ';
			StrLen += 2;
		}
		else if((StrLen%3)==2)
		{
			FmtStr[StrLen] = ' ';
			StrLen += 1;
		}
		SetDlgItemText(JtagDlgHwnd,IDC_JtagOut,FmtStr);		
		StrLen = GetDlgItemText(JtagDlgHwnd,IDC_JtagOut,FmtStr,StrLen+2);
		OutLen = 0;
		for(i=0;i<StrLen;i+=3)
		{		
			memcpy(&ValStr[0],&FmtStr[i],2);
			OutBuf[OutLen] = (UCHAR)mStrToHEX(ValStr);
			OutLen++;
		}
		SetDlgItemInt(JtagDlgHwnd,IDC_JtagOutLen,OutLen,FALSE);
	}
	
	BT = GetCurrentTimerVal();
	if(IsRead)
	{		
		if(IsDr)
			RetVal = CH347Jtag_ByteReadDR(JtagDevIndex,&InLen,InBuf);
		else
			RetVal = CH347Jtag_ByteReadIR(JtagDevIndex,&InLen,InBuf);
	}
	else 
	{
		if(IsDr)
			RetVal = CH347Jtag_ByteWriteDR(JtagDevIndex,OutLen,OutBuf);
		else
			RetVal = CH347Jtag_ByteWriteIR(JtagDevIndex,OutLen,OutBuf);
	}
	UsedT = GetCurrentTimerVal()-BT;	

	if(InLen)
	{//打印
		//Hexadecimal display
		for(i=0;i<InLen;i++)
		{
			sprintf(&FmtStr[strlen(FmtStr)],"%02X ",InBuf[i]);	
		}
		SetDlgItemText(JtagDlgHwnd,IDC_JtagIn,FmtStr);		
		SetDlgItemInt(JtagDlgHwnd,IDC_JtagInLen,InLen,FALSE);		
	}
	if(IsRead)
	{		
		DbgPrint("**<<**Jtag_Data %s %s read %dB/%dB,UsedTime:%fms,Speed:%.3fKB",IsDr?"DR":"IR",RetVal?"succ":"failure.",InLen,InLen,UsedT,InLen/UsedT);
	}
	else
	{
		DbgPrint("**<<**Jtag_Data %s %s,write %dB/%dB,UsedTime:%fms,Speed:%.3fKB",IsDr?"DR":"IR",RetVal?"succ":"failure.",OutLen,OutLen,UsedT,OutLen/UsedT);
	}

Exit:
	if(InBuf)
		free(InBuf);
	if(OutBuf)
		free(OutBuf);
	if(FmtStr)
		free(FmtStr);
	return RetVal;;
}

//JTAT IR/DR data read and write. Read and write by bit. State machine from run-test-idle->; IR/DR-&gt; Exit IR/DR -&gt; Run-Test-Idle
BOOL Jtag_DataRW_Bit(BOOL IsRead)
{
	BOOL RetVal = FALSE;
	BOOL IsDr;
	ULONG i,OutBitLen=0,DLen=0,InBitLen=0,BitCount,BI;
	CHAR ValStr[64]="";
	CHAR FmtStr[4096]="";
	UCHAR OutBuf[4096]="",InBuf[4096]="";	
	double UsedT = 0,BT = 0;

	IsDr = (SendDlgItemMessage(JtagDlgHwnd,IDC_JtaShiftgChannel,CB_GETCURSEL,0,0) == 1);	

	DbgPrint("**>>**Jtag_DataRW_Bit");
	if(!JtagDevIsOpened)
	{
		DbgPrint("Deivce is not opened.");
		goto Exit;
	}
	if(IsRead) //JTAG read
	{
		InBitLen = GetDlgItemInt(JtagDlgHwnd,IDC_JtagInBitLen,NULL,FALSE);	
		if(InBitLen<1)
		{
			DbgPrint("No read length is specified");
			goto Exit;
		}
	}
	else //JTAG write
	{		
		BitCount = GetDlgItemText(JtagDlgHwnd,IDC_JtagOutBit,FmtStr,sizeof(FmtStr));
		OutBitLen = GetDlgItemInt(JtagDlgHwnd,IDC_JtagOutBitLen,NULL,FALSE);	

		if( (BitCount<1) && (OutBitLen<1) )
		{
			DbgPrint("No write data is entered");
			goto Exit;
		}
		if(BitCount<OutBitLen) //If the actual input digit is insufficient, 0 will be added
		{
			memmove(&FmtStr[OutBitLen-BitCount],&FmtStr[0],BitCount);
			memset(&FmtStr[0],'0',OutBitLen-BitCount);
			SetDlgItemText(JtagDlgHwnd,IDC_JtagOutBit,FmtStr);			
		}
		BitCount = GetDlgItemText(JtagDlgHwnd,IDC_JtagOutBit,FmtStr,sizeof(FmtStr));
		if(BitCount>OutBitLen)
		{
			memmove(&FmtStr[0],&FmtStr[BitCount-OutBitLen],OutBitLen);
			BitCount = OutBitLen;
		}
		BI = 0;
		//LSB方式:
		for(i=0;i<BitCount;i++)
		{
			if( (i>0)&& ((i%8)==0) )
			{
				DbgPrint("OutBit[%d]:%02X",BI,OutBuf[BI]);
				BI++;				
			}
			if(FmtStr[BitCount-i-1]=='1')
				OutBuf[BI] |= (1<<(i%8))&0xFF;	
			else if(FmtStr[BitCount-i-1]=='0')
				OutBuf[BI] &= ~(1<<(i%8))&0xFF;	
			else
			{
				DbgPrint("invalid bit input.");
				return FALSE;
			}			
		}		
		{//debug
			DbgPrint("OutBit[%d]:%02X",BI,OutBuf[BI]);
		}
	}	
	BT = GetCurrentTimerVal();
	if(IsRead)
	{
		BitCount = GetDlgItemInt(JtagDlgHwnd,IDC_JtagInBitLen,NULL,FALSE);	
		if(IsDr)
			RetVal = CH347Jtag_BitReadDR(JtagDevIndex,&InBitLen,InBuf);
		else
			RetVal = CH347Jtag_BitReadIR(JtagDevIndex,&InBitLen,InBuf);
	}
	else 
	{
		//位带方式
		//RetVal = JTAG_WriteRead(0,IsDr,BitCount,OutBuf,NULL,NULL);
		if(IsDr)
			RetVal = CH347Jtag_BitWriteDR(JtagDevIndex,OutBitLen,OutBuf);
		else
			RetVal = CH347Jtag_BitWriteIR(JtagDevIndex,OutBitLen,OutBuf);
	}
	UsedT = GetCurrentTimerVal()-BT;
		
	if(InBitLen)
	{//打印
		for(i=0;i<InBitLen;i++)
		{			
			if( InBuf[i/8] & (1<<(i%8)) )
				FmtStr[InBitLen-i-1] = '1';
			else
				FmtStr[InBitLen-i-1] = '0';			
		}
		SetDlgItemText(JtagDlgHwnd,IDC_JtagInBit,FmtStr);
		SetDlgItemInt(JtagDlgHwnd,IDC_JtagInBitLen,InBitLen,FALSE);
	}
	SetDlgItemInt(JtagDlgHwnd,IDC_JtagInBitLen,InBitLen,FALSE);		
	
	if(IsRead)
	{		
		DbgPrint("**<<**Jtag_Data %s %s read %dB/%dB,UsedTime:%fms,Speed:%.3fKB",IsDr?"DR":"IR",RetVal?"succ":"failure.",InBitLen,InBitLen,UsedT,InBitLen/UsedT);
	}
	else
	{
		DbgPrint("**<<**Jtag_Data %s %s,write %dB/%dB,UsedTime:%fms,Speed:%.3fKB",IsDr?"DR":"IR",RetVal?"succ":"failure.",OutBitLen,OutBitLen,UsedT,OutBitLen/UsedT);
	}

Exit:
	return RetVal;
}

// JTAG port firmware download, only demonstrates the fast file transfer from the DR port, not specific model. 
// When the JTAG speed is set to 4, the Jtag_WriteRead_Fast function is called, and the download speed can reach about 4MB/S
DWORD WINAPI DownloadFwFile(LPVOID lpParameter)
{
	CHAR FileName[MAX_PATH] = "",FmtStr[512]="";
	OPENFILENAME mOpenFile={0};
	ULONG TestLen,RLen;
	PUCHAR FileBuf=NULL;
	double BT,UsedT;
	BOOL RetVal = FALSE;
	HANDLE hFile=INVALID_HANDLE_VALUE;
	UCHAR RBuf[4096] = "";
	BOOL IsDr;
	
	// Fill in the OPENFILENAME structure to support a template and hook.
	mOpenFile.lStructSize = sizeof(OPENFILENAME);
	mOpenFile.hwndOwner         = JtagDlgHwnd;
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
		DbgPrint("Write data via jtag from:%s",FileName);
	}
	else goto Exit;

	hFile = CreateFile(FileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_WRITE|FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_ARCHIVE,NULL);
	if(hFile==INVALID_HANDLE_VALUE)
	{
		ShowLastError("DownloadFwFile.OpenFile");
		goto Exit;
	}
	TestLen = GetFileSize(hFile,NULL);
	if(TestLen<1)
	{
		DbgPrint("DownloadFwFile.File is empty");
		goto Exit;
	}
	DbgPrint("*>>*DownloadFwFile.Flash Write %dB",TestLen);
	FileBuf = (PUCHAR)malloc(TestLen+64);
	if(FileBuf==NULL)
	{
		DbgPrint("DownloadFwFile.malloc failure");
		goto Exit;
	}
	memset(FileBuf,0,TestLen+64);
	RLen = TestLen;
	if( !ReadFile(hFile,FileBuf,RLen,&RLen,NULL) )
	{
		ShowLastError("DownloadFwFile.Read file");
		goto Exit;
	}
	if(RLen!=TestLen)
	{
		DbgPrint("DownloadFwFile.ReadFlashFile len err(%d-%d)",RLen,TestLen);
		goto Exit;
	}	

	IsDr = (SendDlgItemMessage(JtagDlgHwnd,IDC_JtaShiftgChannel,CB_GETCURSEL,0,0) == 1);
	DbgPrint("*>>*WriteFlashFromFile.Flash block write");	
	BT = GetCurrentTimerVal();	

	AfxDataTransType = (UCHAR)SendDlgItemMessage(JtagDlgHwnd,IDC_DataTransFunc,CB_GETCURSEL,0,0);
	if(AfxDataTransType)  //DEBUG
	{
		RetVal = CH347Jtag_BitWriteDR(JtagDevIndex,TestLen*8,FileBuf);
	}
	else
	{
		RetVal = CH347Jtag_ByteWriteDR(JtagDevIndex,TestLen,FileBuf);
	}

	if(!RetVal)
		DbgPrint("DownloadFwFile.Write 0x%X] failure",TestLen);	
	UsedT = (GetCurrentTimerVal()-BT)/1000; //Download time, in seconds

	sprintf(FmtStr,"*<<*DownloadFwFile. Write %d bytes %s. Average speed :%.3fKB/ s, total time %.3fs ",TestLen,RetVal?" Success ":" failure ", TestLen/UsedT / 1000, UsedT);
	OutputDebugString(FmtStr);
	DbgPrint("*<<*DownloadFwFile. Write %d bytes %s. Average speed :%.3fKB/ s, total time %.3fs ",TestLen,RetVal?" Success ":" failure ", TestLen/UsedT / 1000, UsedT);

Exit:
	if(FileBuf!=NULL)
		free(FileBuf);
	if(hFile!=INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	return RetVal;
}

//jtag interface config
BOOL Jtag_InterfaceConfig()
{
    UCHAR iClockRate;    //Communication speed; The value ranges from 0 to 5. A larger value indicates a faster communication speed
	
	if(!JtagDevIsOpened)
	{
		DbgPrint("Deivce is not opened.");
		return FALSE;
	}	
	iClockRate = (UCHAR)SendDlgItemMessage(JtagDlgHwnd,IDC_JtagClockRate,CB_GETCURSEL,0,0);

	JtagIsCfg = CH347Jtag_INIT(JtagDevIndex,iClockRate);
	DbgPrint("CH347JTAG_INIT %s.iClockRate=%X",JtagIsCfg?"succ":"failure",iClockRate);
	return JtagIsCfg;
}


//初始化窗体
VOID Jtag_InitWindows()
{
	//查找并显示设备列表 
	SendDlgItemMessage(JtagDlgHwnd,IDC_RefreshObjList,BM_CLICK,0,0);	
	
	EnableWindow(GetDlgItem(JtagDlgHwnd,IDC_OpenDevice),!JtagDevIsOpened);
	EnableWindow(GetDlgItem(JtagDlgHwnd,IDC_CloseDevice),JtagDevIsOpened);	
	//输出框设置显示的最大字符数
	SendDlgItemMessage(JtagDlgHwnd,IDC_InforShow,EM_LIMITTEXT,0xFFFFFFFF,0);

	{
		SendDlgItemMessage(JtagDlgHwnd,IDC_JtagTapState,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Test-Logic Reset");
		SendDlgItemMessage(JtagDlgHwnd,IDC_JtagTapState,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Run-Test/Idle");
		SendDlgItemMessage(JtagDlgHwnd,IDC_JtagTapState,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Run-Test/Idle -> Shift-DR");
		SendDlgItemMessage(JtagDlgHwnd,IDC_JtagTapState,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Shift-DR -> Run-Test/Idle");
		SendDlgItemMessage(JtagDlgHwnd,IDC_JtagTapState,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Run-Test/Idle -> Shift-IR");
		SendDlgItemMessage(JtagDlgHwnd,IDC_JtagTapState,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Shift-IR -> Run-Test/Idle");
		SendDlgItemMessage(JtagDlgHwnd,IDC_JtagTapState,CB_SETCURSEL,0,0);
	}
	{
		SendDlgItemMessage(JtagDlgHwnd,IDC_JtaShiftgChannel,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"  IR");
		SendDlgItemMessage(JtagDlgHwnd,IDC_JtaShiftgChannel,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"  DR");			
		SendDlgItemMessage(JtagDlgHwnd,IDC_JtaShiftgChannel,CB_SETCURSEL,1,0);
	}		
	{			
		SendDlgItemMessage(JtagDlgHwnd,IDC_DataTransFunc,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Byte 方式");
		SendDlgItemMessage(JtagDlgHwnd,IDC_DataTransFunc,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"bit  方式");
		SendDlgItemMessage(JtagDlgHwnd,IDC_DataTransFunc,CB_SETCURSEL,0,0);
	}
	SetFocus(GetDlgItem(JtagDlgHwnd,IDC_JtaShiftgChannel)); //更新操作按钮状态

	{
		//JTAG速度配置值；有效值为0-5，值越大通信速度越快；
		SendDlgItemMessage(JtagDlgHwnd,IDC_JtagClockRate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)" 1.875MHz");    //速度0：1.875MHz；
		SendDlgItemMessage(JtagDlgHwnd,IDC_JtagClockRate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"  3.75MHz");    //速度1： 3.75MHz；
		SendDlgItemMessage(JtagDlgHwnd,IDC_JtagClockRate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"   7.5MHz");    //速度3：  7.5MHz；
		SendDlgItemMessage(JtagDlgHwnd,IDC_JtagClockRate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"    15MHz");    //速度4：   15MHz；
		SendDlgItemMessage(JtagDlgHwnd,IDC_JtagClockRate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"    30MHz");    //速度5：   30MHz；
		SendDlgItemMessage(JtagDlgHwnd,IDC_JtagClockRate,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"    60MHz");    //速度6：   60MHz；
		SendDlgItemMessage(JtagDlgHwnd,IDC_JtagClockRate,CB_SETCURSEL,4,0);
	}	
	SetEditlInputMode(JtagDlgHwnd,IDC_JtagOut,1);
	SetEditlInputMode(JtagDlgHwnd,IDC_JtagIn,1);	

	SetDlgItemText(JtagDlgHwnd,IDC_JtagOutBitLen,"0");
	SetDlgItemText(JtagDlgHwnd,IDC_JtagInBitLen,"0");
	SetDlgItemText(JtagDlgHwnd,IDC_JtagOutLen,"0");
	SetDlgItemText(JtagDlgHwnd,IDC_JtagOutLen,"0");		

	return;
}

//使能操作按钮，需先打开和配置JTAG，否则无法操作
VOID Jtag_EnableButtonEnable()
{
	if(!JtagDevIsOpened)
		JtagIsCfg = FALSE;

	EnableWindow(GetDlgItem(JtagDlgHwnd,IDC_JtagIfConfig),JtagDevIsOpened);

	//更新打开/关闭设备按钮状态
	EnableWindow(GetDlgItem(JtagDlgHwnd,IDC_OpenDevice),!JtagDevIsOpened);
	EnableWindow(GetDlgItem(JtagDlgHwnd,IDC_CloseDevice),JtagDevIsOpened);

	EnableWindow(GetDlgItem(JtagDlgHwnd,IDC_ObjList),!JtagDevIsOpened);
	EnableWindow(GetDlgItem(JtagDlgHwnd,IDC_RefreshObjList),!JtagDevIsOpened);

	EnableWindow(GetDlgItem(JtagDlgHwnd,IDC_Jtag_ByteWrite),JtagIsCfg);
	EnableWindow(GetDlgItem(JtagDlgHwnd,IDC_Jtag_ByteRead),JtagIsCfg);
	EnableWindow(GetDlgItem(JtagDlgHwnd,IDC_Jtag_BitWrite),JtagIsCfg);
	EnableWindow(GetDlgItem(JtagDlgHwnd,IDC_Jtag_BitRead),JtagIsCfg);

	EnableWindow(GetDlgItem(JtagDlgHwnd,IDC_Jtag_DnFile_Exam),JtagIsCfg);
	EnableWindow(GetDlgItem(JtagDlgHwnd,IDC_Jtag_InitTarget),JtagIsCfg);
	EnableWindow(GetDlgItem(JtagDlgHwnd,IDC_JtagState_Switch),JtagIsCfg);
}

// USB设备插拔检测通知程序.因回调函数对函数操作有限制，通过窗体消息转移到消息处理函数内进行处理。
VOID	 CALLBACK	 Jtag_UsbDevPnpNotify (ULONG iEventStatus ) 
{
	if(iEventStatus==CH347_DEVICE_ARRIVAL)// 设备插入事件,已经插入
		PostMessage(JtagDlgHwnd,WM_JtagDevArrive,0,0);
	else if(iEventStatus==CH347_DEVICE_REMOVE)// 设备拔出事件,已经拔出
		PostMessage(JtagDlgHwnd,WM_JtagDevRemove,0,0);
	return;
}

//显示设备信息
BOOL Jtag_ShowDevInfor()
{
	ULONG ObjSel;
	CHAR  FmtStr[128]="";

	ObjSel = SendDlgItemMessage(JtagDlgHwnd,IDC_ObjList,CB_GETCURSEL,0,0);
	if(ObjSel!=CB_ERR)
	{
		sprintf(FmtStr,"**ChipMode:%d,%s,Ver:%02X,DevID:%s",JtagDevInfor[ObjSel].ChipMode,JtagDevInfor[ObjSel].UsbSpeedType?"HS":"FS",JtagDevInfor[ObjSel].FirewareVer,JtagDevInfor[ObjSel].DeviceID);
		SetDlgItemText(JtagDlgHwnd,IDC_DevInfor_Jtag,FmtStr);
	}
	return (ObjSel!=CB_ERR);
}

//枚举设备
ULONG Jtag_EnumDevice()
{
	ULONG i,oLen,DevCnt = 0;
	USB_DEVICE_DESCRIPTOR DevDesc = {0};
	CHAR tem[256] = "";
	mDeviceInforS DevInfor = {0};

	SendDlgItemMessage(JtagDlgHwnd,IDC_ObjList,CB_RESETCONTENT,0,0);	
	for(i=0;i<16;i++)
	{
		if(CH347OpenDevice(i) != INVALID_HANDLE_VALUE)
		{
			oLen = sizeof(USB_DEVICE_DESCRIPTOR);
			CH347GetDeviceInfor(i,&DevInfor);
			if(DevInfor.ChipMode != 3) //JTAG接口只有模式3支持
				continue;
			sprintf(tem,"%d# %s",i,DevInfor.FuncDescStr);
			SendDlgItemMessage(JtagDlgHwnd,IDC_ObjList,CB_ADDSTRING,0,(LPARAM)(LPCTSTR)tem);		
			memcpy(&JtagDevInfor[DevCnt],&DevInfor,sizeof(DevInfor));
			DevCnt++;
		}
		CH347CloseDevice(i);
	}
	if(DevCnt)
	{
		SendDlgItemMessage(JtagDlgHwnd,IDC_ObjList,CB_SETCURSEL,0,0);
		SetFocus(GetDlgItem(JtagDlgHwnd,IDC_ObjList));
	}
	return DevCnt;
}

//窗体进程
BOOL APIENTRY DlgProc_JtagDebug(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	ULONG ThreadID;

	switch (message)
	{
	case WM_INITDIALOG:
		JtagDlgHwnd = hWnd;
		AfxActiveHwnd = hWnd;
		JtagDevIsOpened = FALSE;		
		Jtag_InitWindows(); //初始化窗体
		CheckDlgButton(hWnd,IDC_EnablePnPAutoOpen_Jtag,BST_CHECKED);
		Jtag_EnableButtonEnable(); //使能操作按钮，需先打开和配置JTAG，否则无法操作
		//为USB2.0JTAG设备设置插入和拔出的通知.插入后自动打开设备,拔出后关闭设备
		//CH347SetDeviceNotify(0,JtagUSBID,Jtag_UsbDevPnpNotify);       //设备插拔通知回调函数
		break;	
	//case WM_JtagDevArrive:
	//		DbgPrint("****发现CH347设备插入USB口,打开设备");
	//		//先枚举USB设备
	//		SendDlgItemMessage(JtagDlgHwnd,IDC_RefreshObjList,BM_CLICK,0,0);
	//		//打开设备
	//		SendDlgItemMessage(JtagDlgHwnd,IDC_OpenDevice,BM_CLICK,0,0);
	//		break;
	//case WM_JtagDevRemove:
	//	DbgPrint("****发现CH347(JTAG/I2C)已从USB口移除,关闭设备");
	//	//关闭设备
	//	SendDlgItemMessage(JtagDlgHwnd,IDC_CloseDevice,BM_CLICK,0,0);
	//	SendDlgItemMessage(JtagDlgHwnd,IDC_RefreshObjList,BM_CLICK,0,0);
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
		case IDC_RefreshObjList:
			Jtag_EnumDevice(); //枚举并显示设备
			break;
		case IDC_ObjList:
			Jtag_ShowDevInfor();
			break;
		case IDC_OpenDevice://打开设备
			{
				//获取设备序号
				JtagDevIndex = SendDlgItemMessage(hWnd,IDC_ObjList,CB_GETCURSEL,0,0);
				if(JtagDevIndex==CB_ERR)
				{
					DbgPrint("打开设备失败,请先选择设备");
					break; //退出
				}
				DbgPrint(">>设备打开...");
				JtagDevIsOpened = (CH347OpenDevice(JtagDevIndex)!=INVALID_HANDLE_VALUE);
				if( JtagDevIsOpened ) //打开设备
				{
					CH347SetTimeout(JtagDevIndex,500,500);
					DbgPrint("          成功");
				}
				else
					DbgPrint("          失败");
				Jtag_EnableButtonEnable();//更新按钮状态				
			}
			break;
		case IDC_CloseDevice:
			{				
				DbgPrint(">>设备关闭...");				
				if( !CH347CloseDevice(JtagDevIndex) )
					DbgPrint("          失败");
				else
				{
					DbgPrint("          成功");
					JtagDevIsOpened = FALSE;
				}
				//更新按钮状态
				Jtag_EnableButtonEnable();	
				JtagDevIndex = CB_ERR;
			}
			break;		
		case IDC_Jtag_BitWrite:
			Jtag_DataRW_Bit(FALSE); //JTAG写
			break;
		case IDC_Jtag_BitRead:
			Jtag_DataRW_Bit(TRUE); //JTAG读
			break;
		case IDC_Jtag_ByteWrite:
			Jtag_DataRW_Byte(FALSE); //JTAG写
			break;
		case IDC_Jtag_ByteRead:
			Jtag_DataRW_Byte(TRUE); //JTAG读
			break;
		case IDC_Jtag_DnFile_Exam:
			CloseHandle(CreateThread(NULL,0,DownloadFwFile,NULL,0,&ThreadID)); //JTAG写文件
			break;
		case IDC_JtagState_Switch:
			{
				UCHAR TapState;
				BOOL RetVal = FALSE;
				CHAR StatusStr[64]="";

				TapState = (UCHAR)SendDlgItemMessage(JtagDlgHwnd,IDC_JtagTapState,CB_GETCURSEL,0,0);
				RetVal = CH347Jtag_SwitchTapState(TapState);
				GetDlgItemText(JtagDlgHwnd,IDC_JtagTapState,StatusStr,sizeof(StatusStr));

				DbgPrint("Jtag_SwitchState %s.%s",RetVal?"succ":"failure",StatusStr);
			}
			break;
		case IDC_Jtag_InitTarget:
			Jtag_InitTarget();
			break;
		case IDC_JtaShiftgChannel:
			{
				CHAR BtName[64]="";
				ULONG DataReg;

				DataReg = SendDlgItemMessage(JtagDlgHwnd,IDC_JtaShiftgChannel,CB_GETCURSEL,0,0);

				sprintf(BtName,"JTAG位读%s(D2)",DataReg?"DR":"IR");
				SetDlgItemText(JtagDlgHwnd,IDC_Jtag_BitRead,BtName);

				sprintf(BtName,"JTAG位写%s(D1)",DataReg?"DR":"IR");
				SetDlgItemText(JtagDlgHwnd,IDC_Jtag_BitWrite,BtName);

				sprintf(BtName,"JTAG字节读%s(D4)",DataReg?"DR":"IR");
				SetDlgItemText(JtagDlgHwnd,IDC_Jtag_ByteRead,BtName);

				sprintf(BtName,"JTAG字节写%s(D3)",DataReg?"DR":"IR");
				SetDlgItemText(JtagDlgHwnd,IDC_Jtag_ByteWrite,BtName);
			}
			break;
		case IDC_JtagIfConfig:
			Jtag_InterfaceConfig();
			Jtag_EnableButtonEnable(); //配置成功后，开启相关按钮操作
			break;
		case IDC_DataTransFunc:
			AfxDataTransType = (UCHAR)SendDlgItemMessage(JtagDlgHwnd,IDC_DataTransFunc,CB_GETCURSEL,0,0);
			break;
		case IDC_ClearInfor:
			SetDlgItemText(hWnd,IDC_InforShow,"");
			break;
		case WM_DESTROY:			
			SendDlgItemMessage(hWnd,IDC_CloseDevice,BM_CLICK,0,0);
			//CH347SetDeviceNotify(0,JtagUSBID,NULL);
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;		
		case WM_DESTROY:
			PostQuitMessage(0);
			break;		
	}
	return 0;
}