/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    filename.c

  @Summary
    Brief description of the file.

  @Description
    Describe the purpose of this file.
 */
/* ************************************************************************** */

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

#include "usb_A7672E.h"

#include "definitions.h"
#include "config/default/usb/usb_host.h"
#include "config/default/usb/usb_host_generic.h"

//TEMPORAL
#include "log.h"


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */

USB_HOST_BUS_DATA USBHost0Data;

USB_HOST_SIMCOM_INSTANCE_OBJ gUSBHostSIMCOMObj;

/* ************************************************************************** */
/** Descriptive Data Item Name

  @Summary
    Brief one-line summary of the data item.
    
  @Description
    Full description, explaining the purpose and usage of data item.
    <p>
    Additional description in consecutive paragraphs separated by HTML 
    paragraph breaks, as necessary.
    <p>
    Type "JavaDoc" in the "How Do I?" IDE toolbar for more information on tags.
    
  @Remarks
    Any additional remarks
 */

//USB_HOST_BUS0_DATA get_usb_data(void){
//    return usb_simcom_data;
//}


/* ************************************************************************** */
/* ************************************************************************** */
// Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */

/*  A brief description of a section can be given directly below the section
    banner.
 */

/* Function:  
    USB_HOST_SIMCOM_RESULT F_USB_HOST_CDC_HostResutlToCDCResultMap
    (
        USB_HOST_RESULT result
    )

  Summary: 
    This function will map the USB Host result to CDC Result.

  Description:
    This function will map the USB Host result to CDC Result.

  Remarks:
    This is a local function and should not be called by directly by the
    application.
*/

USB_HOST_SIMCOM_RESULT F_USB_HOST_SIMCOM_HostResutlToSIMCOMResultMap
(
    USB_HOST_RESULT result
)
{
    USB_HOST_SIMCOM_RESULT simcomResult;
  
    switch(result)
    {
        case USB_HOST_RESULT_SUCCESS:
            simcomResult = USB_HOST_SIMCOM_RESULT_SUCCESS;
            break;
        case USB_HOST_RESULT_FAILURE:
            /* Note the fall through here. This is intentional */
        case USB_HOST_RESULT_PARAMETER_INVALID:
        case USB_HOST_RESULT_PIPE_HANDLE_INVALID:
            simcomResult = USB_HOST_SIMCOM_RESULT_FAILURE;
            break;
        case USB_HOST_RESULT_REQUEST_BUSY:
            simcomResult = USB_HOST_SIMCOM_RESULT_BUSY;
            break;
        case USB_HOST_RESULT_REQUEST_STALLED:
            simcomResult = USB_HOST_SIMCOM_RESULT_REQUEST_STALLED;
            break;
        case USB_HOST_RESULT_TRANSFER_ABORTED:
            simcomResult = USB_HOST_SIMCOM_RESULT_ABORTED;
            break;
        default:
            simcomResult = USB_HOST_SIMCOM_RESULT_FAILURE;
            break;
    }
  
    return(simcomResult);
}

static void F_USB_HOST_HostDataInit(void){
    USBHost0Data.state = USB_HOST_STATE_INIT;
    USBHost0Data.n_devices = 0;
}

static void F_USB_HOST_SIMCOMInit(void){
    gUSBHostSIMCOMObj.state = USB_HOST_SIMCOM_STATE_NOT_READY;
}

static USB_HOST_EVENT_RESPONSE usb_host_event_handler
(
    USB_HOST_EVENT event, 
    void * eventData, 
    uintptr_t context
)
{
    switch(event)
    {
        case USB_HOST_EVENT_DEVICE_REJECTED_INSUFFICIENT_POWER:
            _l("Dispositivo USB rechazado. Energía insuficiente\n\r");
            break;

        case USB_HOST_EVENT_DEVICE_UNSUPPORTED:
            _l("Dispositivo USB desconocido.\n\r");
            break;

        case USB_HOST_EVENT_HUB_TIER_LEVEL_EXCEEDED:
            _l("Se ha excedido el número de hubs conectados al host.\n\r");
            break;

        case USB_HOST_EVENT_PORT_OVERCURRENT_DETECTED:
            _l("Overcurrent condition en el hub.\n\r");
            break;
    }

    return USB_HOST_EVENT_RESPONSE_NONE; // No se requiere acción adicional
}

// Generic driver functions
static void F_USB_HOST_SIMCOM_DeviceAssign
(
    USB_HOST_DEVICE_CLIENT_HANDLE deviceHandle,
    USB_HOST_DEVICE_OBJ_HANDLE deviceObjHandle,
    USB_DEVICE_DESCRIPTOR * deviceDescriptor
){
    
    USB_HOST_SIMCOM_INSTANCE_OBJ * simcomInstance = NULL;
    simcomInstance = &gUSBHostSIMCOMObj;
    simcomInstance->deviceClientHandle = deviceHandle;
    simcomInstance->deviceObjHandle = deviceObjHandle;
    
    if (deviceDescriptor->bNumConfigurations > 0U)
    {
        // Abrimos el pipe de control para futuras transferenciasy pasamos a estado donde activamos la configuración
        simcomInstance->state = USB_HOST_SIMCOM_STATE_SET_CONFIGURATION;
        simcomInstance->controlPipeHandle = USB_HOST_DeviceControlPipeOpen(deviceObjHandle);

        // Mostrar descriptores
        //USB_HOST_ShowDescriptors();
        
    }
    else
    {
        // Liberar si no se puede usar
        simcomInstance->state = USB_HOST_SIMCOM_STATE_ERROR;
        (void) USB_HOST_DeviceRelease(deviceHandle);
    }
}

