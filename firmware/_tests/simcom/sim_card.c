/* 
 * File:   sim_card.c
 * Author: jtrodriguez
 *
 * Created on 1 de marzo de 2021, 8:18
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

#include "sim_card.h"
#include "com.h"
#include "simcom.h"
#include "at_commands.h"
#include "at_responses.h"
#include "network.h"
#include "timers.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */
#define TIME_FOR_RETRY_MS               5000
#define TIME_FOR_CARD_FAILURE_RETRY_MS  30000

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Data types & Definitions                                          */
/* ************************************************************************** */

/* ************************************************************************** */
typedef struct
{
  bool wait;
  t_SIM_CARD_STATES state;
  uint32_t timeout;
} t_SIM_CARD_DATA;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Global data & variables                                           */
/* ************************************************************************** */
/* ************************************************************************** */
extern t_SIMCOM_DATA simcomData;
static t_SIM_CARD_DATA simCardData;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Callback Functions                                                */
/* ************************************************************************** */
/* ************************************************************************** */
#define SET_SIM_CARD_STATE(newState)        simCardData.state=newState

static void sim_card_set_wait (uint32_t waitTimeMs)
{
  simCardData.wait = true;
  timer_ms (simCardData.timeout, waitTimeMs);
}

static void cmd_cb (t_COM_COMMAND_CALLBACK_CTX *ctx)
{
  switch (ctx->res)
    {
    case COMMAND_RES_OK:
      break;
    case COMMAND_RES_CME_CMS_ERROR:
      if ((ctx->err == CME_ERROR_SIM_NOT_INSERTED) || (ctx->err == CME_ERROR_SIM_FAILURE))
        {
          SIMCOM_MSG ("(SIMCOM-CARD) NOT PRESENT OR FAILURE. Reset!\n");
          SET_SIM_CARD_STATE (SIM_CARD_STATE_INIT);
          simcom_hard_reset_module ();
        }
      else
        {
          sim_card_set_wait (TIME_FOR_CARD_FAILURE_RETRY_MS);
          SET_SIM_CARD_STATE (SIM_CARD_STATE_FAILURE);
          SIMCOM_MSG ("(SIMCOM-CARD) CME ERROR (%d)\n", ctx->err);
        }
      break;
    default:
      sim_card_set_wait (TIME_FOR_RETRY_MS);
      SET_SIM_CARD_STATE (SIM_CARD_STATE_INIT);
      SIMCOM_MSG ("(SIMCOM-CARD) ERROR!\n");
      break;
    }
}

//static void res_simcard_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
//{
//  if (ctx->nArgs > 0)
//    {
//      if (strcmp (ctx->arg[0], "NOT AVAILABLE") == 0)
//        {
//          SET_SIM_CARD_STATE (SIM_CARD_STATE_FAILURE);
//        }
//    }
//}

static void res_cpin_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  if (ctx->nArgs > 0)
    {
      if (strcmp (CPIN_CODE_READY, ctx->arg[0]) == 0)
        {
          SIMCOM_MSG ("(SIMCOM-CARD) %s\n", ctx->arg[0]);
          simcomData.status.simCardReady = true;
          SET_SIM_CARD_STATE (SIM_CARD_STATE_CICCID);
        }
      else
        {
          SIMCOM_MSG ("(SIMCOM-CARD) %s. Not ready!\n", ctx->arg[0]);
          simcomData.status.simCardReady = false;
          sim_card_set_wait (TIME_FOR_RETRY_MS);
          SET_SIM_CARD_STATE (SIM_CARD_STATE_INIT);
        }
    }
}

static void res_ciccid_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  if (ctx->nArgs > 0)
    {
      strncpy (simcomData.status.iccid, ctx->arg[0], SIMCOM_CARD_ICCID_LENGTH + 1);
      simcomData.status.iccid_updated = 1;
      SIMCOM_MSG ("(SIMCOM-CARD) ICCID: %s\n", simcomData.status.iccid);
      SET_SIM_CARD_STATE (SIM_CARD_STATE_PREPARED);
    }
}

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */
#define execute_command(command, callBack, context)           if (com_command (command, AT_CMD_STR_LEN(command), callBack, context) == COMMAND_ACEPTED) SET_SIM_CARD_STATE (SIM_CARD_STATE_WAIT_RESPONSE)

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */
void sim_card_time (void)
{
  timer_dec (simCardData.timeout);
  network_time ();
}

void sim_card_init (void)
{
  simCardData.wait = false;
  timer_off (simCardData.timeout);
  response_set_call_back (AT_RES_CPIN, res_cpin_cb);
  response_set_call_back (AT_RES_CICCID, res_ciccid_cb);
  SET_SIM_CARD_STATE (SIM_CARD_STATE_INIT);
  SIMCOM_MSG ("(SIMCOM-CARD) Init\n");
}

t_SIM_CARD_STATES sim_card_tasks (void)
{
  if (simCardData.wait)
    {
      if (timer_expired (simCardData.timeout))
        {
          simCardData.wait = false;
          timer_off (simCardData.timeout);
        }
    }
  else
    {
      switch (simCardData.state)
        {
        case SIM_CARD_STATE_INIT:
          SET_SIM_CARD_STATE (SIM_CARD_STATE_CPIN);
          break;
        case SIM_CARD_STATE_CPIN:
          execute_command (AT_CMD_READ_CPIN, cmd_cb, NULL);
          break;
        case SIM_CARD_STATE_CICCID:
          execute_command (AT_CMD_CICCID, cmd_cb, NULL);
          break;
        case SIM_CARD_STATE_PREPARED:
          network_init ();
          SET_SIM_CARD_STATE (SIM_CARD_STATE_READY);
          break;
        case SIM_CARD_STATE_READY:
          network_tasks ();
          break;
        case SIM_CARD_STATE_FAILURE:
          sim_card_set_wait (TIME_FOR_CARD_FAILURE_RETRY_MS);
          SET_SIM_CARD_STATE (SIM_CARD_STATE_INIT);
          break;
          //
        case SIM_CARD_STATE_WAIT_RESPONSE:
          break;
        }
    }
  return simCardData.state;
}

/* ************************************************************** End of File */
