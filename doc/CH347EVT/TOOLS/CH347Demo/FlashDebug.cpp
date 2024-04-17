/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2023                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  基于CH347 SPI接口函数操作FLASH应用示例，FLASH 型号识别、块读、块写、块擦除、FLASH内容读至文件、文件
  写入FLASH、速度测试等操作函数。SPI传输速度可达2M字节/S

Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
Revision History:
  4/3/2022: TECH30
--*/

#include "Main.h"

#define DevID_Mode1_2 "USB\\VID_1A86&PID_55D"

extern HINSTANCE AfxMainIns; //进程实例
extern ULONG Flash_Sector_Count; // FLASH芯片扇区数 
extern USHORT Flash_Sector_Size; // FLASH芯片扇区大小
extern HWND AfxActiveHwnd;
extern BOOL g_isChinese;
//全局变量
HWND FlashEepromDbgHwnd; //窗体句柄
BOOL FlashDevIsOpened;   //设备是否打开
BOOL IsSpiInit;     //SPI使用前需对SPI进行初化
ULONG DevIndex;
mDeviceInforS FlashDevInfor[16] = {0};

//使能操作按钮，需先打开和配置,否则无法操作
VOID FlashDlg_EnableButtonEnable();

//FLASH型号识别
BOOL FlashIdentify()
{
	double BT,UsedT;
	BOOL RetVal;

	if(!FlashDevIsOpened)
	{
		if (g_isChinese)
			DbgPrint("请先打开设备");
		else 
			DbgPrint("Please open the device first");
		return FALSE;
	}
	if (g_isChinese)
		DbgPrint(">>FLASH芯片型号检测...");
	else 
		DbgPrint(">>Flash chip type detection...");
	BT = GetCurrentTimerVal();
	RetVal = FLASH_IC_Check();
	UsedT = GetCurrentTimerVal()-BT;
	if (g_isChinese)
		DbgPrint(">>FLASH芯片型号检测...%s,用时:%.3fS",RetVal?"成功":"失败",UsedT);
	else
		DbgPrint(">>Flash chip type detection...% s.Time:% 3fS",RetVal?"Success":"failure",UsedT);

	return RetVal;
}

//初始化SPI控制器和片选设置
BOOL Flash_InitSpi()
{	
	BOOL RetVal = FALSE;
	mSpiCfgS SpiCfg = {0};

	CH347SPI_GetCfg(DevIndex,&SpiCfg); //获取硬件SPI当前设置
	SpiCfg.iMode = (UCHAR)SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Mode,CB_GETCURSEL,0,0);
	SpiCfg.iClock = (UCHAR)SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Clock,CB_GETCURSEL,0,0);
	SpiCfg.iByteOrder = 1; //MSB
	SpiCfg.iSpiOutDefaultData = 0Xff;

	RetVal = CH347SPI_Init(DevIndex,&SpiCfg);    //设置SPI
	CH347SPI_SetChipSelect(DevIndex,0x0001,0,0,0,0); //使能并启用CS1作为片选信号
	FlashIdentify(); //识别评估板上的W24Q64芯片型号

	DbgPrint("Flash_Init %s",RetVal?"succ":"failure");

	return RetVal;
}


//FLASH字节读
BOOL FlashBlockRead()
{
	double BT,UseT;
	ULONG DataLen,FlashAddr=0,i;
	UCHAR DBuf[8192] = {0};
	CHAR FmtStr[512] = "",FmtStr1[8*1024*3+16]="";

	if(!FlashDevIsOpened)
	{
		if (g_isChinese)
			DbgPrint("请先打开设备");
		else 
			DbgPrint("Please open the device first");

		return FALSE;
	}

	//获取FLASH读的起始地址
	GetDlgItemText(FlashEepromDbgHwnd,IDC_FlashStartAddr,FmtStr,32);
	FlashAddr = mStrToHEX(FmtStr);
	//获取FLASH读的字节数,十六进制
	GetDlgItemText(FlashEepromDbgHwnd,IDC_FlashDataSize,FmtStr,32);
	DataLen = mStrToHEX(FmtStr);
	if(DataLen<1)
	{
		if (g_isChinese)
			DbgPrint("请输入Flash数据操作长度");
		else 
			DbgPrint("Please enter the flash data operation length");

		return FALSE;
	}
	else if(DataLen>(8*1024*3)) //显示限制
	{
		if (g_isChinese)
			DbgPrint("请输入小于0x%X的Flash数据操作长度",8*1024);
		else
			DbgPrint("Please enter a flash data operation length less than 0x%x",8*1024);
		return FALSE;
	}

	BT = GetCurrentTimerVal();
	DataLen = FLASH_RD_Block(FlashAddr,DBuf,DataLen);
	UseT = GetCurrentTimerVal()-BT;

	if(DataLen<1) {
		if (g_isChinese)
			DbgPrint(">>Flash读:从[%X]地址开始读入%d字节...失败.",FlashAddr,DataLen);
		else
			DbgPrint(">>FLASH read: read%d bytes from [%x] address fail.",FlashAddr,DataLen);
	} 
	else
	{	
		if (g_isChinese)
			DbgPrint(">>Flash读:从[%X]地址开始读入%d字节...成功.用时%.3fS",FlashAddr,DataLen,UseT/1000);
		else
			DbgPrint(">>FLASH read: read%d bytes from [%x] address success. Time%.3fS",FlashAddr,DataLen,UseT/1000);
		{//显示FLASH数据,16进制显示
			for(i=0;i<DataLen;i++)		
				sprintf(&FmtStr1[strlen(FmtStr1)],"%02X ",DBuf[i]);						
			SetDlgItemText(FlashEepromDbgHwnd,IDC_FlashData,FmtStr1);
		}
	}
	return TRUE;
}

