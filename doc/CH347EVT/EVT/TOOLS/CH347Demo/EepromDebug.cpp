/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2022                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  基于CH347 I2C接口函数操作FLASH应用示例，EEPROM读写、内容读至文件、文件
  写入等

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
extern HWND FlashEepromDbgHwnd;     //与FLASH合用一窗体
extern HWND AfxDlgFlashEepromDbgHwnd;
EEPROM_TYPE EepromType;
BOOL   IsInitI2C;
extern BOOL FlashDevIsOpened;  //设备是否打开;
extern HINSTANCE AfxMainIns; //进程实例
extern ULONG DevIndex;
ULONG EepromCapacity;

VOID InitWindows_Eeprom()
{
	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_EepromType,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"ID_24C01");	
	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_EepromType,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"ID_24C02");
	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_EepromType,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"ID_24C04");
	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_EepromType,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"ID_24C08");
	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_EepromType,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"ID_24C16");
	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_EepromType,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"ID_24C32");
	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_EepromType,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"ID_24C64");
	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_EepromType,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"ID_24C128");
	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_EepromType,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"ID_24C256");
	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_EepromType,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"ID_24C512");
	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_EepromType,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"ID_24C1024");
	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_EepromType,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"ID_24C2048");
	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_EepromType,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"ID_24C4096");
	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_EepromType,CB_SETCURSEL,1,0);

	SetDlgItemInt(FlashEepromDbgHwnd,IDC_EepromStartAddr,0,FALSE);
	SetDlgItemInt(FlashEepromDbgHwnd,IDC_EepromDataSize,100,FALSE);	
	SetEditlInputMode(FlashEepromDbgHwnd,IDC_EepromData,1);
}

BOOL Eeprom_InitI2C()
{	
	IsInitI2C = TRUE;
	
	EepromType = (EEPROM_TYPE)SendDlgItemMessage(FlashEepromDbgHwnd,IDC_EepromType,CB_GETCURSEL,0,0);
	EepromCapacity = (EepromType+1)*1024/8;
	return TRUE;
}

BOOL EerpromRead()
{
	double BT,UseT;
	ULONG DataLen,Addr=0,i;
	UCHAR DBuf[8192] = {0};
	CHAR FmtStr[512] = "",FmtStr1[8*1024*3+16]="";
	BOOL RetVal;

	if(!FlashDevIsOpened)
	{
		DbgPrint("请先打开设备");
		return FALSE;
	}

	//获取FLASH读的起始地址
	GetDlgItemText(FlashEepromDbgHwnd,IDC_EepromStartAddr,FmtStr,32);
	Addr = mStrToHEX(FmtStr);
	//获取FLASH读的字节数,十六进制
	GetDlgItemText(FlashEepromDbgHwnd,IDC_EepromDataSize,FmtStr,32);
	DataLen = mStrToHEX(FmtStr);
	if(DataLen<1)
	{
		DbgPrint("请输入Eeprom数据操作长度");
		return FALSE;
	}
	else if(DataLen>(8*1024*3)) //显示限制
	{
		DbgPrint("请输入小于0x%X的Flash数据操作长度",8*1024);
		return FALSE;
	}

	BT = GetCurrentTimerVal();
	RetVal = CH347ReadEEPROM(DevIndex,EepromType,Addr,DataLen,DBuf);
	UseT = GetCurrentTimerVal()-BT;

	if(!RetVal)
		DbgPrint(">>Eeprom读:从[%X]地址开始读入%d字节...失败.",Addr,DataLen);
	else
	{	
		DbgPrint(">>Eeprom读:从[%X]地址开始读入%d字节...成功.用时%.3fS",Addr,DataLen,UseT/1000);
		{//显示FLASH数据,16进制显示
			for(i=0;i<DataLen;i++)		
				sprintf(&FmtStr1[strlen(FmtStr1)],"%02X ",DBuf[i]);						
			SetDlgItemText(FlashEepromDbgHwnd,IDC_EepromData,FmtStr1);
		}
	}
	return TRUE;
}

