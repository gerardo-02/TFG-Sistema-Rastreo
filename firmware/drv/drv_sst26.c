#include <string.h>
#include <stddef.h>

#include "sys/kmem.h"
#include "peripheral/sqi/plib_sqi1.h"

#include "log.h"

#include "drv_sst26.h"

// *****************************************************************************
// *****************************************************************************
// Section: Definitions
// *****************************************************************************
// *****************************************************************************

#define DRV_SST26_MAX_HANDLES    2

#if !defined(SQI_BDCTRL_SQICS_CS0) && !defined(SQI_BDCTRL_SQICS_CS1)
#error "No SQI chips select defined!"
#endif
#define SQI_INVALID_CS_VALUE        0xAA

#define DRV_SST26_START_ADDRESS     (0x0U)
#define DRV_SST26_PAGE_SIZE         (256U)
#define DRV_SST26_ERASE_BUFFER_SIZE (4096U)
#define SQI_DMA_TRANSFER_FUNCTION   SQI1_DMATransfer
#define SQI_DMA_CALLBACK_FUNCTION   SQI1_RegisterCallback
#define DRV_SQI_LANE_MODE           SQI_BDCTRL_MODE_QUAD_LANE
#define CMD_BUFFER_MAX_LENGTH       (7)
#define CMD_DESC_NUMBER             (5)
#define BUFF_DESC_NUMBER            (32U)
#define DUMMY_BYTE                  0xFF
#define JEDEC_ID_SIZE               sizeof(t_JEDEC)
#define SECURITY_ID_SIZE            sizeof(t_SECURITY_ID)

/* Pointer to perform DMA Transfer with SQI PLIB. */
typedef void (*DRV_SST26_PLIB_DMA_TRANSFER)(sqi_dma_desc_t *sqiDmaDesc);
/* Pointer to Register event handler with SQI PLIB. */
typedef void (*DRV_SST26_PLIB_REGISTER_CALLBACK)(SQI_EVENT_HANDLER event_handler, uintptr_t context);

typedef enum
{
  DRV_SST26_OPERATION_TYPE_NONE = 0,
  DRV_SST26_OPERATION_TYPE_CMD,
  DRV_SST26_OPERATION_TYPE_READ,
  DRV_SST26_OPERATION_TYPE_WRITE,
  DRV_SST26_OPERATION_TYPE_ERASE,
} DRV_SST26_OPERATION_TYPE;

typedef struct
{
  bool opened;
  volatile uint32_t cs;
  t_JEDEC jedecId;
  DRV_SST26_GEOMETRY geometry;
  t_SECURITY_ID securityId;
} HANDLE_DATA;

typedef struct
{
  bool initialized;
  volatile bool isTransferDone;
  SYS_STATUS status;
  DRV_SST26_OPERATION_TYPE curOpType;
  HANDLE_DATA handle[DRV_SST26_MAX_HANDLES];
  /* PLIB API list that will be used by the driver to access the hardware */
  DRV_SST26_PLIB_DMA_TRANSFER DMATransfer;
  DRV_SST26_PLIB_REGISTER_CALLBACK RegisterCallback;
} DRV_SST26_DATA;

typedef enum
{
  SST26_CMD_FLASH_RESET_ENABLE = 0x66, /* Reset enable command. */
  SST26_CMD_FLASH_RESET = 0x99, /* Command to reset the flash. */
  SST26_CMD_ENABLE_QUAD_IO = 0x38, /* Command to Enable QUAD IO */
  SST26_CMD_RESET_QUAD_IO = 0xFF, /* Command to Reset QUAD IO */
  SST26_CMD_JEDEC_ID_READ = 0x9F, /* Command to read JEDEC-ID of the flash device. */
  SST26_CMD_QUAD_JEDEC_ID_READ = 0xAF, /* QUAD Command to read JEDEC-ID of the flash device. */
  SST26_CMD_HIGH_SPEED_READ = 0x0B, /* Command to perform High Speed Read */
  SST26_CMD_WRITE_ENABLE = 0x06, /* Write enable command. */
  SST26_CMD_PAGE_PROGRAM = 0x02, /* Page Program command. */
  SST26_CMD_READ_STATUS_REG = 0x05, /* Command to read the Flash status register. */
  SST26_CMD_SECTOR_ERASE = 0x20, /* Command to perform sector erase */
  SST26_CMD_BULK_ERASE_64K = 0xD8, /* Command to perform Bulk erase */
  SST26_CMD_CHIP_ERASE = 0xC7, /* Command to perform Chip erase */
  SST26_CMD_UNPROTECT_GLOBAL = 0x98, /* Command to unlock the flash device. */
  SST26_CMD_SECURITY_ID_READ = 0x88, /* Command to read SECURITY-ID of the flash device. */
} SST26_CMD;

