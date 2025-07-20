/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "system/time/sys_time.h"
#include "simcom.h"
#include "timers.h"
#include "com.h"
#include "cfg.h"
#include "sim_card.h"
#include "sockets.h"
#include "gps.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */
#define SIM_T_ON_PWRKEY     500     //typ. 500 ms (The time of active low level impulse of PWRKEY pin to power on module)
#define SIM_T_ON_STATUS     25000   //max. 25000 ms = 25s (The time from power-on issue to STATUS pin output high level(indicating power up ready ))    
#define SIM_T_OFF_PWRKEY    3000    //min. 2500 ms = 2,5 s !Como es minimo Pongo algo mas! (The active low level time pulse on PWRKEY pin to power off module)  
#define SIM_T_OFF_STATUS    15000   //min. 10000 ms = 10s !Como es minimo Pongo algo mas! (The time from power-off issue to STATUS pin output low level(indicating power off ))

#define EXTRA_DELAY_MS      6000    //Para dar compatibilidad con el modulo SIM7600. Este envia al arrancar unos mensajes que hay que desechar. No se tratan y se mezclan con los de inicio y config..
#define TIMEOUT_MS          5000

#define AUTO_RESET_TIME     43200000    //12h en ms

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Data types & Definitions                                          */
/* ************************************************************************** */

/* ************************************************************************** */
typedef enum
{
  POWER_STATE_INIT = 0,
  POWER_STATE_RESET_INIT,
  POWER_STATE_RESET_ON,
  POWER_STATE_WAIT_RESET,
  POWER_STATE_EXTRA_DELAY, //Compatibilidad con el SIM7600
  POWER_STATE_WAIT_EXTRA_DELAY,
  POWER_STATE_CONFIGURING,
  POWER_STATE_READY,
  POWER_STATE_INIT_OFF,
  POWER_STATE_POWERING_OFF,
  POWER_STATE_WAIT_OFF,
  POWER_STATE_OFF,
} t_POWER_STATES;

typedef struct
{
  t_POWER_STATES state;
  uint32_t t;
  uint32_t resetTime;
  uint32_t delayAutoPowerOn;
  bool wait;
} t_POWER_DATA;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Global data & variables                                           */
/* ************************************************************************** */
/* ************************************************************************** */
t_SIMCOM_DATA simcomData;
static SYS_TIME_HANDLE sth;
static t_POWER_DATA powerData;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Callback Functions                                                */
/* ************************************************************************** */

/* ************************************************************************** */
static void simcom_timers (uintptr_t context)
{
  timer_dec (powerData.t);
  timer_dec (powerData.resetTime);
  com_time ();
  cfg_time ();
  sim_card_time ();
  gps_time ();
}

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */
#define STATE_SET(newState)     powerData.state=newState

static void set_wait (uint32_t time_ms)
{
  timer_ms (powerData.t, time_ms);
  powerData.wait = true;
}

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */
#warning "COSAS DESHABILITADAS!!!!"

void simcom_init (void)
{
  memset (&simcomData, 0, sizeof (t_SIMCOM_DATA));
  sth = SYS_TIME_CallbackRegisterMS (simcom_timers, 0, TIME_RESOLUTION_MS, SYS_TIME_PERIODIC);
  if (sth != SYS_TIME_HANDLE_INVALID)
    {
      // Power
      timer_off (powerData.t);
      timer_off (powerData.resetTime);
      powerData.wait = false;
      powerData.delayAutoPowerOn = 0;
      com_register_uart_call_backs (); //Esencial para que no se que pilladas las interrupciones de la uart al iniciar el modulo
      SIMCOM_MSG ("(SIMCOM-POWER) Init\n");
      STATE_SET (POWER_STATE_INIT);
    }
  else
    {
      //error
    }
}

