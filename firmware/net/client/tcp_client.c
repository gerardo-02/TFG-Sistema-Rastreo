#include <stdint.h>

#include "peripheral/coretimer/plib_coretimer.h"
#include "net_pres/pres/net_pres_socketapi.h"

#include "log.h"
#include "../net.h"
#include "lists.h"

#include "tcp_client.h"


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */
#define TIME_RESOLUTION_MS              100
#define CLIENT_CONNECTION_TIMEOUT_MS    20000
#define CLIENT_ERROR_DELAY_MS           5000
#define CLIENT_GENERAL_TIMEOUT_MS       5000
#define TIMEOUT_RESOLUTION_MS           100
#define TIMEOUT_RECEIVING_MS            100
#define TIMEOUT_SENDING_MS              10000

#define RX_BUFFER                       4096

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Data types & Definitions                                          */
/* ************************************************************************** */

/* ************************************************************************** */
typedef enum
{
  STREAM_STATE_IDLE = 0,
  STREAM_STATE_RECEIVING,
  STREAM_STATE_SENDING,
  STREAM_STATE_ERROR,
} t_STREAMS_STATES;

typedef struct
{
  t_STREAMS_STATES state;
  t_LIST_NODE *datNode;
  uint16_t curDataOffset;
} t_STREAM_TX;

typedef struct
{
  uint32_t timeout;
  char hostName_s[TCP_NET_MAX_HOSTNAME_STRING_SIZE];
  TCPIP_DNS_RESULT dnsResult;
  IPV4_ADDR address;
  TCP_PORT port;
  NET_PRES_SKT_HANDLE_T hSocket;
  bool tls;
  t_TCP_CLIENT_STATES state;
  t_STREAM_TX stx;
  TCP_CLIENT_CALLBACK cb;
  void *ctx;
}
t_TCP_CLIENT_DATA;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Global data & variables                                           */
/* ************************************************************************** */
/* ************************************************************************** */
static t_TCP_CLIENT_DATA clientData[TCP_CLIENT_MAX_SOCKETS];

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Callback Functions                                                */
/* ************************************************************************** */
/* ************************************************************************** */


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */
#define SET_TCP_CLIENT_STATE(i,newState)    clientData[i].state=newState
#define GET_CORE_TIMESTAMP(coreTimeStamp)   (coreTimeStamp = CORETIMER_CounterGet ())
#define ELAPSED_S(coreTimeStamp)  ((CORETIMER_CounterGet () - coreTimeStamp) / CORE_TIMER_FREQUENCY)
#define ELAPSED_MS(coreTimeStamp)  ((CORETIMER_CounterGet () - coreTimeStamp) / (CORE_TIMER_FREQUENCY/1000U))

static void call_back (int32_t sid, t_CLIENT_EVENTS e)
{
  t_TCP_CLIENT_CONTEXT ctx;

  if (clientData[sid].cb)
    {
      ctx.sid = sid;
      ctx.event = e;
      ctx.dataLength = NET_PRES_SocketReadIsReady (clientData[sid].hSocket);
      clientData[sid].cb (&ctx, clientData[sid].ctx);
    }
}

static void tx_task (int32_t sid)
{
  uint16_t capacity;
  t_LIST_NODE *nodeAux;

  switch (clientData[sid].stx.state)
    {
    case STREAM_STATE_IDLE:
      if (clientData[sid].stx.datNode)
        {
          clientData[sid].stx.curDataOffset = 0;
          clientData[sid].stx.state = STREAM_STATE_SENDING;
        }
      break;
    case STREAM_STATE_SENDING:
      capacity = NET_PRES_SocketWriteIsReady (clientData[sid].hSocket, clientData[sid].stx.datNode->dataLength - clientData[sid].stx.curDataOffset, 1);
      if (capacity > 0)
        {
          if (capacity < (clientData[sid].stx.datNode->dataLength - clientData[sid].stx.curDataOffset))
            {
              NET_PRES_SocketWrite (clientData[sid].hSocket, clientData[sid].stx.datNode->data + clientData[sid].stx.curDataOffset, capacity);
              clientData[sid].stx.curDataOffset += capacity;
            }
          else
            {
              NET_PRES_SocketWrite (clientData[sid].hSocket, clientData[sid].stx.datNode->data + clientData[sid].stx.curDataOffset, clientData[sid].stx.datNode->dataLength - clientData[sid].stx.curDataOffset);
              nodeAux = list_pull_node (&clientData[sid].stx.datNode, 0);
              list_delete_node (&nodeAux);
              call_back (sid, CLIENT_EVENT_TX);
              clientData[sid].stx.state = STREAM_STATE_IDLE;
            }
        }
      break;
    case STREAM_STATE_ERROR:
    default:
      _l ("(TCP-CLIENT)(%d) Tx state error!\n", sid);
      nodeAux = list_pull_node (&clientData[sid].stx.datNode, 0);
      list_delete_node (&nodeAux);
      call_back (sid, CLIENT_EVENT_ERROR);
      clientData[sid].stx.state = STREAM_STATE_IDLE;
      break;
    }
}

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */
void tcp_client_init (void)
{
  int32_t i;

  for (i = 0; i < TCP_CLIENT_MAX_SOCKETS; i++)
    {
      memset (&clientData[i], 0, sizeof (t_TCP_CLIENT_DATA));
      SET_TCP_CLIENT_STATE (i, TCP_CLIENT_STATE_IDLE);
    }
  _l ("(TCP-CLIENT) Initialized\n");
}

