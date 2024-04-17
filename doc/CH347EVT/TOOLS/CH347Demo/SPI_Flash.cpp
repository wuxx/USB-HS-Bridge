/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2023                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
  ����SPI�ӿڵ�FLASH�����������ṩSPI FLASH �ͺ�ʶ�𡢿������д��������Ⱥ�����

  ��ǰԴ�뷽��,����CH32V305 MCUʵ�ֵ�usb2.0(480M high speed)  to SPI�������ڹ�
  ���Զ���USB����FASH������Ȳ�Ʒ��
  ����Դ�������MCU�̼���USB2.0����(480M)�豸ͨ������(CH372DRV)����λ�����̡�
  ��ǰ����Ϊ�Զ���ͨѶЭ��,SPI�����ٶȿɴ�2MB/S
Environment:
    user mode only,VC6.0 and later
Notes:
  Copyright (c) 2023 Nanjing Qinheng Microelectronics Co., Ltd.
Revision History:
  4/3/2022: TECH30
--*/


#include "Main.h"

UINT8V  Flash_Type;               /* FLASHоƬ����: 0: W25XXXϵ��; 1: SST25XXXϵ��; 2: MT25QXXϵ��  */
UINT32V Flash_ID;
extern UINT32V Flash_Sector_Count;/* FLASHоƬ������ */
extern UINT16V Flash_Sector_Size; /* FLASHоƬ������С */
extern ULONG DevIndex;
/*******************************************************************************
* Function Name  : FLASH_ReadID
* Description    : ��ȡFLASHоƬID
* Input          : None
* Output         : None
* Return         : ����4���ֽ�,����ֽ�Ϊ0x00,
*                  �θ��ֽ�ΪManufacturer ID(0xEF),
*                  �ε��ֽ�ΪMemory Type ID
*                  ����ֽ�ΪCapacity ID
*                  W25X40BL����: 0xEF��0x30��0x13
*                  W25X10BL����: 0xEF��0x30��0x11
*******************************************************************************/
UINT32 FLASH_ReadID( void )
{
	UINT32 dat=0;

	UCHAR oInBuffer[16] = {0};
	ULONG  iInLength;	

	oInBuffer[0] = CMD_FLASH_JEDEC_ID;
	memset(oInBuffer+1,0xFF,3);
	iInLength = 3;
	if ( CH347SPI_WriteRead( DevIndex, 0x80, iInLength+1, oInBuffer ) == FALSE ) 
		return( 0xFFFFFFFF );
	else
	{
		oInBuffer[0] = 0;
		memcpy(&dat,oInBuffer,4);	
	}
//Exit:	
	return( EndSwitch(dat) );
}


/*******************************************************************************
* Function Name  : FLASH_ReadStatusReg
* Description    : FLASHоƬ��ȡ״̬�Ĵ��� 
* Input          : None
* Output         : None
* Return         : ���ؼĴ���ֵ
*******************************************************************************/
UINT8 FLASH_ReadStatusReg( void )
{
	UINT8  status;

	//PIN_FLASH_CS_LOW( );
	ULONG iLen=0;
	UCHAR DBuf[512]={0};

	DBuf[0] = CMD_FLASH_RDSR;
	iLen = 1;
	if( !CH347SPI_WriteRead( DevIndex, 0x80, iLen+1, DBuf) )
		return 0xFF;
	else
		status = DBuf[1];

	//PIN_FLASH_CS_HIGH( );
	return( status );
}


/*******************************************************************************
* Function Name  : FLASH_WriteStatusReg
* Description    : FLASHоƬд״̬�Ĵ���
* Input          : statusdata---��Ҫд���״̬�Ĵ���ֵ
* Output         : None
* Return         : None
* ע��:д״̬�Ĵ���ǰ,�����ȷ���Enable-Write-Status-Register (EWSR),������0X50��Write-Enable,������0X06
*******************************************************************************/
void FLASH_WriteStatusReg( UINT8 dat )
{
	ULONG iLen=0;
	UCHAR DBuf[128]={0};

	DBuf[0] = CMD_FLASH_EWSR;
	iLen = 1;
	if( !CH347SPI_WriteRead( DevIndex, 0x80, iLen, DBuf) )
	{
		DbgPrint("  FLASH_WriteStatusReg failure1.");
		return;
	}
	
	//PIN_FLASH_CS_LOW( );
 	//SPI_FLASH_SendByte( CMD_FLASH_WRSR );		   		  			 						
 	//SPI_FLASH_SendByte( dat );		   		  				 					
 	//PIN_FLASH_CS_HIGH( );
	DBuf[0] = CMD_FLASH_WRSR;/* ����д״̬�Ĵ������� */	
	DBuf[1] = dat;/* ����״̬�Ĵ�����ֵ */
	iLen = 0;
	if( !CH347SPI_WriteRead( DevIndex, 0x80, iLen+2, DBuf) )
	{
		DbgPrint("  FLASH_WriteStatusReg failure2.");
		return;
	}
}

