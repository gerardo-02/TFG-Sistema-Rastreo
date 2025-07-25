/* 
 * File:   com.c
 * Author: jtrodriguez
 *
 * Created on 24 de febrero de 2021, 12:03
 */

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "simcom.h"
#include "timers.h"
#include "list.h"
#include "com.h"
#include "at_commands.h"
#include "at_responses.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */
#define WAIT_RESPONSE_TIME_OUT_MS       5000
#define RX_TIMEOUT_MS                   5000
#define COMMAND_RESPONSE_TIMEOUT        5000

#define CR      0x0D
#define LF      0x0A

#define MAX_STRING_LENGTH         (SIMCOM_MAX_MESSAGE_LENGTH + 100)

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Data types & Definitions                                          */
/* ************************************************************************** */

/* ************************************************************************** */
typedef enum
{
  RX_STATE_DUMP = 0,
  //
  RX_STATE_HEAD_CR,
  RX_STATE_HEAD_LF,
  RX_STATE_MESSAGE_OR_PROMPT,
  RX_STATE_CONTINUE_MESSAGE,
  RX_STATE_TAIL_LF,
  //'+'
  RX_STATE_RECEIVE_HEAD,
  RX_STATE_FRAME,
} t_RX_STATES;

typedef struct
{
  t_RX_STATES state;
  uint32_t timeout;
  uint32_t i;
  uint32_t length;
  char s[MAX_STRING_LENGTH];
  t_LIST_NODE *msgList;
} t_RX;

typedef struct
{
  uint32_t i;
  uint32_t length;
  char s[MAX_STRING_LENGTH];
} t_TX;

typedef enum
{
  COMMAND_STATE_IDLE = 0,
  COMMAND_STATE_SEND,
  COMMAND_STATE_ENTER_MSG,
  COMMAND_STATE_WAIT_RESPONSE,
  COMMAND_STATE_END,
} t_COMMAND_STATES;

typedef struct
{
  t_COMMAND_STATES state;
  t_TX tx;
  uint32_t timeout;
  t_COM_COMMAND_CALLBACK_CTX ctx;
  COM_COMMAND_CALLBACK cb;
} t_COMMAND_DATA;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Global data & variables                                           */
/* ************************************************************************** */
/* ************************************************************************** */
static t_RX rx;
static t_COMMAND_DATA commandData;
static t_COM_STATES comState;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Callback Functions                                                */
/* ************************************************************************** */

/* ************************************************************************** */
#define SET_COMMAND_STATE(newState)     commandData.state=newState

// <editor-fold defaultstate="collapsed" desc="UART-RX">
#define SET_STATE_RX(newState)    rx.state=newState

inline static void rx_reset (void)
{
  rx.i = 0;
  timer_off (rx.timeout);
  SET_STATE_RX (RX_STATE_HEAD_CR);
}

inline static uint32_t get_receive_length (char *b, uint32_t l)
{
  uint32_t j, length = 0;

  if (l > 0)
    {
      j = l - 1;
      if (b[j] == CR)
        {
          b[j] = 0; //Separo el string de +RECEIVE de los datos
          j--;
          while (j > 0 && rx.s[j] != ',') j--;
          if (b[j] == ',')
            {
              j++;
              while (b[j])
                {
                  length *= 10;
                  length += (b[j] - '0');
                  j++;
                }
            }
        }
    }

  return length;
}