//Flash块数据写
BOOL FlashBlockWrite()
{
	ULONG DataLen,FlashAddr=0,i,StrLen;
	UCHAR DBuf[8*1024+16] = {0};
	CHAR FmtStr[8*1024*3+16] = "",ValStr[16]="";
	double BT,UseT;

	//先擦除
	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_FlashErase,BM_CLICK,0,0);

	//获取写FLASH的起始地址,十六进制
	GetDlgItemText(FlashEepromDbgHwnd,IDC_FlashStartAddr,FmtStr,32);
	FlashAddr = mStrToHEX(FmtStr);				

	//获取写FLASH的字节数,十六进制
	DataLen = 0;
	StrLen = GetDlgItemText(FlashEepromDbgHwnd,IDC_FlashData,FmtStr,sizeof(FmtStr));	
	for(i=0;i<StrLen;i+=3)
	{		
		memcpy(&ValStr[0],&FmtStr[i],2);

		DBuf[DataLen] = (UCHAR)mStrToHEX(ValStr);
		DataLen++;
	}
	
	BT = GetCurrentTimerVal();
	DataLen = FLASH_WR_Block(FlashAddr,DBuf,DataLen);
	UseT = GetCurrentTimerVal()-BT;
	if(DataLen<1) {
		if (g_isChinese)
			DbgPrint(">>Flash写:从[%X]地址开始写入%d字节...失败",FlashAddr,DataLen);
		else 
			DbgPrint(">>Flash write: write%d bytes from [%x] address fail",FlashAddr,DataLen);
	}
	else {
		if (g_isChinese)
			DbgPrint(">>Flash写:从[%X]地址开始写入%d字节...成功.用时%.3fS",FlashAddr,DataLen,UseT/1000);
		else 
			DbgPrint(">>Flash write: write%d bytes from [%x] address success. Time% 3fS",FlashAddr,DataLen,UseT/1000);
	}
	return TRUE;
}

//FLASH块擦除
BOOL FlashBlockErase()
{
	ULONG DataLen,FlashAddr=0;
	CHAR FmtStr[128] = "";
	double BT,UseT;
	BOOL RetVal;

	//获取擦除FLASH的起始地址,十六进制
	GetDlgItemText(FlashEepromDbgHwnd,IDC_FlashStartAddr,FmtStr,32);
	FlashAddr = mStrToHEX(FmtStr);
	//获取擦除FLASH的字节数,十六进制
	GetDlgItemText(FlashEepromDbgHwnd,IDC_FlashDataSize,FmtStr,32);
	DataLen = mStrToHEX(FmtStr);

	BT = GetCurrentTimerVal();
	RetVal = FLASH_Erase_Sector(FlashAddr);
	UseT = GetCurrentTimerVal()-BT;
	if (g_isChinese) 
	{
		if( !RetVal )
			DbgPrint(">>FLASH擦除:[%X]...失败",FlashAddr);
		else
			DbgPrint(">>FLASH擦除:[%X]...成功,用时%.3fS",FlashAddr,UseT/1000);
	}
	else 
	{
		if( !RetVal )
			DbgPrint(">>Flash erase: [%x] fail",FlashAddr);
		else
			DbgPrint(">>Flash erase: [%x] Success, time%.3fS",FlashAddr,UseT/1000);
	}

	return TRUE;
}

//显示检验数据出错内容
VOID DumpDataBuf(ULONG Addr,PUCHAR Buf1,PUCHAR SampBuf2,ULONG DataLen1,ULONG ErrLoca)
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