//Flash块数据写
BOOL EepromWrite()
{
	ULONG DataLen,Addr=0,i,StrLen;
	UCHAR DBuf[8*1024+16] = {0};
	CHAR FmtStr[8*1024*3+16] = "",ValStr[16]="";
	double BT,UseT;
	BOOL RetVal;

	//获取写FLASH的起始地址,十六进制
	GetDlgItemText(FlashEepromDbgHwnd,IDC_EepromStartAddr,FmtStr,32);
	Addr = mStrToHEX(FmtStr);				

	//获取写FLASH的字节数,十六进制
	DataLen = 0;
	StrLen = GetDlgItemText(FlashEepromDbgHwnd,IDC_EepromData,FmtStr,sizeof(FmtStr));	
	for(i=0;i<StrLen;i+=3)
	{		
		memcpy(&ValStr[0],&FmtStr[i],2);

		DBuf[DataLen] = (UCHAR)mStrToHEX(ValStr);
		DataLen++;
	}
	GetDlgItemText(FlashEepromDbgHwnd,IDC_EepromDataSize,FmtStr,32);
	i = mStrToHEX(FmtStr);		
	if(i<DataLen)
		DataLen = i;
	
	BT = GetCurrentTimerVal();
	RetVal = CH347WriteEEPROM(DevIndex,EepromType,Addr,DataLen,DBuf);
	UseT = GetCurrentTimerVal()-BT;
	if(!RetVal)
		DbgPrint(">>Eeprom写:从[%X]地址开始写入%d字节...失败",Addr,DataLen);
	else
		DbgPrint(">>Eeprom写:从[%X]地址开始写入%d字节...成功.用时%.3fS",Addr,DataLen,UseT/1000);
	sprintf(FmtStr,"%X",DataLen);
	SetDlgItemText(FlashEepromDbgHwnd,IDC_EepromDataSize,FmtStr);

	return TRUE;
}

//显示检验数据出错内容
VOID DumpDataBuf1(ULONG Addr,PUCHAR Buf1,PUCHAR SampBuf2,ULONG DataLen1,ULONG ErrLoca)
{
	CHAR FmtStr1[8192*3] = "",FmtStr2[8192*3] = "";
	ULONG i;

	memset(FmtStr1,0,sizeof(FmtStr1));	
	memset(FmtStr2,0,sizeof(FmtStr2));	
	//16进制显示
	for(i=0;i<DataLen1;i++)
	{
		if( ((i%16)==0) && (i>0) )
		{
			AddStrToEdit(FlashEepromDbgHwnd,IDC_InforShow,"Data[%08X]:%s\n",i-16+Addr,FmtStr1);
			AddStrToEdit(FlashEepromDbgHwnd,IDC_InforShow,"Samp[%08X]:%s\n",i-16+Addr,FmtStr2);
			memset(FmtStr1,0,16*4);
			memset(FmtStr2,0,16*4);
			if(ErrLoca==i)
			{
				sprintf(&FmtStr1[strlen(FmtStr1)],"[%02X] ",Buf1[i]);						
				sprintf(&FmtStr2[strlen(FmtStr2)],"[%02X] ",SampBuf2[i]);						
			}
			else
			{
				sprintf(&FmtStr1[strlen(FmtStr1)],"%02X ",Buf1[i]);						
				sprintf(&FmtStr2[strlen(FmtStr2)],"%02X ",SampBuf2[i]);						
			}
		}
		else
		{
			if(ErrLoca==i)
			{
				sprintf(&FmtStr1[strlen(FmtStr1)],"[%02X] ",Buf1[i]);						
				sprintf(&FmtStr2[strlen(FmtStr2)],"[%02X] ",SampBuf2[i]);						
			}
			else
			{
				sprintf(&FmtStr1[strlen(FmtStr1)],"%02X ",Buf1[i]);						
				sprintf(&FmtStr2[strlen(FmtStr2)],"%02X ",SampBuf2[i]);						
			}
		}
	}
	AddStrToEdit(FlashEepromDbgHwnd,IDC_InforShow,"Data[%08X]:%s\r\n",(i%16)?(i-i%16+Addr):(i-16+Addr),FmtStr1);
	AddStrToEdit(FlashEepromDbgHwnd,IDC_InforShow,"Samp[%08X]:%s\r\n",(i%16)?(i-i%16+Addr):(i-16+Addr),FmtStr2);
}