static void rx_process_byte (uint8_t byte)
{
  // 2.3 Information responses
  // Information responses start and end with <CR><LF>, i.e. the format of 
  // information responses is "<CR><LF><response><CR><LF>". Inside information 
  // responses, there may be one or more <CR><LF>. 
  if (timer_expired (rx.timeout))
    {
      rx.i = 0;
      SET_STATE_RX (RX_STATE_HEAD_CR);
    }
  if (rx.i >= SIMCOM_MAX_MESSAGE_LENGTH) rx.i = 0;
  rx.s[rx.i] = byte;
  timer_ms (rx.timeout, RX_TIMEOUT_MS); //Refresco timeout        
  switch (rx.state)
    {
    case RX_STATE_DUMP:
      rx.i++;
      if (rx.i > 2)
        {
          if ((rx.s[rx.i - 2] == 0x0D) && (rx.s[rx.i-1] == 0x0A))
            {
              rx.s[rx.i - 2] = 0;
              SIMCOM_MSG ("%s\n", rx.s);
              rx.i = 0;
            }
        }
      break;
    case RX_STATE_HEAD_CR:
      switch (rx.s[rx.i])
        {
        case CR:
          SET_STATE_RX (RX_STATE_HEAD_LF);
          break;
        case '+':
          rx.i++;
          SET_STATE_RX (RX_STATE_RECEIVE_HEAD);
          break;
        }
      break;
    case RX_STATE_HEAD_LF:
      switch (rx.s[rx.i])
        {
        case CR: //Elude CR repetidos
          break;
        case LF:
          SET_STATE_RX (RX_STATE_MESSAGE_OR_PROMPT);
          break;
        default:
          rx_reset ();
          break;
        }
      break;
    case RX_STATE_MESSAGE_OR_PROMPT:
      switch (rx.s[rx.i])
        {
        case '>': //Prompt del CIPSEND. 
          commandData.state = COMMAND_STATE_ENTER_MSG; //Cambio el estado a las bravas!
          rx_reset ();
          break;
        case CR:
          SET_STATE_RX (RX_STATE_TAIL_LF);
          break;
        default:
          rx.i++;
          SET_STATE_RX (RX_STATE_CONTINUE_MESSAGE);
          break;
        }
      break;
    case RX_STATE_CONTINUE_MESSAGE:
      if (rx.s[rx.i] == CR)
        {
          SET_STATE_RX (RX_STATE_TAIL_LF);
        }
      else
        {
          rx.i++;
        }
      break;
    case RX_STATE_TAIL_LF:
      switch (rx.s[rx.i])
        {
        case CR: //CONTINUE messages
          rx.s[rx.i] = 0; //null string
          list_add_data (&rx.msgList, rx.s, rx.i + 1);
          rx.i = 0;
          SET_STATE_RX (RX_STATE_HEAD_LF);
          break;
        case LF: //END menssages
          rx.s[rx.i] = 0; //null string
          list_add_data (&rx.msgList, rx.s, rx.i + 1);
          rx_reset ();
          break;
        default:
          rx_reset ();
          break;
        }
      break;
      // Parte '+' (+RECEIVE)
    case RX_STATE_RECEIVE_HEAD:
      if (rx.s[rx.i] == LF)
        { //+RECEIVE,<link_num>,<message_length>{0D}{0A}{...message...}
          rx.length = get_receive_length (rx.s, rx.i);
          if (rx.length > 0)
            {
              SET_STATE_RX (RX_STATE_FRAME);
            }
          else
            {
              rx_reset ();
            }
        }
      else
        {
          rx.i++; //continue
        }
      break;
    case RX_STATE_FRAME:
      rx.length--;
      rx.i++;
      if (!rx.length)
        {
          list_add_data (&rx.msgList, rx.s, rx.i);
          rx_reset ();
        }
      break;
    }
}

static void rx_cb (UART_EVENT event, uintptr_t context)
{
  uint8_t byte;

  switch (event)
    {
    case UART_EVENT_READ_ERROR:
      asm ("NOP");
      SIMCOM_UART_ERROR_GET ();
      break;
    default:
      SIMCOM_UART_READ ((uint8_t*) & byte, 1);
      rx_process_byte (byte);
      break;
    }
}

//static void uart_error_cb (const SYS_MODULE_INDEX index)
//{
//  SIMCOM_MSG ("(SIMCOM-COM) Uart error (%d)!\n", UART_ERROR_GET ());
//}
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="COMMAND RESPONSES CALL BACK">

static void response_ok_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  if (commandData.state == COMMAND_STATE_WAIT_RESPONSE)
    {
      commandData.ctx.res = COMMAND_RES_OK;
      SET_COMMAND_STATE (COMMAND_STATE_END);
    }
}

static void response_error_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  if (commandData.state == COMMAND_STATE_WAIT_RESPONSE)
    {
      commandData.ctx.res = COMMAND_RES_ERROR;
      SET_COMMAND_STATE (COMMAND_STATE_END);
    }
}

static void response_cme_cms_error_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  if (commandData.state == COMMAND_STATE_WAIT_RESPONSE)
    {
      commandData.ctx.res = COMMAND_RES_CME_CMS_ERROR;
      if (ctx->nArgs > 0) commandData.ctx.err = atoi (ctx->arg[0]);
      SET_COMMAND_STATE (COMMAND_STATE_END);
    }
}

static void response_ciperror_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  if (commandData.state == COMMAND_STATE_WAIT_RESPONSE)
    {
      commandData.ctx.res = COMMAND_RES_ERROR;
      if (ctx->nArgs > 0) commandData.ctx.err = atoi (ctx->arg[0]);
    }
}
// </editor-fold>

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Local Functions                                                   */
/* ************************************************************************** */

/* ************************************************************************** */
static void message_tasks (void)
{
  t_LIST_NODE *message;

  if (rx.msgList)
    {
      message = list_pull_node (&rx.msgList);
      SIMCOM_DBG ("SIMCOM: %s\n", (char*) message->data);
      response_manage ((char*) message->data);
      list_free_node (&message);
    }
}