//FLASH测试，先擦除、写、读、检验
DWORD WINAPI FlashRWSpeedTest(LPVOID lpParameter)
{
	ULONG FlashAddr = 0,TC,TestLen,i,k,BlockSize=0;
	double BT = 0;
	BOOL RetVal = FALSE;
	CHAR FileName[MAX_PATH]="";
	HANDLE hFile=INVALID_HANDLE_VALUE;
	PUCHAR FileBuf=NULL,RBuf=NULL;
	ULONG FileSize,RLen,UnitSize,Addr,WLen;
	double UsedT = 0;
	ULONG Timeout,PrintLen,ErrLoca;
	UCHAR temp;
	OPENFILENAME mOpenFile={0};

	EnableWindow(FlashEepromDbgHwnd,FALSE);

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
	if (g_isChinese)
		mOpenFile.lpstrTitle        = "选择待写入FLASH的数据文件";
	else 
		mOpenFile.lpstrTitle        = "Select the data file to be written to FLASH";

	mOpenFile.nFileOffset       = 0;
	mOpenFile.nFileExtension    = 0;
	mOpenFile.lpstrDefExt       = NULL;
	mOpenFile.lCustData         = 0;
	mOpenFile.lpfnHook 		   = NULL;
	mOpenFile.lpTemplateName    = NULL;
	mOpenFile.Flags             = OFN_SHOWHELP | OFN_EXPLORER | OFN_READONLY | OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&mOpenFile))
		DbgPrint("Write data to flash from:%s",FileName);
	else 
		goto Exit;

	if(strlen(FileName) < 1)
	{
		if (g_isChinese)
			DbgPrint("测试文件无效，请重新选择");
		else
			DbgPrint("The test file is invalid, please select again");
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
		if (g_isChinese)
			DbgPrint("测试文件%d字节太小，请重新选择",FileSize);
		else 
			DbgPrint("Test file%d bytes is too small, please re select",FileSize);
		goto Exit;
	}
	if( FileSize > (Flash_Sector_Size * Flash_Sector_Count) )
		TestLen = Flash_Sector_Size * Flash_Sector_Count;
	else
		TestLen = FileSize;
	
	FileBuf = (PUCHAR)malloc(FileSize);
	if(FileBuf==NULL)
	{
		if (g_isChinese)
			DbgPrint("申请文件内存%dB失败",FileSize);
		else
			DbgPrint("failed to apply for file memory %dB", FileSize);
		goto Exit;
	}
	RBuf = (PUCHAR)malloc(FileSize+128);
	if(RBuf==NULL)
	{
		if (g_isChinese)
			DbgPrint("申请文件内存%dB失败",FileSize);
		else
			DbgPrint("failed to apply for file memory %dB", FileSize);
		goto Exit;
	}
	RLen = FileSize;
	if( !ReadFile(hFile,FileBuf,RLen,&RLen,NULL) )
	{
		if (g_isChinese)
			DbgPrint("从文件中读取测试数据失败",FileSize);
		else 
			DbgPrint("Failed to read the test data from file", FileSize);
		goto Exit;
	}
	DbgPrint("\r\n");
	if (g_isChinese)
	{
		DbgPrint("****开始速度测试,测试数据长度为 %dB",FileSize);
		DbgPrint("*>>*1.Flash擦除速度测试");
	} else {
		DbgPrint("****Start the speed test, and the test data length is %dB",FileSize);
		DbgPrint("*>>*1. Flash erase speed test");
	}
	TestLen = FileSize;
	BT = GetCurrentTimerVal();
	/*
	for(i=0;i<TC;i++)
	{
		RetVal = FLASH_Erase_Sector(FlashAddr+i*Flash_Sector_Size);
		//RetVal = FLASH_Erase_Block(FlashAddr+i*32768);		
		if(!RetVal )
		{
			DbgPrint("  FLASH_Erase_Sector[%X] failure",i);
			break;
		}
	}
	*/
	RetVal = FLASH_Erase_Full();
	UsedT = (GetCurrentTimerVal()-BT)/1000;
	if (g_isChinese) 
	{
		DbgPrint("*<<*擦除全部%s,平均速度:%.2fKB/S,累计用时%.3fS",RetVal?"成功":"失败",TestLen/UsedT/1000,UsedT);	
		DbgPrint("*>>*2.Flash块写速度测试");
	} else {
		DbgPrint("*<<*Erase all%s, average speed:%.2fkb/s, accumulated time%.3fS",RetVal?" Success ":" failed",TestLen/UsedT/1000,UsedT);	
		DbgPrint("*>>*2. Flash block write speed test");
	}
	
	BlockSize = 0x100;
	TestLen = FileSize;	
	TC = (FileSize+BlockSize-1)/BlockSize;
	BT = GetCurrentTimerVal();
	for(i=0;(i<TC)&(RetVal);i++)
	{
		Addr = i*BlockSize;
		if( (i+1)==TC )
			RLen = FileSize-i*BlockSize;
		else
			RLen = BlockSize;		
	
		RetVal = FLASH_WriteEnable();		
		if(!RetVal)		
		{
			DbgPrint("FLASH_WriteEnable failure.");
			break;
		}
		RBuf[0] = CMD_FLASH_BYTE_PROG;
		RBuf[1] = (UINT8)( Addr >> 16 );
		RBuf[2] = (UINT8)( Addr >> 8 );
		RBuf[3] = (UINT8)Addr&0xFF;
		RLen += 4;
		memcpy(&RBuf[4],FileBuf+Addr,RLen);
		RetVal = CH347SPI_Write(DevIndex,0x80,RLen,BlockSize+4,RBuf); //SPI批量写
		if(!RetVal)	
		{
			if (g_isChinese)
				DbgPrint("写FLASH[0x%X][0x%X]失败",Addr,RLen);
			else 
				DbgPrint("Write flash[0x%X][0x%X] failed",Addr,RLen);
			break;
		}

		Timeout = 0;
		do//等待写结束
		{
			temp = FLASH_ReadStatusReg();
			if( (temp & 0x01)<1)
				break;
			Sleep(0);
			Timeout += 1;
			if(Timeout > FLASH_OP_TIMEOUT*20)
			{
				DbgPrint("    [0x%X][0x%X]>FLASH_Read timeout",Addr,RLen);
				RetVal= FALSE; 
				break; //退出写
			}
		}while( temp & 0x01 );		
	}
	UsedT = (GetCurrentTimerVal()-BT)/1000;
	if (g_isChinese) 
	{
		DbgPrint("*<<*块写%d字节 %s.平均速度:%.3fKB/S,累计用时%.3fS",TestLen,RetVal?"成功":"失败",TestLen/UsedT/1000,UsedT);
		DbgPrint("*>>*3.Flash块读速度测试");
	}else {
		DbgPrint("*<<*Block write%d bytes%s. average speed:%.3fkb/s, accumulated time%.3fS",TestLen,RetVal?" Success ":" Failed",TestLen/UsedT/1000,UsedT);
		DbgPrint("*>>*3. Flash block reading speed test");
	}
	TestLen = FileSize;

	//先发送读地址
	Addr = 0;		
	RBuf[0] = CMD_FLASH_READ;
	RBuf[1] = (UINT8)( Addr >> 16 );
	RBuf[2] = (UINT8)( Addr >> 8 );
	RBuf[3] = (UINT8)( Addr );		
	WLen = 4;
	
	//快速读取SPI FLASH
	RLen = TestLen;//一次读完		

	BT = GetCurrentTimerVal();
	RetVal = CH347SPI_Read(DevIndex,0x80,WLen,&RLen,RBuf);//SPI批量读,先发命令
	UsedT = (GetCurrentTimerVal()-BT)/1000;	
	if( !RetVal )
	{
		if (g_isChinese)
			DbgPrint("从FLASH起始地址读%dB数据失败",TestLen);
		else 
			DbgPrint("Failed to read %dB data from flash start address",TestLen);
	}
	if(RLen != TestLen)
	{
		if (g_isChinese)
			DbgPrint("读取数据不完整(0x%X-0x%X)",RLen,TestLen);
		else 
			DbgPrint("Incomplete read data (0x%X-0x%X)",RLen,TestLen);
	}	
	if (g_isChinese)
		DbgPrint("*<<*块读%d字节 %s.平均速度:%.3fKB/S,累计用时%.3fS",TestLen,RetVal?"成功":"失败",TestLen/UsedT/1000,UsedT);	
	else 
		DbgPrint("*<<*Block read%d bytes%s. average speed:%.3fKB/s, accumulated time%.3fS",TestLen,RetVal?" Success ":" failed",TestLen/UsedT/1000,UsedT);

	if( !RetVal )
		goto Exit;
	if (g_isChinese)
		DbgPrint("*>>*4.Flash写读内容检查");
	else 
		DbgPrint("*>>*4.Flash write/read content check");

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
				if (g_isChinese)
					DbgPrint("[%04X]:%02X-%02X:写入和读出的数据不匹配",k+Addr,FileBuf[Addr+k],RBuf[Addr+k]);				
				else 
					DbgPrint("[%04X]:%02X-%02X: data written and read do not match",k+Addr,FileBuf[Addr+k],RBuf[Addr+k]);
				DumpDataBuf((Addr+k)&0xFFF0,RBuf+((Addr+k)&0xFFF0),FileBuf+((Addr+k)&0xFFF0),PrintLen,ErrLoca);
				goto Exit;
			}
		}
	}
	if (g_isChinese)
		DbgPrint("*<<*检查Flash写和读共计%dB,全部匹配",TestLen);
	else 
		DbgPrint("*<<*Check the flash write and read for a total of %dB, all matching",TestLen);
	DbgPrint("Test Result: P A S S");

