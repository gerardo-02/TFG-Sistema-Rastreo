// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <stdbool.h>
#include <stdio.h>

#include "vendor_driver.h"
#include "configuration.h"
#include "log.h"

// *****************************************************************************
// *****************************************************************************
// Section: Type Definitions
// *****************************************************************************
// *****************************************************************************

typedef enum
{
  APP_VENDOR_DRIVER_STATE_WAIT_FOR_ATTACH = 0,
  APP_VENDOR_DRIVER_STATE_SET_CONFIGURATION,
  APP_VENDOR_DRIVER_STATE_WAIT_FOR_SET_CONFIGURATION,
  APP_VENDOR_DRIVER_STATE_WAIT_FOR_INTERFACE_READY,
  APP_VENDOR_DRIVER_STATE_RUNNING,
  APP_VENDOR_DRIVER_STATE_ERROR,
  APP_VENDOR_DRIVER_STATE_ERROR_HOLDING
} APP_VENDOR_DRIVER_STATE;

typedef enum
{
  APP_VENDOR_DEVICE_STATE_DETACHED = 0,
  APP_VENDOR_DEVICE_STATE_ATTACHED,
  APP_VENDOR_DEVICE_STATE_READY
} APP_VENDOR_DEVICE_STATE;

typedef struct
{
  uint8_t buffer[512];
  USB_HOST_DEVICE_CLIENT_HANDLE deviceHandle;
  USB_HOST_DEVICE_OBJ_HANDLE deviceObjHandle;
  USB_HOST_DEVICE_INTERFACE_HANDLE interfaceHandle;
  USB_HOST_PIPE_HANDLE intInPipeHandle;
  USB_HOST_PIPE_HANDLE bulkInPipeHandle;
  USB_HOST_PIPE_HANDLE bulkOutPipeHandle;
  USB_HOST_PIPE_HANDLE controlPipeHandle;
  APP_VENDOR_DRIVER_STATE driverState;
  APP_VENDOR_DEVICE_STATE deviceState;
  bool requestDone;
  bool interfacesProcessed;
  //  uint8_t ledToggleCommand;
  //  uint8_t switchGetCommand;
  uint8_t atCommand[10];
  USB_HOST_TRANSFER_HANDLE curCommandTransferHandle;
  uint8_t intBuffer[64];
  USB_HOST_TRANSFER_HANDLE curIntBufferTransferHandle;
  //  bool switchGetCommandInProgress;
  //  bool switchGetCommmandErrorHadOccurred;
  //  bool ledToggleCommandInProgress;
  USB_HOST_RESULT result;
  //  bool ledToggleCommandErrorHasOccurred;
  uint8_t *appBuffer;
  USB_HOST_TRANSFER_HANDLE curBufferTransferHandle;
} APP_VENDOR_DRIVER_OBJ;

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

USB_ALIGN APP_VENDOR_DRIVER_OBJ vendorDriverObj;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Routines
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
// *****************************************************************************
// Section: Local Functions
// *****************************************************************************
// *****************************************************************************

