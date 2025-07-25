/* 
 * File:   servers.c
 * Author: jtrodriguez
 *
 * Created on 10 de marzo de 2021, 8:45
 */

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "servers.h"
#include "com.h"
#include "simcom.h"
#include "at_responses.h"
#include "at_commands.h"
#include "timers.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */
#define TIME_FOR_RETRY_MS       10000
#define AUX_STRING_MAX_LENGTH   32

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Data types & Definitions                                          */
/* ************************************************************************** */

/* ************************************************************************** */
typedef enum
{
  SERVER_STATE_CHECK = 0,
  SERVER_STATE_PREPARE_OPEN,
  SERVER_STATE_OPEN,
  SERVER_STATE_PREPARE_CLOSE,
  SERVER_STATE_CLOSE,
  //
  SERVER_STATE_WAIT_RESPONSE,
} t_SERVER_STATES;

typedef struct
{
  t_SERVER_STATES state;
  t_SERVER_STATES savedState;
  uint32_t timeout;
  bool wait;
  uint32_t idForCmd;
  char cmdAux[AUX_STRING_MAX_LENGTH];
} t_SERVER_DATA;

typedef struct
{
  t_SERVER_SERVICE_STATES state;
  t_SERVER_SERVICE_STATES savedState;
  uint32_t timeout;
  bool wait;
} t_SERVER_SERVICE_DATA;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Global data & variables                                           */
/* ************************************************************************** */
/* ************************************************************************** */
extern t_SIMCOM_DATA simcomData;
static t_SERVER_SERVICE_DATA serverServiceData;
static t_SERVER_DATA server[SIMCOM_MAX_TCP_SERVERS];

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Callback Functions                                                */
/* ************************************************************************** */
/* ************************************************************************** */

// <editor-fold defaultstate="collapsed" desc="SERVERS">
#define SET_SERVER_STATE(index,newState)     server[index].state=newState

static void server_set_wait (uint32_t index, uint32_t waitTimeMs)
{
  server[index].wait = true;
  timer_ms (server[index].timeout, waitTimeMs);
}

static void cmd_open_cb (t_COM_COMMAND_CALLBACK_CTX *ctx)
{
  t_SERVER_DATA *sd = (t_SERVER_DATA*) (ctx->local);

  if (ctx->res == COMMAND_RES_OK)
    {
      simcomData.status.server[sd->idForCmd].state = SIMCOM_SERVER_STATE_LISTEN;
      simcomData.status.server[sd->idForCmd].port = simcomData.cfg.serverPort[sd->idForCmd];
      SIMCOM_MSG ("(SIMCOM-SERVER)(%d) Listen on port %d\n", sd->idForCmd, simcomData.status.server[sd->idForCmd].port);
    }
  else
    {
      switch (ctx->err)
        {
        case US_TCP_ERR_SERVER_ALREADY_LISTEN:
          SIMCOM_MSG ("(SIMCOM-SERVER)(%d) OPEN ERROR, ALREADY OPEN! --> FAIL...\n", sd->idForCmd);
          simcomData.status.server[sd->idForCmd].state = SIMCOM_SERVER_STATE_FAIL;
          break;
        default:
          server_set_wait (sd->idForCmd, 2000);
          SIMCOM_MSG ("(SIMCOM-SERVER)(%d) OPEN ERROR! (err=%d), wait=2s...\n", sd->idForCmd, ctx->err);
          break;
        }
    }
  SET_SERVER_STATE (sd->idForCmd, SERVER_STATE_CHECK);
}

