/* 
 * File:   pdp.c
 * Author: jtrodriguez
 *
 * Created on 9 de marzo de 2021, 11:05
 */

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "pdp.h"
#include "com.h"
#include "simcom.h"
#include "at_responses.h"
#include "at_commands.h"
#include "servers.h"
#include "sockets.h"
#include "timers.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */
#define TIME_FOR_RETRY_MS       20000
#define TIME_FOR_REFRESH_NET    60000
#define NUM_ERRORS_ALOWED       5

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Data types & Definitions                                          */
/* ************************************************************************** */

/* ************************************************************************** */
typedef struct
{
  t_PDP_STATES state;
  uint32_t timeout;
  bool wait;
  bool execNetopen;
  uint32_t nErrs;
} t_PDP_DATA;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Global data & variables                                           */
/* ************************************************************************** */
/* ************************************************************************** */
extern t_SIMCOM_DATA simcomData;
static t_PDP_DATA pdpData;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Callback Functions                                                */
/* ************************************************************************** */
/* ************************************************************************** */
#define SET_PDP_STATE(newState)     pdpData.state=newState

static void pdp_set_wait (uint32_t waitTimeMs)
{
  pdpData.wait = true;
  timer_ms (pdpData.timeout, waitTimeMs);
}

static void cmd_cb (t_COM_COMMAND_CALLBACK_CTX *ctx)
{
  if (ctx->res == COMMAND_RES_OK)
    {
      pdpData.nErrs = 0;
    }
  else
    {
      pdpData.nErrs++;
      pdp_set_wait (TIME_FOR_RETRY_MS);
      SET_PDP_STATE (PDP_STATE_INIT);
      SIMCOM_MSG ("(SIMCOM-PDP) (%d) ERROR!\n", pdpData.nErrs);
    }
}

static void cmd_tcp_sendmode_cb (t_COM_COMMAND_CALLBACK_CTX *ctx)
{
  if (ctx->res == COMMAND_RES_OK)
    {
      SET_PDP_STATE (PDP_STATE_CFG_TCP_TIMEOUTS);
    }
  else
    {
      pdp_set_wait (TIME_FOR_RETRY_MS);
      SIMCOM_MSG ("(SIMCOM-PDP) Config sendmode error!\n");
      SET_PDP_STATE (PDP_STATE_CFG_TCP_SENDMODE);
    }
}

static void cmd_tcp_timeouts_cb (t_COM_COMMAND_CALLBACK_CTX *ctx)
{
  if (ctx->res == COMMAND_RES_OK)
    {
      SET_PDP_STATE (PDP_STATE_CFG_TCP_MODE);
    }
  else
    {
      pdp_set_wait (TIME_FOR_RETRY_MS);
      SIMCOM_MSG ("(SIMCOM-PDP) Config timeouts error!\n");
      SET_PDP_STATE (PDP_STATE_CFG_TCP_TIMEOUTS);
    }
}

static void cmd_tcp_mode_cb (t_COM_COMMAND_CALLBACK_CTX *ctx)
{
  if (ctx->res == COMMAND_RES_OK)
    {
      SET_PDP_STATE (PDP_STATE_OPEN_NET);
    }
  else
    {
      pdp_set_wait (TIME_FOR_RETRY_MS);
      SIMCOM_MSG ("(SIMCOM-PDP) Config timeouts error!\n");
      SET_PDP_STATE (PDP_STATE_CFG_TCP_MODE);
    }
}

static void res_netopen_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  int32_t err;

  if (ctx->nArgs > 0)
    {
      if (pdpData.execNetopen)
        {
          err = atoi (ctx->arg[0]);
          switch (err)
            {
            case US_TCP_ERR_OPERATION_SUCCESS:
              server_service_init ();
              sockets_service_init ();
              simcomData.status.netOpen = true;
              SET_PDP_STATE (PDP_STATE_GET_LOCAL_IP);
              break;
            case US_TCP_ERR_NETWORK_FAILURE:
              SIMCOM_MSG ("(SIMCOM-PDP) NETWORK FAILURE, reset...\n");
              simcom_reset_module ();
              break;
            default:
              SIMCOM_MSG ("(SIMCOM-PDP) NET OPEN FAIL, error: %d\n", err);
              simcomData.status.netOpen = false;
              SET_PDP_STATE (PDP_STATE_INIT);
              break;
            }
        }
      else
        {
          if (*(ctx->arg[0]) == NETOPEN_NET_STATE_CLOSE)
            {
              SIMCOM_MSG ("(SIMCOM-PDP) PDP CLOSE!, prepare to open...\n");
              simcomData.status.netOpen = false;
              SET_PDP_STATE (PDP_STATE_CFG_TCP_SENDMODE);
            }
          else
            {
              if (!simcomData.status.netOpen)
                {
                  server_service_init ();
                  sockets_service_init ();
                }
              SIMCOM_MSG ("(SIMCOM-PDP) NET OPEN.\n");
              simcomData.status.netOpen = true;
              SET_PDP_STATE (PDP_STATE_GET_LOCAL_IP);
            }
        }
    }
}