static void print_descriptor (void *desc, uint8_t descType)
{
  USB_DEVICE_DESCRIPTOR *deviceDescriptor;
  USB_INTERFACE_ASSOCIATION_DESCRIPTOR *interfaceAssociationDescriptor;
  USB_INTERFACE_DESCRIPTOR *interfaceDescriptor;

  switch (descType)
    {
    case USB_DESCRIPTOR_DEVICE:
      deviceDescriptor = (USB_DEVICE_DESCRIPTOR*) desc;
      _l ("\n *** DEVICE DESCRIPTOR ***\n Length: (%hhu)\n Type: %hhu\n bcdUSB: %hu\n bcdDevice: %hu\n Class: %X\n SubClass: %X\n Protocol: %X\n Vendor: %X, Product: %X\n numConfigs: %hhu\n\n"
          , deviceDescriptor->bLength
          , deviceDescriptor->bDescriptorType
          , deviceDescriptor->bcdUSB
          , deviceDescriptor->bcdDevice
          , (int) deviceDescriptor->bDeviceClass
          , (int) deviceDescriptor->bDeviceSubClass
          , (int) deviceDescriptor->bDeviceProtocol
          , (int) deviceDescriptor->idVendor
          , (int) deviceDescriptor->idProduct
          , deviceDescriptor->bNumConfigurations);
      break;
    case USB_DESCRIPTOR_CONFIGURATION:
      _l ("\n *** CONFIGURATION DESCRIPTOR ***\n Not implemented\n\n");
      break;
    case USB_DESCRIPTOR_STRING:
      _l ("\n *** STRING DESCRIPTOR ***\n Not implemented\n\n");
      break;
    case USB_DESCRIPTOR_INTERFACE:
      interfaceDescriptor = (USB_INTERFACE_DESCRIPTOR*) desc;
      _l ("\n *** INTERFACE DESCRIPTOR ***\n Length: (%hhu)\n Type: %hhu\n Num: %hhu \n Class: %X\n SubClass: %X\n Protocol: %X\n EPs: %hhu\n AlternateSetting: %hhu\n\n"
          , interfaceDescriptor->bLength
          , interfaceDescriptor->bDescriptorType
          , interfaceDescriptor->bInterfaceNumber
          , (int) interfaceDescriptor->bInterfaceClass
          , (int) interfaceDescriptor->bInterfaceSubClass
          , (int) interfaceDescriptor->bInterfaceProtocol
          , interfaceDescriptor->bNumEndPoints
          , interfaceDescriptor->bAlternateSetting);
      break;
      break;
    case USB_DESCRIPTOR_ENDPOINT:
      _l ("\n *** ENDPOINT DESCRIPTOR ***\n Not implemented\n\n");
      break;
    case USB_DESCRIPTOR_DEVICE_QUALIFIER:
      _l ("\n *** DEVICE QUALIFIER DESCRIPTOR ***\n Not implemented\n\n");
      break;
    case USB_DESCRIPTOR_OTHER_SPEED:
      _l ("\n *** OTHER SPEED DESCRIPTOR ***\n Not implemented\n\n");
      break;
    case USB_DESCRIPTOR_INTERFACE_POWER:
      _l ("\n *** POWER INTERFACE DESCRIPTOR ***\n Not implemented\n\n");
      break;
    case USB_DESCRIPTOR_OTG:
      _l ("\n *** OTG DESCRIPTOR ***\n Not implemented\n\n");
      break;
    case USB_DESCRIPTOR_INTERFACE_ASSOCIATION:
      interfaceAssociationDescriptor = (USB_INTERFACE_ASSOCIATION_DESCRIPTOR*) desc;
      _l ("\n *** INTERFACE ASSOCIATION DESCRIPTOR ***\n Length: (%hhu)\n Type: %hhu\n Class: %X\n SubClass: %X\n Protocol: %X\n FirstInterface: %hhu, InterfaceCount: %hhu\n\n"
          , interfaceAssociationDescriptor->bLength
          , interfaceAssociationDescriptor->bDescriptorType
          , (int) interfaceAssociationDescriptor->bFunctionClass
          , (int) interfaceAssociationDescriptor->bFunctionSubClass
          , (int) interfaceAssociationDescriptor->bFunctionProtocol
          , interfaceAssociationDescriptor->bFirstInterface
          , interfaceAssociationDescriptor->bInterfaceCount);
      break;
    case USB_DESCRIPTOR_BOS:
      _l ("\n *** BOS DESCRIPTOR ***\n Not implemented\n\n");
      break;
    case USB_DESCRIPTOR_DEVICE_CAPABILITY:
      _l ("\n *** DEVICE CAPABILITY DESCRIPTOR ***\n Not implemented\n\n");
      break;
    default:
      _l ("\n !!! UNKNOWN DESCRIPTOR !!!\n\n");
      break;
    }
}

