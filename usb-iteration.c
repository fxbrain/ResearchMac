//
// Created by Stefan Schwarz on 11/11/2016.
//

#include <stdint.h>
#include <IOKit/usb/IOUSBLib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOReturn.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/USBSpec.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <sys/param.h>

#define RESET   "\033[0m"
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */

enum USBDeviceErrorCode
{
    kSuccess                        = 0,        // description...
    kFailure                        = 1 << 0,   // description...
    kDeviceNotOnline                = 1 << 1,   // description...
    kActiveConfigFailed             = 1 << 2,   // description...
    kDeviceInUse                    = 1 << 3,   // description...
};


IOReturn getDeviceManufacturer(IOUSBDeviceInterface650** usbDevice,
        CFStringRef* manufString)
{
    UInt8 stringIndex;
    IOUSBDevRequest devRequest;
    UInt8 buffer[256];
    IOReturn error;

    // Get the index in the string table for the manufacturer
    error = (*usbDevice)->USBGetManufacturerStringIndex(usbDevice, &stringIndex);
    if (error!=kIOReturnSuccess) {
        return error;
    }

    // Perform a device request to read the string descriptor
    devRequest.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBStandard, kUSBDevice);
    devRequest.bRequest = kUSBRqGetDescriptor;
    devRequest.wValue = (kUSBStringDesc << 8) | stringIndex;
    devRequest.wIndex = 0x409;        // Language setting - specify US English
    devRequest.wLength = sizeof(buffer);
    devRequest.pData = &buffer[0];
    bzero(&buffer[0], sizeof(buffer));
    //
    error = (*usbDevice)->DeviceRequest(usbDevice, &devRequest);
    if (error!=kIOReturnSuccess) {
        return error;
    }

    // Create a CFString representation of the returned data
    int strLength;
    strLength = buffer[0]-2;        // First byte is length (in bytes)
    (*manufString) = CFStringCreateWithBytes(kCFAllocatorDefault, &buffer[2], strLength,
            kCFStringEncodingUTF16LE, false);
    return error;
}

/**
 Fetches the product name of the usbDevice

 @param usbDevice the actual usb device
 @param prodString the returned name of the device
 @return the result code of the kernel operation
 */
IOReturn getProductName(IOUSBDeviceInterface650** usbDevice, CFStringRef* prodString)
{
    UInt8 stringIndex;
    IOUSBDevRequest devRequest;
    UInt8 buffer[256];
    IOReturn error;

    // Get the index in the string table for the manufacturer
    error = (*usbDevice)->USBGetProductStringIndex(usbDevice, &stringIndex);
    if (error!=kIOReturnSuccess) {
        return error;
    }

    // Perform a device request to read the string descriptor
    devRequest.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBStandard, kUSBDevice);
    devRequest.bRequest = kUSBRqGetDescriptor;
    devRequest.wValue = (kUSBStringDesc << 8) | stringIndex;
    devRequest.wIndex = 0x409;        // Language setting - specify US English
    devRequest.wLength = sizeof(buffer);
    devRequest.pData = &buffer[0];
    bzero(&buffer[0], sizeof(buffer));
    //
    error = (*usbDevice)->DeviceRequest(usbDevice, &devRequest);
    if (error!=kIOReturnSuccess) {
        return error;
    }

    // Create a CFString representation of the returned data
    int strLength;
    strLength = buffer[0]-2;        // First byte is length (in bytes)
    (*prodString) = CFStringCreateWithBytes(kCFAllocatorDefault, &buffer[2],
            strLength, kCFStringEncodingUTF16LE,
            false);
    return error;
}

/**
 Returns the serial number as a CFString.
 It is the caller's responsibility to release the returned CFString when done with it.

 @param serialNumber OUT: the serial number
 */
