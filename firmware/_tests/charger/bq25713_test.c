/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <string.h>

#include "peripheral/i2c/master/plib_i2c2_master.h"

#include "log.h"
#include "bq25713_test.h"
#include "peripheral/coretimer/plib_coretimer.h"
#include "peripheral/gpio/plib_gpio.h"


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */
#define BQ25713_I2C_ADDR    0x6B
#define BQ25713B_I2C_ADDR   0x6A

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Data types & Definitions                                          */
/* ************************************************************************** */

/* ************************************************************************** */
typedef union
{

  struct
  {
    unsigned reserved0 : 6; //Not used. Value Ignored.
    unsigned add64mA : 1;
    unsigned add128mA : 1;
    unsigned add256mA : 1;
    unsigned add512mA : 1;
    unsigned add1024mA : 1;
    unsigned add2048mA : 1;
    unsigned add4096mA : 1;
    unsigned reserved1 : 3; //Not used. 1 = invalid write.
  };

  struct
  {
    uint8_t h02Reg;
    uint8_t h03Reg;
  };
  uint16_t w;
} t_CHARGE_CURRENT_02h_03h;
#define REG_CHARGE_CURRENT_ADDR     0x02

typedef union
{

  struct
  {
    unsigned faultOtgUvp : 1;
    unsigned faultOtgOvp : 1;
    unsigned faultLatchoff : 1;
    unsigned faultSysShort : 1;
    unsigned sysOvpStat : 1;
    unsigned faultACOC : 1;
    unsigned faultBATOC : 1;
    unsigned faultACOV : 1;
    unsigned inOtg : 1;
    unsigned inPchrg : 1;
    unsigned inFchrg : 1;
    unsigned inIindpm : 1;
    unsigned inVindpm : 1;
    unsigned inVap : 1;
    unsigned icoDone : 1;
    unsigned acStat : 1;
  };

  struct
  {
    uint8_t h20Reg;
    uint8_t h21Reg;
  };

  uint16_t w;
} t_CHARGER_STATUS_20h_21h;
#define REG_CHARGER_STATUS_ADDR     0x20

typedef union
{

  struct
  {
    uint8_t psys; //PSYS: Full range: 3.06 V, LSB: 12 mV
    uint8_t vbus; //VBUS: Full range: 3200 mV to 19520 mV, LSB: 64 mV
  };

  uint16_t w;
} t_ADC_PSYS_VBUS_26h_27h;
#define REG_ADC_PSYS_VBUS           0x26

typedef union
{

  struct
  {
    unsigned batDisCurrent : 7; //IDCHG: Full range: 32.512 A, LSB: 256 mA
    unsigned reserved0 : 1;
    unsigned batChgCurrent : 7; //ICHG: Full range: 8.128 A, LSB: 64 mA
    unsigned reserved1 : 1;
  };

  uint16_t w;
} t_ADC_IBAT_28h_29h;
#define REG_ADCIBAT                 0x28

typedef union
{

  struct
  {
    uint8_t voltage; //CMPIN: Full range: 3.06 V, LSB: 12 mV
    uint8_t current; //IIN: Full range: 12.75 A, LSB: 50 mA. For 10m? sense resistor, IIN full range = 6.4A
  };

  uint16_t w;
} t_ADC_IIN_CMPIN_2Ah_2Bh;
#define REG_ADCIINCMPIN             0x2A

typedef union
{

  struct
  {
    uint8_t batVoltage; //VBAT: Full range: 2.88 V to 19.2 V, LSB: 64 mV
    uint8_t sysVoltage; //VSYS: Full range: 2.88 V to 19.2 V, LSB: 64 mV
  };

  uint16_t w;
} t_ADC_VSYS_VBAT_2Ch_2Dh;
#define REG_ADC_VSYS_VBAT             0x2C