void tcp_client_task (void)
{
  static char ipStr[TCP_NET_MAX_IP_STR_LENGTH];
  int sid;

  for (sid = 0; sid < TCP_CLIENT_MAX_SOCKETS; sid++)
    {
      switch (clientData[sid].state)
        {
        case TCP_CLIENT_STATE_IDLE:
          //waiting...
          break;
          // DNS resolution
        case TCP_CLIENT_STATE_DNS_RESOLVE_HOST_ADDRESS:
          clientData[sid].dnsResult = TCPIP_DNS_Resolve (clientData[sid].hostName_s, TCPIP_DNS_TYPE_A);
          if (clientData[sid].dnsResult < 0)
            {
              SET_TCP_CLIENT_STATE (sid, TCP_CLIENT_STATE_DNS_ERROR);
            }
          else if (clientData[sid].dnsResult == TCPIP_DNS_RES_NAME_IS_IPADDRESS)
            {
              TCPIP_Helper_StringToIPAddress (clientData[sid].hostName_s, &clientData[sid].address);
              SET_TCP_CLIENT_STATE (sid, TCP_CLIENT_STATE_CONNECTION_INIT);
            }
          else
            {
              SET_TCP_CLIENT_STATE (sid, TCP_CLIENT_STATE_DNS_WAIT);
            }
          break;
        case TCP_CLIENT_STATE_DNS_WAIT:
          clientData[sid].dnsResult = TCPIP_DNS_IsResolved (clientData[sid].hostName_s, (IP_MULTI_ADDRESS*) & clientData[sid].address, IP_ADDRESS_TYPE_IPV4);
          switch (clientData[sid].dnsResult)
            {
            case TCPIP_DNS_RES_OK:
              TCPIP_Helper_IPAddressToString (&clientData[sid].address, ipStr, sizeof (ipStr));
              _l ("(TCP-CLIENT)(%d) DNS: %s -> %s\n", sid, clientData[sid].hostName_s, ipStr);
              SET_TCP_CLIENT_STATE (sid, TCP_CLIENT_STATE_CONNECTION_INIT);
              break;
            case TCPIP_DNS_RES_PENDING:
              break;
            case TCPIP_DNS_RES_SERVER_TMO:
              SET_TCP_CLIENT_STATE (sid, TCP_CLIENT_STATE_DNS_TIMEOUT);
              break;
            default:
              SET_TCP_CLIENT_STATE (sid, TCP_CLIENT_STATE_DNS_ERROR);
              break;
            }
          break;
        case TCP_CLIENT_STATE_DNS_TIMEOUT:
          _l ("(TCP-CLIENT)(%d) DNS response timeout!\n", sid);
          call_back (sid, CLIENT_EVENT_TIMEOUT);
          GET_CORE_TIMESTAMP (clientData[sid].timeout);
          SET_TCP_CLIENT_STATE (sid, TCP_CLIENT_STATE_WAIT_FOR_RETRY);
          break;
        case TCP_CLIENT_STATE_DNS_ERROR:
          _l ("(TCP-CLIENT)(%d) Resolve HOST error in DNS!\n", sid);
          call_back (sid, CLIENT_EVENT_ERROR);
          GET_CORE_TIMESTAMP (clientData[sid].timeout);
          SET_TCP_CLIENT_STATE (sid, TCP_CLIENT_STATE_WAIT_FOR_RETRY);
          break;
          // Init connection
        case TCP_CLIENT_STATE_CONNECTION_INIT:
          clientData[sid].hSocket = NET_PRES_SocketOpen (0, NET_PRES_SKT_UNENCRYPTED_STREAM_CLIENT, IP_ADDRESS_TYPE_IPV4, clientData[sid].port, (NET_PRES_ADDRESS*) & clientData[sid].address, NULL);
          if (clientData[sid].hSocket == INVALID_SOCKET)
            {
              _l ("(TCP-CLIENT)(%d,%d) Can NOT open connection!\n", sid, (int) clientData[sid].hSocket);
              call_back (sid, CLIENT_EVENT_ERROR);
              GET_CORE_TIMESTAMP (clientData[sid].timeout);
              SET_TCP_CLIENT_STATE (sid, TCP_CLIENT_STATE_WAIT_FOR_RETRY);
            }
          else
            {
              NET_PRES_SocketWasReset (clientData[sid].hSocket); //Init wasReset function
              //              NET_PRES_SocketWasDisconnected (clientData[sid].hSocket); //Init wasDisconnet function
              TCPIP_Helper_IPAddressToString (&clientData[sid].address, ipStr, sizeof (ipStr));
              _l ("(TCP-CLIENT)(%d,%d) Connecting to %s:%d\n", sid, (int) clientData[sid].hSocket, ipStr, (int) clientData[sid].port);
              GET_CORE_TIMESTAMP (clientData[sid].timeout);
              SET_TCP_CLIENT_STATE (sid, TCP_CLIENT_STATE_CONNECTION_WAITING_FOR);
            }
          break;
        case TCP_CLIENT_STATE_CONNECTION_WAITING_FOR:
          if (NET_PRES_SocketIsConnected (clientData[sid].hSocket))
            {
              _l ("(TCP-CLIENT)(%d,%d) Socket connected!\n", sid, (int) clientData[sid].hSocket);
              clientData[sid].stx.state = STREAM_STATE_IDLE;
              if (clientData[sid].tls)
                {
                  if (NET_PRES_SocketEncryptSocket (clientData[sid].hSocket))
                    {
                      _l ("(TCP-CLIENT)(%d,%d) Starting SSL Negotiation...\n", sid, (int) clientData[sid].hSocket);
                      GET_CORE_TIMESTAMP (clientData[sid].timeout);
                      SET_TCP_CLIENT_STATE (sid, TCP_CLIENT_STATE_CONNECTION_SSL_NEGOTIATION_WAIT);
                    }
                  else
                    {
                      _l ("(TCP-CLIENT)(%d,%d) Starting SSL Negotiation failed.\n", sid, (int) clientData[sid].hSocket);
                      SET_TCP_CLIENT_STATE (sid, TCP_CLIENT_STATE_CONNECTION_ERROR);
                    }
                }
              else
                {
                  SET_TCP_CLIENT_STATE (sid, TCP_CLIENT_STATE_CONNECTION_STABLISHED);
                  call_back (sid, CLIENT_EVENT_CONNECTED);
                }
            }
          else
            {
              if (ELAPSED_MS (clientData[sid].timeout) > CLIENT_CONNECTION_TIMEOUT_MS)
                {
                  SET_TCP_CLIENT_STATE (sid, TCP_CLIENT_STATE_CONNECTION_TIMEOUT);
                }
            }
          break;
        case TCP_CLIENT_STATE_CONNECTION_SSL_NEGOTIATION_WAIT:
          if (NET_PRES_SocketIsNegotiatingEncryption (clientData[sid].hSocket))
            {
              if (ELAPSED_MS (clientData[sid].timeout) > CLIENT_CONNECTION_TIMEOUT_MS)
                {
                  SET_TCP_CLIENT_STATE (sid, TCP_CLIENT_STATE_CONNECTION_TIMEOUT);
                }
            }
          else
            {
              if (NET_PRES_SocketIsSecure (clientData[sid].hSocket))
                {
                  SET_TCP_CLIENT_STATE (sid, TCP_CLIENT_STATE_CONNECTION_STABLISHED);
                  call_back (sid, CLIENT_EVENT_CONNECTED);
                }
              else
                {
                  _l ("(TCP-CLIENT)(%d,%d) SSL Negotiating Failed.\n", sid, (int) clientData[sid].hSocket);
                  SET_TCP_CLIENT_STATE (sid, TCP_CLIENT_STATE_CONNECTION_ERROR);
                }
            }
          break;
        case TCP_CLIENT_STATE_CONNECTION_STABLISHED:
          if (NET_PRES_SocketWasReset (clientData[sid].hSocket) || NET_PRES_SocketWasDisconnected (clientData[sid].hSocket))
            {
              NET_PRES_SocketDisconnect (clientData[sid].hSocket); // Close the socket connection.
              _l ("(TCP-CLIENT)(%d,%d) Disconnection by server\n", sid, (int) clientData[sid].hSocket);
              call_back (sid, CLIENT_EVENT_DISCONNECTED);
              SET_TCP_CLIENT_STATE (sid, TCP_CLIENT_STATE_CONNECTION_CLOSE);
            }
          else
            {
              tx_task (sid);
              if (NET_PRES_SocketReadIsReady (clientData[sid].hSocket))
                {
                  call_back (sid, CLIENT_EVENT_RX);
                }
            }
          break;
        case TCP_CLIENT_STATE_CONNECTION_TIMEOUT:
          _l ("(TCP-CLIENT)(%d,%d) Connection timeout, %s:%d\n", sid, (int) clientData[sid].hSocket, ipStr, (int) clientData[sid].port);
          call_back (sid, CLIENT_EVENT_TIMEOUT);
          SET_TCP_CLIENT_STATE (sid, TCP_CLIENT_STATE_CONNECTION_CLOSE);
          break;
        case TCP_CLIENT_STATE_CONNECTION_ERROR:
          _l ("(TCP-CLIENT)(%d,%d) Connection error, %s:%d\n", sid, (int) clientData[sid].hSocket, ipStr, (int) clientData[sid].port);
          call_back (sid, CLIENT_EVENT_ERROR);
          SET_TCP_CLIENT_STATE (sid, TCP_CLIENT_STATE_CONNECTION_CLOSE);
          break;
        case TCP_CLIENT_STATE_CONNECTION_CLOSE:
          _l ("(TCP-CLIENT)(%d,%d) Connection closed\n", sid, (int) clientData[sid].hSocket);
          NET_PRES_SocketClose (clientData[sid].hSocket); // Clean up, close the socket connection.
          clientData[sid].hSocket = INVALID_SOCKET;
          list_delete (&clientData[sid].stx.datNode);
          GET_CORE_TIMESTAMP (clientData[sid].timeout);
          SET_TCP_CLIENT_STATE (sid, TCP_CLIENT_STATE_WAIT_FOR_RETRY);
          break;
          //
        case TCP_CLIENT_STATE_WAIT_FOR_RETRY:
          if (ELAPSED_MS (clientData[sid].timeout) > CLIENT_ERROR_DELAY_MS)
            {
              SET_TCP_CLIENT_STATE (sid, TCP_CLIENT_STATE_IDLE);
            }
          break;
        }
    }
}

