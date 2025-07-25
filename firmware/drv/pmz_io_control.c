#include <string.h>

#include "system/time/sys_time.h"
#include "peripheral/gpio/plib_gpio.h"

#include "log.h"

#include "pmz_io_control.h"

#if !defined(EXT_IN_01_U2RX_R_RS485_PIN)
#error "EXT_IN_01_U2RX_R_RS485_PIN is neccesary to be defined!"
#endif
#if !defined(EXT_IN_02_U2TX_D_RS485_PIN)
#error "EXT_IN_02_U2TX_D_RS485_PIN is neccesary to be defined!"
#endif
#if !defined(EXT_IN_03_U4RX_RS232_PIN)
#error "EXT_IN_03_U4RX_RS232_PIN is neccesary to be defined!"
#endif
#if !defined(EXT_IN_04_U4TX_RS232_PIN)
#error "EXT_IN_04_U4TX_RS232_PIN is neccesary to be defined!"
#endif

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */
#define TIME_RESOLUTION_MS          100
#define CTE_TIME_MS                 100

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Data types & Definitions                                          */
/* ************************************************************************** */

/* ************************************************************************** */
typedef struct
{
  t_IO_INPUT_STATES state;
  uint32_t t;
  IO_INPUT_CALL_BACK risingEdgeCb;
  void *risingEdgeCtx;
  IO_INPUT_CALL_BACK fallingEdgeCb;
  void *fallingEdgeCtx;
} t_INPUT;

typedef enum
{
  IO_OUTPUT_MODE_SET = 0,
  IO_OUTPUT_MODE_PULSE,
  IO_OUTPUT_MODE_BLINK,
} t_IO_OUTPUT_MODE;

typedef struct
{
  t_IO_OUTPUT_STATES state;
  t_IO_OUTPUT_MODE mode;
  uint32_t tPulse;
  uint32_t tOper;
  uint16_t ms_pulse_high;
  uint16_t ms_pulse_low;
} t_OUTPUT;

typedef enum
{
  OUTPUT_ACTION_LOW = 0,
  OUTPUT_ACTION_HIGH,
  OUTPUT_ACTION_TOGGLE,
} t_OUTPUT_ACTION;

typedef enum
{
  IO_STATE_INIT = 0,
  IO_STATE_RUNNING,
  IO_STATE_ERROR,
} t_IO_STATES;

typedef struct
{
  t_IO_STATES state;
  t_INPUT input[IO_INPUT_MAX];
  t_OUTPUT output[IO_OUTPUT_MAX];
} t_IO_DATA;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Global data & variables                                           */
/* ************************************************************************** */
/* ************************************************************************** */
static SYS_TIME_HANDLE sth;
static t_IO_DATA io;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Callback Functions                                                */
/* ************************************************************************** */
/* ************************************************************************** */
#define timer_ms(timer,time_ms)     (timer=((time_ms/TIME_RESOLUTION_MS)+1))
#define timer_expired(timer)        (timer==1)
#define timer_off(timer)            (timer=0) 
#define timer_dec(timer)            if(timer>1)timer--

static void io_tasks (void);

static void local_timers (uintptr_t ctx)
{
  uint32_t i;

  for (i = 0; i < IO_INPUT_MAX; i++)
    {
      timer_dec (io.input[i].t);
    }
  for (i = 0; i < IO_OUTPUT_MAX; i++)
    {
      timer_dec (io.output[i].tPulse);
      timer_dec (io.output[i].tOper);
    }
  io_tasks ();
}

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Local Functions                                                   */
/* ************************************************************************** */