static void cmd_close_cb (t_COM_COMMAND_CALLBACK_CTX *ctx)
{
  t_SERVER_DATA *sd = (t_SERVER_DATA*) (ctx->local);

  if (ctx->res == COMMAND_RES_OK)
    {
      simcomData.status.server[sd->idForCmd].state = SIMCOM_SERVER_STATE_CLOSE;
      SIMCOM_MSG ("(SIMCOM-SERVER)(%d) Close\n", sd->idForCmd);
    }
  else
    {
      server_set_wait (sd->idForCmd, TIME_FOR_RETRY_MS);
      SIMCOM_MSG ("(SIMCOM-SERVER)(%d) CLOSE ERROR!\n", sd->idForCmd);
    }
  SET_SERVER_STATE (sd->idForCmd, SERVER_STATE_CHECK);
}

static void res_server_stop_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  int32_t index, err;
  if (ctx->nArgs > 1)
    {
      index = atoi (ctx->arg[0]);
      err = atoi (ctx->arg[1]);
      if (err != US_TCP_ERR_OPERATION_SUCCESS)
        {
          SIMCOM_MSG ("(SIMCOM-SERVER)(%d) Error (%d) ServerStop! Close\n", index, err);
        }
      simcomData.status.server[index].state = SIMCOM_SERVER_STATE_CLOSE;
    }
}
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="SERVER SERVICE">

static void res_server_status_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  int32_t index;

  if (ctx->nArgs > 1)
    {
      index = atoi (ctx->arg[0]);
      simcomData.status.server[index].state = SIMCOM_SERVER_STATE_LISTEN;
      simcomData.status.server[index].port = atoi (ctx->arg[1]);
      SIMCOM_MSG ("(SIMCOM-SERVER-SERVICE) Server (%d) listen on port %d\n", index, simcomData.status.server[index].port);
    }
}

static void cmd_get_server_service_status_cb (t_COM_COMMAND_CALLBACK_CTX *ctx)
{
  if (ctx->res == COMMAND_RES_OK)
    {
      serverServiceData.state = SERVER_SERVICE_STATE_RUNNING;
    }
  else
    {
      serverServiceData.wait = true;
      timer_ms (serverServiceData.timeout, TIME_FOR_RETRY_MS);
      serverServiceData.state = SERVER_SERVICE_STATE_GET_STATUS;
      SIMCOM_MSG ("(SIMCOM-SERVER-SERVICE) ERROR! %d\n", serverServiceData.savedState);
    }
}
// </editor-fold>

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Local Functions                                                   */
/* ************************************************************************** */

/* ************************************************************************** */
static void server_execute_command (uint32_t index, char *command, COM_COMMAND_CALLBACK cb, void* localCtx)
{
  if (com_command (command, strlen (command), cb, localCtx) == COMMAND_ACEPTED)
    {
      server[index].savedState = server[index].state;
      SET_SERVER_STATE (index, SERVER_STATE_WAIT_RESPONSE);
    }
}

