/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2023                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  ����CH347 I2C�ӿں�������FLASHӦ��ʾ����EEPROM��д�����ݶ����ļ����ļ�
  д���

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
extern HWND FlashEepromDbgHwnd;     //��FLASH����һ����
extern HWND AfxDlgFlashEepromDbgHwnd;
EEPROM_TYPE EepromType;
BOOL   IsInitI2C;
extern BOOL FlashDevIsOpened;  //�豸�Ƿ��;
extern HINSTANCE AfxMainIns; //����ʵ��
extern ULONG DevIndex;
extern BOOL g_isChinese;
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
		if (g_isChinese)
			DbgPrint("���ȴ��豸");
		else 
			DbgPrint("Please open the device first");
		return FALSE;
	}

	//��ȡFLASH������ʼ��ַ
	GetDlgItemText(FlashEepromDbgHwnd,IDC_EepromStartAddr,FmtStr,32);
	Addr = mStrToHEX(FmtStr);
	//��ȡFLASH�����ֽ���,ʮ������
	GetDlgItemText(FlashEepromDbgHwnd,IDC_EepromDataSize,FmtStr,32);
	DataLen = mStrToHEX(FmtStr);
	if(DataLen<1)
	{
		if (g_isChinese)
			DbgPrint("������Eeprom���ݲ�������");
		else 
			DbgPrint("Please enter the EEPROM data operation length");
		return FALSE;
	}
	else if(DataLen>(8*1024*3)) //��ʾ����
	{
		if (g_isChinese)
			DbgPrint("������С��0x%X��Flash���ݲ�������",8*1024);
		else
			DbgPrint("Please enter the flash data operation length less than 0x%x ", 8*1024);
		return FALSE;
	}

	BT = GetCurrentTimerVal();
	RetVal = CH347ReadEEPROM(DevIndex,EepromType,Addr,DataLen,DBuf);
	UseT = GetCurrentTimerVal()-BT;

	if(!RetVal){
		if (g_isChinese)
			DbgPrint(">>Eeprom��:��[%X]��ַ��ʼ����%d�ֽ�...ʧ��.",Addr,DataLen);
		else 
			DbgPrint("> >eeprom read: read %d bytes from [%x] address... Failed.", DataLen, Addr);
	}else
	{	
		if (g_isChinese)
			DbgPrint(">>Eeprom��:��[%X]��ַ��ʼ����%d�ֽ�...�ɹ�.��ʱ%.3fS",Addr,DataLen,UseT/1000);
		else 
			DbgPrint("> >eeprom read: read %d bytes from [%x] address... Success. Time%.3fs", DataLen,Addr,UseT/1000);
		{//��ʾFLASH����,16������ʾ
			for(i=0;i<DataLen;i++)		
				sprintf(&FmtStr1[strlen(FmtStr1)],"%02X ",DBuf[i]);						
			SetDlgItemText(FlashEepromDbgHwnd,IDC_EepromData,FmtStr1);
		}
	}
	return TRUE;
}

//Flash������д
BOOL EepromWrite()
{
	ULONG DataLen,Addr=0,i,StrLen;
	UCHAR DBuf[8*1024+16] = {0};
	CHAR FmtStr[8*1024*3+16] = "",ValStr[16]="";
	double BT,UseT;
	BOOL RetVal;

	//��ȡдFLASH����ʼ��ַ,ʮ������
	GetDlgItemText(FlashEepromDbgHwnd,IDC_EepromStartAddr,FmtStr,32);
	Addr = mStrToHEX(FmtStr);				

	//��ȡдFLASH���ֽ���,ʮ������
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
	if(!RetVal) {
		if (g_isChinese)
			DbgPrint(">>Eepromд:��[%X]��ַ��ʼд��%d�ֽ�...ʧ��",Addr,DataLen);
		else 
			DbgPrint("> >eeprom write: write %d bytes from [%x] address... Failed",DataLen,Addr);
	}else {
		if (g_isChinese)	
			DbgPrint(">>Eepromд:��[%X]��ַ��ʼд��%d�ֽ�...�ɹ�.��ʱ%.3fS",Addr,DataLen,UseT/1000);
		else
			DbgPrint("> >eeprom write: write %d bytes from [%x] address... Success. Time%.3fs", DataLen, Addr, UseT/1000);
	}
	sprintf(FmtStr,"%X",DataLen);
	SetDlgItemText(FlashEepromDbgHwnd,IDC_EepromDataSize,FmtStr);

	return TRUE;
}