// *****************************************************************************
// *****************************************************************************
// Section: Global objects
// *****************************************************************************
// *****************************************************************************
/* Table mapping the Flash ID's to their sizes. */
static uint32_t gSstFlashIdSizeTable [5][2] = {
  {0x01, 0x200000}, /* 16 MBit */
  {0x41, 0x200000}, /* 16 MBit */
  {0x02, 0x400000}, /* 32 MBit */
  {0x42, 0x400000}, /* 32 MBit */
  {0x43, 0x800000} /* 64 MBit */
};

static DRV_SST26_DATA drvData;
static sqi_dma_desc_t CACHE_ALIGN sqiCmdDesc[CMD_DESC_NUMBER];
static sqi_dma_desc_t CACHE_ALIGN sqiBufDesc[BUFF_DESC_NUMBER];
static uint8_t CACHE_ALIGN statusRegVal;
static uint8_t CACHE_ALIGN jedecID[JEDEC_ID_SIZE];
static uint8_t CACHE_ALIGN securityID[SECURITY_ID_SIZE];
static uint8_t CACHE_ALIGN sqi_cmd_wren;
static uint8_t CACHE_ALIGN sqi_cmd[CMD_BUFFER_MAX_LENGTH];

// *****************************************************************************
// *****************************************************************************
// Section: SST26 Driver Local Functions
// *****************************************************************************
// *****************************************************************************

static void event_handler (uintptr_t context)
{
  drvData.isTransferDone = true;
}

static void init_handle_data (HANDLE_DATA *hd)
{
  memset (hd, 0, sizeof (HANDLE_DATA));
  hd->cs = SQI_INVALID_CS_VALUE;
  hd->opened = false;
}

static uint32_t get_flash_size (uint8_t deviceId)
{
  int i = 0;
  for (i = 0; i < 5; i++)
    {
      if (deviceId == gSstFlashIdSizeTable[i][0])
        {
          return gSstFlashIdSizeTable[i][1];
        }
    }
  return 0;
}

static void reset_flash (HANDLE_DATA *h)
{
  drvData.isTransferDone = false;
  drvData.curOpType = DRV_SST26_OPERATION_TYPE_CMD;
  //Command
  sqi_cmd[0] = (uint8_t) SST26_CMD_FLASH_RESET_ENABLE;
  //Command descriptor
  sqiCmdDesc[0].bd_ctrl = (SQI_BDCTRL_BUFFLEN_VAL (1) | SQI_BDCTRL_PKTINTEN | SQI_BDCTRL_LASTPKT | SQI_BDCTRL_LASTBD | h->cs | SQI_BDCTRL_DEASSERT | SQI_BDCTRL_DESCEN);
  sqiCmdDesc[0].bd_bufaddr = (uint32_t *) KVA_TO_PA (&sqi_cmd[0]);
  sqiCmdDesc[0].bd_stat = 0;
  sqiCmdDesc[0].bd_nxtptr = NULL;
  //Init DMA transfer
  drvData.DMATransfer ((sqi_dma_desc_t *) KVA_TO_PA (&sqiCmdDesc[0]));
  while (drvData.isTransferDone == false)
    {
      /* Wait for transfer to complete */
    }
  drvData.isTransferDone = false;
  //Command
  sqi_cmd[0] = (uint8_t) SST26_CMD_FLASH_RESET;
  //Command descriptor
  sqiCmdDesc[1].bd_ctrl = (SQI_BDCTRL_BUFFLEN_VAL (1) | SQI_BDCTRL_PKTINTEN | SQI_BDCTRL_LASTPKT | SQI_BDCTRL_LASTBD | h->cs | SQI_BDCTRL_DEASSERT | SQI_BDCTRL_DESCEN);
  sqiCmdDesc[1].bd_bufaddr = (uint32_t *) KVA_TO_PA (&sqi_cmd[0]);
  sqiCmdDesc[1].bd_stat = 0;
  sqiCmdDesc[1].bd_nxtptr = NULL;
  drvData.DMATransfer ((sqi_dma_desc_t *) KVA_TO_PA (&sqiCmdDesc[1]));
  while (drvData.isTransferDone == false)
    {
      /* Wait for transfer to complete */
    }
}

