/*****************************************************************************
**                      Copyright  (C)  WCH  2001-2022                      **
**                      Web:  http://wch.cn                                 **
******************************************************************************
Abstract:
    USB2.0 adapter chip ch347, based on 480mbps high-speed USB bus, extends UART/SPI/I2C/JTAG
Environment:
user mode only,VC6.0 and later
Notes:
Copyright (c) 2022 Nanjing Qinheng Microelectronics Co., Ltd.
--*/

#ifndef		_CH347_DLL_H
#define		_CH347_DLL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN64
#define		mOFFSET( s, m )			( (ULONG_PTR) & ( ( ( s * ) 0 ) -> m ) )	// Defines a macro that gets the relative offset address of a structure member
#else
#define		mOFFSET( s, m )			( (ULONG) & ( ( ( s * ) 0 ) -> m ) )	// Defines a macro that gets the relative offset address of a structure member
#endif

#ifndef		max
#define		max( a, b )				( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )	// Larger value
#endif

#ifndef		min
#define		min( a, b )				( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )	// Smaller Value
#endif

#ifdef		ExAllocatePool
#undef		ExAllocatePool						// Delete memory allocation with TAG
#endif

#ifndef		NTSTATUS
typedef		LONG	NTSTATUS;					// Return status
#endif

//Sharing CH341WDM driver with CH341DLL
#ifndef _CH341_DLL_H
typedef	struct	_USB_SETUP_PKT {				// Data request packet structure in the setup phase of USB control transfer
	UCHAR			mUspReqType;				// 00H request type
	UCHAR			mUspRequest;				// 01H request code
	union	{
		struct	{
			UCHAR	mUspValueLow;				// 02H Value parameter low byte
			UCHAR	mUspValueHigh;				// 03H Value parameter high byte
		};
		USHORT		mUspValue;					// 02H-03H value parameters
	};
	union	{
		struct	{
			UCHAR	mUspIndexLow;				// 04H index parameter low byte
			UCHAR	mUspIndexHigh;				// 05H index parameter high byte
		};
		USHORT		mUspIndex;					// 04H-05H index parameter
	};
	USHORT			mLength;					// 06H-07H data length of data stage
} mUSB_SETUP_PKT, *mPUSB_SETUP_PKT;


typedef	struct	_WIN32_COMMAND {				// Define the WIN32 command interface structure
	union	{
		ULONG		mFunction;					// Specify function code or pipe number when inputting
		NTSTATUS	mStatus;					// return operation status when output
	};
	ULONG			mLength;					// access length, return the length of subsequent data
	union	{
		mUSB_SETUP_PKT	mSetupPkt;				// Data request in the setup phase of USB control transfer
		UCHAR			mBuffer[ 512];      	// data buffer, the length is 0 to 255B
	};
} mWIN32_COMMAND, *mPWIN32_COMMAND;
// WIN32 Application layer interface command
#define		IOCTL_CH341_COMMAND		( FILE_DEVICE_UNKNOWN << 16 | FILE_ANY_ACCESS << 14 | 0x0f34 << 2 | METHOD_BUFFERED )	// private interface

#define		mWIN32_COMMAND_HEAD		mOFFSET( mWIN32_COMMAND, mBuffer )	// Header length of WIN32 command interface

#define		mCH341_MAX_NUMBER		16			// Maximum number of CH375 connected at the same time

#define		mMAX_BUFFER_LENGTH		0x1000		// The maximum length of the data buffer is 4MB

#define		mMAX_COMMAND_LENGTH		( mWIN32_COMMAND_HEAD + mMAX_BUFFER_LENGTH )	// maximum data length plus the length of the command structure header

#define		mDEFAULT_BUFFER_LEN		0x0400		// The default length of the data buffer is 1024

#define		mDEFAULT_COMMAND_LEN	( mWIN32_COMMAND_HEAD + mDEFAULT_BUFFER_LEN )	// default data length plus the length of the command structure header

// CH341 endpoint address
#define		mCH347_ENDP_DATA_UP		0x86		// The address of the data block upload endpoint of CH347
#define		mCH347_ENDP_DATA_DOWN	0x06		// The address of the data block download endpoint of CH347