/*******************************************************************************
* Function Name  : MT25QXX_ReadExAddrReg
* Description    : MT25QXXXоƬ��ȡ��չ��ַ�Ĵ���
* Input          : None
* Output         : None
* Return         : ���ؼĴ���ֵ
*******************************************************************************/
UINT8 MT25QXXX_ReadExAddrReg( void )
{
	UINT8  status;

	ULONG iLen=0;
	UCHAR DBuf[128]={0};

	DBuf[0] = CMD_RD_EX_ADDR_REG;
	iLen = 1;
	if( !CH347SPI_WriteRead( DevIndex, 0x80, iLen+1, DBuf) )
		return 0xFF;
	else
		status = DBuf[1];

	return( status );
}

/*******************************************************************************
* Function Name  : MT25QXXX_WriteExAddrReg
* Description    : MT25QXXXоƬд��չ��ַ�Ĵ���
* Input          : dat---��Ҫд��ļĴ���ֵ
* Output         : None
* Return         : None
*******************************************************************************/
BOOL MT25QXXX_WriteExAddrReg( UINT8 dat )
{
	ULONG iLen=0;
	UCHAR DBuf[128]={0};

	DBuf[0] = CMD_FLASH_WREN;
	iLen = 0;
	if( !CH347SPI_WriteRead( DevIndex, 0x80, iLen+1, DBuf) )
		return FALSE;	

	DBuf[0] = CMD_WR_EX_ADDR_REG;
	DBuf[1] = dat;
	iLen = 0;
	if( !CH347SPI_WriteRead( DevIndex, 0x80, iLen+2, DBuf) )
		return FALSE;
	else
		return TRUE;
}

/*******************************************************************************
* Function Name  : MT25QXXX_ChangeExAddrReg
* Description    : MT25QXXXоƬ�ı���չ��ַ�Ĵ���
* Input          : dat---��Ҫд��ļĴ���ֵ
* Output         : None
* Return         : None
*******************************************************************************/
void MT25QXXX_ChangeExAddrReg( UINT8 dat )
{
	UINT8 ex_addr;

	ex_addr = MT25QXXX_ReadExAddrReg( );
	if( ex_addr != dat )
	{
		MT25QXXX_WriteExAddrReg( dat );
	}
}

/*******************************************************************************
* Function Name  : FLASH_IC_Check
* Description    : FLASHоƬ���
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
BOOL FLASH_IC_Check( void )
{
	UINT32 count;
	UINT8  temp8;
	CHAR   FlashID[128]="";
	BOOL   RetVal = FALSE;

	Flash_ID = 0;
	/* ��ȡFLASHоƬID�� */	
	Flash_ID = FLASH_ReadID( );													/* ��ȡFLASH��ID�� */
	/* ��ȡFLASH��ID�� */
	DbgPrint("  Flash_ID: %X\n",Flash_ID);

	/* ����оƬ�ͺ�,�ж�������С */
	Flash_Type = 0x00;																
	Flash_Sector_Count = 0x00;													/* FLASHоƬ������ */
	Flash_Sector_Size = 0x00;													/* FLASHоƬ������С */