static void enable_quad_io (HANDLE_DATA *h)
{
  drvData.isTransferDone = false;
  drvData.curOpType = DRV_SST26_OPERATION_TYPE_CMD;
  //Command
  sqi_cmd[0] = (uint8_t) SST26_CMD_ENABLE_QUAD_IO;
  //Command descriptor
  sqiCmdDesc[0].bd_ctrl = (SQI_BDCTRL_BUFFLEN_VAL (1) | SQI_BDCTRL_PKTINTEN | SQI_BDCTRL_LASTPKT | SQI_BDCTRL_LASTBD | h->cs | SQI_BDCTRL_DEASSERT | SQI_BDCTRL_DESCEN);
  sqiCmdDesc[0].bd_bufaddr = (uint32_t *) KVA_TO_PA (&sqi_cmd[0]);
  sqiCmdDesc[0].bd_stat = 0;
  sqiCmdDesc[0].bd_nxtptr = NULL;
  //Init DMA transfer
  drvData.DMATransfer ((sqi_dma_desc_t *) KVA_TO_PA (&sqiCmdDesc[0]));
  while (drvData.isTransferDone == false)
    {
      /* Wait for transfer to finish */
    }
}

static void enable_write (HANDLE_DATA *h)
{
  sqi_cmd_wren = (uint8_t) SST26_CMD_WRITE_ENABLE;
  sqiCmdDesc[0].bd_ctrl = (SQI_BDCTRL_BUFFLEN_VAL (1) | DRV_SQI_LANE_MODE | h->cs | SQI_BDCTRL_DEASSERT | SQI_BDCTRL_DESCEN);
  sqiCmdDesc[0].bd_bufaddr = (uint32_t *) KVA_TO_PA (&sqi_cmd_wren);
  sqiCmdDesc[0].bd_stat = 0;
  sqiCmdDesc[0].bd_nxtptr = (sqi_dma_desc_t *) KVA_TO_PA (&sqiCmdDesc[1]);
}

static void unlock_flash (HANDLE_DATA *h)
{
  drvData.isTransferDone = false;
  drvData.curOpType = DRV_SST26_OPERATION_TYPE_CMD;
  enable_write (h);
  sqi_cmd[0] = (uint8_t) SST26_CMD_UNPROTECT_GLOBAL;
  sqiCmdDesc[1].bd_ctrl = (SQI_BDCTRL_BUFFLEN_VAL (1) | SQI_BDCTRL_PKTINTEN | SQI_BDCTRL_LASTPKT | SQI_BDCTRL_LASTBD | DRV_SQI_LANE_MODE | h->cs | SQI_BDCTRL_DEASSERT | SQI_BDCTRL_DESCEN);
  sqiCmdDesc[1].bd_bufaddr = (uint32_t *) KVA_TO_PA (&sqi_cmd[0]);
  sqiCmdDesc[1].bd_stat = 0;
  sqiCmdDesc[1].bd_nxtptr = NULL;
  drvData.DMATransfer ((sqi_dma_desc_t *) KVA_TO_PA (&sqiCmdDesc[0]));
  while (drvData.isTransferDone == false)
    {
      /* Wait for  transfer to complete */
    }
}

static void erase (HANDLE_DATA *h, uint8_t *instruction, uint32_t length)
{
  drvData.isTransferDone = false;
  drvData.curOpType = DRV_SST26_OPERATION_TYPE_ERASE;
  enable_write (h);
  sqiCmdDesc[1].bd_ctrl = (SQI_BDCTRL_BUFFLEN_VAL (length) | SQI_BDCTRL_PKTINTEN | SQI_BDCTRL_LASTPKT | SQI_BDCTRL_LASTBD | DRV_SQI_LANE_MODE | SQI_BDCTRL_SCHECK | h->cs | SQI_BDCTRL_DEASSERT | SQI_BDCTRL_DESCEN);
  sqiCmdDesc[1].bd_bufaddr = (uint32_t *) KVA_TO_PA (instruction);
  sqiCmdDesc[1].bd_stat = 0;
  sqiCmdDesc[1].bd_nxtptr = NULL;
  //Init DMA transfer
  drvData.DMATransfer ((sqi_dma_desc_t *) KVA_TO_PA (&sqiCmdDesc[0]));
}