void tcp_client_reset (void)
{
  int i;

  for (i = 0; i < TCP_CLIENT_MAX_SOCKETS; i++)
    {
      if (clientData[i].hSocket != INVALID_SOCKET)
        {
          if (clientData[i].stx.datNode)
            {
              list_delete (&clientData[i].stx.datNode);
              clientData[i].stx.state = STREAM_STATE_IDLE;
            }
          NET_PRES_SocketClose (clientData[i].hSocket);
          clientData[i].hSocket = INVALID_SOCKET;
        }
      _l ("(TCP-CLIENT)(%d) Client RESET!\n", i);
      SET_TCP_CLIENT_STATE (i, TCP_CLIENT_STATE_IDLE);
    }
}

t_TCP_CLIENT_STATES tcp_client_get_state (int32_t sid)
{
  return clientData[sid].state;
}

int32_t tcp_client_open_host (char *host, TCP_PORT port, bool tls, TCP_CLIENT_CALLBACK cb, void *ctx)
{
  int32_t i = 0;

  while ((clientData[i].state != TCP_CLIENT_STATE_IDLE) && (i < TCP_CLIENT_MAX_SOCKETS)) i++;
  if (i < TCP_CLIENT_MAX_SOCKETS)
    {
      strcpy (clientData[i].hostName_s, host);
      clientData[i].port = port;
      clientData[i].tls = tls;
      clientData[i].cb = cb;
      clientData[i].ctx = ctx;
      SET_TCP_CLIENT_STATE (i, TCP_CLIENT_STATE_DNS_RESOLVE_HOST_ADDRESS);
    }
  else
    {
      _l ("(CLIENT OPEN) ERROR. No sockets availables!)\n");
      return TCP_CLIENT_OP_BUSY;
    }

  return TCP_CLIENT_OP_SUCCESS;
}