void simcom_tasks (void)
{
  if (powerData.wait)
    {
      if (timer_expired (powerData.t))
        {
          timer_off (powerData.t);
          powerData.wait = false;
        }
    }
  else
    {
      switch (powerData.state)
        {
        case POWER_STATE_INIT:
          if (SIMCOM_STATUS_ON ())
            {
              SIMCOM_MSG ("(SIMCOM-POWER) COM init...\n");
              STATE_SET (POWER_STATE_EXTRA_DELAY);
            }
          else
            {
              SIMCOM_MSG ("(SIMCOM-POWER) Wake up.\n");
              STATE_SET (POWER_STATE_RESET_INIT);
            }
          break;
          // <editor-fold defaultstate="collapsed" desc="POWER ON/RESET">
        case POWER_STATE_RESET_INIT:
          if (SIMCOM_STATUS_ON ())
            {
              SIMCOM_PWRKEY_HIGH ();
              SIMCOM_RESET_LOW ();
              SIMCOM_DBG ("(SIMCOM-POWER) Reseting ...\n");
            }
          else
            {
              SIMCOM_RESET_HIGH ();
              SIMCOM_PWRKEY_LOW ();
              SIMCOM_DBG ("(SIMCOM-POWER) Powering ON ...\n");
            }
          set_wait (SIM_T_ON_PWRKEY);
          STATE_SET (POWER_STATE_RESET_ON);
          break;
        case POWER_STATE_RESET_ON:
          SIMCOM_PWRKEY_HIGH ();
          SIMCOM_RESET_HIGH ();
          timer_ms (powerData.t, SIM_T_ON_STATUS);
          STATE_SET (POWER_STATE_WAIT_RESET);
          break;
        case POWER_STATE_WAIT_RESET:
          if (SIMCOM_STATUS_ON ())
            {
              SIMCOM_DBG ("(SIMCOM-POWER) COM init...\n");
              STATE_SET (POWER_STATE_EXTRA_DELAY);
            }
          else
            {
              if (timer_expired (powerData.t))
                {
                  SIMCOM_DBG ("(SIMCOM-POWER) Power ON/Reset ERROR!\n");
                  STATE_SET (POWER_STATE_RESET_INIT);
                }
            }
          break;
          // </editor-fold>
        case POWER_STATE_EXTRA_DELAY:
          /* Compatibilidad con el SIM7600. Este modulo envia al iniciar un RDY, un SMS.. un PD... 
          cosas que no se tratan en este firmware. Por lo tanto espero a que las mande y continuo.
           De este modo el resto de ordenes AT son como las usadas para el SIM7100E */
          timer_ms (powerData.t, EXTRA_DELAY_MS);
          STATE_SET (POWER_STATE_WAIT_EXTRA_DELAY);
          break;
        case POWER_STATE_WAIT_EXTRA_DELAY:
          if (timer_expired (powerData.t))
            {
              memset (&simcomData.status, 0, sizeof (t_SIMCOM_STATUS_DATA));
              SIMCOM_DBG ("(SIMCOM-POWER) Status variables clean. Configuring...\n");
//              com_init ();
//              cfg_init ();
//              gps_init ();
              timer_ms (powerData.t, TIMEOUT_MS);
              STATE_SET (POWER_STATE_CONFIGURING);
            }
          break;
        case POWER_STATE_CONFIGURING:
//          com_tasks ();
//          if (cfg_tasks () == CFG_STATE_OK)
//            {
//              sim_card_init ();
              SIMCOM_DBG ("(SIMCOM-POWER) Configured. Ready.\n");
              timer_ms (powerData.resetTime, AUTO_RESET_TIME);
              STATE_SET (POWER_STATE_READY);
//            }
//          else
//            {
//              if (timer_expired (powerData.t))
//                {
//                  SIMCOM_DBG ("(SIMCOM-POWER) Config timeout!\n");
//                  timer_off (powerData.t);
//                  STATE_SET (POWER_STATE_RESET_INIT);
//                }
//            }
          break;
        case POWER_STATE_READY:
          if (!SIMCOM_STATUS_ON ())
            {
              SIMCOM_DBG ("(SIMCOM-POWER) Not ready! Reset...\n");
              STATE_SET (POWER_STATE_RESET_INIT);
            }
          else
            {
              if ((powerData.resetTime % 360000) == 0)
                {
                  SIMCOM_DBG ("(SIMCOM-POWER) %ds remaining for auto-reset.\n", powerData.resetTime / TIME_RESOLUTION_MS);
                }
              if (timer_expired (powerData.resetTime))
                {
                  SIMCOM_DBG ("(SIMCOM-POWER) AUTO-RESET!...\n");
                  simcom_hard_reset_module ();
                }
              else
                {
                  //com_tasks ();
                  //sim_card_tasks ();
                  //gps_tasks ();
                }
            }
          break;
          // POWER-OFF no tiene funcionalidad en este momento
          // <editor-fold defaultstate="collapsed" desc="POWER OFF">
        case POWER_STATE_INIT_OFF:
          SIMCOM_RESET_HIGH ();
          SIMCOM_PWRKEY_LOW ();
          SIMCOM_DBG ("(SIMCOM-POWER) Powering OFF ...\n");
          set_wait (SIM_T_OFF_PWRKEY);
          STATE_SET (POWER_STATE_POWERING_OFF);
          break;
        case POWER_STATE_POWERING_OFF:
          SIMCOM_PWRKEY_HIGH ();
          SIMCOM_RESET_HIGH ();
          timer_ms (powerData.t, SIM_T_OFF_STATUS);
          STATE_SET (POWER_STATE_WAIT_OFF);
          break;
        case POWER_STATE_WAIT_OFF:
          if (SIMCOM_STATUS_ON ())
            {
              if (timer_expired (powerData.t))
                {
                  SIMCOM_DBG ("(SIMCOM-POWER) Power OFF ERROR!, retry...\n");
                  STATE_SET (POWER_STATE_INIT_OFF);
                }
            }
          else
            {
              SIMCOM_DBG ("(SIMCOM-POWER) Power OFF\n");
              if (powerData.delayAutoPowerOn)
                {
                  timer_ms (powerData.t, powerData.delayAutoPowerOn);
                }
              else
                {
                  timer_off (powerData.t);
                }
              STATE_SET (POWER_STATE_OFF);
            }
          break;
        case POWER_STATE_OFF:
          if (SIMCOM_STATUS_ON ())
            {
              SIMCOM_DBG ("(SIMCOM-POWER) Ready? Reset...\n");
              STATE_SET (POWER_STATE_RESET_INIT);
            }
          else
            {
              if (timer_expired (powerData.t))
                {
                  powerData.delayAutoPowerOn = 0;
                  SIMCOM_DBG ("(SIMCOM-POWER) Auto power ON...\n");
                  STATE_SET (POWER_STATE_RESET_INIT);
                }
            }
          break;
          // </editor-fold>
        }
    }
}

