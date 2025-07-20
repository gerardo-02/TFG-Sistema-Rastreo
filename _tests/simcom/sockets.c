/* 
 * File:   sockets.c
 * Author: jtrodriguez
 *
 * Created on 10 de marzo de 2021, 14:30
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
#include <stdio.h>

#include "com.h"
#include "simcom.h"
#include "at_commands.h"
#include "at_responses.h"
#include "list.h"
#include "sockets.h"
#include "timers.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */
#define TIME_FOR_ERROR_RETRY_MS     20000
#define TIME_FOR_CLIENT_RETRY_MS    5000
#define TIME_FOR_BUSY_RETRY_MS      300
#define TIME_MAX_IDLE_OPEN_SOCKET   3000
#define TIME_MAX_CLOSING_SOCKET     5000
#define TIME_FOR_IDLE_CHECK_MS      500

#define AUX_STRING_MAX_LENGTH       64

#define NOT_AS_A_TCP_SERVER         (-1)

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Data types & Definitions                                          */
/* ************************************************************************** */

/* ************************************************************************** */
typedef enum
{
  SOCKET_STATE_IDLE = 0,
  SOCKET_STATE_PREPARE_OPEN,
  SOCKET_STATE_OPEN_COMMAND,
  SOCKET_STATE_PREPARE_SEND,
  SOCKET_STATE_SEND_COMMAND,
  SOCKET_STATE_PREPARE_CLOSE,
  SOCKET_STATE_CLOSE_COMMAND,
  //
  SOCKET_STATE_WAIT_RESPONSE,
} t_SOCKET_STATES;

typedef struct
{
  t_SIMCOM_OPERATION_RESULT op_res;
  void *local;
} t_OPEN_CALLBACK_CTX;
typedef void (*OPEN_CALLBACK)(t_OPEN_CALLBACK_CTX *ctx);

typedef struct
{
  t_SOCKET_STATES state;
  t_SOCKET_STATES savedState;
  uint32_t openCloseTimeout;
  uint32_t retryTimeout;
  uint32_t so_index;
  t_IPV4_ADDR ip;
  uint16_t port;
  char cmdAux[AUX_STRING_MAX_LENGTH];
  t_LIST_NODE *out;
  OPEN_CALLBACK openCallBack;
  t_OPEN_CALLBACK_CTX openCtx;
  SIMCOM_SENT_CALLBACK msgSentCallBack;
  t_SIMCOM_SENT_CALLBACK_CTX msgSentCtx;
} t_SOCKET_DATA;

typedef struct
{
  t_IPV4_ADDR ip;
  uint16_t port;
  SIMCOM_SENT_CALLBACK cb;
} t_ADDRESSEE_INFO;

typedef struct
{
  uint8_t curIndexClient;
  t_LIST_NODE *recipients; //list de destinatarios (El primer destinatario va con el primer mensaje. Casados 1 a 1)
  t_LIST_NODE *posts; //lista de mensajes (El primer mensaje va con el primer Destinatario. Casados 1 a 1)
} t_CLIENT_OUTBOX;

typedef struct
{
  t_SOCKET_SERVICE_STATES state;
  t_SOCKET_SERVICE_STATES savedState;
  uint32_t timeout;
  bool wait;
  bool socketsReady;
  t_CLIENT_OUTBOX outbox;
} t_SOCKET_SERVICE_DATA;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Global data & variables                                           */
/* ************************************************************************** */
/* ************************************************************************** */
extern t_SIMCOM_DATA simcomData;
static t_SOCKET_SERVICE_DATA socketServiceData;
static t_SOCKET_DATA socket[SIMCOM_MAX_TCP_SOCKETS];

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Callback Functions                                                */
/* ************************************************************************** */
/* ************************************************************************** */

#define SET_SOCKET_STATE(index,newState)     socket[index].state=newState

// <editor-fold defaultstate="collapsed" desc="SOCKET-OPEN">