void HostSerialNumber(CFStringRef* serialNumber)
{
    if (serialNumber!=NULL) {
        *serialNumber = NULL;

        io_service_t platformExpert =
                IOServiceGetMatchingService(kIOMasterPortDefault,
                        IOServiceMatching("IOPlatformExpertDevice"));

        if (platformExpert) {
            CFTypeRef serialNumberAsCFString =
                    IORegistryEntryCreateCFProperty(platformExpert,
                            CFSTR(kIOPlatformSerialNumberKey),
                            kCFAllocatorDefault, 0);
            if (serialNumberAsCFString) {
                *serialNumber = serialNumberAsCFString;
            }

            IOObjectRelease(platformExpert);
        }
    }
}

const char* CFStringCopyUTF8String(CFStringRef aString)
{
    if (aString==NULL) {
        return NULL;
    }

    CFIndex length = CFStringGetLength(aString);
    CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8)+1;
    char* buffer = (char*) malloc((size_t) maxSize);

    if (CFStringGetCString(aString, buffer, maxSize, kCFStringEncodingUTF8)) {
        return buffer;
    }

    return NULL;
}

/**
 Investigates the actual device file for the given
 Vendor/Product identifiers

 @param idVendor the vendor id
 @param idProduct the product id
 @param deviceFilePath the actual file path of the device file in /dev
 @return the result code of the operation
 */
enum USBDeviceErrorCode getDeviceFilePath(SInt32 idVendor, SInt32 idProduct,
        char* deviceFilePath)
{
    CFMutableDictionaryRef matchingDictionary = NULL;
    io_iterator_t iterator = 0;
    io_service_t usbRef;
    SInt32 score;
    IOCFPlugInInterface** plugin;
    IOUSBDeviceInterface650** usbDevice = NULL;
    IOUSBFindInterfaceRequest interfaceRequest;

    matchingDictionary = IOServiceMatching(kIOUSBDeviceClassName);
    CFDictionaryAddValue(matchingDictionary, CFSTR(kUSBVendorID),
            CFNumberCreate(kCFAllocatorDefault,
                    kCFNumberSInt32Type, &idVendor));
    CFDictionaryAddValue(matchingDictionary, CFSTR(kUSBProductID),
            CFNumberCreate(kCFAllocatorDefault,
                    kCFNumberSInt32Type, &idProduct));
    IOServiceGetMatchingServices(kIOMasterPortDefault,
            matchingDictionary, &iterator);
    usbRef = IOIteratorNext(iterator);

    if (usbRef==0) {
        fprintf(stderr, "Device is not online.\n");
        return kDeviceNotOnline;
    }

    IOObjectRelease(iterator);
    IOCreatePlugInInterfaceForService(usbRef, kIOUSBDeviceUserClientTypeID,
            kIOCFPlugInInterfaceID, &plugin, &score);
    IOObjectRelease(usbRef);
    (*plugin)->QueryInterface(plugin, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID300),
            (LPVOID) &usbDevice);

    (*plugin)->Release(plugin);

    interfaceRequest.bInterfaceClass = kIOUSBFindInterfaceDontCare;
    interfaceRequest.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
    interfaceRequest.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
    interfaceRequest.bAlternateSetting = kIOUSBFindInterfaceDontCare;

    (*usbDevice)->CreateInterfaceIterator(usbDevice, &interfaceRequest, &iterator);

    IOIteratorNext(iterator); // skip interface #0
    usbRef = IOIteratorNext(iterator);
    IOObjectRelease(iterator);

    CFStringRef deviceBSDName_cf =
            (CFStringRef) IORegistryEntrySearchCFProperty(usbRef,
                    kIOServicePlane,
                    CFSTR (kIOCalloutDeviceKey),
                    kCFAllocatorDefault,
                    kIORegistryIterateRecursively);
    if (deviceBSDName_cf) {
        Boolean result;

        // Convert the path from a CFString to a NULL-terminated C string
        // for use with the POSIX open() call.

        result = CFStringGetCString(deviceBSDName_cf,
                deviceFilePath,
                1024,
                kCFStringEncodingASCII);
        CFRelease(deviceBSDName_cf);

        if (result) {
            return kSuccess;
        }
    }
    return kFailure;
}

