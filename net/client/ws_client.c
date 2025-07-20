#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "peripheral/coretimer/plib_coretimer.h"
#include "tcpip/tcpip.h"
#include "tcpip/tcp.h"
#include "tcpip/tcpip_helpers.h"
#include "wolfcrypt/sha.h"
#include "ssl.h"

#include "log.h"
#include "drv/pmz_time.h"
#include "drv/drv_dmem.h"
#include "tcp_client.h"
#include "../http_request.h"
#include "../http_parser.h"

#include "ws_client.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */

#define RETRY_TIME_S                30

#define WS_KEY_LENGTH               256
#define WS_RECEIVE_BUFFER           2048
#define WS_STR_PING_LENGTH          128     

#define HTTP_MAX_RESPONSE_HEADERS   10
#define FRAME_CHUNK_LENGTH          512 //1024

#define CLIENT_IS_SSL               0x01
#define CLIENT_CONNECTING           0x02
#define CLIENT_SHOULD_CLOSE         0x04
#define CLIENT_SENT_CLOSE_FRAME     0x08

#define REQUEST_HAS_CONNECTION      0x10
#define REQUEST_HAS_UPGRADE         0x20
#define REQUEST_VALID_STATUS        0x40
#define REQUEST_VALID_ACCEPT        0x80

const char *UUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Data types & Definitions                                          */
/* ************************************************************************** */

/* ************************************************************************** */
typedef struct _ws_frame
{
  unsigned int fin;
  unsigned int opcode;
  unsigned int mask_offset;
  unsigned int payload_offset;
  unsigned int rawdata_idx;
  unsigned int rawdata_sz;
  unsigned long long payload_len;
  char *rawdata;
  struct _ws_frame *next_frame;
  struct _ws_frame *prev_frame;
  unsigned char mask[4];
} t_WS_FRAME;

typedef struct _wsclient
{
  t_URL_DATA *urlData;
  uint16_t nPort;
  int32_t curSocketId;
  t_WS_STATES state;
  int flags;
  //  int (*onopen)(struct _wsclient *);
  //  int (*onclose)(struct _wsclient *);
  //  int (*onerror)(struct _wsclient *, t_WS_ERROR *err);
  //  int (*onmessage)(struct _wsclient *, t_WS_MESSAGE *msg);
  WS_ON_OPEN_CALLBACK onOpen;
  WS_ON_CLOSE_CALLBACK onClose;
  WS_ON_MESSAGE_CALLBACK onMessage;
  WS_ON_ERROR_CALLBACK onError;
  t_WS_FRAME *current_frame;
  char webSocketKey[WS_KEY_LENGTH];
  char recv_buf[WS_RECEIVE_BUFFER];
  uint32_t ts;
  uint32_t activityTimeout; //segundos
  char pingStr[WS_STR_PING_LENGTH];
} t_WS_CLIENT_DATA;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Global data & variables                                           */
/* ************************************************************************** */
/* ************************************************************************** */
static t_WS_CLIENT_DATA wscData[WS_MAX_CLIENTS];

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Callback Functions                                                */
/* ************************************************************************** */
/* ************************************************************************** */
#define SET_WS_CLIENT_STATE(i,newState)     wscData[i].state=newState
#define GET_CORE_TIMESTAMP(coreTimeStamp)   (coreTimeStamp = CORETIMER_CounterGet ())
#define ELAPSED_S(coreTimeStamp)  ((CORETIMER_CounterGet () - coreTimeStamp) / CORE_TIMER_FREQUENCY)
#define ELAPSED_MS(coreTimeStamp)  ((CORETIMER_CounterGet () - coreTimeStamp) / (CORE_TIMER_FREQUENCY/1000U))

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Local Functions                                                   */
/* ************************************************************************** */

/* ************************************************************************** */
static void sock_cb (t_TCP_CLIENT_CONTEXT * tcpCtx, void *appCtx) //TCP socket callback
{
  t_WS_CLIENT_DATA *c = (t_WS_CLIENT_DATA*) appCtx;

  switch (tcpCtx->event)
    {
    case CLIENT_EVENT_CONNECTED:
      _l ("(WS-CLIENT) Connected!, performing request...\n");
      c->curSocketId = tcpCtx->sid;
      c->state = WS_STATE_INIT_HANDSHAKE;
      break;
    case CLIENT_EVENT_TX:
      _d ("(WS-CLIENT) Sent! t=%lu\n", *pmz_time_get ());
      break;
    case CLIENT_EVENT_RX:
      //Nothing! It will be treated individually in each state: WS_STATE_WAIT_HANDSHAKE_RESPONSE and WS_STATE_CONNECTION_ESTABLISHED
      break;
    case CLIENT_EVENT_DISCONNECTED:
      if (tcpCtx->dataLength)
        {
          _l ("(WS-CLIENT) Disconnected!, but there is still data in the buffer.\n");
        }
      c->state = WS_STATE_CLOSE;
      break;
    case CLIENT_EVENT_TIMEOUT:
    case CLIENT_EVENT_ERROR:
    default:
      c->state = WS_STATE_ERROR;
      break;
    }
}
////////////////////////////////////////////////////////////////////////////////