static void cmd_open_cb (t_COM_COMMAND_CALLBACK_CTX *ctx)
{
  t_SOCKET_DATA *sd = (t_SOCKET_DATA*) (ctx->local);
  if (ctx->res != COMMAND_RES_OK)
    {
      SIMCOM_MSG ("(SIMCOM-SOCKET) Command open error. %d\n", sd->so_index);
    }
}

static void res_socket_open_status_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  uint8_t err;
  uint8_t link_num;

  if (ctx->nArgs > 0)
    {
      link_num = atoi (ctx->arg[0]);
      switch (ctx->nArgs)
        {
        case 1: //Read cipopen socket closed
          timer_off (socket[link_num].openCloseTimeout);
          simcomData.status.socket[link_num].state = SIMCOM_SOCKET_STATE_CLOSE;
          break;
        case 2: //Write cipopen result
          err = atoi (ctx->arg[1]);
          switch (err)
            {
            case US_TCP_ERR_OPERATION_SUCCESS:
              SIMCOM_MSG ("(SIMCOM-SOCKET) Socket %d opened\n", link_num);
              socket[link_num].openCtx.op_res = SIMCOM_OPER_SUCCESS;
              timer_ms (socket[link_num].openCloseTimeout, TIME_MAX_IDLE_OPEN_SOCKET);
              simcomData.status.socket[link_num].server_index = NOT_AS_A_TCP_SERVER;
              simcomData.status.socket[link_num].state = SIMCOM_SOCKET_STATE_OPEN;
              break;
            case US_TCP_ERR_NETWORK_FAILURE:
              SIMCOM_MSG ("(SIMCOM-SOCKET) Network failure!\n", link_num);
              socket[link_num].openCtx.op_res = SIMCOM_OPER_ERROR;
              simcomData.status.socket[link_num].state = SIMCOM_SOCKET_STATE_FAIL;
              break;
            case US_TCP_ERR_SOCKETS_OPENED:
              SIMCOM_MSG ("(SIMCOM-SOCKET) Atencion! Socket %d already opened\n", link_num);
              socket[link_num].openCtx.op_res = SIMCOM_OPER_ERROR;
              simcomData.status.socket[link_num].state = SIMCOM_SOCKET_STATE_OPEN;
              break;
            case US_TCP_ERR_TIMEOUT:
              SIMCOM_MSG ("(SIMCOM-SOCKET) Open socket timeout! Reset...\n", link_num);
              socket[link_num].openCtx.op_res = SIMCOM_OPER_ERROR;
              simcomData.status.socket[link_num].state = SIMCOM_SOCKET_STATE_FAIL;
              simcom_reset_module ();
              break;
            default:
              SIMCOM_MSG ("(SIMCOM-SOCKET) Socket %d open error\n", link_num);
              socket[link_num].openCtx.op_res = SIMCOM_OPER_ERROR;
              simcomData.status.socket[link_num].state = SIMCOM_SOCKET_STATE_FAIL;
              break;
            }
          if (socket[link_num].openCallBack)
            {
              socket[link_num].openCallBack (&socket[link_num].openCtx);
              socket[link_num].openCallBack = NULL;
            }
          break;
        case 5: //Read cipopen open socket info
          simcomData.status.socket[link_num].state = SIMCOM_SOCKET_STATE_OPEN;
          //ctx->args[1]    Tipo TCP o UDP
          //ctx->args[2]    Server IP  
          socket[link_num].ip.v = 0;
          //ctx->args[3]    Server Port
          socket[link_num].port = 0;
          simcomData.status.socket[link_num].server_index = atoi (ctx->arg[4]);
          break;
        default: //caso desconocido
          break;
        }
      SET_SOCKET_STATE (link_num, SOCKET_STATE_IDLE);
    }
  else
    {
      SIMCOM_MSG ("(SIMCOM-SOCKET-SERVICE) CIPOPEN arguments error!\n");
    }
}
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="SOCKET-CLOSE">