/**
 Checks if the specified device through Vendor/Product pair
 is online and usable.

 @param idVendor the vendor id
 @param idProduct the product id
 @return the result code of the operation
 */
enum USBDeviceErrorCode checkDeviceState(SInt32 idVendor, SInt32 idProduct)
{
    CFMutableDictionaryRef matchingDictionary = NULL;
    io_iterator_t iterator = 0;
    io_service_t usbRef;
    SInt32 score;
    IOCFPlugInInterface** plugin;
    //    IOUSBDeviceInterface300** usbDevice = NULL;
    IOUSBDeviceInterface650** usbDevice = NULL;
    IOReturn ret;
    IOUSBConfigurationDescriptorPtr config;
    IOUSBFindInterfaceRequest interfaceRequest;
    IOUSBInterfaceInterface300** usbInterface;
    CFStringRef manufString, prodString;
    CFStringRef serialNumberHost;
    //    cuPrivateData   *privateDataRef = NULL;
    UInt32 locationID, bandwith;
    UInt16 vendor, product;

    HostSerialNumber(&serialNumberHost);
    printf("Host-Serial-Number is %s\n", CFStringCopyUTF8String(serialNumberHost));
    CFRelease(serialNumberHost);

    /* try to find USB device using the vendor and product id */
    matchingDictionary = IOServiceMatching(kIOUSBDeviceClassName);
    CFDictionaryAddValue(matchingDictionary, CFSTR(kUSBVendorID),
            CFNumberCreate(kCFAllocatorDefault,
                    kCFNumberSInt32Type, &idVendor));
    CFDictionaryAddValue(matchingDictionary, CFSTR(kUSBProductID),
            CFNumberCreate(kCFAllocatorDefault,
                    kCFNumberSInt32Type, &idProduct));
    IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDictionary, &iterator);
    usbRef = IOIteratorNext(iterator);

    if (usbRef==0) {
        fprintf(stderr, "Device is not online.\n");
        return kDeviceNotOnline;
    }
    IOObjectRelease(iterator);
    IOCreatePlugInInterfaceForService(usbRef, kIOUSBDeviceUserClientTypeID,

            kIOCFPlugInInterfaceID, &plugin, &score);
    IOObjectRelease(usbRef);
    (*plugin)->QueryInterface(plugin, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID300),
            (LPVOID) &usbDevice);

    (*plugin)->Release(plugin);

    // The locationID uniquely identifies the device
    // and will remain the same, even across reboots, so long as the bus
    // topology doesn't change.
    (*usbDevice)->GetLocationID(usbDevice, &locationID);
    (*usbDevice)->GetDeviceVendor(usbDevice, &vendor);
    (*usbDevice)->GetDeviceProduct(usbDevice, &product);
    (*usbDevice)->GetBandwidthAvailableForDevice(usbDevice, &bandwith);

    fprintf(stdout, "LocationID is : 0x%x\n", locationID);
    fprintf(stdout, "VendorID is : 0x%x\n", vendor);
    fprintf(stdout, "ProductID is : 0x%x\n", product);

    getDeviceManufacturer(usbDevice, &manufString);
    getProductName(usbDevice, &prodString);

    /*
     * now that we have found the device, we open the device and set the
     * first configuration as active
     */

    ret = (*usbDevice)->USBDeviceOpen(usbDevice);
    if (ret==kIOReturnSuccess) {
        // set first configuration as active
        ret = (*usbDevice)->GetConfigurationDescriptorPtr(usbDevice, 0, &config);
        if (ret!=kIOReturnSuccess) {
            printf("Could not set active configuration (error: %x)\n", ret);
            return kActiveConfigFailed;
        }
        (*usbDevice)->SetConfiguration(usbDevice, config->bConfigurationValue);
    }
    interfaceRequest.bInterfaceClass = kIOUSBFindInterfaceDontCare;
    interfaceRequest.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
    interfaceRequest.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
    interfaceRequest.bAlternateSetting = kIOUSBFindInterfaceDontCare;

    (*usbDevice)->CreateInterfaceIterator(usbDevice, &interfaceRequest, &iterator);

    IOIteratorNext(iterator); // skip interface #0
    usbRef = IOIteratorNext(iterator);
    IOObjectRelease(iterator);

    IOCreatePlugInInterfaceForService(usbRef, kIOUSBInterfaceUserClientTypeID,
            kIOCFPlugInInterfaceID, &plugin, &score);
    IOObjectRelease(usbRef);

    (*plugin)->QueryInterface(
            plugin,
            CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID300),
            (LPVOID) &usbInterface
    );
    (*plugin)->Release(plugin);

    if (usbInterface==NULL) {
        printf("Device " BOLDMAGENTA "%s : %s" RESET  \
               "cannot be opened. [ " RED "ERROR" RESET " ]\n",
                CFStringCopyUTF8String(manufString),
                CFStringCopyUTF8String(prodString));
        CFRelease(manufString);
        CFRelease(prodString);
        return kDeviceInUse;
    }
    ret = (*usbInterface)->USBInterfaceOpen(usbInterface);
    if (ret!=kIOReturnSuccess) {
        if (ret==kIOReturnExclusiveAccess) {
            printf("Device " BOLDMAGENTA "%s : %s" RESET \
                   ", is already in use. [ " RED "ERROR" RESET " ]\n",
                    CFStringCopyUTF8String(manufString),
                    CFStringCopyUTF8String(prodString));
            CFRelease(manufString);
            CFRelease(prodString);
            return kDeviceInUse;
        }
    }

    printf("Device " BOLDMAGENTA "%s : %s" RESET \
           ", is ready to use. [ " GREEN "OK" RESET " ]\n",
            CFStringCopyUTF8String(manufString),
            CFStringCopyUTF8String(prodString));
    CFRelease(manufString);
    CFRelease(prodString);

    (*usbInterface)->USBInterfaceClose(usbInterface);
    (*usbDevice)->USBDeviceClose(usbDevice);

    return kSuccess;
}

