/* 
 * File:   drv_mcp7940n.c
 * Author: JT
 *
 * Created on 10 de agosto de 2020, 10:14
 */

#include <stddef.h>
#include <string.h>

#include "system/time/sys_time.h"
#include "peripheral/i2c/master/plib_i2c5_master.h"

#include "drv_mcp7940x.h"
#include "log.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */
#define REG_SECONDS             0x00    //<7:7> ST <6:4> 10Seconds <3:0> Seconds
#define REG_MINUTES             0x01    //<6:4> 10Minutes <3:0> Minutes
#define REG_HOURS               0x02    //<6:6> 12/24 <5:4> 10Hours/AM_PM <3:0> Hours
#define REG_DAY                 0x03    //<5:5> OSCON <4:4> VBAT <3:3> VBATEN <2:0> Day (Day of week)
#define REG_DATE                0x04    //<5:4> 10Date <3:0> Date (Day of month)
#define REG_MONTH               0x05    //<5:5> LP <4:4> 10Month <3:0> Month
#define REG_YEAR                0x06    //<7:4> 10Year <3:0> Year
#define REG_CONTROL_CFG         0x07    //<7:7> OUT <6:6> SQWE <5:5> ALM1 <4:4> ALM0 <3:3> EXTOSC <2:2> RS2 <1:1> RS1 <0:0> RS0
#define REG_CALIBRATION         0x08    //Calibration
//#define REG_RESERVED_1          0x09    //Reserved do not use
#define REG_ALM0_SECONDS        0x0A    //<6:4> 10Seconds <3:0> Seconds
#define REG_ALM0_MINUTES        0x0B    //<6:4> 10Minutes <3:0> Minutes
#define REG_ALM0_HOURS          0x0C    //<6:6> 12/24 <5:5> 10Hours AM/PM <4:4> 10Hours <3:0> Hours
#define REG_ALM0_DAY            0x0D    //<7:7> ALM0POL <6:6> ALM0C2 <5:5> ALM0C1 <4:4> ALM0C0 <3:3> ALM0IF <2:0> Day
#define REG_ALM0_DATE           0x0E    //<5:4> 10Date <3:0> Date
#define REG_ALM0_MONTH          0x0F    //<4:4> 10Month <3:0> Month
//#define REG_RESERVED_2          0x10    //Reserved do not use
#define REG_ALM1_SECONDS        0x11    //<6:4> 10Seconds <3:0> Seconds
#define REG_ALM1_MINUTES        0x12    //<6:4> 10Minutes <3:0> Minutes
#define REG_ALM1_HOURS          0x13    //<6:6> 12/24 <5:5> 10Hours AM/PM <4:4> 10Hours <3:0> Hours
#define REG_ALM1_DAY            0x14    //<7:7> ALM1POL <6:6> ALM1C2 <5:5> ALM1C1 <4:4> ALM1C0 <3:3> ALM1IF <2:0> Day
#define REG_ALM1_DATE           0x15    //<5:4> 10Date <3:0> Date
#define REG_ALM1_MONTH          0x16    //<4:4> 10Month <3:0> Month
//#define REG_RESERVED_3          0x17    //Reserved do not use
#define REG_TIMESAVER_1_MINUTES 0x18    //<6:4> 10Minutes <3:0> Minutes
#define REG_TIMESAVER_1_HOURS   0x19    //<6:6> 12/24 <5:5> 10Hours AM/PM <4:4> 10Hours <3:0> Hours
#define REG_TIMESAVER_1_DATE    0x1A    //<5:4> 10Date <3:0> Date
#define REG_TIMESAVER_1_MONTH   0x1B    //<4:4> 10Month <3:0> Month
#define REG_TIMESAVER_2_MINUTES 0x1C    //<6:4> 10Minutes <3:0> Minutes
#define REG_TIMESAVER_2_HOURS   0x1D    //<6:6> 12/24 <5:5> 10Hours AM/PM <4:4> 10Hours <3:0> Hours
#define REG_TIMESAVER_2_DATE    0x1E    //<5:4> 10Date <3:0> Date
#define REG_TIMESAVER_2_MONTH   0x1F    //<4:4> 10Month <3:0> Month

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Data types & Definitions                                          */
/* ************************************************************************** */