static void cmd_close_cb (t_COM_COMMAND_CALLBACK_CTX *ctx)
{
  t_SOCKET_DATA *sd = (t_SOCKET_DATA*) (ctx->local);
  if (ctx->res != COMMAND_RES_OK)
    {
      SIMCOM_MSG ("(SIMCOM-SOCKET) Command close error. %d\n", sd->so_index);
    }
  SET_SOCKET_STATE (sd->so_index, SOCKET_STATE_IDLE);
}

static void res_socket_close_command_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  uint8_t err;
  uint8_t link_num;

  if (ctx->nArgs == 2)
    {
      link_num = atoi (ctx->arg[0]);
      err = atoi (ctx->arg[1]);
      if (err != US_TCP_ERR_OPERATION_SUCCESS)
        {
          SIMCOM_MSG ("(SIMCOM-SOCKET-SERVICE) Close socket %d, command error: %d\n", link_num, err);
        }
      timer_off (socket[link_num].openCloseTimeout);
      simcomData.status.socket[link_num].state = SIMCOM_SOCKET_STATE_CLOSE;
    }
  else
    {
      SIMCOM_MSG ("(SIMCOM-SOCKET-SERVICE) CIPCLOSE arguments error!\n");
    }
}
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="SOCKET-SEND">

static void cmd_send_cb (t_COM_COMMAND_CALLBACK_CTX *ctx)
{
  t_SOCKET_DATA *sd = (t_SOCKET_DATA*) (ctx->local);
  bool error = false;

  switch (ctx->res)
    {
    case COMMAND_RES_ENTER_MSG:
      if (com_write_message (socket[sd->so_index].out->data, socket[sd->so_index].out->size, cmd_send_cb, &socket[sd->so_index]) != COMMAND_ACEPTED)
        {
          SIMCOM_MSG ("(SIMCOM-SOCKET) Command send error on %d. Entering message.\n", sd->so_index);
          error = true;
        }
      break;
    case COMMAND_RES_OK:
      //Nada. Espera a +CIPSEND...
      break;
    case COMMAND_RES_ERROR:
      if (ctx->err == US_TCP_ERR_BUSY)
        {
          //Al parecer si se está a la espera de un +CIPSEND: <link_num>,<reqSendLength>, <cnfSendLength>, no se puede hacer otro AT+CIPSEND porque te da un +CIPERROR: 8. Busy! Pero no significa que haya nada roto.
          SIMCOM_MSG ("(SIMCOM-SOCKET) (%d) Command send busy. Retry...\n", sd->so_index, ctx->err);
          timer_ms (socket[sd->so_index].retryTimeout, TIME_FOR_BUSY_RETRY_MS);
          SET_SOCKET_STATE (sd->so_index, SOCKET_STATE_SEND_COMMAND);
        }
      break;
    default:
      SIMCOM_MSG ("(SIMCOM-SOCKET) (%d) Command send error: %d\n", sd->so_index, ctx->err);
      error = true;
      break;
    }
  if (error)
    {
      timer_off (socket[sd->so_index].retryTimeout);
      simcomData.status.socket[sd->so_index].state = SIMCOM_SOCKET_STATE_FAIL;
      if (socket[sd->so_index].msgSentCallBack)
        {
          socket[sd->so_index].msgSentCtx.op_res = SIMCOM_OPER_ERROR;
          socket[sd->so_index].msgSentCallBack (&socket[sd->so_index].msgSentCtx);
          socket[sd->so_index].msgSentCallBack = NULL;
        }
      SET_SOCKET_STATE (sd->so_index, SOCKET_STATE_IDLE);
    }
}

