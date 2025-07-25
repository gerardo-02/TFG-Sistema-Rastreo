#ifndef VENDOR_DRIVER_H
#define	VENDOR_DRIVER_H

#include "usb/usb_host_client_driver.h"

#ifdef	__cplusplus
extern "C" {
#endif

    void APP_VENDOR_DRIVER_Initialize(void);
    void APP_VENDOR_DRIVER_Tasks(void);
    void APP_VENDOR_DRIVER_DeviceAssign(USB_HOST_DEVICE_CLIENT_HANDLE deviceHandle, USB_HOST_DEVICE_OBJ_HANDLE deviceObjHandle, USB_DEVICE_DESCRIPTOR * deviceDescriptor);
    void APP_VENDOR_DRIVER_DeviceRelease(USB_HOST_DEVICE_CLIENT_HANDLE deviceHandle);
    USB_HOST_DEVICE_EVENT_RESPONSE APP_VENDOR_DRIVER_DeviceEventHandler(USB_HOST_DEVICE_CLIENT_HANDLE deviceHandle, USB_HOST_DEVICE_EVENT event, void * eventData, uintptr_t context);
    void APP_VENDOR_DRIVER_InterfaceAssign(USB_HOST_DEVICE_INTERFACE_HANDLE * interfaces, USB_HOST_DEVICE_OBJ_HANDLE deviceObjHandle, size_t nInterfaces, uint8_t * descriptor);
    void APP_VENDOR_DRIVER_InterfaceRelease(USB_HOST_DEVICE_INTERFACE_HANDLE interfaceHandle);
    USB_HOST_DEVICE_INTERFACE_EVENT_RESPONSE APP_VENDOR_DRIVER_InterfaceEventHandler(USB_HOST_DEVICE_INTERFACE_HANDLE interfaceHandle, USB_HOST_DEVICE_INTERFACE_EVENT event, void * eventData, uintptr_t context);
    //    bool APP_VENDOR_DRIVER_LedToggleCommandInProgress(void);
    bool APP_VENDOR_DRIVER_DeviceIsAvailable(void);
    //    bool APP_VENDOR_DRIVER_ToggleLED(void);
    bool APP_VENDOR_DRIVER_DeviceSwitchStatusGet(uint8_t * appBuffer);
    bool APP_VENDOR_DRIVER_DeviceSwitchStatusGetInProgress(void);

#ifdef	__cplusplus
}
#endif

#endif	/* VENDOR_DRIVER_H */

