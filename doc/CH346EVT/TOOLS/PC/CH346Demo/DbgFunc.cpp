/**********************************************************
**  Copyright  (C)  WCH  2001-2023                       **
**  Web:  http://wch.cn                                  **
***********************************************************
Abstract:
    Auxiliary function
Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
  SPDX-License-Identifier: Apache-2.0
Revision History:
  3/1/2022: TECH30
--*/
//To disable deprecation, use _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <winsock2.h.>
#pragma comment(lib,"ws2_32")
#include "Main.h"


CHAR AfxShareBuf[1024]; //共享内存
HWND AfxActiveHwnd;
ULONG AfxDbgI = 0;
//输出格式化字符串,用dbgview软件接收


//输出格式化字符串,用dbgview软件接收
VOID  DbgPrint(LPCTSTR lpFormat,...)
{   
   CHAR TextBufferTmp[10240]="";
   {
	   SYSTEMTIME lpSystemTime;
	   GetLocalTime(&lpSystemTime);
	   sprintf(TextBufferTmp,"%04d#%02d:%02d:%02d:%03d:: \0",AfxDbgI++,		   
		   lpSystemTime.wHour ,lpSystemTime.wMinute ,lpSystemTime.wSecond,lpSystemTime.wMilliseconds );
   }

   va_list arglist;
   va_start(arglist, lpFormat);
   vsprintf(&TextBufferTmp[strlen(TextBufferTmp)],lpFormat,arglist);
   va_end(arglist);
   strcat(TextBufferTmp,"\r\n");
   OutputDebugString(TextBufferTmp);

   SendDlgItemMessage(AfxActiveHwnd,IDC_DbgShow,EM_SETSEL,0xFFFFFFFE,0xFFFFFFFE);
   SendDlgItemMessage(AfxActiveHwnd,IDC_DbgShow,EM_REPLACESEL,0,(LPARAM)TextBufferTmp);
   SendDlgItemMessage(AfxActiveHwnd,IDC_DbgShow,EM_SETSEL,0xFFFFFFFE,0xFFFFFFFE);
  
   return ;
}

/*显示上次运行错误*/
void ShowLastError(LPCTSTR lpFormat,...) 
{
	DWORD LastResult=0; // pointer to variable to receive error codes	
	CHAR szSysMsg[4096] = "";
	CHAR PreBuffer[4096] = "";  	
	LastResult=GetLastError();
    {
		va_list arglist;
		va_start(arglist, lpFormat);
		vsprintf(PreBuffer,lpFormat,arglist);
		va_end(arglist);   
		
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,0,LastResult,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),szSysMsg,sizeof(szSysMsg),0);	
		DbgPrint("%s错误:(0x%X)%s",PreBuffer,LastResult,szSysMsg);
	}	
}

//获取硬件计数器已运行时间,ms为单位,比GetTickCount更准确
double GetCurrentTimerVal()
{
	LARGE_INTEGER litmp; 
	double dfFreq,QPart1; 
	QueryPerformanceFrequency(&litmp);  //频率以HZ为单位
	dfFreq = (double)litmp.QuadPart;    //获得计数器的时钟频率
	QueryPerformanceCounter(&litmp);
	QPart1 = (double)litmp.QuadPart;        //获得初始值
	return(QPart1 *1000/dfFreq  );  //获得对应的时间值=振荡次数/振荡频率，单位为秒
}

/*将十六进制字符转换成十进制码,数字转换成字符用ltoa()函数*/
ULONG mStrToHEX(PCHAR str) 
{  
	char mlen,i=0;
	UCHAR iChar=0,Char[9]="";
	UINT mBCD=0,de=1;
	mlen=strlen(str);
	memcpy(Char,str,mlen);
	for(i=mlen-1;i>=0;i--)
	{	iChar=Char[i];
	if ( iChar >= '0' && iChar <= '9' )
		mBCD = mBCD+(iChar -'0')*de;
	else if ( iChar >= 'A' && iChar <= 'F' ) 
		mBCD =mBCD+ (iChar - 'A' + 0x0a)*de;
	else if ( iChar >= 'a' && iChar <= 'f' )
		mBCD =mBCD+ (iChar - 'a' + 0x0a)*de;
	else return(0);
	de*=16;
	}
	return(mBCD);
}

//将格式化字符信息输出到文本框末尾
VOID  AddStrToEdit (HWND hDlg,ULONG EditID,const char * Format,...)
{
   va_list arglist;   
   int cb;
   CHAR buffer[10240]="";

   va_start(arglist, Format);
   cb = _vsnprintf(&buffer[strlen(buffer)], sizeof(buffer), Format, arglist);
   if (cb == -1) 
   {
      buffer[sizeof(buffer) - 2] = '\n';
   }
   if(strlen(buffer) && buffer[strlen(buffer)-1]!='\n' )
	   strcat(buffer,"\0");   
   va_end(arglist);
   
   SendDlgItemMessage(hDlg,EditID,EM_SETSEL,0xFFFFFFFE,0xFFFFFFFE);
   SendDlgItemMessage(hDlg,EditID,EM_REPLACESEL,0,(LPARAM)buffer);
   SendDlgItemMessage(hDlg,EditID,EM_SETSEL,0xFFFFFFFE,0xFFFFFFFE);
   return ;
}


//大小端切换
ULONG EndSwitch(ULONG dwVal)
{
	ULONG SV;

	((PUCHAR)&SV)[0] = ((PUCHAR)&dwVal)[3];
	((PUCHAR)&SV)[1] = ((PUCHAR)&dwVal)[2];
	((PUCHAR)&SV)[2] = ((PUCHAR)&dwVal)[1];
	((PUCHAR)&SV)[3] = ((PUCHAR)&dwVal)[0];
	return SV;
}