static void res_socket_send_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  uint16_t reqSendLength, cnfSendLength;
  uint8_t link_num;
  t_LIST_NODE *msg;

  if (ctx->nArgs == 3)
    {
      link_num = atoi (ctx->arg[0]);
      reqSendLength = atoi (ctx->arg[1]);
      cnfSendLength = atoi (ctx->arg[2]);
      // <cnfSendLength> is a numeric parameter that confirmed number of data
      // bytes to be transmitted:
      // = -1 the connection is disconnected.
      // = 0 own s1end buffer or other side?s congestion window are full.
      // Note: If the <cnfSendLength> is not equal to the <reqSendLength>, the socket then cannot be used further.      
      if (reqSendLength == cnfSendLength)
        {
          msg = list_pull_node (&socket[link_num].out);
          list_free_node (&msg);
          socket[link_num].msgSentCtx.op_res = SIMCOM_OPER_SUCCESS;
        }
      else
        {
          SIMCOM_MSG ("(SIMCOM-SOCKET-SERVICE) Send error (%d)\n", link_num);
          socket[link_num].msgSentCtx.op_res = SIMCOM_OPER_ERROR;
          simcomData.status.socket[link_num].state = SIMCOM_SOCKET_STATE_FAIL;
        }
      //External Call-back
      if (socket[link_num].msgSentCallBack)
        {
          socket[link_num].msgSentCallBack (&socket[link_num].msgSentCtx);
          socket[link_num].msgSentCallBack = NULL;
        }
      SET_SOCKET_STATE (link_num, SOCKET_STATE_IDLE);
    }
  else
    {
      SIMCOM_MSG ("(SIMCOM-SOCKET-SERVICE) CIPSEND arguments error\n");
    }
}
// </editor-fold>

// <editor-fold defaultstate="collapsed" desc="SOCKET SERVICE">

static void cmd_get_socket_service_status_cb (t_COM_COMMAND_CALLBACK_CTX *ctx)
{
  if (ctx->res == COMMAND_RES_OK)
    {
      socketServiceData.state = SOCKET_SERVICE_STATE_CLEAN_UP;
    }
  else
    {
      socketServiceData.wait = true;
      timer_ms (socketServiceData.timeout, TIME_FOR_ERROR_RETRY_MS);
      socketServiceData.state = SOCKET_SERVICE_STATE_GET_STATUS;
      SIMCOM_MSG ("(SIMCOM-SOCKET-SERVICE) ERROR! %d\n", socketServiceData.savedState);
    }
}

static void res_cause_socket_close_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  uint8_t cause;
  uint8_t client_index;

  if (ctx->nArgs == 2)
    {
      client_index = atoi (ctx->arg[0]);
      cause = atoi (ctx->arg[1]);
      SIMCOM_MSG ("(SIMCOM-SOCKET-SERVICE) Socket %d close. Cause: %d\n", client_index, cause);
      if (socket[client_index].out)
        {
          list_delete (&socket[client_index].out);
          SIMCOM_MSG ("(SIMCOM-SOCKET-SERVICE)(%d) Habia datos no enviados!\n", client_index);
        }
      timer_off (socket[client_index].openCloseTimeout);
      simcomData.status.socket[client_index].state = SIMCOM_SOCKET_STATE_CLOSE;
      SET_SOCKET_STATE (client_index, SOCKET_STATE_IDLE);
    }
  else
    {
      SIMCOM_MSG ("(SIMCOM-SOCKET-SERVICE) IPCLOSE arguments error\n");
    }
}

static void res_socket_client_cb (t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  uint8_t link_num;
  int8_t server_index;

  if (ctx->nArgs == 3)
    {
      link_num = atoi (ctx->arg[0]);
      server_index = atoi (ctx->arg[1]);
      SIMCOM_DBG ("(SIMCOM-SOCKET-SERVICE) Socket %d incomming connection from: %s, to server: %d\n", link_num, ctx->arg[2], server_index);
      if (socket[link_num].state != SOCKET_STATE_IDLE)
        {
          SIMCOM_MSG ("(SIMCOM-SOCKET-SERVICE) Socket %d collision!\n", link_num);
        }
      simcomData.status.socket[link_num].server_index = server_index;
      // ctx->args[2] es IP:Port
      socket[link_num].ip.v = 0;
      socket[link_num].port = 0;
      simcomData.status.socket[link_num].state = SIMCOM_SOCKET_STATE_OPEN;
    }
  else
    {
      SIMCOM_MSG ("(SIMCOM-SOCKET-SERVICE) IPCLOSE arguments error\n");
    }
}

