/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2023                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************/
// USB总线接口芯片CH346应用层接口库,CH346基于480Mbps高速USB总线扩展UART/SPI SLAVE,UART/PARALLEL SLAVE
// CH346-DLL  V1.1
// 运行环境: Windows 98/ME, Windows 2000/XP, WIN7/8/10/11,and later.
// support USB chip: CH341, CH341A,CH347
//Notes:
//Copyright (C) 2024 Nanjing Qinheng Microelectronics Co., Ltd.


#ifndef		_CH346_DLL_H
#define		_CH346_DLL_H

#ifdef __cplusplus
extern "C" {
#endif

#define		mOFFSET( s, m )			( (ULONG) (ULONG_PTR)& ( ( ( s * ) 0 ) -> m ) )	// 定义获取结构成员相对偏移地址的宏

#ifndef		max
#define		max( a, b )				( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )	// 较大值
#endif

#ifndef		min
#define		min( a, b )				( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )	// 较小值
#endif

#ifdef		ExAllocatePool
#undef		ExAllocatePool						// 删除带TAG的内存分配
#endif

#ifndef		NTSTATUS
typedef		LONG	NTSTATUS;					// 返回状态
#endif


#define		mCH346_PACKET_LENGTH	64			// CH346支持的数据包的长度
#define		mCH346_PKT_LEN_SHORT	8			// CH346支持的短数据包的长度

#ifndef _CH375_DLL_H
typedef	struct	_USB_SETUP_PKT {				// USB控制传输的建立阶段的数据请求包结构
	UCHAR			mUspReqType;				// 00H 请求类型
	UCHAR			mUspRequest;				// 01H 请求代码
	union	{
		struct	{
			UCHAR	mUspValueLow;				// 02H 值参数低字节
			UCHAR	mUspValueHigh;				// 03H 值参数高字节
		};
		USHORT		mUspValue;					// 02H-03H 值参数
	};
	union	{
		struct	{
			UCHAR	mUspIndexLow;				// 04H 索引参数低字节
			UCHAR	mUspIndexHigh;				// 05H 索引参数高字节
		};
		USHORT		mUspIndex;					// 04H-05H 索引参数
	};
	USHORT			mLength;					// 06H-07H 数据阶段的数据长度
} mUSB_SETUP_PKT, *mPUSB_SETUP_PKT;

typedef	struct	_WIN32_COMMAND {				// 定义WIN32命令接口结构
	union	{
		ULONG		mFunction;					// 输入时指定功能代码或者管道号
		NTSTATUS	mStatus;					// 输出时返回操作状态
	};
	ULONG			mLength;					// 存取长度,返回后续数据的长度
	union	{
		mUSB_SETUP_PKT	mSetupPkt;				// USB控制传输的建立阶段的数据请求
		UCHAR			mBuffer[ mCH346_PACKET_LENGTH ];	// 数据缓冲区,长度为0至255B
	};
} mWIN32_COMMAND, *mPWIN32_COMMAND;
#endif

// WIN32应用层接口命令
#define		IOCTL_CH346_COMMAND		( FILE_DEVICE_UNKNOWN << 16 | FILE_ANY_ACCESS << 14 | 0x0f37 << 2 | METHOD_BUFFERED )	// 专用接口

#define		mWIN32_COMMAND_HEAD		mOFFSET( mWIN32_COMMAND, mBuffer )	// WIN32命令接口的头长度

#define		mCH346_MAX_NUMBER		16			// 最多同时连接的CH346数

#define		mMAX_BUFFER_LENGTH		0x400000		// 数据缓冲区最大长度4MB

#define		mMAX_COMMAND_LENGTH		( mWIN32_COMMAND_HEAD + mMAX_BUFFER_LENGTH )	// 最大数据长度加上命令结构头的长度

#define		mDEFAULT_BUFFER_LEN		0x0400		// 数据缓冲区默认长度1024

#define		mDEFAULT_COMMAND_LEN	( mWIN32_COMMAND_HEAD + mDEFAULT_BUFFER_LEN )	// 默认数据长度加上命令结构头的长度

// 设备层接口提供的管道操作命令
#define		mPipeDeviceCtrl			0x00000004	// CH346的综合控制管道
#define		mPipeInterUp			0x00000005	// CH346的中断数据上传管道
#define		mPipeDataUp				0x00000006	// CH346的数据块上传管道
#define		mPipeDataDown			0x00000007	// CH346的数据块下传管道
#define		mPipeAuxDown			0x00000008	// CH346的辅助数据下传管道

// 应用层接口的功能代码
#define		mFuncNoOperation		0x00000000	// 无操作
#define		mFuncGetVersion			0x00000001	// 获取驱动程序版本号
#define		mFuncGetConfig			0x00000002	// 获取USB设备配置描述符
#define		mFuncSetExclusive		0x0000000b	// 设置独占使用
#define		mFuncResetDevice		0x0000000c	// 复位USB设备
#define		mFuncResetPipe			0x0000000d	// 复位USB管道
#define		mFuncAbortPipe			0x0000000e	// 取消USB管道的数据请求
#define		mFuncSetTimeout			0x0000000f	// 设置USB通讯超时
#define		mFuncBufferMode			0x00000010	// 设定缓冲上传模式及查询缓冲区中的数据长度
#define		mFuncBufferModeDn		0x00000011	// 设定缓冲下传模式及查询缓冲区中的数据长度
#define		mFuncSetIOMode  		0x00000012	// 在缓冲上传模式时，设定发送是否同步

// USB设备标准请求代码
#define		mUSB_CLR_FEATURE		0x01
#define		mUSB_SET_FEATURE		0x03
#define		mUSB_GET_STATUS			0x00
#define		mUSB_SET_ADDRESS		0x05
#define		mUSB_GET_DESCR			0x06
#define		mUSB_SET_DESCR			0x07
#define		mUSB_GET_CONFIG			0x08
#define		mUSB_SET_CONFIG			0x09
#define		mUSB_GET_INTERF			0x0a
#define		mUSB_SET_INTERF			0x0b
#define		mUSB_SYNC_FRAME			0x0c


#define		MAX_DEVICE_PATH_SIZE	128			// 设备名称的最大字符数
#define		MAX_DEVICE_ID_SIZE		64			// 设备ID的最大字符数

#pragma pack(1) 
//设备信息
typedef struct _DEV_INFOR{
	UCHAR    iIndex;                 // 当前打开序号
	UCHAR    DevicePath[MAX_PATH];   // 设备链接名,用于CreateFile
	UCHAR    UsbClass;               // 驱动类别 0:CH347_USB_CH341, 2:CH347_USB_HID,3:CH347_USB_VCP
	UCHAR    FuncType;               // 功能类别 0:CH347_FUNC_UART,1:CH347_FUNC_SPI_I2C,2:CH347_FUNC_JTAG_I2C
	CHAR     DeviceID[64];           // USB\VID_xxxx&PID_xxxx
	UCHAR    ChipMode;               // 芯片工作模式,0:Mode0(UART0/1); 1:Mode1(Uart1+SPI+I2C); 2:Mode2(HID Uart1+SPI+I2C) 3:Mode3(Uart1+Jtag+IIC) 4:CH347F(Uart*2+Jtag/SPI/IIC)
	HANDLE   DevHandle;              // 设备句柄
	USHORT   BulkOutEndpMaxSize;     // 批量上传端点大小
	USHORT   BulkInEndpMaxSize;      // 批量下传端点大小
	UCHAR    UsbSpeedType;           // USB速度类型，0:FS,1:HS,2:SS
	UCHAR    CH347IfNum;             // USB接口号: CH347T: IF0:UART;   IF1:SPI/IIC/JTAG/GPIO
									 //            CH347F: IF0:UART0;  IF1:UART1; IF 2:SPI/IIC/JTAG/GPIO
	UCHAR    DataUpEndp;             // 批量上传端点地址
	UCHAR    DataDnEndp;             // 批量下传端点地址
	CHAR     ProductString[64];      // USB产品字符串
	CHAR     ManufacturerString[64]; // USB厂商字符串	
	ULONG    WriteTimeout;           // USB写超时
	ULONG    ReadTimeout;            // USB读超时
	CHAR     FuncDescStr[64];        // 接口功能描述符
	UCHAR    FirewareVer;            // 固件版本,十六进制值
	CHAR     SerialNumberString[64]; // USB产品序列号字符串
}mDeviceInforS,*mPDeviceInforS;

#pragma pack() 
// 打开设备,返回句柄,出错则无效
HANDLE	WINAPI	CH346OpenDevice(ULONG			iIndex ); // 指定设备序号,0对应第一个设备,-1则自动搜索一个可以被打开的设备并返回序号

// 关闭设备
VOID	WINAPI	CH346CloseDevice(ULONG			iIndex );  // 指定设备序号

// 设置芯片工作模式.模式,0: Uart + 并口; 模式1:Uart + SPI
BOOL	WINAPI	CH346SetChipMode(ULONG			iIndex,     // 指定设备序号
								 UCHAR          iChipMode,  // 芯片工作模式,0: Uart + 并口; 1:Uart + SPI
								 BOOL           iIsSave);   // 是否掉电保存

// 获取设备信息
BOOL WINAPI CH346GetDeviceInfor(ULONG iIndex,mDeviceInforS *DevInformation);

// 读取USB序列号字符串。SNString可作为设备的识别码
BOOL	WINAPI	CH346GetSnString(ULONG			iIndex,      // 指定设备序号
								PVOID			oBuffer,     // 指向一个足够大的缓冲区,用于保存描述符
								PULONG			ioLength );  // 指向长度单元,输入时为准备读取的长度,返回后为实际读取的长度

// 复位USB设备
BOOL	WINAPI	CH346ResetDevice(ULONG			iIndex );  // 指定设备序号

// 返回指向设备名称的缓冲区,出错则返回NULL。此名称可用于CreateFile打开设备
PVOID	WINAPI	CH346GetDeviceName(ULONG			iIndex );  // 指定设备序号,0对应第一个设备

#define		CH346_DEVICE_ARRIVAL		3		// 设备插入事件,已经插入
#define		CH346_DEVICE_REMOVE_PEND	1		// 设备将要拔出
#define		CH346_DEVICE_REMOVE			0		// 设备拔出事件,已经拔出

typedef		VOID	( CALLBACK	* mPCH346_NOTIFY_ROUTINE ) (  // 设备事件通知回调程序
	ULONG			iEventStatus );  // 设备事件和当前状态(在下行定义): 0=设备拔出事件, 3=设备插入事件

// 设定设备USB插拔后事件通知程序
BOOL	WINAPI	CH346SetDeviceNotify(ULONG					iIndex,            // 指定设备序号,0对应第一个设备
									 PCHAR					iDeviceID,         // 可选参数,指向字符串,指定被监控的设备的ID,字符串以\0终止
									 mPCH346_NOTIFY_ROUTINE	iNotifyRoutine );  // 指定设备事件回调程序,为NULL则取消事件通知,否则在检测到事件时调用该程序

// 设置USB数据读写的超时
BOOL	WINAPI	CH346SetTimeout(ULONG			iIndex,           // 指定设备序号
								ULONG			iWriteTimeout,    // 指定USB写出数据块的超时时间,以毫秒mS为单位,0xFFFFFFFF指定不超时(默认值)
								ULONG			iReadTimeout);    // 指定USB读取数据块的超时时间,以毫秒mS为单位,0xFFFFFFFF指定不超时(默认值)	

// SPI/并口数据读。
// 如芯片在模式0,则通过USB获取硬件并口接收到的数据
// 如芯片在模式1,则通过USB获取硬件SPI接口接收到的数据
BOOL	WINAPI	CH346ReadData(ULONG			iIndex,      // 指定设备序号	
							  PVOID			oBuffer,     // 指向一个足够大的缓冲区,用于保存读取的数据,不超过4MB
							  PULONG		ioLength );  // 指向长度单元,输入时为准备读取的长度,返回后为实际读取的长度	
// SPI/并口数据写
// 如芯片在模式0,通过USB往并口发送数据
// 如芯片在模式1,通过USB,往SPI发送数据
BOOL	WINAPI	CH346WriteData(ULONG			iIndex,      // 指定设备序号	
							   PVOID			iBuffer,     // 指向一个缓冲区,放置准备写出的数据
							   PULONG		    ioLength );  // 指向长度单元,输入时为准备写出的长度,返回后为实际写出的长度

// 中止当前USB读操作,适用于USB直接读操作(非缓冲上传模式)
BOOL	WINAPI	CH346AbortRead(ULONG			iIndex);  // 指定设备序号

// 中止当前USB写操作,适用于USB直接写操作(非缓冲上传模式)
BOOL	WINAPI	CH346AbortWrite(ULONG			iIndex ); // 指定设备序号

// 设定内部缓冲上传模式。读速度较直接上传模式高,上传的数据包大小,必须是BufSize所设定的大小。
// 如启用,驱动通过USB主动轮询硬件并口或SPI口接收的数据,并缓存至驱动缓冲区内,CH346ReadData函数直接从内部缓冲区内获取数据
BOOL	WINAPI	CH346SetBufUpload( ULONG			iIndex,        // 指定设备序号,0对应第一个设备
									ULONG			iEnableOrClear,// 为0则禁止内部缓冲上传模式,使用直接上传,非0则启用内部缓冲上传模式并清除缓冲区中的已有数据
									ULONG			BufSize );     // 每包缓冲区大小，最大4MB

// 查询内部上传缓冲区中的已有数据包个数和总字节数,成功返回TRUE,失败FALSE
BOOL	WINAPI	CH346QueryBufUpload(ULONG		iIndex,			// 指定设备序号,0对应第一个设备
									 PULONG		oPacketNum,		// 返回内部缓冲区内已接收数据包个数	
									 PULONG		oTotalLen );    // 返回内部缓冲区内已接收数据总字节数,为包大小BufSize的倍数

// 清空内部上传缓冲区数据,无需暂停内部缓冲上传模式
BOOL	WINAPI	CH346ClearBufUpload(ULONG			iIndex); // 指定设备序号,0对应第一个设备

// 设定内部缓冲下传模式,写速度比直接下传模式高. 
// 如启用,CH346WriteData写入数据,先缓存至驱动内部下传缓冲区内后直接返回。由驱动通过USB轮询下发数据至硬件并口或SPI口
BOOL	WINAPI	CH346SetBufDownload(ULONG			iIndex,           // 指定设备序号,0对应第一个设备
									ULONG			iEnableOrClear,   // 为0则禁止内部缓冲下传模式,使用直接下传,非0则启用内部缓冲下传模式并清除缓冲区中的已有数据
									ULONG		    iPacketCnt);      // 最多缓存的包数量，最大10包

// 查询内部下传缓冲区中的剩余数据包个数(尚未发送),成功返回数据包个数,成功返回TRUE,失败FALSE
BOOL	WINAPI	CH346QueryBufDownload(ULONG			iIndex,			// 指定设备序号,0对应第一个设备	
									  PULONG		oPacketCnt,		// 返回内部缓冲中的剩余数据包个数
									  PULONG		oTotalLen ); 	// 返回内部缓冲中的剩余数据包总字节数	


#ifdef __cplusplus
}
#endif

#endif		// _CH346_DLL_H