static void F_USB_HOST_SIMCOM_DeviceRelease
(
    USB_HOST_DEVICE_CLIENT_HANDLE deviceHandle
){
    
    USB_HOST_SIMCOM_INSTANCE_OBJ * simcomInstance;
    simcomInstance = &gUSBHostSIMCOMObj;
    
    if(simcomInstance->RNDISInterface.bulkInPipeHandle != USB_HOST_PIPE_HANDLE_INVALID)
    {
        /* Close the bulk in pipe and invalidate the pipe handle */
        (void) USB_HOST_DevicePipeClose(simcomInstance->RNDISInterface.bulkInPipeHandle);
        simcomInstance->RNDISInterface.bulkInPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
    }
    
    if(simcomInstance->RNDISInterface.bulkOutPipeHandle != USB_HOST_PIPE_HANDLE_INVALID)
    {
        /* Close the bulk in pipe and invalidate the pipe handle */
        (void) USB_HOST_DevicePipeClose(simcomInstance->RNDISInterface.bulkOutPipeHandle);
        simcomInstance->RNDISInterface.bulkOutPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
    }
    
    if(simcomInstance->RNDISInterface.interruptPipeHandle != USB_HOST_PIPE_HANDLE_INVALID)
    {
        /* Close the bulk in pipe and invalidate the pipe handle */
        (void) USB_HOST_DevicePipeClose(simcomInstance->RNDISInterface.interruptPipeHandle);
        simcomInstance->RNDISInterface.interruptPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
    }
    
    if(simcomInstance->diagInterface.bulkInPipeHandle != USB_HOST_PIPE_HANDLE_INVALID)
    {
        /* Close the bulk in pipe and invalidate the pipe handle */
        (void) USB_HOST_DevicePipeClose(simcomInstance->diagInterface.bulkInPipeHandle);
        simcomInstance->diagInterface.bulkInPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
    }
    
    if(simcomInstance->diagInterface.bulkOutPipeHandle != USB_HOST_PIPE_HANDLE_INVALID)
    {
        /* Close the bulk in pipe and invalidate the pipe handle */
        (void) USB_HOST_DevicePipeClose(simcomInstance->diagInterface.bulkOutPipeHandle);
        simcomInstance->diagInterface.bulkOutPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
    }
    
    if(simcomInstance->ATInterface.bulkInPipeHandle != USB_HOST_PIPE_HANDLE_INVALID)
    {
        /* Close the bulk in pipe and invalidate the pipe handle */
        (void) USB_HOST_DevicePipeClose(simcomInstance->ATInterface.bulkInPipeHandle);
        simcomInstance->ATInterface.bulkInPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
    }
    
    if(simcomInstance->ATInterface.bulkOutPipeHandle != USB_HOST_PIPE_HANDLE_INVALID)
    {
        /* Close the bulk in pipe and invalidate the pipe handle */
        (void) USB_HOST_DevicePipeClose(simcomInstance->ATInterface.bulkOutPipeHandle);
        simcomInstance->ATInterface.bulkOutPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
    }
    
    if(simcomInstance->ATInterface.interruptPipeHandle != USB_HOST_PIPE_HANDLE_INVALID)
    {
        /* Close the bulk in pipe and invalidate the pipe handle */
        (void) USB_HOST_DevicePipeClose(simcomInstance->ATInterface.interruptPipeHandle);
        simcomInstance->ATInterface.interruptPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
    }
    
    if(simcomInstance->ECMInterface.bulkInPipeHandle != USB_HOST_PIPE_HANDLE_INVALID)
    {
        /* Close the bulk in pipe and invalidate the pipe handle */
        (void) USB_HOST_DevicePipeClose(simcomInstance->ECMInterface.bulkInPipeHandle);
        simcomInstance->ECMInterface.bulkInPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
    }
    
    if(simcomInstance->ECMInterface.bulkOutPipeHandle != USB_HOST_PIPE_HANDLE_INVALID)
    {
        /* Close the bulk in pipe and invalidate the pipe handle */
        (void) USB_HOST_DevicePipeClose(simcomInstance->ECMInterface.bulkOutPipeHandle);
        simcomInstance->ECMInterface.bulkOutPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
    }
    
    if(simcomInstance->ECMInterface.interruptPipeHandle != USB_HOST_PIPE_HANDLE_INVALID)
    {
        /* Close the bulk in pipe and invalidate the pipe handle */
        (void) USB_HOST_DevicePipeClose(simcomInstance->ECMInterface.interruptPipeHandle);
        simcomInstance->ECMInterface.interruptPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
    }
    
    if(simcomInstance->eventHandler != NULL)
    {
        /* Let the client know that the device is detached */
        (void) simcomInstance->eventHandler( USB_HOST_SIMCOM_EVENT_DEVICE_DETACHED,
                NULL, simcomInstance->context);
    }
    
    /* Release the object */
    simcomInstance->eventHandler = NULL;
    simcomInstance->deviceObjHandle = USB_HOST_DEVICE_OBJ_HANDLE_INVALID;
    simcomInstance->deviceClientHandle = USB_HOST_DEVICE_CLIENT_HANDLE_INVALID;
    
}

void cb( // AÑADIDO POR GERARDO
    USB_HOST_REQUEST_HANDLE requestHandle, 
    size_t size, 
    uintptr_t context
){
    
}