static void client_open_cb (t_OPEN_CALLBACK_CTX *ctx)
{
  if (ctx->op_res == SIMCOM_OPER_SUCCESS)
    {
      SIMCOM_DBG ("Client socket %d! abierto\n", socketServiceData.outbox.curIndexClient);
      socketServiceData.state = SOCKET_SERVICE_STATE_CLIENT_DELIVER_TO_SOCKET;
    }
  else
    {
      SIMCOM_MSG ("(SIMCOM-SOCKET) Error abriendo client socket %d!\n", socketServiceData.outbox.curIndexClient);
      socketServiceData.wait = true;
      timer_ms (socketServiceData.timeout, TIME_FOR_CLIENT_RETRY_MS);
      socketServiceData.state = SOCKET_SERVICE_STATE_CLIENT_OPEN_ERROR;
    }
}

// </editor-fold>

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Local Functions                                                   */
/* ************************************************************************** */

/* ************************************************************************** */
static uint8_t search_active_socket_with (t_IPV4_ADDR ip, uint16_t port)
{
  uint8_t i = 0;
  bool find = false;

  while ((i < SIMCOM_MAX_TCP_SOCKETS) && !find)
    {
      if (simcomData.status.socket[i].state == SIMCOM_SOCKET_STATE_OPEN)
        {
          if (simcomData.status.socket[i].server_index == NOT_AS_A_TCP_SERVER)
            {
              if ((socket[i].ip.v == ip.v) && (socket[i].port == port)) find = true;
            }
        }
      if (!find) i++;
    }

  return i;
}

static uint8_t search_idle_socket (void)
{
  uint8_t i = 0;

  while ((i < SIMCOM_MAX_TCP_SOCKETS) && (simcomData.status.socket[i].state != SIMCOM_SOCKET_STATE_CLOSE)) i++;

  return i;
}

static void socket_execute_command (uint32_t index, char *command, COM_COMMAND_CALLBACK cb, void* localCtx)
{
  if (com_command (command, strlen (command), cb, localCtx) == COMMAND_ACEPTED)
    {
      socket[index].savedState = socket[index].state;
      SET_SOCKET_STATE (index, SOCKET_STATE_WAIT_RESPONSE);
    }
}