static int complete_frame (t_WS_HANDLER wsh, t_WS_FRAME *frame)
{
  int payload_len_short, i;
  unsigned long long payload_len = 0;
  if (frame->rawdata_idx < 2)
    {
      return 0;
    }
  frame->fin = (*(frame->rawdata) & 0x80) == 0x80 ? 1 : 0;
  frame->opcode = *(frame->rawdata) & 0x0f;
  frame->payload_offset = 2;
  if ((*(frame->rawdata + 1) & 0x80) != 0x0)
    {
      _l ("(WS-CLIENT) WS_COMPLETE_FRAME_MASKED_ERR\n");
      wscData[wsh].flags |= CLIENT_SHOULD_CLOSE;
      return -1;
    }
  payload_len_short = *(frame->rawdata + 1) & 0x7f;
  switch (payload_len_short)
    {
    case 126:
      if (frame->rawdata_idx < 4)
        return 0;
      for (i = 0; i < 2; i++)
        memcpy ((void *) &payload_len + i, frame->rawdata + 3 - i, 1);
      frame->payload_offset += 2;
      frame->payload_len = payload_len;
      break;
    case 127:
      if (frame->rawdata_idx < 10)
        return 0;
      for (i = 0; i < 8; i++)
        memcpy ((void *) &payload_len + i, frame->rawdata + 9 - i, 1);
      frame->payload_offset += 8;
      frame->payload_len = payload_len;
      break;
    default:
      frame->payload_len = payload_len_short;
      break;
    }
  if (frame->rawdata_idx < frame->payload_offset + frame->payload_len)
    return 0;

  return 1;
}

static int handle_control_frame (t_WS_HANDLER wsh, t_WS_FRAME *ctl_frame)
{
  t_WS_FRAME *ptr = NULL;
  int i, n = 0;
  char mask[4];
  int mask_int;

  srand (time (NULL));
  mask_int = rand ();
  memcpy (mask, &mask_int, 4);
  switch (ctl_frame->opcode)
    {
    case 0x8:
      //close frame
      if ((wscData[wsh].flags & CLIENT_SENT_CLOSE_FRAME) == 0)
        {
          //server request close.  Send close frame as acknowledgement.
          for (i = 0; i < ctl_frame->payload_len; i++)
            *(ctl_frame->rawdata + ctl_frame->payload_offset + i) ^= (mask[i % 4] & 0xff); //mask payload
          *(ctl_frame->rawdata + 1) |= 0x80; //turn mask bit on
          i = 0;
          while (i < ctl_frame->payload_offset + ctl_frame->payload_len && n >= 0)
            {
              tcp_client_send (wscData[wsh].curSocketId, (uint8_t*) (ctl_frame->rawdata + i), ctl_frame->payload_offset + ctl_frame->payload_len - i);
              n = ctl_frame->payload_offset + ctl_frame->payload_len - i; //Hago un acto de fe y pienso que se va a enviar todo en un solo golpe
              i += n;
            }
          if (n < 0)
            {
              _l ("(WS-CLIENT) WS_COMPLETE_FRAME_MASKED_ERR\n");
            }
        }
      wscData[wsh].flags |= CLIENT_SHOULD_CLOSE;
      break;
    default:
      _l ("(WS-CLIENT) Unhandled control frame received.  Opcode: %d\n", ctl_frame->opcode);
      break;
    }
  ptr = ctl_frame->prev_frame; //This very well may be a NULL pointer, but just in case we preserve it.
  dmem_release (ctl_frame->rawdata);
  memset (ctl_frame, 0, sizeof (t_WS_FRAME));
  ctl_frame->prev_frame = ptr;
  ctl_frame->rawdata = (char *) dmem_create (FRAME_CHUNK_LENGTH);
  if (ctl_frame->rawdata)
    {
      memset (ctl_frame->rawdata, 0, FRAME_CHUNK_LENGTH);
    }
  else
    {
      _l ("(WS-CLIENT) Unable to alloc new frame raw data!\n");
      return -1;
    }

  return 0;
}

