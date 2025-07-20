/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    usb_simcom.h

  @Summary
    Brief description of the file.

  @Description
    Describe the purpose of this file.
 */
/* ************************************************************************** */

#ifndef _usb_simcom_H    /* Guard against multiple inclusion */
#define _usb_simcom_H


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

#include"config/default/usb/usb_host.h"
#include "usb/usb_host_client_driver.h"
#include <stdbool.h>

/* TODO:  Include other files here if needed. */
//#include "config/default/usb/usb_cdc.h"

/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */

typedef uintptr_t USB_HOST_SIMCOM_HANDLE;

typedef uintptr_t USB_HOST_SIMCOM_TRANSFER_HANDLE;

#define USB_HOST_SIMCOM_HANDLE_INVALID ((USB_HOST_CDC_HANDLE)(-1))

#define MAX_NUM_DEVICES 5

// RNDIS IAD Interfaces codes
#define USB_RNDIS_CLASS_CODE    0xE0
#define USB_RNDIS_SUBCLASS_CODE 0x01
#define USB_RNDIS_PROTOCOL      0x03

#define USB_CDC_DATA_CLASS_CODE     0x0AU

#define USB_VENDOR_SPECIFIC_CLASS_CODE  0xFF

/*DOM-IGNORE-BEGIN*/#define USB_HOST_SIMCOM_RESULT_MIN -100/*DOM-IGNORE-END*/

// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************

typedef enum{
    // Un fallo desconocido ha ocurrido
    USB_HOST_SIMCOM_RESULT_FAILURE /*DOM-IGNORE-BEGIN*/ = USB_HOST_SIMCOM_RESULT_MIN /*DOM-IGNORE-END*/,
    USB_HOST_SIMCOM_RESULT_REQUEST_STALLED,
    USB_HOST_SIMCOM_RESULT_INVALID_PARAMETER,
    USB_HOST_SIMCOM_RESULT_BUSY,
    USB_HOST_SIMCOM_RESULT_ABORTED,
    USB_HOST_SIMCOM_RESULT_HANDLE_INVALID,
    USB_HOST_SIMCOM_RESULT_SUCCESS = 1
}USB_HOST_SIMCOM_RESULT;

/** USB_HOST_STATE

  @Summary
 Estado del host usb

  @Description
 * Estado del host usb
 */

typedef enum{
    USB_HOST_STATE_ERROR = -1,
    USB_HOST_STATE_INIT = 0,
    USB_HOST_STATE_ENABLING_BUS,
    USB_HOST_STATE_GETTING_DEVICES,
    USB_HOST_STATE_READY
} USB_HOST_STATE;

/** USB_HOST_SIMCOM_STATE

  @Summary
 Estado del driver que opera el módulo SIMCOM

  @Description
 * Estado del driver que opera el módulo SIMCOM
 */

typedef enum{

    USB_HOST_SIMCOM_STATE_ERROR = -1,
    USB_HOST_SIMCOM_STATE_NOT_READY = 0,
    USB_HOST_SIMCOM_STATE_SET_CONFIGURATION,
    USB_HOST_SIMCOM_STATE_WAIT_FOR_CONFIGURATION_SET,
    USB_HOST_SIMCOM_STATE_WAIT_FOR_INTERFACES,
    USB_HOST_SIMCOM_STATE_READY

} USB_HOST_SIMCOM_STATE;

typedef struct
{
    /* Transfer handle of this transfer */
    USB_HOST_SIMCOM_TRANSFER_HANDLE transferHandle;

    /* Termination transfer status */
    USB_HOST_RESULT result;

    /* Size of the data transferred in the request */
    size_t length;
}USB_HOST_SIMCOM_TRANSFER_COMPLETE_DATA;

