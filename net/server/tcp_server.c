#include "log.h"
#include "lists.h"
#include "../net.h"

#include "tcp_server.h"


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
typedef enum
{
  TX_STATE_IDLE = 0,
  TX_STATE_SENDING,
  TX_STATE_ERROR,
} t_TX_STATES;

typedef struct
{
  t_TX_STATES state;
  t_LIST_NODE *queueData;
  int32_t dataIndex;
} t_OUT_TX;

typedef enum
{
  TCP_SERVER_STATE_INIT = 0,
  TCP_SERVER_STATE_OPEN_SOCKET,
  TCP_SERVER_STATE_WAIT_FOR_CONNECTION,
  TCP_SERVER_STATE_CONNECTION_STABLISHED,
  TCP_SERVER_STATE_DISCONNECTING,
  TCP_SERVER_STATE_WAIT_DISCONNECT,
  TCP_SERVER_STATE_ABORTING,
  TCP_SERVER_STATE_WAIT_ABORT,
  TCP_SERVER_STATE_CLOSE,
  TCP_SERVER_STATE_WAIT_CLOSE,
  TCP_SERVER_STATE_DISABLE,
} t_TCP_SERVER_SM_STATES;

typedef struct
{
  bool inUse;
  TCP_PORT listenPort;
  TCP_SOCKET hSocket;
  t_TCP_SERVER_SM_STATES state;
  IPV4_ADDR remoteAddress;
  TCP_PORT remotePort;
  t_OUT_TX out;
  TCP_SERVER_RX_PROCESSOR rxP;
  TCP_SERVER_EVENT_CALLBACK cb;
}
t_TCP_SERVER_DATA;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Global data & variables                                           */
/* ************************************************************************** */
/* ************************************************************************** */
static t_TCP_SERVER_DATA serverData[TCP_SERVER_MAX_SOCKETS];

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Callback Functions                                                */
/* ************************************************************************** */
/* ************************************************************************** */
#define GET_SERVER_INDEX_FROM_POINTER(s)    (((uint32_t)s-(uint32_t)(&serverData[0]))/sizeof(t_TCP_SERVER_DATA))

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */
#define SET_TCP_SERVER_STATE(i,newState)    serverData[i].state=newState

static void emit_event (int32_t serverIndex, t_SERVER_EVENTS e)
{
  t_TCP_SERVER_EVENT_CALLBACK_CONTEXT ctx;

  if (serverData[serverIndex].cb)
    {
      ctx.serverIndex = serverIndex;
      ctx.event = e;
      serverData[serverIndex].cb (&ctx);
    }
}