static void cleanup_frames (t_WS_FRAME *first)
{
  t_WS_FRAME *this = NULL;
  t_WS_FRAME *next = first;
  while (next != NULL)
    {
      this = next;
      next = this->next_frame;
      if (this->rawdata != NULL) dmem_release (this->rawdata);
      dmem_release (this);
    }
}

static int dispatch_message (t_WS_HANDLER wsh, t_WS_FRAME *current)
{
  unsigned long long message_payload_len, message_offset;
  int message_opcode;
  char *message_payload;
  t_WS_FRAME *first = NULL;
  t_WS_MESSAGE *msg = NULL;
  t_WS_ERROR e;

  if (current == NULL)
    {
      _l ("(WS-CLIENT) WS_DISPATCH_MESSAGE_NULL_PTR_ERR\n");
      if (wscData[wsh].onError)
        {
          e.code = 0;
          e.extra_code = 0;
          e.str = NULL;
          wscData[wsh].onError (wsh, &e);
        }
      wscData[wsh].state = WS_STATE_ERROR;
      return -1;
    }
  message_offset = 0;
  message_payload_len = current->payload_len;
  for (; current->prev_frame != NULL; current = current->prev_frame)
    message_payload_len += current->payload_len;
  first = current;
  message_opcode = current->opcode;
  message_payload = (char *) dmem_create (message_payload_len + 1);
  if (message_payload)
    {
      memset (message_payload, 0, message_payload_len + 1);
      for (; current != NULL; current = current->next_frame)
        {
          memcpy (message_payload + message_offset, current->rawdata + current->payload_offset, current->payload_len);
          message_offset += current->payload_len;
        }
      _d ("(WS-CLIENT) Clean frames!\n");
      cleanup_frames (first);
      msg = (t_WS_MESSAGE *) dmem_create (sizeof (t_WS_MESSAGE));
      if (msg)
        {
          memset (msg, 0, sizeof (t_WS_MESSAGE));
          msg->opcode = message_opcode;
          msg->payload_len = message_offset;
          msg->payload = message_payload;
          if (wscData[wsh].onMessage != NULL)
            {
              wscData[wsh].onMessage (wsh, msg);
            }
          else
            {
              _l ("(WS-CLIENT) No onmessage call back registered with\n");
            }
          dmem_release (msg);
        }
      else
        {
          _l ("(WS-CLIENT) Unable to alloc msg!\n");
        }
      dmem_release (message_payload);
    }
  else
    {
      _l ("(WS-CLIENT) Unable to alloc message payload!\n");
      return -1;
    }

  return 0;
}

static int in_data (t_WS_HANDLER wsh, char in)
{
  t_WS_FRAME *current = NULL, *new = NULL;
  char* rAllocAux;

  if (wscData[wsh].current_frame == NULL)
    {
      wscData[wsh].current_frame = (t_WS_FRAME *) dmem_create (sizeof (t_WS_FRAME));
      if (wscData[wsh].current_frame)
        {
          memset (wscData[wsh].current_frame, 0, sizeof (t_WS_FRAME));
          wscData[wsh].current_frame->payload_len = -1;
          wscData[wsh].current_frame->rawdata_sz = FRAME_CHUNK_LENGTH;
          wscData[wsh].current_frame->rawdata = (char *) dmem_create (wscData[wsh].current_frame->rawdata_sz);
          if (wscData[wsh].current_frame->rawdata)
            {
              memset (wscData[wsh].current_frame->rawdata, 0, wscData[wsh].current_frame->rawdata_sz);
            }
          else
            {
              dmem_release (wscData[wsh].current_frame);
              wscData[wsh].current_frame = NULL;
              _l ("(WS-CLIENT) Unable to alloc current frame raw data!\n");
              return -1;
            }
        }
      else
        {
          _l ("(WS-CLIENT) Unable to alloc current frame!\n");
          return -1;
        }
    }
  //
  current = wscData[wsh].current_frame;
  if (current->rawdata_idx >= current->rawdata_sz)
    {
      current->rawdata_sz += FRAME_CHUNK_LENGTH;
      rAllocAux = (char *) dmem_extend (current->rawdata, current->rawdata_sz);
      if (rAllocAux)
        {
//#warning "este realloc luego no se hace el free. Habria que seguir que pasa con esta current->rawdata porque parece que por aqui esta el tema de la fuga de memoria!!!!!!"
          current->rawdata = rAllocAux;
          memset (current->rawdata + current->rawdata_idx, 0, current->rawdata_sz - current->rawdata_idx);
        }
      else
        {
          dmem_release (current->rawdata);
          current->rawdata = NULL;
          dmem_release (wscData[wsh].current_frame);
          wscData[wsh].current_frame = NULL;
          _l ("(WS-CLIENT) Unable to realloc current frame raw data!\n");
          return -1;
        }
    }
  if (current->rawdata)
    {
      *(current->rawdata + current->rawdata_idx++) = in;
      if (complete_frame (wsh, current) == 1)
        {
          if (current->fin == 1)
            {
              //is control frame
              if ((current->opcode & 0x08) == 0x08)
                {
                  if (handle_control_frame (wsh, current) != 0)
                    {
                      _l ("(WS-CLIENT) Error handle control frame!\n");
                      return -1;
                    }
                }
              else
                {
                  dispatch_message (wsh, current);
//#warning "Creo que la movida del alloc que se pierde esta aqui!!!!!!"
                  wscData[wsh].current_frame = NULL; ///<---- Creo que la movida del alloc que se pierde esta aqui!!!!!!
                }
            }
          else
            {
              new = (t_WS_FRAME *) dmem_create (sizeof (t_WS_FRAME));
              if (new)
                {
                  memset (new, 0, sizeof (t_WS_FRAME));
                  new->payload_len = -1;
                  new->rawdata = (char *) dmem_create (FRAME_CHUNK_LENGTH);
                  if (new->rawdata)
                    {
                      memset (new->rawdata, 0, FRAME_CHUNK_LENGTH);
                      new->prev_frame = current;
                      current->next_frame = new;
                      wscData[wsh].current_frame = new;
                    }
                  else
                    {
                      _l ("(WS-CLIENT) Unable to alloc new frame raw data!\n");
                      return -1;
                    }
                }
              else
                {
                  _l ("(WS-CLIENT) Unable to alloc new frame!\n");
                  return -1;
                }
            }
        }
    }
  else
    {
      _l ("(WS-CLIENT) Raw data is NULL!\n");
      return -1;
    }

  return 0;
}

