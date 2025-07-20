#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "log.h"
#include "cjson.h"
#include "drv/pmz_time.h"
#include "ws_client.h"

#include "web_socket_pusher.h"
#include "drv/drv_dmem.h"

////////////////////////////////////////////////////////////////////////////////
#define MAX_LENGTH_EVENT_MESSAGE    64
#define MAX_LENGTH_QUERY_FRAME      2048

typedef struct
{
  bool inUse;
  char event_register[MAX_LENGTH_EVENT_MESSAGE];
  WS_MESSAGE_CALLBACK cb;
  void *ctx;
}
t_EVENT_MESSAGE_REGISTER;
#define MAX_MESSAGES_REGISTER_LIST  16

////////////////////////////////////////////////////////////////////////////////

static t_WS_CLIENT *ws_client = NULL;
static t_EVENT_MESSAGE_REGISTER msgList[MAX_MESSAGES_REGISTER_LIST];

////////////////////////////////////////////////////////////////////////////////

static void event_connection_established_cb (char *data, void *ctx)
{
  char dd[128];
  cJSON *info, *at;

  _d ("(WEB SOCKET) DATA: %s\n", data);
  info = cJSON_Parse (data);
  if (info)
    {
      at = cJSON_GetObjectItemCaseSensitive (info, "activity_timeout");
      if (at)
        {
          ws_client->activityTimeout = (uint32_t) cJSON_GetNumberValue (at);
          _l ("(WEB SOCKET) Set activity timeout: %d\n", (int) ws_client->activityTimeout);
        }
    }
  //snprintf (dd, sizeof (dd), "{\"event\":\"pusher:subscribe\",\"data\":{\"auth\":\"\",\"channel\":\"ac3.%llu\"}}", ac.rv.reg.hwId.id);
  snprintf (dd, sizeof (dd), "{\"event\":\"pusher:subscribe\",\"data\":{\"auth\":\"\",\"channel\":\"xxx.123456789\"}}");
  ws_send (ws_client, dd);
  return;
}

static void event_pong_cb (char *data, void *ctx)
{
  _l ("(WEB SOCKET) PONG!\n");
  return;
}

static void event_subscription_succeeded_cb (char *data, void *ctx)
{
  _l ("(WEB SOCKET) Subcription success. **** System ONLINE ****\n");
  //  ac.rv.reg.systemOnline = 1;
  return;
}

static int ws_onclose_cb (t_WS_CLIENT *c)
{
  _l ("(WEB SOCKET) ON-CLOSE: %d. **** System OFFLINE ****\n", c->curSocketId);
  //  ac.rv.reg.systemOnline = 0;
  return 0;
}

static int ws_onerror_cb (t_WS_CLIENT *c, t_WS_ERROR *err)
{
  _l ("(WEB SOCKET) ON-ERROR: (%d): %s\n", err->code, err->str);
  if (err->extra_code)
    {
      _l ("(WEB SOCKET) Extra code error: (%d): %s\n", err->extra_code);
    }

  return 0;
}

static int ws_onmessage_cb (t_WS_CLIENT *c, t_WS_MESSAGE *msg)
{
  cJSON *mmm, *event;
  int i, found;
  char *t;

  _d ("(ON_MESSAGE) t=%lu\n", *pmz_time_get ());
  mmm = cJSON_ParseWithLength (msg->payload, msg->payload_len);
  if (mmm)
    {
      event = cJSON_GetObjectItemCaseSensitive (mmm, "event");
      if (event)
        {
          i = 0;
          found = 0;
          while ((i < MAX_MESSAGES_REGISTER_LIST) && !found)
            {
              if (msgList[i].inUse)
                {
                  if (strcmp (msgList[i].event_register, cJSON_GetStringValue (event)) == 0)
                    {
                      found = 1;
                      msgList[i].cb (cJSON_GetStringValue (cJSON_GetObjectItemCaseSensitive (mmm, "data")), msgList[i].ctx);
                    }
                }
              i++;
            }
          if (!found)
            {
              _l ("(WEB SOCKET) Event NOT registered!\n  -> %s\n", t = cJSON_Print (mmm));
              dmem_release (t);
            }
        }
      else
        {
          _l ("(WEB SOCKET) NO EVENT field!\n  -> %s\n", t = cJSON_Print (mmm));
          dmem_release (t);
        }
      cJSON_Delete (mmm);
    }
  else
    {
      _l ("(WEB SOCKET) ON-MESSAGE: message undefined(len=%llu): %s\n", msg->payload_len, msg->payload);
    }

  return 0;
}