#if 1
	switch( Flash_ID )
	{
		/**************************************************************************/
		/* W25XXXϵ��оƬ */
		case W25X10_FLASH_ID:            										/* 0xEF3011-----1M bit */			
			strcpy(FlashID,"W25X10_FLASH_ID");
			count = 1;
			break;

		case W25X20_FLASH_ID:            										/* 0xEF3012-----2M bit */
			strcpy(FlashID,"W25X20_FLASH_ID");
			count = 2;
			break;

		case W25X40_FLASH_ID:                    								/* 0xEF3013-----4M bit */
			strcpy(FlashID,"W25X40_FLASH_ID");
			count = 4;
			break;

		case W25X80_FLASH_ID:                        							/* 0xEF4014-----8M bit */
			strcpy(FlashID,"W25X80_FLASH_ID");
			count = 8;
			break;

		case W25Q16_FLASH_ID1:                    								/* 0xEF3015-----16M bit */
		case W25Q16_FLASH_ID2:                    								/* 0xEF4015-----16M bit */
			strcpy(FlashID,"W25Q16_FLASH_ID1/2");
			count = 16;
			break;

		case W25Q32_FLASH_ID1:                    								/* 0xEF4016-----32M bit */
		case W25Q32_FLASH_ID2:                    								/* 0xEF6016-----32M bit */
			strcpy(FlashID,"W25Q32_FLASH_ID1/2");
			count = 32;
			break;

		case W25Q64_FLASH_ID1:                    								/* 0xEF4017-----64M bit */
		case W25Q64_FLASH_ID2:                    								/* 0xEF6017-----64M bit */
			strcpy(FlashID,"W25Q64_FLASH_ID1/2");
			count = 64;
			break;

		case W25Q128_FLASH_ID1:                   								/* 0xEF4018-----128M bit */
		case W25Q128_FLASH_ID2:                   								/* 0xEF6018-----128M bit */
			strcpy(FlashID,"W25Q128_FLASH_ID1/2");
			count = 128;
			break;

		case W25Q256_FLASH_ID1:                   								/* 0xEF4019-----256M bit */
		case W25Q256_FLASH_ID2:                   								/* 0xEF6019-----256M bit */
			strcpy(FlashID,"W25Q256_FLASH_ID1/2");
			count = 256;
			break;

		/**************************************************************************/
		/* SST25XXXϵ��оƬ */
        case SST25VF040_FLASH_ID:                                               /* 0xBF258D-----4M bit */
            count = 4;
            Flash_Type = DEF_TYPE_SST25XXX;
			strcpy(FlashID,"SST25VF040_FLASH_ID");
            break;

        case SST25VF080_FLASH_ID:                                               /* 0xBF258E-----8M bit */
            count = 8;
            Flash_Type = DEF_TYPE_SST25XXX;                                     /* FLASHоƬ����: 0: W25ϵ��; 1: SSTϵ�� */
			strcpy(FlashID,"SST25VF080_FLASH_ID");
            break;

		case SST25VF016_FLASH_ID:           									/* 0xBF254A-----16M bit */
			count = 16;
			Flash_Type = DEF_TYPE_SST25XXX;										/* FLASHоƬ����: 0: W25ϵ��; 1: SSTϵ�� */
			strcpy(FlashID,"SST25VF016_FLASH_ID");
			break;

		case SST25VF032_FLASH_ID:           									/* 0xBF254A-----32M bit */
			count = 32;
			Flash_Type = DEF_TYPE_SST25XXX;										/* FLASHоƬ����: 0: W25ϵ��; 1: SSTϵ�� */
			strcpy(FlashID,"SST25VF032_FLASH_ID");
			break;

		case SST25VF064_FLASH_ID:           									/* 0xBF254B-----64M bit */
			count = 64;
			Flash_Type = DEF_TYPE_SST25XXX;										/* FLASHоƬ����: 0: W25ϵ��; 1: SSTϵ�� */
			strcpy(FlashID,"SST25VF064_FLASH_ID");
			break;

	        /**************************************************************************/
			/* M25PXXXϵ�� */
        case M25P40_FLASH_ID:                                                   /* 4M bit */
            count = 4;
            Flash_Type = DEF_TYPE_M25PXXX;
			strcpy(FlashID,"M25P40_FLASH_ID");
            break;

        case M25P80_FLASH_ID:                                                   /* 8M bit */
            count = 8;
            Flash_Type = DEF_TYPE_M25PXXX;
			strcpy(FlashID,"M25P80_FLASH_ID");
            break;

        case M25P16_FLASH_ID:                                                   /* 16M bit */
            count = 16;
            Flash_Type = DEF_TYPE_M25PXXX;
			strcpy(FlashID,"M25P16_FLASH_ID");
            break;

        case M25P32_FLASH_ID:                                                   /* 32M bit */
            count = 32;
            Flash_Type = DEF_TYPE_M25PXXX;
			strcpy(FlashID,"M25P32_FLASH_ID");
            break;

        case M25P64_FLASH_ID:                                                   /* 64M bit */
            count = 64;
            Flash_Type = DEF_TYPE_M25PXXX;
			strcpy(FlashID,"M25P64_FLASH_ID");
            break;

            /**************************************************************************/
            /* MX25LXXXϵ�� */
        case MX25L40_FLASH_ID:                                                  /* 4M bit */
            count = 4;
            Flash_Type = DEF_TYPE_MX25LXXX;
			strcpy(FlashID,"MX25L40_FLASH_ID");
            break;

        case MX25L80_FLASH_ID:                                                  /* 8M bit */
            count = 8;
            Flash_Type = DEF_TYPE_MX25LXXX;
			strcpy(FlashID,"MX25L80_FLASH_ID");
            break;

        case MX25L16_FLASH_ID:                                                  /* 16M bit */
            count = 16;
            Flash_Type = DEF_TYPE_MX25LXXX;
			strcpy(FlashID,"MX25L16_FLASH_ID");
            break;

        case MX25L32_FLASH_ID:                                                  /* 32M bit */
            count = 32;
            Flash_Type = DEF_TYPE_MX25LXXX;
			strcpy(FlashID,"MX25L32_FLASH_ID");
            break;

        case MX25L64_FLASH_ID:                                                  /* 64M bit */
            count = 64;
            Flash_Type = DEF_TYPE_MX25LXXX;
			strcpy(FlashID,"MX25L64_FLASH_ID");
            break;

			/**************************************************************************/
			/* MT25QXXϵ��оƬ */
		case MT25Q_FLASH_64Mb:           										/* 0x20BA17-----64M bit */
			count = 64;
			Flash_Type = DEF_TYPE_MT25QXXX;
			strcpy(FlashID,"MT25Q_FLASH_64Mb");
			break;

		case MT25Q_FLASH_128Mb:           										/* 0x20BA18-----128M bit */
			count = 128;
			Flash_Type = DEF_TYPE_MT25QXXX;
			strcpy(FlashID,"MT25Q_FLASH_128Mb");
			break;

		case MT25Q_FLASH_256Mb:           										/* 0x20BA19-----256M bit */
			count = 256;
			Flash_Type = DEF_TYPE_MT25QXXX;
			strcpy(FlashID,"MT25Q_FLASH_256Mb");
			break;

		case MT25Q_FLASH_512Mb:           										/* 0x20BA20-----512M bit */
			count = 512;
			Flash_Type = DEF_TYPE_MT25QXXX;
			strcpy(FlashID,"MT25Q_FLASH_512Mb");
			break;

		case MT25Q_FLASH_1024Mb:           										/* 0x20BA21-----1G bit */
			count = 1024;
			Flash_Type = DEF_TYPE_MT25QXXX;
			strcpy(FlashID,"MT25Q_FLASH_1024Mb");
			break;

		case MT25Q_FLASH_2048Mb:           										/* 0x20BA22-----2G bit */
			count = 2048;
			Flash_Type = DEF_TYPE_MT25QXXX;
			strcpy(FlashID,"MT25Q_FLASH_2048Mb");
			break;
 		default:
			strcpy(FlashID,"Unknonw flash type");
 		    if( ( Flash_ID != 0xFFFFFFFF ) || ( Flash_ID != 0x00000000 ) )
 		    {
 		       count = 16;
 		    }
 		    else
 		    {
 	            count = 0x00;
            }
			break;
	}
	count = ( (UINT32)count * 1024 ) * ( (UINT32)1024 / 8 );