// Pipeline operation command provided by equipment layer interface
#define		mPipeDeviceCtrl			0x00000004	// CH347 integrated control pipeline
#define		mPipeDataUp				0x00000006	// CH347 data block upload pipeline
#define		mPipeDataDown			0x00000007	// CH347 data block download pipeline

// Function code of application layer interface
#define		mFuncNoOperation		0x00000000	// no operation
#define		mFuncGetVersion			0x00000001	// Get the driver version number
#define		mFuncGetConfig			0x00000002	// Get the USB device configuration descriptor
#define		mFuncSetTimeout			0x00000009	// Set USB communication timeout
#define		mFuncSetExclusive		0x0000000b	// Set exclusive use
#define		mFuncResetDevice		0x0000000c	// Reset the USB device
#define		mFuncResetPipe			0x0000000d	// Reset the USB pipe
#define		mFuncAbortPipe			0x0000000e	// Cancel the data request of the USB pipe
#define		mFuncBufferMode			0x00000020	// Set the buffer upload mode and query the data length in the buffer
#define		mFuncBufferModeDn		0x00000021	// Set the buffer download mode and query the data length in the buffer
#define		mFuncGetVersionEx		0x00000022	// Get the driver version number and chip model
// USB device standard request code
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

// CH341 manufacturer specific request type for control transmission
#define		mCH341_VENDOR_READ		0xC0		// CH347 vendor-specific read operation through control transfer
#define		mCH341_VENDOR_WRITE		0x40		// CH347 vendor-specific write operation through control transfer

#define		mCH341A_CMD_I2C_STREAM	0xAA		// The command package of the I2C interface, starting from the secondary byte, is the I2C command stream
#define		mCH341A_CMD_UIO_STREAM	0xAB		// The command package of the UIO interface, starting from the secondary byte, is the command stream
#define		mCH341A_CMD_PIO_STREAM	0xAE		// The command package of PIO interface, starting from the secondary byte, is the data stream
// CH341A manufacturer specific request code for control transmission
#define		mCH341A_BUF_CLEAR		0xB2		// Clear incomplete data
#define		mCH341A_I2C_CMD_X		0x54		// Issue the command of I2C interface and execute it immediately
#define		mCH341A_DELAY_MS		0x5E		// Delay the specified time in microseconds
#define		mCH341A_GET_VER			0x5F		// Get chip version

#define		mCH341A_CMD_I2C_STM_STA	0x74		// Command flow of I2C interface: generate start bit
#define		mCH341A_CMD_I2C_STM_STO	0x75		// Command flow of I2C interface: generate stop bit
#define		mCH341A_CMD_I2C_STM_OUT	0x80		// Command flow of I2C interface: output data, bit 5- bit 0 is the length, subsequent bytes are data, and length 0 only sends one byte and returns an answer
#define		mCH341A_CMD_I2C_STM_IN	0xC0		// I2C interface command flow: input data, bit 5-bit 0 is the length, and 0 length only receives one byte and sends no response
#define		mCH341A_CMD_I2C_STM_MAX	( min( 0x3F, mCH341_PACKET_LENGTH ) )	// The maximum length of input and output data of a single command in the command stream of I2C interface
#define		mCH341A_CMD_I2C_STM_SET	0x60		// Command flow of I2C interface: set parameters, bit 2=i/o number of SPI (0= single input single output, 1= double input double output), bit 1 0=i2c speed (00= low speed, 01= standard, 10= fast, 11= high speed)
#define		mCH341A_CMD_I2C_STM_US	0x40		// Command flow of I2C interface: delay in microseconds, bit 3- bit 0 as delay value
#define		mCH341A_CMD_I2C_STM_MS	0x50		// Command flow of I2C interface: delay in microseconds, bit 3-bit 0 as delay value
#define		mCH341A_CMD_I2C_STM_DLY	0x0F		// Maximum value of single command delay of command flow of I2C interface
#define		mCH341A_CMD_I2C_STM_END	0x00		// Command flow of I2C interface: Command package ends in advance

