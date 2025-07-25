/* 
 * File:   network.c
 * Author: jtrodriguez
 *
 * Created on 2 de marzo de 2021, 17:55
 */

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <stdint.h>
#include <stdbool.h>

#include "network.h"
#include "at_commands.h"
#include "com.h"
#include "simcom.h"
#include "at_responses.h"
#include "date_time.h"
#include "pdp.h"
#include "timers.h"
#include "rssi_helper.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */
#define TIME_FOR_RETRY_MS       20000
#define TIME_FOR_CNSMOD_TEST    300000 //60000
#define MAX_REGISTER_TRY_COUNT  20

const char *cnsmodStr[] = {
  "NO SERVICE",                     //0
  "GSM",                            //1
  "GPRS",                           //2
  "EGPRS (EDGE)",                   //3
  "WCDMA",                          //4
  "HSDPA only(WCDMA)",              //5
  "HSUPA only(WCDMA)",              //6
  "HSPA (HSDPA and HSUPA, WCDMA)",  //7
  "LTE",                            //8
  "TDS-CDMA",                       //9
  "TDS-HSDPA only",                 //10
  "TDS- HSUPA only",                //11
  "TDS- HSPA (HSDPA and HSUPA)",    //12
  "CDMA",                           //13
  "EVDO",                           //14
  "HYBRID (CDMA and EVDO)",         //15
};

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Data types & Definitions                                          */
/* ************************************************************************** */

/* ************************************************************************** */
typedef struct
{
  t_NETWORK_STATES state;
  uint32_t timeout;
  uint32_t registerTryCount;
  bool wait;
} t_NETWORK_DATA;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Global data & variables                                           */
/* ************************************************************************** */
/* ************************************************************************** */
extern t_SIMCOM_DATA simcomData;
static t_NETWORK_DATA networkData;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Callback Functions                                                */
/* ************************************************************************** */
/* ************************************************************************** */
#define SET_NETWORK_STATE(newState)     networkData.state=newState

static void network_set_wait (uint32_t waitTimeMs)
{
  networkData.wait = true;
  timer_ms (networkData.timeout, waitTimeMs);
}

static void cmd_cb (t_COM_COMMAND_CALLBACK_CTX *ctx)
{
  if (ctx->res != COMMAND_RES_OK)
    {
      network_set_wait (TIME_FOR_RETRY_MS);
      SET_NETWORK_STATE (NETWORK_STATE_INIT);
      SIMCOM_MSG ("(SIMCOM-NETWORK) ERROR!\n");
    }
}
static void cmd_csq_cb (t_COM_COMMAND_CALLBACK_CTX *ctx)
{
  if (ctx->res != COMMAND_RES_OK)
    {
      network_set_wait (TIME_FOR_RETRY_MS);
      SET_NETWORK_STATE (NETWORK_STATE_INIT);
      SIMCOM_MSG ("(SIMCOM-NETWORK) ERROR First CSQ!\n");
    }
  else{
      SET_NETWORK_STATE (NETWORK_STATE_CREG);
    }
}

static void res_csq_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  if (ctx->nArgs > 1)
    {
      simcomData.status.rssi_dbm = rssi_get_dbm (atoi (ctx->arg[0]));
      simcomData.status.rssi_updated = 1; //Esta variable es para consultarla fuera. Si se hiciera por call-back como por ejemplo lo del gps, esta variable no tendría sentido.
    }
}

void res_creg_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  if (ctx->nArgs > 1)
    {
      switch (*ctx->arg[1])
        {
        case CREG_STAT_NO_REG_NO_SEARCH:
          SIMCOM_MSG ("(SIMCOM-NETWORK) Not register & Not search! Reset?\n");
          simcom_hard_reset_module ();
          SET_NETWORK_STATE (NETWORK_STATE_INIT);
          break;
        case CREG_STAT_REG_HOME_NET:
        case CREG_STAT_REG_ROAMING:
          networkData.registerTryCount = 0;
          simcomData.status.networkRegistered = true;
          SET_NETWORK_STATE (NETWORK_STATE_SYSTEM_MODE);
          break;
        case CREG_STAT_NO_REG_SEARCHING:
        case CREG_STAT_REG_DENIED:
          network_set_wait (TIME_FOR_RETRY_MS);
          networkData.registerTryCount++;
          simcomData.status.networkRegistered = false;
          SIMCOM_MSG ("(SIMCOM-NETWORK) Not register or denied (%c), wait...(try=%d)\n", *ctx->arg[1], networkData.registerTryCount);
          SET_NETWORK_STATE (NETWORK_STATE_CREG);
          break;
        case CREG_STAT_UNKNOWN:
        default:
          network_set_wait (TIME_FOR_RETRY_MS);
          networkData.registerTryCount++;
          simcomData.status.networkRegistered = false;
          SIMCOM_MSG ("(SIMCOM-NETWORK) Register error (%c), wait...(try=%d)\n", *ctx->arg[1], networkData.registerTryCount);
          SET_NETWORK_STATE (NETWORK_STATE_CREG);
          break;
        }
      if (networkData.registerTryCount >= MAX_REGISTER_TRY_COUNT)
        {
          SIMCOM_MSG ("(SIMCOM-NETWORK) Error trying (%d)! Reset?\n", networkData.registerTryCount);
          simcom_hard_reset_module ();
          SET_NETWORK_STATE (NETWORK_STATE_INIT);
    }
}
}