#endif

#if 0
    if( ( Flash_ID != 0xFFFFFFFF ) || ( Flash_ID != 0x00000000 ) )
    {
       count = 8;
    }
    else
    {
        count = 0x00;
    }
    count = ( (UINT32)count * 1024 ) * ( (UINT32)1024 / 8 );
#endif

	/* ���ݲ�ͬ����оƬ�������⴦�� */
	if( Flash_Type == DEF_TYPE_SST25XXX )
	{
		temp8 = FLASH_ReadStatusReg( );
		FLASH_WriteStatusReg( 0x00 );			   				  	 			/* ��״̬�Ĵ����е�λ:BP0��BP1��BP2,����FLASH���в�����д���� */
		temp8 = FLASH_ReadStatusReg( );
	}

	/* ����FLASHоƬ������Ŀ,������С����Ϣ */
	if( count )
	{
		Flash_Sector_Count = count / SPI_FLASH_SectorSize;//DEF_UDISK_SECTOR_SIZE;						/* FLASHоƬ������ */
		Flash_Sector_Size = SPI_FLASH_SectorSize;//DEF_UDISK_SECTOR_SIZE;								/* FLASHоƬ������С */
	}	
	DbgPrint("  FlashID:%s(%X),Cap:%dMbit,%dB*%d",
		FlashID,Flash_ID,
		Flash_Sector_Count*Flash_Sector_Size/1024/1024*8,Flash_Sector_Size,Flash_Sector_Count);

	RetVal = TRUE;
	return RetVal;
}