#define		mCH341A_CMD_UIO_STM_IN	0x00		// Command flow of UIO interface: input data D7-D0
#define		mCH341A_CMD_UIO_STM_DIR	0x40		// Command flow of UIO interface: set i/o direction D5-D0, bit 5- bit 0 as direction data
#define		mCH341A_CMD_UIO_STM_OUT	0x80		// Command flow of UIO interface: output data D5-D0, bit 5-bit 0 is data
#define		mCH341A_CMD_UIO_STM_US	0xC0		// Command flow of UIO interface: delay in microseconds, bit 5- bit 0 as delay value
#define		mCH341A_CMD_UIO_STM_END	0x20		// Command flow of UIO interface: Command package ends in advance

#define		MAX_DEVICE_PATH_SIZE	128			// Maximum number of characters for device name
#define		MAX_DEVICE_ID_SIZE		64			// Maximum number of characters for device ID
#endif _CH341_DLL_H

//Drive Interface
#define CH347_USB_VENDOR   0
#define CH347_USB_HID      2
#define CH347_USB_VCP      3

//CH347_USB_VENDOR support CH341/7
#define CHIP_TYPE_CH341    0
#define CHIP_TYPE_CH347    1

//Chip Function Interface Number
#define CH347_FUNC_UART      0
#define CH347_FUNC_SPI_IIC   1
#define CH347_FUNC_JTAG_IIC  2

#define DEFAULT_READ_TIMEOUT  500  // Default read timeout duration (mS)
#define DEFAULT_WRITE_TIMEOUT 500  // Default write timeout duration (mS)

#define		mCH347_PACKET_LENGTH	512		// Length of packets supported by ch347
#pragma pack(1) 
//SPI Controller Configuration
typedef struct _SPI_CONFIG{
	UCHAR           iMode;                 // 0-3:SPI Mode0/1/2/3
	UCHAR           iClock;                // 0=60MHz, 1=30MHz, 2=15MHz, 3=7.5MHz, 4=3.75MHz, 5=1.875MHz, 6=937.5KHz,7=468.75KHz
	UCHAR			iByteOrder;            // 0=LSB first(LSB), 1=MSB first(MSB)
	USHORT          iSpiWriteReadInterval; // The SPI interface routinely reads and writes data command, the unit is uS
	UCHAR           iSpiOutDefaultData;    // SPI prints data by default when it reads data
	ULONG			iChipSelect;           // Piece of selected control, if bit 7 is 0, slice selection control is ignored, if bit 7 is 1, the parameter is valid: bit 1 bit 0 is 00/01 and CS1/CS2 pins are selected as low level active chip options respectively
	UCHAR           CS1Polarity;           // Bit 0: CS1 polarity control: 0: effective low level; 1: effective lhigh level;
	UCHAR           CS2Polarity;           // Bit 0: CS2 polarity control: 0: effective low level; 1: effective lhigh level;
	USHORT          iIsAutoDeativeCS;      // Whether to undo slice selection automatically after the operation is complete
	USHORT          iActiveDelay;          // Set the latency for read/write operations after slice selection,the unit is us
	ULONG           iDelayDeactive;        // Delay time for read and write operations after slice selection is unselected,the unit is us
}mSpiCfgS,*mPSpiCfgS;

//Device Information 
typedef struct _DEV_INFOR{
	UCHAR    iIndex;                 // Current open number
	UCHAR    DevicePath[MAX_PATH];   // Device link name, used for CreateFile
	UCHAR    UsbClass;               // 0:CH347_USB_CH341, 2:CH347_USB_HID,3:CH347_USB_VCP
	UCHAR    FuncType;               // 0:CH347_FUNC_UART,1:CH347_FUNC_SPI_IIC,2:CH347_FUNC_JTAG_IIC
	CHAR     DeviceID[64];           // USB\VID_xxxx&PID_xxxx
	UCHAR    ChipMode;               // Chip Mode, 0:Mode0(UART0/1); 1:Mode1(Uart1+SPI+IIC); 2:Mode2(HID Uart1+SPI+IIC) 3:Mode3(Uart1+Jtag+IIC)
	HANDLE   DevHandle;              // Device handle
	USHORT   BulkOutEndpMaxSize;     // Upload endpoint size
	USHORT   BulkInEndpMaxSize;      // The size of the downstream endpoint
	UCHAR    UsbSpeedType;           // USB Speed type,0:FS,1:HS,2:SS
	UCHAR    CH347IfNum;             // USB interface number: 0:UART,1:SPI/IIC/JTAG/GPIO
	UCHAR    DataUpEndp;             // The endpoint address
	UCHAR    DataDnEndp;             // The endpoint address
	CHAR     ProductString[64];      // Product string in USB descriptor
	CHAR     ManufacturerString[64]; // Manufacturer string in USB descriptor
	ULONG    WriteTimeout;           // USB write timeout
	ULONG    ReadTimeout;            // USB read timeout
	CHAR     FuncDescStr[64];        // Interface functions
	UCHAR    FirewareVer;            // Firmware version

}mDeviceInforS,*mPDeviceInforS;
#pragma pack() 