static void read_jedec_id (HANDLE_DATA *h)
{
  drvData.isTransferDone = false;
  drvData.curOpType = DRV_SST26_OPERATION_TYPE_READ;
  //command
  sqi_cmd[0] = (uint8_t) SST26_CMD_QUAD_JEDEC_ID_READ;
  sqi_cmd[1] = DUMMY_BYTE;
  //Command descriptor
  sqiCmdDesc[0].bd_ctrl = (SQI_BDCTRL_BUFFLEN_VAL (2) | DRV_SQI_LANE_MODE | h->cs | SQI_BDCTRL_DESCEN);
  sqiCmdDesc[0].bd_bufaddr = (uint32_t *) KVA_TO_PA (&sqi_cmd[0]);
  sqiCmdDesc[0].bd_stat = 0;
  sqiCmdDesc[0].bd_nxtptr = (sqi_dma_desc_t *) KVA_TO_PA (&sqiBufDesc[0]);
  //Buffer descriptor
  sqiBufDesc[0].bd_ctrl = (SQI_BDCTRL_BUFFLEN_VAL (4) | SQI_BDCTRL_PKTINTEN | SQI_BDCTRL_LASTPKT | SQI_BDCTRL_LASTBD | DRV_SQI_LANE_MODE | SQI_BDCTRL_DIR_READ | h->cs | SQI_BDCTRL_DEASSERT | SQI_BDCTRL_DESCEN);
  sqiBufDesc[0].bd_bufaddr = (uint32_t *) KVA_TO_PA (jedecID);
  sqiBufDesc[0].bd_stat = 0;
  sqiBufDesc[0].bd_nxtptr = NULL;
  // Initialize the root buffer descriptor
  drvData.DMATransfer ((sqi_dma_desc_t *) KVA_TO_PA (&sqiCmdDesc[0]));
  while (drvData.isTransferDone == false)
    {
      /* Wait for transfer to complete */
    }
  memcpy (&h->jedecId, jedecID, JEDEC_ID_SIZE);
}

static void fill_geometry (uint32_t flash_size, DRV_SST26_GEOMETRY *geometry)
{
  /* Read block size and number of blocks */
  geometry->read_blockSize = 1;
  geometry->read_numBlocks = flash_size;
  /* Write block size and number of blocks */
  geometry->write_blockSize = DRV_SST26_PAGE_SIZE;
  geometry->write_numBlocks = (flash_size / DRV_SST26_PAGE_SIZE);
  /* Erase block size and number of blocks */
  geometry->erase_blockSize = DRV_SST26_ERASE_BUFFER_SIZE;
  geometry->erase_numBlocks = (flash_size / DRV_SST26_ERASE_BUFFER_SIZE);
  geometry->numReadRegions = 1;
  geometry->numWriteRegions = 1;
  geometry->numEraseRegions = 1;
  geometry->blockStartAddress = DRV_SST26_START_ADDRESS;
}

static void read_security_id (HANDLE_DATA *h)
{
  uint32_t address = 0;

  drvData.isTransferDone = false;
  drvData.curOpType = DRV_SST26_OPERATION_TYPE_READ;
  //Command SQI differ from command SPI
  sqi_cmd[0] = (uint8_t) SST26_CMD_SECURITY_ID_READ;
  sqi_cmd[1] = (uint8_t) (0xFFU & (address >> 8));
  sqi_cmd[2] = (uint8_t) (0xFFU & (address >> 0));
  sqi_cmd[3] = DUMMY_BYTE;
  sqi_cmd[4] = DUMMY_BYTE;
  sqi_cmd[5] = DUMMY_BYTE;
  //Command descriptor
  sqiCmdDesc[0].bd_ctrl = (SQI_BDCTRL_BUFFLEN_VAL (6) | DRV_SQI_LANE_MODE | h->cs | SQI_BDCTRL_DESCEN);
  sqiCmdDesc[0].bd_bufaddr = (uint32_t *) KVA_TO_PA (&sqi_cmd[0]);
  sqiCmdDesc[0].bd_stat = 0;
  sqiCmdDesc[0].bd_nxtptr = (sqi_dma_desc_t *) KVA_TO_PA (&sqiBufDesc[0]);
  //Buffer descriptor
  sqiBufDesc[0].bd_ctrl = (SQI_BDCTRL_BUFFLEN_VAL (SECURITY_ID_SIZE) | SQI_BDCTRL_PKTINTEN | SQI_BDCTRL_LASTPKT | SQI_BDCTRL_LASTBD | DRV_SQI_LANE_MODE | SQI_BDCTRL_DIR_READ | h->cs | SQI_BDCTRL_DEASSERT | SQI_BDCTRL_DESCEN);
  sqiBufDesc[0].bd_bufaddr = (uint32_t *) KVA_TO_PA (securityID);
  sqiBufDesc[0].bd_stat = 0;
  sqiBufDesc[0].bd_nxtptr = NULL;
  // Init DMA trasnfer
  drvData.DMATransfer ((sqi_dma_desc_t *) KVA_TO_PA (&sqiCmdDesc[0]));
  while (drvData.isTransferDone == false)
    {
      /* Wait for transfer to complete */
    }
  memcpy (&h->securityId, securityID, SECURITY_ID_SIZE);
}