//��ʾ�������ݳ�������
VOID DumpDataBuf1(ULONG Addr,PUCHAR Buf1,PUCHAR SampBuf2,ULONG DataLen1,ULONG ErrLoca)
{
	CHAR FmtStr1[8192*3] = "",FmtStr2[8192*3] = "";
	ULONG i;

	memset(FmtStr1,0,sizeof(FmtStr1));	
	memset(FmtStr2,0,sizeof(FmtStr2));	
	//16������ʾ
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
	if (g_isChinese)
		mOpenFile.lpstrTitle        = "ѡ���д��Eeprom�������ļ�";
	else 
		mOpenFile.lpstrTitle        = "Select the data file to be written to EEPROM";

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
		if (g_isChinese)
			ShowLastError("WriteEepromFromFile.�򿪱����ļ�");
		else 
			ShowLastError("WriteEepromFromFile.Open save file");
		goto Exit;
	}
	TestLen = GetFileSize(hFile,NULL);
	if(TestLen<1)
	{
		if (g_isChinese)
			DbgPrint("WriteEepromFromFile.д��Eeprom�����ļ�Ϊ�գ�����ѡ");
		else
			DbgPrint("WriteEepromFromFile. Write EEPROM data file is empty, please reselect");
		goto Exit;
	}
	if( TestLen > EepromCapacity ) //Flash����
		TestLen = EepromCapacity;
	
	if (g_isChinese)
		DbgPrint("*>>*WriteEepromFromFile.Eepromд%d�ֽ�",TestLen);
	else
		DbgPrint("*>>*WriteEepromFromFile.Eeprom write %d Bytes",TestLen);
	FileBuf = (PUCHAR)malloc(TestLen+64);
	if(FileBuf==NULL)
	{
		if (g_isChinese)
			DbgPrint("WriteEepromFromFile.�����ڴ�ʧ��");
		else 
			DbgPrint("WriteEepromFromFile. Failed to request memory");
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
	DbgPrint("*>>*1.WriteEepromFromFile.Eepromд");
	BT = GetCurrentTimerVal();
	//for(i=0;i<1000;i++)
	//{
		RetVal = CH347WriteEEPROM(DevIndex,EepromType,0,TestLen,FileBuf);	
	//}
	UsedT = (GetCurrentTimerVal()-BT)/1000;
	if (g_isChinese)
		DbgPrint("*<<*WriteEepromFromFile.��д%d�ֽ� %s.ƽ���ٶ�:%.3fKB/S,�ۼ���ʱ%.3fS",TestLen*1000,RetVal?"�ɹ�":"ʧ��",TestLen*1000/UsedT/1000,UsedT);
	else 
		DbgPrint("*<<*WriteEepromFromFile.block write %d bytes %s. average speed:%.3fkb/s, cumulative time%.3fs", TestLen*1000, RetVal? "Success": "failure", TestLen*1000/UsedT/1000, UsedT);
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

	if (g_isChinese)
		DbgPrint("*>>*��ʼ��֤FLASH����");
	else
		DbgPrint("* > > * start verifying flash content");
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
		mOpenFile.lpstrTitle    = "ѡ��Eeprom�Ա������ļ�";
	else 
		mOpenFile.lpstrTitle   = "Select EEPROM comparison data file";

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
		if (g_isChinese)
			DbgPrint("�ļ���Ч��������ѡ��");
		else 
			DbgPrint("Invalid file, please reselect");

		goto Exit;
	}	
	hFile = CreateFile(FileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);//���ļ�
	if(hFile == INVALID_HANDLE_VALUE)
	{
		ShowLastError("CreateFile");
		goto Exit;
	}
	FileSize = GetFileSize(hFile,NULL);
	if(FileSize<128)
	{
		if (g_isChinese)
			DbgPrint("���ڱȶԵ��ļ�%d�ֽ�̫С��������ѡ��",FileSize);
		else 
			DbgPrint("The file used for comparison %d bytes is too small, please reselect", FileSize);
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
		if (g_isChinese)
			DbgPrint("�����ļ��ڴ�%dBʧ��",FileSize);
		else
			DbgPrint("failed to apply for file memory %dB", FileSize);

		goto Exit;
	}
	RBuf = (PUCHAR)malloc(FileSize+128);
	if(RBuf==NULL)
	{
		if (g_isChinese)
			DbgPrint("�����ļ��ڴ�%dBʧ��",FileSize);
		else
			DbgPrint("failed to apply for file memory %dB", FileSize);

		goto Exit;
	}
	RLen = TestLen;
	if( !ReadFile(hFile,FileBuf,RLen,&RLen,NULL) )
	{
		if (g_isChinese)
			DbgPrint("���ļ��ж�ȡ����ʧ��",FileSize);
		else 
			DbgPrint("Failed to read data from file", FileSize);
		goto Exit;
	}
	//���ٶ�ȡSPI FLASH
	RLen = TestLen;//һ�ζ���		

	BT = GetCurrentTimerVal();
	RetVal = CH347ReadEEPROM(DevIndex,EepromType,0,RLen,RBuf);//��	
	if( !RetVal )
	{
		if (g_isChinese)
			DbgPrint("��Eeprom��ʼ��ַ��%dB����ʧ��",TestLen);
		else 
			DbgPrint("Failed to read %dB data from EEPROM starting address", TestLen);
	}
	if(RLen != TestLen)
	{
		if (g_isChinese)
			DbgPrint("��ȡ���ݲ�����(0x%X-0x%X)",RLen,TestLen);
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
					DbgPrint("[%04X]:%02X-%02X:д��Ͷ��������ݲ�ƥ��",k+Addr,FileBuf[Addr+k],RBuf[Addr+k]);				
				else
					DbgPrint("[%04x]:%02x-%02x: the data written and read do not match", k+Addr,FileBuf[Addr+k],RBuf[Addr+k]);
				DumpDataBuf1((Addr+k)&0xFFF0,RBuf+((Addr+k)&0xFFF0),FileBuf+((Addr+k)&0xFFF0),PrintLen,ErrLoca);
				goto Exit;
			}
		}
	}
	RetVal = TRUE;
	UsedT = (GetCurrentTimerVal()-BT)/1000;	
	if (g_isChinese)
		DbgPrint("*<<*��֤Eeprom����%dB,���ļ�����ƥ��,ƽ���ٶ�:%.3fKB/S,�ۼ���ʱ%.3fS",TestLen,TestLen/UsedT/1000,UsedT);
	else 
		DbgPrint("*<<*Verify Eeprom content %dB, match the file content, average speed:%.3fkb/s, cumulative time%.3fs",TestLen,TestLen/UsedT/1000,UsedT);