//CH347 Mode Common Function,support open,close,USB read,USB written and HID of all modes.
//Open USB device
HANDLE WINAPI CH347OpenDevice(ULONG DevI);

//Close USB device
BOOL WINAPI CH347CloseDevice(ULONG iIndex);

//Get Device Information
BOOL WINAPI CH347GetDeviceInfor(ULONG iIndex,mDeviceInforS *DevInformation);

// Obtain driver version, library version, device version and chip type(CH341(FS)/CH347HS)
BOOL	WINAPI	CH347GetVersion(ULONG 			 iIndex, 
							    PUCHAR           iDriverVer,
								PUCHAR           iDLLVer,
							    PUCHAR           ibcdDevice,
							    PUCHAR           iChipType);  //CHIP_TYPE_CH341/7

typedef		VOID	( CALLBACK	* mPCH347_NOTIFY_ROUTINE ) (  								 // Device event notification callback routine
															ULONG			iEventStatus );  // Device event and current status (refer to the description below): 0=Device unplug event, 3=Device insertion event

#define		CH347_DEVICE_ARRIVAL		3		// Device insertion event,has been inserted
#define		CH347_DEVICE_REMOVE_PEND	1		// Device wil be unplugged
#define		CH347_DEVICE_REMOVE			0		// Device unplug event,has been pulled out

BOOL	WINAPI	CH347SetDeviceNotify(  											// Configure device event notifier
									 ULONG					iIndex,  			// Specifies the device number,bit 0 corresponds to the first device
									 PCHAR					iDeviceID,  		// Optional parameter,pointing to a string terminated by \0,specifying the ID of the monitored device
									 mPCH347_NOTIFY_ROUTINE	iNotifyRoutine );   // Specifies the port device event callback program. If it is NULL, the event notification will be canceled. Otherwise, the program will be called when an event is detected.

// Read USB data block
BOOL	WINAPI	CH347ReadData( ULONG			iIndex,      // Specifies the device number
							  PVOID			oBuffer,         // Points to a buffer large enough to save the read data
							  PULONG			ioLength );  // Points to the length unit, the length to be read when input is the length to be read, and the actual read length after return

// Write USB data block
BOOL	WINAPI	CH347WriteData(ULONG			iIndex,      // Specifies the device number
							   PVOID			iBuffer,     // Points to a buffer large enough to save the written data
							   PULONG			ioLength );  // Points to the length unit,the input length is the intended length, and the return length is the actual length

// Set the timeout of USB data read and write
BOOL	WINAPI	CH347SetTimeout(ULONG			iIndex,         // Specifies the device number
							    ULONG			iWriteTimeout,  // Specifies the timeout for USB to write data blocks, in milliseconds mS,0xFFFFFFFF specifies no timeout (default)
								ULONG			iReadTimeout ); // Specifies the timeout for USB to read data blocks, in milliseconds mS,0xFFFFFFFF specifies no timeout (default)

/***************SPI********************/
// SPI Controller Initialization
BOOL	WINAPI	CH347SPI_Init(ULONG iIndex,mSpiCfgS *SpiCfg);

// Get SPI controller configuration information
BOOL    WINAPI  CH347SPI_GetCfg(ULONG iIndex,mSpiCfgS *SpiCfg);

// Before setting the chip selection status, call CH347SPI_Init to set CS
BOOL	WINAPI	CH347SPI_ChangeCS(ULONG			iIndex,         // Specify device number	
								  UCHAR         iStatus);       // 0=Cancel the piece to choose,1=Set piece selected