static void server_run (uint32_t index)
{
  if (server[index].wait)
    {
      if (timer_expired (server[index].timeout))
        {
          server[index].wait = false;
          timer_off (server[index].timeout);
        }
    }
  else
    {
      switch (server[index].state)
        {
        case SERVER_STATE_CHECK:
          switch (simcomData.status.server[index].state)
            {
            case SIMCOM_SERVER_STATE_CLOSE:
              if (simcomData.cfg.serverPort[index] != 0)
                {
                  SIMCOM_MSG ("(SIMCOM-SERVER)(%d) Init...\n", index);
                  SET_SERVER_STATE (index, SERVER_STATE_PREPARE_OPEN);
                }
              break;
            case SIMCOM_SERVER_STATE_LISTEN:
              if (simcomData.cfg.serverPort[index] == 0)
                {
                  SIMCOM_MSG ("(SIMCOM-SERVER)(%d) closing...\n", index);
                  SET_SERVER_STATE (index, SERVER_STATE_PREPARE_CLOSE);
                }
              else if (simcomData.cfg.serverPort[index] != simcomData.status.server[index].port)
                {
                  SIMCOM_MSG ("(SIMCOM-SERVER)(%d) port update, closing...\n", index);
                  SET_SERVER_STATE (index, SERVER_STATE_PREPARE_CLOSE);
                }
              break;
            case SIMCOM_SERVER_STATE_FAIL:
              SIMCOM_MSG ("(SIMCOM-SERVER)(%d) Fail!\n", index);
              SET_SERVER_STATE (index, SERVER_STATE_PREPARE_CLOSE);
              break;
            }
          break;
        case SERVER_STATE_PREPARE_OPEN:
          snprintf (server[index].cmdAux, AUX_STRING_MAX_LENGTH, "%s=%d,%d\r", AT_STARTUP_TCP_SERVER, simcomData.cfg.serverPort[index], index);
          server[index].idForCmd = index;
          SET_SERVER_STATE (index, SERVER_STATE_OPEN);
          break;
        case SERVER_STATE_OPEN:
          server_execute_command (index, server[index].cmdAux, cmd_open_cb, &server[index]);
          break;
        case SERVER_STATE_PREPARE_CLOSE:
          snprintf (server[index].cmdAux, AUX_STRING_MAX_LENGTH, "%s=%d\r", AT_STOP_TCP_SERVER, index);
          server[index].idForCmd = index;
          SET_SERVER_STATE (index, SERVER_STATE_CLOSE);
          break;
        case SERVER_STATE_CLOSE:
          server_execute_command (index, server[index].cmdAux, cmd_close_cb, &server[index]);
          break;
          //
        case SERVER_STATE_WAIT_RESPONSE:
          break;
        }
    }
}

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */
void server_service_time (void)
{
  uint32_t si;

  for (si = 0; si < SIMCOM_MAX_TCP_SERVERS; si++)
    {
      timer_dec (server[si].timeout);
    }
  timer_dec (serverServiceData.timeout);
}

void server_service_init (void)
{
  memset (&serverServiceData, 0, sizeof (t_SERVER_SERVICE_DATA));
  //Register call-back
  response_set_call_back (AT_RES_SERVERSTART, res_server_status_cb);
  response_set_call_back (AT_RES_SERVERSTOP, res_server_stop_cb);
  SIMCOM_MSG ("(SIMCOM-SERVERS) Init...\n");
  serverServiceData.state = SERVER_SERVICE_STATE_INIT;
}

t_SERVER_SERVICE_STATES server_service_tasks (void)
{
  static uint32_t csi; // Current server index

  if (serverServiceData.wait)
    {
      if (timer_expired (serverServiceData.timeout))
        {
          serverServiceData.wait = false;
          timer_off (serverServiceData.timeout);
        }
    }
  else
    {
      switch (serverServiceData.state)
        {
        case SERVER_SERVICE_STATE_INIT:
          for (csi = 0; csi < SIMCOM_MAX_TCP_SERVERS; csi++)
            {
              memset (&server[csi], 0, sizeof (t_SERVER_DATA));
              server[csi].state = SERVER_STATE_CHECK;
              simcomData.status.server[csi].state = SIMCOM_SERVER_STATE_CLOSE; //Los inicializo a cerrados. A ver que dice el "SERVERSTAR?"
            }
          csi = 0;
          serverServiceData.state = SERVER_SERVICE_STATE_GET_STATUS;
          break;
        case SERVER_SERVICE_STATE_GET_STATUS:
          if (com_command (AT_CMD_READ_SERVERSTART, AT_CMD_STR_LEN (AT_CMD_READ_SERVERSTART), cmd_get_server_service_status_cb, NULL) == COMMAND_ACEPTED)
            {
              serverServiceData.savedState = serverServiceData.state;
              serverServiceData.state = SERVER_SERVICE_STATE_WAIT_RESPONSE;
            }
          break;
        case SERVER_SERVICE_STATE_RUNNING:
          server_run (csi);
          if (++csi >= SIMCOM_MAX_TCP_SERVERS) csi = 0;
          break;
          //
        case SERVER_SERVICE_STATE_WAIT_RESPONSE:
          break;
        }
    }

  return serverServiceData.state;
}

/* ************************************************************** End of File */