typedef union
{

  struct
  {
    unsigned en_adc_vbat : 1;
    unsigned en_adc_vsys : 1;
    unsigned en_adc_ichg : 1;
    unsigned en_adc_idchg : 1;
    unsigned en_adc_iin : 1;
    unsigned en_adc_psys : 1;
    unsigned en_adc_vbus : 1;
    unsigned en_adc_cmpin : 1;
    unsigned reserved : 5;
    unsigned adc_fullScale : 1;
    unsigned adc_start : 1;
    unsigned adc_conv : 1;
  };

  struct
  {
    uint8_t h3aReg;
    uint8_t h3bReg;
  };

  uint16_t w;
} t_ADC_OPTION_3Ah_3Bh;
#define REG_ADC_OPTION              0x3A

#define REG_MANUFACTURER_DEVICE_ID_ADDR    0x2E
#define REG_MANUFACTURER_DEVICE_ID_SIZE    2

typedef union
{

  struct
  {
    uint8_t manufacturer;
    uint8_t device;
  };

  uint16_t w;
} t_ID_2Eh_2Fh;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Global data & variables                                           */
/* ************************************************************************** */
/* ************************************************************************** */
static int state;
static uint8_t addrReg;
static t_ID_2Eh_2Fh idReg;
static t_CHARGER_STATUS_20h_21h statReg, aux;
static t_ADC_OPTION_3Ah_3Bh adcOptReg;
static t_ADC_PSYS_VBUS_26h_27h pSysVbusReg;
static t_CHARGE_CURRENT_02h_03h chargeCurrentReg;
static t_ADC_VSYS_VBAT_2Ch_2Dh vSysVbatReg;
static uint8_t www[3];
static uint32_t time;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Callback Functions                                                */
/* ************************************************************************** */

/* ************************************************************************** */
static void cb (uintptr_t context)
{
  switch (I2C2_ErrorGet ())
    {
    case I2C_ERROR_NONE:
      state++;
      break;
    case I2C_ERROR_NACK:
      _l ("(BQ25713) Error NACK\n");
      state = 100;
      break;
    case I2C_ERROR_BUS_COLLISION:
      _l ("(BQ25713) Error Bus collision\n");
      state = 100;
      break;
    }
}

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
void charger_test_init (void)
{
  state = 0;
}

