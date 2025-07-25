/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <string.h>

#include "configuration.h"
#include "driver/sst26/drv_sst26.h"

#include "log.h"

#include "sst26_test.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */
/* Erase-Write-Read 2 Sector of Data (4096 *2)*/
#define BUFFER_SIZE     8192 //*NOTA: para soportar este buffer size, se necesitar configurar el numero de "buffer descriptors" a 32 o mas, en el driver del sst26.
#define MEM_ADDRESS     0x0

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Data types & Definitions                                          */
/* ************************************************************************** */

/* ************************************************************************** */
typedef enum
{
  APP_STATE_INIT, /* Application's state machine's initial state. */
  APP_STATE_OPEN_DRIVER, /* Open the Driver */
  /* Get Device Geometry */
  APP_STATE_GEOMETRY_GET,
  /* Erase Flash */
  APP_STATE_ERASE_FLASH,
  /* Erase Wait */
  APP_STATE_ERASE_WAIT,
  /* Write to Memory */
  APP_STATE_WRITE_MEMORY,
  /* Write Wait */
  APP_STATE_WRITE_WAIT,
  /* Read From Memory */
  APP_STATE_READ_MEMORY,
  /* Read Wait */
  APP_STATE_READ_WAIT,
  /* Verify Data Read */
  APP_STATE_VERIFY_DATA,
  /* The app idles */
  APP_STATE_SUCCESS,
  /* An app error has occurred */
  APP_STATE_ERROR,
  APP_STATE_IDLE,
} APP_STATES;

typedef struct
{
  /* The application's current state */
  APP_STATES state;
  /* Driver Handle */
  DRV_HANDLE handle;
  /* SST26 Device Geometry */
  DRV_SST26_GEOMETRY geometry;
  /* Jedec-ID*/
  uint32_t jedec_id;
  /* Read Buffer */
  uint8_t readBuffer[BUFFER_SIZE];
  /* Write Buffer*/
  uint8_t writeBuffer[BUFFER_SIZE];
} APP_DATA;


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Global data & variables                                           */
/* ************************************************************************** */
/* ************************************************************************** */
static APP_DATA CACHE_ALIGN appData;
static uint32_t erase_index = 0;
static uint32_t write_index = 0;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Callback Functions                                                */
/* ************************************************************************** */
/* ************************************************************************** */


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */
void sst26_test_init (void)
{
  uint32_t i = 0;

  for (i = 0; i < BUFFER_SIZE; i++) appData.writeBuffer[i] = i;
  
  appData.state = APP_STATE_IDLE;
}

