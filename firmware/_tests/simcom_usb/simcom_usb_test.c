// *****************************************************************************
// *****************************************************************************
// Section: Included Files 
// *****************************************************************************
// *****************************************************************************
#include <stdbool.h>

#include "configuration.h"
#include "usb/usb_host.h"
#include "usb/usb_host_generic.h"

#include "log.h"
#include "vendor_driver.h"

#include "simcom_usb_test.h"


// *****************************************************************************
// *****************************************************************************
// Section: Type Definitions
// *****************************************************************************
// *****************************************************************************
#define APP_SWITCH_DEBOUNCE_TIME_MS 50

typedef enum
{
  /* Application's state machine's initial state. */
  GEN_STATE_BUS_ENABLE = 0,
  /* TODO: Define states used by the application state machine. */
  APP_STATE_WAIT_FOR_BUS_ENABLE,
  APP_STATE_WAIT_FOR_ATTACH,
  APP_STATE_CHECK_SWITCH_STATE
} APP_STATES_GEN;

typedef enum
{
  APP_SWITCH_STATE_CHECK_SWITCH_PRESS = 0,
  APP_SWITCH_STATE_WAIT_FOR_DEBOUNCE,
  APP_SWITCH_STATE_WAIT_FOR_RELEASE
} APP_SWITCH_STATE;

typedef enum
{
  APP_DEVICE_SWITCH_STATUS_STATE_GET = 0,
  APP_DEVICE_SWITCH_STATUS_STATE_WAIT_GET
} APP_DEVICE_SWITCH_STATUS_STATE;

typedef struct
{
  /* The application's current state */
  APP_STATES_GEN state;
  APP_SWITCH_STATE switchState;
  APP_DEVICE_SWITCH_STATUS_STATE switchStatusTaskState;
  //  SYS_TIME_HANDLE debounceTimer;
  uint8_t appBuffer[512];
  uint8_t oldSwitchStatus;
  bool switchIsPressed;
} APP_DATA_GEN;

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************
// *****************************************************************************
APP_DATA_GEN appDataGen;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Routines
// *****************************************************************************
// *****************************************************************************

USB_HOST_EVENT_RESPONSE APP_USBHostEventHandler (USB_HOST_EVENT event, void * eventData, uintptr_t context)
{
  /* This function is called by the USB Host whenever a USB Host Layer event has occurred. In this example we only handle the device unsupported event */
  switch (event)
    {
    case USB_HOST_EVENT_DEVICE_UNSUPPORTED:
      /* The attached device is not supported for some reason */
      break;
    default:
      break;
    }
  return (USB_HOST_EVENT_RESPONSE_NONE);
}

// *****************************************************************************
// *****************************************************************************
// Section: Local Functions
// *****************************************************************************
// *****************************************************************************

//void APP_SwitchTasks (void)
//{
//  if (!appData.switchIsPressed)
//    {
//      /* We will check the switch press only if the switch was not 
//       * already pressed. */
//      switch (appData.switchState)
//        {
//        case APP_SWITCH_STATE_CHECK_SWITCH_PRESS:
//
//          /* Check if the switch was pressed. */
//          if (SWITCH_Get () == SWITCH_STATE_PRESSED)
//            {
//              /* The switch is pressed. Start the de-bounce timer. If the timer
//               * was not created successfully, we will not advance to the next
//               * state. */
//
//              if (SYS_TIME_DelayMS (APP_SWITCH_DEBOUNCE_TIME_MS, &appData.debounceTimer) == SYS_TIME_SUCCESS)
//                {
//                  appData.switchState = APP_SWITCH_STATE_WAIT_FOR_DEBOUNCE;
//                }
//            }
//          break;
//
//        case APP_SWITCH_STATE_WAIT_FOR_DEBOUNCE:
//
//          /* Here we will check if the de-bounce time is complete */
//          if (SYS_TIME_DelayIsComplete (appData.debounceTimer))
//            {
//              /* Check if the switch is still pressed. If yes, then we
//               * have a valid switch press. */
//              if (SWITCH_Get () == SWITCH_STATE_PRESSED)
//                {
//                  /* Switch press is valid*/
//                  appData.switchState = APP_SWITCH_STATE_WAIT_FOR_RELEASE;
//                }
//              else
//                {
//                  /* This was an invalid switch press */
//                  appData.switchState = APP_SWITCH_STATE_CHECK_SWITCH_PRESS;
//                }
//            }
//          break;
//
//        case APP_SWITCH_STATE_WAIT_FOR_RELEASE:
//          if (SWITCH_Get () == SWITCH_STATE_RELEASED)
//            {
//              appData.switchIsPressed = true;
//              appData.switchState = APP_SWITCH_STATE_CHECK_SWITCH_PRESS;
//            }
//          break;
//
//        default:
//          break;
//        }
//    }
//}
//
//bool APP_SwitchIsPressed (void)
//{
//  bool result = false;
//
//  /* If the switch is pressed, clear the switch pressed flag and
//   * then return true. */
//  if (appData.switchIsPressed == true)
//    {
//      appData.switchIsPressed = false;
//      result = true;
//    }
//
//  return (result);
//}
//

