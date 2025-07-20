#ifndef DRV_47XXX_H
#define DRV_47XXX_H

#include <stdio.h>
#include <stdbool.h>
#include <device.h>

#include "driver/driver.h"
#include "system/system.h"
#include "configuration.h"

/* Particular I2C PLIB functions */
#include "peripheral/i2c/master/plib_i2c_master_common.h"
#include "peripheral/i2c/master/plib_i2c1_master.h"
#define DRV_47XXX_I2C_PLIB_WRITE_READ   I2C1_WriteRead
#define DRV_47XXX_I2C_PLIB_WRITE        I2C1_Write
#define DRV_47XXX_I2C_PLIB_READ         I2C1_Read
#define DRV_47XXX_I2C_PLIB_BUSY         I2C1_IsBusy
#define DRV_47XXX_I2C_PLIB_ERROR_GET    I2C1_ErrorGet
#define DRV_47XXX_I2C_PLIB_CALLBACK_REG I2C1_CallbackRegister


// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility
extern "C" {
#endif

    // DOM-IGNORE-END

    // *****************************************************************************
    // *****************************************************************************
    // Section: Data Types
    // *****************************************************************************
    // *****************************************************************************
    // *****************************************************************************

    typedef union {

        struct {
            unsigned event : 1; // Event Detect bit: 1 = An event was detected on the HS pin, 0 = No event was detected on the HS pin
            unsigned ase : 1; //Auto-Store Enable bit: 1 = Auto-Store feature is enabled, 0 = Auto-Store feature is disabled
            unsigned bp : 3; //Block Protect bits: 000 = Entire array is unprotected, 001 = Upper 1/64 of array is write-protected, 010 = Upper 1/32 of array is write-protected, 011 = Upper 1/16 of array is write-protected, 100 = Upper 1/8 of array is write-protected, 101 = Upper 1/4 of array is write-protected, 110 = Upper 1/2 of array is write-protected, 111 = Entire array is write-protected
            unsigned : 2; //Unimplemented: Read as 0             
            unsigned am : 1; //Array Modified bit: 1 = SRAM array has been modified, 0 = SRAM array has not been modified
        };
        uint8_t val;
    } DRV_47XXX_STATUS_REGISTER;

    //    typedef enum {
    //        DRV_47XXX_MEM_IC_INDEX_0 = 0,
    //        //
    //        DRV_47XXX_MEM_IC_MAX,
    //    } DRV_47XXX_MEM_IC_INDEX;

    //    typedef enum {
    //        DRV_47XXX_TRANSFER_STATUS_BUSY,
    //        DRV_47XXX_TRANSFER_STATUS_COMPLETED,
    //        DRV_47XXX_TRANSFER_STATUS_ERROR
    //    } DRV_47XXX_TRANSFER_STATUS;
    //
    //    typedef struct {
    //        uint32_t readBlockSize;
    //        uint32_t readNumBlocks;
    //        uint32_t readNumRegions;
    //        uint32_t writeBlockSize;
    //        uint32_t writeNumBlocks;
    //        uint32_t writeNumRegions;
    //        uint32_t eraseBlockSize;
    //        uint32_t eraseNumBlocks;
    //        uint32_t eraseNumRegions;
    //        uint32_t blockStartAddress;
    //    } DRV_47XXX_GEOMETRY;

    // *****************************************************************************
    // *****************************************************************************
    // Section: DRV_47XXX Driver Module Interface Routines
    // *****************************************************************************
    // *****************************************************************************

    // *****************************************************************************
    typedef void (*DRV_47XXX_EVENT_CB)(I2C_ERROR i2cErr, uintptr_t context);

    //bool drv_47xxx_initialize(uint8_t a2a1_addr);
    void drv_47xxx_initialize(void);
    bool drv_47xxx_read_status_request(DRV_47XXX_EVENT_CB respCB);
    //    DRV_HANDLE drv_47xxx_open(SYS_MODULE_INDEX drvIndex);
    //    void drv_47xxx_close(DRV_HANDLE handle);
    //    void drv_47xxx_event_handler_set(DRV_HANDLE handle, const DRV_47XXX_EVENT_HANDLER eventHandler, const uintptr_t context);
    //    bool drv_47xxx_read(DRV_HANDLE handle, void* rxData, uint32_t rxDataLength, uint32_t address);
    //    bool drv_47xxx_write(DRV_HANDLE handle, void* txData, uint32_t txDataLength, uint32_t address);
    //    bool drv_47xxx_page_write(DRV_HANDLE handle, void *txData, uint32_t address);
    //    DRV_47XXX_TRANSFER_STATUS drv_47xxx_transfer_get_status(DRV_HANDLE handle);
    //    bool drv_47xxx_gometry_get(const DRV_HANDLE handle, DRV_47XXX_GEOMETRY *geometry);
    
    DRV_47XXX_STATUS_REGISTER drv_47xxx_status (uint8_t a2a1_addr);

#ifdef __cplusplus
}
#endif

#endif // #ifndef DRV_47XXX_H
/**************************************************************** End of File */