static void socket_run (uint32_t index)
{
  switch (socket[index].state)
    {
    case SOCKET_STATE_IDLE:
      switch (simcomData.status.socket[index].state)
        {
        case SIMCOM_SOCKET_STATE_CLOSE:
          break;
        case SIMCOM_SOCKET_STATE_OPEN:
          if (socket[index].out)
            {
              timer_ms (socket[index].openCloseTimeout, TIME_MAX_IDLE_OPEN_SOCKET);
              SET_SOCKET_STATE (index, SOCKET_STATE_PREPARE_SEND);
            }
          else
            {
              if (simcomData.status.socket[index].server_index == NOT_AS_A_TCP_SERVER)
                {
                  //Si es un socket perteneciente a un cliente entonces miro si ha experido. Si fuera un socket de un servidor lo dejo abierto. que lo cierre el cliente que lo habrio...
                  if (timer_expired (socket[index].openCloseTimeout))
                    {
                      SIMCOM_MSG ("(SIMCOM-SOCKET)(%d) No hay nada mas que enviar! Closing...\n", index);
                      timer_off (socket[index].openCloseTimeout);
                      SET_SOCKET_STATE (index, SOCKET_STATE_PREPARE_CLOSE);
                    }
                }
            }
          break;
        case SIMCOM_SOCKET_STATE_FAIL:
          if (timer_active (socket[index].openCloseTimeout))
            {
              if (timer_expired (socket[index].openCloseTimeout))
                {                
                  SIMCOM_MSG ("(SIMCOM-SOCKET)(%d) Fail! Closing...\n", index);
                  timer_ms (socket[index].openCloseTimeout, TIME_MAX_CLOSING_SOCKET);
                  SET_SOCKET_STATE (index, SOCKET_STATE_PREPARE_CLOSE);
                }
            }
          else
            {
              SIMCOM_MSG ("(SIMCOM-SOCKET)(%d) Fail! Closing...\n", index);
              timer_ms (socket[index].openCloseTimeout, TIME_MAX_CLOSING_SOCKET);
              SET_SOCKET_STATE (index, SOCKET_STATE_PREPARE_CLOSE);
            }
          break;
        }
      break;
      //OPEN
    case SOCKET_STATE_PREPARE_OPEN:
      snprintf (socket[index].cmdAux, AUX_STRING_MAX_LENGTH, "%s=%d,\"TCP\",\"%s\",%d\r", AT_CMD_OPEN_SOCKET, index, iph_ip2str (socket[index].ip), socket[index].port);
      socket[index].so_index = index;
      SET_SOCKET_STATE (index, SOCKET_STATE_OPEN_COMMAND);
      break;
    case SOCKET_STATE_OPEN_COMMAND:
      socket_execute_command (index, socket[index].cmdAux, cmd_open_cb, &socket[index]);
      break;
      //SEND
    case SOCKET_STATE_PREPARE_SEND:
      snprintf (socket[index].cmdAux, AUX_STRING_MAX_LENGTH, "%s=%d,%d\r", AT_CMD_CIPSEND, index, socket[index].out->size);
      socket[index].so_index = index;
      SET_SOCKET_STATE (index, SOCKET_STATE_SEND_COMMAND);
      break;
    case SOCKET_STATE_SEND_COMMAND:
      if (timer_active (socket[index].retryTimeout))
        {
          if (timer_expired (socket[index].retryTimeout))
            {
              timer_off (socket[index].retryTimeout);
              socket_execute_command (index, socket[index].cmdAux, cmd_send_cb, &socket[index]); //En el call-back se envian los datos si todo es OK
            }
        }
      else
        {
          socket_execute_command (index, socket[index].cmdAux, cmd_send_cb, &socket[index]); //En el call-back se envian los datos si todo es OK
        }
      break;
      //CLOSE
    case SOCKET_STATE_PREPARE_CLOSE:
      if (socket[index].out)
        {
          list_delete (&socket[index].out);
          SIMCOM_MSG ("(SIMCOM-SOCKET)(%d) Habia datos no enviados!\n", index);
        }
      snprintf (socket[index].cmdAux, AUX_STRING_MAX_LENGTH, "%s=%d\r", AT_CMD_CLOSE_SOCKET, index);
      socket[index].so_index = index;
      SET_SOCKET_STATE (index, SOCKET_STATE_CLOSE_COMMAND);
      break;
    case SOCKET_STATE_CLOSE_COMMAND:
      socket_execute_command (index, socket[index].cmdAux, cmd_close_cb, &socket[index]);
      break;
      //
    case SOCKET_STATE_WAIT_RESPONSE:
      break;
    }
}

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */
void sockets_service_time (void)
{
  uint32_t si;

  for (si = 0; si < SIMCOM_MAX_TCP_SOCKETS; si++)
    {
      timer_dec (socket[si].retryTimeout);
      timer_dec (socket[si].openCloseTimeout);
    }
  timer_dec (socketServiceData.timeout);
}

void sockets_service_init (void)
{
  memset (&socketServiceData, 0, sizeof (t_SOCKET_SERVICE_DATA));
  // Register call backs
  response_set_call_back (AT_RES_CIPOPEN, res_socket_open_status_cb);
  response_set_call_back (AT_RES_CIPCLOSE, res_socket_close_command_cb);
  response_set_call_back (AT_RES_IPCLOSE, res_cause_socket_close_cb);
  response_set_call_back (AT_RES_CLIENT, res_socket_client_cb);
  response_set_call_back (AT_RES_CIPSEND, res_socket_send_cb);
  SIMCOM_MSG ("(SIMCOM-SOCKETS) Init...\n");
  socketServiceData.state = SOCKET_SERVICE_STATE_INIT;
}