static void F_USB_HOST_SIMCOM_InterfaceAssign
(
    USB_HOST_DEVICE_INTERFACE_HANDLE * interfaces,
    USB_HOST_DEVICE_OBJ_HANDLE deviceObjHandle,
    size_t nInterfaces,
    uint8_t * descriptor
){
    
    uint32_t iterator;
    USB_HOST_SIMCOM_INSTANCE_OBJ * simcomInstance = NULL;
    USB_INTERFACE_DESCRIPTOR * interfaceDescriptor;
    USB_ENDPOINT_DESCRIPTOR * endpointDescriptor;
    USB_HOST_ENDPOINT_DESCRIPTOR_QUERY endpointDescriptorQuery;
    USB_HOST_INTERFACE_DESCRIPTOR_QUERY interfaceDescriptorQuery;
    uint32_t temp_32;
    
    simcomInstance = &gUSBHostSIMCOMObj;
    
    if (nInterfaces == 1U){
        
        // En este caso puede ser cualquiera de las otras 3 interfaces
        interfaceDescriptor = (USB_INTERFACE_DESCRIPTOR *)descriptor;
        
        // Todas las interfaces que no tienen una IAD son  clase vendor specific
        if((interfaceDescriptor->bInterfaceClass == (uint32_t)USB_VENDOR_SPECIFIC_CLASS_CODE))
        {
            switch(interfaceDescriptor->bInterfaceNumber){
                case 0x02:
                    // Interfaz de diagnóstico
                    simcomInstance->diagInterface.interfaceHandle = interfaces[0];
                    simcomInstance->diagInterface.interfaceNumber = interfaceDescriptor->bInterfaceNumber;
                    
                    USB_HOST_DeviceEndpointQueryContextClear(&endpointDescriptorQuery);
                    endpointDescriptorQuery.transferType = USB_TRANSFER_TYPE_BULK;
                    endpointDescriptorQuery.direction = USB_DATA_DIRECTION_DEVICE_TO_HOST;
                    temp_32 = ((uint32_t)USB_HOST_ENDPOINT_QUERY_BY_TRANSFER_TYPE|(uint32_t)USB_HOST_ENDPOINT_QUERY_BY_DIRECTION);
                    endpointDescriptorQuery.flags = (USB_HOST_ENDPOINT_QUERY_FLAG)temp_32;
                    endpointDescriptor = USB_HOST_DeviceEndpointDescriptorQuery(interfaceDescriptor, &endpointDescriptorQuery);
                    
                    /* Did we find the endpoint? */
                    if(endpointDescriptor != NULL)
                    {
                        /* Found the endpoint. Open the pipe. If the pipe was not
                         * opened, the device will never move to a ready state. */
                        simcomInstance->diagInterface.bulkInPipeHandle = USB_HOST_DevicePipeOpen(simcomInstance->diagInterface.interfaceHandle,
                                endpointDescriptor->bEndpointAddress);
                    }
                    else
                    {
                        /* Make the pipe handle invalid */
                        simcomInstance->diagInterface.bulkInPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
                    }
                    
                    /* Bulk in pipe is opened. Now open the bulk out pipe */
                    USB_HOST_DeviceEndpointQueryContextClear(&endpointDescriptorQuery);
                    endpointDescriptorQuery.transferType = USB_TRANSFER_TYPE_BULK;
                    endpointDescriptorQuery.direction = USB_DATA_DIRECTION_HOST_TO_DEVICE;
                    temp_32 = ((uint32_t)USB_HOST_ENDPOINT_QUERY_BY_TRANSFER_TYPE|(uint32_t)USB_HOST_ENDPOINT_QUERY_BY_DIRECTION);
                    endpointDescriptorQuery.flags = (USB_HOST_ENDPOINT_QUERY_FLAG)temp_32;
                    endpointDescriptor = USB_HOST_DeviceEndpointDescriptorQuery(interfaceDescriptor, &endpointDescriptorQuery);
                    
                    /* Did we find the pipe */
                    if(endpointDescriptor != NULL)
                    {
                        /* Yes we did. Open this pipe */
                        simcomInstance->diagInterface.bulkOutPipeHandle = USB_HOST_DevicePipeOpen(simcomInstance->diagInterface.interfaceHandle, 
                                endpointDescriptor->bEndpointAddress);
                    }
                    else
                    {
                        /* Make the pipe handle invalid */
                        simcomInstance->diagInterface.bulkOutPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
                    }
                    break;
                    
                case 0x04:
                    // Interfaz de comandos AT
                    simcomInstance->ATInterface.interfaceHandle = interfaces[0];
                    simcomInstance->ATInterface.interfaceNumber = interfaceDescriptor->bInterfaceNumber;
                    
                    USB_HOST_DeviceEndpointQueryContextClear(&endpointDescriptorQuery);
                    endpointDescriptorQuery.transferType = USB_TRANSFER_TYPE_INTERRUPT;
                    endpointDescriptorQuery.direction = USB_DATA_DIRECTION_DEVICE_TO_HOST;
                    temp_32 = ((uint32_t)USB_HOST_ENDPOINT_QUERY_BY_TRANSFER_TYPE|(uint32_t)USB_HOST_ENDPOINT_QUERY_BY_DIRECTION);
                    endpointDescriptorQuery.flags = (USB_HOST_ENDPOINT_QUERY_FLAG)temp_32;
                    endpointDescriptor = USB_HOST_DeviceEndpointDescriptorQuery(interfaceDescriptor, &endpointDescriptorQuery);
                    
                    /* Did we find the endpoint? */
                    if(endpointDescriptor != NULL)
                    {
                        /* Found the endpoint. Open the pipe. If the pipe was not
                         * opened, the device will never move to a ready state. */
                        simcomInstance->ATInterface.interruptPipeHandle = USB_HOST_DevicePipeOpen(simcomInstance->ATInterface.interfaceHandle,
                                endpointDescriptor->bEndpointAddress);
                    }
                    else
                    {
                        /* Make the pipe handle invalid */
                        simcomInstance->ATInterface.interruptPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
                    }
                    
                    USB_HOST_DeviceEndpointQueryContextClear(&endpointDescriptorQuery);
                    endpointDescriptorQuery.transferType = USB_TRANSFER_TYPE_BULK;
                    endpointDescriptorQuery.direction = USB_DATA_DIRECTION_DEVICE_TO_HOST;
                    temp_32 = ((uint32_t)USB_HOST_ENDPOINT_QUERY_BY_TRANSFER_TYPE|(uint32_t)USB_HOST_ENDPOINT_QUERY_BY_DIRECTION);
                    endpointDescriptorQuery.flags = (USB_HOST_ENDPOINT_QUERY_FLAG)temp_32;
                    endpointDescriptor = USB_HOST_DeviceEndpointDescriptorQuery(interfaceDescriptor, &endpointDescriptorQuery);
                    
                    /* Did we find the endpoint? */
                    if(endpointDescriptor != NULL)
                    {
                        /* Found the endpoint. Open the pipe. If the pipe was not
                         * opened, the device will never move to a ready state. */
                        simcomInstance->ATInterface.bulkInPipeHandle = USB_HOST_DevicePipeOpen(simcomInstance->ATInterface.interfaceHandle,
                                endpointDescriptor->bEndpointAddress);
                    }
                    else
                    {
                        /* Make the pipe handle invalid */
                        simcomInstance->ATInterface.bulkInPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
                    }
                    
                    /* Bulk in pipe is opened. Now open the bulk out pipe */
                    USB_HOST_DeviceEndpointQueryContextClear(&endpointDescriptorQuery);
                    endpointDescriptorQuery.transferType = USB_TRANSFER_TYPE_BULK;
                    endpointDescriptorQuery.direction = USB_DATA_DIRECTION_HOST_TO_DEVICE;
                    temp_32 = ((uint32_t)USB_HOST_ENDPOINT_QUERY_BY_TRANSFER_TYPE|(uint32_t)USB_HOST_ENDPOINT_QUERY_BY_DIRECTION);
                    endpointDescriptorQuery.flags = (USB_HOST_ENDPOINT_QUERY_FLAG)temp_32;
                    endpointDescriptor = USB_HOST_DeviceEndpointDescriptorQuery(interfaceDescriptor, &endpointDescriptorQuery);
                    
                    /* Did we find the pipe */
                    if(endpointDescriptor != NULL)
                    {
                        /* Yes we did. Open this pipe */
                        simcomInstance->ATInterface.bulkOutPipeHandle = USB_HOST_DevicePipeOpen(simcomInstance->ATInterface.interfaceHandle, 
                                endpointDescriptor->bEndpointAddress);
                    }
                    else
                    {
                        /* Make the pipe handle invalid */
                        simcomInstance->ATInterface.bulkOutPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
                    }
                    break;
                    
                case 0x05:
                    // Interfaz de módem ECM
                    simcomInstance->ECMInterface.interfaceHandle = interfaces[0];
                    simcomInstance->ECMInterface.interfaceNumber = interfaceDescriptor->bInterfaceNumber;
                    
                    USB_HOST_DeviceEndpointQueryContextClear(&endpointDescriptorQuery);
                    endpointDescriptorQuery.transferType = USB_TRANSFER_TYPE_INTERRUPT;
                    endpointDescriptorQuery.direction = USB_DATA_DIRECTION_DEVICE_TO_HOST;
                    temp_32 = ((uint32_t)USB_HOST_ENDPOINT_QUERY_BY_TRANSFER_TYPE|(uint32_t)USB_HOST_ENDPOINT_QUERY_BY_DIRECTION);
                    endpointDescriptorQuery.flags = (USB_HOST_ENDPOINT_QUERY_FLAG)temp_32;
                    endpointDescriptor = USB_HOST_DeviceEndpointDescriptorQuery(interfaceDescriptor, &endpointDescriptorQuery);
                    
                    /* Did we find the endpoint? */
                    if(endpointDescriptor != NULL)
                    {
                        /* Found the endpoint. Open the pipe. If the pipe was not
                         * opened, the device will never move to a ready state. */
                        simcomInstance->ECMInterface.interruptPipeHandle = USB_HOST_DevicePipeOpen(simcomInstance->ECMInterface.interfaceHandle,
                                endpointDescriptor->bEndpointAddress);
                    }
                    else
                    {
                        /* Make the pipe handle invalid */
                        simcomInstance->ECMInterface.interruptPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
                    }
                    
                    USB_HOST_DeviceEndpointQueryContextClear(&endpointDescriptorQuery);
                    endpointDescriptorQuery.transferType = USB_TRANSFER_TYPE_BULK;
                    endpointDescriptorQuery.direction = USB_DATA_DIRECTION_DEVICE_TO_HOST;
                    temp_32 = ((uint32_t)USB_HOST_ENDPOINT_QUERY_BY_TRANSFER_TYPE|(uint32_t)USB_HOST_ENDPOINT_QUERY_BY_DIRECTION);
                    endpointDescriptorQuery.flags = (USB_HOST_ENDPOINT_QUERY_FLAG)temp_32;
                    endpointDescriptor = USB_HOST_DeviceEndpointDescriptorQuery(interfaceDescriptor, &endpointDescriptorQuery);
                    
                    /* Did we find the endpoint? */
                    if(endpointDescriptor != NULL)
                    {
                        /* Found the endpoint. Open the pipe. If the pipe was not
                         * opened, the device will never move to a ready state. */
                        simcomInstance->ECMInterface.bulkInPipeHandle = USB_HOST_DevicePipeOpen(simcomInstance->ECMInterface.interfaceHandle,
                                endpointDescriptor->bEndpointAddress);
                    }
                    else
                    {
                        /* Make the pipe handle invalid */
                        simcomInstance->ECMInterface.bulkInPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
                    }
                    
                    /* Bulk in pipe is opened. Now open the bulk out pipe */
                    USB_HOST_DeviceEndpointQueryContextClear(&endpointDescriptorQuery);
                    endpointDescriptorQuery.transferType = USB_TRANSFER_TYPE_BULK;
                    endpointDescriptorQuery.direction = USB_DATA_DIRECTION_HOST_TO_DEVICE;
                    temp_32 = ((uint32_t)USB_HOST_ENDPOINT_QUERY_BY_TRANSFER_TYPE|(uint32_t)USB_HOST_ENDPOINT_QUERY_BY_DIRECTION);
                    endpointDescriptorQuery.flags = (USB_HOST_ENDPOINT_QUERY_FLAG)temp_32;
                    endpointDescriptor = USB_HOST_DeviceEndpointDescriptorQuery(interfaceDescriptor, &endpointDescriptorQuery);
                    
                    /* Did we find the pipe */
                    if(endpointDescriptor != NULL)
                    {
                        /* Yes we did. Open this pipe */
                        simcomInstance->ECMInterface.bulkOutPipeHandle = USB_HOST_DevicePipeOpen(simcomInstance->ECMInterface.interfaceHandle, 
                                endpointDescriptor->bEndpointAddress);
                    }
                    else
                    {
                        /* Make the pipe handle invalid */
                        simcomInstance->ECMInterface.bulkOutPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
                    }
                    break;
                    
                default:
                    (void) USB_HOST_DeviceInterfaceRelease(interfaces[0]);
                    break;
            }
        }
        else{
            (void) USB_HOST_DeviceInterfaceRelease(interfaces[0]);
        }
        
    }
    else if(nInterfaces > 1U){
        
        // En caso de que haya más de una interfaz, significa que es una Asociación
        // de interfaces. La única asociación de interfaces es el módem RNDIS,
        // por lo que se puede asignar directamente
        
        USB_HOST_DeviceInterfaceQueryContextClear(&interfaceDescriptorQuery);
        interfaceDescriptorQuery.flags = USB_HOST_INTERFACE_QUERY_ANY;
        
        for(iterator = 0; iterator < 2U; iterator ++){
            
            interfaceDescriptor = USB_HOST_DeviceGeneralInterfaceDescriptorQuery
            ((USB_INTERFACE_ASSOCIATION_DESCRIPTOR *)(descriptor), &interfaceDescriptorQuery);
            
            if(interfaceDescriptor != NULL){
                
                if((interfaceDescriptor->bInterfaceClass == USB_RNDIS_CLASS_CODE) &&
                    (interfaceDescriptor->bInterfaceSubClass == (uint32_t)USB_RNDIS_SUBCLASS_CODE) &&
                    (interfaceDescriptor->bInterfaceProtocol == (uint32_t)USB_RNDIS_PROTOCOL))
                {
                    /* Hemos encontrado la interfaz RNDIS */
                    simcomInstance->RNDISInterface.commInterfaceNumber = interfaceDescriptor->bInterfaceNumber;
                    simcomInstance->RNDISInterface.commInterfaceHandle = interfaces[iterator];
                    
                    /* Create the endpoint query */
                    USB_HOST_DeviceEndpointQueryContextClear(&endpointDescriptorQuery);
                    endpointDescriptorQuery.transferType = USB_TRANSFER_TYPE_INTERRUPT;
                    endpointDescriptorQuery.direction = USB_DATA_DIRECTION_DEVICE_TO_HOST;
                    temp_32 = ((uint32_t)USB_HOST_ENDPOINT_QUERY_BY_TRANSFER_TYPE|(uint32_t)USB_HOST_ENDPOINT_QUERY_BY_DIRECTION);
                    endpointDescriptorQuery.flags = (USB_HOST_ENDPOINT_QUERY_FLAG)temp_32;
                    endpointDescriptor = USB_HOST_DeviceEndpointDescriptorQuery(interfaceDescriptor, &endpointDescriptorQuery);

                    if(endpointDescriptor != NULL)
                    {
                        /* Open the pipe */
                        simcomInstance->RNDISInterface.interruptPipeHandle = USB_HOST_DevicePipeOpen(simcomInstance->RNDISInterface.commInterfaceHandle, 
                                endpointDescriptor->bEndpointAddress);
                    }
                    else
                    {
                        /* Make sure that the pipe handle stays invalid if
                         * we could not open the pipe */
                        simcomInstance->RNDISInterface.interruptPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
                    }
                }
                else if(interfaceDescriptor->bInterfaceClass == USB_CDC_DATA_CLASS_CODE)
                {
                    /* Hemos encontrado la interfaz CDC Data */
                    simcomInstance->RNDISInterface.dataInterfaceNumber = interfaceDescriptor->bInterfaceNumber;
                    simcomInstance->RNDISInterface.dataInterfaceHandle = interfaces[iterator];
                    
                    /* Obtenemos el endpoint bulk in */
                    USB_HOST_DeviceEndpointQueryContextClear(&endpointDescriptorQuery);
                    endpointDescriptorQuery.transferType = USB_TRANSFER_TYPE_BULK;
                    endpointDescriptorQuery.direction = USB_DATA_DIRECTION_DEVICE_TO_HOST;
                    temp_32 = ((uint32_t)USB_HOST_ENDPOINT_QUERY_BY_TRANSFER_TYPE|(uint32_t)USB_HOST_ENDPOINT_QUERY_BY_DIRECTION);
                    endpointDescriptorQuery.flags = (USB_HOST_ENDPOINT_QUERY_FLAG)temp_32;
                    endpointDescriptor = USB_HOST_DeviceEndpointDescriptorQuery(interfaceDescriptor, &endpointDescriptorQuery);

                    if(endpointDescriptor != NULL)
                    {
                        /* Open the pipe */
                        simcomInstance->RNDISInterface.bulkInPipeHandle = USB_HOST_DevicePipeOpen(simcomInstance->RNDISInterface.dataInterfaceHandle, 
                                endpointDescriptor->bEndpointAddress);
                    }
                    else
                    {
                        /* Make sure that the pipe handle stays invalid if
                         * we could not open the pipe */
                        simcomInstance->RNDISInterface.bulkInPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
                    }
                    
                    /* Obtenemos el endpoint bulk out */
                    USB_HOST_DeviceEndpointQueryContextClear(&endpointDescriptorQuery);
                    endpointDescriptorQuery.transferType = USB_TRANSFER_TYPE_BULK;
                    endpointDescriptorQuery.direction = USB_DATA_DIRECTION_HOST_TO_DEVICE;
                    temp_32 = ((uint32_t)USB_HOST_ENDPOINT_QUERY_BY_TRANSFER_TYPE|(uint32_t)USB_HOST_ENDPOINT_QUERY_BY_DIRECTION);
                    endpointDescriptorQuery.flags = (USB_HOST_ENDPOINT_QUERY_FLAG)temp_32;
                    endpointDescriptor = USB_HOST_DeviceEndpointDescriptorQuery(interfaceDescriptor, &endpointDescriptorQuery);

                    if(endpointDescriptor != NULL)
                    {
                        /* Open the pipe */
                        simcomInstance->RNDISInterface.bulkOutPipeHandle = USB_HOST_DevicePipeOpen(simcomInstance->RNDISInterface.dataInterfaceHandle, 
                                endpointDescriptor->bEndpointAddress);
                    }
                    else
                    {
                        /* Make sure that the pipe handle stays invalid if
                         * we could not open the pipe */
                        simcomInstance->RNDISInterface.bulkOutPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
                    }
                }
                else{
                    /*NADA*/
                }
                
            }
                
        }
    }
}