void res_cnsmod_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  int32_t nwsm;

  switch (ctx->nArgs)
    {
    case 1:
      // +CNSMOD:<stat> --> Auto report
      nwsm = atoi (ctx->arg[0]);
      break;
    case 2:
      // +CNSMOD: <n>,<stat> --> Command read
      nwsm = atoi (ctx->arg[1]);
      break;
    default:
      nwsm = CNSMOD_STAT_NO_SERVICE;
      break;
    }
  switch (nwsm)
    {
    case CNSMOD_STAT_NO_SERVICE:
    case CNSMOD_STAT_GSM:
      network_set_wait (TIME_FOR_RETRY_MS);
      if (simcomData.status.networkSystemMode > CNSMOD_STAT_GSM)
        {
          SIMCOM_MSG ("NETWORK DATA MODE OFF! (%d - %s)\n", nwsm, cnsmodStr[nwsm]);
        }
      simcomData.status.networkSystemMode = nwsm;
      SET_NETWORK_STATE (NETWORK_STATE_CREG);
      break;
    default:
      network_set_wait (TIME_FOR_CNSMOD_TEST);
      if (simcomData.status.networkSystemMode < CNSMOD_STAT_GPRS)
        {
          SIMCOM_MSG ("NETWORK DATA MODE ON! (%d - %s)\n", nwsm, cnsmodStr[nwsm]);
          simcomData.status.networkRegistered = true;
          pdp_init ();
        }
      simcomData.status.networkSystemMode = nwsm;
      SET_NETWORK_STATE (NETWORK_STATE_SYSTEM_MODE);
      break;
    }
}

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */
#define execute_command(command, callBack, context)           if (com_command (command, AT_CMD_STR_LEN(command), callBack, context) == COMMAND_ACEPTED) SET_NETWORK_STATE (NETWORK_STATE_WAIT_RESPONSE)

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */
void network_time (void)
{
  timer_dec (networkData.timeout);
  cclk_time ();
  pdp_time ();
}

void network_init (void)
{
  networkData.wait = false;
  timer_off (networkData.timeout);
  response_set_call_back (AT_RES_CSQ, res_csq_cb);
  response_set_call_back (AT_RES_CREG, res_creg_cb);
  response_set_call_back (AT_RES_CNSMOD, res_cnsmod_cb);
  SIMCOM_MSG("(SIMCOM-NETWORK) Init...\n");
  SET_NETWORK_STATE (NETWORK_STATE_INIT);
}

t_NETWORK_STATES network_tasks (void)
{
  if (networkData.wait)
    {
      if (timer_expired (networkData.timeout))
        {
          networkData.wait = false;
          timer_off (networkData.timeout);
        }
    }
  else
    {
      switch (networkData.state)
        {
        case NETWORK_STATE_INIT:
          simcomData.status.networkRegistered = false;
          simcomData.status.networkSystemMode = CNSMOD_STAT_NO_SERVICE;
          networkData.registerTryCount = 0;
          cclk_init ();
          SET_NETWORK_STATE (NETWORK_STATE_FIRST_CSQ);
          break;
        case NETWORK_STATE_FIRST_CSQ:
          execute_command (AT_CMD_QUERY_CSQ, cmd_csq_cb, NULL);
          break;
        case NETWORK_STATE_CREG:
          execute_command (AT_CMD_READ_CREG, cmd_cb, NULL);
          break;
        case NETWORK_STATE_SYSTEM_MODE:
          execute_command (AT_CMD_READ_CNSMOD, cmd_cb, NULL);
          break;
          //
        case NETWORK_STATE_WAIT_RESPONSE:
        default:
          break;
        }
    }
  // Tasks whereas registered
  if (simcomData.status.networkRegistered) cclk_tasks ();
  // Tasks whereas system mode is data mode
  if (simcomData.status.networkSystemMode > CNSMOD_STAT_GSM)
    {
      //Conectado al menos en modo GPRS
      pdp_tasks ();
    }

  return networkData.state;
}

/* ************************************************************** End of File */
