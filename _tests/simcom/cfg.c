/* 
 * File:   cfg.c
 * Author: jtrodriguez
 *
 * Created on 1 de marzo de 2021, 9:01
 */

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "cfg.h"
#include "com.h"
#include "at_commands.h"
#include "simcom.h"
#include "at_responses.h"
#include "timers.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */
#define TIME_FOR_RETRY_MS       5000

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Data types & Definitions                                          */
/* ************************************************************************** */

/* ************************************************************************** */
typedef struct
{
  t_CFG_STATES state;
  t_CFG_STATES currentConfigState; //Por donde va de la config
  uint32_t timeout;
  bool wait;
} t_CFG_DATA;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Global data & variables                                           */
/* ************************************************************************** */
/* ************************************************************************** */
extern t_SIMCOM_DATA simcomData;
t_CFG_DATA cfgData;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Callback Functions                                                */
/* ************************************************************************** */
/* ************************************************************************** */
#define SET_CFG_STATE(newState)     cfgData.state=newState

static void cfg_set_wait (uint32_t waitTimeMs)
{
  cfgData.wait = true;
  timer_ms (cfgData.timeout, waitTimeMs);
}

static void cmd_cb (t_COM_COMMAND_CALLBACK_CTX *ctx)
{
  if (ctx->res == COMMAND_RES_OK)
    {
      SIMCOM_MSG ("(SIMCOM-CFG) OK, (%d)\n", cfgData.currentConfigState);
      SET_CFG_STATE (cfgData.currentConfigState + 1); //Pasa a la siguiente config... 
    }
  else
    {
      cfg_set_wait (TIME_FOR_RETRY_MS);
      SIMCOM_MSG ("(SIMCOM-CFG) ERROR, (%d). Retry...\n", cfgData.currentConfigState);
      SET_CFG_STATE (cfgData.currentConfigState);
    }
}

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Local Functions                                                   */
/* ************************************************************************** */

/* ************************************************************************** */
static inline void execute_command (char *command, COM_COMMAND_CALLBACK cb)
{
  if (com_command (command, strlen (command), cb, NULL) == COMMAND_ACEPTED)
    {
      cfgData.currentConfigState = cfgData.state;
      SET_CFG_STATE (CFG_STATE_WAIT_COMMAND_RESPONSE);
    }
}

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */
void cfg_time (void)
{
  timer_dec (cfgData.timeout);
}

void cfg_init (void)
{
  cfgData.wait = false;
  timer_off (cfgData.timeout);
  SIMCOM_MSG ("(SIMCOM-CFG) Init...\n");
  SET_CFG_STATE (CFG_STATE_INIT);
}

t_CFG_STATES cfg_tasks (void)
{
  char cmdAux[SIMCOM_MAX_APN_LENGTH + 100];

  if (cfgData.wait)
    {
      if (timer_expired (cfgData.timeout))
        {
          cfgData.wait = false;
          timer_off (cfgData.timeout);
        }
    }
  else
    {
      switch (cfgData.state)
        {
        case CFG_STATE_INIT:
          SET_CFG_STATE (CFG_STATE_AUTOCSQ);
          break;
          //
        case CFG_STATE_AUTOCSQ:
          execute_command (AT_CMD_AUTOCSQ_ENABLE, cmd_cb);
          break;
        case CFG_STATE_CMEE:
          execute_command (AT_CMD_CMEE_ENABLE, cmd_cb);
          break;
        case CFG_STATE_CNMP_AUTO:
          execute_command (AT_CMD_CNMP_SET_AUTO, cmd_cb);
          break;
        case CFG_STATE_CPSI:
          execute_command (AT_CMD_READ_CPSI, cmd_cb);
          break;
        case CFG_STATE_CNSMOD:
          execute_command (AT_CMD_CNSMOD_ENABLE_AUTO_REPORT, cmd_cb);
          break;
        case CFG_STATE_CTZU:
          execute_command (AT_CMD_CTZU_ENABLE, cmd_cb);
          break;
        case CFG_STATE_CTZR:
          execute_command (AT_CMD_CTZR_ON, cmd_cb);
          break;
        case CFG_STATE_CGEREP:
          execute_command (AT_CMD_CGEREP_2_0, cmd_cb);
          break;
        case CFG_STATE_APN:
          if (simcomData.cfg.apn[0] != 0)
            {
              snprintf (cmdAux, sizeof (cmdAux), "%s=1,\"IP\",\"%s\"\r", AT_DEFINE_SOCKET_PDP_CONTEXT, simcomData.cfg.apn);
              execute_command (cmdAux, cmd_cb);
            }
          else
            {
              // No hay APN definido, hemos terminado!
              SET_CFG_STATE (CFG_STATE_OK);
            }
          break;
        case CFG_STATE_SELECT_APN:
          execute_command (AT_CMD_SELECT_PDP_1, cmd_cb);
          break;
        case CFG_STATE_CIPCCFG:
          execute_command (AT_CMD_CIPCCFG, cmd_cb);
          break;
        case CFG_STATE_IPHEAD:
          execute_command (AT_CMD_CIPHEAD_MODE_1, cmd_cb);
          break;
        case CFG_STATE_IPSRIP:
          execute_command (AT_CMD_CIPSRIP_MODE_0, cmd_cb);
          break;
        case CFG_STATE_CGPSINFO:
          snprintf (cmdAux, sizeof (cmdAux), "%s=0\r", AT_GET_GPS_FIXED_POSITION_INFO);
          execute_command (cmdAux, cmd_cb);
          break;
          //
        case CFG_STATE_OK:
          break;
          //
        case CFG_STATE_WAIT_COMMAND_RESPONSE:
          break;
        }
    }

  return cfgData.state;
}

/* ************************************************************** End of File */