Exit:
	if(hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	if(FileBuf!=NULL)
		free(FileBuf);

	EnableWindow(FlashEepromDbgHwnd,TRUE);
	return RetVal;
}

//读FLASH内容至文件
DWORD WINAPI WriteFlashFromFile(LPVOID lpParameter)
{
	CHAR FileName[MAX_PATH] = "",FmtStr[64]="";
	OPENFILENAME mOpenFile={0};
	ULONG TestLen,RLen,Addr,TC,i,Timeout,temp,BlockSize=0;
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
	if (g_isChinese)
		mOpenFile.lpstrTitle        = "选择待写入FLASH的数据文件";
	else 
		mOpenFile.lpstrTitle        = "Select the data file to be written to FLASH";

	mOpenFile.nFileOffset       = 0;
	mOpenFile.nFileExtension    = 0;
	mOpenFile.lpstrDefExt       = NULL;
	mOpenFile.lCustData         = 0;
	mOpenFile.lpfnHook 		   = NULL;
	mOpenFile.lpTemplateName    = NULL;
	mOpenFile.Flags             = OFN_SHOWHELP | OFN_EXPLORER | OFN_READONLY | OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&mOpenFile))
		DbgPrint("Write data to flash from:%s",FileName);
	else
		goto Exit;

	hFile = CreateFile(FileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_WRITE|FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_ARCHIVE,NULL);
	if(hFile==INVALID_HANDLE_VALUE)
	{
		if (g_isChinese)
			ShowLastError("WriteFlashFromFile.打开保存文件");
		else 
			ShowLastError("WriteFlashFromFile.Open save file");
		goto Exit;
	}
	TestLen = GetFileSize(hFile,NULL);
	if(TestLen<1)
	{
		if (g_isChinese)
			DbgPrint("WriteFlashFromFile.写入FLASH数据文件为空，请重选");
		else
			DbgPrint("WriteFlashFromFile. Write FLASH data file is empty, please reselect");
		goto Exit;
	}
	if( TestLen > (Flash_Sector_Size * Flash_Sector_Count) ) //Flash容量
		TestLen = Flash_Sector_Size * Flash_Sector_Count;

	if (g_isChinese)
		DbgPrint("*>>*WriteFlashFromFile.Flash写%d字节",TestLen);
	else 
		DbgPrint("*>>*WriteFlashFromFile. Flash writes%d bytes",TestLen);
	FileBuf = (PUCHAR)malloc(TestLen+64);
	if(FileBuf==NULL)
	{
		if (g_isChinese)
			DbgPrint("WriteFlashFromFile.申请内存失败");
		else
			DbgPrint("WriteFlashFromFile.Failed to request memory");
		goto Exit;
	}
	memset(FileBuf,0,TestLen+64);
	RLen = TestLen;
	if( !ReadFile(hFile,FileBuf,RLen,&RLen,NULL) )
	{
		ShowLastError("WriteFlashFromFile.Read file");
		goto Exit;
	}
	if(RLen!=TestLen)
	{
		DbgPrint("WriteFlashFromFile.ReadFlashFile len err(%d-%d)",RLen,TestLen);
		goto Exit;
	}
	if (g_isChinese)
		DbgPrint("*>>*1.WriteFlashFromFile.擦除");
	else 
		DbgPrint("*>>*1.writeflashfromfile.erase");
	Addr=0;
	TC = (TestLen+Flash_Sector_Size-1)/Flash_Sector_Size; //page	
	BT = GetCurrentTimerVal();
	for(i=0;i<TC;i++)
	{
		RetVal = FLASH_Erase_Sector(Addr+i*Flash_Sector_Size);
		//RetVal = FLASH_Erase_Block(FlashAddr+i*32768);		
		if(!RetVal )
		{
			DbgPrint("  FLASH_Erase_Sector[%X] failure",i);
			break;
		}
	}
	UsedT = (GetCurrentTimerVal()-BT)/1000;
	if (g_isChinese) 
	{
		DbgPrint("*<<*WriteFlashFromFile.擦除%d块(%dB) %s,平均速度:%.2fKB/S,累计用时%.3fS",
			TC,TC*32768,RetVal?"成功":"失败",
			TestLen/UsedT/1000,UsedT);

		DbgPrint("*>>*2.WriteFlashFromFile.Flash块写");
	}else {
		DbgPrint("*<<*WriteFlashFromFile.Erase%d blocks(%dB) %s,Average speed:%.2fKB/S,Average speed%.3fS",
			TC,TC*32768,RetVal?"Success":"Failed",
			TestLen/UsedT/1000,UsedT);

		DbgPrint("*>>*2.WriteFlashFromFile.Flash blocks write");
	}

	BlockSize = 0x100;
	TC = (TestLen+BlockSize-1)/BlockSize;
	BT = GetCurrentTimerVal();
	for(i=0;(i<TC)&(RetVal);i++)
	{
		Addr = i*BlockSize;
		if( (i+1)==TC )
			RLen = TestLen-i*BlockSize;
		else
			RLen = BlockSize;		
	
		RetVal = FLASH_WriteEnable();		
		if(RetVal)		
		{			
			RBuf[0] = CMD_FLASH_BYTE_PROG;
			RBuf[1] = (UINT8)( Addr >> 16 );
			RBuf[2] = (UINT8)( Addr >> 8 );
			RBuf[3] = (UINT8)Addr;
			RLen += 4;			
			memcpy(&RBuf[4],FileBuf+Addr,RLen);
			RetVal = CH347SPI_Write(DevIndex,0x80,RLen,BlockSize+4,RBuf);
			if(!RetVal)	DbgPrint("WriteFlashFromFile.Write FLASH[0x%X][0x%X] Failed",Addr,RLen);
		}
		else break;
		Timeout = 0;
		do
		{
			temp = FLASH_ReadStatusReg();
			if( (temp & 0x01)<1)
				break;
			Sleep(0);
			Timeout += 1;
			if(Timeout > FLASH_OP_TIMEOUT)
			{
				DbgPrint("    [0x%X][0x%X]>FLASH_Read timeout",Addr,RLen);
				RetVal= FALSE; break; //退出写
			}
		}while( temp & 0x01 );		
	}
	UsedT = (GetCurrentTimerVal()-BT)/1000;
	if (g_isChinese)
		DbgPrint("*<<*WriteFlashFromFile.块写%d字节 %s.平均速度:%.3fKB/S,累计用时%.3fS",TestLen,RetVal?"成功":"失败",TestLen/UsedT/1000,UsedT);
	else 
		DbgPrint("*<<*WriteFlashFromFile.Block write%d bytes %s.Average speed:%.3fKB/S,Accumulated time%.3fS",TestLen,RetVal?"Success":"Failed",TestLen/UsedT/1000,UsedT);