static bool config_endpoints (USB_HOST_DEVICE_INTERFACE_HANDLE ih, USB_INTERFACE_DESCRIPTOR *id)
{
  bool ret = true;
  USB_HOST_ENDPOINT_DESCRIPTOR_QUERY endpointDescriptorQuery;
  USB_ENDPOINT_DESCRIPTOR * endpointDescriptor;

  /* We know now save the interface handle and the interface descriptor as we 
   * will need this to open the pipe and to communicate with the device. */
  vendorDriverObj.interfaceHandle = ih;
  /* We will use the USB Host provided endpoint descriptor query objects to
   * to get the endpoint descriptors and then open the pipes. To find the 
   * endpoint descriptor, we will use a query. We need to clear the query object
   * context before running the query. In the following query, we are looking 
   * for a Bulk IN endpoint. We will query by type and direction. */
  USB_HOST_DeviceEndpointQueryContextClear (&endpointDescriptorQuery);
  endpointDescriptorQuery.transferType = USB_TRANSFER_TYPE_BULK;
  endpointDescriptorQuery.direction = USB_DATA_DIRECTION_DEVICE_TO_HOST;
  endpointDescriptorQuery.flags = (USB_HOST_ENDPOINT_QUERY_FLAG) (USB_HOST_ENDPOINT_QUERY_BY_TRANSFER_TYPE | USB_HOST_ENDPOINT_QUERY_BY_DIRECTION);
  endpointDescriptor = USB_HOST_DeviceEndpointDescriptorQuery (id, &endpointDescriptorQuery);
  if (endpointDescriptor != NULL)
    {
      vendorDriverObj.bulkInPipeHandle = USB_HOST_DevicePipeOpen (vendorDriverObj.interfaceHandle, endpointDescriptor->bEndpointAddress);
      if (vendorDriverObj.bulkInPipeHandle == USB_HOST_PIPE_HANDLE_INVALID)
        {
          _l ("\n(VENDOR DRIVER) Could not open bulk IN pipe \n");
          ret = false;
        }
    }
  else
    {
      _l ("\n(VENDOR DRIVER) Could not find bulk IN endpoint descriptor \n");
      ret = false;
    }
  /* In this query, we are looking for the Bulk OUT endpoint. */
  USB_HOST_DeviceEndpointQueryContextClear (&endpointDescriptorQuery);
  endpointDescriptorQuery.transferType = USB_TRANSFER_TYPE_BULK;
  endpointDescriptorQuery.direction = USB_DATA_DIRECTION_HOST_TO_DEVICE;
  endpointDescriptorQuery.flags = (USB_HOST_ENDPOINT_QUERY_FLAG) (USB_HOST_ENDPOINT_QUERY_BY_TRANSFER_TYPE | USB_HOST_ENDPOINT_QUERY_BY_DIRECTION);
  endpointDescriptor = USB_HOST_DeviceEndpointDescriptorQuery (id, &endpointDescriptorQuery);
  if (endpointDescriptor != NULL)
    {
      vendorDriverObj.bulkOutPipeHandle = USB_HOST_DevicePipeOpen (vendorDriverObj.interfaceHandle, endpointDescriptor->bEndpointAddress);
      if (vendorDriverObj.bulkOutPipeHandle == USB_HOST_PIPE_HANDLE_INVALID)
        {
          _l ("\n(VENDOR DRIVER) Could not open bulk OUT pipe \n");
          ret = false;
        }
    }
  else
    {
      _l ("\n(VENDOR DRIVER) Could not find OUT endpoint descriptor \n");
      ret = false;
    }
  /* In this query, we are looking for the Interrupt IN endpoint. */
  USB_HOST_DeviceEndpointQueryContextClear (&endpointDescriptorQuery);
  endpointDescriptorQuery.transferType = USB_TRANSFER_TYPE_INTERRUPT;
  endpointDescriptorQuery.direction = USB_DATA_DIRECTION_DEVICE_TO_HOST;
  endpointDescriptorQuery.flags = (USB_HOST_ENDPOINT_QUERY_FLAG) (USB_HOST_ENDPOINT_QUERY_BY_TRANSFER_TYPE | USB_HOST_ENDPOINT_QUERY_BY_DIRECTION);
  endpointDescriptor = USB_HOST_DeviceEndpointDescriptorQuery (id, &endpointDescriptorQuery);
  if (endpointDescriptor != NULL)
    {
      vendorDriverObj.intInPipeHandle = USB_HOST_DevicePipeOpen (vendorDriverObj.interfaceHandle, endpointDescriptor->bEndpointAddress);
      if (vendorDriverObj.intInPipeHandle == USB_HOST_PIPE_HANDLE_INVALID)
        {
          _l ("\n(VENDOR DRIVER) Could not open interrupt IN pipe \n");
          ret = false;
        }
    }
  else
    {
      _l ("\n(VENDOR DRIVER) Could not find interrupt IN endpoint descriptor \n");
      ret = false;
    }

  return ret;
}

// *****************************************************************************
// *****************************************************************************
// Section: Public Functions
// *****************************************************************************
// *****************************************************************************

void APP_VENDOR_DRIVER_Initialize (void)
{
  vendorDriverObj.bulkInPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
  vendorDriverObj.bulkOutPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
  vendorDriverObj.controlPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
  vendorDriverObj.deviceHandle = USB_HOST_DEVICE_CLIENT_HANDLE_INVALID;
  vendorDriverObj.deviceObjHandle = USB_HOST_DEVICE_OBJ_HANDLE_INVALID;
  vendorDriverObj.driverState = APP_VENDOR_DRIVER_STATE_WAIT_FOR_ATTACH;
  vendorDriverObj.deviceState = APP_VENDOR_DEVICE_STATE_DETACHED;
  vendorDriverObj.interfacesProcessed = false;
  //  vendorDriverObj.ledToggleCommand = 0x80;
  //  vendorDriverObj.ledToggleCommandErrorHasOccurred = false;
  //  vendorDriverObj.ledToggleCommandInProgress = false;
  //  vendorDriverObj.switchGetCommand = 0x81;
  //  vendorDriverObj.switchGetCommandInProgress = false;
  //  vendorDriverObj.switchGetCommmandErrorHadOccurred = false;
  _l ("\n>>> (VENDOR DRIVER) Initialization Complete <<<\n");
}