int32_t tcp_client_open_ip (IPV4_ADDR ip, TCP_PORT port, bool tls, TCP_CLIENT_CALLBACK cb, void *ctx)
{
  int32_t i = 0;

  while ((clientData[i].state != TCP_CLIENT_STATE_IDLE) && (i < TCP_CLIENT_MAX_SOCKETS)) i++;
  if (i < TCP_CLIENT_MAX_SOCKETS)
    {
      clientData[i].address = ip;
      clientData[i].port = port;
      clientData[i].tls = tls;
      clientData[i].cb = cb;
      clientData[i].ctx = ctx;
      SET_TCP_CLIENT_STATE (i, TCP_CLIENT_STATE_CONNECTION_INIT);
    }
  else
    {
      _l ("(CLIENT OPEN) ERROR. No sockets availables!)\n");
      return TCP_CLIENT_OP_BUSY;
    }

  return TCP_CLIENT_OP_SUCCESS;
}

int32_t tcp_client_send (int32_t sid, uint8_t *data, uint16_t dataLength)
{
  t_LIST_NODE *nodeAux;

  if (sid < TCP_CLIENT_MAX_SOCKETS)
    {
      if (clientData[sid].state == TCP_CLIENT_STATE_CONNECTION_STABLISHED)
        {
          if (data && (dataLength > 0))
            {
              nodeAux = list_new_node (data, dataLength);
              if (nodeAux)
                {
                  list_push_node (&clientData[sid].stx.datNode, nodeAux, 0);
                }
              else
                {
                  _l ("(CLIENT SEND) (%d) ERROR. No list node create!)\n", sid);
                  return TCP_CLIENT_OP_ERROR;
                }
            }
        }
      else
        {
          _l ("(CLIENT SEND) (%d) ERROR. Client no connected!)\n", sid);
          return TCP_CLIENT_OP_WORNG_ID_CLIENT;
        }
    }
  else
    {
      _l ("(CLIENT SEND) ERROR. (%d), worng socket id!)\n", sid);
      return TCP_CLIENT_OP_WORNG_PARAMETERS;
    }

  return TCP_CLIENT_OP_SUCCESS;
}