Exit:
	if(FileBuf!=NULL)
		free(FileBuf);
	if(hFile!=INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	return RetVal;
}

//检验FLASH内容
DWORD WINAPI FlashVerifyWithFile(LPVOID lpParameter)
{
	ULONG FlashAddr = 0,TC,TestLen,i,k;
	double BT = 0;
	BOOL RetVal = FALSE;
	CHAR FileName[MAX_PATH]="";
	HANDLE hFile=INVALID_HANDLE_VALUE;
	PUCHAR FileBuf=NULL,RBuf=NULL;
	ULONG FileSize,RLen,UnitSize,Addr,WLen;
	double UsedT = 0;
	ULONG PrintLen,ErrLoca;
	OPENFILENAME mOpenFile={0};

	if (g_isChinese)
		DbgPrint("*>>*开始验证FLASH内容");
	else
		DbgPrint("*>>*Start verifying flash content");
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
	if (g_isChinese)
		mOpenFile.lpstrTitle    = "选择待写入Flash的数据文件";
	else
		mOpenFile.lpstrTitle    = "Select the data file to be written to flash";

	mOpenFile.nFileOffset       = 0;
	mOpenFile.nFileExtension    = 0;
	mOpenFile.lpstrDefExt       = NULL;
	mOpenFile.lCustData         = 0;
	mOpenFile.lpfnHook 		   = NULL;
	mOpenFile.lpTemplateName    = NULL;
	mOpenFile.Flags             = OFN_SHOWHELP | OFN_EXPLORER | OFN_READONLY | OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&mOpenFile))
	{ 	
		DbgPrint("Verify flash with file:%s",FileName);
	}
	else
		goto Exit;

	if(strlen(FileName) < 1)
	{
		if (g_isChinese)
			DbgPrint("文件无效，请重新选择");
		else 
			DbgPrint("Invalid file, please reselect");
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
		if (g_isChinese)
			DbgPrint("用于比对的文件%d字节太小，请重新选择",FileSize);
		else 
			DbgPrint("The file used for comparison%d bytes is too small, please reselect", FileSize);
		goto Exit;
	}
	if( FileSize > (Flash_Sector_Size * Flash_Sector_Count) )
	{
		TestLen = Flash_Sector_Size * Flash_Sector_Count;		
	}
	else
		TestLen = FileSize;
	
	FileBuf = (PUCHAR)malloc(FileSize);
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
	RLen = TestLen;
	if( !ReadFile(hFile,FileBuf,RLen,&RLen,NULL) )
	{
		if (g_isChinese)
			DbgPrint("从文件中读取数据失败",FileSize);
		else 
			DbgPrint("Failed to read data from file", FileSize);
		goto Exit;
	}

	//先发送读地址
	Addr = 0;		
	RBuf[0] = CMD_FLASH_READ;
	RBuf[1] = (UINT8)( Addr >> 16 );
	RBuf[2] = (UINT8)( Addr >> 8 );
	RBuf[3] = (UINT8)( Addr );		
	WLen = 4;
	
	//快速读取SPI FLASH
	RLen = TestLen;//一次读完		

	BT = GetCurrentTimerVal();
	RetVal = CH347SPI_Read(DevIndex,0x80,WLen,&RLen,RBuf);//读	
	if( !RetVal )
	{
		if (g_isChinese)
			DbgPrint("从FLASH起始地址读%dB数据失败",TestLen);
		else 
			DbgPrint("Failed to read %dB data from FLASH starting address", TestLen);
	}
	if(RLen != TestLen)
	{
		if (g_isChinese)
			DbgPrint("读取数据不完整(0x%X-0x%X)",RLen,TestLen);
		else
			DbgPrint("Read data incomplete (0x%x-0x%x)",RLen,TestLen);
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
				if (g_isChinese)
					DbgPrint("[%04X]:%02X-%02X:写入和读出的数据不匹配",k+Addr,FileBuf[Addr+k],RBuf[Addr+k]);				
				else
					DbgPrint("[%04x]:%02x-%02x: the data written and read do not match", k+Addr,FileBuf[Addr+k],RBuf[Addr+k]);				
				DumpDataBuf((Addr+k)&0xFFF0,RBuf+((Addr+k)&0xFFF0),FileBuf+((Addr+k)&0xFFF0),PrintLen,ErrLoca);
				goto Exit;
			}
		}
	}
	RetVal = TRUE;
	UsedT = (GetCurrentTimerVal()-BT)/1000;	
	if (g_isChinese)
		DbgPrint("*<<*验证FLASH内容%dB,与文件内容匹配,平均速度:%.3fKB/S,累计用时%.3fS",TestLen,TestLen/UsedT/1000,UsedT);
	else 
		DbgPrint("*<<*Verify FLASH content %dB, match the file content, average speed:%.3fkb/s, cumulative time%.3fs",TestLen,TestLen/UsedT/1000,UsedT);