void APP_VENDOR_DRIVER_DeviceAssign (USB_HOST_DEVICE_CLIENT_HANDLE deviceHandle, USB_HOST_DEVICE_OBJ_HANDLE deviceObjHandle, USB_DEVICE_DESCRIPTOR * deviceDescriptor)
{
  _l ("\n--- (VENDOR DRIVER) DEVICE ASSIGN CALLBACK\n");
  /* The USB Host Generic Driver framework will call this function when
   * it the attached device VID PID or Class Subclass Protocol matches the
   * filter criteria. The device descriptor can be inspected further to 
   * verify  if our vendor driver will indeed support the attached device.
   * If the device cannot be supported for any reason, we can return the
   * device to USB Host by calling the USB_HOST_DeviceRelease() function. 
   * The USB Host layer will then try to assign this device to other drivers
   * in the TPL table that could handle this device.*/
  print_descriptor (deviceDescriptor, USB_DESCRIPTOR_DEVICE);

  /* Un-comment the below line if the driver should not handle this device*/
  // USB_HOST_DeviceRelease(deviceHandle);

  /* Lets keep track of the device object and client handles. We will need
   * these to operate the device. Change the state of the device to reflect
   * the attach. This is also a good time to initialize the variables.*/
  vendorDriverObj.deviceState = APP_VENDOR_DEVICE_STATE_ATTACHED;
  vendorDriverObj.deviceObjHandle = deviceObjHandle;
  vendorDriverObj.deviceHandle = deviceHandle;
  vendorDriverObj.bulkInPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
  vendorDriverObj.bulkOutPipeHandle = USB_HOST_PIPE_HANDLE_INVALID;
  vendorDriverObj.driverState = APP_VENDOR_DRIVER_STATE_WAIT_FOR_ATTACH;
  vendorDriverObj.interfacesProcessed = false;
  //  vendorDriverObj.ledToggleCommandErrorHasOccurred = false;
  //  vendorDriverObj.ledToggleCommandInProgress = false;
  //  vendorDriverObj.switchGetCommandInProgress = false;
  //  vendorDriverObj.switchGetCommmandErrorHadOccurred = false;
}

void APP_VENDOR_DRIVER_DeviceRelease (USB_HOST_DEVICE_CLIENT_HANDLE deviceHandle)
{
  _l ("\n--- (VENDOR DRIVER) DEVICE RELEASE CALLBACK\n");
  /* This function will be called first when the device is detached.
   * We will use this function to close the pipes. Because this function
   * is called from an interrupt context, it should be ensure that variables
   * that are updated here are not updated in the device tasks. */

  vendorDriverObj.deviceState = APP_VENDOR_DEVICE_STATE_DETACHED;

  /* Close any open pipes. Don't bother with control pipe. That will be handled by the host layer. */
  USB_HOST_DevicePipeClose (vendorDriverObj.bulkInPipeHandle);
  USB_HOST_DevicePipeClose (vendorDriverObj.bulkOutPipeHandle);

}

USB_HOST_DEVICE_EVENT_RESPONSE APP_VENDOR_DRIVER_DeviceEventHandler (USB_HOST_DEVICE_CLIENT_HANDLE deviceHandle, USB_HOST_DEVICE_EVENT event, void * eventData, uintptr_t context)
{
  USB_HOST_DEVICE_EVENT_CONFIGURATION_SET_DATA * data = NULL;

  _l ("\n--- (VENDOR DRIVER) DEVICE EVENT CALLBACK\n");
  switch (event)
    {
      /* This event occurs when the host layer has configured the device. All
       * client drivers that have opened this device will receive this event. The
       * event is accompanied by the USB_HOST_DEVICE_DATA_CONFIGURATION_SET_DATA
       * type of event data that contains the result of the operation. */
    case USB_HOST_DEVICE_EVENT_CONFIGURATION_SET:
      data = (USB_HOST_DEVICE_EVENT_CONFIGURATION_SET_DATA *) eventData;
      vendorDriverObj.requestDone = true;
      vendorDriverObj.result = data->result;
      _l ("(VENDOR DRIVER) EVENT CONFIGURATION SET. Result = %d\n", (int) data->result);
      break;

      /* This event occurs when a USB_HOST_ConfigurationDescriptorGet() function
       * has completed. This event is accompanied by the
       * USB_HOST_DEVICE_EVENT_CONFIGURATION_DESCRIPTOR_GET_COMPLETE_DATA type of
       * event data that contains the result of the transfer. */
    case USB_HOST_DEVICE_EVENT_CONFIGURATION_DESCRIPTOR_GET_COMPLETE:
      _l ("(VENDOR DRIVER) EVENT CONFIGURATION GET\n");
      break;
    }

  return (USB_HOST_DEVICE_EVENT_RESPONSE_NONE);
}