static bool check_handle (DRV_HANDLE handle)
{
  if (drvData.status != SYS_STATUS_READY)
    {
      _l ("(DRV-SST26) Driver not initialized!\n");
      return false;
    }
  if (!drvData.isTransferDone)
    {
      _l ("(DRV-SST26) Driver is busy!\n");
      return false;
    }
  if (handle >= DRV_SST26_MAX_HANDLES)
    {
      _l ("(DRV-SST26) Handle %d is invalid!\n", (int) handle);
      return false;
    }
  if (!drvData.handle[handle].opened)
    {
      _l ("(DRV-SST26) Handle %d is not opened!\n", (int) handle);
      return false;
    }

  return true;
}

// *****************************************************************************
// *****************************************************************************
// Section: SST26 Driver Functions
// *****************************************************************************
// *****************************************************************************

int DRV_SST26_Initialize (void)
{
  int i;

  if (drvData.initialized)
    {
      _l ("(DRV-SST26) Driver already initialized!\n");
      return -1;
    }
  drvData.initialized = true;
  drvData.isTransferDone = true;
  drvData.status = SYS_STATUS_READY;
  drvData.curOpType = DRV_SST26_OPERATION_TYPE_NONE;
  for (i = 0; i < DRV_SST26_MAX_HANDLES; i++)
    {
      init_handle_data (&drvData.handle[i]);
    }
#if defined(SQI_BDCTRL_SQICS_CS0)
  drvData.handle[0].cs = SQI_BDCTRL_SQICS_CS0;
#endif
#if defined(SQI_BDCTRL_SQICS_CS1)
  drvData.handle[1].cs = SQI_BDCTRL_SQICS_CS1;
#endif
  drvData.DMATransfer = SQI_DMA_TRANSFER_FUNCTION;
  drvData.RegisterCallback = SQI_DMA_CALLBACK_FUNCTION;
  drvData.RegisterCallback (event_handler, (uintptr_t) & drvData);

  return 0;
}

SYS_STATUS DRV_SST26_Status (void)
{
  return (drvData.status);
}

bool DRV_SST26_UnlockFlash (const DRV_HANDLE handle)
{
  if (check_handle (handle))
    {
      unlock_flash (&drvData.handle[handle]);
      return true;
    }

  return false;
}

t_JEDEC *DRV_SST26_read_jedec_id (const DRV_HANDLE handle)
{
  if (check_handle (handle))
    return &drvData.handle[handle].jedecId;

  return NULL;
}

DRV_SST26_GEOMETRY *DRV_SST26_get_geometry (const DRV_HANDLE handle)
{
  if (check_handle (handle))
    return &drvData.handle[handle].geometry;

  return NULL;
}

t_SECURITY_ID *DRV_SST26_read_security_id (const DRV_HANDLE handle)
{
  if (check_handle (handle))
    return &drvData.handle[handle].securityId;

  return NULL;
}