// Set SPI slice selection
BOOL	WINAPI	CH347SPI_SetChipSelect(ULONG			iIndex,            // Specify device number
									   USHORT           iEnableSelect,     // The lower octet is CS1 and the higher octet is CS2. A byte value of 1= sets CS, 0= ignores this CS setting
									   USHORT           iChipSelect,       // The lower octet is CS1 and the higher octet is CS2. A byte value of 1= sets CS, 0= ignores this CS setting
									   ULONG            iIsAutoDeativeCS,  // The lower 16 bits are CS1 and the higher 16 bits are CS2. Whether to undo slice selection automatically after the operation is complete
									   ULONG            iActiveDelay,      // The lower 16 bits are CS1 and the higher 16 bits are CS2. Set the latency of read/write operations after chip selection, the unit is us
									   ULONG            iDelayDeactive);   // The lower 16 bits are CS1 and the higher 16 bits are CS2. Delay time for read and write operations after slice selection the unit is us

//SPI4 write data
BOOL	WINAPI	CH347SPI_Write(ULONG			iIndex,          // Specify device number	
							   ULONG			iChipSelect,     // Slice selection control, when bit 7 is 0, slice selection control is ignored, and when bit 7 is 1, slice selection operation is performed
							   ULONG			iLength,         // Number of bytes of data to be transferred	
							   ULONG			iWriteStep,      // The length of a single block to be read
							   PVOID			ioBuffer);       // Point to a buffer to place the data to be written out from MOSI

//SPI4 read data. No need to write data first, the efficiency is higher than that of the CH347SPI_WriteRead
BOOL	WINAPI	CH347SPI_Read(ULONG			iIndex,           // Specify device number
							  ULONG			iChipSelect,      // Slice selection control, when bit 7 is 0, slice selection control is ignored, and when bit 7 is 1, slice selection operation is performed
							  ULONG         oLength,          // Number of bytes to send
							  PULONG		iLength,          // Number of bytes of data to be read in
							  PVOID			ioBuffer);        // Points to a buffer that place the data to be written out from DOUT, return the data read in from DIN

// Handle SPI data stream 4-wire interface
BOOL	WINAPI	CH347SPI_WriteRead(ULONG			iIndex,       // Specify the device number
								   ULONG			iChipSelect,  // Selection control, if the film selection control bit 7 is 0, ignore the film selection control bit 7 is 1 and operate the film selection
								   ULONG			iLength,      // Number of bytes of data to be transferred
								   PVOID			ioBuffer );   // Points to a buffer that place the data to be written out from DOUT, return the data read in from DIN

//place the data to be written from MOSI, return the data read in from MISO
BOOL	WINAPI	CH347StreamSPI4(ULONG			iIndex,       // Specify the device number
								ULONG			iChipSelect,  // Film selection control, if bit 7 is 0, slice selection control is ignored.If bit 7 is 1, the parameter is valid:Bit 1 bit 0 is 00/01/10. Select D0/D1/D2 pins as low level active chip options respectively
								ULONG			iLength,      // Number of bytes of data to be transferred
								PVOID			ioBuffer );   // Points to a buffer, places data to be written out from DOUT, and returns data to be read in from DIN

/***************JTAG********************/
//JTAG interface initialization, set mode and speed
BOOL	WINAPI	CH347Jtag_INIT(ULONG iIndex,
							   UCHAR iClockRate);      // Communication speed; The value ranges from 0 to 5. A larger value indicates a faster communication speed

//Gets Jtag speed configuration
BOOL    WINAPI  CH347Jtag_GetCfg(ULONG iIndex,         // Specify the device number
							     UCHAR *ClockRate);    // Communication speed; The value ranges from 0 to 5. A larger value indicates a faster communication speed

//Bit band mode JTAG IR/DR data read and write. Suitable for reading and writing small amounts of data. Such as command operation, state machine switching and other control transmission. For batch data transmission, you are advised to use CH347Jtag_WriteRead_Fast
//Command packages are read and written in batches in 4096 bytes
//The state machine: Run-Test->Shift-IR/DR..->Exit IR/DR -> Run-Test
BOOL	WINAPI	CH347Jtag_WriteRead(ULONG			iIndex,           // Specify the device number
									BOOL            IsDR,             // =TRUE: DR data read/write, =FALSE:IR data read/write
									ULONG			iWriteBitLength,  // Write length, the length to be written
									PVOID			iWriteBitBuffer,  // Points to a buffer to place data ready to be written out
									PULONG			oReadBitLength,   // Points to the length unit and returns the length actually read
									PVOID			oReadBitBuffer ); // Points to a buffer large enough to hold the read data