void APP_VENDOR_DRIVER_InterfaceAssign (USB_HOST_DEVICE_INTERFACE_HANDLE * interfaces, USB_HOST_DEVICE_OBJ_HANDLE deviceObjHandle, size_t nInterfaces, uint8_t * descriptor)
{
  USB_HOST_INTERFACE_DESCRIPTOR_QUERY interfaceDescriptorQuery;
  USB_INTERFACE_DESCRIPTOR * interfaceDescriptor;
  USB_INTERFACE_ASSOCIATION_DESCRIPTOR * interfaceAssociationDescriptor;
  uint8_t i;

  _l ("\n--- (VENDOR DRIVER) INTERFACE ASSIGN (nInterfaces = %d)\n", (int) nInterfaces);
  /* The USB Host Generic Driver Framework will call this function after the
   * the configuration has been set. The function will be called for each
   * interface descriptor or Interface Associate Descriptor in the configuration.
   * In case of IAD, nInterface will be greater than one and interfaces will
   * be an array. */
  print_descriptor (descriptor, ((USB_DESCRIPTOR_HEADER*) descriptor)->descType);
  switch (((USB_DESCRIPTOR_HEADER*) descriptor)->descType)
    {
    case USB_DESCRIPTOR_INTERFACE_ASSOCIATION:
      /* We know now that this is an IAD. */
      interfaceAssociationDescriptor = (USB_INTERFACE_ASSOCIATION_DESCRIPTOR*) descriptor;
      for (i = 0; i < interfaceAssociationDescriptor->bInterfaceCount; i++)
        {
          interfaceDescriptorQuery.context = 0; //Init search from "descriptor" point
          interfaceDescriptorQuery.bInterfaceNumber = interfaceAssociationDescriptor->bFirstInterface + i;
          interfaceDescriptorQuery.flags = USB_HOST_INTERFACE_QUERY_BY_NUMBER;
          interfaceDescriptor = USB_HOST_DeviceGeneralInterfaceDescriptorQuery (descriptor, &interfaceDescriptorQuery);
          interfaceDescriptorQuery.context = (uintptr_t) interfaceDescriptor; //Save new descriptor point for interation search.
          if (interfaceDescriptor)
            {
              print_descriptor (interfaceDescriptor, ((USB_DESCRIPTOR_HEADER*) interfaceDescriptor)->descType);
            }
          else
            {
              _l ("(VENDOR DRIVER) INTERFACE %d, from IAD, NOT found!\n", (int) (interfaceAssociationDescriptor->bFirstInterface + i));
            }
        }
      break;
    case USB_DESCRIPTOR_INTERFACE:
      interfaceDescriptor = (USB_INTERFACE_DESCRIPTOR*) descriptor;
      //---- For test interface 4 AT-PORT
      if (interfaceDescriptor->bInterfaceNumber == 4)
        {
          _l ("(VENDOR DRIVER) INTERFACE FOR TEST\n");
          /* We will let the tasks routine know that the interfaces have been processed. */
          vendorDriverObj.interfacesProcessed = config_endpoints (interfaces[0], interfaceDescriptor);
        }
      //---------------------------
      break;
    default:
      _l ("(VENDOR DRIVER) INTERFACE DESCRIPTOR UNKNOWN\n");
      break;
    }
}

void APP_VENDOR_DRIVER_InterfaceRelease (USB_HOST_DEVICE_INTERFACE_HANDLE interfaceHandle)
{
  _l ("\n\n(VENDOR DRIVER) INTERFACE RELEASE\n");
  /* This function will be called after the device release function is called.
   * If the device release function implements all the detach level clean up
   * the nothing needs to be done here. In a case where the drive is an interface
   * level driver, then the device release function would not be called and only
   * the interface release function would be called. */
}