static void F_USB_HOST_SIMCOM_InterfaceRelease
(
    USB_HOST_DEVICE_CLIENT_HANDLE deviceHandle
){
    
}

static USB_HOST_DEVICE_EVENT_RESPONSE F_USB_HOST_SIMCOM_DeviceEventHandler
(
    USB_HOST_DEVICE_CLIENT_HANDLE deviceHandle,
    USB_HOST_DEVICE_EVENT event,
    void * eventData,
    uintptr_t context
){
    
    /* The event context is the pointer to the SIMCOM Instance Object */
    USB_HOST_SIMCOM_INSTANCE_OBJ * simcomInstance = (USB_HOST_SIMCOM_INSTANCE_OBJ *)(context);
    USB_HOST_DEVICE_EVENT_CONFIGURATION_SET_DATA * configSetEventData;
    
    switch(event)
    {
        case USB_HOST_DEVICE_EVENT_CONFIGURATION_SET:

            /* This means the configuration was set. Update the instance
             * variables to let the main state machine know. */
            configSetEventData = (USB_HOST_DEVICE_EVENT_CONFIGURATION_SET_DATA *)(eventData);
            simcomInstance->hostRequestResult =  configSetEventData->result;
            simcomInstance->hostRequestDone = true;
            break;

        case USB_HOST_DEVICE_EVENT_CONFIGURATION_DESCRIPTOR_GET_COMPLETE:
            break;

        default:
            /* Do Nothing */
            break;
    }
    
    return (USB_HOST_DEVICE_EVENT_RESPONSE_NONE);
}