//JTAG IR/DR data batch read and write The IR/DR data is used for multi-byte continuous read and write. For example, download firmware. The hardware has four buffers. If the buffer is written before the buffer is read, the length cannot exceed 4096 bytes. Buffer size can be adjusted
//The state machine: Run-Test->Shift-IR/DR..->Exit IR/DR -> Run-Test
BOOL	WINAPI	CH347Jtag_WriteRead_Fast(ULONG		iIndex,            // Specify the device number
										 BOOL       IsDR,              // =TRUE: DRdata read/write, =FALSE:IR data read/write
										 ULONG		iWriteBitLength,   // Write length. The length to be written
										 PVOID		iWriteBitBuffer,   // Points to a buffer to place data ready to be written out
										 PULONG		oReadBitLength,    // Point to the length unit and return the actual read length
										 PVOID		oReadBitBuffer );  // Points to a buffer large enough to hold the read data

//Switch the JTAG state machine
BOOL WINAPI CH347Jtag_SwitchTapState(UCHAR TapState);

//JTAG DR Write, in bytes, for multi-byte sequential reads and writes. For example, download firmware.
//The state machine: Run-Test->Shift-DR..->Exit DR -> Run-Test
BOOL	WINAPI	CH347Jtag_ByteWriteDR(ULONG			iIndex,        // Specify the device number								
									  ULONG			iWriteLength,  // Write length, length of bytes to be written
									  PVOID			iWriteBuffer); // Points to a buffer to place data ready to be written out	

//JTAG DR Read, read multiple bytes consecutively in bytes.
//The state machine: Run-Test->Shift-DR..->Exit DR -> Run-Test
BOOL	WINAPI	CH347Jtag_ByteReadDR(ULONG			iIndex,        // Specify the device number							
									 PULONG			oReadLength,   // Points to the length unit and returns the length of the bytes actually read
									 PVOID			oReadBuffer ); // Points to a buffer large enough to hold the read data

//JTAG IR write, written in bytes, multiple bytes are written consecutively.
//The state machine: Run-Test->Shift-IR..->Exit IR -> Run-Test
BOOL	WINAPI	CH347Jtag_ByteWriteIR(ULONG			iIndex,        // Specify the CH347 device number						
									  ULONG			iWriteLength,  // Write length, the length of bytes to be written
									  PVOID			iWriteBuffer); // Points to a buffer to place data ready to be written out

//JTAG IR read, readen in bytes, multiple bytes are readen consecutively.
//The state machine: Run-Test->Shift-IR..->Exit IR -> Run-Test
BOOL	WINAPI	CH347Jtag_ByteReadIR(ULONG			iIndex,         // Specify the device number									
									 PULONG			oReadLength,    // Points to the length unit and returns the length of the bytes actually read
									 PVOID			oReadBuffer );  // Points to a buffer large enough to hold the read data

//Bit band mode JTAG DR data write. Suitable for reading and writing small amounts of data. Such as command operation, state machine switching and other control transmission. For batch data transmission, CH347Jtag_ByteWriteDR is recommended
//The state machine: Run-Test->Shift-DR..->Exit DR -> Run-Test
BOOL	WINAPI	CH347Jtag_BitWriteDR(ULONG			iIndex,             // Specify the device number									
									 ULONG    	    iWriteBitLength,    // Points to the length unit and returns the length of the bytes actually read
									 PVOID			iWriteBitBuffer );  // Points to a buffer large enough to hold the read data

//Bit band mode JTAG IR data write. Suitable for reading and writing small amounts of data. Such as command operation, state machine switching and other control transmission. For batch data transmission, CH347Jtag_ByteWriteIR is recommended
//The state machine: Run-Test->Shift-IR..->Exit IR -> Run-Test
BOOL	WINAPI	CH347Jtag_BitWriteIR(ULONG			iIndex,             // Specify the device number									
									 ULONG    	    iWriteBitLength,    // Points to the length unit and returns the length of the bytes actually read
									 PVOID			iWriteBitBuffer );  // Points to a buffer large enough to hold the read data

