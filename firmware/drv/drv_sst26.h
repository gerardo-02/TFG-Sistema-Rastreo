#ifndef DRV_SST26_H
#define DRV_SST26_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "toolchain_specifics.h"
#include "driver/driver_common.h"
#include "system/system.h"

#ifdef __cplusplus  // Provide C++ Compatibility
extern "C" {
#endif

    typedef enum {
        DRV_SST26_MEM_IC_INDEX_0 = 0,
        DRV_SST26_MEM_IC_INDEX_1 = 1,
        //
        DRV_SST26_MEM_IC_MAX,
    } DRV_SST26_MEM_IC_INDEX;

    /* This data type will be used to indicate the current transfer status for SST26 driver. */
    typedef enum {
        DRV_SST26_TRANSFER_BUSY, // Transfer is being processed
        DRV_SST26_TRANSFER_COMPLETED, //Transfer is successfully completed
        DRV_SST26_TRANSFER_ERROR_UNKNOWN, //Transfer had error
    } DRV_SST26_TRANSFER_STATUS;

    typedef union {

        struct {
            uint8_t id;
            uint8_t minorRevisionNumber;
            uint8_t majorRevisionNumber;
            uint8_t tableLength;
        };
        uint32_t v;
    } t_JEDEC;

    typedef uint64_t t_SECURITY_ID;

    /* This data type will be used to get the geometry details of the SST26 flash device. */
    typedef struct {
        uint32_t read_blockSize;
        uint32_t read_numBlocks;
        uint32_t numReadRegions;
        uint32_t write_blockSize;
        uint32_t write_numBlocks;
        uint32_t numWriteRegions;
        uint32_t erase_blockSize;
        uint32_t erase_numBlocks;
        uint32_t numEraseRegions;
        uint32_t blockStartAddress;
    } DRV_SST26_GEOMETRY;


    typedef void (*DRV_SST26_EVENT_HANDLER) (DRV_SST26_TRANSFER_STATUS event, uintptr_t context);

    int DRV_SST26_Initialize(void);
    SYS_STATUS DRV_SST26_Status(void);
    DRV_HANDLE DRV_SST26_Open(DRV_SST26_MEM_IC_INDEX index);
    void DRV_SST26_Close(const DRV_HANDLE handle);
    bool DRV_SST26_UnlockFlash(const DRV_HANDLE handle);
    t_JEDEC *DRV_SST26_read_jedec_id(const DRV_HANDLE handle);
    DRV_SST26_GEOMETRY *DRV_SST26_get_geometry(const DRV_HANDLE handle);
    t_SECURITY_ID *DRV_SST26_read_security_id(const DRV_HANDLE handle);
    bool DRV_SST26_ReadStatus(const DRV_HANDLE handle, void *rx_data, uint32_t rx_data_length);
    DRV_SST26_TRANSFER_STATUS DRV_SST26_TransferStatusGet(const DRV_HANDLE handle);
    bool DRV_SST26_Read(const DRV_HANDLE handle, void *rx_data, uint32_t rx_data_length, uint32_t address);
    bool DRV_SST26_PageWrite(const DRV_HANDLE handle, void *tx_data, uint32_t address);
    bool DRV_SST26_SectorErase(const DRV_HANDLE handle, uint32_t address);
    bool DRV_SST26_BulkErase(const DRV_HANDLE handle, uint32_t address);
    bool DRV_SST26_ChipErase(const DRV_HANDLE handle);

#ifdef __cplusplus
}
#endif

#endif // #ifndef DRV_SST26_H
/**************************************************************** End of File */
