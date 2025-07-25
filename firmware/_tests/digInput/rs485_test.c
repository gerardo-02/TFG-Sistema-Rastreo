/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "peripheral/gpio/plib_gpio.h"

#include "log.h"
#include "rs485_test.h"
#include "peripheral/uart/plib_uart2.h"
#include "peripheral/uart/plib_uart5.h"


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Data types & Definitions                                          */
/* ************************************************************************** */
/* ************************************************************************** */


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Global data & variables                                           */
/* ************************************************************************** */
/* ************************************************************************** */
static uint8_t buff[32];
static char reply[sizeof (buff) + 16] = "HELLO FROM PATIS MZ...";
static int state;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Callback Functions                                                */
/* ************************************************************************** */

/* ************************************************************************** */
static void rs485_mode_rx (void)
{
  DE_RS485_DIR_Clear (); //DataEnable low, put in Z D pin.
  RE_RS485_DIR_Clear (); //RecibeEnable low, active R output for receive data.
}

static void rs485_mode_tx (void)
{
  RE_RS485_DIR_Set (); //RecibeEnable high, put in Z R pin.
  DE_RS485_DIR_Set (); //DataEnable high, active D input for transmit.
}

static void read_cb (UART_EVENT event, uintptr_t context)
{
  UART2_ReadCallbackRegister (NULL, 0);
  rs485_mode_tx ();
  
  size_t c = UART2_ReadCountGet ();
  _l ("(RS485) Read call-back!, event:%d\n", (int) event);
  if (c)
    {
      if (c >= sizeof (buff))
        {
          _l ("(RS485) Buffer big!\n");
          c = sizeof (buff) - 1;
        }
      UART2_Read (buff, c);
      _dump (buff, c, DUMP_MODE_MIX);
      buff[c] = 0; // null-string
      snprintf (reply, sizeof (reply), "E(%d): %s", (int) c, (char*) buff);
      state = 0;
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
void rs485_test_init (void)
{
#if !defined(EXT_IN_01_U2RX_R_RS485_Set) && !defined(EXT_IN_02_U2TX_D_RS485_Set)
  state = 0;
  _l ("(RS485) Test initialized. Echo function.\n");
#else
  _l ("(RS485) Not well configured pins for RS485. NOT init!!!!\n");
#endif
}

void rs485_test_task (void)
{
  switch (state)
    {
    case 0:
      _l ("(RS485) msg(%d): %s. Sending...\n", strlen (reply), reply);
      UART2_Write ((uint8_t*) reply, strlen (reply));
      state++;
      break;
    case 1:
      if (UART2_TransmitComplete ())
        {
          _l ("(RS485) Send message end\n");
          state++;
        }
      break;
    case 2:
      _l ("(RS485) Ready for receiving....\n");
      UART2_ReadNotificationEnable (true, true);
      UART2_ReadThresholdSet (5);
      UART2_ReadCallbackRegister (read_cb, 0);
      state++;
      break;
    case 3:
      rs485_mode_rx ();
      state++;
      break;
    case 4:
      //running
      break;
    }
}

/* ************************************************************** End of File */