/*******************************************************************************
* Function Name  : FLASH_WriteEnable
* Description    : FLASHоƬ����д����
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
BOOL FLASH_WriteEnable( void )
{
    //PIN_FLASH_CS_LOW( );
	//SPI_FLASH_SendByte( CMD_FLASH_WREN );		   		  			 			/* ��������д�������� */					
	//PIN_FLASH_CS_HIGH( );

	ULONG iLen=0;
	UCHAR DBuf[128]={0};

	DBuf[0] = CMD_FLASH_WREN;
	iLen = 0;
	return CH347SPI_WriteRead( DevIndex, 0x80, iLen+1, DBuf);
}

/*******************************************************************************
* Function Name  : FLASH_WriteDisable
* Description    : FLASHоƬ��ֹд����
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
BOOL FLASH_WriteDisable( void )
{
    //PIN_FLASH_CS_LOW( );
	//SPI_FLASH_SendByte( CMD_FLASH_WRDI );		   		  			 			/* ���ͽ�ֹд�������� */					
	//PIN_FLASH_CS_HIGH( );
	ULONG iLen=0;
	UCHAR DBuf[128]={0};

	DBuf[0] = CMD_FLASH_WRDI;
	iLen = 0;
	return CH347SPI_WriteRead( DevIndex, 0x80, iLen+1, DBuf);
}

/*******************************************************************************
* Function Name  : W25XXX_WR_Page
* Description    : д��һ������
*				   ע����ǰ����֧�ֿ�ҳд����	  
* Input          : address----׼��д���׵�ַ
*				   len--------׼��д�����ݳ���
*				   *pbuf------׼��д��Ļ�������ַ 
* Output         : None
* Return         : None
*******************************************************************************/
BOOL W25XXX_WR_Page( UINT8 *pbuf, UINT32 address, UINT32 len )
{
	UINT8  temp;

	/* д֮ǰ�����ȴ�дʹ��,�ұ���д�ڲ������ĵ�ַ�� */
	FLASH_WriteEnable( );														/* ��дʹ�� */

	/* �ж������MT25QXXXоƬ����Ҫ�жϵ�ַ����ֽ�,��ǰ���������ĸ����  */
	if( Flash_Type == DEF_TYPE_MT25QXXX )
	{
		MT25QXXX_ChangeExAddrReg( (UINT8)( address >> 24 ) );
	}

	/* ��ʼд������ */
	if( len > SPI_FLASH_PerWritePageSize )										/* �޶����� */
	{
		len = SPI_FLASH_PerWritePageSize;
	}
	/* Deselect the FLASH: Chip Select high */
	ULONG iLen=0,Timeout=0;
	UCHAR DBuf[8192]={0};

	if(len>sizeof(DBuf))
	{
		DbgPrint("  W25XXX_WR_Page�������Ȳ��ܲ���%d",len);
		return FALSE;
	}
	DBuf[0] = CMD_FLASH_BYTE_PROG;
	DBuf[1] = (UINT8)( address >> 16 );
	DBuf[2] = (UINT8)( address >> 8 );
	DBuf[3] = (UINT8)address;
	memcpy(&DBuf[4],pbuf,len);
	iLen = 4;
	
	if( !CH347SPI_Write(DevIndex,0x80,iLen+len,SPI_FLASH_PerWritePageSize+4,DBuf))
	{
		DbgPrint("  W25XXX_WR_Page[%X]>>CH347SPI_WriteRead failure.",address);
		return FALSE;
	}
	
	Timeout = 0;
	/* Wait the end of Flash writing */
	do
	{
		temp = FLASH_ReadStatusReg( );	
		if( (temp & 0x01)<1)
			break;
		Sleep(5);
		Timeout += 5;
		if(Timeout > FLASH_OP_TIMEOUT)
		{
			DbgPrint("  W25XXX_WR_Page>FLASH_ReadStatusReg timeout");
			return FALSE;
		}
	}while( temp & 0x01 );										     			/* �ȴ���ǰ����ִ����� */  
	
	return TRUE;
}