static void perform_web_socket_key (char* wk, size_t length)
{
  uint32_t key_nonce[4]; //16 bytes
  int i;

  srand (time (NULL));
  for (i = 0; i < 4; i++)
    {
      key_nonce[i] = (uint32_t) rand ();
    }
  TCPIP_Helper_Base64Encode ((uint8_t*) key_nonce, sizeof (key_nonce), (uint8_t*) wk, length);
}

static void perfom_expected_key (char *pek, char* ek, size_t length)
{
  char preEncode[WS_KEY_LENGTH];
  uint8_t sha1Hash[SHA_DIGEST_SIZE]; //20

  snprintf (preEncode, WS_KEY_LENGTH, "%s%s", pek, UUID);
  wc_ShaHash ((byte*) preEncode, strlen (preEncode), (byte*) sha1Hash);
  TCPIP_Helper_Base64Encode (sha1Hash, SHA_DIGEST_SIZE, (uint8_t*) ek, length);
}

static void handshake_response_parse (t_WS_HANDLER wsh, char *response, size_t length)
{
  char expectedBase64[WS_KEY_LENGTH * 2];
  int minorv, status, bodyOffset, ih;
  size_t msgLen, nh = HTTP_MAX_RESPONSE_HEADERS;
  struct phr_header h[HTTP_MAX_RESPONSE_HEADERS];
  char *msg;

  //_d ("RESPONSE: \n\n ---\n%s\n ---\n\n", response);
  bodyOffset = phr_parse_response ((const char*) response, length, &minorv, &status, (const char**) &msg, &msgLen, h, &nh, 0);
  if (bodyOffset < 0)
    {
      _l ("(WS-CLIENT) ERROR: parse response. Bodyoffset(%d), headers(%d)\n", bodyOffset, nh);
    }
  else
    {
      *(response + length) = 0; //Null-string at response end
      if (status == 101)
        {
          _l ("(WS-CLIENT) Request valid status\n");
          wscData[wsh].flags |= REQUEST_VALID_STATUS;
          for (ih = 0; ih < nh; ih++)
            {
              if (strncmp ("Upgrade", h[ih].name, h[ih].name_len) == 0)
                {
                  if (strncmp ("websocket", h[ih].value, h[ih].value_len) == 0)
                    {
                      _l ("(WS-CLIENT) Request has upgrade\n");
                      wscData[wsh].flags |= REQUEST_HAS_UPGRADE;
                    }
                }
              else if (strncmp ("Connection", h[ih].name, h[ih].name_len) == 0)
                {
                  if (strncmp ("upgrade", h[ih].value, h[ih].value_len) == 0)
                    {
                      _l ("(WS-CLIENT) Request has connection\n");
                      wscData[wsh].flags |= REQUEST_HAS_CONNECTION;
                    }
                }
              else if (strncmp ("Sec-WebSocket-Accept", h[ih].name, h[ih].name_len) == 0)
                {
                  perfom_expected_key (wscData[wsh].webSocketKey, expectedBase64, WS_KEY_LENGTH * 2);
                  if (strncmp (expectedBase64, h[ih].value, h[ih].value_len) == 0)
                    {
                      _l ("(WS-CLIENT) Request valid accept\n");
                      wscData[wsh].flags |= REQUEST_VALID_ACCEPT;
                    }
                  else
                    {
                      _l ("(WS-CLIENT) Expected key error! (%s)\n\n", expectedBase64);
                    }
                }
            }
        }
      else
        {
          _l ("(WS-CLIENT) HTTP status error. (status=%d)\n", status);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

void ws_client_init (void)
{
  int32_t i;

  for (i = 0; i < WS_MAX_CLIENTS; i++)
    {
      memset (&wscData[i], 0, sizeof (t_WS_CLIENT_DATA));
      SET_WS_CLIENT_STATE (i, WS_STATE_INIT);
    }
  _l ("(WS-CLIENT) Init...\n");
}

void ws_client_task (void)
{
  t_HTTP_REQUEST_HANDLE *reqHandle;
  t_WS_HANDLER wsh;
  int i, n;
  char *t;

  for (wsh = 0; wsh < WS_MAX_CLIENTS; wsh++)
    {
      switch (wscData[wsh].state)
        {
        case WS_STATE_INIT:
          _l ("(WS-CLIENT)(%d) Ready for connection.\n", wsh);
          SET_WS_CLIENT_STATE (wsh, WS_STATE_IDLE);
          break;
        case WS_STATE_IDLE:
          //...
          break;
        case WS_STATE_MANAGE_URL:
          _d ("(WS-CLIENT)(%d) Manage URL context:\n scheme: %s\n host: %s\n url port: %s\n path: %s\n", wscData[wsh].urlData->scheme, wscData[wsh].urlData->host, wscData[wsh].urlData->port, wscData[wsh].urlData->path);
          SET_WS_CLIENT_STATE (wsh, WS_STATE_OPEN_CONNECTION);
          if (strcmp (wscData[wsh].urlData->scheme, "wss") == 0)
            {
              wscData[wsh].nPort = wscData[wsh].urlData->port ? atoi (wscData[wsh].urlData->port) : 443;
              wscData[wsh].flags |= CLIENT_IS_SSL;
              _d ("(WS-CLIENT)(%d) Connecting on port: %d\n", wsh, (int) wscData[wsh].nPort);
            }
          else if (strcmp (wscData[wsh].urlData->scheme, "ws") == 0)
            {
              wscData[wsh].nPort = wscData[wsh].urlData->port ? atoi (wscData[wsh].urlData->port) : 80;
              _d ("(WS-CLIENT)(%d) connecting on port: %d\n", wsh, (int) wscData[wsh].nPort);
            }
          else
            {
              _l ("(WS-CLIENT)(%d) ERROR. Scheme NOT valid. Halted!\n", wsh);
              SET_WS_CLIENT_STATE (wsh, WS_STATE_HALT);
            }
          break;
        case WS_STATE_OPEN_CONNECTION:
          if (tcp_client_open_host (wscData[wsh].urlData->host, wscData[wsh].nPort, (wscData[wsh].flags & CLIENT_IS_SSL), sock_cb, &wscData[wsh]) == TCP_CLIENT_OP_SUCCESS)
            {
              _l ("(WS-CLIENT)(%d) Connecting to:\n host: %s\n port: %d\n", wsh, wscData[wsh].urlData->host, wscData[wsh].nPort);
              SET_WS_CLIENT_STATE (wsh, WS_STATE_WAIT_TCP_SOCKET_CONNECTION);
            }
          else
            {
              _l ("(WS-CLIENT)(%d) Unable to connect!\n", wsh);
              SET_WS_CLIENT_STATE (wsh, WS_STATE_ERROR);
            }
          break;
        case WS_STATE_WAIT_TCP_SOCKET_CONNECTION:
          //...
          break;
        case WS_STATE_INIT_HANDSHAKE:
          perform_web_socket_key (wscData[wsh].webSocketKey, WS_KEY_LENGTH);
          reqHandle = http_request_init (HTTP_METHOD_GET, wscData[wsh].urlData->host, wscData[wsh].nPort, wscData[wsh].urlData->path, NULL);
          if (reqHandle)
            {
              http_request_add_generic (reqHandle, "Upgrade", "websocket");
              http_request_add_generic (reqHandle, "Connection", "Upgrade");
              http_request_add_generic (reqHandle, "Sec-WebSocket-Key", wscData[wsh].webSocketKey);
              http_request_add_generic (reqHandle, "Sec-WebSocket-Version", "13");
              /* SEND*/
              if (tcp_client_send (wscData[wsh].curSocketId, (uint8_t*) reqHandle->raw, reqHandle->length) == TCP_CLIENT_OP_SUCCESS)
                {
                  memset (wscData[wsh].recv_buf, 0, WS_RECEIVE_BUFFER);
                  //_d ("(WS CLIENT INIT HANDSHAKE) Sending request...\n ---\n%s\n ---\n", reqHandle->raw);
                  SET_WS_CLIENT_STATE (wsh, WS_STATE_WAIT_HANDSHAKE_RESPONSE);
                }
              else
                {
                  tcp_client_close (wscData[wsh].curSocketId);
                  _l ("(WS-CLIENT)(%d) Send handshake error!\n", wsh);
                  SET_WS_CLIENT_STATE (wsh, WS_STATE_ERROR);
                }
              http_request_delete (&reqHandle);
            }
          else
            {
              tcp_client_close (wscData[wsh].curSocketId);
              _l ("(WS-CLIENT)(%d) HTTP request error!\n", wsh);
              SET_WS_CLIENT_STATE (wsh, WS_STATE_ERROR);
            }
          break;
        case WS_STATE_WAIT_HANDSHAKE_RESPONSE:
          //TODO: actually handle data after \r\n\r\n in case server sends post-handshake data that gets coalesced in this recv
          n = tcp_client_peek_rx (wscData[wsh].curSocketId, (uint8_t*) wscData[wsh].recv_buf, WS_RECEIVE_BUFFER - 1);
          if (n > 4)
            {
              t = strstr (wscData[wsh].recv_buf, "\r\n\r\n");
              if (t != NULL)
                {
                  //_d ("(WS CLIENT PARSE RESPONSE) buffer=%d, fh=%d\n", n, (int) (t - wscData[wsid].recv_buf + 4));
                  tcp_client_get_rx (wscData[wsh].curSocketId, (uint8_t*) wscData[wsh].recv_buf, (uint16_t) (t - wscData[wsh].recv_buf + 4));
                  SET_WS_CLIENT_STATE (wsh, WS_STATE_WAIT_HANDSHAKE_RESPONSE_PARSE);
                }
            }
          break;
        case WS_STATE_WAIT_HANDSHAKE_RESPONSE_PARSE:
          handshake_response_parse (wsh, wscData[wsh].recv_buf, strlen (wscData[wsh].recv_buf));
          if ((wscData[wsh].flags & REQUEST_HAS_UPGRADE) && (wscData[wsh].flags & REQUEST_HAS_CONNECTION) && (wscData[wsh].flags & REQUEST_VALID_ACCEPT))
            {
              wscData[wsh].flags &= ~CLIENT_CONNECTING;
              if (wscData[wsh].onOpen) wscData[wsh].onOpen (wsh);
              _l ("(WS-CLIENT)(%d) CONNECTED!\n", wsh);
              SET_WS_CLIENT_STATE (wsh, WS_STATE_CONNECTION_ESTABLISHED);
            }
          else
            {
              tcp_client_close (wscData[wsh].curSocketId);
              _l ("(WS-CLIENT)(%d) Error!\n", wsh);
              SET_WS_CLIENT_STATE (wsh, WS_STATE_ERROR);
            }
          break;
        case WS_STATE_CONNECTION_ESTABLISHED:
          n = tcp_client_get_rx (wscData[wsh].curSocketId, (uint8_t*) wscData[wsh].recv_buf, WS_RECEIVE_BUFFER - 1);
          if (n)
            {
              GET_CORE_TIMESTAMP (wscData[wsh].ts);
              for (i = 0; i < n; i++)
                in_data (wsh, wscData[wsh].recv_buf[i]);
            }
          else
            {
              if (wscData[wsh].activityTimeout > 0)
                {
                  if (ELAPSED_S (wscData[wsh].ts) > wscData[wsh].activityTimeout)
                    {
                      //SEND PING
                      GET_CORE_TIMESTAMP (wscData[wsh].ts);
                      ws_client_send (wsh, wscData[wsh].pingStr);
                    }
                }
            }
          break;
        case WS_STATE_ERROR:
          tcp_client_close (wscData[wsh].curSocketId);
          _l ("(WS-CLIENT)(%d) ERROR! -> client close!\n", wsh);
          SET_WS_CLIENT_STATE (wsh, WS_STATE_CLOSE);
          break;
        case WS_STATE_CLOSE:
          GET_CORE_TIMESTAMP (wscData[wsh].ts);
          if (wscData[wsh].onClose) wscData[wsh].onClose (wsh);
          if (wscData[wsh].flags & CLIENT_SHOULD_CLOSE)
            {
              if (wscData[wsh].urlData)
                {
                  url_parse_free (wscData[wsh].urlData);
                  wscData[wsh].urlData = NULL;
                }
              _l ("(WS-CLIENT)(%d) CLOSE!\n", wsh);
              SET_WS_CLIENT_STATE (wsh, WS_STATE_INIT);
            }
          else
            {
              _l ("(WS-CLIENT)(%d) CLOSE -> client retry in %ds!\n", wsh, (int) RETRY_TIME_S);
              SET_WS_CLIENT_STATE (wsh, WS_STATE_RETRY);
            }
          break;
        case WS_STATE_RETRY:
          if (ELAPSED_S (wscData[wsh].ts) > RETRY_TIME_S)
            {
              _l ("(WS-CLIENT)(%d) Retry init...\n", wsh);
              SET_WS_CLIENT_STATE (wsh, WS_STATE_MANAGE_URL);
            }
          break;
          //
        case WS_STATE_HALT:
          break;
        default:
          _l ("(WS-CLIENT)(%d) Unknown state -> client init\n", wsh);
          SET_WS_CLIENT_STATE (wsh, WS_STATE_INIT);
          break;
        }
    }
}

void ws_client_reset (void)
{
  _l ("(WS-CLIENT) WS RESET Do nothing!!!!!\n");
  return;
}

t_WS_HANDLER ws_client_open (char* url, WS_ON_OPEN_CALLBACK onOpen_cb, WS_ON_CLOSE_CALLBACK onClose_cb, WS_ON_MESSAGE_CALLBACK onMessage_cb, WS_ON_ERROR_CALLBACK onError_cb)
{
  t_WS_HANDLER wsid = 0;
  bool found = false;

  while ((wsid < WS_MAX_CLIENTS) && !found)
    {
      if (wscData[wsid].state == WS_STATE_IDLE)
        {
          found = true;
          wscData[wsid].urlData = url_parse (url);
          if (wscData[wsid].urlData)
            {
              wscData[wsid].flags |= CLIENT_CONNECTING;
              wscData[wsid].onOpen = onOpen_cb;
              wscData[wsid].onClose = onClose_cb;
              wscData[wsid].onMessage = onMessage_cb;
              wscData[wsid].onError = onError_cb;
              SET_WS_CLIENT_STATE (wsid, WS_STATE_MANAGE_URL);
            }
          else
            {
              _l ("(WS-CLIENT) Unable to parse url data!\n");
              wsid = WS_INVALID_HANDLER;
            }
        }
      else
        {
          wsid++;
        }
    }
  if (!found)
    {
      _l ("(WS-CLIENT) No ws socket available!\n");
      wsid = WS_INVALID_HANDLER;
    }

  return wsid;
}

int ws_client_set_ping (t_WS_HANDLER wsh, char *strPing, uint32_t pingInterval)
{
  wscData[wsh].activityTimeout = pingInterval;
  snprintf (wscData[wsh].pingStr, WS_STR_PING_LENGTH, "%s", strPing);
  _l ("(WS-CLIENT)(%d) Set ping interval: %us, ping string = %s\n", wsh, wscData[wsh].activityTimeout, wscData[wsh].pingStr);

  return 0;
}

int ws_client_send (t_WS_HANDLER wsh, char *strdata)
{
  unsigned char mask[4];
  unsigned int mask_int;
  unsigned long long payload_len;
  unsigned char finNopcode;
  unsigned int payload_len_small;
  unsigned int payload_offset = 6;
  unsigned int len_size;
  unsigned int ret = -1; //by default error
  int i;
  unsigned int frame_size;
  char *data;
  t_WS_ERROR e;

  if (wscData[wsh].flags & CLIENT_SENT_CLOSE_FRAME)
    {
      _l ("(WS-CLIENT)(%d) WS_SEND_AFTER_CLOSE_FRAME_ERR! flags = %d\n", wsh, wscData[wsh].flags);
      if (wscData[wsh].onError)
        {
          e.code = 1;
          e.extra_code = 0;
          e.str = NULL;
          wscData[wsh].onError (wsh, &e);
        }
      SET_WS_CLIENT_STATE (wsh, WS_STATE_ERROR);
      return ret;
    }
  if (wscData[wsh].flags & CLIENT_CONNECTING)
    {
      _l ("(WS-CLIENT)(%d) WS_SEND_DURING_CONNECT_ERR! flags = %d\n", wsh, wscData[wsh].flags);
      if (wscData[wsh].onError)
        {
          e.code = 1;
          e.extra_code = 1;
          e.str = NULL;
          wscData[wsh].onError (wsh, &e);
        }
      SET_WS_CLIENT_STATE (wsh, WS_STATE_ERROR);
      return ret;
    }
  if (strdata == NULL)
    {
      _l ("(WS-CLIENT)(%d) WS_SEND_NULL_DATA_ERR! flags = %d\n", wsh, wscData[wsh].flags);
      if (wscData[wsh].onError)
        {
          e.code = 1;
          e.extra_code = 2;
          e.str = NULL;
          wscData[wsh].onError (wsh, &e);
        }
      SET_WS_CLIENT_STATE (wsh, WS_STATE_ERROR);
      return ret;
    }
  srand (time (NULL));
  mask_int = rand ();
  memcpy (mask, &mask_int, 4);
  payload_len = strlen (strdata);
  finNopcode = 0x81; //FIN and text opcode.
  if (payload_len <= 125)
    {
      frame_size = 6 + payload_len;
      payload_len_small = payload_len;
    }
  else if (payload_len > 125 && payload_len <= 0xffff)
    {
      frame_size = 8 + payload_len;
      payload_len_small = 126;
      payload_offset += 2;
    }
  else if (payload_len > 0xffff && payload_len <= 0xffffffffffffffffLL)
    {
      frame_size = 14 + payload_len;
      payload_len_small = 127;
      payload_offset += 8;
    }
  else
    {
      _l ("(WS-CLIENT)(%d) WS_SEND_DATA_TOO_LARGE_ERR!\n", wsh);
      if (wscData[wsh].onError)
        {
          e.code = 1;
          e.extra_code = 3;
          e.str = NULL;
          wscData[wsh].onError (wsh, &e);
        }
      SET_WS_CLIENT_STATE (wsh, WS_STATE_ERROR);
      return ret;
    }
  data = (char *) dmem_create (frame_size);
  if (data)
    {
      memset (data, 0, frame_size);
      *data = finNopcode;
      *(data + 1) = payload_len_small | 0x80; //payload length with mask bit on
      if (payload_len_small == 126)
        {
          payload_len &= 0xffff;
          len_size = 2;
          for (i = 0; i < len_size; i++)
            {
              *(data + 2 + i) = *((char *) &payload_len + (len_size - i - 1));
            }
        }
      if (payload_len_small == 127)
        {
          payload_len &= 0xffffffffffffffffLL;
          len_size = 8;
          for (i = 0; i < len_size; i++)
            *(data + 2 + i) = *((char *) &payload_len + (len_size - i - 1));
        }
      for (i = 0; i < 4; i++)
        *(data + (payload_offset - 4) + i) = mask[i];

      memcpy (data + payload_offset, strdata, strlen (strdata));
      for (i = 0; i < strlen (strdata); i++)
        *(data + payload_offset + i) ^= mask[i % 4] & 0xff;

      _d ("(WS-CLIENT)(%d) sending ... (%s)\n\n", wsh, strdata);
      //_d ("(WS CLIENT SEND) sending ... \n");
      if (tcp_client_send (wscData[wsh].curSocketId, (uint8_t*) data, frame_size) == TCP_CLIENT_OP_SUCCESS)
        {
          ret = 0; //ALL OK
        }
      else
        {
          _l ("(WS-CLIENT)(%d) ERROR. ABORT SEND!\n", wsh);
          if (wscData[wsh].onError)
            {
              e.code = 1;
              e.extra_code = 4;
              e.str = NULL;
              wscData[wsh].onError (wsh, &e);
            }
          SET_WS_CLIENT_STATE (wsh, WS_STATE_ERROR);
        }
      //free (data);
      dmem_release (data);
    }
  else
    {
      _l ("(WS-CLIENT)(%d) Data alloc error. NO SEND!\n", wsh);
      if (wscData[wsh].onError)
        {

          e.code = 1;
          e.extra_code = 4;
          e.str = NULL;
          wscData[wsh].onError (wsh, &e);
        }
      SET_WS_CLIENT_STATE (wsh, WS_STATE_ERROR);
    }

  return ret;
}

int ws_client_close (t_WS_HANDLER wsh)
{
  tcp_client_close (wscData[wsh].curSocketId);
  wscData[wsh].flags |= CLIENT_SHOULD_CLOSE;
  _l ("(WS-CLIENT)(%d) Closing...\n", wsh);

  return 0;
}