/* ************************************************************************** */
typedef enum
{
  BUSI2C_STATE_IDLE,
  BUSI2C_STATE_PRE_INIT,
  BUSI2C_STATE_INIT,
  BUSI2C_STATE_IN_PROGRESS_TX,
  BUSI2C_STATE_IN_PROGRESS_RX,
  BUSI2C_STATE_COMPLETED,
  BUSI2C_STATE_NAK,
  BUSI2C_STATE_COLLISION,
  BUSI2C_STATE_ERROR
} t_BUSI2C_STATE;

typedef struct
{
  uint8_t *tx;
  uint32_t txLength;
  uint8_t *rx;
  uint32_t rxLength;
  t_BUSI2C_STATE state;
  uint16_t slaveAddressControlByte;
} t_I2C_SM_DATA;

#define MCP7940N_SLAVE_ADDRESS_CONTROL_BYTE 0x6F    // 01101111
#define MCP7940N_MAX_BYTE_ADDRESS           0x1F    // Este el la direccion del registro mayor

typedef union
{

  struct
  {
    unsigned seconds1 : 4;
    unsigned seconds10 : 3;
    unsigned st : 1;
    unsigned minutes1 : 4;
    unsigned minutes10 : 3;
    unsigned : 1;
    unsigned hour1 : 4;
    unsigned hour10 : 2;
    unsigned m_12_24 : 1;
    unsigned : 1;
    unsigned day : 3;
    unsigned vbaten : 1;
    unsigned vbat : 1;
    unsigned oscon : 1;
    unsigned : 2;
    unsigned date1 : 4;
    unsigned date10 : 2;
    unsigned : 2;
    unsigned month1 : 4;
    unsigned month10 : 1;
    unsigned lp : 1;
    unsigned : 2;
    unsigned year1 : 4;
    unsigned year10 : 4;
    unsigned rs0 : 1;
    unsigned rs1 : 1;
    unsigned rs2 : 1;
    unsigned extosc : 1;
    unsigned alm0 : 1;
    unsigned alm1 : 1;
    unsigned sqwe : 1;
    unsigned out : 1;
    uint8_t calibration;
  };

  struct
  {
    unsigned : 21;
    unsigned am_pm : 1;
  };

  struct
  {
    uint8_t seconds_00h;
    uint8_t minutes_01h;
    uint8_t hours_02h;
    uint8_t day_03h;
    uint8_t date_04h;
    uint8_t month_05h;
    uint8_t year_06h;
    uint8_t control_reg_07h;
    uint8_t calibration_08h;
  };
  uint8_t s[9];
} t_MCP7940N_REGS_DATE;

typedef struct
{
  uint8_t startAddress;
  uint8_t length;
  uint8_t *data;
} t_RW_PARAMS;

typedef struct
{
  t_MCP7940N_STATES sm_currentState;
  t_RW_PARAMS rwParams;
  t_MCP7940N_REGS_DATE sys;
  struct tm ts_local;
  DRV_MCP7940N_CALL_BACK cb;
} t_MCP7940N_DRV_DATA;

typedef struct
{
  t_MCP7940N_STATES stateOnSuccessful;
  t_MCP7940N_STATES stateOnError;
  void (*subTask)(void*);
  void *context;
  uint32_t state;
} t_SUB_TASK_DATA;

typedef union
{

  struct
  {
    unsigned b1 : 4;
    unsigned b10 : 4;
  };
  uint8_t b;
} t_BCD_BYTE;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Global data & variables                                           */
/* ************************************************************************** */
/* ************************************************************************** */
static t_I2C_SM_DATA i2cData;
static t_MCP7940N_DRV_DATA drvData;
static t_SUB_TASK_DATA subtaskData;
static uint32_t timeDelayms;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Callback Functions                                                */
/* ************************************************************************** */
/* ************************************************************************** */
static void init_cb(t_MCP7940N_CALL_BACK_CONTEXT *ctx)
{
   _l ("(DRV_MCP7940x) Initialized\n");
  drvData.cb = NULL;
}

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */
// <editor-fold defaultstate="collapsed" desc="I2C">