bool DRV_SST26_ReadStatus (const DRV_HANDLE handle, void *rx_data, uint32_t rx_data_length)
{
  uint8_t *status = (uint8_t *) rx_data;

  if (check_handle (handle))
    {
      drvData.isTransferDone = false;
      sqi_cmd[0] = (uint8_t) SST26_CMD_READ_STATUS_REG;
      sqi_cmd[1] = DUMMY_BYTE;
      sqiCmdDesc[0].bd_ctrl = (SQI_BDCTRL_BUFFLEN_VAL (2) | DRV_SQI_LANE_MODE | drvData.handle[handle].cs | SQI_BDCTRL_DESCEN);
      sqiCmdDesc[0].bd_bufaddr = (uint32_t *) KVA_TO_PA (&sqi_cmd[0]);
      sqiCmdDesc[0].bd_stat = 0;
      sqiCmdDesc[0].bd_nxtptr = (sqi_dma_desc_t *) KVA_TO_PA (&sqiBufDesc[0]);
      sqiBufDesc[0].bd_ctrl = (SQI_BDCTRL_BUFFLEN_VAL (rx_data_length) | SQI_BDCTRL_PKTINTEN | SQI_BDCTRL_LASTPKT | SQI_BDCTRL_LASTBD | DRV_SQI_LANE_MODE | SQI_BDCTRL_DIR_READ | drvData.handle[handle].cs | SQI_BDCTRL_DEASSERT | SQI_BDCTRL_DESCEN);
      sqiBufDesc[0].bd_bufaddr = (uint32_t *) KVA_TO_PA (&statusRegVal);
      sqiBufDesc[0].bd_stat = 0;
      sqiBufDesc[0].bd_nxtptr = NULL;
      drvData.curOpType = DRV_SST26_OPERATION_TYPE_READ;
      // Initialize the root buffer descriptor
      drvData.DMATransfer ((sqi_dma_desc_t *) KVA_TO_PA (&sqiCmdDesc[0]));
      while (drvData.isTransferDone == false)
        {
          /* Wait for transfer to complete */
        }
      *status = statusRegVal;
      return true;
    }

  return false;
}

DRV_HANDLE DRV_SST26_Open (DRV_SST26_MEM_IC_INDEX index)
{
  uint32_t flashSize;

  if (drvData.status != SYS_STATUS_READY)
    {
      _l ("(DRV-SST26) Driver not initialized!\n");
      return DRV_HANDLE_INVALID;
    }
  switch (index)
    {
    case DRV_SST26_MEM_IC_INDEX_0:
#if !defined(SQI_BDCTRL_SQICS_CS0)
      _l ("(DRV-SST26) Mem IC index = %d is not available!\n", index);
      return DRV_HANDLE_INVALID;
#endif
      break;
    case DRV_SST26_MEM_IC_INDEX_1:
#if !defined(SQI_BDCTRL_SQICS_CS1)
      _l ("(DRV-SST26) Mem IC index = %d is not available!\n", index);
      return DRV_HANDLE_INVALID;
#endif
      break;
    default:
      _l ("(DRV-SST26) Mem IC index = %d is invalid!\n", index);
      return DRV_HANDLE_INVALID;
      break;
    }

  drvData.handle[index].opened = false;
  reset_flash (&drvData.handle[index]);
  enable_quad_io (&drvData.handle[index]);
  unlock_flash (&drvData.handle[index]);
  read_jedec_id (&drvData.handle[index]);
  _l ("(DRV-SST26) Index %d, JEDEC-ID:\n   ID: %02X\n   Major: %02X\n   Minor: %02X\n   Table length: %02X\n",
      index,
      drvData.handle[index].jedecId.id,
      drvData.handle[index].jedecId.majorRevisionNumber,
      drvData.handle[index].jedecId.minorRevisionNumber,
      drvData.handle[index].jedecId.tableLength
      );
  flashSize = get_flash_size (drvData.handle[index].jedecId.majorRevisionNumber);
  if (flashSize > 0)
    {
      if (DRV_SST26_START_ADDRESS < flashSize)
        {
          flashSize -= DRV_SST26_START_ADDRESS;
          if (flashSize >= DRV_SST26_ERASE_BUFFER_SIZE)
            {
              fill_geometry (flashSize, &drvData.handle[index].geometry);
              _l ("(DRV-SST26) Index = %d, FLASH GEOMETRY:\n   Block start = %u\n   Erase block size = %u\n   Erase block num = %u\n   Erase region num = %u\n   Read region num = %u\n   Write region num = %u\n   Read block size = %u\n   Read block num = %u\n   Write block size = %u\n   Write block num = %u\n",
                  index,
                  drvData.handle[index].geometry.blockStartAddress,
                  drvData.handle[index].geometry.erase_blockSize,
                  drvData.handle[index].geometry.erase_numBlocks,
                  drvData.handle[index].geometry.numEraseRegions,
                  drvData.handle[index].geometry.numReadRegions,
                  drvData.handle[index].geometry.numWriteRegions,
                  drvData.handle[index].geometry.read_blockSize,
                  drvData.handle[index].geometry.read_numBlocks,
                  drvData.handle[index].geometry.write_blockSize,
                  drvData.handle[index].geometry.write_numBlocks
                  );
              read_security_id (&drvData.handle[index]);
              _l ("(DRV-SST26) Index = %d, security id = %08llX\n", index, drvData.handle[index].securityId);
              drvData.handle[index].opened = true;
              _l ("(DRV-SST26) Index = %d, opened! Handle = %d.\n", index, index);
              return ((DRV_HANDLE) index);
            }
          else
            {
              _l ("(DRV-SST26) Index = %d. Flash size should be at-least of a Erase Block size!\n", index);
            }
        }
      else
        {
          _l ("(DRV-SST26) Index %d. Error: Flash size minor than start address!\n", (int) index);
        }
    }
  else
    {
      _l ("(DRV-SST26) Index %d. Flash size = 0. Error!\n", (int) index);
    }

  return DRV_HANDLE_INVALID;
}