Exit:
	if(hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	if(FileBuf!=NULL)
		free(FileBuf);
	if (g_isChinese)
		DbgPrint("*>>*Eeprom��֤%s",RetVal?"���":"ʧ��");
	else 
		DbgPrint("*>>*Eeprom verifies%s",RetVal?"success":"failed");
	DbgPrint("\r\n");	

	return RetVal;
}

DWORD WINAPI ReadEepromToFile(LPVOID lpParameter)
{
	// ��ȡ��Ҫ���͵��ļ���
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
	if (g_isChinese)
		mOpenFile.lpstrTitle    = "ѡ��Eeprom���ݱ����ļ�";
	else 
		mOpenFile.lpstrTitle	= "Select EEPROM data save file";

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

	TestLen = EepromCapacity; //Flash����
	if (g_isChinese)
		DbgPrint("*>>*ReadEepromToFile.Eeprom��%d�ֽ����ļ�",TestLen);
	else 
		DbgPrint("*>>*ReadEepromToFile. Eeprom reads%d bytes to file",TestLen);

	hFile = CreateFile(FileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_WRITE|FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_ARCHIVE,NULL);
	if(hFile==INVALID_HANDLE_VALUE)
	{
		if (g_isChinese)
			ShowLastError("ReadEepromToFile.�򿪱����ļ�");
		else 
			ShowLastError("ReadEepromToFile.open save file");
		goto Exit;
	}
	RBuf = (PUCHAR)malloc(TestLen+64);
	if(RBuf==NULL)
	{
		if (g_isChinese)
			DbgPrint("ReadEepromToFile.�����ڴ�ʧ��");
		else 
			DbgPrint("ReadEepromToFile.Failed to request memory");
		goto Exit;
	}	
	BT = GetCurrentTimerVal();
	RetVal = CH347ReadEEPROM(DevIndex,EepromType,0,TestLen,RBuf);
	UsedT = (GetCurrentTimerVal()-BT)/1000;
	if (g_isChinese)
		DbgPrint("*<<*ReadEepromToFile.���%d�ֽ� %s.ƽ���ٶ�:%.3fKB/S,�ۼ���ʱ%.3fS",TestLen,RetVal?"�ɹ�":"ʧ��",TestLen/UsedT/1000,UsedT);			
	else 
		DbgPrint("*<<*readeepromtofile. block read%d bytes%s. average speed:%.3fkb/s, cumulative time%.3fs", TestLen,RetVal? "Success": "failure",TestLen/UsedT/1000,UsedT);
	if(RetVal)
	{
		if( !WriteFile(hFile,RBuf,TestLen,&RLen,NULL) )
		{
			if (g_isChinese)
				ShowLastError("ReadEepromToFile.����д���ļ�:");
			else
				ShowLastError("ReadEepromToFile.Data write file:");
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
