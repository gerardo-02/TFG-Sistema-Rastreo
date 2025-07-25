// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
#include <stdlib.h>
#include <time.h>

#include "config/default/peripheral/rcon/plib_rcon.h"
#include "peripheral/wdt/plib_wdt.h"
#include "config/default/system/time/sys_time.h"

#include "drv/pmz_io_control.h"
#include "drv/drv_mcp7940x.h"

//#include "reles/rele_test.h"
//#include "ext_rtcc/int_ext_rtcc_test.h"
//#include "sst26/sst26_test.h"
//#include "sd/sd_test.h"
//#include "charger/bq25713_test.h"
//#include "digInput/extInput_test.h"
//#include "digInput/rs232_test.h"
//#include "digInput/rs485_test.h"
#include "A7672E.h"
#include "usb_A7672E.h"
#include "pmz_console.h"
#include "log.h"

#include "pmz.h"
#include "net/net.h"
#include "storage/pmz_storage.h"



// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

typedef enum
{
  PMZ_STATE_INIT = 0,
  //  PMZ_STATE_EXTERNAL_RTCC_STARTUP,
  //  PMZ_STATE_WAIT_EXTERNAL_RTCC_UPDATE,
  //  PMZ_STATE_READ_NET_CFG,
  //  PMZ_STATE_WAIT_READ_NET_CFG,
  //  PMZ_STATE_READ_SYSTEM_CFG,
  //  PMZ_STATE_WAIT_READ_SYSTEM_CFG,
  //  PMZ_STATE_READ_RUNNIG_VARS,
  //  PMZ_STATE_WAIT_READ_RUNNIG_VARS,
  //  PMZ_STATE_INIT_SUB_SYSTEM_TASKS,
  PMZ_STATE_RUNNING,
} t_PMZ_STATES;

typedef struct
{
  t_PMZ_STATES state;
  RCON_RESET_CAUSE rrc;
} t_PMZ_DATA;

t_PMZ_DATA pmzData;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************
void simcomPowerSupply(uintptr_t context){
    if (io_output_get_state(IO_OUTPUT_SIMCOM_POWER) == IO_OUTPUT_STATE_LOW){
        io_output_set_state (IO_OUTPUT_SIMCOM_POWER, IO_OUTPUT_STATE_HIGH);  // Dejar de dar corriente durante 2 segundos y reiniciar el módulo SIMCOM
    }
}

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************

static void printRRC (RCON_RESET_CAUSE rrc)
{
  _l ("(PMZ) Reset reasons: (%X)\n", rrc);
  uint32_t test = 1;

  while (test < 0x10000000)
    {
      switch (rrc & test)
        {
        case 0:
          break; //Nothing
        case RCON_RESET_CAUSE_POR:
          _l (" -> Power-on Reset\n");
          break;
        case RCON_RESET_CAUSE_BOR:
          _l (" -> Brown-out Reset\n");
          break;
        case RCON_RESET_CAUSE_IDLE:
          _l (" -> Wake From Idle\n");
          break;
        case RCON_RESET_CAUSE_SLEEP:
          _l (" -> Wake From Sleep\n");
          break;
        case RCON_RESET_CAUSE_WDTO:
          _l (" -> Watchdog Timer Time-out\n");
          break;
        case RCON_RESET_CAUSE_DMTO:
          _l (" -> DeadMan Timer Time-out\n");
          break;
        case RCON_RESET_CAUSE_SWR:
          _l (" -> Software Reset\n");
          break;
        case RCON_RESET_CAUSE_EXTR:
          _l (" -> External Reset (MCLR)\n");
          break;
        case RCON_RESET_CAUSE_CMR:
          _l (" -> Configuration Mismatch Reset\n");
          break;
        case RCON_RESET_CAUSE_BCFGFAIL:
          _l (" -> BCFGFAIL\n");
          break;
        case RCON_RESET_CAUSE_BCFGERR:
          _l (" -> BCFGERR\n");
          break;
        default:
          _l (" -> Unknown reason!\n");
          break;
        }
      test <<= 1;
    }
}



// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

void PMZ_Initialize (void)
{
  /* Reset Reason */
  pmzData.rrc = RCON_ResetCauseGet () & 0x3DF;
  RCON_ResetCauseClear (0x3DF); //Reset all causes
  _l ("\n\r<<<<<<<<<<<<<<<<<<<< INIT SYSTEM >>>>>>>>>>>>>>>>>>>>\n");
  printRRC (pmzData.rrc);
  _l ("\n>>>>>>>>>> Firmware %s initializing <<<<<<<<<<\n\r", PMZ_FIRMWARE_VERSION_STR);
  io_initialize ();
  io_output_blink (IO_OUTPUT_GREEN_LED, 1000, 100, 0);
  io_output_set_state (IO_OUTPUT_AMBER_LED, IO_OUTPUT_STATE_LOW);
  io_output_set_state (IO_OUTPUT_SIMCOM_POWER, IO_OUTPUT_STATE_LOW);
  drv_mcp7940x_init ();
    //  drv_ee24_initialize ();
  //  drv_sst26_initialize ();
  pmz_console_init ();
  //  relay_init ();
  //  int_ext_rtcc_test_init ();
  //  sst26_test_init ();
  //  sd_test_initialize ();
  //  charger_test_init ();
  //  external_input_init ();
  //  rs232_test_init ();
  //  rs485_test_init ();
  simcom_init ();
  usb_simcom_init ();
  
  // El módulo se apaga durante 1 segundo y medio cada vez que se reinicia
  SYS_TIME_HANDLE simcomTimerHandle;
  simcomTimerHandle = SYS_TIME_TimerCreate(0, SYS_TIME_MSToCount(5000), simcomPowerSupply, (uintptr_t) 0, SYS_TIME_SINGLE);
  SYS_TIME_TimerStart(simcomTimerHandle);

  pmzData.state = PMZ_STATE_INIT;
}

void PMZ_Tasks (void)
{
    
  WDT_Clear (); //Clear watch dog timer
  //
  drv_mcp7940x_tasks ();

  //  relay_task ();
  //  int_ext_rtcc_test_task ();
  //  sst26_test_task ();
  //  sd_test_task ();
  //  charger_test_task ();
  //  external_input_task ();
  //  rs232_test_task ();
  //  rs485_test_task ();
  simcom_tasks ();
  usb_simcom_tasks ();

  switch (pmzData.state)
    {
    case PMZ_STATE_INIT:
      net_init ();
      storage_init ();
      pmzData.state = PMZ_STATE_RUNNING;
      break;
    case PMZ_STATE_RUNNING:
      net_tasks ();
      storage_task ();
      break;
    }
}

/**************************************************************** End of File */