static void tcp_event_callback (TCP_SOCKET hTCP, TCPIP_NET_HANDLE hNet, TCPIP_TCP_SIGNAL_TYPE sigType, const void* param)
{
  t_TCP_SERVER_DATA *sd = (t_TCP_SERVER_DATA*) param;
  TCP_SOCKET_INFO tcpSockInfo;
  char ipStr[TCP_NET_MAX_IP_STR_LENGTH];

  switch (sigType)
    {
    case TCPIP_TCP_SIGNAL_TX_DONE:
      //      _l ("(TCP-SERVER)(s=%d) Signal tx done. Do nothing!\n", (int) hTCP);
      break;
    case TCPIP_TCP_SIGNAL_TX_DATA_DONE:
      //      _l ("(TCP-SERVER)(s=%d) Signal tx data done. Do nothing!\n", (int) hTCP);
      break;
    case TCPIP_TCP_SIGNAL_TX_SPACE:
      //      _l ("(TCP-SERVER)(s=%d) Signal tx space. Do nothing!\n", (int) hTCP);
      break;
    case TCPIP_TCP_SIGNAL_TX_RST:
      //      _l ("(TCP-SERVER)(s=%d) Signal tx reset. Do nothing!\n", (int) hTCP);
      break;
      // RX related signals
    case TCPIP_TCP_SIGNAL_ESTABLISHED:
      TCPIP_TCP_SocketInfoGet (hTCP, &tcpSockInfo);
      sd->remoteAddress = tcpSockInfo.remoteIPaddress.v4Add;
      sd->remotePort = tcpSockInfo.remotePort;
      TCPIP_Helper_IPAddressToString (&sd->remoteAddress, ipStr, TCP_NET_MAX_IP_STR_LENGTH);
      _l ("(TCP-SERVER)(sk=%d)(si=%d) Conection stablished to remote %s:%d\n", (int) hTCP, GET_SERVER_INDEX_FROM_POINTER (sd), ipStr, (int) sd->remotePort);
      sd->state = TCP_SERVER_STATE_CONNECTION_STABLISHED; // Connection stablished...
      emit_event (GET_SERVER_INDEX_FROM_POINTER (sd), SERVER_EVENT_CONNECTION_STABLISHED);
      break;
    case TCPIP_TCP_SIGNAL_RX_DATA:
      //      _l ("(TCP-SERVER)(s=%d) Signal rx data. Do nothing!\n", (int) hTCP);
      break;
    case TCPIP_TCP_SIGNAL_RX_FIN:
      sd->state = TCP_SERVER_STATE_DISCONNECTING;
      _l ("(TCP-SERVER)(sk=%d)(si=%d) Request disconnect...\n", (int) hTCP, GET_SERVER_INDEX_FROM_POINTER (sd));
      break;
    case TCPIP_TCP_SIGNAL_RX_RST:
      sd->state = TCP_SERVER_STATE_ABORTING;
      _l ("(TCP-SERVER)(sk=%d)(si=%d) Connection Abort.\n", (int) hTCP, GET_SERVER_INDEX_FROM_POINTER (sd));
      break;
    case TCPIP_TCP_SIGNAL_KEEP_ALIVE_TMO:
      _l ("(TCP-SERVER)(sk=%d)(si=%d) Signal keep alive time out. Do nothing!\n", (int) hTCP, GET_SERVER_INDEX_FROM_POINTER (sd));
      break;
      // Interface related signals
    case TCPIP_TCP_SIGNAL_IF_DOWN:
      _l ("(TCP-SERVER)(sk=%d)(si=%d) Signal if down. Do nothing!\n", (int) hTCP, GET_SERVER_INDEX_FROM_POINTER (sd));
      break;
    case TCPIP_TCP_SIGNAL_IF_CHANGE:
      _l ("(TCP-SERVER)(sk=%d)(si=%d) Signal if change. Do nothing!\n", (int) hTCP, GET_SERVER_INDEX_FROM_POINTER (sd));
      break;
    }
}

static void tx_task (int32_t serverIndex)
{
  uint16_t capacity;
  t_LIST_NODE *nodeAux;

  switch (serverData[serverIndex].out.state)
    {
    case TX_STATE_IDLE:
      if (serverData[serverIndex].out.queueData)
        {
          serverData[serverIndex].out.dataIndex = 0;
          serverData[serverIndex].out.state = TX_STATE_SENDING;
        }
      break;
    case TX_STATE_SENDING:
      capacity = TCPIP_TCP_PutIsReady (serverData[serverIndex].hSocket);
      if (capacity > 0)
        {
          if (capacity < (serverData[serverIndex].out.queueData->dataLength - serverData[serverIndex].out.dataIndex))
            {
              TCPIP_TCP_ArrayPut (serverData[serverIndex].hSocket, serverData[serverIndex].out.queueData->data + serverData[serverIndex].out.dataIndex, capacity);
              serverData[serverIndex].out.dataIndex += capacity;
            }
          else
            {
              TCPIP_TCP_ArrayPut (serverData[serverIndex].hSocket, serverData[serverIndex].out.queueData->data + serverData[serverIndex].out.dataIndex, serverData[serverIndex].out.queueData->dataLength - serverData[serverIndex].out.dataIndex);
              nodeAux = list_pull_node (&serverData[serverIndex].out.queueData, 0);
              list_delete_node (&nodeAux);
              emit_event (serverIndex, SERVER_EVENT_MESSAGE_SENT);
              serverData[serverIndex].out.state = TX_STATE_IDLE;
            }
        }
      break;
    case TX_STATE_ERROR:
    default:
      nodeAux = list_pull_node (&serverData[serverIndex].out.queueData, 0);
      list_delete_node (&nodeAux);
      serverData[serverIndex].out.state = TX_STATE_IDLE;
      break;
    }
}

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */
void tcp_server_init (void)
{
  int i;

  for (i = 0; i < TCP_SERVER_MAX_SOCKETS; i++)
    {
      memset (&serverData[i], 0, sizeof (t_TCP_SERVER_DATA));
      serverData[i].out.state = TX_STATE_IDLE;
      SET_TCP_SERVER_STATE (i, TCP_SERVER_STATE_INIT);
    }
}