Exit:
	if(hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	if(FileBuf!=NULL)
		free(FileBuf);

	if (g_isChinese)
		DbgPrint("*>>*FLASH验证%s",RetVal?"完成":"失败");
	else 
		DbgPrint("*>>*FLASH verifies%s",RetVal?"success":"failed");
	DbgPrint("\r\n");	

	return RetVal;
}

//读FLASH内容至文件
DWORD WINAPI ReadFlashToFile(LPVOID lpParameter)
{
	// 获取将要发送的文件名
	CHAR FileName[MAX_PATH] = "",FmtStr[64]="";
	OPENFILENAME mOpenFile={0};
	ULONG TestLen,RLen,Addr;
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
	if (g_isChinese)
		mOpenFile.lpstrTitle    = "选择FLASH数据保存文件";
	else 
		mOpenFile.lpstrTitle	= "Select FLASH data save file";

	mOpenFile.nFileOffset       = 0;
	mOpenFile.nFileExtension    = 0;
	mOpenFile.lpstrDefExt       = NULL;
	mOpenFile.lCustData         = 0;
	mOpenFile.lpfnHook 		   = NULL;
	mOpenFile.lpTemplateName    = NULL;
	mOpenFile.Flags             = OFN_SHOWHELP | OFN_EXPLORER | OFN_READONLY | OFN_FILEMUSTEXIST;
	if (GetSaveFileName(&mOpenFile))
		DbgPrint("FlashData will save to:%s",FileName);
	else
		goto Exit;

	TestLen = Flash_Sector_Size * Flash_Sector_Count; //Flash容量
	if (g_isChinese)
		DbgPrint("*>>*ReadFlashToFile.Flash读%d字节至文件",TestLen);
	else 
		DbgPrint("*>>*ReadFlashToFile. Flash reads%d bytes to file",TestLen);

	hFile = CreateFile(FileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_WRITE|FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_ARCHIVE,NULL);
	if(hFile==INVALID_HANDLE_VALUE)
	{
		if (g_isChinese)
			ShowLastError("ReadFlashToFile.打开保存文件");
		else 
			ShowLastError("ReadFlashToFile.open save file");
		goto Exit;
	}
	RBuf = (PUCHAR)malloc(TestLen+64);
	if(RBuf==NULL)
	{
		if (g_isChinese)
			DbgPrint("ReadFlashToFile.申请内存失败");
		else 
			DbgPrint("ReadFlashToFile.Failed to request memory");
		goto Exit;
	}	
	//发送读地址
	Addr = 0;		
	RBuf[0] = CMD_FLASH_READ;
	RBuf[1] = (UINT8)( Addr >> 16 );
	RBuf[2] = (UINT8)( Addr >> 8 );
	RBuf[3] = (UINT8)( Addr );

	BT = GetCurrentTimerVal();
	//快速读取SPI FLASH
	RLen = TestLen;//待读长度
	RetVal = CH347SPI_Read(DevIndex,0x80,4,&RLen,RBuf);
	if( !RetVal )
	{
		if (g_isChinese)
			DbgPrint("ReadFlashToFile.从FLASH起始地址读%dB数据失败",RLen);
		else
			DbgPrint("ReadFlashToFile.Failed to read%dB data from flash start address",RLen);
	}
	if(RLen != TestLen)
	{
		if (g_isChinese)
			DbgPrint("ReadFlashToFile.读取数据不完整(0x%X-0x%X)",RLen,TestLen);
		else 
			DbgPrint("ReadFlashToFile.Incomplete read data(0x%X-0x%X)",RLen,TestLen);
	}
	UsedT = (GetCurrentTimerVal()-BT)/1000;
	if (g_isChinese)
		DbgPrint("*<<*ReadFlashToFile.块读%d字节 %s.平均速度:%.3fKB/S,累计用时%.3fS",TestLen,RetVal?"成功":"失败",TestLen/UsedT/1000,UsedT);			
	else 
		DbgPrint("*<<*ReadFlashToFile. block read%d bytes%s. average speed:%.3fkb/s, cumulative time%.3fs", TestLen,RetVal? "Success": "failure",TestLen/UsedT/1000,UsedT);
	if( !WriteFile(hFile,RBuf,RLen,&RLen,NULL) )
	{
		if (g_isChinese)
			ShowLastError("ReadFlashToFile.数据写入文件:");
		else
			ShowLastError("ReadFlashToFile.Data write file:");
		goto Exit;
	}
	if(RLen!=TestLen)
	{
		if (g_isChinese)
			DbgPrint("ReadFlashToFile写入不完整");
		else 
			DbgPrint("ReadFlashToFile Incomplete writing");
		goto Exit;
	}		
	RetVal = TRUE;
Exit:
	if(RBuf!=NULL)
		free(RBuf);
	if(hFile!=INVALID_HANDLE_VALUE)
		CloseHandle(hFile);

	return RetVal;
}