static void res_ipaddr_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  if (ctx->nArgs > 0)
    {
      SIMCOM_MSG ("(SIMCOM-PDP) Local IP: %s\n", ctx->arg[0]);
      simcomData.status.localIp = iph_str2ip (ctx->arg[0]);
      simcomData.status.localIpUpdated = true;
    }
  SET_PDP_STATE (PDP_STATE_NET_OPENED);
}

static void res_cipevent_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  if (ctx->nArgs > 0)
    {
      SIMCOM_MSG ("(SIMCOM-PDP) CIPEVENT: %s. Reset module...\n", ctx->arg[0]);
      simcom_reset_module ();
    }
}

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */
#define execute_command(command, callBack, context)           if (com_command (command, AT_CMD_STR_LEN(command), callBack, context) == COMMAND_ACEPTED) SET_PDP_STATE (PDP_STATE_WAIT_RESPONSE)

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */
void pdp_time (void)
{
  timer_dec (pdpData.timeout);
  server_service_time ();
  sockets_service_time ();
}

void pdp_init (void)
{
  pdpData.wait = false;
  pdpData.execNetopen = false;
  pdpData.nErrs = 0;
  timer_off (pdpData.timeout);
  response_set_call_back (AT_RES_NET_OPEN, res_netopen_cb);
  response_set_call_back (AT_RES_IPADDR, res_ipaddr_cb);
  response_set_call_back (AT_RES_CIPEVENT, res_cipevent_cb);
  SIMCOM_MSG ("(SIMCOM-PDP) Init...\n");
  SET_PDP_STATE (PDP_STATE_INIT);
}

t_PDP_STATES pdp_tasks (void)
{
  if (pdpData.wait)
    {
      if (timer_expired (pdpData.timeout))
        {
          pdpData.wait = false;
          timer_off (pdpData.timeout);
        }
    }
  else
    {
      switch (pdpData.state)
        {
        case PDP_STATE_INIT:
          if (pdpData.nErrs < NUM_ERRORS_ALOWED)
            {
          simcomData.status.netOpen = false;
          simcomData.status.localIpUpdated = false;
          SET_PDP_STATE (PDP_STATE_TEST_OPEN_NET);
            }
          else
            {
              SIMCOM_MSG ("(SIMCOM-PDP) Max ERROR allowed. Reset...\n");
              simcom_hard_reset_module ();
            }
          break;
        case PDP_STATE_TEST_OPEN_NET:
          pdpData.execNetopen = false;
          execute_command (AT_CMD_READ_NETOPEN, cmd_cb, NULL);
          break;
        case PDP_STATE_CFG_TCP_SENDMODE:
          execute_command (AT_CMD_CIPSENDMODE_1, cmd_tcp_sendmode_cb, NULL);
          break;
        case PDP_STATE_CFG_TCP_TIMEOUTS:
          execute_command (AT_CMD_CIPTIMEOUT, cmd_tcp_timeouts_cb, NULL);
          break;
        case PDP_STATE_CFG_TCP_MODE:
          execute_command (AT_CMD_CIPMODE_0, cmd_tcp_mode_cb, NULL);
          break;
        case PDP_STATE_OPEN_NET:
          pdpData.execNetopen = true;
          execute_command (AT_CMD_EXEC_NETOPEN, cmd_cb, NULL);
          break;
        case PDP_STATE_GET_LOCAL_IP:
          if (simcomData.status.localIpUpdated)
            {
              SET_PDP_STATE (PDP_STATE_NET_OPENED);
            }
          else
            {
              execute_command (AT_CMD_EXEC_IPADDR, cmd_cb, NULL);
            }
          break;
        case PDP_STATE_NET_OPENED:
          pdp_set_wait (TIME_FOR_REFRESH_NET);
          SET_PDP_STATE (PDP_STATE_TEST_OPEN_NET);
          break;
          //
        case PDP_STATE_WAIT_RESPONSE:
          break;
        }
    }
  // Tasks whereas net open
  if (simcomData.status.netOpen)
    {
      server_service_tasks ();
      sockets_service_tasks ();
    }
  return pdpData.state;
}

/* ************************************************************** End of File */