USB_HOST_DEVICE_INTERFACE_EVENT_RESPONSE APP_VENDOR_DRIVER_InterfaceEventHandler (USB_HOST_DEVICE_INTERFACE_HANDLE interfaceHandle, USB_HOST_DEVICE_INTERFACE_EVENT event, void * eventData, uintptr_t context)
{
  USB_HOST_DEVICE_INTERFACE_EVENT_TRANSFER_COMPLETE_DATA * result;

  _l ("\n\n(VENDOR DRIVER) INTERFACE EVENT (%d)\n", event);
  switch (event)
    {
      /* This event occurs when a Bulk, Isochronous or Interrupt transfer has
       * completed. This event is accompanied by the
       * USB_HOST_DEVICE_INTERFACE_EVENT_TRANSFER_COMPLETE_DATA type of event data
       * that contains the result of the transfer. */
    case USB_HOST_DEVICE_INTERFACE_EVENT_TRANSFER_COMPLETE:
      /* We get the event related data by type casting the eventData parameter. */
      result = (USB_HOST_DEVICE_INTERFACE_EVENT_TRANSFER_COMPLETE_DATA *) (eventData);
      _l ("\n\n(VENDOR DRIVER) Transfer completed. Handle = %d, length = %d, result = %d\n", (int) (result->transferHandle), (int) (result->length), (int) (result->result));
      if (result->transferHandle == vendorDriverObj.curCommandTransferHandle)
        {
          vendorDriverObj.curCommandTransferHandle = USB_HOST_TRANSFER_HANDLE_INVALID;
          _l ("\n\n(VENDOR DRIVER) Command Sent.\n");
        }
      if (result->transferHandle == vendorDriverObj.curBufferTransferHandle)
        {
          vendorDriverObj.appBuffer[result->length] = 0;
          _l ("\n\n(VENDOR DRIVER) Data Received: %s\n", (char*) vendorDriverObj.appBuffer);
          if (USB_HOST_DeviceTransfer (vendorDriverObj.bulkInPipeHandle, &vendorDriverObj.curBufferTransferHandle, vendorDriverObj.appBuffer, 512, 0) == USB_HOST_RESULT_SUCCESS)
            {
              _l ("\n(VENDOR DRIVER) Ready for receiving data again. Handle = %d\n", (int) vendorDriverObj.curBufferTransferHandle);
            }
        }
      if (result->transferHandle == vendorDriverObj.curIntBufferTransferHandle)
        {
          vendorDriverObj.intBuffer[result->length] = 0;
          _l ("\n\n(VENDOR DRIVER) Interrupt Data Received: %s\n", (char*) vendorDriverObj.intBuffer);
          if (USB_HOST_DeviceTransfer (vendorDriverObj.intInPipeHandle, &vendorDriverObj.curIntBufferTransferHandle, vendorDriverObj.intBuffer, sizeof(vendorDriverObj.intBuffer), 0) == USB_HOST_RESULT_SUCCESS)
            {
              _l ("\n(VENDOR DRIVER) Ready for receiving interrupt data again. Handle = %d\n", (int) vendorDriverObj.curIntBufferTransferHandle);
            }
        }
      /* Now we can use the context parameter to check if the transfer was related to the LED command. */
      //      if (0)//(uint8_t) context == vendorDriverObj.ledToggleCommand)
      //        {
      //          //          if (result->result != USB_HOST_RESULT_SUCCESS)
      //          //            {
      //          //              vendorDriverObj.ledToggleCommandErrorHasOccurred = true;
      //          //            }
      //          //
      //          //          /* Update the flag to reflect the completion.*/
      //          //          vendorDriverObj.ledToggleCommandInProgress = false;
      //        }
      //      else if ((uint8_t) context == vendorDriverObj.switchGetCommand)
      //        {
      //          /* This is the first of the switch status get command. */
      //          if (result->result != USB_HOST_RESULT_SUCCESS)
      //            {
      //              vendorDriverObj.switchGetCommmandErrorHadOccurred = true;
      //            }
      //        }
      //      else if ((uint8_t) context == (vendorDriverObj.switchGetCommand + 1))
      //        {
      //          /* This is the second of the switch status get command. */
      //          if (result->result != USB_HOST_RESULT_SUCCESS)
      //            {
      //              /* An error has occurred. */
      //              vendorDriverObj.switchGetCommmandErrorHadOccurred = true;
      //            }
      //
      //          /* This is the end of the switch status get command. The
      //           * switch status is available in the second element of 
      //           * the array. */
      //          *vendorDriverObj.appBuffer = vendorDriverObj.buffer[1];
      //          vendorDriverObj.switchGetCommandInProgress = false;
      //        }
      break;
      /* This event occurs when a alternate setting request has completed. This
       * event is accompanied by the
       * USB_HOST_DEVICE_INTERFACE_EVENT_SET_INTERFACE_COMPLETE_DATA type of event
       * data that contains the result of the transfer. */
    case USB_HOST_DEVICE_INTERFACE_EVENT_SET_INTERFACE_COMPLETE:
      break;
      /* This event occurs when a endpoint halt clear request has completed. This
       * event is accompanied by the
       * USB_HOST_DEVICE_INTERFACE_EVENT_PIPE_HALT_CLEAR_COMPLETE_DATA type of
       * event data that contains the result of the transfer. */
    case USB_HOST_DEVICE_INTERFACE_EVENT_PIPE_HALT_CLEAR_COMPLETE:
      break;
    }

  return (USB_HOST_DEVICE_INTERFACE_EVENT_RESPONSE_NONE);
}

