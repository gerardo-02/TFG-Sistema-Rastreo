/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "peripheral/gpio/plib_gpio.h"

#include "log.h"
#include "rs232_test.h"
#include "peripheral/uart/plib_uart4.h"


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
char buff[32];

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Callback Functions                                                */
/* ************************************************************************** */

/* ************************************************************************** */
static void read_cb (uintptr_t context)
{
  static char reply[sizeof (buff) + 32];

  size_t c = UART4_ReadCountGet ();
  _l ("(RS232) Call-back!\n");
  if (c)
    {
      _dump (buff, c, DUMP_MODE_MIX);
      buff[c] = 0; // null-string
      snprintf (reply, sizeof (reply), "Buffer full (%d): %s\n", c, buff);
      UART4_Write (reply, strlen (reply)); //El buffer que usa el UARTx_Write debe ser estatico, porque lo usa el framework. No lo copia en otro buffer!!!!
    }
  UART4_Read (buff, sizeof (buff) - 1); //Re-arm buffer for next incoming bytes...
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
void rs232_test_init (void)
{
#if !defined(EXT_IN_03_U4RX_RS232_Set) && !defined(EXT_IN_04_U4TX_RS232_Set)
  UART4_ReadCallbackRegister (read_cb, 0); //Call-back when buffer is full!
  UART4_Read (buff, sizeof (buff) - 1);
  _l ("(RS232) Test initialized. Read for eco!\n");
#else
  _l ("(RS232) Not well configured pins for RS232. NOT init!!!!\n");
#endif
}

void rs232_test_task (void)
{
  static char r[sizeof (buff) + 16];

  size_t n = UART4_ReadCountGet ();
  if (n > 0)
    {
      if (buff[n - 1] == '\r')
        {
          _l ("(RS232) Polling!\n");
          _dump (buff, n, DUMP_MODE_MIX);
          buff[n - 1] = 0; // null-string
          snprintf (r, sizeof (r), "Eco(%d): %s\n", n, buff);
          UART4_ReadAbort (); //Flush buffer.
          UART4_Write (r, strlen (r)); //El buffer que usa el UARTx_Write debe ser estatico, porque lo usa el framework. No lo copia en otro buffer!!!!
          UART4_Read (buff, sizeof (buff) - 1); //Re-arm buffer for next incoming bytes...
        }
    }
}

/* ************************************************************** End of File */