static void i2c_reset (void)
{
  i2cData.rx = i2cData.tx = NULL;
  i2cData.rxLength = i2cData.txLength = 0;
  i2cData.state = BUSI2C_STATE_IDLE;
}

static void i2c_task (void)
{
  switch (i2cData.state)
    {
    case BUSI2C_STATE_IDLE: break;
    case BUSI2C_STATE_PRE_INIT: break;
    case BUSI2C_STATE_INIT:
      {
        if (I2C5_IsBusy ()) return;
        i2cData.state = BUSI2C_STATE_ERROR;
        if (i2cData.tx && i2cData.txLength)
          {
            if (i2cData.rx && i2cData.txLength)
              {
                //WriteRead
                if (I2C5_WriteRead (i2cData.slaveAddressControlByte, i2cData.tx, i2cData.txLength, i2cData.rx, i2cData.rxLength)) i2cData.state = BUSI2C_STATE_IN_PROGRESS_RX;
              }
            else
              {
                //Write
                if (I2C5_Write (i2cData.slaveAddressControlByte, i2cData.tx, i2cData.txLength)) i2cData.state = BUSI2C_STATE_IN_PROGRESS_TX;
              }
          }
      }
      break;
    case BUSI2C_STATE_IN_PROGRESS_TX:
    case BUSI2C_STATE_IN_PROGRESS_RX:
      {
        if (!I2C5_IsBusy ())
          {
            switch (I2C5_ErrorGet ())
              {
              case I2C_ERROR_NONE: i2cData.state = BUSI2C_STATE_COMPLETED;
                break;
              case I2C_ERROR_BUS_COLLISION: i2cData.state = BUSI2C_STATE_COLLISION;
                break;
              case I2C_ERROR_NACK: i2cData.state = BUSI2C_STATE_NAK;
                break;
              default: i2cData.state = BUSI2C_STATE_ERROR;
                break;
              }
          }
      }
      break;
    case BUSI2C_STATE_COLLISION: break;
    case BUSI2C_STATE_NAK: break;
    case BUSI2C_STATE_COMPLETED: break;
    case BUSI2C_STATE_ERROR: break;
    }
}

static int i2c_rw (uint8_t *txDat, uint32_t txLen, uint8_t *rxDat, uint32_t rxLen, uint8_t slaveAddr)
{
  if (i2cData.state != BUSI2C_STATE_IDLE) return 0; //Bus ocupado
  i2cData.tx = txDat;
  i2cData.txLength = txLen;
  i2cData.rx = rxDat;
  i2cData.rxLength = rxLen;
  i2cData.slaveAddressControlByte = slaveAddr;
  i2cData.state = BUSI2C_STATE_INIT; //Inicia la transaccion I2C
  return 1; //OK
}

//static int i2c_rw_init (void)
//{
//  if (i2cData.state != BUSI2C_STATE_IDLE) return 0; //Bus ocupado
//  i2cData.state = BUSI2C_STATE_PRE_INIT;
//  return 1;
//}

//static int i2c_rw_continue (uint8_t *txDat, uint32_t txLen, uint8_t *rxDat, uint32_t rxLen, uint8_t slaveAddr)
//{
//  if (i2cData.state != BUSI2C_STATE_PRE_INIT) return 0;
//  i2cData.tx = txDat;
//  i2cData.txLength = txLen;
//  i2cData.rx = rxDat;
//  i2cData.rxLength = rxLen;
//  i2cData.slaveAddressControlByte = slaveAddr;
//  i2cData.state = BUSI2C_STATE_INIT; //Inicia la transaccion I2C
//  return 1; //OK
//}

static t_BUSI2C_STATE i2c_get_state (void)
{
  return i2cData.state;
}
// </editor-fold>

#define SET_DRV_STATE(new_state)    drvData.sm_currentState=new_state

static uint8_t bcd2dec (t_BCD_BYTE bcd)
{
  return (uint8_t) (((bcd.b >> 4) * 10) + (bcd.b & 0x0F));
}

static t_BCD_BYTE dec2bcd (uint8_t dec)
{
  t_BCD_BYTE bcd;

  bcd.b = ((dec / 10) << 4) | (dec % 10);

  return bcd;
}