void APP_VENDOR_DRIVER_Tasks (void)
{
  USB_HOST_RESULT result;

  /* If at all the device gets detached, we should reset the state machine to wait for attach. */
  if ((vendorDriverObj.deviceState == APP_VENDOR_DEVICE_STATE_DETACHED) && (vendorDriverObj.driverState > APP_VENDOR_DRIVER_STATE_WAIT_FOR_ATTACH))
    {
      vendorDriverObj.driverState = APP_VENDOR_DRIVER_STATE_WAIT_FOR_ATTACH;
      _l ("\n(VENDOR DRIVER) Device was detached. APP Vendor Driver: Waiting for device attach. \n");
    }
  //  if (vendorDriverObj.ledToggleCommandErrorHasOccurred)
  //    {
  //      _l ("\n(VENDOR DRIVER) An error while executing the LED Toggle command. \n");
  //      vendorDriverObj.ledToggleCommandErrorHasOccurred = false;
  //    }
  //
  //  if (vendorDriverObj.switchGetCommmandErrorHadOccurred)
  //    {
  //      _l ("\n(VENDOR DRIVER) An error while executing the Switch Status Get command. \n");
  //      vendorDriverObj.switchGetCommmandErrorHadOccurred = false;
  //    }

  switch (vendorDriverObj.driverState)
    {
    case APP_VENDOR_DRIVER_STATE_WAIT_FOR_ATTACH:
      /* In this state, we check if the device was attached. If the device
       * was attached, we can open the control pipe and move to the state
       * where we set the device configuration. */
      if (vendorDriverObj.deviceState == APP_VENDOR_DEVICE_STATE_ATTACHED)
        {
          /* We will keep things simple and will not check if the control
           * pipe was obtained successfully. The value can be compared to
           * USB_HOST_PIPE_HANDLE_INVALID and if so, the driver can release
           * device. */
          vendorDriverObj.controlPipeHandle = USB_HOST_DeviceControlPipeOpen (vendorDriverObj.deviceObjHandle);
          vendorDriverObj.driverState = APP_VENDOR_DRIVER_STATE_SET_CONFIGURATION;
          _l ("\n(VENDOR DRIVER) Device Attached \n");
        }
      break;
    case APP_VENDOR_DRIVER_STATE_SET_CONFIGURATION:
      /* In this state we will set the device configuration. There are two
       * approaches that the driver can take. The first approach is where
       * we already know which configuration to set because presumably we
       * have developed the attached device. The other approach is where
       * USB_HOST_DeviceConfigurationGet() function to get the 
       * configuration, analyze it and then set the configuration. In
       * this case, we know the index of the device configuration to be set.
       */
      vendorDriverObj.requestDone = false;
      result = USB_HOST_DeviceConfigurationSet (vendorDriverObj.deviceHandle, NULL, 0x0, 0);
      _l ("\n(VENDOR DRIVER) Trying to Set Configuration 1. Result=%d\n", result);
      if (result == USB_HOST_RESULT_REQUEST_BUSY)
        {
          /* The device is not ready. We will stay in this state */
        }
      else if (result == USB_HOST_RESULT_SUCCESS)
        {
          /* Now we will wait for the configuration set to be complete. */
          vendorDriverObj.driverState = APP_VENDOR_DRIVER_STATE_WAIT_FOR_SET_CONFIGURATION;
        }
      else
        {
          /* If the configuration set request was not successful,
           * we cannot proceed. Lets just wait for the device to 
           * be detached. */
          vendorDriverObj.driverState = APP_VENDOR_DRIVER_STATE_ERROR;
          _l ("\n(VENDOR DRIVER) Error while trying to Set Configuration 1.\n");
        }
      break;
    case APP_VENDOR_DRIVER_STATE_WAIT_FOR_SET_CONFIGURATION:

      /* The requestDone flag will be set to true in the USB_HOST_DEVICE_EVENT_CONFIGURATION_SET event handling section. */
      if (vendorDriverObj.requestDone == true)
        {
          /* This means the configuration set request is complete. Now we check if the request was successful. */
          if (vendorDriverObj.result != USB_HOST_RESULT_SUCCESS)
            {
              /* This means the configuration could not be set. We will place the driver in an error state. */
              _l ("\n(VENDOR DRIVER) Error while trying to Set Configuration 1. Result=%d\n", vendorDriverObj.result);
              vendorDriverObj.driverState = APP_VENDOR_DRIVER_STATE_ERROR;
            }
          else
            {
              _l ("\n(VENDOR DRIVER) Set Configuration complete \n");
              vendorDriverObj.driverState = APP_VENDOR_DRIVER_STATE_WAIT_FOR_INTERFACE_READY;
            }
        }
      break;
    case APP_VENDOR_DRIVER_STATE_WAIT_FOR_INTERFACE_READY:
      /* The interfaces are processed and communication pipes are opened
       * in the APP_VENDOR_DRIVER_InterfaceAssign() function. In the task
       * routine we will check if the pipes were opened successfully. */
      if (vendorDriverObj.interfacesProcessed == true)
        {
          if ((vendorDriverObj.bulkInPipeHandle != USB_HOST_PIPE_HANDLE_INVALID) && (vendorDriverObj.bulkOutPipeHandle != USB_HOST_PIPE_HANDLE_INVALID)&& (vendorDriverObj.intInPipeHandle != USB_HOST_PIPE_HANDLE_INVALID))
            {
              if (USB_HOST_DeviceTransfer (vendorDriverObj.intInPipeHandle, &vendorDriverObj.curIntBufferTransferHandle, vendorDriverObj.intBuffer, sizeof (vendorDriverObj.intBuffer), 0) == USB_HOST_RESULT_SUCCESS)
                {
                  _l ("\n(VENDOR DRIVER) Ready for receiving interrupt data. Handle = %d\n", (int) vendorDriverObj.curIntBufferTransferHandle);
                }
              /* This means that both the pipes were opened successfully. The device is now ready for use */
              _l ("\n(VENDOR DRIVER) All pipes are open. Device is ready \n");
              vendorDriverObj.driverState = APP_VENDOR_DRIVER_STATE_RUNNING;
            }
          else
            {
              /* This means one or both pipes could not be opened. We will place the driver in an error state. */
              _l ("\n(VENDOR DRIVER) Could not open pipes. Driver is in error state \n");
              vendorDriverObj.driverState = APP_VENDOR_DRIVER_STATE_ERROR;
            }
        }
      break;
    case APP_VENDOR_DRIVER_STATE_RUNNING:
      break;
    case APP_VENDOR_DRIVER_STATE_ERROR:
      break;
    case APP_VENDOR_DRIVER_STATE_ERROR_HOLDING:
      break;
    default:
      break;
    }
}