DWORD WINAPI WriteEepromFromFile(LPVOID lpParameter)
{
	CHAR FileName[MAX_PATH] = "",FmtStr[64]="";
	OPENFILENAME mOpenFile={0};
	ULONG TestLen,RLen,BlockSize=0,i=0;
	PUCHAR FileBuf=NULL;
	double BT,UsedT;
	BOOL RetVal = FALSE;
	HANDLE hFile=INVALID_HANDLE_VALUE;
	UCHAR RBuf[4096] = "";
	
	// Fill in the OPENFILENAME structure to support a template and hook.
	mOpenFile.lStructSize = sizeof(OPENFILENAME);
	mOpenFile.hwndOwner         = FlashEepromDbgHwnd;
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
	mOpenFile.lpstrTitle        = "选择待写入Eeprom的数据文件";

	mOpenFile.nFileOffset       = 0;
	mOpenFile.nFileExtension    = 0;
	mOpenFile.lpstrDefExt       = NULL;
	mOpenFile.lCustData         = 0;
	mOpenFile.lpfnHook 		    = NULL;
	mOpenFile.lpTemplateName    = NULL;
	mOpenFile.Flags             = OFN_SHOWHELP | OFN_EXPLORER | OFN_READONLY | OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&mOpenFile))
	{ 	
		DbgPrint("Write data to flash from:%s",FileName);
	}
	else
		goto Exit;

	hFile = CreateFile(FileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_WRITE|FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_ARCHIVE,NULL);
	if(hFile==INVALID_HANDLE_VALUE)
	{
		ShowLastError("WriteEepromFromFile.打开保存文件");
		goto Exit;
	}
	TestLen = GetFileSize(hFile,NULL);
	if(TestLen<1)
	{
		DbgPrint("WriteEepromFromFile.写入Eeprom数据文件为空，请重选");
		goto Exit;
	}
	if( TestLen > EepromCapacity ) //Flash容量
		TestLen = EepromCapacity;

	DbgPrint("*>>*WriteEepromFromFile.Eeprom写%d字节",TestLen);
	FileBuf = (PUCHAR)malloc(TestLen+64);
	if(FileBuf==NULL)
	{
		DbgPrint("WriteEepromFromFile.申请内存失败");
		goto Exit;
	}
	memset(FileBuf,0,TestLen+64);
	RLen = TestLen;
	if( !ReadFile(hFile,FileBuf,RLen,&RLen,NULL) )
	{
		ShowLastError("WriteEepromFromFile.Read file");
		goto Exit;
	}
	if(RLen!=TestLen)
	{
		DbgPrint("WriteEepromFromFile.ReadFile len err(%d-%d)",RLen,TestLen);
		goto Exit;
	}		
	DbgPrint("*>>*1.WriteEepromFromFile.Eeprom写");
	BT = GetCurrentTimerVal();
	//for(i=0;i<1000;i++)
	//{
		RetVal = CH347WriteEEPROM(DevIndex,EepromType,0,TestLen,FileBuf);	
	//}
	UsedT = (GetCurrentTimerVal()-BT)/1000;
	DbgPrint("*<<*WriteEepromFromFile.块写%d字节 %s.平均速度:%.3fKB/S,累计用时%.3fS",TestLen*1000,RetVal?"成功":"失败",TestLen*1000/UsedT/1000,UsedT);

Exit:
	if(FileBuf!=NULL)
		free(FileBuf);
	if(hFile!=INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	return RetVal;
}