// <editor-fold defaultstate="collapsed" desc="SUB TASK">
/******************************************************************************/
#define ST_STATE_INIT__0__          0
#define SUB_TASK_STATE_INIT()       subtaskData.state=ST_STATE_INIT__0__
#define SUB_TASK_STATE_NEXT()       subtaskData.state++
#define SUB_TASK_STATE_PREVIOUS()   subtaskData.state--
#define SUB_TASK_STATE_GOTO(n)      subtaskData.state=n
#define ST_END_OK                   1
#define ST_END_ERROR                0

static void sub_task_init (void (*stf)(void*), void *subtaskContext, t_MCP7940N_STATES stateOK, t_MCP7940N_STATES stateError)
{
  SUB_TASK_STATE_INIT ();
  subtaskData.context = subtaskContext;
  subtaskData.subTask = stf;
  subtaskData.stateOnSuccessful = stateOK;
  subtaskData.stateOnError = stateError;
}

static void sub_task_end (uint8_t end_type)
{
  subtaskData.subTask = NULL;
  i2c_reset ();
  if (end_type)
    {
      SET_DRV_STATE (subtaskData.stateOnSuccessful);
    }
  else
    {
      _l ("(DRV_MCP7940x) Subtask end on error!\n");
      SET_DRV_STATE (subtaskData.stateOnError);
    }
}

/******************************************************************************/
static void sub_task_wait (void *ctx)
{
  static SYS_TIME_HANDLE hDelay;

  switch (subtaskData.state)
    {
    case ST_STATE_INIT__0__:
      {
        if (SYS_TIME_DelayMS (*((uint32_t*) ctx), &hDelay) != SYS_TIME_SUCCESS)
          {
            sub_task_end (ST_END_ERROR);
          }
        else
          {
            SUB_TASK_STATE_NEXT ();
          }
      }
      break;
    case 1:
      {
        if (SYS_TIME_DelayIsComplete (hDelay))
          {
            sub_task_end (ST_END_OK);
          }
      }
      break;
    }
}

static void sub_task_read_registers (void *ctx)
{
  static uint8_t sa;

  i2c_task ();
  switch (subtaskData.state)
    {
    case ST_STATE_INIT__0__:
      {
        sa = ((t_RW_PARAMS*) ctx)->startAddress;
        if (i2c_rw (&sa, 1, ((t_RW_PARAMS*) ctx)->data, ((t_RW_PARAMS*) ctx)->length, MCP7940N_SLAVE_ADDRESS_CONTROL_BYTE))
          {
            SUB_TASK_STATE_NEXT ();
          }
        else
          {
            sub_task_end (ST_END_ERROR);
          }
      }
      break;
    case 1:
      {
        switch (i2c_get_state ())
          {
          case BUSI2C_STATE_NAK: SUB_TASK_STATE_PREVIOUS ();
            break;
          case BUSI2C_STATE_COMPLETED: sub_task_end (ST_END_OK);
            break;
          case BUSI2C_STATE_COLLISION:
          case BUSI2C_STATE_ERROR: sub_task_end (ST_END_ERROR);
            break;
          default: return;
          }
      }
      break;
    }
}

static void sub_task_write_registers (void *ctx)
{
  static uint8_t sa[MCP7940N_MAX_BYTE_ADDRESS + 1];

  i2c_task ();
  switch (subtaskData.state)
    {
    case ST_STATE_INIT__0__:
      {
        sa[0] = ((t_RW_PARAMS*) ctx)->startAddress;
        memcpy (&sa[1], ((t_RW_PARAMS*) ctx)->data, ((t_RW_PARAMS*) ctx)->length);
        if (i2c_rw (sa, ((t_RW_PARAMS*) ctx)->length + 1, NULL, 0, MCP7940N_SLAVE_ADDRESS_CONTROL_BYTE))
          {
            SUB_TASK_STATE_NEXT ();
          }
        else
          {
            sub_task_end (ST_END_ERROR);
          }
      }
      break;
    case 1:
      {
        switch (i2c_get_state ())
          {
          case BUSI2C_STATE_NAK: SUB_TASK_STATE_PREVIOUS ();
            break;
          case BUSI2C_STATE_COMPLETED: sub_task_end (ST_END_OK);
            break;
          case BUSI2C_STATE_COLLISION:
          case BUSI2C_STATE_ERROR: sub_task_end (ST_END_ERROR);
            break;
          default: return;
          }
      }
      break;
    }
}
// </editor-fold>