/* ************************************************************************** */
static void set_output (t_IO_OUTPUT_ID id, t_OUTPUT_ACTION action)
{
  switch (id)
    {
    case IO_OUTPUT_GREEN_LED:
      switch (action)
        {
        case OUTPUT_ACTION_LOW:
          LED_VERDE_Clear ();
          break;
        case OUTPUT_ACTION_HIGH:
          LED_VERDE_Set ();
          break;
        case OUTPUT_ACTION_TOGGLE:
          LED_VERDE_Toggle ();
          break;
        }
      break;
    case IO_OUTPUT_AMBER_LED:
      switch (action)
        {
        case OUTPUT_ACTION_LOW:
          LED_AMBAR_Clear ();
          break;
        case OUTPUT_ACTION_HIGH:
          LED_AMBAR_Set ();
          break;
        case OUTPUT_ACTION_TOGGLE:
          LED_AMBAR_Toggle ();
          break;
        }
      break;
    case IO_OUTPUT_RELE_ID_1:
      switch (action)
        {
        case OUTPUT_ACTION_LOW:
          O_RELAY_1_Clear ();
          break;
        case OUTPUT_ACTION_HIGH:
          O_RELAY_1_Set ();
          break;
        case OUTPUT_ACTION_TOGGLE:
          O_RELAY_1_Toggle ();
          break;
        }
      break;
    case IO_OUTPUT_RELE_ID_2:
      switch (action)
        {
        case OUTPUT_ACTION_LOW:
          O_RELAY_2_Clear ();
          break;
        case OUTPUT_ACTION_HIGH:
          O_RELAY_2_Set ();
          break;
        case OUTPUT_ACTION_TOGGLE:
          O_RELAY_2_Toggle ();
          break;
        }
      break;
    case IO_OUTPUT_RELE_ID_3:
      switch (action)
        {
        case OUTPUT_ACTION_LOW:
          O_RELAY_3_Clear ();
          break;
        case OUTPUT_ACTION_HIGH:
          O_RELAY_3_Set ();
          break;
        case OUTPUT_ACTION_TOGGLE:
          O_RELAY_3_Toggle ();
          break;
        }
      break;
    case IO_OUTPUT_RELE_ID_4:
      switch (action)
        {
        case OUTPUT_ACTION_LOW:
          O_RELAY_4_Clear ();
          break;
        case OUTPUT_ACTION_HIGH:
          O_RELAY_4_Set ();
          break;
        case OUTPUT_ACTION_TOGGLE:
          O_RELAY_4_Toggle ();
          break;
        }
      break;
    case IO_OUTPUT_RELE_ID_5:
      switch (action)
        {
        case OUTPUT_ACTION_LOW:
          O_RELAY_5_Clear ();
          break;
        case OUTPUT_ACTION_HIGH:
          O_RELAY_5_Set ();
          break;
        case OUTPUT_ACTION_TOGGLE:
          O_RELAY_5_Toggle ();
          break;
        }
      break;
    case IO_OUTPUT_RELE_ID_6:
      switch (action)
        {
        case OUTPUT_ACTION_LOW:
          O_RELAY_6_Clear ();
          break;
        case OUTPUT_ACTION_HIGH:
          O_RELAY_6_Set ();
          break;
        case OUTPUT_ACTION_TOGGLE:
          O_RELAY_6_Toggle ();
          break;
        }
      break;
    case IO_OUTPUT_RELE_ID_7:
      switch (action)
        {
        case OUTPUT_ACTION_LOW:
          O_RELAY_7_Clear ();
          break;
        case OUTPUT_ACTION_HIGH:
          O_RELAY_7_Set ();
          break;
        case OUTPUT_ACTION_TOGGLE:
          O_RELAY_7_Toggle ();
          break;
        }
      break;
    case IO_OUTPUT_RELE_ID_8:
      switch (action)
        {
        case OUTPUT_ACTION_LOW:
          O_RELAY_8_Clear ();
          break;
        case OUTPUT_ACTION_HIGH:
          O_RELAY_8_Set ();
          break;
        case OUTPUT_ACTION_TOGGLE:
          O_RELAY_8_Toggle ();
          break;
        }
      break;
    case IO_OUTPUT_SIMCOM_POWER:
      switch (action)
        {
        case OUTPUT_ACTION_LOW:
          SIMCOM_POWER_Clear ();
          break;
        case OUTPUT_ACTION_HIGH:
          SIMCOM_POWER_Set ();
          break;
        case OUTPUT_ACTION_TOGGLE:
          SIMCOM_POWER_Toggle ();
          break;
        }
      break;
    default:
      break;
    }
}

