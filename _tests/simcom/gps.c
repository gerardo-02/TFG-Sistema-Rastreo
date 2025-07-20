/* 
 * File:   gps.c
 * Author: jtrodriguez
 *
 * Created on 25 de marzo de 2021, 11:22
 */

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include "gps.h"
//#include "time_helper.h"
#include "com.h"
//#include "log_console.h"
#include "simcom.h"
#include "at_responses.h"
#include "at_commands.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */
#define TIME_FOR_RETRY_MS               5000
#define TIME_FOR_RETRY_QUERY_INFO_MS    30000
#define TIME_MAX_SESSION_UPDATE_GPS     600000 //10 minutos

#define TIME_RESOLUTION_MS      100

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Data types & Definitions                                          */
/* ************************************************************************** */

/* ************************************************************************** */
typedef struct
{
  t_GPS_STATES state;
  t_GPS_STATES savedState;
  uint32_t timeout;
  uint32_t timeSession;
  bool wait;
} t_GPS_DATA;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Global data & variables                                           */
/* ************************************************************************** */
/* ************************************************************************** */
extern t_SIMCOM_DATA simcomData;
static t_GPS_DATA gpsData;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Callback Functions                                                */
/* ************************************************************************** */
/* ************************************************************************** */
#define timer_ms(timer,time_ms)     (timer=((time_ms/TIME_RESOLUTION_MS)+1))
#define timer_expired(timer)        (timer==1)
#define timer_off(timer)            (timer=0) 
#define timer_dec(timer)            if(timer>1)timer--
#define SET_GPS_STATE(newState)     gpsData.state=newState

static void gps_set_wait (uint32_t waitTimeMs)
{
  gpsData.wait = true;
  timer_ms (gpsData.timeout, waitTimeMs);
}

static void cmd_cb (t_COM_COMMAND_CALLBACK_CTX *ctx)
{
  if (ctx->res == COMMAND_RES_OK)
    {
      switch (gpsData.savedState)
        {
        case GPS_STATE_STARTING_SESSION:
          SIMCOM_MSG ("(SIMCOM-GPS) Session started.\n");
          timer_ms (gpsData.timeSession, TIME_MAX_SESSION_UPDATE_GPS);
          SET_GPS_STATE (GPS_STATE_SESSION_STARTED);
          break;
        case GPS_STATE_QUERY_INFO:
          SET_GPS_STATE (GPS_STATE_SESSION_STARTED);
          break;
        default:
          break;
        }
      //En el caso de: <off>, it will report indication +CGPS: 0
    }
  else
    {
      gps_set_wait (TIME_FOR_RETRY_MS);
      SIMCOM_MSG ("(SIMCOM-GPS) ERROR execute comman. State: (%d). Retry...\n", gpsData.savedState);
      SET_GPS_STATE (gpsData.savedState);
    }
}

//static void query_info_cb (t_COM_COMMAND_CALLBACK_CTX *ctx)
//{
//  if (ctx->res != COMMAND_RES_OK)
//    {
//      SIMCOM_MSG ("(SIMCOM-GPS) ERROR execute query info. State: (%d)\n", gpsData.savedState);
//    }
//  SET_GPS_STATE (GPS_STATE_GET_SESSION_STATE);
//}

static void res_cgps_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  if (ctx->nArgs > 0)
    {
      if (*(ctx->arg[0]) == GPS_RES_SESSION_START)
        {
          simcomData.status.gpsSessionActive = 1;
          SIMCOM_MSG ("(SIMCOM-GPS) Session alredy started.\n");
          timer_ms (gpsData.timeSession, TIME_MAX_SESSION_UPDATE_GPS);
          SET_GPS_STATE (GPS_STATE_SESSION_STARTED);
        }
      else
        {
          simcomData.status.gpsSessionActive = 0;
          SIMCOM_MSG ("(SIMCOM-GPS) NO active session\n");
          SET_GPS_STATE (GPS_STATE_SESSION_STOPED);
        }
    }
}

static float atof_ (char *a) //<--- El "atof" de <stdlib.> en XC32 NO compila!!! Da error al linkar: "undefined reference to '_dstrtod'" Vete tu a saber
{
  float i = 0.0, d = 0.0;
  a = strtok (a, ".");
  if (a)
    {
      // Parte entera
      i = (float) atoi (a);
      a = strtok (NULL, ".");
      if (a)
        {
          // Parte decimal
          d = (float) atoi (a) / powf (10.0, strlen (a));
        }
    }

  return i + d;
}