bool APP_VENDOR_DRIVER_DeviceIsAvailable (void)
{
  /* This function will return true if the device is ready to operated*/
  bool result = false;

  if ((vendorDriverObj.driverState == APP_VENDOR_DRIVER_STATE_RUNNING) && (vendorDriverObj.deviceState == APP_VENDOR_DEVICE_STATE_ATTACHED))
    {
      result = true;
    }

  return (result);
}

//bool APP_VENDOR_DRIVER_ToggleLED (void)
//{
//  /* This will function will toggle the LED on device. The toggle LED
//   * command value is 0x80. The function will use the USB_HOST_DeviceTransfer()
//   * function to write data to the device. */
//
//  USB_HOST_TRANSFER_HANDLE tempTransferHandle;
//  bool result = true;
//
//  /* Note how we specify the command value as the context in the device transfer
//   * function. This will allow us to easily identify the source transfer
//   * in the transfer done event. The ledToggleCommandDone flag will be updated
//   * in the event.*/
//
//  if (vendorDriverObj.ledToggleCommandInProgress == false)
//    {
//      /* We will process the command only if the command is not already in
//       * progress. */
//
//      vendorDriverObj.ledToggleCommandInProgress = true;
//
//      if (USB_HOST_DeviceTransfer (vendorDriverObj.bulkOutPipeHandle, &tempTransferHandle, &vendorDriverObj.ledToggleCommand, 1, (uintptr_t) (vendorDriverObj.ledToggleCommand)) != USB_HOST_RESULT_SUCCESS)
//        {
//          /* This means the transfer could not be scheduled for some reason.*/
//          vendorDriverObj.ledToggleCommandInProgress = false;
//          result = false;
//        }
//    }
//
//  return (result);
//}

//bool APP_VENDOR_DRIVER_LedToggleCommandInProgress (void)
//{
//  /* Return the current LED Toggle Command Progress. */
//  return (vendorDriverObj.ledToggleCommandInProgress);
//}

bool APP_VENDOR_DRIVER_DeviceSwitchStatusGet (uint8_t * appBuffer)
{
  bool result = false;

  /* The switch status get operation required two transfers to be scheduled. 
   * We can use the queuing feature of the host layer to queue transactions.*/
  //  if (vendorDriverObj.switchGetCommandInProgress == false)
  //    {
  //      vendorDriverObj.switchGetCommandInProgress = true;
  /* The context value of the second command is the actual command + 1*/
  vendorDriverObj.appBuffer = appBuffer;
  //if (USB_HOST_DeviceTransfer (vendorDriverObj.bulkInPipeHandle, &tempTransferHandle, vendorDriverObj.buffer, 512, 0) == USB_HOST_RESULT_SUCCESS)
  if (USB_HOST_DeviceTransfer (vendorDriverObj.bulkInPipeHandle, &vendorDriverObj.curBufferTransferHandle, vendorDriverObj.appBuffer, 512, 0) == USB_HOST_RESULT_SUCCESS)
    {
      _l ("\n(VENDOR DRIVER) Ready for receiving data. Handle = %d\n", (int) vendorDriverObj.curBufferTransferHandle);
      result = true;
    }
  /* The context value of the first command is the actual command */
  snprintf ((char*) vendorDriverObj.atCommand, sizeof (vendorDriverObj.atCommand), "at\r");
  if (USB_HOST_DeviceTransfer (vendorDriverObj.bulkOutPipeHandle, &vendorDriverObj.curCommandTransferHandle, vendorDriverObj.atCommand, 4, 0) == USB_HOST_RESULT_SUCCESS)
    {
      _l ("\n(VENDOR DRIVER) Sending AT. Handle = %d\n", (int) vendorDriverObj.curCommandTransferHandle);
      result = true;
    }
  //      if (result == false)
  //        {
  //          /* If either of the command could not be transferred, then command is not in progress */
  //          vendorDriverObj.switchGetCommandInProgress = false;
  //        }
  //    }

  return (result);
}

//bool APP_VENDOR_DRIVER_DeviceSwitchStatusGetInProgress (void)
//{
//  /* Return the status of the switch get command */
//  return (vendorDriverObj.switchGetCommandInProgress);
//}