DWORD WINAPI EepromVerifyWithFile(LPVOID lpParameter)
{
	ULONG Addr = 0,TC,TestLen,i,k;
	double BT = 0;
	BOOL RetVal = FALSE;
	CHAR FileName[MAX_PATH]="";
	HANDLE hFile=INVALID_HANDLE_VALUE;
	PUCHAR FileBuf=NULL,RBuf=NULL;
	ULONG FileSize,RLen,UnitSize;
	double UsedT = 0;
	ULONG PrintLen,ErrLoca;
	OPENFILENAME mOpenFile={0};

	DbgPrint("*>>*开始验证FLASH内容");
	// Fill in the OPENFILENAME structure to support a template and hook.
	mOpenFile.lStructSize = sizeof(OPENFILENAME);
	mOpenFile.hwndOwner         = FlashEepromDbgHwnd;
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
	mOpenFile.lpstrTitle        = "选择Eeprom对比数据文件";

	mOpenFile.nFileOffset       = 0;
	mOpenFile.nFileExtension    = 0;
	mOpenFile.lpstrDefExt       = NULL;
	mOpenFile.lCustData         = 0;
	mOpenFile.lpfnHook 		   = NULL;
	mOpenFile.lpTemplateName    = NULL;
	mOpenFile.Flags             = OFN_SHOWHELP | OFN_EXPLORER | OFN_READONLY | OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&mOpenFile))
	{ 	
		DbgPrint("Verify Eeprom with file:%s",FileName);
	}
	else
		goto Exit;

	if(strlen(FileName) < 1)
	{
		DbgPrint("文件无效，请重新选择");
		goto Exit;
	}	
	hFile = CreateFile(FileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);//打开文件
	if(hFile == INVALID_HANDLE_VALUE)
	{
		ShowLastError("CreateFile");
		goto Exit;
	}
	FileSize = GetFileSize(hFile,NULL);
	if(FileSize<128)
	{
		DbgPrint("用于比对的文件%d字节太小，请重新选择",FileSize);
		goto Exit;
	}
	if( FileSize > EepromCapacity )
	{
		TestLen = EepromCapacity;		
	}
	else
		TestLen = FileSize;
	
	FileBuf = (PUCHAR)malloc(FileSize);
	if(FileBuf==NULL)
	{
		DbgPrint("申请文件内存%dB失败",FileSize);
		goto Exit;
	}
	RBuf = (PUCHAR)malloc(FileSize+128);
	if(RBuf==NULL)
	{
		DbgPrint("申请测试内存%dB失败",FileSize);
		goto Exit;
	}
	RLen = TestLen;
	if( !ReadFile(hFile,FileBuf,RLen,&RLen,NULL) )
	{
		DbgPrint("从文件中读取数据失败",FileSize);
		goto Exit;
	}
	//快速读取SPI FLASH
	RLen = TestLen;//一次读完		

	BT = GetCurrentTimerVal();
	RetVal = CH347ReadEEPROM(DevIndex,EepromType,0,RLen,RBuf);//读	
	if( !RetVal )
	{
		DbgPrint("从Eeprom起始地址读%dB数据失败",TestLen);
	}
	if(RLen != TestLen)
	{
		DbgPrint("读取数据不完整(0x%X-0x%X)",RLen,TestLen);
	}	

	if( !RetVal )
		goto Exit;

	TestLen = FileSize;
	TC = (TestLen+8192-1)/8192;
	for(i=0;i<TC;i++)
	{
		Addr = i*8192;
		if( (i+1)==TC)
			UnitSize = FileSize-i*8192;
		else
			UnitSize = 8192;
		for(k=0;k<UnitSize;k++)
		{
			if(FileBuf[Addr+k]!=RBuf[Addr+k])
			{	
				if(((Addr+k)&0xFFF0+16)>FileSize)
					PrintLen = FileSize - ((Addr+k)&0xFFF0+16);
				else 
					PrintLen = 16;
				ErrLoca = (Addr+k)%16;
				DbgPrint("[%04X]:%02X-%02X:写入和读出的数据不匹配",k+Addr,FileBuf[Addr+k],RBuf[Addr+k]);				
				DumpDataBuf1((Addr+k)&0xFFF0,RBuf+((Addr+k)&0xFFF0),FileBuf+((Addr+k)&0xFFF0),PrintLen,ErrLoca);
				goto Exit;
			}
		}
	}
	RetVal = TRUE;
	UsedT = (GetCurrentTimerVal()-BT)/1000;	
	DbgPrint("*<<*验证Eeprom内容%dB,与文件内容匹配,平均速度:%.3fKB/S,累计用时%.3fS",TestLen,TestLen/UsedT/1000,UsedT);
