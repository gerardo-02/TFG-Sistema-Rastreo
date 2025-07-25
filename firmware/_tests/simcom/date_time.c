/* 
 * File:   date_time.c
 * Author: jtrodriguez
 *
 * Created on 8 de marzo de 2021, 7:56
 */

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "date_time.h"
#include "com.h"
#include "simcom.h"
#include "at_commands.h"
#include "at_responses.h"
#include "timers.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */
#define TIME_FOR_RETRY_MS       5000
#define TIME_FOR_REFRESH_MS     1200000

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Data types & Definitions                                          */
/* ************************************************************************** */

/* ************************************************************************** */
typedef struct
{
  t_CCLK_STATES state;
  uint32_t timeout;
  bool wait;
} t_CCLK_DATA;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Global data & variables                                           */
/* ************************************************************************** */
/* ************************************************************************** */
extern t_SIMCOM_DATA simcomData;
static t_CCLK_DATA cclkData;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Callback Functions                                                */
/* ************************************************************************** */
/* ************************************************************************** */
#define SET_CCLK_STATE(newState)     cclkData.state=newState

static void cclk_set_wait (uint32_t waitTimeMs)
{
  cclkData.wait = true;
  timer_ms (cclkData.timeout, waitTimeMs);
}

static void cmd_cclk_cb (t_COM_COMMAND_CALLBACK_CTX *ctx)
{
  if (ctx->res == COMMAND_RES_OK)
    {
      SET_CCLK_STATE (CCLK_STATE_REFRESH);
    }
  else
    {
      cclk_set_wait (TIME_FOR_RETRY_MS);
      SET_CCLK_STATE (CCLK_STATE_INIT);
      SIMCOM_MSG ("(SIMCOM-CCLK) ERROR!\n");
    }
}

void res_cclk_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  t_SIMCOM_CLOCK_SYNC_CALL_BACK_CONTEXT t;

  if (simcomData.cfg.clockSyncCallBack)
    {
      if (ctx->nArgs > 1)
        {
          //Date: yy/MM/dd 
          *(ctx->arg[0] + 2) = *(ctx->arg[0] + 5) = 0; //null string para separar yy, mm, dd
          t.year = atoi (ctx->arg[0]); //yy
          t.mounth = atoi (ctx->arg[0] + 3); //MM
          t.day = atoi (ctx->arg[0] + 6); //dd
          //Time: hh:mm:ss±zz
          t.gmt = atoi (ctx->arg[1] + 8) / 4; //±zz (indicates the difference, expressed in quarters of an hour, between the local time and GMT)
          *(ctx->arg[1] + 2) = *(ctx->arg[1] + 5) = *(ctx->arg[1] + 8) = 0; //null string para separar hh, mm, ss, zz
          t.hour = atoi (ctx->arg[1]); //hh
          t.minute = atoi (ctx->arg[1] + 3); //mm
          t.second = atoi (ctx->arg[1] + 6); //ss
          // Call-back
          simcomData.cfg.clockSyncCallBack (&t);
        }
    }
}

void res_ctzv_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  SIMCOM_MSG ("(SIMCOM-CCLK) Time zone update reporting!\n");
  if (cclkData.wait)
    {
      cclkData.wait = false;
      timer_off (cclkData.timeout);
    }
}

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */
#define execute_command(command, callBack, context)           if (com_command (command, AT_CMD_STR_LEN(command), callBack, context) == COMMAND_ACEPTED) SET_CCLK_STATE (CCLK_STATE_WAIT_RESPONSE)

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */
void cclk_time (void)
{
  timer_dec (cclkData.timeout);
}

void cclk_init (void)
{
  cclkData.wait = false;
  timer_off (cclkData.timeout);
  response_set_call_back (AT_RES_CCLK, res_cclk_cb);
  response_set_call_back (AT_RES_CTZV, res_ctzv_cb);
  SIMCOM_MSG ("(SIMCOM-CCLK) Init...\n");
  SET_CCLK_STATE (CCLK_STATE_INIT);
}

t_CCLK_STATES cclk_tasks (void)
{
  if (cclkData.wait)
    {
      if (timer_expired (cclkData.timeout))
        {
          cclkData.wait = false;
          timer_off (cclkData.timeout);
        }
    }
  else
    {
      switch (cclkData.state)
        {
        case CCLK_STATE_INIT:
          SET_CCLK_STATE (CCLK_STATE_QUERY);
          break;
        case CCLK_STATE_QUERY:
          execute_command (AT_CMD_READ_CCLK, cmd_cclk_cb, NULL);
          break;
        case CCLK_STATE_REFRESH:
          cclk_set_wait (TIME_FOR_REFRESH_MS);
          SET_CCLK_STATE (CCLK_STATE_QUERY);
          break;
          //
        case CCLK_STATE_WAIT_RESPONSE:
          break;
        }
    }
  
  return cclkData.state;
}

/* ************************************************************** End of File */