static USB_HOST_DEVICE_INTERFACE_EVENT_RESPONSE F_USB_HOST_SIMCOM_InterfaceEventHandler
(
    USB_HOST_DEVICE_INTERFACE_HANDLE interfaceHandle,
    USB_HOST_DEVICE_INTERFACE_EVENT event,
    void * eventData,
    uintptr_t context
){
    
    USB_HOST_SIMCOM_INSTANCE_OBJ * simcomInstance;
    USB_HOST_DEVICE_INTERFACE_EVENT_TRANSFER_COMPLETE_DATA * dataTransferEvent;
    USB_HOST_SIMCOM_EVENT simcomEvent;
    USB_HOST_SIMCOM_TRANSFER_COMPLETE_DATA simcomTransferCompleteData;
    
    simcomInstance = &gUSBHostSIMCOMObj;
    simcomEvent = (USB_HOST_SIMCOM_EVENT)(context);
    
    switch(event)
       {
           case USB_HOST_DEVICE_INTERFACE_EVENT_TRANSFER_COMPLETE:

              /* This means a data transfer has completed */
              dataTransferEvent = (USB_HOST_DEVICE_INTERFACE_EVENT_TRANSFER_COMPLETE_DATA *)(eventData);
              simcomTransferCompleteData.transferHandle = dataTransferEvent->transferHandle;
              simcomTransferCompleteData.result = dataTransferEvent->result;
              simcomTransferCompleteData.length = dataTransferEvent->length;

               if(simcomInstance->eventHandler != NULL)
               {
                  (void) simcomInstance->eventHandler(simcomEvent, &simcomTransferCompleteData, simcomInstance->context);
               }

            break;

          default:
             /* Do Nothing */
            break;
        }
    
    return (USB_HOST_DEVICE_INTERFACE_EVENT_RESPONSE_NONE);
}

/* ************************************************************************** */
/* ************************************************************************** */
// Section: Interface Functions                                               */
/* ************************************************************************** */
/* ************************************************************************** */

// *****************************************************************************

USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_EventHandlerSet
(
    USB_HOST_SIMCOM_EVENT_HANDLER eventHandler,
    uintptr_t context
)
{
    USB_HOST_SIMCOM_RESULT result = USB_HOST_SIMCOM_RESULT_HANDLE_INVALID;
    USB_HOST_SIMCOM_INSTANCE_OBJ * simcomInstance = &gUSBHostSIMCOMObj;

    simcomInstance->eventHandler = eventHandler;
    simcomInstance->context = context;
    result = USB_HOST_SIMCOM_RESULT_SUCCESS;

    return(result);
}

/** 
    Function
      void F_USB_HOST_SIMCOM_Initialize(void* data)

    Summary:
    This function is called when the Host Layer is initializing.

    Description:
    This function is called when the Host Layer is initializing.
*/
void F_USB_HOST_SIMCOM_Initialize(void* data){
    
    USB_HOST_SIMCOM_INSTANCE_OBJ * simcomInstance;
    
    simcomInstance = &gUSBHostSIMCOMObj;
    simcomInstance->deviceClientHandle = USB_HOST_DEVICE_CLIENT_HANDLE_INVALID;
    simcomInstance->deviceObjHandle = USB_HOST_DEVICE_OBJ_HANDLE_INVALID;
    simcomInstance->RNDISInterface.bulkInPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
    simcomInstance->RNDISInterface.bulkOutPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
    simcomInstance->RNDISInterface.interruptPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
    simcomInstance->diagInterface.bulkInPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
    simcomInstance->diagInterface.bulkOutPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
    simcomInstance->ATInterface.bulkInPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
    simcomInstance->ATInterface.bulkOutPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
    simcomInstance->ATInterface.interruptPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
    simcomInstance->ECMInterface.bulkInPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
    simcomInstance->ECMInterface.bulkOutPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
    simcomInstance->ECMInterface.interruptPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
}

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
void F_USB_HOST_SIMCOM_DeviceTasks(USB_HOST_DEVICE_CLIENT_HANDLE deviceHandle){
    
    USB_HOST_SIMCOM_INSTANCE_OBJ * simcomInstance;
    USB_HOST_RESULT result;
    USB_HOST_REQUEST_HANDLE requestHandle;
    
    simcomInstance = &gUSBHostSIMCOMObj;
    
    switch (simcomInstance->state){
        
        case USB_HOST_SIMCOM_STATE_NOT_READY:
            // Esperar a que se ejecute DeviceAssign
            break;
            
        case USB_HOST_SIMCOM_STATE_SET_CONFIGURATION:
            
            simcomInstance->hostRequestDone = false;
            result = USB_HOST_DeviceConfigurationSet(simcomInstance->deviceClientHandle, &requestHandle, 
                    0, (uintptr_t)(simcomInstance));

            if(result == USB_HOST_RESULT_SUCCESS)
            {
                /* The result was successful. Change state to wating for
                 * configuration. */
                simcomInstance->state = USB_HOST_SIMCOM_STATE_WAIT_FOR_CONFIGURATION_SET;
            }
            break;
            
        case USB_HOST_SIMCOM_STATE_WAIT_FOR_CONFIGURATION_SET:
            
            /* Here we are waiting for the configuration to be set */
            if(simcomInstance->hostRequestDone == true) 
            {
                if(simcomInstance->hostRequestResult == USB_HOST_RESULT_SUCCESS)
                {
                    /* The configuration has been set. Now wait for the host
                     * layer to send the communication and the data
                     * interface to the client driver */

                    simcomInstance->state = USB_HOST_SIMCOM_STATE_WAIT_FOR_INTERFACES;
                }
                else
                {
                    /* If we could not set the configuration, then state
                     * instance state to error */
                    simcomInstance->state = USB_HOST_SIMCOM_STATE_ERROR;
                }
            }
            break;
            
        case USB_HOST_SIMCOM_STATE_WAIT_FOR_INTERFACES:
            
            if((simcomInstance->RNDISInterface.interruptPipeHandle != USB_HOST_PIPE_HANDLE_INVALID) &&
                    (simcomInstance->RNDISInterface.bulkInPipeHandle != USB_HOST_PIPE_HANDLE_INVALID) &&
                    (simcomInstance->RNDISInterface.bulkOutPipeHandle != USB_HOST_PIPE_HANDLE_INVALID) &&
                    (simcomInstance->ATInterface.interruptPipeHandle != USB_HOST_PIPE_HANDLE_INVALID) &&
                    (simcomInstance->ATInterface.bulkInPipeHandle != USB_HOST_PIPE_HANDLE_INVALID) &&
                    (simcomInstance->ATInterface.bulkOutPipeHandle != USB_HOST_PIPE_HANDLE_INVALID) &&
                    (simcomInstance->diagInterface.bulkInPipeHandle != USB_HOST_PIPE_HANDLE_INVALID) &&
                    (simcomInstance->diagInterface.bulkOutPipeHandle != USB_HOST_PIPE_HANDLE_INVALID) &&
                    (simcomInstance->ECMInterface.interruptPipeHandle != USB_HOST_PIPE_HANDLE_INVALID) &&
                    (simcomInstance->ECMInterface.bulkInPipeHandle != USB_HOST_PIPE_HANDLE_INVALID) &&
                    (simcomInstance->ECMInterface.bulkOutPipeHandle != USB_HOST_PIPE_HANDLE_INVALID))
                {
                    /* All the pipes are opened. The client driver is ready */
                    simcomInstance->state = USB_HOST_SIMCOM_STATE_READY;

                    /* We know that the client driver is now ready. We can 
                     * let all the listeners know that the device has been
                     * attached.  */

//                    for(iterator = 0; iterator < USB_HOST_CDC_ATTACH_LISTENERS_NUMBER; iterator ++)
//                    {
//                        if(gUSBHostCDCAttachListener[iterator].inUse)
//                        {
//                            /* Call the attach listener event handler 
//                             * function. */
//                            gUSBHostCDCAttachListener[iterator].eventHandler((USB_HOST_CDC_OBJ)(cdcInstance), 
//                                    gUSBHostCDCAttachListener[iterator].context);
//                        }
//                    }
                }
            break;
            
        case USB_HOST_SIMCOM_STATE_READY:
            // El driver está preparado para operar
            break;
            
        case USB_HOST_SIMCOM_STATE_ERROR:
            // Caso error, no hacemos nada
            break;
            
        default:
            // Hacer nada
            break;
    }
}

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
void F_USB_HOST_SIMCOM_InterfaceTasks(USB_HOST_DEVICE_INTERFACE_HANDLE interfaceHandle){
//    if (interfaceHandle == gUSBHostSIMCOMObj.ATInterface.interfaceHandle){
//        _d("Ejecutando task de interfaz AT\r\n");
//    }
//    else if (interfaceHandle == gUSBHostSIMCOMObj.ECMInterface.interfaceHandle){
//        _d("Ejecutando task de interfaz EDM modem\r\n");
//    }
//    else if (interfaceHandle == gUSBHostSIMCOMObj.diagInterface.interfaceHandle){
//        _d("Ejecutando task de interfaz de diagnóstico\r\n");
//    }
//    else if (interfaceHandle == gUSBHostSIMCOMObj.RNDISInterface.commInterfaceHandle){
//        _d("Ejecutando task de interfaz communication de RNDIS\r\n");
//    }
//    else if (interfaceHandle == gUSBHostSIMCOMObj.RNDISInterface.dataInterfaceHandle){
//        _d("Ejecutando task de interfaz de datos de RNDIS\r\n");
//    }
//    else{
//        _d("Interfaz desconocida\r\n");
//    }
}