static void outputs_task (void)
{
  t_IO_OUTPUT_ID i;

  for (i = 0; i < IO_OUTPUT_MAX; i++)
    {
      if (timer_expired (io.output[i].tPulse))
        {
          switch (io.output[i].mode)
            {
            case IO_OUTPUT_MODE_SET:
            case IO_OUTPUT_MODE_PULSE:
              break;
            case IO_OUTPUT_MODE_BLINK:
              if (io_output_get_state (i) == IO_OUTPUT_STATE_HIGH)
                {
                  timer_ms (io.output[i].tPulse, io.output[i].ms_pulse_low);
                }
              else
                {
                  timer_ms (io.output[i].tPulse, io.output[i].ms_pulse_high);
                }
              set_output (i, OUTPUT_ACTION_TOGGLE);
            }
        }
      if (timer_expired (io.output[i].tOper))
        {
          set_output (i, OUTPUT_ACTION_TOGGLE);
          timer_off (io.output[i].tPulse);
          timer_off (io.output[i].tOper);
          io.output[i].mode = IO_OUTPUT_MODE_SET;
        }
    }
}

static void inputs_task (t_IO_INPUT_ID id, int pin_state)
{
  t_INPUT *i = &io.input[id];

  switch (i->state)
    {
    case IO_INPUT_STATE_DISABLE:
      break;
    case IO_INPUT_STATE_UNKNOWN:
      timer_off (i->t);
      i->state = pin_state ? IO_INPUT_STATE_HIGH : IO_INPUT_STATE_LOW;
      break;
    case IO_INPUT_STATE_LOW:
      if (pin_state)
        {
          timer_ms (i->t, CTE_TIME_MS);
          i->state = IO_INPUT_STATE_LOW_TO_HIGH;
        }
      break;
    case IO_INPUT_STATE_LOW_TO_HIGH:
      if (!pin_state)
        {
          timer_ms (i->t, CTE_TIME_MS);
          i->state = IO_INPUT_STATE_HIGH_TO_LOW;
        }
      else
        {
          if (timer_expired (i->t))
            {
              timer_off (i->t);
              if (i->risingEdgeCb)
                i->risingEdgeCb (id, i->state, i->risingEdgeCtx); //Low -> High
              i->state = IO_INPUT_STATE_HIGH;
            }
        }
      break;
    case IO_INPUT_STATE_HIGH:
      if (!pin_state)
        {
          timer_ms (i->t, CTE_TIME_MS);
          i->state = IO_INPUT_STATE_HIGH_TO_LOW;
        }
      break;
    case IO_INPUT_STATE_HIGH_TO_LOW:
      if (pin_state)
        {
          timer_ms (i->t, CTE_TIME_MS);
          i->state = IO_INPUT_STATE_LOW_TO_HIGH;
        }
      else
        {
          if (timer_expired (i->t))
            {
              timer_off (i->t);
              if (i->fallingEdgeCb)
                i->fallingEdgeCb (id, i->state, i->fallingEdgeCtx); //High -> Low
              i->state = IO_INPUT_STATE_LOW;
            }
        }
      break;
    }
}

