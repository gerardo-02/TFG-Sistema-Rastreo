#ifndef PMZ_STORAGE_H
#define	PMZ_STORAGE_H

#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif

    //    //EE24xx1026
    //#define MEM_ADDR_NET_CFG            0x0000
    //#define MEM_ADDR_NET_CFG_LIMIT      0x007F  //128 bytes
    //
    //#define MEM_ADDR_CFG                0x0080
    //#define MEM_ADDR_CFG_LIMIT          0x02FF  
    //#define MEM_ADDR_RUN_VARS           0x0300
    //#define MEM_ADDR_RUN_VARS_LIMIT     0x04FF  //4 bloques de 128 Bytes            
    //


    //    /* SST26 */
    //    //GAP (4 bloque de 8Kbytes)
    //    //GAP (1 bloque de 32Kbytes)
    //#define MEM_ADDR_USERS              0x010000
    //#define MEM_ADDR_USERS_LIMIT        0x01FFFF    //1 bloques de 64Kbytes
    //    //GAP (13 bloques de 64Kbytes)
    //#define MEM_ADDR_PASSPORTS          0x100000
    //#define MEM_ADDR_PASSPORTS_LIMIT    0x12FFFF    //3 bloques de 64Kbytes
    //    //GAP (29 bloques de 64Kbytes)    
    //#define MEM_ADDR_EVENTS             0x300000    
    //#define MEM_ADDR_EVENTS_LIMIT       0x33FFFF    //4 bloques de 64Kbytes
    //    //GAP (76 bloques de 64Kbytes)    
    //#define MEM_ADDR_H_PROFILES         0x7F0000
    //#define MEM_ADDR_H_PROFILES_LIMIT   0x7F7FFF    
    //    //GAP (? bloques)   


    //    /* MARK BLOCK VALIDATION */
    //#define NET_CFG_BLOCK_STORAGE_MARK  0xA5A1    
    //#define CFG_BLOCK_STORAGE_MARK      0xA5A2
    //#define RV_BLOCK_STORAGE_MARK       0xA350    
    //#define MASK_V_BLOCK_STORAGE_MARK   0x0000FFFF    
    /* BLOCK DATA STATUS */
#define BLOCK_STATUS_ACTIVE         0xB1
#define BLOCK_STATUS_DISABLE        0xB2
#define BLOCK_STATUS_SENT           0xB3
#define MASK_V_BLOCK_STATUS         0x00FF0000
    /* BLOCK DATA ROTATION */
#define BLOCK_ROTATION_EVEN         0x00    
#define BLOCK_ROTATION_ODD          0x01
#define MASK_V_BLOCK_ROTATION       0xFF000000   
    /* ASIGN BLOCK INFO MACRO */
#define BS_TO_V(mark,status,rotation)   ((rotation<<24)|(status<<16)|mark)   

    typedef union {

        struct {
            uint16_t mark;
            uint8_t status;
            uint8_t rotation;
        };
        uint32_t v;
    } t_BLOCK_STORAGE_MARK;

    void storage_init(void);
    void storage_task(void);

#ifdef	__cplusplus
}
#endif

#endif	/* PMZ_STORAGE_H */