void charger_test_task (void)
{
  switch (state)
    {
    case 0:
      aux.w = 0;
      _l ("(BQ25713) Init test...\n");
      time = CORETIMER_CounterGet ();
      state++;
      break;
    case 1:
      if (!I2C2_IsBusy ())
        {
          I2C2_CallbackRegister (cb, (uintptr_t) NULL);
          state++;
        }
      break;
      //Request manufacturer and device id
    case 2:
      addrReg = REG_MANUFACTURER_DEVICE_ID_ADDR;
      idReg.w = 0;
      if (I2C2_WriteRead (BQ25713_I2C_ADDR, &addrReg, 1, (uint8_t*) & idReg, REG_MANUFACTURER_DEVICE_ID_SIZE))
        {
          state++;
          _l ("(BQ25713) Requesting manufacturer & device id...\n");
        }
      break;
    case 3:
      //WAIT manufacturer ID
      break;
    case 4:
      state++;
      _l ("(BQ25713) manufacturer id: %X, device id: %X\n", idReg.manufacturer, idReg.device);
      break;
      //Enable ADCs
    case 5:
      www[0] = REG_ADC_OPTION;
      adcOptReg.w = 0;
      adcOptReg.adc_conv = 1;
      adcOptReg.adc_fullScale = 1;
      adcOptReg.h3aReg = 0xFF; //all ADCs enabled
      www[1] = adcOptReg.h3aReg;
      www[2] = adcOptReg.h3bReg;
      if (I2C2_Write (BQ25713_I2C_ADDR, www, 3))
        {
          state++;
          _l ("(BQ25713) Enabling ADCs conversions...\n");
        }
      break;
    case 6:
      //WAIT Enable Charge current
      break;
    case 7:
      www[0] = REG_CHARGE_CURRENT_ADDR;
      chargeCurrentReg.w = 0;
      chargeCurrentReg.add512mA = 1;
      www[1] = chargeCurrentReg.h02Reg;
      www[2] = chargeCurrentReg.h03Reg;
      if (I2C2_Write (BQ25713_I2C_ADDR, www, 3))
        {
          state++;
          _l ("(BQ25713) Enabling charge current...\n");
        }
      break;
    case 8:
      //WAIT enable charge current
      break;

      //////////////////////////////////////////////////////////////////////////
    case 9:
      //Go to gets status registers
      state = 100;
      break;

      // Request charger status ////////////////////////////////////////////////
    case 100:
      time = CORETIMER_CounterGet ();
      addrReg = REG_CHARGER_STATUS_ADDR;
      statReg.w = 0;
      if (I2C2_WriteRead (BQ25713_I2C_ADDR, &addrReg, 1, (uint8_t*) & statReg, sizeof (t_CHARGER_STATUS_20h_21h)))
        {
          state++;
          //_l ("(BQ25713) Requesting charger state...\n");
        }
      break;
    case 101:
      //Wait Charger status
      break;
    case 102:
      addrReg = REG_ADC_PSYS_VBUS;
      pSysVbusReg.w = 0;
      if (I2C2_WriteRead (BQ25713_I2C_ADDR, &addrReg, 1, (uint8_t*) & pSysVbusReg, sizeof (t_ADC_PSYS_VBUS_26h_27h)))
        {
          state++;
        }
      break;
    case 103:
      //Wait psys & vbus adc register
      break;
    case 104:
      addrReg = REG_ADC_VSYS_VBAT;
      vSysVbatReg.w = 0;
      if (I2C2_WriteRead (BQ25713_I2C_ADDR, &addrReg, 1, (uint8_t*) & vSysVbatReg, sizeof (t_ADC_VSYS_VBAT_2Ch_2Dh)))
        {
          state++;
        }
      break;
    case 105:
      //WAIT vsys & vbat register
      break;
    case 106:
      if (aux.w != statReg.w)
        {
          aux = statReg;
          _l ("(BQ25713) Charger state: %X\n   fault OTG UVP: %d\n   fault OTG OVP: %d\n   fault latch OFF: %d\n   fault SYS short: %d\n   fault SYS OVP stat: %d\n   fault ACOC: %d\n   fault BATOC: %d\n   fault ACOV: %d\n"
              "   inOTG: %d\n   inPchrg: %d\n   infChrg: %d\n   inIindpm: %d\n   inVindpm: %d\n   inVAP: %d\n   icoDone: %d\n   acStat: %d\n"
              , (int) statReg.w
              , (int) statReg.faultOtgUvp
              , (int) statReg.faultOtgOvp
              , (int) statReg.faultLatchoff
              , (int) statReg.faultSysShort
              , (int) statReg.sysOvpStat
              , (int) statReg.faultACOC
              , (int) statReg.faultBATOC
              , (int) statReg.faultACOV
              , (int) statReg.inOtg
              , (int) statReg.inPchrg
              , (int) statReg.inFchrg
              , (int) statReg.inIindpm
              , (int) statReg.inVindpm
              , (int) statReg.inVap
              , (int) statReg.icoDone
              , (int) statReg.acStat
              );
        }
      _l ("(BQ25713) PSYS=%fV, VBUS=%fV\n", (float) pSysVbusReg.psys * 0.012, (float) pSysVbusReg.vbus * 0.064);
      _l ("(BQ25713) VSYS=%fV, VBAT=%fV\n", (float) vSysVbatReg.sysVoltage * 0.064, (float) vSysVbatReg.batVoltage * 0.064);
      state = 200;
      break;

      //Test ok ////////////////////////////////////////////////////////////////
    case 200:
      if (((CORETIMER_CounterGet () - time) / CORETIMER_FrequencyGet ()) >= 5)
        {
          state = 100;
        }
      break;
      //
    default:
      if (((CORETIMER_CounterGet () - time) / CORETIMER_FrequencyGet ()) >= 60)
        {
          _l ("(BQ25713) retry test...\n");
          state = 0;
        }
      break;
    }
}

/* ************************************************************** End of File */