void DRV_SST26_Close (const DRV_HANDLE handle)
{
  if (check_handle (handle))
    {
      init_handle_data (&drvData.handle[handle]);
      _l ("(DRV-SST26) Handle = %d, closed!\n", handle);
    }
}

DRV_SST26_TRANSFER_STATUS DRV_SST26_TransferStatusGet (const DRV_HANDLE handle)
{
  DRV_SST26_TRANSFER_STATUS status = DRV_SST26_TRANSFER_ERROR_UNKNOWN;

  if (check_handle (handle))
    {

      status = drvData.isTransferDone ? DRV_SST26_TRANSFER_COMPLETED : DRV_SST26_TRANSFER_BUSY;
    }

  return status;
}

bool DRV_SST26_Read (const DRV_HANDLE handle, void *rx_data, uint32_t rx_data_length, uint32_t address)
{
  uint32_t pendingBytes = rx_data_length;
  uint8_t *readBuffer = (uint8_t *) rx_data;
  uint32_t numBytes = 0;
  uint32_t i = 0;

  if (check_handle (handle))
    {
      if ((rx_data_length > 0U) || (rx_data_length <= (DRV_SST26_PAGE_SIZE * BUFF_DESC_NUMBER)))
        {
          drvData.isTransferDone = false;
          drvData.curOpType = DRV_SST26_OPERATION_TYPE_READ;
          // Construct parameters to issue read command
          sqi_cmd[0] = (uint8_t) SST26_CMD_HIGH_SPEED_READ;
          sqi_cmd[1] = (uint8_t) (0xFFU & (address >> 16));
          sqi_cmd[2] = (uint8_t) (0xFFU & (address >> 8));
          sqi_cmd[3] = (uint8_t) (0xFFU & (address >> 0));
          sqi_cmd[4] = DUMMY_BYTE;
          sqi_cmd[5] = DUMMY_BYTE;
          sqi_cmd[6] = DUMMY_BYTE;
          sqiCmdDesc[1].bd_ctrl = (SQI_BDCTRL_BUFFLEN_VAL (7) | DRV_SQI_LANE_MODE | drvData.handle[handle].cs | SQI_BDCTRL_DESCEN);
          sqiCmdDesc[1].bd_bufaddr = (uint32_t *) KVA_TO_PA (&sqi_cmd[0]);
          sqiCmdDesc[1].bd_stat = 0;
          sqiCmdDesc[1].bd_nxtptr = (sqi_dma_desc_t *) KVA_TO_PA (&sqiBufDesc[0]);
          while (i < BUFF_DESC_NUMBER)
            {
              if (pendingBytes >= DRV_SST26_PAGE_SIZE)
                {
                  numBytes = DRV_SST26_PAGE_SIZE;
                }
              else
                {
                  numBytes = pendingBytes;
                }
              sqiBufDesc[i].bd_ctrl = (SQI_BDCTRL_BUFFLEN_VAL (numBytes) | SQI_BDCTRL_PKTINTEN | DRV_SQI_LANE_MODE | SQI_BDCTRL_DIR_READ | drvData.handle[handle].cs | SQI_BDCTRL_DESCEN);
              sqiBufDesc[i].bd_bufaddr = (uint32_t *) KVA_TO_PA (readBuffer);
              sqiBufDesc[i].bd_stat = 0;
              sqiBufDesc[i].bd_nxtptr = (sqi_dma_desc_t *) KVA_TO_PA (&sqiBufDesc[i + 1]);
              pendingBytes -= numBytes;
              readBuffer += numBytes;
              i++;
              if (pendingBytes == 0U)
                {
                  break;
                }
            }
          /* The last descriptor must indicate the end of the descriptor list */
          sqiBufDesc[i - 1].bd_ctrl |= (SQI_BDCTRL_LASTPKT | SQI_BDCTRL_LASTBD | SQI_BDCTRL_DEASSERT);
          sqiBufDesc[i - 1].bd_nxtptr = NULL;
          // Initialize the root buffer descriptor
          drvData.DMATransfer ((sqi_dma_desc_t *) KVA_TO_PA (&sqiCmdDesc[0]));
          return true;
        }
      else
        {

          _l ("(DRV-SST26) RX data length = %d, invalid parametrer!\n", rx_data_length);
        }
    }

  return false;
}