t_SOCKET_SERVICE_STATES sockets_service_tasks (void)
{
  static uint8_t csi; // Current socket index
  t_ADDRESSEE_INFO *addresseeInfo;
  t_LIST_NODE *nodeAuxPost, *nodeAuxRecipient;
  t_SIMCOM_SENT_CALLBACK_CTX auxCtx;

  if (socketServiceData.wait)
    {
      if (timer_expired (socketServiceData.timeout))
        {
          socketServiceData.wait = false;
          timer_off (socketServiceData.timeout);
        }
    }
  else
    {
      switch (socketServiceData.state)
        {
        case SOCKET_SERVICE_STATE_INIT:
          socketServiceData.state = SOCKET_SERVICE_STATE_GET_STATUS;
          break;
        case SOCKET_SERVICE_STATE_GET_STATUS:
          if (com_command (AT_CMD_READ_CIPOPEN, AT_CMD_STR_LEN (AT_CMD_READ_CIPOPEN), cmd_get_socket_service_status_cb, NULL) == COMMAND_ACEPTED)
            {
              socketServiceData.savedState = socketServiceData.state;
              socketServiceData.state = SOCKET_SERVICE_STATE_WAIT_RESPONSE;
            }
          break;
        case SOCKET_SERVICE_STATE_CLEAN_UP: //Si al iniciar hay socket abierto los pongo en FAIL para que se cierren
          for (csi = 0; csi < SIMCOM_MAX_TCP_SOCKETS; csi++)
            {
              memset (&socket[csi], 0, sizeof (t_SOCKET_DATA));
              if (simcomData.status.socket[csi].state == SIMCOM_SOCKET_STATE_OPEN)
                {
                  simcomData.status.socket[csi].state = SIMCOM_SOCKET_STATE_FAIL;
                }
              socket[csi].state = SOCKET_STATE_IDLE;
            }
          csi = 0;
          socketServiceData.socketsReady = true;
          socketServiceData.state = SOCKET_SERVICE_STATE_IDLE;
          break;
        case SOCKET_SERVICE_STATE_IDLE:
          if (socketServiceData.outbox.posts)
            {
              SIMCOM_DBG ("Hay cosas que enviar, por cliente...\n");
              addresseeInfo = (t_ADDRESSEE_INFO*) socketServiceData.outbox.recipients->data;
              socketServiceData.outbox.curIndexClient = search_active_socket_with (addresseeInfo->ip, addresseeInfo->port);
              if (socketServiceData.outbox.curIndexClient < SIMCOM_MAX_TCP_SOCKETS)
                {
                  SIMCOM_DBG ("Hay cliente activo para esa ip-puerto: %d\n", socketServiceData.outbox.curIndexClient);
                  socketServiceData.state = SOCKET_SERVICE_STATE_CLIENT_DELIVER_TO_SOCKET;
                }
              else
                {
                  SIMCOM_DBG ("No hay socket activo para esa direccion. Busco uno libre...\n");
                  socketServiceData.outbox.curIndexClient = search_idle_socket ();
                  if (socketServiceData.outbox.curIndexClient < SIMCOM_MAX_TCP_SOCKETS)
                    {
                      SIMCOM_DBG ("Socket %d disponible. Opening...\n", socketServiceData.outbox.curIndexClient);
                      socket[socketServiceData.outbox.curIndexClient].ip = addresseeInfo->ip;
                      socket[socketServiceData.outbox.curIndexClient].port = addresseeInfo->port;
                      socket[socketServiceData.outbox.curIndexClient].openCallBack = client_open_cb;
                      socket[socketServiceData.outbox.curIndexClient].state = SOCKET_STATE_PREPARE_OPEN;
                      socketServiceData.state = SOCKET_SERVICE_STATE_CLIENT_WAIT_OPEN;
                    }
                  else
                    {
                      SIMCOM_DBG ("No hay socket disponibles!\n");
                      socketServiceData.wait = true;
                      timer_ms (socketServiceData.timeout, TIME_FOR_CLIENT_RETRY_MS);
                      socketServiceData.state = SOCKET_SERVICE_STATE_IDLE;
                    }
                }
            }
          break;
        case SOCKET_SERVICE_STATE_CLIENT_WAIT_OPEN:
          break;
        case SOCKET_SERVICE_STATE_CLIENT_OPEN_ERROR:
          nodeAuxPost = list_pull_node (&socketServiceData.outbox.posts);
          nodeAuxRecipient = list_pull_node (&socketServiceData.outbox.recipients);
          if (nodeAuxPost && nodeAuxRecipient)
            {
              addresseeInfo = (t_ADDRESSEE_INFO*) nodeAuxRecipient->data;
              if (addresseeInfo->cb)
                {
                  auxCtx.op_res = SIMCOM_OPER_ERROR;
                  addresseeInfo->cb (&auxCtx);
                }
              list_free_node (&nodeAuxPost);
              list_free_node (&nodeAuxRecipient);
            }
          else
            {
              SIMCOM_MSG ("(SIMCOM-SOCKETS)(%d) Hubo algun error en open error socket.\n", socketServiceData.outbox.curIndexClient);
            }
          socketServiceData.state = SOCKET_SERVICE_STATE_IDLE;
          break;
        case SOCKET_SERVICE_STATE_CLIENT_DELIVER_TO_SOCKET:
          //Paso los datos del mensaje a la lista de salida del socket en cuestion
          nodeAuxPost = list_pull_node (&socketServiceData.outbox.posts);
          nodeAuxRecipient = list_pull_node (&socketServiceData.outbox.recipients);
          if (nodeAuxPost && nodeAuxRecipient)
            {
              addresseeInfo = (t_ADDRESSEE_INFO*) nodeAuxRecipient->data;
              sockets_send (socketServiceData.outbox.curIndexClient, nodeAuxPost->data, nodeAuxPost->size, addresseeInfo->cb);
              list_free_node (&nodeAuxPost);
              list_free_node (&nodeAuxRecipient);
            }
          else
            {
              SIMCOM_MSG ("(SIMCOM-SOCKETS)(%d) Hubo algun error en deliver to socket.\n", socketServiceData.outbox.curIndexClient);
            }
          socketServiceData.state = SOCKET_SERVICE_STATE_IDLE;
          break;
          //
        case SOCKET_SERVICE_STATE_WAIT_RESPONSE:
          break;
        }
    }
  //Sockets
  if (socketServiceData.socketsReady)
    {
      socket_run (csi);
      if (++csi >= SIMCOM_MAX_TCP_SOCKETS) csi = 0;
    }

  return socketServiceData.state;
}