//Bit band mode JTAG IR data read. Suitable for reading and writing small amounts of data. Such as command operation, state machine switching, etc. For batch data transmission, CH347Jtag_ByteReadIR is recommended.
//The state machine: Run-Test->Shift-IR..->Exit IR -> Run-Test
BOOL	WINAPI	CH347Jtag_BitReadIR(ULONG			iIndex,            // Specify the device number									
									PULONG    	oReadBitLength,        // Points to the length unit and returns the length of the bytes actually read
									PVOID			oReadBitBuffer );  // Points to a buffer large enough to hold the read data

//Bit band mode JTAG DR data read. Suitable for reading and writing small amounts of data. For batch and high-speed data transmission, CH347Jtag_ByteReadDR is recommended
//The state machine: Run-Test->Shift-DR..->Exit DR -> Run-Test
BOOL	WINAPI	CH347Jtag_BitReadDR(ULONG			iIndex,            // Specify the device number									
									PULONG    	oReadBitLength,        // Points to the length unit and returns the length of the bytes actually read
									PVOID			oReadBitBuffer );  // Points to a buffer large enough to hold the read data

//Get the GPIO direction and pin level of CH347
BOOL WINAPI CH347GPIO_Get(ULONG iIndex,
						  UCHAR *iDir,       // Pin direction: GPIo0-7 corresponding bit 0-7,0: input; 1: output
						  UCHAR *iData);     // GPIO0 level: GPIO0-7 corresponding bit 0-7,0: low level; 1: high level


//Set the GPIO direction and pin level of CH347
BOOL WINAPI CH347GPIO_Set(ULONG iIndex,
						  UCHAR iEnable,        // Data validity flag: The corresponding bits 0-7 correspond to GPIO0-7.
						  UCHAR iSetDirOut,     // Sets the I/O direction, with pin 0 corresponding to input and pin 1 corresponding to output. Gpio0-7 corresponds to bits 0-7.
						  UCHAR iSetDataOut);   // Outputs data. If the I/O direction is output, then a pin outputs low level at a clear 0 and high level at a position 1

//Enter the IAP firmware upgrade mode
BOOL	WINAPI	CH347StartIapFwUpate(ULONG    iIndex,
									 ULONG	  FwSize); // The length of the firmware

/**************HID/VCP Serial Port*********************/
//Open serial port
HANDLE WINAPI CH347Uart_Open(ULONG iIndex);

//Close serial port
BOOL WINAPI CH347Uart_Close(ULONG iIndex);

//Set the device event notification program
BOOL	WINAPI	CH347Uart_SetDeviceNotify(  
										  ULONG					iIndex,                // Specify the device number, 0 corresponds to the first device
										  PCHAR					iDeviceID,             // Optional parameter,points to a string specifying the ID of the device to be monitored, terminated with \0
										  mPCH347_NOTIFY_ROUTINE	iNotifyRoutine );  // Specifies the device event callback. NULL cancels event notification, otherwise it will be called when an event is detected

//Obtain UART hardware configuration
BOOL    WINAPI  CH347Uart_GetCfg(ULONG   iIndex,       // Specify the device number
								 PULONG  BaudRate,     // Baud rate
								 PUCHAR  ByteSize,     // Data bits (5,6,7,8,16)
								 PUCHAR  Parity,       // Parity bits(0:None; 1:Odd; 2:Even; 3:Mark; 4:Space)
								 PUCHAR  StopBits,     // Stop bits (0:1 stop bits; 1:1.5 stop bit; 2:2 stop bit);
								 PUCHAR  ByteTimeout); // Byte timeout

//Set UART configuration
BOOL  WINAPI   CH347Uart_Init(ULONG  iIndex,         // Specify the device number
								 DWORD  BaudRate,    // Baud rate 
								 UCHAR  ByteSize,    // Data bits (5,6,7,8,16)
								 UCHAR  Parity,      // Check bit (0:None; 1:Odd; 2:Even; 3:Mark; 4:Space)
								 UCHAR  StopBits,    // Stop bits  0:1 Stop bit; 1:1.5 stop bit; 2:2 stop bit);
								 UCHAR  ByteTimeout);// Byte Timeout duration, in unit of 100uS