static void command_tasks (void)
{
  switch (commandData.state)
    {
    case COMMAND_STATE_IDLE:
      if (commandData.tx.i < commandData.tx.length)
        {
          commandData.ctx.err = AT_RES_ERROR_CME_CMS_UNKNOWN;
          SET_COMMAND_STATE (COMMAND_STATE_SEND);
        }
      break;
    case COMMAND_STATE_SEND:
      while ((commandData.tx.i < commandData.tx.length) && SIMCOM_UART_WRITE_FREE ())
        {
          SIMCOM_UART_WRITE ((uint8_t*) & commandData.tx.s[commandData.tx.i++], 1); //LLeno el buffer de salida de la uart
        }
      if (commandData.tx.i >= commandData.tx.length)
        {
          commandData.tx.i = commandData.tx.length = 0;
          timer_ms (commandData.timeout, COMMAND_RESPONSE_TIMEOUT);
          SET_COMMAND_STATE (COMMAND_STATE_WAIT_RESPONSE);
        }
      break;
    case COMMAND_STATE_ENTER_MSG:
      if (commandData.cb)
        {
          commandData.ctx.res = COMMAND_RES_ENTER_MSG;
          commandData.cb (&commandData.ctx); //En este call-back se debe hacer un com_write_msg....
        }
      else
        {
          //TODO: Si el modulo esta esperando entrar x bytes se va ha quedar pillado como cancelar comunicacion. No lo se!
          SIMCOM_MSG ("(SIMCOM-COM) Enter message without call-back!\n");
          SET_COMMAND_STATE (COMMAND_STATE_END);
        }
      break;
    case COMMAND_STATE_WAIT_RESPONSE:
      if (timer_expired (commandData.timeout))
        {
          SIMCOM_MSG ("(SIMCOM-COM) Wait response timeout!\n");
          commandData.ctx.res = COMMAND_RES_TIMEOUT;
          SET_COMMAND_STATE (COMMAND_STATE_END);
        }
      break;
    case COMMAND_STATE_END:
      timer_off (commandData.timeout);
      if (commandData.cb) commandData.cb (&commandData.ctx);
      SET_COMMAND_STATE (COMMAND_STATE_IDLE);
      break;
    }
}

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */
void com_time (void)
{
  timer_dec (rx.timeout);
  timer_dec (commandData.timeout);
}

void com_register_uart_call_backs (void)
{
//  RX
//  SET_STATE_RX (RX_STATE_DUMP);
//  SIMCOM_UART_RECEIVER_CALL_BACK (rx_cb, (uintptr_t) NULL);
//  TX
//  DRV_USART0_ByteTransmitCallbackSet (NULL);
//  UART ERROR
//  DRV_USART0_ByteErrorCallbackSet (uart_error_cb);
}

void com_init (void)
{
  //RX - Stream
  SET_STATE_RX (RX_STATE_HEAD_CR);
  timer_off (rx.timeout);
  rx.i = rx.length = 0;
  rx.msgList = NULL;
  //TX - COMMANDS
  SET_COMMAND_STATE (COMMAND_STATE_IDLE);
  commandData.tx.i = commandData.tx.length = 0;
  //Responses init
  response_init ();
  response_set_call_back (AT_RES_OK, response_ok_cb);
  response_set_call_back (AT_RES_ERROR, response_error_cb);
  response_set_call_back (AT_RES_CME_ERROR, response_cme_cms_error_cb);
  response_set_call_back (AT_RES_CIPERROR, response_ciperror_cb);
  //COM state init
  comState = COM_STATE_INIT;
  SIMCOM_MSG ("(SIMCOM-COM) Init\n");
}

t_COM_STATES com_tasks (void)
{
  command_tasks ();
  message_tasks ();
  switch (comState)
    {
    case COM_STATE_INIT:
      com_command (AT_CMD_ECHO_OFF, AT_CMD_STR_LEN (AT_CMD_ECHO_OFF), NULL, NULL);
      comState = COM_STATE_WAIT;
      break;
    case COM_STATE_WAIT:
      if (commandData.state == COMMAND_STATE_IDLE)
        {
          SET_STATE_RX (RX_STATE_HEAD_CR);
          SIMCOM_MSG ("(SIMCOM-COM) Initialized\n");
          comState = COM_STATE_READY;
        }
      break;
    case COM_STATE_READY:
      break;
    }
  return comState;
}

int32_t com_command (char *command, uint32_t commandLength, COM_COMMAND_CALLBACK cb, void *localCtx)
{
  int32_t res = COMMAND_ACEPTED;

  if (commandData.state == COMMAND_STATE_IDLE)
    {
      commandData.tx.length = commandLength;
      memcpy (commandData.tx.s, command, commandLength);
      commandData.cb = cb;
      commandData.ctx.local = localCtx;
      SIMCOM_DBG ("uC: %s\n", command);
      SET_COMMAND_STATE (COMMAND_STATE_SEND);
    }
  else
    {
      res = COMMAND_BUSY;
    }

  return res;
}

int32_t com_write_message (uint8_t *msg, uint32_t msgLength, COM_COMMAND_CALLBACK cb, void *localCtx)
{
  int32_t res = COMMAND_ACEPTED;

  if (commandData.state == COMMAND_STATE_ENTER_MSG)
    {
      commandData.tx.length = msgLength;
      memcpy (commandData.tx.s, msg, msgLength);
      commandData.cb = cb;
      commandData.ctx.local = localCtx;
      SIMCOM_DBG ("uC: write %d bytes message\n", msgLength);
      SET_COMMAND_STATE (COMMAND_STATE_SEND);
    }
  else
    {
      res = COMMAND_BUSY;
    }

  return res;
}

/* ************************************************************** End of File */