int32_t sockets_send (uint8_t index, uint8_t *data, uint32_t dataLength, SIMCOM_SENT_CALLBACK cb)
{
  int32_t err = 0;

  if (list_add_data (&socket[index].out, data, dataLength) == 0)
    {
      socket[index].msgSentCallBack = cb;
    }
  else
    {
      err = -1;
      SIMCOM_MSG ("(SIMCOM-SOCKETS)(%d) Error insertando mensaje en el out del socket.\n", index);
    }
  return err;
}

int32_t sockets_tcp_send (t_IPV4_ADDR ip, uint16_t port, uint8_t *data, uint32_t dataLength, SIMCOM_SENT_CALLBACK cb)
{
  t_ADDRESSEE_INFO addrInfo;
  t_LIST_NODE *nodeAux;
  int32_t err = 0;

  addrInfo.ip = ip;
  addrInfo.port = port;
  addrInfo.cb = cb;
  if (list_add_data (&socketServiceData.outbox.recipients, &addrInfo, sizeof (t_ADDRESSEE_INFO)) == 0)
    {
      if (list_add_data (&socketServiceData.outbox.posts, data, dataLength) != 0)
        {
          nodeAux = list_pull_node (&socketServiceData.outbox.recipients);
          list_free_node (&nodeAux);
          err = -1;
          SIMCOM_MSG ("(SIMCOM-SOCKETS) Fatal ERROR! Insertando mensaje en el outbox.\n");
        }
    }
  else
    {
      err = -1;
      SIMCOM_MSG ("(SIMCOM-SOCKETS) Fatal ERROR! Info remitente en el outbox.\n");
    }

  return err;
}

/* ************************************************************** End of File */