bool DRV_SST26_PageWrite (const DRV_HANDLE handle, void *tx_data, uint32_t address)
{
  if (check_handle (handle))
    {

      drvData.isTransferDone = false;
      drvData.curOpType = DRV_SST26_OPERATION_TYPE_WRITE;
      enable_write (&drvData.handle[handle]);
      // Construct parameters to issue page program command
      sqi_cmd[0] = (uint8_t) SST26_CMD_PAGE_PROGRAM;
      sqi_cmd[1] = (uint8_t) (0xFFU & (address >> 16));
      sqi_cmd[2] = (uint8_t) (0xFFU & (address >> 8));
      sqi_cmd[3] = (uint8_t) (0xFFU & (address >> 0));
      //Command descript. Start in index 1 because "enable write" uses index 0
      sqiCmdDesc[1].bd_ctrl = (SQI_BDCTRL_BUFFLEN_VAL (4) | DRV_SQI_LANE_MODE | drvData.handle[handle].cs | SQI_BDCTRL_DESCEN);
      sqiCmdDesc[1].bd_bufaddr = (uint32_t *) KVA_TO_PA (&sqi_cmd[0]);
      sqiCmdDesc[1].bd_stat = 0;
      sqiCmdDesc[1].bd_nxtptr = (sqi_dma_desc_t *) KVA_TO_PA (&sqiBufDesc[0]);
      //Buffer descriptor
      sqiBufDesc[0].bd_ctrl = (SQI_BDCTRL_BUFFLEN_VAL (DRV_SST26_PAGE_SIZE) | SQI_BDCTRL_PKTINTEN | SQI_BDCTRL_LASTPKT | SQI_BDCTRL_LASTBD | DRV_SQI_LANE_MODE | SQI_BDCTRL_SCHECK | drvData.handle[handle].cs | SQI_BDCTRL_DEASSERT | SQI_BDCTRL_DESCEN);
      sqiBufDesc[0].bd_bufaddr = (uint32_t *) KVA_TO_PA ((uint8_t*) tx_data);
      sqiBufDesc[0].bd_stat = 0;
      sqiBufDesc[0].bd_nxtptr = NULL;
      // Initialize the root buffer descriptor
      drvData.DMATransfer ((sqi_dma_desc_t *) KVA_TO_PA (&sqiCmdDesc[0]));
      return true;
    }

  return false;
}

bool DRV_SST26_SectorErase (const DRV_HANDLE handle, uint32_t address)
{
  if (check_handle (handle))
    {
      sqi_cmd[0] = (uint8_t) SST26_CMD_SECTOR_ERASE;
      sqi_cmd[1] = (uint8_t) (0xFFU & (address >> 16));
      sqi_cmd[2] = (uint8_t) (0xFFU & (address >> 8));
      sqi_cmd[3] = (uint8_t) (0xFFU & (address >> 0));
      erase (&drvData.handle[handle], &sqi_cmd[0], 4);
      return true;
    }

  return false;
}

bool DRV_SST26_BulkErase (const DRV_HANDLE handle, uint32_t address)
{
  if (check_handle (handle))
    {
      sqi_cmd[0] = (uint8_t) SST26_CMD_BULK_ERASE_64K;
      sqi_cmd[1] = (uint8_t) (0xFFU & (address >> 16));
      sqi_cmd[2] = (uint8_t) (0xFFU & (address >> 8));
      sqi_cmd[3] = (uint8_t) (0xFFU & (address >> 0));
      erase (&drvData.handle[handle], &sqi_cmd[0], 4);
      return true;
    }

  return false;
}

bool DRV_SST26_ChipErase (const DRV_HANDLE handle)
{
  if (check_handle (handle))
    {
      sqi_cmd[0] = (uint8_t) SST26_CMD_CHIP_ERASE;
      erase (&drvData.handle[handle], &sqi_cmd[0], 1);
      return true;
    }

  return false;
}
