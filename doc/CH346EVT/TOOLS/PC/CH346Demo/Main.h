// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <mmsystem.h>
#pragma comment(lib,"winmm")
#include <time.h>

#include <windows.h>
#include <stdio.h>
//#include <initguid.h>
#include <devguid.h>
#include <regstr.h>
#include <setupapi.h>
#pragma comment(lib,"setupapi")
#pragma comment(lib,"comctl32.lib")
//#include "cfgmgr32.h"
//#pragma comment(lib,"cfgmgr32.LIB")

#include "DbgFunc.h"
#include "resource.h"
#include <stdio.h>
#include <initguid.h>

//#define CH375_IF

#ifdef CH375_IF
#include "ExternalLib\\CH375DLL.H"
#pragma comment(lib,"ExternalLib\\i386\\CH375DLL")
#else
#include "ExternalLib\\CH346DLL.H"
#pragma comment(lib,"ExternalLib\\i386\\CH346DLL")
#endif


typedef struct _USB_DEVICE_DESCRIPTOR {
    UCHAR bLength;
    UCHAR bDescriptorType;
    USHORT bcdUSB;
    UCHAR bDeviceClass;
    UCHAR bDeviceSubClass;
    UCHAR bDeviceProtocol;
    UCHAR bMaxPacketSize0;
    USHORT idVendor;
    USHORT idProduct;
    USHORT bcdDevice;
    UCHAR iManufacturer;
    UCHAR iProduct;
    UCHAR iSerialNumber;
    UCHAR bNumConfigurations;
} USB_DEVICE_DESCRIPTOR, *PUSB_DEVICE_DESCRIPTOR;
#pragma pack() 


