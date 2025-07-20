#include <string.h>
#include <stddef.h>

#include "log.h"

#include "drv_47xxx.h"


#define I2C_OP_CODE_SRAM            0xA0    //SRAM read/write
#define I2C_OP_CODE_CONTROL_REGISER 0x30    //Control register read/write

// *****************************************************************************
// *****************************************************************************
// Section: Global objects
// *****************************************************************************
// *****************************************************************************

static uint8_t readBuff[10];
static uint8_t writeBuff[10];

// *****************************************************************************
// *****************************************************************************
// Section: Local Functions
// *****************************************************************************
// *****************************************************************************

static void i2c_event_handler (uintptr_t context)
{
  switch (DRV_47XXX_I2C_PLIB_ERROR_GET ())
    {
    case I2C_ERROR_NONE:
      _l ("(DRV-47XXX) Event NONE\n");
      break;
    case I2C_ERROR_NACK:
      _l ("(DRV-47XXX) Event NACK\n");
      break;
    case I2C_ERROR_BUS_COLLISION:
      _l ("(DRV-47XXX) Event BUS COLLISION\n");
      break;
    }
}

// *****************************************************************************
// *****************************************************************************
// Section: DRV_AT24 Driver Global Functions
// *****************************************************************************
// *****************************************************************************

void drv_47xxx_initialize (void)
{
  DRV_47XXX_I2C_PLIB_CALLBACK_REG (i2c_event_handler, 0);
  _l ("(DRV-47XXX) Initialized\n");
}

//DRV_HANDLE drv_47xxx_initialize (DRV_47XXX_MEM_IC_INDEX icIndex) {
//  //  DRV_AT24_INIT *at24Init = (DRV_AT24_INIT *) init;
//  /* Validate the request */
//  if (icIndex >= DRV_47XXX_MEM_IC_MAX)
//    {
//      return DRV_HANDLE_INVALID;
//    }
//  if (drvData.inUse)
//    {
//      return DRV_HANDLE_INVALID;
//    }
//
//  drvData.status = SYS_STATUS_UNINITIALIZED;
//  drvData.inUse = true;
//  //  drvData.nClients = 0;
//  drvData.transferStatus = DRV_47XXX_TRANSFER_STATUS_ERROR;
//  //I2C plib
//  drvData.i2cPlib.read = DRV_47XXX_I2C_PLIB_READ;
//  drvData.i2cPlib.write = DRV_47XXX_I2C_PLIB_WRITE;
//  drvData.i2cPlib.write_read = DRV_47XXX_I2C_PLIB_WRITE_READ;
//  drvData.i2cPlib.is_busy = DRV_47XXX_I2C_PLIB_BUSY;
//  drvData.i2cPlib.error_get = DRV_47XXX_I2C_PLIB_ERROR_GET;
//  drvData.i2cPlib.callback_register = DRV_47XXX_I2C_PLIB_CALLBACK_REG;
//  //I2C Slave addresses
//  drvData.slaveAddressRW = 0x50; // A2=0, A1=0 this address is for read write operation
//  drvData.slaveAddressCR = 0x18; // A2=0, A1=0 this address is for control register operation
//  //MEM Sizes
//  //drvData.pageSize = DRV_AT24_EEPROM_PAGE_SIZE;
//  //drvData.flashSize = MEM_FLASH_SIZE;
//  //  drvData.nClientsMax = DRV_AT24_CLIENTS_NUMBER_IDX;
//  //drvData.blockStartAddress = 0x0;
//  drvData.eventHandler = NULL;
//  drvData.context = 0;
//  drvData.i2cPlib.callback_register (i2c_event_handler, 0);
//  /* Update the status */
//  drvData.status = SYS_STATUS_READY;
//
//  return (DRV_HANDLE) icIndex;
//}

DRV_47XXX_STATUS_REGISTER drv_47xxx_status (uint8_t a2a1_addr)
{
  uint8_t control_byte = (I2C_OP_CODE_CONTROL_REGISER >> 1) | a2a1_addr;
  readBuff[0] = 0xFF;
  writeBuff[0] = 0x00;

  if (DRV_47XXX_I2C_PLIB_BUSY ())
    {
      _l ("(DRV-47XXX) Busy\n");
    }
  else
    {
      DRV_47XXX_I2C_PLIB_WRITE_READ (control_byte, writeBuff, 1, readBuff, 1);
      _l ("(DRV-47XXX) ... %d\n", (int) readBuff[0]);
    }
  return ((DRV_47XXX_STATUS_REGISTER) readBuff[0]);
}