Exit:
	if(hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	if(FileBuf!=NULL)
		free(FileBuf);

	DbgPrint("*>>*Eeprom验证%s",RetVal?"完成":"失败");
	DbgPrint("\r\n");	

	return RetVal;
}

DWORD WINAPI ReadEepromToFile(LPVOID lpParameter)
{
	// 获取将要发送的文件名
	CHAR FileName[MAX_PATH] = "",FmtStr[64]="";
	OPENFILENAME mOpenFile={0};
	ULONG TestLen,RLen;
	PUCHAR RBuf=NULL;
	double BT,UsedT;
	BOOL RetVal = FALSE;
	HANDLE hFile=INVALID_HANDLE_VALUE;

	// Fill in the OPENFILENAME structure to support a template and hook.
	mOpenFile.lStructSize = sizeof(OPENFILENAME);
	mOpenFile.hwndOwner         = FlashEepromDbgHwnd;
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
	mOpenFile.lpstrTitle        = "选择Eeprom数据保存文件";

	mOpenFile.nFileOffset       = 0;
	mOpenFile.nFileExtension    = 0;
	mOpenFile.lpstrDefExt       = NULL;
	mOpenFile.lCustData         = 0;
	mOpenFile.lpfnHook 		   = NULL;
	mOpenFile.lpTemplateName    = NULL;
	mOpenFile.Flags             = OFN_SHOWHELP | OFN_EXPLORER | OFN_READONLY | OFN_FILEMUSTEXIST;
	if (GetSaveFileName(&mOpenFile))
	{ 	
		DbgPrint("FlashData will save to:%s",FileName);
	}
	else
		goto Exit;

	TestLen = EepromCapacity; //Flash容量
	DbgPrint("*>>*ReadEepromToFile.Flash读%d字节至文件",TestLen);

	hFile = CreateFile(FileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_WRITE|FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_ARCHIVE,NULL);
	if(hFile==INVALID_HANDLE_VALUE)
	{
		ShowLastError("ReadEepromToFile.打开保存文件");
		goto Exit;
	}
	RBuf = (PUCHAR)malloc(TestLen+64);
	if(RBuf==NULL)
	{
		DbgPrint("ReadEepromToFile.申请内存失败");
		goto Exit;
	}	
	BT = GetCurrentTimerVal();
	RetVal = CH347ReadEEPROM(DevIndex,EepromType,0,TestLen,RBuf);
	UsedT = (GetCurrentTimerVal()-BT)/1000;
	DbgPrint("*<<*ReadEepromToFile.块读%d字节 %s.平均速度:%.3fKB/S,累计用时%.3fS",TestLen,RetVal?"成功":"失败",TestLen/UsedT/1000,UsedT);			
	if(RetVal)
	{
		if( !WriteFile(hFile,RBuf,TestLen,&RLen,NULL) )
		{
			ShowLastError("ReadEepromToFile.数据写入文件:");
			goto Exit;
		}
	}	
	RetVal = TRUE;
Exit:
	if(RBuf!=NULL)
		free(RBuf);
	if(hFile!=INVALID_HANDLE_VALUE)
		CloseHandle(hFile);

	return RetVal;
}