bool simcom_module_ready (void)
{
  return powerData.state == POWER_STATE_READY;
}

void simcom_reset_module (void)
{
  com_register_uart_call_backs (); //A partir de aqui elimina todo lo que entre por la uart!!
  SIMCOM_MSG ("(SIMCOM) Forze reset module...\n");
  STATE_SET (POWER_STATE_RESET_INIT);
}

void simcom_hard_reset_module (void)
{
  com_register_uart_call_backs (); //A partir de aqui elimina todo lo que entre por la uart!!
  powerData.delayAutoPowerOn = 30000; //10s
  SIMCOM_MSG ("(SIMCOM) HARD reset module...\n");
  STATE_SET (POWER_STATE_INIT_OFF);
}

t_SIMCOM_STATUS_DATA simcom_get_status (void)
{
  return simcomData.status;
}

int32_t simcom_call_back_register_clock_sync (SIMCOM_CLOCK_SYNC_CALL_BACK cb)
{
  simcomData.cfg.clockSyncCallBack = cb;
  return 0;
}

int32_t simcom_set_apn (char *apn)
{
  strcpy (simcomData.cfg.apn, apn);
  return 0;
}

int32_t simcom_setup_server (uint32_t index, uint16_t port)
{
  if (index < SIMCOM_MAX_TCP_SERVERS)
    {
      simcomData.cfg.serverPort[index] = port;
    }
  return 0;
}

int32_t simcom_call_back_register_receive (SIMCOM_RECEIVE_CALL_BACK cb)
{
  simcomData.cfg.receiveCallBack = cb;
  return 0;
}

int32_t simcom_response (uint8_t socketIndex, uint8_t *data, uint32_t dataLength, SIMCOM_SENT_CALLBACK cb)
{
  if (simcomData.status.socket[socketIndex].state == SIMCOM_SOCKET_STATE_OPEN)
    {
      sockets_send (socketIndex, data, dataLength, cb);
    }
  else
    {
      SIMCOM_MSG ("(SIMCOM) No response. Connection (%d) closed\n", socketIndex);
    }
  return 0;
}

int32_t simcom_send (t_IPV4_ADDR ip, uint16_t port, uint8_t *data, uint32_t dataLength, SIMCOM_SENT_CALLBACK cb)
{
  return sockets_tcp_send (ip, port, data, dataLength, cb);
}

int32_t simcom_set_gps_mode (uint8_t mode)
{
  SIMCOM_MSG ("(SIMCOM) GPS mode changed to %d\n", mode);
  simcomData.cfg.gpsMode = mode;

  return 0;
}

int32_t simcom_call_back_register_gps_update (SIMCOM_GPS_UPDATE_CALL_BACK cb)
{
  simcomData.cfg.gpsUpdateCallBack = cb;
  return 0;
}

/**********/
unsigned int simcomSIMCOM_DBGprint (char *buff, unsigned short bufflen)
{
  unsigned int bytes = 0;

  bytes = snprintf (buff, bufflen, "\n*** MODULO COMUNICACIONES ***\n");
  bytes += snprintf (buff + bytes, bufflen - bytes, "SimCard ready: %d\n", simcomData.status.simCardReady);
  bytes += snprintf (buff + bytes, bufflen - bytes, "Network registered: %d\n", simcomData.status.networkRegistered);
  bytes += snprintf (buff + bytes, bufflen - bytes, "Network system mode: %d\n", simcomData.status.networkSystemMode);
  bytes += snprintf (buff + bytes, bufflen - bytes, "Net open: %d\n", simcomData.status.netOpen);
  bytes += snprintf (buff + bytes, bufflen - bytes, "RSSI: %ddbm\n", simcomData.status.rssi_dbm);

  return bytes;
}
/**************************************************************** End of File */