void APP_DeviceSwitchStatusTasks (void)
{
  switch (appDataGen.switchStatusTaskState)
    {
    case APP_DEVICE_SWITCH_STATUS_STATE_GET:
      /* In this state we initiate the device switch status command */
      if (APP_VENDOR_DRIVER_DeviceSwitchStatusGet (appDataGen.appBuffer) == true)
        {
          /* The command was initiated successfully. We can wait for the command to complete. */
          appDataGen.switchStatusTaskState = APP_DEVICE_SWITCH_STATUS_STATE_WAIT_GET;
        }
      break;
    case APP_DEVICE_SWITCH_STATUS_STATE_WAIT_GET:
      /* Here we check if the command has completed */
      //      if (!APP_VENDOR_DRIVER_DeviceSwitchStatusGetInProgress ())
      //        {
      /* This means we have obtained the switch status. If the
       * value if 0x0, then the switch was pressed. If it is 0x1
       * this means the switch is not pressed. We activate the
       * LED accordingly. The data will be available in the second
       * byte of appBuffer */

      //          if (appDataGen.oldSwitchStatus != appDataGen.appBuffer)
      //            {
      //              /* This means there is a change in the device switch status.
      //               * Lets send a message to the console. */
      //              _l ("(SIMCOM USB) Change detected in the device switch status\n");
      //              appDataGen.oldSwitchStatus = appDataGen.appBuffer;
      //            }
      //
      //          if (appDataGen.appBuffer == 0x0)
      //            {
      //              //              _l ("(SIMCOM USB) appDataBuffer = 0\n");
      //              //              LED_On ();
      //            }
      //          else
      //            {
      //              //              _l ("(SIMCOM USB) appDataBuffer != 0\n");
      //              //              LED_Off ();
      //            }
//      if (appDataGen.appBuffer[0] != 0)
//        {
//          _l ("\n(SIMCOM USB) APP-BUFFER: %s\n", appDataGen.appBuffer);
//          appDataGen.appBuffer[0] = 0;
//        }

      /* We can launch the command again. */
      //          appDataGen.switchStatusTaskState = APP_DEVICE_SWITCH_STATUS_STATE_GET;
      //        }

    default:
      break;
    }
}


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

void simcom_usb_init (void)
{
  appDataGen.state = GEN_STATE_BUS_ENABLE;
  //  appDataGen.switchState = APP_SWITCH_STATE_CHECK_SWITCH_PRESS;
  appDataGen.switchStatusTaskState = APP_DEVICE_SWITCH_STATUS_STATE_GET;
  //  appDataGen.switchIsPressed = false;
  //  appDataGen.oldSwitchStatus = 0x1;

  APP_VENDOR_DRIVER_Initialize ();

  _l ("(SIMCOM USB) Initialization Done!\n");
}

void simcom_usb_task (void)
{
  /* Call our Vendor Driver Tasks routine to ensure that the driver functions and states are up to date.*/
  APP_VENDOR_DRIVER_Tasks ();
  if (!APP_VENDOR_DRIVER_DeviceIsAvailable () && (appDataGen.state > APP_STATE_WAIT_FOR_ATTACH))
    {
      /* If the device was attached but is now detached, we will reset the application state to waiting for attach. */
      _l ("(SIMCOM USB) device is not available. Waiting for device to be available \n");
      appDataGen.state = APP_STATE_WAIT_FOR_ATTACH;
    }
  switch (appDataGen.state)
    {
    case GEN_STATE_BUS_ENABLE:
      /* First we assign the generic driver framework support functions.
       * Note that vendor driver initialize function (that we have
       * implemented) has already been invoked at this point in the 
       * APP_Initialize() function before the bus is enabled. This way our
       * driver is ready for device attach. We then register the USB Host 
       * Event Handler and then enabled the USB Host. */
      USB_HOST_GENERIC_Register (APP_VENDOR_DRIVER_DeviceAssign, APP_VENDOR_DRIVER_DeviceRelease, APP_VENDOR_DRIVER_InterfaceAssign, APP_VENDOR_DRIVER_InterfaceRelease, APP_VENDOR_DRIVER_DeviceEventHandler, APP_VENDOR_DRIVER_InterfaceEventHandler);
      USB_HOST_EventHandlerSet (APP_USBHostEventHandler, (uintptr_t) 0);
      _l ("(SIMCOM USB) Enabling USB Host Bus \n");
      USB_HOST_BusEnable (USB_HOST_BUS_ALL);
      appDataGen.state = APP_STATE_WAIT_FOR_BUS_ENABLE;
      break;
    case APP_STATE_WAIT_FOR_BUS_ENABLE:
      if (USB_HOST_BusIsEnabled (USB_HOST_BUS_ALL) == true)
        {
          _l ("(SIMCOM USB) USB Host Bus is enabled. Waiting for device to be available\n");
          appDataGen.state = APP_STATE_WAIT_FOR_ATTACH;
        }
      break;
    case APP_STATE_WAIT_FOR_ATTACH:
      /* Check if the device is attached and available */
      if (APP_VENDOR_DRIVER_DeviceIsAvailable ())
        {
          /* This means device is available. */
          _l ("(SIMCOM USB) Device is available \r\n");
          appDataGen.state = APP_STATE_CHECK_SWITCH_STATE;
        }
      break;
    case APP_STATE_CHECK_SWITCH_STATE:
      //      APP_SwitchTasks ();
      APP_DeviceSwitchStatusTasks ();
      //      if (APP_SwitchIsPressed () && !APP_VENDOR_DRIVER_LedToggleCommandInProgress ())
      //        {
      //          /* This means the switch is pressed and no LED Toggle command is
      //           * in progress. Send the command to the device */
      //          _l ("(SIMCOM USB) Switch was pressed. Sending LED toggle command to the device. \r\n");
      //          APP_VENDOR_DRIVER_ToggleLED ();
      //        }
      break;
      //
    default:
      break;
    }
}

/**************************************************************** End of File */