//打开设备
BOOL FlashSpi_OpenDevice()
{
	//获取设备序号
	DevIndex = SendDlgItemMessage(FlashEepromDbgHwnd,IDC_ObjList,CB_GETCURSEL,0,0);
	if(DevIndex==CB_ERR)
	{
		if (g_isChinese)
			DbgPrint("请先打开设备");
		else 
			DbgPrint("Please open the device first");
		goto Exit; //退出
	}	
	FlashDevIsOpened = (CH347OpenDevice(DevIndex) != INVALID_HANDLE_VALUE);
	if (g_isChinese)
		DbgPrint(">>%d#设备打开...%s",DevIndex,FlashDevIsOpened?"成功":"失败");
	else 
		DbgPrint(">>%d#Open Device...%s",DevIndex,FlashDevIsOpened?"Success":"Failed");
Exit:
	FlashDlg_EnableButtonEnable();
	return FlashDevIsOpened;
}

//关闭设备
BOOL FlashSpi_CloseDevice()
{
	CH347CloseDevice(DevIndex);
	FlashDevIsOpened = FALSE;
	if (g_isChinese)
		DbgPrint(">>设备已关闭");
	else 
		DbgPrint(">>The Devices is closed");
	DevIndex = CB_ERR;

	FlashDlg_EnableButtonEnable();

	return TRUE;
}


//显示设备信息
BOOL  FlashDlg_ShowDevInfor()
{
	ULONG ObjSel;
	CHAR  FmtStr[128]="";

	ObjSel = SendDlgItemMessage(FlashEepromDbgHwnd,IDC_ObjList,CB_GETCURSEL,0,0);
	if(ObjSel!=CB_ERR)
	{
		sprintf(FmtStr,"**ChipMode:%d,%s,Ver:%02X,DevID:%s",FlashDevInfor[ObjSel].ChipMode,FlashDevInfor[ObjSel].UsbSpeedType?"HS":"FS",FlashDevInfor[ObjSel].FirewareVer,FlashDevInfor[ObjSel].DeviceID);
		SetDlgItemText(FlashEepromDbgHwnd,IDC_DevInfor,FmtStr);
	}
	return (ObjSel!=CB_ERR);
}

//枚举设备
ULONG  FlashDlg_EnumDevice()
{
	ULONG i,oLen,DevCnt = 0;
	USB_DEVICE_DESCRIPTOR DevDesc = {0};
	CHAR tem[256] = "";
	mDeviceInforS DevInfor = {0};

	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_ObjList,CB_RESETCONTENT,0,0);	
	for(i=0;i<16;i++)
	{
		if(CH347OpenDevice(i) != INVALID_HANDLE_VALUE)
		{
			oLen = sizeof(USB_DEVICE_DESCRIPTOR);
			CH347GetDeviceInfor(i,&DevInfor);
			if(DevInfor.ChipMode == 3) //模式3此接口为JTAG/I2C
				continue;
			sprintf(tem,"%d# %s",i,DevInfor.FuncDescStr);
			SendDlgItemMessage(FlashEepromDbgHwnd,IDC_ObjList,CB_ADDSTRING,0,(LPARAM)(LPCTSTR)tem);		
			memcpy(&FlashDevInfor[DevCnt],&DevInfor,sizeof(DevInfor));
			DevCnt++;
		}
		CH347CloseDevice(i);
	}
	if(DevCnt)
	{
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_ObjList,CB_SETCURSEL,0,0);
		SetFocus(GetDlgItem(FlashEepromDbgHwnd,IDC_ObjList));
	}
	return DevCnt;
}


//使能操作按钮，需先打开和配置,否则无法操作
VOID FlashDlg_EnableButtonEnable()
{
	if(!FlashDevIsOpened)
		IsSpiInit = FALSE;

	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_CMD_InitSPI),FlashDevIsOpened);

	//更新打开/关闭设备按钮状态
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_OpenDevice),!FlashDevIsOpened);
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_CloseDevice),FlashDevIsOpened);	

	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_ObjList),!FlashDevIsOpened);
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_RefreshObjList),!FlashDevIsOpened);

	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_FlashVerify),IsSpiInit);
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_FlashRWSpeedTest),IsSpiInit);	
	
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_FlashIdentify),IsSpiInit);
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_FlashRead),IsSpiInit);
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_FlashWrite),IsSpiInit);
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_FlashErase),IsSpiInit);

	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_ReadToFile),IsSpiInit);
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_WriteFormFile),IsSpiInit);
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_FlashVerify),IsSpiInit);
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_FlashRWSpeedTest),IsSpiInit);	
}