static int ws_onopen_cb (t_WS_CLIENT * c)
{
  _l ("(WEB SOCKET) ON-OPEN: %d\n", (int) c->curSocketId);
  return 0;
}

////////////////////////////////////////////////////////////////////////////////

void web_socket_pusher_init (void)
{
  //  ac.rv.reg.systemOnline = 0;
  //
  //ws_client = ws_client_init (ac.sysCfg.reg.wsURL);
  ws_client = ws_client_init ();
  //ws_client = ws_client_init ("ws://10.114.10.104/app/MDNjY2NkZDlkNTQ2Njc1ODE1MzE2N2Ez");
  if (ws_client)
    {
      ws_onopen_callback_register (ws_client, ws_onopen_cb);
      ws_onclose_callback_register (ws_client, ws_onclose_cb);
      ws_onmessage_callback_register (ws_client, ws_onmessage_cb);
      ws_onerror_callback_register (ws_client, ws_onerror_cb);
      memset (msgList, 0, sizeof (msgList));
      //Register local events
      web_socket_pusher_event_register ("pusher:connection_established", event_connection_established_cb, NULL);
      web_socket_pusher_event_register ("pusher:pong", event_pong_cb, NULL);
      web_socket_pusher_event_register ("pusher_internal:subscription_succeeded", event_subscription_succeeded_cb, NULL);
	  
	  //SEND PING
      //ws_send (client, "{\"event\":\"pusher:ping\",\"data\":{}}");
    }
}

void web_socket_pusher_set_url (char *url)
{
  ws_client_set_url (ws_client, url);
}

void web_socket_pusher_task (void)
{
  ws_client_task (ws_client);
}

void web_socket_pusher_reset (void)
{
  if (ws_client)
    {
      free (ws_client);
      ws_client = NULL;
    }
}

//int web_socket_query (t_WS_MESSAGE_TYPE type, char *data) {
//  //  char ws_frame[MAX_LENGTH_QUERY_FRAME];
//  //  int i;
//  //
//  //  i = snprintf (ws_frame, MAX_LENGTH_QUERY_FRAME, "{\"event\":");
//  //  switch (type)
//  //    {
//  //    case WS_MESSAGE_TYPE_CLIENT_RESPONSE:
//  //      i += snprintf (ws_frame + i, MAX_LENGTH_QUERY_FRAME - i, "\"client-response\"");
//  //      break;
//  //    case WS_MESSAGE_TYPE_CLIENT_CHECK_ACCESO:
//  //      i += snprintf (ws_frame + i, MAX_LENGTH_QUERY_FRAME - i, "\"client-check-access\"");
//  //      break;
//  //    case WS_MESSAGE_TYPE_CLIENT_EVENT:
//  //      i += snprintf (ws_frame + i, MAX_LENGTH_QUERY_FRAME - i, "\"client-event\"");
//  //      break;
//  //    default:
//  //      _l ("(WEB SOCKET) (%d) Illegal type!. NO SEND!\n", (int) type);
//  //      return -1;
//  //      break;
//  //    }
//  //  snprintf (ws_frame + i, sizeof (ws_frame) - i, ",\"channel\":\"ac3.%llu\",\"data\":%s}", ac.rv.reg.hwId.id, data);
//  //  return ws_send (ws_client, ws_frame);
//}

int web_socket_pusher_event_register (char* event, WS_MESSAGE_CALLBACK cb, void *ctx)
{
  int i = 0;

  if (strlen (event) < MAX_LENGTH_EVENT_MESSAGE)
    {
      while (i < MAX_MESSAGES_REGISTER_LIST)
        {
          if (!msgList[i].inUse)
            {
              strcpy (msgList[i].event_register, event);
              msgList[i].cb = cb;
              msgList[i].ctx = ctx;
              msgList[i].inUse = true;
              _l ("(WEB SOCKET) Registered event message [%s] with id=%d\n", event, i);
              return i;
            }
          i++;
        }
      _l ("(WEB SOCKET) Message list full!\n");
    }
  else
    {
      _l ("(WEB SOCKET) Event message to register, too large!\n");
    }

  return -1;
}

int web_socket_pusher_event_unregister (int registerEventId)
{
  if (registerEventId < MAX_MESSAGES_REGISTER_LIST)
    {
      msgList[registerEventId].inUse = false;
      _l ("(WEB SOCKET) Unregistered event message with id=%d\n", registerEventId);
      return 0;
    }

  return -1;
}