void tcp_server_task (void)
{
  int i;

  for (i = 0; i < TCP_SERVER_MAX_SOCKETS; i++)
    {
      switch (serverData[i].state)
        {
        case TCP_SERVER_STATE_INIT:
          _l ("(TCP-SERVER)(%d) Init...\n", i);
          if (serverData[i].inUse)
            {
              SET_TCP_SERVER_STATE (i, TCP_SERVER_STATE_OPEN_SOCKET);
            }
          else
            {
              _l ("(TCP-SERVER)(%d) Available.\n", i);
              SET_TCP_SERVER_STATE (i, TCP_SERVER_STATE_DISABLE);
            }
          break;
        case TCP_SERVER_STATE_OPEN_SOCKET:
          serverData[i].hSocket = TCPIP_TCP_ServerOpen (IP_ADDRESS_TYPE_IPV4, serverData[i].listenPort, NULL);
          if (serverData[i].hSocket == INVALID_SOCKET)
            {
              _l ("(TCP-SERVER)(%d) Couldn't open server socket\n", i);
            }
          else
            {
              TCPIP_TCP_SignalHandlerRegister (serverData[i].hSocket, (TCPIP_TCP_SIGNAL_TYPE) 0x3F0F/*0x3F0F ALL-EVENTS*/, tcp_event_callback, &serverData[i]);
              _l ("(TCP-SERVER)(%d) Listen on port %d. Socket=%d\n", i, serverData[i].listenPort, serverData[i].hSocket);
              SET_TCP_SERVER_STATE (i, TCP_SERVER_STATE_WAIT_FOR_CONNECTION);
            }
          break;
        case TCP_SERVER_STATE_WAIT_FOR_CONNECTION:
          //...
          break;
        case TCP_SERVER_STATE_CONNECTION_STABLISHED:
          if (serverData[i].rxP)
            {
              serverData[i].rxP (i);
            }
          else
            {
              //No RX proccesor set. Discard RX buffer
              TCPIP_TCP_Discard (serverData[i].hSocket);
            }
          tx_task (i);
          break;
        case TCP_SERVER_STATE_DISCONNECTING:
          if (serverData[i].rxP)
            {
              if (serverData[i].rxP (i) == 0)
                {
                  TCPIP_TCP_Disconnect (serverData[i].hSocket); // Disconnect the connection.
                  SET_TCP_SERVER_STATE (i, TCP_SERVER_STATE_WAIT_DISCONNECT);
                }
            }
          else
            {
              TCPIP_TCP_Disconnect (serverData[i].hSocket); // Disconnect the connection.
              SET_TCP_SERVER_STATE (i, TCP_SERVER_STATE_WAIT_DISCONNECT);
            }
          break;
        case TCP_SERVER_STATE_WAIT_DISCONNECT:
          if (TCPIP_TCP_WasDisconnected (serverData[i].hSocket))
            {
              emit_event (i, SERVER_EVENT_DISCONNECTED);
              _l ("(TCP-SERVER)(%d) Socket disconected. Wait for connection\n", i);
              SET_TCP_SERVER_STATE (i, TCP_SERVER_STATE_WAIT_FOR_CONNECTION);
            }
          break;
        case TCP_SERVER_STATE_ABORTING:
          list_delete (&serverData[i].out.queueData);
          TCPIP_TCP_Abort (serverData[i].hSocket, false);
          SET_TCP_SERVER_STATE (i, TCP_SERVER_STATE_WAIT_ABORT);
          break;
        case TCP_SERVER_STATE_WAIT_ABORT:
          if (TCPIP_TCP_WasDisconnected (serverData[i].hSocket))
            {
              emit_event (i, SERVER_EVENT_DISCONNECTED);
              _l ("(TCP-SERVER)(%d) Socket aborted. Wait for connection\n", i);
              SET_TCP_SERVER_STATE (i, TCP_SERVER_STATE_WAIT_FOR_CONNECTION);
            }
          break;
        case TCP_SERVER_STATE_CLOSE:
          list_delete (&serverData[i].out.queueData);
          TCPIP_TCP_Close (serverData[i].hSocket);
          SET_TCP_SERVER_STATE (i, TCP_SERVER_STATE_WAIT_CLOSE);
          break;
        case TCP_SERVER_STATE_WAIT_CLOSE:
          emit_event (i, SERVER_EVENT_CLOSE);
          serverData[i].hSocket = INVALID_SOCKET;
          _l ("(TCP-SERVER)(%d) Connection closed\n", i);
          SET_TCP_SERVER_STATE (i, TCP_SERVER_STATE_INIT);
          break;
          //
        case TCP_SERVER_STATE_DISABLE:
          break;
        }
    }
}