typedef enum{
    USB_HOST_SIMCOM_EVENT_AT_READ_COMPLETE,
    USB_HOST_SIMCOM_EVENT_AT_WRITE_COMPLETE,
    USB_HOST_SIMCOM_EVENT_AT_SERIAL_STATE_NOTIFICATION_RECEIVED,
    USB_HOST_SIMCOM_EVENT_RNDIS_READ_COMPLETE,
    USB_HOST_SIMCOM_EVENT_RNDIS_WRITE_COMPLETE,
    USB_HOST_SIMCOM_EVENT_RNDIS_SERIAL_STATE_NOTIFICATION_RECEIVED,
    USB_HOST_SIMCOM_EVENT_ECM_READ_COMPLETE,
    USB_HOST_SIMCOM_EVENT_ECM_WRITE_COMPLETE,
    USB_HOST_SIMCOM_EVENT_ECM_SERIAL_STATE_NOTIFICATION_RECEIVED,
    USB_HOST_SIMCOM_EVENT_DIAG_READ_COMPLETE,
    USB_HOST_SIMCOM_EVENT_DIAG_WRITE_COMPLETE,
    USB_HOST_SIMCOM_EVENT_DEVICE_DETACHED
}USB_HOST_SIMCOM_EVENT;

typedef enum
{
    /* This means no response is required */
    USB_HOST_SIMCOM_EVENT_RESPONE_NONE   /*DOM-IGNORE-BEGIN*/= 0 /*DOM-IGNORE-END*/

} USB_HOST_SIMCOM_EVENT_RESPONSE;

typedef USB_HOST_SIMCOM_EVENT_RESPONSE (* USB_HOST_SIMCOM_EVENT_HANDLER)
(
    USB_HOST_SIMCOM_EVENT event,
    void * eventData,
    uintptr_t context
);

typedef struct{
    USB_HOST_DEVICE_INFO info[MAX_NUM_DEVICES];
    USB_HOST_STATE state;
    uint8_t n_devices;

}USB_HOST_BUS_DATA;

/*Mobile RNDIS Network Adapter*/
typedef struct{
    
    /* RNDIS modem comm Interface Handle */
    USB_HOST_DEVICE_INTERFACE_HANDLE commInterfaceHandle;   // Interfaz 0
    
    /* RNDIS modem data Interface Handle */
    USB_HOST_DEVICE_INTERFACE_HANDLE dataInterfaceHandle;   // Interfaz 1
    
    /* Interrupt pipe handle for Comunication Interface */
    USB_HOST_PIPE_HANDLE interruptPipeHandle;
    
    /* Bulk pipe handle for Data Interface */
    USB_HOST_PIPE_HANDLE bulkInPipeHandle;
    USB_HOST_PIPE_HANDLE bulkOutPipeHandle;
    
    uint8_t commInterfaceNumber;
    uint8_t dataInterfaceNumber;
    
}SIMCOM_IAD_RNDIS;

/*Mobile AT Interface*/
typedef struct{
    /* AT port Interface Handle */
    USB_HOST_DEVICE_INTERFACE_HANDLE interfaceHandle;       // Interfaz 4

    /* Bulk pipe handle for Data Interface */
    USB_HOST_PIPE_HANDLE bulkInPipeHandle;
    USB_HOST_PIPE_HANDLE bulkOutPipeHandle;

    /* Interrupt pipe handle for Comunication Interface */
    USB_HOST_PIPE_HANDLE interruptPipeHandle;
    
    uint8_t interfaceNumber;
}SIMCOM_INTERFACE_AT;

typedef struct{
    /* Diagnostics Interface Handle */
    USB_HOST_DEVICE_INTERFACE_HANDLE interfaceHandle;       // Interfaz 2

    /* Bulk pipe handle for Data Interface */
    USB_HOST_PIPE_HANDLE bulkInPipeHandle;
    USB_HOST_PIPE_HANDLE bulkOutPipeHandle;

    uint8_t interfaceNumber;
}SIMCOM_INTERFACE_DIAGNOSTICS;