//int32_t mcp7940n_refresh (void)
//{
//  if (drvData.sm_currentState != MCP7940N_STATE_IDLE) return -1;
//  drvData.rwParams.startAddress = REG_SECONDS;
//  drvData.rwParams.data = drvData.sys.s;
//  drvData.rwParams.length = 9;
//  SET_DRV_STATE (MCP7940N_STATE_REFRESH_DATA);
//
//  return 0;
//}

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */
int32_t drv_mcp7940n_initialize (void)
{
  memset (&drvData, 0, sizeof (t_MCP7940N_DRV_DATA));
  subtaskData.subTask = NULL;
  timeDelayms = 500;
  sub_task_init (sub_task_wait, &timeDelayms, MCP7940N_STATE_INIT, MCP7940N_STATE_ERROR);

  return 0;
}

void drv_mcp7940n_tasks (void)
{
  int yearAux;
  t_MCP7940N_CALL_BACK_CONTEXT ctx;

  if (subtaskData.subTask)
    {
      /* Sub-task pendiente */
      subtaskData.subTask (subtaskData.context);
      return;
    }
  /* Si no hay subtask pendiente revisa la state-machine principal */
  switch (drvData.sm_currentState)
    {
    case MCP7940N_STATE_INIT:
      {
        _l ("(DRV_MCP7940x) Init...\n");
        drvData.rwParams.startAddress = REG_SECONDS;
        drvData.rwParams.data = drvData.sys.s;
        drvData.rwParams.length = 9;
        drvData.cb = init_cb;
        SET_DRV_STATE (MCP7940N_STATE_REFRESH_DATA);
      }
      break;
      /* IDLE */
    case MCP7940N_STATE_IDLE: break;
      /* REFRESH DATA */
    case MCP7940N_STATE_REFRESH_DATA:
      {
        if (i2c_get_state () == BUSI2C_STATE_IDLE)
          {
            sub_task_init (sub_task_read_registers, &drvData.rwParams, MCP7940N_STATE_CHECK_ON, MCP7940N_STATE_ERROR);
          }
      }
      break;
    case MCP7940N_STATE_CHECK_ON:
      {
        if (drvData.sys.st && drvData.sys.vbaten)
          {
            drvData.ts_local.tm_sec = bcd2dec ((t_BCD_BYTE) (uint8_t) (drvData.sys.seconds_00h & 0x7F));
            drvData.ts_local.tm_min = bcd2dec ((t_BCD_BYTE) (uint8_t) (drvData.sys.minutes_01h & 0x7F));
            drvData.ts_local.tm_hour = bcd2dec ((t_BCD_BYTE) (uint8_t) (drvData.sys.hours_02h & 0x3F));
            drvData.ts_local.tm_wday = drvData.sys.day - 1;
            drvData.ts_local.tm_mday = bcd2dec ((t_BCD_BYTE) (uint8_t) (drvData.sys.date_04h & 0x3F));
            drvData.ts_local.tm_mon = bcd2dec ((t_BCD_BYTE) (uint8_t) (drvData.sys.month_05h & 0x3F));
            yearAux = bcd2dec ((t_BCD_BYTE) drvData.sys.year_06h);
            drvData.ts_local.tm_year = yearAux + 100; //2000 - 1900;            
            ctx.res = MCP7940N_RES_SUCCESS;
            ctx.ts = &drvData.ts_local;
            SET_DRV_STATE (MCP7940N_STATE_IDLE);
          }
        else
          {
            ctx.res = MCP7940N_RES_NOT_READY;
            ctx.ts = NULL;
            SET_DRV_STATE (MCP7940N_STATE_TURN_ON);
          }
        if (drvData.cb) drvData.cb (&ctx);
      }
      break;
    case MCP7940N_STATE_TURN_ON:
      {
        if (i2c_get_state () == BUSI2C_STATE_IDLE)
          {
            _l ("(DRV_MCP7940x) Turn ON\n");
            drvData.rwParams.startAddress = REG_SECONDS;
            drvData.sys.st = 1; //RTCC ON (00h)
            drvData.sys.vbaten = 1; //VBAT ENABLE (03h)
            drvData.sys.sqwe = 1; // MFP (square wave output) enable 
            drvData.sys.rs0 = 1; 
            drvData.sys.rs1 = 1;
            drvData.sys.rs2 = 0; //rs -> 011 = 32.768kHz
            drvData.rwParams.data = drvData.sys.s;
            drvData.rwParams.length = 8; //Solo actualizo los registros de 00h hasta el 07h                  
            sub_task_init (sub_task_write_registers, &drvData.rwParams, MCP7940N_STATE_IDLE, MCP7940N_STATE_ERROR);
          }
      }
      break;
      /* SET FULL DATE */
    case MCP7940N_STATE_REFRESH_FOR_UPDATE:
      {
        if (i2c_get_state () == BUSI2C_STATE_IDLE)
          {
            sub_task_init (sub_task_read_registers, &drvData.rwParams, MCP7940N_STATE_UPDATING, MCP7940N_STATE_ERROR);
          }
      }
      break;
    case MCP7940N_STATE_UPDATING:
      {
        if (i2c_get_state () == BUSI2C_STATE_IDLE)
          {
            _l ("(DRV_MCP7940x) Updating...\n");
            drvData.rwParams.startAddress = REG_SECONDS;
            drvData.sys.seconds_00h = (drvData.sys.seconds_00h & 0x80) | dec2bcd (drvData.ts_local.tm_sec).b;
            drvData.sys.minutes_01h = (drvData.sys.minutes_01h & 0x80) | dec2bcd (drvData.ts_local.tm_min).b;
            drvData.sys.hours_02h = (drvData.sys.hours_02h & 0xC0) | dec2bcd (drvData.ts_local.tm_hour).b;
            drvData.sys.day = drvData.ts_local.tm_wday + 1;
            drvData.sys.date_04h = (drvData.sys.date_04h & 0xC0) | dec2bcd (drvData.ts_local.tm_mday).b;
            drvData.sys.month_05h = (drvData.sys.month_05h & 0xC0) | dec2bcd (drvData.ts_local.tm_mon).b;
            yearAux = drvData.ts_local.tm_year % 100;
            drvData.sys.year_06h = dec2bcd (yearAux).b;
            drvData.rwParams.data = drvData.sys.s;
            drvData.rwParams.length = 7; //Actualizo los registros de 00h a 06h        
            sub_task_init (sub_task_write_registers, &drvData.rwParams, MCP7940N_STATE_IDLE, MCP7940N_STATE_ERROR);
          }
      }
      break;
      /* ERROR */
    case MCP7940N_STATE_ERROR:
      {
        _l ("(DRV_MCP7940x) ERROR! init task in 5 sec...\n");
        timeDelayms = 5000;
        sub_task_init (sub_task_wait, &timeDelayms, MCP7940N_STATE_INIT, MCP7940N_STATE_ERROR);
      }
      break;
    default:
      {
        _l ("(DRV_MCP7940x) task state unknown!, ERROR!\n");
        SET_DRV_STATE (MCP7940N_STATE_ERROR);
      }
      break;
    }
}

t_MCP7940N_STATES mcp7940n_get_state (void)
{
  return drvData.sm_currentState;
}

int32_t mcp7940n_get_ts (DRV_MCP7940N_CALL_BACK cb)
{
  if (drvData.sm_currentState != MCP7940N_STATE_IDLE) return -1;
  drvData.rwParams.startAddress = REG_SECONDS;
  drvData.rwParams.data = drvData.sys.s;
  drvData.rwParams.length = 9;
  drvData.cb = cb;
  SET_DRV_STATE (MCP7940N_STATE_REFRESH_DATA);
  return 0;
}

int32_t mcp7940n_set_ts (struct tm *ts)
{
  if (drvData.sm_currentState != MCP7940N_STATE_IDLE) return -1;
  memcpy (&drvData.ts_local, ts, sizeof (struct tm));
  drvData.rwParams.startAddress = REG_SECONDS;
  drvData.rwParams.data = drvData.sys.s;
  drvData.rwParams.length = 7;
  SET_DRV_STATE (MCP7940N_STATE_REFRESH_FOR_UPDATE);

  return 0;
}

/* ************************************************************** End of File */