static void res_cgpsinfo_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  if (ctx->nArgs == 9)
    {
      // Latitude
      simcomData.status.gpsInfo.latDM = atof_ (ctx->arg[0] + 2); // xxmm.mmmmmm
      *(ctx->arg[0] + 2) = 0; // null string on dd0m.mmmmmm for isolate degrees 
      simcomData.status.gpsInfo.latD = atoi (ctx->arg[0]); // dd
      simcomData.status.gpsInfo.latO = *(ctx->arg[1]); // N/S
      // Longitude
      simcomData.status.gpsInfo.logDM = atof_ (ctx->arg[2] + 3); // xxxmm.mmmmmm  
      *(ctx->arg[2] + 3) = 0; // null string on ddd0m.mmmmmm for isolate degrees
      simcomData.status.gpsInfo.logD = atoi (ctx->arg[2]); // dd
      simcomData.status.gpsInfo.logO = *(ctx->arg[3]); // E/W
      // Date & UTC time
      strcpy (simcomData.status.gpsInfo.date, ctx->arg[4]);
      strcpy (simcomData.status.gpsInfo.utcTime, ctx->arg[5]);
      // Altitude
      simcomData.status.gpsInfo.alt = atof_ (ctx->arg[6]);
      // Speed
      simcomData.status.gpsInfo.speed = atof_ (ctx->arg[7]);
      // Course
      simcomData.status.gpsInfo.course = atoi (ctx->arg[8]);
      simcomData.status.gpsUdapte = 1;
      if (simcomData.cfg.gpsUpdateCallBack)
        {
          simcomData.cfg.gpsUpdateCallBack (&simcomData.status.gpsInfo);
        }
    }
  else
    {
      SIMCOM_MSG ("(SIMCOM-GPS) GPS info NO reconized!\n");
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
      gpsData.savedState = gpsData.state;
      SET_GPS_STATE (GPS_STATE_WAIT_COMMAND_RESPONSE);
    }
}

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */
void gps_time (void)
{
  timer_dec (gpsData.timeout);
  timer_dec (gpsData.timeSession);
}

void gps_init (void)
{
  gpsData.wait = false;
  timer_off (gpsData.timeout);
  response_set_call_back (AT_RES_CGPS, res_cgps_cb);
  response_set_call_back (AT_RES_CGPSINFO, res_cgpsinfo_cb);
  SIMCOM_MSG ("(SIMCOM-GPS) Init...\n");
  SET_GPS_STATE (GPS_STATE_INIT);
}

t_GPS_STATES gps_tasks (void)
{
  if (gpsData.wait)
    {
      if (timer_expired (gpsData.timeout))
        {
          gpsData.wait = false;
          timer_off (gpsData.timeout);
        }
    }
  else
    {
      switch (gpsData.state)
        {
        case GPS_STATE_INIT:
          SET_GPS_STATE (GPS_STATE_GET_SESSION_STATE);
          break;
        case GPS_STATE_GET_SESSION_STATE:
          execute_command (AT_CMD_GET_GPS_SESSION_STATE, cmd_cb);
          break;
          // ** Despues de GET_SESSION_INFO, se quedara en Wait command response hasta que llegue +CGPS e informe de cual es el estado de la session GPS. ** //
        case GPS_STATE_STARTING_SESSION:
          execute_command (AT_CMD_START_GPS_SESSION, cmd_cb);
          break;
        case GPS_STATE_SESSION_STARTED:
          switch (simcomData.cfg.gpsMode)
            {
            case SIMCOM_GPS_MODE_OFF:
              SET_GPS_STATE (GPS_STATE_STOPPING_SESSION);
              break;
            case SIMCOM_GPS_MODE_ON:
              SIMCOM_MSG ("(SIMCOM-GPS) GPS mode ON. Quering in %dms\n", TIME_FOR_RETRY_QUERY_INFO_MS);
              gps_set_wait (TIME_FOR_RETRY_QUERY_INFO_MS);
              SET_GPS_STATE (GPS_STATE_QUERY_INFO);
              break;
            case SIMCOM_GPS_MODE_ON_UNTIL_GET_POS:
              if (simcomData.status.gpsUdapte)
                {
                  SIMCOM_MSG ("(SIMCOM-GPS) GPS update => Closing session...\n");
                  simcomData.cfg.gpsMode = SIMCOM_GPS_MODE_OFF;
                  SET_GPS_STATE (GPS_STATE_STOPPING_SESSION);
                }
              else
                {
                  if (timer_expired (gpsData.timeSession))
                    {
                      timer_off (gpsData.timeSession);
                      SIMCOM_MSG ("(SIMCOM-GPS) WARNING! session expired before update. Closing session...\n");
                      simcomData.cfg.gpsMode = SIMCOM_GPS_MODE_OFF;
                      SET_GPS_STATE (GPS_STATE_STOPPING_SESSION);
                    }
                  else
                    {
                      SIMCOM_MSG ("(SIMCOM-GPS) GPS mode ON until get pos. Session time: %dms. Quering in %dms\n", gpsData.timeSession, TIME_FOR_RETRY_QUERY_INFO_MS);
                      gps_set_wait (TIME_FOR_RETRY_QUERY_INFO_MS);
                      SET_GPS_STATE (GPS_STATE_QUERY_INFO);
                    }
                }
              break;
            default:
              SIMCOM_MSG ("(SIMCOM-GPS) Error unknown mode!\n");
              break;
            }
          break;
        case GPS_STATE_QUERY_INFO:
          execute_command (AT_CMD_EXEC_CGPSINFO, cmd_cb);
          break;
        case GPS_STATE_STOPPING_SESSION:
          execute_command (AT_CMD_STOP_GPS_SESSION, cmd_cb);
          break;
        case GPS_STATE_SESSION_STOPED:
          switch (simcomData.cfg.gpsMode)
            {
            case SIMCOM_GPS_MODE_OFF:
              break;
            case SIMCOM_GPS_MODE_ON:
            case SIMCOM_GPS_MODE_ON_UNTIL_GET_POS:
              simcomData.status.gpsUdapte = 0;
              SET_GPS_STATE (GPS_STATE_STARTING_SESSION);
              break;
            default:
              SIMCOM_MSG ("(SIMCOM-GPS) Error unknown mode!\n");
              break;
            }
          break;
          //
        case GPS_STATE_WAIT_COMMAND_RESPONSE:
          break;
        }
    }

  return gpsData.state;
}

/* ************************************************************** End of File */
