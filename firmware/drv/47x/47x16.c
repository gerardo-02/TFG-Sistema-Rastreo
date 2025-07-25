/*!*****************************************************************************
 * @file    47x16.c
 * @author  Fabien 'Emandhal' MAILLY
 * @version 1.2.0
 * @date    04/06/2023
 * @brief   EERAM47x16 driver
 * @details I2C-Compatible (2-wire) 16-Kbit (2kB x 8) Serial EERAM
 * Follow datasheet 47L04/47C04/47L16/47C16 Rev.C (Jun 2016)
 ******************************************************************************/

//-----------------------------------------------------------------------------
#include "47x16.h"
//-----------------------------------------------------------------------------
#ifdef __cplusplus
#include <cstdint>
extern "C" {
#endif
//-----------------------------------------------------------------------------

#ifdef USE_DYNAMIC_INTERFACE
#  define GET_I2C_INTERFACE  pComp->Eeprom.I2C
#else
#  define GET_I2C_INTERFACE  &pComp->Eeprom.I2C
#endif

//-----------------------------------------------------------------------------

#ifdef USE_EEPROM_GENERICNESS
// 47(L/C)16 EERAM configurations
const EEPROM_Conf EERAM47L16_Conf = { .ChipAddress = 0xA0, .ChipSelect = EEPROM_CHIP_ADDRESS_A2A1, .AddressType = EEPROM_ADDRESS_2Bytes, .PageWriteTime = 25, .PageSize = 2048, .OffsetAddress = 0, .TotalByteSize = 2048, .MaxI2CclockSpeed = 1000000, };
const EEPROM_Conf EERAM47C16_Conf = { .ChipAddress = 0xA0, .ChipSelect = EEPROM_CHIP_ADDRESS_A2A1, .AddressType = EEPROM_ADDRESS_2Bytes, .PageWriteTime = 25, .PageSize = 2048, .OffsetAddress = 0, .TotalByteSize = 2048, .MaxI2CclockSpeed = 1000000, };
#endif

//-----------------------------------------------------------------------------





//=============================================================================
// Prototypes for private functions
//=============================================================================
// Write 1 or 2-bytes address the EERAM47x16 (DO NOT USE DIRECTLY)
static eERRORRESULT __EERAM47x16_WriteAddress(EERAM47x16 *pComp, const uint8_t chipAddr, const uint16_t address, const bool useNonBlocking, const eI2C_TransferType transferType);
// Write data to the EERAM47x16 (DO NOT USE DIRECTLY, use EERAM47x16_WriteSRAMData() or EERAM47x16_WriteRegister() instead)
static eERRORRESULT __EERAM47x16_WriteData(EERAM47x16 *pComp, const uint8_t chipAddr, uint16_t address, const uint8_t* data, size_t size);
//-----------------------------------------------------------------------------
#define EERAM47x16_TIME_DIFF(begin,end)  ( ((end) >= (begin)) ? ((end) - (begin)) : (UINT32_MAX - ((begin) - (end) - 1)) ) // Works only if time difference is strictly inferior to (UINT32_MAX/2) and call often
//-----------------------------------------------------------------------------





//**********************************************************************************************************************************************************
//=============================================================================
// EERAM47x16 initialization
//=============================================================================
eERRORRESULT Init_EERAM47x16(EERAM47x16 *pComp)
{
#ifdef CHECK_NULL_PARAM
  if (pComp == NULL) return ERR__PARAMETER_ERROR;
#endif
  I2C_Interface* pI2C = GET_I2C_INTERFACE;
#if defined(CHECK_NULL_PARAM)
# if defined(USE_DYNAMIC_INTERFACE)
  if (pI2C == NULL) return ERR__PARAMETER_ERROR;
# endif
  if (pI2C->fnI2C_Init == NULL) return ERR__PARAMETER_ERROR;
#endif
  eERRORRESULT Error;

  if (pComp->Eeprom.I2CclockSpeed > EERAM47x16_I2CCLOCK_MAX) return ERR__I2C_FREQUENCY_ERROR;
  Error = pI2C->fnI2C_Init(pI2C, pComp->Eeprom.I2CclockSpeed);
  if (Error != ERR_NONE) return Error; // If there is an error while calling fnI2C_Init() then return the Error

  pComp->Eeprom.InternalConfig = 0;
  return (EERAM47x16_IsReady(pComp) ? ERR_NONE : ERR__NO_DEVICE_DETECTED);
}


//=============================================================================
// Is the EERAM47x16 device ready
//=============================================================================
bool EERAM47x16_IsReady(EERAM47x16 *pComp)
{
#ifdef CHECK_NULL_PARAM
  if (pComp == NULL) return false;
#endif
  I2C_Interface* pI2C = GET_I2C_INTERFACE;
#if defined(CHECK_NULL_PARAM)
# if defined(USE_DYNAMIC_INTERFACE)
  if (pI2C == NULL) return false;
# endif
  if (pI2C->fnI2C_Transfer == NULL) return false;
#endif
  I2CInterface_Packet PacketDesc = I2C_INTERFACE8_NO_DATA_DESC(EERAM47x16_SRAM_CHIPADDRESS_BASE | pComp->Eeprom.AddrA2A1A0);
  return (pI2C->fnI2C_Transfer(pI2C, &PacketDesc) == ERR_NONE); // Send only the chip address and get the Ack flag
}

//-----------------------------------------------------------------------------




//=============================================================================
// [STATIC] Write 2-bytes address the EERAM47x16 (DO NOT USE DIRECTLY)
//=============================================================================
eERRORRESULT __EERAM47x16_WriteAddress(EERAM47x16 *pComp, const uint8_t chipAddr, const uint16_t address, const bool useNonBlocking, const eI2C_TransferType transferType)
{
#ifdef CHECK_NULL_PARAM
  if (pComp == NULL) return ERR__PARAMETER_ERROR;
#endif
  I2C_Interface* pI2C = GET_I2C_INTERFACE;
#if defined(CHECK_NULL_PARAM)
# if defined(USE_DYNAMIC_INTERFACE)
  if (pI2C == NULL) return ERR__PARAMETER_ERROR;
# endif
  if (pI2C->fnI2C_Transfer == NULL) return ERR__PARAMETER_ERROR;
#endif
  const uint8_t AddrBytes = ((chipAddr & EERAM47x16_CHIPADDRESS_BASE_MASK) == EERAM47x16_SRAM_CHIPADDRESS_BASE ? 2 : 1); // If the base chip address is the SRAM then the address is 2 bytes else 1 byte (control registers)
  eERRORRESULT Error;

  //--- Send the address ---
  uint8_t Address[sizeof(uint16_t)] = { (uint8_t)((address >> 8) & 0xFF), (uint8_t)(address & 0xFF) };
  I2CInterface_Packet PacketDesc =
  {
    I2C_MEMBER(Config.Value) (useNonBlocking ? I2C_USE_NON_BLOCKING : I2C_BLOCKING) | I2C_USE_8BITS_ADDRESS | I2C_ENDIAN_TRANSFORM_SET(I2C_NO_ENDIAN_CHANGE) | I2C_TRANSFER_TYPE_SET(transferType),
    I2C_MEMBER(ChipAddr    ) chipAddr & I2C_WRITE_ANDMASK,
    I2C_MEMBER(Start       ) true,
    I2C_MEMBER(pBuffer     ) &Address[2 - AddrBytes],
    I2C_MEMBER(BufferSize  ) AddrBytes,
    I2C_MEMBER(Stop        ) false,
  };
  Error = pI2C->fnI2C_Transfer(pI2C, &PacketDesc);                  // Transfer the address
  if (Error == ERR__I2C_NACK) return ERR__NOT_READY;                // If the device receive a NAK, then the device is not ready
  if (Error == ERR__I2C_NACK_DATA) return ERR__I2C_INVALID_ADDRESS; // If the device receive a NAK while transferring data, then this is an invalid address
  return Error;
}


//=============================================================================
// Read SRAM data from the EERAM47x16 device
//=============================================================================
eERRORRESULT EERAM47x16_ReadSRAMData(EERAM47x16 *pComp, uint16_t address, uint8_t* data, size_t size)
{
#ifdef CHECK_NULL_PARAM
  if ((pComp == NULL) || (data == NULL)) return ERR__PARAMETER_ERROR;
#endif
  I2C_Interface* pI2C = GET_I2C_INTERFACE;
#if defined(CHECK_NULL_PARAM)
# if defined(USE_DYNAMIC_INTERFACE)
  if (pI2C == NULL) return ERR__PARAMETER_ERROR;
# endif
  if (pI2C->fnI2C_Transfer == NULL) return ERR__PARAMETER_ERROR;
#endif
  if ((address + (uint16_t)size) > EERAM47x16_EERAM_SIZE) return ERR__OUT_OF_MEMORY;
  const uint8_t ChipAddr = ((EERAM47x16_SRAM_CHIPADDRESS_BASE | pComp->Eeprom.AddrA2A1A0) & EERAM47x16_CHIPADDRESS_MASK);
  eERRORRESULT Error;

  //--- Read data ---
  Error = __EERAM47x16_WriteAddress(pComp, ChipAddr, address, false, I2C_WRITE_THEN_READ_FIRST_PART); // Start a write at address with the device
  if (Error == ERR_NONE)                                                                              // If there is no error while writing address then
  {
    I2CInterface_Packet PacketDesc = I2C_INTERFACE8_RX_DATA_DESC(ChipAddr, true, data, size, true, I2C_WRITE_THEN_READ_SECOND_PART);
    Error = pI2C->fnI2C_Transfer(pI2C, &PacketDesc);                                                  // Restart a read transfer, get the data and stop transfer
  }
  return Error;
}


//=============================================================================
// Read Control register from the EERAM47x16 device
//=============================================================================
eERRORRESULT EERAM47x16_ReadRegister(EERAM47x16 *pComp, uint8_t* data)
{
#ifdef CHECK_NULL_PARAM
  if ((pComp == NULL) || (data == NULL)) return ERR__PARAMETER_ERROR;
#endif
  I2C_Interface* pI2C = GET_I2C_INTERFACE;
#if defined(CHECK_NULL_PARAM)
# if defined(USE_DYNAMIC_INTERFACE)
  if (pI2C == NULL) return ERR__PARAMETER_ERROR;
# endif
  if (pI2C->fnI2C_Transfer == NULL) return ERR__PARAMETER_ERROR;
#endif
  const uint8_t ChipAddr = ((EERAM47x16_REG_CHIPADDRESS_BASE | pComp->Eeprom.AddrA2A1A0) & EERAM47x16_CHIPADDRESS_MASK);
  //--- Read data from I2C ---
  I2CInterface_Packet PacketDesc = I2C_INTERFACE8_RX_DATA_DESC(ChipAddr, true, data, 1, true, I2C_SIMPLE_TRANSFER);
  return pI2C->fnI2C_Transfer(pI2C, &PacketDesc); // Start a read transfer, get the data and stop transfer
}


//=============================================================================
// Read SRAM data with DMA from the EERAM47x16 device
//=============================================================================
eERRORRESULT EERAM47x16_ReadSRAMDataWithDMA(EERAM47x16 *pComp, uint16_t address, uint8_t* data, size_t size)
{
#ifdef CHECK_NULL_PARAM
  if (pComp == NULL) return ERR__PARAMETER_ERROR;
#endif
  I2C_Interface* pI2C = GET_I2C_INTERFACE;
#if defined(CHECK_NULL_PARAM)
# if defined(USE_DYNAMIC_INTERFACE)
  if (pI2C == NULL) return ERR__PARAMETER_ERROR;
# endif
  if (pI2C->fnI2C_Transfer == NULL) return ERR__PARAMETER_ERROR;
#endif
  if ((address + (uint16_t)size) > EERAM47x16_EERAM_SIZE) return ERR__OUT_OF_MEMORY;
  const uint8_t ChipAddr = ((EERAM47x16_SRAM_CHIPADDRESS_BASE | pComp->Eeprom.AddrA2A1A0) & EERAM47x16_CHIPADDRESS_MASK);
  eERRORRESULT Error;

  //--- Check DMA ---
  if (EERAM47x16_IS_DMA_TRANSFER_IN_PROGRESS(pComp->Eeprom.InternalConfig))
  {
    const uint16_t CurrTransactionNumber = EERAM47x16_TRANSACTION_NUMBER_GET(pComp->Eeprom.InternalConfig);
    I2CInterface_Packet PacketDescCheck = I2C_INTERFACE8_CHECK_DMA_DESC(ChipAddr, CurrTransactionNumber);
    Error = pI2C->fnI2C_Transfer(pI2C, &PacketDescCheck);                                            // Send only the chip address and get the Ack flag, to return the status of the current transfer
    if ((Error != ERR__I2C_BUSY) && (Error != ERR__I2C_OTHER_BUSY)) pComp->Eeprom.InternalConfig &= EERAM47x16_NO_DMA_TRANSFER_IN_PROGRESS_SET;
    return Error;
  }

  //--- Read data ---
  Error = __EERAM47x16_WriteAddress(pComp, ChipAddr, address, true, I2C_WRITE_THEN_READ_FIRST_PART); // Start a read at address with the device
  if (Error == ERR_NONE)                                                                             // If there is no error while writing address then
  {
    I2CInterface_Packet PacketDescData = I2C_INTERFACE8_RX_DATA_DESC(ChipAddr, true, data, size, true, I2C_WRITE_THEN_READ_SECOND_PART);
    Error = pI2C->fnI2C_Transfer(pI2C, &PacketDescData);                                             // Restart at first data read transfer, get the data and stop transfer at last data
    if (Error != ERR__I2C_OTHER_BUSY) pComp->Eeprom.InternalConfig &= EERAM47x16_NO_DMA_TRANSFER_IN_PROGRESS_SET;
    if (Error == ERR__I2C_BUSY) pComp->Eeprom.InternalConfig |= EERAM47x16_DMA_TRANSFER_IN_PROGRESS;
    EERAM47x16_TRANSACTION_NUMBER_CLEAR(pComp->Eeprom.InternalConfig);
    pComp->Eeprom.InternalConfig |= EERAM47x16_TRANSACTION_NUMBER_SET(I2C_TRANSACTION_NUMBER_GET(PacketDescData.Config.Value));
  }
  return Error;
}

//-----------------------------------------------------------------------------



//=============================================================================
// [STATIC] Write data to the EERAM47x16 (DO NOT USE DIRECTLY, use EERAM47x16_WriteSRAMData() or EERAM47x16_WriteRegister() instead)
//=============================================================================
eERRORRESULT __EERAM47x16_WriteData(EERAM47x16 *pComp, const uint8_t chipAddr, uint16_t address, const uint8_t* data, size_t size)
{
#ifdef CHECK_NULL_PARAM
  if ((pComp == NULL) || (data == NULL)) return ERR__PARAMETER_ERROR;
#endif
  I2C_Interface* pI2C = GET_I2C_INTERFACE;
#if defined(CHECK_NULL_PARAM)
# if defined(USE_DYNAMIC_INTERFACE)
  if (pI2C == NULL) return ERR__PARAMETER_ERROR;
# endif
  if (pI2C->fnI2C_Transfer == NULL) return ERR__PARAMETER_ERROR;
#endif
  if ((address + (uint16_t)size) > EERAM47x16_EERAM_SIZE) return ERR__OUT_OF_MEMORY;
  const uint8_t ChipAddr = (chipAddr | pComp->Eeprom.AddrA2A1A0) & EERAM47x16_CHIPADDRESS_MASK;
  uint8_t* pData = (uint8_t*)data;
  eERRORRESULT Error;

  //--- Write data ---
  Error = __EERAM47x16_WriteAddress(pComp, ChipAddr, address, false, I2C_WRITE_THEN_WRITE_FIRST_PART); // Start a write at address with the device
  if (Error == ERR_NONE)                                                                               // If there is no error while writing address then
  {
    I2CInterface_Packet PacketDesc = I2C_INTERFACE8_TX_DATA_DESC(ChipAddr, false, pData, size, true, I2C_WRITE_THEN_WRITE_SECOND_PART);
    Error = pI2C->fnI2C_Transfer(pI2C, &PacketDesc); // Continue the transfer by sending the data and stop transfer
  }
  return Error;
}


//=============================================================================
// Write SRAM data to the EERAM47x16 device
//=============================================================================
eERRORRESULT EERAM47x16_WriteSRAMData(EERAM47x16 *pComp, uint16_t address, const uint8_t* data, size_t size)
{
  return __EERAM47x16_WriteData(pComp, EERAM47x16_SRAM_CHIPADDRESS_BASE, address, data, size);
}


//=============================================================================
// Write Control register to the EERAM47x16 device
//=============================================================================
eERRORRESULT EERAM47x16_WriteRegister(EERAM47x16 *pComp, uint8_t address, const uint8_t data)
{
  eERRORRESULT Error = __EERAM47x16_WriteData(pComp, EERAM47x16_REG_CHIPADDRESS_BASE, address, &data, 1);
  if (Error == ERR__I2C_NACK_DATA) Error = ERR__I2C_INVALID_COMMAND; // Here a NACK indicate that the command (aka 'data') is invalid. It cannot be anything else
  return Error;
}


//=============================================================================
// Write SRAM data with DMA to the EERAM47x16 device
//=============================================================================
eERRORRESULT EERAM47x16_WriteSRAMDataWithDMA(EERAM47x16 *pComp, uint16_t address, const uint8_t* data, size_t size)
{
#ifdef CHECK_NULL_PARAM
  if (pComp == NULL) return ERR__PARAMETER_ERROR;
#endif
  I2C_Interface* pI2C = GET_I2C_INTERFACE;
#if defined(CHECK_NULL_PARAM)
# if defined(USE_DYNAMIC_INTERFACE)
  if (pI2C == NULL) return ERR__PARAMETER_ERROR;
# endif
  if (pI2C->fnI2C_Transfer == NULL) return ERR__PARAMETER_ERROR;
#endif
  if ((address + (uint16_t)size) > EERAM47x16_EERAM_SIZE) return ERR__OUT_OF_MEMORY;
  const uint8_t ChipAddr = ((EERAM47x16_SRAM_CHIPADDRESS_BASE | pComp->Eeprom.AddrA2A1A0) & EERAM47x16_CHIPADDRESS_MASK);
  uint8_t* pData = (uint8_t*)data;
  eERRORRESULT Error;

  //--- Check DMA ---
  if (EERAM47x16_IS_DMA_TRANSFER_IN_PROGRESS(pComp->Eeprom.InternalConfig))
  {
    const uint16_t CurrTransactionNumber = EERAM47x16_TRANSACTION_NUMBER_GET(pComp->Eeprom.InternalConfig);
    I2CInterface_Packet PacketDescCheck = I2C_INTERFACE8_CHECK_DMA_DESC(ChipAddr, CurrTransactionNumber);
    Error = pI2C->fnI2C_Transfer(pI2C, &PacketDescCheck);                                             // Send only the chip address and get the Ack flag, to return the status of the current transfer
    if ((Error != ERR__I2C_BUSY) && (Error != ERR__I2C_OTHER_BUSY)) pComp->Eeprom.InternalConfig &= EERAM47x16_NO_DMA_TRANSFER_IN_PROGRESS_SET;
    return Error;
  }

  //--- Write data ---
  Error = __EERAM47x16_WriteAddress(pComp, ChipAddr, address, true, I2C_WRITE_THEN_WRITE_FIRST_PART); // Start a read at address with the device
  if (Error == ERR_NONE)                                                                              // If there is no error while writing address then
  {
    I2CInterface_Packet PacketDescData = I2C_INTERFACE8_TX_DATA_DESC(ChipAddr, true, pData, size, true, I2C_WRITE_THEN_WRITE_SECOND_PART);
    Error = pI2C->fnI2C_Transfer(pI2C, &PacketDescData);                                              // Restart at first data read transfer, get the data and stop transfer at last data
    if (Error != ERR__I2C_OTHER_BUSY) pComp->Eeprom.InternalConfig &= EERAM47x16_NO_DMA_TRANSFER_IN_PROGRESS_SET;
    if (Error == ERR__I2C_BUSY) pComp->Eeprom.InternalConfig |= EERAM47x16_DMA_TRANSFER_IN_PROGRESS;
    EERAM47x16_TRANSACTION_NUMBER_CLEAR(pComp->Eeprom.InternalConfig);
    pComp->Eeprom.InternalConfig |= EERAM47x16_TRANSACTION_NUMBER_SET(I2C_TRANSACTION_NUMBER_GET(PacketDescData.Config.Value));
  }
  return Error;
}

//-----------------------------------------------------------------------------



//=============================================================================
// Store all the SRAM to the EEPROM of the EERAM47x16 device
//=============================================================================
eERRORRESULT EERAM47x16_StoreSRAMtoEEPROM(EERAM47x16 *pComp, bool forceStore, bool waitEndOfStore)
{
#ifdef CHECK_NULL_PARAM
  if (pComp->Eeprom.fnGetCurrentms == NULL) return ERR__PARAMETER_ERROR;
#endif
  eERRORRESULT Error = ERR_NONE;
  EERAM47x16_Status_Register Reg;
  bool Store = forceStore;

  //--- Check the need of store operation ---
  if (forceStore == false)                                                           // Check register Status.AM if the store is not forced
  {
    Error = EERAM47x16_ReadRegister(pComp, &Reg.Status);                             // Get the status register
    if (Error != ERR_NONE) return Error;                                             // If there is an error while reading register, then return the error
    Store = ((Reg.Status & EERAM47x16_ARRAY_MODIFIED) > 0);                          // If the array has been modified, flag the store
  }
  //--- Send the store operation if necessary or asked ---
  if (Store)
  {
    uint8_t Data = EERAM47x16_STORE_COMMAND;
    Error = EERAM47x16_WriteRegister(pComp, EERAM47x16_COMMAND_REGISTER_ADDR, Data); // Execute a store
    if (Error != ERR_NONE) return Error;                                             // If there is an error while reading register, then return the error
    //--- Wait the end of store if asked ---
    if (waitEndOfStore)
    {
      Reg.Status = EERAM47x16_ARRAY_MODIFIED;
      uint32_t StartTime = pComp->Eeprom.fnGetCurrentms();                           // Start the timeout
      while (true)
      {
        Error = EERAM47x16_ReadRegister(pComp, &Reg.Status);                         // Get the status register
        if ((Error != ERR_NONE) && (Error != ERR__I2C_NACK)) return Error;           // If there is an error while calling EERAM47x16_ReadRegister() then return the error
        if ((Reg.Status & EERAM47x16_ARRAY_MODIFIED) == 0) break;                    // The store is finished, all went fine
        if (EERAM47x16_TIME_DIFF(StartTime, pComp->Eeprom.fnGetCurrentms()) > EERAM47x16_STORE_TIMEOUT + 1) // Wait at least STORE_TIMEOUT + 1ms because GetCurrentms can be 1 cycle before the new ms
        {
          return ERR__DEVICE_TIMEOUT;                                                // Timeout? return the error
        }
      }
    }
  }
  return Error;
}


//=============================================================================
// Recall all data from EEPROM to SRAM of the EERAM47x16 device
//=============================================================================
eERRORRESULT EERAM47x16_RecallEEPROMtoSRAM(EERAM47x16 *pComp, bool waitEndOfRecall)
{
#ifdef CHECK_NULL_PARAM
  if (pComp->Eeprom.fnGetCurrentms == NULL) return ERR__PARAMETER_ERROR;
#endif
  eERRORRESULT Error = ERR_NONE;
  EERAM47x16_Status_Register Reg;

  //--- Send a recall operation ---
  uint8_t Data = EERAM47x16_RECALL_COMMAND;
  Error = EERAM47x16_WriteRegister(pComp, EERAM47x16_COMMAND_REGISTER_ADDR, Data); // Execute a recall
  if (Error != ERR_NONE) return Error;                                             // If there is an error while reading register, then return the error
  //--- Wait the end of recall if asked ---
  if (waitEndOfRecall)
  {
    Reg.Status = EERAM47x16_ARRAY_MODIFIED;
    uint32_t StartTime = pComp->Eeprom.fnGetCurrentms();                           // Start the timeout
    while (true)
    {
      Error = EERAM47x16_ReadRegister(pComp, &Reg.Status);                         // Get the status register
      if ((Error != ERR_NONE) && (Error != ERR__I2C_NACK)) return Error;           // If there is an error while calling EERAM47x16_ReadRegister() then return the error
      if ((Reg.Status & EERAM47x16_ARRAY_MODIFIED) == 0) break;                    // The store is finished, all went fine
      if (EERAM47x16_TIME_DIFF(StartTime, pComp->Eeprom.fnGetCurrentms()) > EERAM47x16_RECALL_TIMEOUT + 1) // Wait at least RECALL_TIMEOUT + 1ms because GetCurrentms can be 1 cycle before the new ms
      {
        return ERR__DEVICE_TIMEOUT;                                                // Timeout? return the error
      }
    }
  }
  return ERR_NONE;
}

//-----------------------------------------------------------------------------



//=============================================================================
// Activate the Auto-Store of the EERAM47x16 device
//=============================================================================
eERRORRESULT EERAM47x16_ActivateAutoStore(EERAM47x16 *pComp)
{
  eERRORRESULT Error = ERR_NONE;
  EERAM47x16_Status_Register Reg;

  //--- Get the status register ---
  Error = EERAM47x16_ReadRegister(pComp, &Reg.Status);                                 // Get the status register
  if (Error != ERR_NONE) return Error;                                                 // If there is an error while sending data to I2C then stop the transfer
  //--- Set the status register ---
  Reg.Status |= EERAM47x16_ASE_ENABLE;                                                 // Set the Auto-Store flag
  return EERAM47x16_WriteRegister(pComp, EERAM47x16_STATUS_REGISTER_ADDR, Reg.Status); // Save the status register
}


//=============================================================================
// Deactivate the Auto-Store of the EERAM47x16 device
//=============================================================================
eERRORRESULT EERAM47x16_DeactivateAutoStore(EERAM47x16 *pComp)
{
  eERRORRESULT Error = ERR_NONE;
  EERAM47x16_Status_Register Reg;

  //--- Get the status register ---
  Error = EERAM47x16_ReadRegister(pComp, &Reg.Status);                                 // Get the status register
  if (Error != ERR_NONE) return Error;                                                 // If there is an error while sending data to I2C then stop the transfer
  //--- Set the status register ---
  Reg.Status &= ~EERAM47x16_ASE_ENABLE;                                                // Unset the Auto-Store flag
  return EERAM47x16_WriteRegister(pComp, EERAM47x16_STATUS_REGISTER_ADDR, Reg.Status); // Save the status register
}

//-----------------------------------------------------------------------------



//=============================================================================
// Set block write protect of the EERAM47x16 device
//=============================================================================
eERRORRESULT EERAM47x16_SetBlockWriteProtect(EERAM47x16 *pComp, eEERAM47x16_BlockProtect blockProtect)
{
  eERRORRESULT Error = ERR_NONE;
  EERAM47x16_Status_Register Reg;

  //--- Get the status register ---
  Error = EERAM47x16_ReadRegister(pComp, &Reg.Status);                                 // Get the status register
  if (Error != ERR_NONE) return Error;                                                 // If there is an error while sending data to I2C then stop the transfer
  //--- Set the status register ---
  Reg.Bits.BP = (uint8_t)blockProtect;                                                 // Set the block protect
  return EERAM47x16_WriteRegister(pComp, EERAM47x16_STATUS_REGISTER_ADDR, Reg.Status); // Save the status register
}

//-----------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif
//-----------------------------------------------------------------------------