//Set the timeout of USB data read and write
BOOL	WINAPI	CH347Uart_SetTimeout(ULONG			iIndex,         // Specify the device number
									 ULONG			iWriteTimeout,  // Specifies the timeout for USB to write data blocks, in milliseconds mS,0xFFFFFFFF specifies no timeout (default)
									 ULONG			iReadTimeout ); // Specifies the timeout for USB to read data blocks, in milliseconds mS,0xFFFFFFFF specifies no timeout (default)

//Read data block
BOOL  WINAPI   CH347Uart_Read(ULONG			iIndex,    // Specify the device number
							  PVOID			oBuffer,   // Points to a buffer large enough to hold the read data
							  PULONG		ioLength );// Refers to the length unit. The input is the length to be read and the return is the actual length to be read

//Write out blocks of data
BOOL  WINAPI   CH347Uart_Write(ULONG		iIndex,    // Specify the device number
							   PVOID		iBuffer,   // Points to a buffer to place data ready to be written out 
							   PULONG		ioLength );// Point to the length unit. The input is the intended length and the return is the actual length

//Query how many bytes are unfetched in the read buffer
BOOL WINAPI CH347Uart_QueryBufUpload(ULONG			iIndex,    // Specify the device number
									 LONGLONG       *RemainBytes);

//Obtaining Device Information
BOOL WINAPI CH347Uart_GetDeviceInfor(ULONG iIndex,mDeviceInforS *DevInformation);

/********IIC***********/
//Set the serial port flow mode
BOOL	WINAPI	CH347I2C_Set(ULONG			iIndex,   // Specify the device number
							 ULONG			iMode );  // See downlink for the specified mode 
//bit 1-bit 0: I2C interface speed /SCL frequency, 00= low speed /20KHz,01= standard /100KHz(default),10= fast /400KHz,11= high speed /750KHz
//Other reservations, must be 0

//Set the hardware asynchronous delay to a specified number of milliseconds before the next stream operation
BOOL	WINAPI	CH347I2C_SetDelaymS(ULONG			iIndex,        // Specify the device number
									ULONG			iDelay ) ;     // Specifies the delay duration (mS)

//Process I2C data stream, 2-wire interface, clock line for SCL pin, data line for SDA pin
BOOL	WINAPI	CH347StreamI2C( ULONG			iIndex,        // Specify the device number
							   ULONG			iWriteLength,  // The number of bytes of data to write
							   PVOID			iWriteBuffer,  // Points to a buffer to place data ready to be written out, the first byte is usually the I2C device address and read/write direction bit
							   ULONG			iReadLength,   // Number of bytes of data to be read
							   PVOID			oReadBuffer ); // Points to a buffer to place data ready to be read in
#ifndef _CH341_DLL_H
typedef	enum	_EEPROM_TYPE {// EEPROM type
	ID_24C01,
	ID_24C02,
	ID_24C04,
	ID_24C08,
	ID_24C16,
	ID_24C32,
	ID_24C64,
	ID_24C128,
	ID_24C256,
	ID_24C512,
	ID_24C1024,
	ID_24C2048,
	ID_24C4096
} EEPROM_TYPE;
#endif

// Reads data blocks from EEPROM at a speed of about 56 KB
BOOL	WINAPI	CH347ReadEEPROM(ULONG			iIndex,     // Specify the device number
								EEPROM_TYPE		iEepromID,  // Specifies the EEPROM model
								ULONG			iAddr,      // Specifies the address of data unit
								ULONG			iLength,    // Number of bytes of data to be read
								PUCHAR			oBuffer );  // Points to a buffer to place data ready to be read in

// Writes a data block to the EEPROM
BOOL	WINAPI	CH347WriteEEPROM(ULONG			iIndex,     // Specify the device number
								 EEPROM_TYPE	iEepromID,  // Specifies the EEPROM model
								 ULONG			iAddr,      // Specifies the address of data unit
								 ULONG			iLength,    // Number of bytes of data to be written out
								 PUCHAR			iBuffer );  // Points to a buffer to place data ready to be written out

#ifdef __cplusplus
}
#endif

#endif		// _CH347_DLL_H