/*******************************************************************************
* Function Name  : SST25XXX_WriteBlock
* Description    : �鷽ʽд(���ٷ�ʽ)
*			       ע��: ���ڲ�ͬ��оƬ,���������ܲ�����.����SST25VF512B,������ʽ��һ��	
* Input          : address---׼��д���׵�ַ
*				   length----׼��д�����ݳ���
*				   *pbuf-----׼��д��Ļ�������ַ
* Output         : None
* Return         : None
�ٶȲ��ԣ�д512�ֽ���Ҫ���3MS
		  д4K�����Ҫ24.5MS
*******************************************************************************/
BOOL SST25XXX_WriteBlock( UINT32 address, UINT32 length, UINT8 *pbuf )
{
	UINT8  temp; 
	UINT32 i;	

	ULONG iLen=0,Timeout;
	UCHAR DBuf[4096]={0};
	
	/* д֮ǰ�����ȴ�дʹ��,�ұ���д�ڲ������ĵ�ַ�� */
	FLASH_WriteEnable( );									 					/* ��дʹ�� */
	
	/* ����Auto Address Increment (AAI) Program��ʽ,����д���� */
	DBuf[0] = CMD_FLASH_AAI_WORD_PROG;
	DBuf[1] = (UINT8)( address >> 16 );
	DBuf[2] = (UINT8)( address >> 8 );
	DBuf[3] = (UINT8) address;
	memcpy(&DBuf[4],pbuf,2);
	iLen = 0;
	if( !CH347SPI_WriteRead( DevIndex, 0x80, iLen+4+2, DBuf) )
	{
		DbgPrint("  SST25XXX_WriteBlock>>CH347SPI_WriteRead-1 failure.");
		return FALSE;
	}

	Timeout = 0;
	do
	{
		temp=FLASH_ReadStatusReg( );	
		if( (temp & 0x01)<1)
			break;

		Sleep(0);
		Timeout += 1;
		if(Timeout > FLASH_OP_TIMEOUT)
		{
			DbgPrint("  SST25XXX_WriteBlock>FLASH_ReadStatusReg1 timeout");
			return FALSE;
		}
	}while( temp & 0X01 );										 				/* �ȴ���ǰ����ִ����� */

	for( i = 2; i < length; i = i + 2  )					 		 			/* ���ͺ������� */
	{
		DBuf[0] = CMD_FLASH_AAI_WORD_PROG;		/* ����д�������� */
		memcpy(&DBuf[1],&pbuf[i],2);
		iLen = 0;
		if( !CH347SPI_WriteRead( DevIndex, 0x80, iLen+3, DBuf) )
		{
			DbgPrint("  SST25XXX_WriteBlock>>CH347SPI_WriteRead-2 failure.");
			return FALSE;
		}

		/* �ȴ��������� */
		Timeout = 0;
		do
		{
			temp = FLASH_ReadStatusReg( );	
			if( (temp & 0x01)<1)
				break;

			Sleep(0);
			Timeout += 1;
			if(Timeout > FLASH_OP_TIMEOUT)
			{
				DbgPrint("  SST25XXX_WriteBlock>FLASH_ReadStatusReg2 timeout");
				return FALSE;
			}
		}while( temp & 0X01 );										 			/* �ȴ���ǰ����ִ����� */
	}

	/* д���֮��,�ر�дʹ���˳�AAIģʽ */
	FLASH_WriteDisable( );	  								 					/* �ر�дʹ�� */
	Timeout = 0;
	do
	{
		temp = FLASH_ReadStatusReg( );	
		if( (temp & 0x01)<1)
			break;

		Sleep(0);
		Timeout += 1;
		if(Timeout > FLASH_OP_TIMEOUT)
		{
			DbgPrint("  SST25XXX_WriteBlock>FLASH_ReadStatusReg3 timeout");
			return FALSE;
		}
	}while( temp & 0X01 );										     			/* �ȴ���ǰ����ִ����� */
	return TRUE;
}
/*******************************************************************************
* Function Name  : W25XXX_WR_Block
* Description    : д��һ������
* Input          : address----׼��д���׵�ַ
*				   len--------׼��д�����ݳ���
*				   *pbuf------׼��д��Ļ�������ַ
* Output         : None
* Return         : None
*******************************************************************************/
UINT32 W25XXX_WR_Block( UINT8 *pbuf, UINT32 address, UINT32 len )
{
	UINT32 NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;

	Addr = address % SPI_FLASH_PageSize;
	count = SPI_FLASH_PageSize - Addr;
	NumOfPage =  len / SPI_FLASH_PageSize;
	NumOfSingle = len % SPI_FLASH_PageSize;

	if( Addr == 0 ) 															/* WriteAddr is SPI_FLASH_PageSize aligned  */
	{
		if( NumOfPage == 0 ) 													/* NumByteToWrite < SPI_FLASH_PageSize */
		{
			W25XXX_WR_Page( pbuf, address, len );
		}
		else 																	/* NumByteToWrite > SPI_FLASH_PageSize */
		{
			while( NumOfPage-- )
			{
				W25XXX_WR_Page( pbuf, address, SPI_FLASH_PageSize );
				address +=  SPI_FLASH_PageSize;
				pbuf += SPI_FLASH_PageSize;
			}
			if(NumOfSingle)
				W25XXX_WR_Page( pbuf, address, NumOfSingle );
		}
	}
	else 																		/* WriteAddr is not SPI_FLASH_PageSize aligned  */
	{
		if( NumOfPage == 0 ) 													/* NumByteToWrite < SPI_FLASH_PageSize */
		{
			if( NumOfSingle > count ) 											/* (NumByteToWrite + WriteAddr) > SPI_FLASH_PageSize */
			{
				temp = NumOfSingle - count;
				W25XXX_WR_Page( pbuf, address, count );
				address +=  count;
				pbuf += count;
				W25XXX_WR_Page( pbuf, address, temp );
			}
			else
			{
				W25XXX_WR_Page( pbuf, address, len );
			}
		}
		else/* NumByteToWrite > SPI_FLASH_PageSize */
		{
			len -= count;
			NumOfPage =  len / SPI_FLASH_PageSize;
			NumOfSingle = len % SPI_FLASH_PageSize;
			W25XXX_WR_Page( pbuf, address, count );
			address +=  count;
			pbuf += count;
			while( NumOfPage-- )
			{
				W25XXX_WR_Page( pbuf, address, SPI_FLASH_PageSize );
				address += SPI_FLASH_PageSize;
				pbuf += SPI_FLASH_PageSize;
			}
			if( NumOfSingle != 0 )
			{
				W25XXX_WR_Page( pbuf, address, NumOfSingle );
			}
		}
	}
	return len;
}