void tcp_server_reset (void)
{
  int i;

  for (i = 0; i < TCP_SERVER_MAX_SOCKETS; i++)
    {
      if (serverData[i].hSocket != INVALID_SOCKET)
        {
          if (serverData[i].out.queueData)
            {
              list_delete (&serverData[i].out.queueData);
              serverData[i].out.state = TX_STATE_IDLE;
            }
          TCPIP_TCP_Close (serverData[i].hSocket);
          serverData[i].hSocket = INVALID_SOCKET;
        }
      _l ("(TCP-SERVER)(%d) Server RESET!\n", i);
      SET_TCP_SERVER_STATE (i, TCP_SERVER_STATE_INIT);
    }
}

int32_t tcp_server_config (TCP_PORT listenPort, TCP_SERVER_RX_PROCESSOR rxP, TCP_SERVER_EVENT_CALLBACK cb)
{
  int i = 0;
  while (i < TCP_SERVER_MAX_SOCKETS)
    {
      if (!serverData[i].inUse)
        {
          serverData[i].listenPort = listenPort;
          serverData[i].rxP = rxP;
          serverData[i].cb = cb;
          serverData[i].inUse = true;
          serverData[i].state = TCP_SERVER_STATE_INIT;
          return i;
        }
      i++;
    }
  return TCP_SERVER_OP_ERROR;
}

uint16_t tcp_server_get_rx_length (int32_t serverIndex)
{
  return TCPIP_TCP_GetIsReady (serverData[serverIndex].hSocket);
}

uint16_t tcp_server_pull_rx_data (int32_t serverIndex, uint8_t* buffer, uint16_t count)
{
  return TCPIP_TCP_ArrayGet (serverData[serverIndex].hSocket, buffer, count);
}

int32_t tcp_server_send (int32_t serverIndex, uint8_t *data, uint16_t dataLength)
{
  t_LIST_NODE *nodeAux;

  if (serverData[serverIndex].state == TCP_SERVER_STATE_CONNECTION_STABLISHED)
    {
      nodeAux = list_new_node (data, dataLength);
      if (nodeAux)
        {
          list_push_node (&serverData[serverIndex].out.queueData, nodeAux, 0);
          _l ("(TCP-SERVER) (%d) Send message queued\n", serverIndex);
        }
      else
        {
          _l ("(TCP-SERVER) (%d) Send ERROR. No list node create!)\n", serverIndex);
        }
    }
  else
    {
      _l ("(TCP-SERVER) (%d) Send NO connection stablished. Discard!)\n", serverIndex);
    }

  return 0;
}

void tcp_server_force_disconnect (int32_t serverIndex)
{
  _l ("(TCP-SERVER) (%d) Forced disconnection...\n", serverIndex);
  serverData[serverIndex].state = TCP_SERVER_STATE_DISCONNECTING;
}

/* ************************************************************** End of File */