/**
 * Lists the available usb devices registered inside the system.
 * @return the result code of the operation
 */
enum USBDeviceErrorCode listDevices()
{
    CFMutableDictionaryRef matchingDictionary = NULL;
    IOCFPlugInInterface** plugin;
    IOUSBDeviceInterface650** usbDevice = NULL;
    SInt32 score;
    io_iterator_t iter = 0;
    io_service_t device;
    kern_return_t kr;
    UInt16 vendor, product;

    matchingDictionary = IOServiceMatching(kIOUSBDeviceClassName);
    if (matchingDictionary==NULL) {
        return kFailure;
    }

    kr = IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDictionary, &iter);
    if (kr!=KERN_SUCCESS) {
        return kFailure;
    }

    while ((device = IOIteratorNext(iter))) {
        IOCreatePlugInInterfaceForService(device, kIOUSBDeviceUserClientTypeID,
                kIOCFPlugInInterfaceID, &plugin, &score);
        (*plugin)->QueryInterface(plugin,
                CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID300),
                (LPVOID) &usbDevice);
        (*plugin)->Release(plugin);

        (*usbDevice)->GetDeviceVendor(usbDevice, &vendor);
        (*usbDevice)->GetDeviceProduct(usbDevice, &product);

        fprintf(stdout, "0x%04x : 0x%04x\n", vendor, product);
        fprintf(stdout, "\n\n");
        IOObjectRelease(device);
    }
    return kSuccess;
}


int main(int argc, char **argv)
{
    char path[MAXPATHLEN];
    listDevices();
    checkDeviceState(0x05ac, 0x8006);
    getDeviceFilePath(0x05ac, 0x8006, path);
    fprintf(stdout, "Device Path : %s\n", path);
    return EXIT_SUCCESS;
}