/*******************************************************************************
* Function Name  : FLASH_RD_Block
* Description    : FLASH�����ݶ�ȡ
* Input          : None 
* Output         : None
* Return         : None
*******************************************************************************/  
UINT32 FLASH_RD_Block( UINT32 address,UINT8 *pbuf,UINT32 len )
{
	/* W25ϵ��FLASH��SSTϵ��FLASH */
	ULONG iLen=0;
	UCHAR DBuf[8192]={0};

	DBuf[0] = CMD_FLASH_READ;
	DBuf[1] = (UINT8)( address >> 16 );
	DBuf[2] = (UINT8)( address >> 8 );
	DBuf[3] = (UINT8)( address );	
	
	iLen = len;
	if( !CH347SPI_Read(DevIndex,0x80,4,&iLen,DBuf) )		
	{
		//DbgPrint("  FLASH_RD_Block %d bytes failure.");
		return 0;
	}
	else
	{
		memcpy(pbuf,DBuf,len);
		return len;
	}	
	
}

BOOL FLASH_Erase_Sector( UINT32 address )								/* FLASH�������� */
{
	UINT8  temp;
	ULONG iLen=0,Timeout =0;
	UCHAR DBuf[8192]={0};

	/* ����֮ǰ,�������дʹ�ܲ��� */
	FLASH_WriteEnable( );									 					/* ��дʹ�� */

	/* �ж������MT25QXXXоƬ����Ҫ�жϵ�ַ����ֽ�,��ǰ���������ĸ����  */
	if( Flash_Type == DEF_TYPE_MT25QXXX )
	{
		MT25QXXX_ChangeExAddrReg( (UINT8)( address >> 24 ) );
	}

	/* ����FLASH���� */
	//PIN_FLASH_CS_LOW( );
	if( Flash_Type == DEF_TYPE_M25PXXX )
	{
	    //SPI_FLASH_SendByte( CMD_M25PXX_SECTOR_ERASE );
		DBuf[0] = CMD_M25PXX_SECTOR_ERASE;
	}
	else
	{
		DBuf[0] = CMD_FLASH_SECTOR_ERASE;
    }

	DBuf[1] = (UINT8)( address >> 16 ); /* ���������ֽ���Ҫ��ȡ���ݵĵ�ַ */	
	DBuf[2] = (UINT8)( address >> 8 );  /* ���ݵ�ַ���ֽ� */
	DBuf[3] = (UINT8)address;           /* ���ݵ�ַ����ֽ� */ 

	iLen = 0;
	if( !CH347SPI_WriteRead( DevIndex, 0x80, iLen+4, DBuf) )
	{
		DbgPrint("  FLASH_Erase_Sector>>CH347SPI_WriteRead failure.");
		return FALSE;
	}

	/* �ȴ��������� */
	do
	{
		temp = FLASH_ReadStatusReg( );
		if( (temp & 0x01)<1)
			break;
		Sleep(30);
		Timeout += 30;
		if(Timeout > FLASH_OP_TIMEOUT)
		{
			DbgPrint("  FLASH_Erase_Sector>FLASH_ReadStatusReg timeout");
			return FALSE;
		}
	}while( temp & 0x01 );	

	return TRUE;
}