void sst26_test_task (void)
{
  DRV_SST26_TRANSFER_STATUS transferStatus = DRV_SST26_TRANSFER_ERROR_UNKNOWN;

  switch (appData.state)
    {
    case APP_STATE_IDLE:
      break;
    case APP_STATE_INIT:
      if (DRV_SST26_Status (DRV_SST26_INDEX) == SYS_STATUS_READY)
        {
          appData.state = APP_STATE_OPEN_DRIVER;
        }
      break;
    case APP_STATE_OPEN_DRIVER:
      appData.handle = DRV_SST26_Open (DRV_SST26_INDEX, DRV_IO_INTENT_READWRITE);
      if (appData.handle != DRV_HANDLE_INVALID)
        {
          appData.state = APP_STATE_GEOMETRY_GET;
        }
      break;
    case APP_STATE_GEOMETRY_GET:
      if (DRV_SST26_GeometryGet (appData.handle, &appData.geometry) != true)
        {
          appData.state = APP_STATE_ERROR;
          break;
        }
      _l ("(SST26 TEST) Regions:%u, Blocks:%u, BlockSize:%u\n", appData.geometry.numReadRegions, appData.geometry.read_numBlocks, appData.geometry.read_blockSize);
      erase_index = 0;
      write_index = 0;
      _l ("(SST26 TEST) memory erase...\n");
      appData.state = APP_STATE_ERASE_FLASH;
      break;
    case APP_STATE_ERASE_FLASH:
      if (DRV_SST26_SectorErase (appData.handle, (MEM_ADDRESS + erase_index)) != true)
        {
          appData.state = APP_STATE_ERROR;
        }
      appData.state = APP_STATE_ERASE_WAIT;
      break;
    case APP_STATE_ERASE_WAIT:
      transferStatus = DRV_SST26_TransferStatusGet (appData.handle);
      if (transferStatus == DRV_SST26_TRANSFER_COMPLETED)
        {
          erase_index += appData.geometry.erase_blockSize;
          if (erase_index < BUFFER_SIZE)
            {
              appData.state = APP_STATE_ERASE_FLASH;
            }
          else
            {
              _l ("(SST26 TEST) erase OK\n");
              _l ("(SST26 TEST) memory write...\n");
              appData.state = APP_STATE_WRITE_MEMORY;
            }
        }
      else if (transferStatus == DRV_SST26_TRANSFER_ERROR_UNKNOWN)
        {
          appData.state = APP_STATE_ERROR;
        }
      break;
    case APP_STATE_WRITE_MEMORY:
      if (DRV_SST26_PageWrite (appData.handle, (uint32_t *) & appData.writeBuffer[write_index], (MEM_ADDRESS + write_index)) != true)
        {
          appData.state = APP_STATE_ERROR;
          break;
        }

      appData.state = APP_STATE_WRITE_WAIT;
      break;
    case APP_STATE_WRITE_WAIT:
      transferStatus = DRV_SST26_TransferStatusGet (appData.handle);
      if (transferStatus == DRV_SST26_TRANSFER_COMPLETED)
        {
          write_index += appData.geometry.write_blockSize;

          if (write_index < BUFFER_SIZE)
            {
              appData.state = APP_STATE_WRITE_MEMORY;
            }
          else
            {
              _l ("(SST26 TEST) write OK\n");
              _l ("(SST26 TEST) memory read...\n");
              appData.state = APP_STATE_READ_MEMORY;
            }
        }
      else if (transferStatus == DRV_SST26_TRANSFER_ERROR_UNKNOWN)
        {
          appData.state = APP_STATE_ERROR;
        }
      break;
    case APP_STATE_READ_MEMORY:
      if (DRV_SST26_Read (appData.handle, (uint32_t *) & appData.readBuffer, BUFFER_SIZE, MEM_ADDRESS) != true)
        {
          appData.state = APP_STATE_ERROR;
        }
      else
        {
          appData.state = APP_STATE_READ_WAIT;
        }
      break;
    case APP_STATE_READ_WAIT:
      transferStatus = DRV_SST26_TransferStatusGet (appData.handle);
      if (transferStatus == DRV_SST26_TRANSFER_COMPLETED)
        {
          _l ("(SST26 TEST) read OK\n");
          _l ("(SST26 TEST) verify...\n");
          appData.state = APP_STATE_VERIFY_DATA;
        }
      else if (transferStatus == DRV_SST26_TRANSFER_ERROR_UNKNOWN)
        {
          appData.state = APP_STATE_ERROR;
        }
      break;
    case APP_STATE_VERIFY_DATA:
      if (!memcmp (appData.writeBuffer, appData.readBuffer, BUFFER_SIZE))
        {
          appData.state = APP_STATE_SUCCESS;
        }
      else
        {
          appData.state = APP_STATE_ERROR;
        }
      break;
    case APP_STATE_SUCCESS:
      DRV_SST26_Close (appData.handle);
      _l ("(SST26 TEST) OK!\n");
      appData.state = APP_STATE_IDLE;
      break;
    case APP_STATE_ERROR:
      DRV_SST26_Close (appData.handle);
      _l ("(SST26 TEST) ERROR!\n");
      appData.state = APP_STATE_IDLE;
      break;
    default:
      break;
    }
}

void sst26_testing (void)
{
  switch (appData.state)
    {
    case APP_STATE_IDLE:
      _l ("(SST26 TEST) Init test...\n");
      appData.state = APP_STATE_INIT;
      break;
    default:
      _l ("(SST26 TEST) busy!\n");
      break;
    }
}
/* ************************************************************** End of File */