uint16_t tcp_client_peek_rx (int32_t sid, uint8_t *buffer, uint16_t requestLength)
{
  uint16_t nBytes = 0;

  if (clientData[sid].state == TCP_CLIENT_STATE_CONNECTION_STABLISHED)
    {
      nBytes = NET_PRES_SocketPeek (clientData[sid].hSocket, buffer, requestLength);
    }

  return nBytes;
}

uint16_t tcp_client_get_rx (int32_t sid, uint8_t *buffer, uint16_t requestLength)
{
  uint16_t nBytes = 0;

  if (clientData[sid].state == TCP_CLIENT_STATE_CONNECTION_STABLISHED)
    {
      nBytes = NET_PRES_SocketRead (clientData[sid].hSocket, buffer, requestLength);
    }

  return nBytes;
}

int32_t tcp_client_close (int32_t sid)
{
  if (sid < TCP_CLIENT_MAX_SOCKETS)
    {
      if ((clientData[sid].hSocket != INVALID_SOCKET))
        {
          SET_TCP_CLIENT_STATE (sid, TCP_CLIENT_STATE_CONNECTION_CLOSE);
        }
      else
        {
          _l ("(CLIENT CLOSE) (%d) Client not connected!\n", sid);
          return TCP_CLIENT_OP_WORNG_ID_CLIENT;
        }
    }
  else
    {
      _l ("(CLIENT CLOSE) (%d), Worng client id!)\n", sid);
      return TCP_CLIENT_OP_WORNG_PARAMETERS;
    }

  return TCP_CLIENT_OP_SUCCESS;
}

/* ************************************************************** End of File */
