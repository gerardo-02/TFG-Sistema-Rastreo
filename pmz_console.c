/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <stdlib.h>

#include "system/command/sys_command.h"
#include "bootloader/bootloader_udp.h"

#include "log.h"
#include "drv/pmz_time.h"
#include "usb_A7672E.h"

#include "pmz_console.h"



/* ************************************************************************** */
/* ************************************************************************** */
// Section: CallBack Functions                                                */
/* ************************************************************************** */

/* ************************************************************************** */
static void _cmd_get_time (SYS_CMD_DEVICE_NODE* pCmdIO, int argc, char** argv)
{
  time_t t = pmz_time_utc_to_local (*pmz_time_get ());

  (*pCmdIO->pCmdApi->print)(pCmdIO->cmdIoParam, "\r(UTC)   %s\r", ctime (pmz_time_get ()));
  (*pCmdIO->pCmdApi->print)(pCmdIO->cmdIoParam, "(LOCAL) %s\r", ctime (&t));
}

static void _cmd_set_time (SYS_CMD_DEVICE_NODE* pCmdIO, int argc, char** argv)
{
  struct tm t;
  time_t ts;

  if (argc != 7)
    {
      (*pCmdIO->pCmdApi->print)(pCmdIO->cmdIoParam, "%d: settime DD MM YY hh mm ss (Using UTC time)\r", argc);
      return;
    }
  t.tm_mday = atoi (argv[1]);
  t.tm_mon = atoi (argv[2]) - 1; //mon 0-11
  t.tm_year = atoi (argv[3]) + 100; //+2000 - 1900
  t.tm_hour = atoi (argv[4]);
  t.tm_min = atoi (argv[5]);
  t.tm_sec = atoi (argv[6]);
  ts = mktime (&t);
  pmz_time_set (&ts, true);
  (*pCmdIO->pCmdApi->print)(pCmdIO->cmdIoParam, "Time (UTC) set to: %s\r", ctime (&ts));
}

static void _cmd_update (SYS_CMD_DEVICE_NODE* pCmdIO, int argc, char** argv)
{
  bootloader_SwapAndReset ();
}

// TEMPORAL
static void _cmd_debug (SYS_CMD_DEVICE_NODE* pCmdIO, int argc, char** argv)
{
    /*t_usb_simcom_DATA data = get_usb_data();
    if (data.state == usb_simcom_STATE_READY)
        (*pCmdIO->pCmdApi->print)(pCmdIO->cmdIoParam, "Listing usb bus devices:\n\r");
    else if (data.state == usb_simcom_STATE_INIT)
        (*pCmdIO->pCmdApi->print)(pCmdIO->cmdIoParam, "Bus not initialized yet\n\r");
    else if (data.state == usb_simcom_STATE_GET_DEVICES)
        (*pCmdIO->pCmdApi->print)(pCmdIO->cmdIoParam, "Getting devices\n\r");
    else if (data.state == usb_simcom_STATE_ENABLE)
        (*pCmdIO->pCmdApi->print)(pCmdIO->cmdIoParam, "Enabling bus\n\r");
    
    if (data.result == USB_HOST_RESULT_END_OF_DEVICE_LIST)
        (*pCmdIO->pCmdApi->print)(pCmdIO->cmdIoParam, "No hay dispositivo lol\n\r");
    if (data.result == USB_HOST_RESULT_BUS_UNKNOWN)
        (*pCmdIO->pCmdApi->print)(pCmdIO->cmdIoParam, "Bus desconocido?\n\r");
    if (data.result == USB_HOST_RESULT_BUS_NOT_ENABLED)
        (*pCmdIO->pCmdApi->print)(pCmdIO->cmdIoParam, "Bus no habilitado\n\r");
    if (data.result == USB_HOST_RESULT_PARAMETER_INVALID)
        (*pCmdIO->pCmdApi->print)(pCmdIO->cmdIoParam, "Parametro invalido\n\r");
    if (data.result == USB_HOST_RESULT_FAILURE)
        (*pCmdIO->pCmdApi->print)(pCmdIO->cmdIoParam, "Fallo desconocido\n\r");
    if (data.result == USB_HOST_RESULT_SUCCESS)
        (*pCmdIO->pCmdApi->print)(pCmdIO->cmdIoParam, "OK\n\r");
    
    for (int i=0; i<data.n_devices; i++){
        (*pCmdIO->pCmdApi->print)(pCmdIO->cmdIoParam, "Device %d - Bus: %d\tAdress: %d\n\r", i, data.info[i].bus, data.info[i].deviceAddress);
    }*/
    
    USB_HOST_ShowDescriptors(); // AÑADIDO POR GERARDO
}

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */
static const SYS_CMD_DESCRIPTOR cmdPMZ[] = {
  {"gettime", _cmd_get_time, ": Show system time"},
  {"settime", _cmd_set_time, ": Set system time"},
  {"update", _cmd_update, ": Apply live update and reset"},
  // TEMPORAL
  {"debug", _cmd_debug, ": Debug a certain function"},
};

/* ************************************************************************** */
/* ************************************************************************** */
// Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */


/* ************************************************************************** */
/* ************************************************************************** */
// Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */
int pmz_console_init (void)
{
  if (!SYS_CMD_ADDGRP (cmdPMZ, sizeof (cmdPMZ) / sizeof (*cmdPMZ), "pmz", ": pmz system commands"))
    {
      _l ("(COMMANDS) ERROR!, failed to create pmz commands group\n");
      return 0;
    }
  else
    {
      _l ("(COMMANDS) pmz commands group created\n");
    }

  return 1;
}

/* ************************************************************** End of File */