typedef struct{
    /* ECM modem Interface Handle */
    USB_HOST_DEVICE_INTERFACE_HANDLE interfaceHandle;       // Interfaz 5

    /* Bulk pipe handle for Data Interface */
    USB_HOST_PIPE_HANDLE bulkInPipeHandle;
    USB_HOST_PIPE_HANDLE bulkOutPipeHandle;

    /* Interrupt pipe handle for Comunication Interface */
    USB_HOST_PIPE_HANDLE interruptPipeHandle;
    
    uint8_t interfaceNumber;
}SIMCOM_INTERFACE_ECM_MODEM;

typedef struct{

    /* Device client handle */
    USB_HOST_DEVICE_CLIENT_HANDLE deviceClientHandle;

    /* Device object handle */
    USB_HOST_DEVICE_OBJ_HANDLE deviceObjHandle;

    SIMCOM_IAD_RNDIS RNDISInterface;

    SIMCOM_INTERFACE_AT ATInterface;

    SIMCOM_INTERFACE_DIAGNOSTICS diagInterface;

    SIMCOM_INTERFACE_ECM_MODEM ECMInterface;

    /*Control Pipe Handle */
    USB_HOST_PIPE_HANDLE controlPipeHandle;

    /* True if an ongoing host request is done */
    bool hostRequestDone;

    /* Result of the host request */
    USB_HOST_RESULT hostRequestResult;
    
    /* Setup packet information */
    //USB_SETUP_PACKET    setupPacket;

    /* Control transfer object */
    //USB_HOST_SIMCOM_CONTROL_TRANSFER_OBJ controlTransferObj;

    /* Application defined context */
    uintptr_t context;

    /* Application callback */
    USB_HOST_SIMCOM_EVENT_HANDLER eventHandler;

    /*SIMCOM state*/
    USB_HOST_SIMCOM_STATE state;

} USB_HOST_SIMCOM_INSTANCE_OBJ;

extern USB_HOST_SIMCOM_INSTANCE_OBJ gUSBHostSIMCOMObj;

// *****************************************************************************
// *****************************************************************************
// Section: Interface Functions
// *****************************************************************************
// *****************************************************************************

/* Function:
    USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_EventHandlerSet
    (
        USB_HOST_CDC_EVENT_HANDLER eventHandler,
        uintptr_t context
    );

  Summary:
    Registers an event handler with the SIMCOM Host Client Driver.

  Description:
    This function registers a client specific SIMCOM Host Client Driver event
    handler. The SIMCOM Host Client Driver will call this function with relevant
    event and associated event data, in response to command requests and data
    transfers that have been scheduled by the client.
    
  Precondition:
    None.

  Parameters:

    eventHandler - A pointer to event handler function. If NULL, then events
    will not be generated.
    
    context - Application specific context that is returned in the event handler.
*/
USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_EventHandlerSet
(
    USB_HOST_SIMCOM_EVENT_HANDLER eventHandler,
    uintptr_t context
);

// Funciones para el host genérico

/** 
Function
  void F_USB_HOST_SIMCOM_Initialize(void* data)

Summary:
This function is called when the Host Layer is initializing.

Description:
This function is called when the Host Layer is initializing.
*/
void F_USB_HOST_SIMCOM_Initialize(void* data);

/** 
Function
  void F_USB_HOST_SIMCOM_DeviceTasks(USB_HOST_DEVICE_CLIENT_HANDLE deviceHandle)

Summary: 
This function is called when the host layer want the device to update its
state. 

Description:
This function is called when the host layer want the device to update its
state
*/
void F_USB_HOST_SIMCOM_DeviceTasks(USB_HOST_DEVICE_CLIENT_HANDLE deviceHandle);

/** 
Function
  void F_USB_HOST_SIMCOM_InterfaceTasks(USB_HOST_DEVICE_INTERFACE_HANDLE interfaceHandle)

Summary:
This function is called by the Host Layer to update the state of this
driver.

Description:
This function is called by the Host Layer to update the state of this
driver.
*/
void F_USB_HOST_SIMCOM_InterfaceTasks(USB_HOST_DEVICE_INTERFACE_HANDLE interfaceHandle);

