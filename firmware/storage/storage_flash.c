#include "stdint.h"
#include "stdbool.h"

#include "../drv/drv_sst26.h"
#include "log.h"

#include "storage_flash.h"

#define BUFFER_SIZE     8192 //*NOTA: para soportar este buffer size, se necesitar configurar el numero de "buffer descriptors" a 32 o mas, en el driver del sst26.
#define MEM_ADDRESS     0x0
#define MEM_NUM         2

typedef enum
{
  APP_STATE_INIT, /* Application's state machine's initial state. */
  APP_STATE_OPEN_DRIVER, /* Open the Driver */
  /* Get Device Geometry */
  APP_STATE_ERROR,
  //
  APP_STATE_IDLE,
} APP_STATES;

typedef struct
{
  /* The application's current state */
  APP_STATES state;
  /* Driver Handle */
  DRV_HANDLE handle[2];
  /* SST26 Device Geometry */
  DRV_SST26_GEOMETRY geometry[2];
  /* Jedec-ID*/
  uint32_t jedec_id[2];
  /* Security-ID*/
  uint64_t security_id[2];
  /* Read Buffer */
  uint8_t readBuffer[BUFFER_SIZE];
  /* Write Buffer*/
  uint8_t writeBuffer[BUFFER_SIZE];
} APP_DATA;


static APP_DATA CACHE_ALIGN appData;

int storage_flash_init (void)
{
  int i;

  DRV_SST26_Initialize ();
  for (i = 0; i < MEM_NUM; i++)
    {
      appData.handle[i] = DRV_HANDLE_INVALID;
    }
  appData.state = APP_STATE_INIT;
  return 0;
}

int storage_flash_task (void)
{
  static int mmm = 0;
  //  DRV_SST26_TRANSFER_STATUS transferStatus = DRV_SST26_TRANSFER_ERROR_UNKNOWN;

  switch (appData.state)
    {
    case APP_STATE_INIT:
      if (DRV_SST26_Status () == SYS_STATUS_READY)
        {
          _l ("(STORAGE-SST26) SST26 driver initialized.\n");
          appData.state = APP_STATE_OPEN_DRIVER;
        }
      else
        {
          _l ("(STORAGE-SST26) SST26 driver init error!\n");
          appData.state = APP_STATE_ERROR;
        }
      break;
    case APP_STATE_OPEN_DRIVER:
      appData.handle[mmm] = DRV_SST26_Open (mmm);
      if (appData.handle[mmm] != DRV_HANDLE_INVALID)
        {
          _l ("(STORAGE-SST26) SST26 device %d, successfully opened. Getting geometry...\n", mmm);
          if (mmm < DRV_SST26_MEM_IC_MAX)
            {
              mmm++;
              appData.state = APP_STATE_OPEN_DRIVER;
            }
          else
            {
              appData.state = APP_STATE_IDLE;
            }
        }
      else
        {
          _l ("(STORAGE-SST26) SST26 device %d, open error!\n", mmm);
          appData.state = APP_STATE_ERROR;
        }
      break;
      //
    case APP_STATE_IDLE:
      break;
      //
    case APP_STATE_ERROR:
      break;
    }

  return 0;
}