//初始化窗体
VOID FlashDlg_InitWindows()
{	
	//查找并显示设备列表 
	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_RefreshObjList,BM_CLICK,0,0);	
	
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_OpenDevice),!FlashDevIsOpened);
	EnableWindow(GetDlgItem(FlashEepromDbgHwnd,IDC_CloseDevice),FlashDevIsOpened);
	//Flash地址斌初值
	SetDlgItemText(FlashEepromDbgHwnd,IDC_FlashStartAddr,"0");
	//Flash操作数斌初值
	SetDlgItemText(FlashEepromDbgHwnd,IDC_FlashDataSize,"100");
	//清空Flash数据模式
	SetDlgItemText(FlashEepromDbgHwnd,IDC_FlashData,"");
	//输出框设置显示的最大字符数
	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_InforShow,EM_LIMITTEXT,0xFFFFFFFF,0);

	{
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode0");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode1");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode2");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Mode,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"Mode3");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Mode,CB_SETCURSEL,3,0);
	}
	{//0=60MHz, 1=30MHz, 2=15MHz, 3=7.5MHz, 4=3.75MHz, 5=1.875MHz, 6=937.5KHz，7=468.75KHz
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"60MHz");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"30MHz");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"15MHz");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"7.5MHz");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"3.75MHz");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"1.875MHz");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"937.5KHz");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"468.75KHz");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_SpiCfg_Clock,CB_SETCURSEL,1,0);
	}
	//I2C的时钟配置
	{
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_I2CCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"20KHz");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_I2CCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"100KHz");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_I2CCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"400KHz");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_I2CCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"750KHz");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_I2CCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"50KHz");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_I2CCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"200KHz");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_I2CCfg_Clock,CB_ADDSTRING,0,(LPARAM) (LPCTSTR)"1MHz");
		SendDlgItemMessage(FlashEepromDbgHwnd,IDC_I2CCfg_Clock,CB_SETCURSEL,0,3);
	}
	SetDlgItemInt(FlashEepromDbgHwnd,IDC_TestLen,100,FALSE);
	SetEditlInputMode(FlashEepromDbgHwnd,IDC_FlashData,1); //设置edit为十六进制输入模式

	return;
}

//主窗体进程
BOOL APIENTRY DlgProc_FlashEepromDbg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	ULONG ThreadID;
	UCHAR I2CClockSel;	//I2C

	switch (message)
	{
	case WM_INITDIALOG:
		FlashEepromDbgHwnd = hWnd;
		AfxActiveHwnd = hWnd;	
		FlashDevIsOpened = FALSE;		
		CheckDlgButton(hWnd,IDC_EnablePnPAutoOpen_Flash,BST_CHECKED);
		FlashDlg_InitWindows(); //初始化窗体		
		FlashDlg_EnableButtonEnable();

		InitWindows_Eeprom();
		Eeprom_InitI2C();
		//Uart_InitParam();		
		//为USB2.0JTAG设备设置插入和拔出的通知.插入后自动打开设备,拔出后关闭设备
	//	if(CH341SetDeviceNotify(0,DevID_Mode1_2, UsbDevPnpNotify) )       //设备插拔通知回调函数
		//	DbgPrint("已开启USB设备插拔监视");
	//	break;	
	//case WM_USB20SpiDevArrive: //监测到设备插入
	//	DbgPrint("****发现CH347设备插入USB口,打开设备");
		//先枚举USB设备
	//	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_RefreshObjList,BM_CLICK,0,0);
		//打开设备
	//	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_OpenDevice,BM_CLICK,0,0);
	//	break;
	//case WM_USB20SpiDevRemove: //监测到设备拔出
	//	DbgPrint("****发现CH347已从USB口移除,关闭设备");
		//关闭设备
	//	SendDlgItemMessage(FlashEepromDbgHwnd,IDC_CloseDevice,BM_CLICK,0,0);
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
			FlashDlg_EnumDevice();   //枚举并显示设备
			break;
		case IDC_ObjList:
			FlashDlg_ShowDevInfor();
			break;
		case IDC_OpenDevice://打开设备
			FlashSpi_OpenDevice();
			FlashDlg_EnableButtonEnable();	//更新按钮状态
			break;
		case IDC_CloseDevice:			
			FlashSpi_CloseDevice();				
			FlashDlg_EnableButtonEnable();	//更新按钮状态			
			break;	
		case IDC_CMD_InitSPI:
			IsSpiInit = Flash_InitSpi();
			FlashDlg_EnableButtonEnable();
			break;
		case IDC_CMD_InitI2C:
			I2CClockSel = (UCHAR)SendDlgItemMessage(FlashEepromDbgHwnd,IDC_I2CCfg_Clock,CB_GETCURSEL,0,0);
			CH347I2C_Set(DevIndex,I2CClockSel); //配置I2C速度为快速750K
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
		case IDC_FlashVerify:
			CloseHandle(CreateThread(NULL,0,FlashVerifyWithFile,NULL,0,&ThreadID));
			break;
		case IDC_WriteFormFile:
			CloseHandle(CreateThread(NULL,0,WriteFlashFromFile,NULL,0,&ThreadID));
			break;
		case IDC_ReadToFile:
			CloseHandle(CreateThread(NULL,0,ReadFlashToFile,NULL,0,&ThreadID));
			break;
		case IDC_FlashRWSpeedTest://读写测速
			CloseHandle(CreateThread(NULL,0,FlashRWSpeedTest,NULL,0,&ThreadID));
			break;
		case IDC_EepromRead:
			EerpromRead();
			break;
		case IDC_EepromWrite:
			EepromWrite();
			break;
		case IDC_EepromVerify:
			CloseHandle(CreateThread(NULL,0,EepromVerifyWithFile,NULL,0,&ThreadID));
			break;
		case IDC_WriteEepromFormFile:
			CloseHandle(CreateThread(NULL,0,WriteEepromFromFile,NULL,0,&ThreadID));
			break;
		case IDC_ReadEepromToFile:
			CloseHandle(CreateThread(NULL,0,ReadEepromToFile,NULL,0,&ThreadID));
			break;
		case IDC_EepromType:
			Eeprom_InitI2C();
			break;
		case IDC_ClearInfor:
			SetDlgItemText(hWnd,IDC_InforShow,"");
			break;
		case WM_DESTROY:			
			SendDlgItemMessage(hWnd,IDC_CloseDevice,BM_CLICK,0,0);
			//CH347SetDeviceNotify(DevIndex,DevID_Mode1_2,NULL);
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