static void io_tasks (void)
{
  uint32_t i;

  switch (io.state)
    {
    case IO_STATE_INIT:
      //Inputs 01 & 02 or RS485 Config initialization
#if defined(EXT_IN_01_U2RX_R_RS485_Set) && defined(EXT_IN_02_U2TX_D_RS485_Set)
      RE_RS485_DIR_Set ();
      io.input[IO_INPUT_ID_01].state = IO_INPUT_STATE_UNKNOWN;
      io.input[IO_INPUT_ID_02].state = IO_INPUT_STATE_UNKNOWN;
#else
      io.input[IO_INPUT_ID_01].state = IO_INPUT_STATE_DISABLE;
      io.input[IO_INPUT_ID_02].state = IO_INPUT_STATE_DISABLE;
#endif 
      //Inputs 03 & 04 or RS232 Config initialization
#if defined(EXT_IN_03_U4RX_RS232_Set) && defined(EXT_IN_04_U4TX_RS232_Set)
      io.input[IO_INPUT_ID_03].state = IO_INPUT_STATE_UNKNOWN;
      io.input[IO_INPUT_ID_04].state = IO_INPUT_STATE_UNKNOWN;
#else
      io.input[IO_INPUT_ID_03].state = IO_INPUT_STATE_DISABLE;
      io.input[IO_INPUT_ID_04].state = IO_INPUT_STATE_DISABLE;
#endif 
      //Rest of inputs
      for (i = IO_INPUT_ID_05; i < IO_INPUT_MAX; i++)
        {
          io.input[i].state = IO_INPUT_STATE_UNKNOWN;
        }
      _l ("(IO) Initialized. Running...\n");
      io.state = IO_STATE_RUNNING;
      break;
    case IO_STATE_RUNNING:
      outputs_task ();
#if defined(EXT_IN_01_U2RX_R_RS485_Set) && defined(EXT_IN_02_U2TX_D_RS485_Set)
      inputs_task (IO_INPUT_ID_01, EXT_IN_01_U2RX_R_RS485_Get ());
      inputs_task (IO_INPUT_ID_02, EXT_IN_02_U2TX_D_RS485_Get ());
#else
      //Inputs 1 & 2 are for RS485 module
#endif
#if defined(EXT_IN_03_U4RX_RS232_Set) && defined(EXT_IN_04_U4TX_RS232_Set)
      inputs_task (IO_INPUT_ID_03, EXT_IN_03_U4RX_RS232_Get ());
      inputs_task (IO_INPUT_ID_04, EXT_IN_04_U4TX_RS232_Get ());
#else
      //Inputs 3 & 4 are for RS232 Module
#endif
      inputs_task (IO_INPUT_ID_05, EXT_IN_05_Get ());
      inputs_task (IO_INPUT_ID_06, EXT_IN_06_Get ());
      inputs_task (IO_INPUT_ID_07, EXT_IN_07_Get ());
      inputs_task (IO_INPUT_ID_08, EXT_IN_08_Get ());
      inputs_task (IO_INPUT_ID_09, EXT_IN_09_Get ());
      inputs_task (IO_INPUT_ID_10, EXT_IN_10_Get ());
      inputs_task (IO_INPUT_ID_11, EXT_IN_11_Get ());
      inputs_task (IO_INPUT_ID_12, EXT_IN_12_Get ());
      break;
    case IO_STATE_ERROR:
      break;
    }
}

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */
void io_initialize (void)
{
  int i;

  memset (&io, 0, sizeof (t_IO_DATA));
  for (i = 0; i < IO_OUTPUT_MAX; i++)
    set_output (i, OUTPUT_ACTION_LOW);
  for (i = 0; i < IO_INPUT_MAX; i++)
    {
      timer_off (io.input[i].t);
      io.input[i].fallingEdgeCb = NULL;
      io.input[i].fallingEdgeCtx = NULL;
      io.input[i].risingEdgeCb = NULL;
      io.input[i].risingEdgeCtx = NULL;
      io.input[i].state = IO_INPUT_STATE_DISABLE;
    }
  sth = SYS_TIME_CallbackRegisterMS (local_timers, 0, TIME_RESOLUTION_MS, SYS_TIME_PERIODIC);
  if (sth == SYS_TIME_HANDLE_INVALID)
    {
      io.state = IO_STATE_ERROR;
    }
  else
    {
      _l ("(IO) Initializing...\n");
      io.state = IO_STATE_INIT;
    }
}

t_IO_INPUT_STATES io_input_get_state (t_IO_INPUT_ID id)
{
  return io.input[id].state;
}

void io_register_input_cb (t_IO_INPUT_ID id, t_IO_EDGE edge, IO_INPUT_CALL_BACK cb, void *localCtx)
{
  switch (edge)
    {
    case IO_EDGE_FALLING:
      io.input[id].fallingEdgeCb = cb;
      io.input[id].fallingEdgeCtx = localCtx;
      break;
    case IO_EDGE_RISING:
      io.input[id].risingEdgeCb = cb;
      io.input[id].risingEdgeCtx = localCtx;
      break;
    case IO_EDGE_BOTH:
      io.input[id].fallingEdgeCb = cb;
      io.input[id].risingEdgeCb = cb;
      break;
    default:
      break;
    }
}