BOOL FLASH_Erase_Block( UINT32 address )								/* FLASH�������� */
{
	UINT8  temp;
	ULONG iLen=0,Timeout =0;
	UCHAR DBuf[8192]={0};

	/* ����֮ǰ,�������дʹ�ܲ��� */
	FLASH_WriteEnable( );									 					/* ��дʹ�� */

	/* �ж������MT25QXXXоƬ����Ҫ�жϵ�ַ����ֽ�,��ǰ���������ĸ����  */
	if( Flash_Type == DEF_TYPE_MT25QXXX )
	{
		MT25QXXX_ChangeExAddrReg( (UINT8)( address >> 24 ) );
	}

	/* ����FLASH���� */
	//PIN_FLASH_CS_LOW( );
	if( Flash_Type == DEF_TYPE_M25PXXX )
	{
	    //SPI_FLASH_SendByte( CMD_M25PXX_SECTOR_ERASE );
		DBuf[0] = CMD_M25PXX_SECTOR_ERASE;
	}
	else
	{
	    //SPI_FLASH_SendByte( CMD_FLASH_SECTOR_ERASE );                           
		DBuf[0] = CMD_FLASH_BLOCK_ERASE1;/* ����4-KByte Sector-Erase���� */
    }

	DBuf[1] = (UINT8)( address >> 16 );/* ���������ֽ���Ҫ��ȡ���ݵĵ�ַ */	
	DBuf[2] = (UINT8)( address >> 8 );/* ���ݵ�ַ���ֽ� */	
	DBuf[3] = (UINT8)address;/* ���ݵ�ַ����ֽ� */ 

	iLen = 0;
	if( !CH347SPI_WriteRead( DevIndex, 0x80, iLen+4, DBuf) )
	{
		DbgPrint("  FLASH_Erase_Block>>CH347SPI_WriteRead failure.");
		return FALSE;
	}

	/* �ȴ��������� */
	do
	{
		temp = FLASH_ReadStatusReg( );
		if( (temp & 0x01)<1)
			break;
		Sleep(10);
		Timeout += 10;
		if(Timeout > FLASH_OP_TIMEOUT)
		{
			DbgPrint("  FLASH_Erase_Block>FLASH_ReadStatusReg timeout");
			return FALSE;
		}
	}while( temp & 0x01 );	

	return TRUE;
}


BOOL FLASH_Erase_Full()								/* FLASHȫƬ���� */
{
	UINT8  temp;
	ULONG iLen=0,Timeout =0;
	UCHAR DBuf[8192]={0};

	/* ����֮ǰ,�������дʹ�ܲ��� */
	FLASH_WriteEnable( );						 					/* ��дʹ�� */
	
	//SPI_FLASH_SendByte( CMD_FLASH_SECTOR_ERASE );                           /* ����4-KByte Sector-Erase���� */
	DBuf[0] = CMD_FLASH_CHIP_ERASE1;
   
	iLen = 0;
	if( !CH347SPI_WriteRead( DevIndex, 0x80, iLen+1, DBuf) )
	{
		DbgPrint("  FLASH_Erase_Block>>CH347SPI_WriteRead failure.");
		return FALSE;
	}

	/* �ȴ��������� */
	do
	{
		temp = FLASH_ReadStatusReg( );
		if( (temp & 0x01)<1)
			break;
		Sleep(10);
		Timeout += 10;
		if(Timeout > FLASH_OP_TIMEOUT*20)
		{
			DbgPrint("  FLASH_Erase_Block>FLASH_ReadStatusReg timeout");
			return FALSE;
		}
	}while( temp & 0x01 );	

	return TRUE;
}

UINT32 FLASH_WR_Block( UINT32 address, UINT8 *pbuf, UINT32 len )			/* FLASH������д�� */
{
	/* W25ϵ��FLASH��SSTϵ��FLASH */
	if( Flash_Type == DEF_TYPE_W25XXX )
	{
		return W25XXX_WR_Block( pbuf, address, len );
	}
	else if( Flash_Type == DEF_TYPE_SST25XXX )
	{
		return SST25XXX_WriteBlock( address, len, pbuf );
	}
	else
	{
		return W25XXX_WR_Block( pbuf, address, len );
	}
	return 0;
}