/* Function:
    bool USB_HOST_SIMCOM_IsInReadyState(void);
           
  Summary:
 Devuelve true si el driver está en el estado USB_HOST_SIMCOM_STATE_READY

  Description:
    Devuelve true si el driver está en el estado USB_HOST_SIMCOM_STATE_READY                                                               
*/
bool USB_HOST_SIMCOM_IsInReadyState(void);

/* Function:
    USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_AT_Write(void * data, size_t size);
           
  Summary:
    Función que envía comandos a través de la interfaz AT.

  Description:
    Función que envía comandos a través de la interfaz AT.

  Precondition:
    size > 0.

  Input:

    data - pointer to the buffer containing the data to be written. The
    contents of the buffer should not be changed till the
    USB_HOST_CDC_EVENT_WRITE_COMPLETE event has occurred.

    size - Number of bytes to write.

  Return:
    USB_HOST_SIMCOM_RESULT_SUCCESS - The operation was successful.

    USB_HOST_SIMCOM_RESULT_FAILURE - An unknown failure occurred.
    
  Example:
    <code>
    </code>

  Remarks:
    None.                                                                   
*/
USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_AT_Write
(
    USB_HOST_SIMCOM_TRANSFER_HANDLE * transferHandle,
    void * data,
    size_t size
);

USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_AT_Read
(
    USB_HOST_SIMCOM_TRANSFER_HANDLE * transferHandle,
    void * data,
    size_t size
);

USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_AT_SerialStateNotificationGet
(
    USB_HOST_SIMCOM_TRANSFER_HANDLE * transferHandle,
    void * data,
    size_t size
);

USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_RNDIS_Write
(
    USB_HOST_SIMCOM_TRANSFER_HANDLE * transferHandle,
    void * data,
    size_t size
);

USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_RNDIS_Read
(
    USB_HOST_SIMCOM_TRANSFER_HANDLE * transferHandle,
    void * data,
    size_t size
);

USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_RNDIS_SerialStateNotificationGet
(
    USB_HOST_SIMCOM_TRANSFER_HANDLE * transferHandle,
    void * data,
    size_t size
);

USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_ECM_Write
(
    USB_HOST_SIMCOM_TRANSFER_HANDLE * transferHandle,
    void * data,
    size_t size
);

USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_ECM_Read
(
    USB_HOST_SIMCOM_TRANSFER_HANDLE * transferHandle,
    void * data,
    size_t size
);

USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_ECM_SerialStateNotificationGet
(
    USB_HOST_SIMCOM_TRANSFER_HANDLE * transferHandle,
    void * data,
    size_t size
);

USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_DIAG_Write
(
    USB_HOST_SIMCOM_TRANSFER_HANDLE * transferHandle,
    void * data,
    size_t size
);

USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_DIAG_Read
(
    USB_HOST_SIMCOM_TRANSFER_HANDLE * transferHandle,
    void * data,
    size_t size
);

/** 
@Function
  int usb_simcom_init(void) 

@Summary
  Inicializa la lógica de usb del módulo de comunicaciones SIMCOM A7672E

@Remarks
  Inicializa la lógica de usb del módulo de comunicaciones SIMCOM A7672E.
* Esta función tiene en cuenta que el microcontrolador donde es ejecutada tiene
* un único bus usb, a través del cual pueden comunicarse como máximo 
* MAX_NUM_DEVICES dispositivos.
*/
int usb_simcom_init(void);

/** 
@Function
  int usb_simcom_tasks(void) 

@Summary
  Task con la lógica de usb del módulo de comunicaciones SIMCOM A7672E

@Remarks
  Task con la lógica de usb del módulo de comunicaciones SIMCOM A7672E.
* Esta función tiene en cuenta que el microcontrolador donde es ejecutada tiene
* un único bus usb, a través del cual pueden comunicarse como máximo 
* MAX_NUM_DEVICES dispositivos.
*/

int usb_simcom_tasks(void);


/* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _EXAMPLE_FILE_NAME_H */

/* *****************************************************************************
 End of File
 */