t_IO_OUTPUT_STATES io_output_get_state (t_IO_OUTPUT_ID id)
{
  switch (id)
    {
    case IO_OUTPUT_GREEN_LED:
      return !(LED_VERDE_Get ());
      break;
    case IO_OUTPUT_AMBER_LED:
      return !(LED_AMBAR_Get ());
      break;
    case IO_OUTPUT_RELE_ID_1:
      return O_RELAY_1_Get ();
      break;
    case IO_OUTPUT_RELE_ID_2:
      return O_RELAY_2_Get ();
      break;
    case IO_OUTPUT_RELE_ID_3:
      return O_RELAY_3_Get ();
      break;
    case IO_OUTPUT_RELE_ID_4:
      return O_RELAY_4_Get ();
      break;
    case IO_OUTPUT_RELE_ID_5:
      return O_RELAY_5_Get ();
      break;
    case IO_OUTPUT_RELE_ID_6:
      return O_RELAY_6_Get ();
      break;
    case IO_OUTPUT_RELE_ID_7:
      return O_RELAY_7_Get ();
      break;
    case IO_OUTPUT_RELE_ID_8:
      return O_RELAY_8_Get ();
      break;
    case IO_OUTPUT_SIMCOM_POWER:
      return SIMCOM_POWER_Get();
      break;
    default:
      return IO_OUTPUT_STATE_LOW;
      break;
    }
}

int io_output_set_state (t_IO_OUTPUT_ID id, t_IO_OUTPUT_STATES state)
{
  timer_off (io.output[id].tPulse);
  timer_off (io.output[id].tOper);
  io.output[id].mode = IO_OUTPUT_MODE_SET;
  switch (state)
    {
    case IO_OUTPUT_STATE_LOW:
      set_output (id, OUTPUT_ACTION_LOW);
      _d ("(IO) Output=%d, set to low\n", id);
      break;
    case IO_OUTPUT_STATE_HIGH:
      set_output (id, OUTPUT_ACTION_HIGH);
      _d ("(IO) Output=%d, set to high\n", id);
      break;
    }

  return 0;
}

int io_output_toggle (t_IO_OUTPUT_ID id)
{
  timer_off (io.output[id].tOper);
  timer_off (io.output[id].tPulse);
  io.output[id].mode = IO_OUTPUT_MODE_SET;
  set_output (id, OUTPUT_ACTION_TOGGLE);
  _d ("(IO) Output=%d, toggling\n", id);

  return 0;
}

int io_output_pulse (t_IO_OUTPUT_ID id, t_IO_OUTPUT_STATES state, uint32_t ms_operation_time)
{
  _d ("(IO) Output=%d, operation time is %u\n", id, ms_operation_time);
  timer_ms (io.output[id].tOper, ms_operation_time);
  timer_off (io.output[id].tPulse);
  io.output[id].mode = IO_OUTPUT_MODE_PULSE;
  switch (state)
    {
    case IO_OUTPUT_STATE_LOW:
      set_output (id, OUTPUT_ACTION_LOW);
      _d ("(IO) Output=%d, set to low\n", id);
      break;
    case IO_OUTPUT_STATE_HIGH:
      set_output (id, OUTPUT_ACTION_HIGH);
      _d ("(IO) Output=%d, set to high\n", id);
      break;
    }

  return 0;
}

int io_output_blink (t_IO_OUTPUT_ID id, uint16_t ms_pulse_high, uint16_t ms_pulse_low, uint32_t ms_operation_time)
{
  timer_ms (io.output[id].tPulse, ms_pulse_high);
  if (ms_operation_time > 0)
    {
      _d ("(IO) Output=%d, operation time is %u\n", id, ms_operation_time);
      timer_ms (io.output[id].tOper, ms_operation_time);
    }
  else
    {
      _d ("(IO) Output=%d, operation time is undefined\n", id);
      timer_off (io.output[id].tOper); //If operation time is zero then undefined time. Until change...
    }
  io.output[id].mode = IO_OUTPUT_MODE_BLINK;
  set_output (id, OUTPUT_ACTION_HIGH);
  io.output[id].ms_pulse_high = ms_pulse_high;
  io.output[id].ms_pulse_low = ms_pulse_low;

  return 0;
}

/* ************************************************************** End of File */