//DRV_HANDLE drv_47xxx_open (SYS_MODULE_INDEX drvIndex)
//{
//  uint8_t status;
//  
//  if ((drvIndex >= DRV_47XXX_MEM_IC_MAX) || (drvData.status != SYS_STATUS_READY) || !drvData.inUse)
//    {
//      return DRV_HANDLE_INVALID;
//    }
//  i2c_read (&status, 1, drvData.slaveAddressCR, 0);
//  drvData.inUse = true;
//
//  return ((DRV_HANDLE) drvIndex);
//}
//
//void drv_47xxx_close (DRV_HANDLE handle)
//{
//  return;
//}
//
//void drv_47xxx_event_handler_set (DRV_HANDLE handle, const DRV_47XXX_EVENT_HANDLER eventHandler, const uintptr_t context)
//{
//  if (check_handle (handle) && (drvData.transferStatus != DRV_47XXX_TRANSFER_STATUS_BUSY))
//    {
//      drvData.eventHandler = eventHandler;
//      drvData.context = context;
//    }
//}
//
//bool drv_47xxx_read (DRV_HANDLE handle, void* rxData, uint32_t rxDataLength, uint32_t address)
//{
//  if (check_handle (handle) && rxData && (rxDataLength > 0) && (drvData.transferStatus != DRV_47XXX_TRANSFER_STATUS_BUSY))
//    {
//      return i2c_read (rxData, rxDataLength, address);
//    }
//
//  return false;
//}
//
//bool drv_47xxx_write (DRV_HANDLE handle, void* txData, uint32_t txDataLength, uint32_t address)
//{
//  if ((handle != DRV_HANDLE_INVALID) && (handle == 0U) && (txData != NULL) && (txDataLength != 0U) && (drvData.transferStatus != DRV_47XXX_TRANSFER_STATUS_BUSY))
//    {
//      return i2c_write (txData, txDataLength, address);
//    }
//
//  return false;
//}
//
//bool drv_47xxx_page_write (DRV_HANDLE handle, void *txData, uint32_t address)
//{
//  return drv_47xxx_write (handle, txData, MEM_PAGE_SIZE, address);
//}
//
//DRV_47XXX_TRANSFER_STATUS drv_47xxx_transfer_get_status (DRV_HANDLE handle)
//{
//  if (check_handle (handle))
//    {
//      return drvData.transferStatus;
//    }
//
//  return DRV_47XXX_TRANSFER_STATUS_ERROR;
//}
//
//bool drv_47xxx_gometry_get (const DRV_HANDLE handle, DRV_47XXX_GEOMETRY *geometry)
//{
//  uint32_t flash_size = 0;
//
//  if ((handle == DRV_HANDLE_INVALID) || (handle > 0U))
//    {
//      return false;
//    }
//  //  flash_size = drvData.flashSize;
//  //  if ((flash_size == 0U) || (MEM_START_BLOCK_ADDRESS >= MEM_FLASH_SIZE))
//  //    {
//  //      return false;
//  //    }
//  flash_size = MEM_FLASH_SIZE - MEM_START_BLOCK_ADDRESS;
//  /* Flash size should be at-least of a Write Block size */
//  if (flash_size < MEM_PAGE_SIZE)
//    {
//      return false;
//    }
//  /* Read block size and number of blocks */
//  geometry->readBlockSize = 1;
//  geometry->readNumBlocks = flash_size;
//  /* Write block size and number of blocks */
//  geometry->writeBlockSize = MEM_PAGE_SIZE;
//  geometry->writeNumBlocks = (flash_size / MEM_PAGE_SIZE);
//  /* Erase block size and number of blocks */
//  geometry->eraseBlockSize = 1;
//  geometry->eraseNumBlocks = flash_size;
//  /* Number of regions */
//  geometry->readNumRegions = 1;
//  geometry->writeNumRegions = 1;
//  geometry->eraseNumRegions = 1;
//  /* Block start address */
//  geometry->blockStartAddress = MEM_START_BLOCK_ADDRESS;
//
//  return true;
//}