bool USB_HOST_SIMCOM_IsInReadyState(void){
    USB_HOST_SIMCOM_INSTANCE_OBJ * simcomInstance;
    simcomInstance = &gUSBHostSIMCOMObj;
    return (simcomInstance->state == USB_HOST_SIMCOM_STATE_READY);
}

USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_AT_Write
(
    USB_HOST_SIMCOM_TRANSFER_HANDLE * transferHandle,
    void * data,
    size_t size
){
    USB_HOST_SIMCOM_INSTANCE_OBJ * simcomInstance;
    USB_HOST_SIMCOM_TRANSFER_HANDLE * tempTransferHandle, localTransferHandle;
    USB_HOST_SIMCOM_RESULT simcomResult = USB_HOST_SIMCOM_RESULT_FAILURE;
    USB_HOST_RESULT hostResult;
    
    simcomInstance = &gUSBHostSIMCOMObj;
    tempTransferHandle = (transferHandle == NULL) ? &localTransferHandle: transferHandle;
    
    if (simcomInstance->state != USB_HOST_SIMCOM_STATE_READY){
        simcomResult = USB_HOST_SIMCOM_RESULT_BUSY;
    }
    else{
        if((size != 0U) && (data == NULL))
        {
            /* Input paramters are not valid */
            simcomResult = USB_HOST_SIMCOM_RESULT_INVALID_PARAMETER;
        }
        else
        {
            hostResult = USB_HOST_DeviceTransfer(simcomInstance->ATInterface.bulkOutPipeHandle, tempTransferHandle, data, size, (uintptr_t)(USB_HOST_SIMCOM_EVENT_AT_WRITE_COMPLETE));
            simcomResult = F_USB_HOST_SIMCOM_HostResutlToSIMCOMResultMap(hostResult);
        }
    }
    
    return (simcomResult);
}

USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_AT_Read
(
    USB_HOST_SIMCOM_TRANSFER_HANDLE * transferHandle,
    void * data,
    size_t size
){
    USB_HOST_SIMCOM_INSTANCE_OBJ * simcomInstance;
    USB_HOST_SIMCOM_TRANSFER_HANDLE * tempTransferHandle, localTransferHandle;
    USB_HOST_SIMCOM_RESULT simcomResult = USB_HOST_SIMCOM_RESULT_FAILURE;
    USB_HOST_RESULT hostResult;
    
    simcomInstance = &gUSBHostSIMCOMObj;
    tempTransferHandle = (transferHandle == NULL) ? &localTransferHandle: transferHandle;
    
    if (simcomInstance->state != USB_HOST_SIMCOM_STATE_READY){
        simcomResult = USB_HOST_SIMCOM_RESULT_BUSY;
    }
    else{
        if((size != 0U) && (data == NULL))
        {
            /* Input paramters are not valid */
            simcomResult = USB_HOST_SIMCOM_RESULT_INVALID_PARAMETER;
        }
        else
        {
            hostResult = USB_HOST_DeviceTransfer(simcomInstance->ATInterface.bulkInPipeHandle, tempTransferHandle, data, size, (uintptr_t)(USB_HOST_SIMCOM_EVENT_AT_READ_COMPLETE));
            simcomResult = F_USB_HOST_SIMCOM_HostResutlToSIMCOMResultMap(hostResult);
        }
    }
    
    return (simcomResult);
}

USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_AT_SerialStateNotificationGet
(
    USB_HOST_SIMCOM_TRANSFER_HANDLE * transferHandle,
    void * data,
    size_t size
){
    USB_HOST_SIMCOM_INSTANCE_OBJ * simcomInstance;
    USB_HOST_SIMCOM_TRANSFER_HANDLE * tempTransferHandle, localTransferHandle;
    USB_HOST_SIMCOM_RESULT simcomResult = USB_HOST_SIMCOM_RESULT_FAILURE;
    USB_HOST_RESULT hostResult;
    
    simcomInstance = &gUSBHostSIMCOMObj;
    tempTransferHandle = (transferHandle == NULL) ? &localTransferHandle: transferHandle;
    
    if (simcomInstance->state != USB_HOST_SIMCOM_STATE_READY){
        simcomResult = USB_HOST_SIMCOM_RESULT_BUSY;
    }
    else{
        if((size != 0U) && (data == NULL))
        {
            /* Input paramters are not valid */
            simcomResult = USB_HOST_SIMCOM_RESULT_INVALID_PARAMETER;
        }
        else
        {
            hostResult = USB_HOST_DeviceTransfer(simcomInstance->ATInterface.interruptPipeHandle, tempTransferHandle, data, size,
                    (uintptr_t)(USB_HOST_SIMCOM_EVENT_AT_SERIAL_STATE_NOTIFICATION_RECEIVED));
            simcomResult = F_USB_HOST_SIMCOM_HostResutlToSIMCOMResultMap(hostResult);
        }
    }
    
    return (simcomResult);
}

USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_RNDIS_Write
(
    USB_HOST_SIMCOM_TRANSFER_HANDLE * transferHandle,
    void * data,
    size_t size
){
    USB_HOST_SIMCOM_INSTANCE_OBJ * simcomInstance;
    USB_HOST_SIMCOM_TRANSFER_HANDLE * tempTransferHandle, localTransferHandle;
    USB_HOST_SIMCOM_RESULT simcomResult = USB_HOST_SIMCOM_RESULT_FAILURE;
    USB_HOST_RESULT hostResult;
    
    simcomInstance = &gUSBHostSIMCOMObj;
    tempTransferHandle = (transferHandle == NULL) ? &localTransferHandle: transferHandle;
    
    if (simcomInstance->state != USB_HOST_SIMCOM_STATE_READY){
        simcomResult = USB_HOST_SIMCOM_RESULT_BUSY;
    }
    else{
        if((size != 0U) && (data == NULL))
        {
            /* Input paramters are not valid */
            simcomResult = USB_HOST_SIMCOM_RESULT_INVALID_PARAMETER;
        }
        else
        {
            hostResult = USB_HOST_DeviceTransfer(simcomInstance->RNDISInterface.bulkOutPipeHandle, tempTransferHandle, data, size, (uintptr_t)(USB_HOST_SIMCOM_EVENT_RNDIS_WRITE_COMPLETE));
            simcomResult = F_USB_HOST_SIMCOM_HostResutlToSIMCOMResultMap(hostResult);
        }
    }
    
    return (simcomResult);
}

USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_RNDIS_Read
(
    USB_HOST_SIMCOM_TRANSFER_HANDLE * transferHandle,
    void * data,
    size_t size
){
    USB_HOST_SIMCOM_INSTANCE_OBJ * simcomInstance;
    USB_HOST_SIMCOM_TRANSFER_HANDLE * tempTransferHandle, localTransferHandle;
    USB_HOST_SIMCOM_RESULT simcomResult = USB_HOST_SIMCOM_RESULT_FAILURE;
    USB_HOST_RESULT hostResult;
    
    simcomInstance = &gUSBHostSIMCOMObj;
    tempTransferHandle = (transferHandle == NULL) ? &localTransferHandle: transferHandle;
    
    if (simcomInstance->state != USB_HOST_SIMCOM_STATE_READY){
        simcomResult = USB_HOST_SIMCOM_RESULT_BUSY;
    }
    else{
        if((size != 0U) && (data == NULL))
        {
            /* Input paramters are not valid */
            simcomResult = USB_HOST_SIMCOM_RESULT_INVALID_PARAMETER;
        }
        else
        {
            hostResult = USB_HOST_DeviceTransfer(simcomInstance->RNDISInterface.bulkInPipeHandle, tempTransferHandle, data, size, (uintptr_t)(USB_HOST_SIMCOM_EVENT_RNDIS_READ_COMPLETE));
            simcomResult = F_USB_HOST_SIMCOM_HostResutlToSIMCOMResultMap(hostResult);
        }
    }
    
    return (simcomResult);
}

USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_RNDIS_SerialStateNotificationGet
(
    USB_HOST_SIMCOM_TRANSFER_HANDLE * transferHandle,
    void * data,
    size_t size
){
    USB_HOST_SIMCOM_INSTANCE_OBJ * simcomInstance;
    USB_HOST_SIMCOM_TRANSFER_HANDLE * tempTransferHandle, localTransferHandle;
    USB_HOST_SIMCOM_RESULT simcomResult = USB_HOST_SIMCOM_RESULT_FAILURE;
    USB_HOST_RESULT hostResult;
    
    simcomInstance = &gUSBHostSIMCOMObj;
    tempTransferHandle = (transferHandle == NULL) ? &localTransferHandle: transferHandle;
    
    if (simcomInstance->state != USB_HOST_SIMCOM_STATE_READY){
        simcomResult = USB_HOST_SIMCOM_RESULT_BUSY;
    }
    else{
        if((size != 0U) && (data == NULL))
        {
            /* Input paramters are not valid */
            simcomResult = USB_HOST_SIMCOM_RESULT_INVALID_PARAMETER;
        }
        else
        {
            hostResult = USB_HOST_DeviceTransfer(simcomInstance->RNDISInterface.interruptPipeHandle, tempTransferHandle, data, size,
                    (uintptr_t)(USB_HOST_SIMCOM_EVENT_RNDIS_SERIAL_STATE_NOTIFICATION_RECEIVED));
            simcomResult = F_USB_HOST_SIMCOM_HostResutlToSIMCOMResultMap(hostResult);
        }
    }
    
    return (simcomResult);
}

USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_ECM_Write
(
    USB_HOST_SIMCOM_TRANSFER_HANDLE * transferHandle,
    void * data,
    size_t size
){
    USB_HOST_SIMCOM_INSTANCE_OBJ * simcomInstance;
    USB_HOST_SIMCOM_TRANSFER_HANDLE * tempTransferHandle, localTransferHandle;
    USB_HOST_SIMCOM_RESULT simcomResult = USB_HOST_SIMCOM_RESULT_FAILURE;
    USB_HOST_RESULT hostResult;
    
    simcomInstance = &gUSBHostSIMCOMObj;
    tempTransferHandle = (transferHandle == NULL) ? &localTransferHandle: transferHandle;
    
    if (simcomInstance->state != USB_HOST_SIMCOM_STATE_READY){
        simcomResult = USB_HOST_SIMCOM_RESULT_BUSY;
    }
    else{
        if((size != 0U) && (data == NULL))
        {
            /* Input paramters are not valid */
            simcomResult = USB_HOST_SIMCOM_RESULT_INVALID_PARAMETER;
        }
        else
        {
            hostResult = USB_HOST_DeviceTransfer(simcomInstance->ECMInterface.bulkOutPipeHandle, tempTransferHandle, data, size, (uintptr_t)(USB_HOST_SIMCOM_EVENT_ECM_WRITE_COMPLETE));
            simcomResult = F_USB_HOST_SIMCOM_HostResutlToSIMCOMResultMap(hostResult);
        }
    }
    
    return (simcomResult);
}

USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_ECM_Read
(
    USB_HOST_SIMCOM_TRANSFER_HANDLE * transferHandle,
    void * data,
    size_t size
){
    USB_HOST_SIMCOM_INSTANCE_OBJ * simcomInstance;
    USB_HOST_SIMCOM_TRANSFER_HANDLE * tempTransferHandle, localTransferHandle;
    USB_HOST_SIMCOM_RESULT simcomResult = USB_HOST_SIMCOM_RESULT_FAILURE;
    USB_HOST_RESULT hostResult;
    
    simcomInstance = &gUSBHostSIMCOMObj;
    tempTransferHandle = (transferHandle == NULL) ? &localTransferHandle: transferHandle;
    
    if (simcomInstance->state != USB_HOST_SIMCOM_STATE_READY){
        simcomResult = USB_HOST_SIMCOM_RESULT_BUSY;
    }
    else{
        if((size != 0U) && (data == NULL))
        {
            /* Input paramters are not valid */
            simcomResult = USB_HOST_SIMCOM_RESULT_INVALID_PARAMETER;
        }
        else
        {
            hostResult = USB_HOST_DeviceTransfer(simcomInstance->ECMInterface.bulkInPipeHandle, tempTransferHandle, data, size, (uintptr_t)(USB_HOST_SIMCOM_EVENT_ECM_READ_COMPLETE));
            simcomResult = F_USB_HOST_SIMCOM_HostResutlToSIMCOMResultMap(hostResult);
        }
    }
    
    return (simcomResult);
}

USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_ECM_SerialStateNotificationGet
(
    USB_HOST_SIMCOM_TRANSFER_HANDLE * transferHandle,
    void * data,
    size_t size
){
    USB_HOST_SIMCOM_INSTANCE_OBJ * simcomInstance;
    USB_HOST_SIMCOM_TRANSFER_HANDLE * tempTransferHandle, localTransferHandle;
    USB_HOST_SIMCOM_RESULT simcomResult = USB_HOST_SIMCOM_RESULT_FAILURE;
    USB_HOST_RESULT hostResult;
    
    simcomInstance = &gUSBHostSIMCOMObj;
    tempTransferHandle = (transferHandle == NULL) ? &localTransferHandle: transferHandle;
    
    if (simcomInstance->state != USB_HOST_SIMCOM_STATE_READY){
        simcomResult = USB_HOST_SIMCOM_RESULT_BUSY;
    }
    else{
        if((size != 0U) && (data == NULL))
        {
            /* Input paramters are not valid */
            simcomResult = USB_HOST_SIMCOM_RESULT_INVALID_PARAMETER;
        }
        else
        {
            hostResult = USB_HOST_DeviceTransfer(simcomInstance->ECMInterface.interruptPipeHandle, tempTransferHandle, data, size,
                    (uintptr_t)(USB_HOST_SIMCOM_EVENT_ECM_SERIAL_STATE_NOTIFICATION_RECEIVED));
            simcomResult = F_USB_HOST_SIMCOM_HostResutlToSIMCOMResultMap(hostResult);
        }
    }
    
    return (simcomResult);
}

USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_DIAG_Write
(
    USB_HOST_SIMCOM_TRANSFER_HANDLE * transferHandle,
    void * data,
    size_t size
){
    USB_HOST_SIMCOM_INSTANCE_OBJ * simcomInstance;
    USB_HOST_SIMCOM_TRANSFER_HANDLE * tempTransferHandle, localTransferHandle;
    USB_HOST_SIMCOM_RESULT simcomResult = USB_HOST_SIMCOM_RESULT_FAILURE;
    USB_HOST_RESULT hostResult;
    
    simcomInstance = &gUSBHostSIMCOMObj;
    tempTransferHandle = (transferHandle == NULL) ? &localTransferHandle: transferHandle;
    
    if (simcomInstance->state != USB_HOST_SIMCOM_STATE_READY){
        simcomResult = USB_HOST_SIMCOM_RESULT_BUSY;
    }
    else{
        if((size != 0U) && (data == NULL))
        {
            /* Input paramters are not valid */
            simcomResult = USB_HOST_SIMCOM_RESULT_INVALID_PARAMETER;
        }
        else
        {
            hostResult = USB_HOST_DeviceTransfer(simcomInstance->diagInterface.bulkOutPipeHandle, tempTransferHandle, data, size, (uintptr_t)(USB_HOST_SIMCOM_EVENT_DIAG_WRITE_COMPLETE));
            simcomResult = F_USB_HOST_SIMCOM_HostResutlToSIMCOMResultMap(hostResult);
        }
    }
    
    return (simcomResult);
}

USB_HOST_SIMCOM_RESULT USB_HOST_SIMCOM_DIAG_Read
(
    USB_HOST_SIMCOM_TRANSFER_HANDLE * transferHandle,
    void * data,
    size_t size
){
    USB_HOST_SIMCOM_INSTANCE_OBJ * simcomInstance;
    USB_HOST_SIMCOM_TRANSFER_HANDLE * tempTransferHandle, localTransferHandle;
    USB_HOST_SIMCOM_RESULT simcomResult = USB_HOST_SIMCOM_RESULT_FAILURE;
    USB_HOST_RESULT hostResult;
    
    simcomInstance = &gUSBHostSIMCOMObj;
    tempTransferHandle = (transferHandle == NULL) ? &localTransferHandle: transferHandle;
    
    if (simcomInstance->state != USB_HOST_SIMCOM_STATE_READY){
        simcomResult = USB_HOST_SIMCOM_RESULT_BUSY;
    }
    else{
        if((size != 0U) && (data == NULL))
        {
            /* Input paramters are not valid */
            simcomResult = USB_HOST_SIMCOM_RESULT_INVALID_PARAMETER;
        }
        else
        {
            hostResult = USB_HOST_DeviceTransfer(simcomInstance->diagInterface.bulkInPipeHandle, tempTransferHandle, data, size, (uintptr_t)(USB_HOST_SIMCOM_EVENT_DIAG_READ_COMPLETE));
            simcomResult = F_USB_HOST_SIMCOM_HostResutlToSIMCOMResultMap(hostResult);
        }
    }
    
    return (simcomResult);
}

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
int usb_simcom_init(void) {
    
    F_USB_HOST_HostDataInit();
    F_USB_HOST_SIMCOMInit();
    return 0;
}

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
int usb_simcom_tasks(void) {
    
    int i;
    switch (USBHost0Data.state){
        
        case USB_HOST_STATE_INIT:
            if (USB_HOST_Status(sysObj.usbHostObject0) == SYS_STATUS_READY){
                USB_HOST_EventHandlerSet(usb_host_event_handler, 0);  // Asignamos el manejador de eventos
                // Asignamos las funciones del driver genérico
                USB_HOST_GENERIC_Register(F_USB_HOST_SIMCOM_DeviceAssign,
                        F_USB_HOST_SIMCOM_DeviceRelease,
                        F_USB_HOST_SIMCOM_InterfaceAssign,
                        F_USB_HOST_SIMCOM_InterfaceRelease,
                        F_USB_HOST_SIMCOM_DeviceEventHandler,
                        F_USB_HOST_SIMCOM_InterfaceEventHandler);

                USB_HOST_BusEnable(0);                                // habilitamos el bus
                USBHost0Data.state = USB_HOST_STATE_ENABLING_BUS;
            }
            break;
            
        case USB_HOST_STATE_ENABLING_BUS:
            if (USB_HOST_BusIsEnabled(0) == USB_HOST_RESULT_TRUE){ //Indice 0 porque solo tenemos un bus USB
                USBHost0Data.state = USB_HOST_STATE_GETTING_DEVICES;
            }
            break;
        case USB_HOST_STATE_GETTING_DEVICES:
            if (USB_HOST_DeviceGetFirst(0, &USBHost0Data.info[0]) == USB_HOST_RESULT_SUCCESS){
                USBHost0Data.n_devices++;
                i = 1;
                while(USB_HOST_DeviceGetNext(&USBHost0Data.info[i]) == USB_HOST_RESULT_SUCCESS && i < MAX_NUM_DEVICES){
                    i++;
                    USBHost0Data.n_devices++;
                }
                USBHost0Data.state = USB_HOST_STATE_READY;
            }
            else{
                USBHost0Data.state = USB_HOST_STATE_ERROR;
            }
            break;
        case USB_HOST_STATE_ERROR:
            break;
        case USB_HOST_STATE_READY:
            break;
    }
    return 0;
}


/* *****************************************************************************
 End of File
 */
