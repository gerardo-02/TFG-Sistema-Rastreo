#ifndef WS_CLIENT_H
#define	WS_CLIENT_H

#include <stdint.h>
#include "../url_parser.h"

#define WS_MAX_CLIENTS          (1)
#define WS_INVALID_HANDLER      (-1)
typedef int t_WS_HANDLER;

#ifdef	__cplusplus
extern "C" {
#endif

    typedef enum {
        WS_STATE_INIT = 0,
        WS_STATE_IDLE,
        WS_STATE_MANAGE_URL,
        WS_STATE_OPEN_CONNECTION,
        WS_STATE_WAIT_TCP_SOCKET_CONNECTION,
        WS_STATE_INIT_HANDSHAKE,
        WS_STATE_WAIT_HANDSHAKE_RESPONSE,
        WS_STATE_WAIT_HANDSHAKE_RESPONSE_PARSE,
        WS_STATE_CONNECTION_ESTABLISHED,
        WS_STATE_CLOSE,
        //
        WS_STATE_ERROR,
        WS_STATE_RETRY,
        WS_STATE_HALT,
        WS_STATE_NULL,
    } t_WS_STATES;

    typedef struct {
        unsigned int opcode;
        unsigned long long payload_len;
        char *payload;
    } t_WS_MESSAGE;

    typedef struct {
        int code;
        int extra_code;
        char *str;
    } t_WS_ERROR;

    typedef void (*WS_ON_OPEN_CALLBACK)(t_WS_HANDLER wsh);
    typedef void (*WS_ON_CLOSE_CALLBACK)(t_WS_HANDLER wsh);
    typedef void (*WS_ON_MESSAGE_CALLBACK)(t_WS_HANDLER wsh, t_WS_MESSAGE *msg);
    typedef void (*WS_ON_ERROR_CALLBACK)(t_WS_HANDLER wsh, t_WS_ERROR *err);

    ////////////////////////////////////////////////////////////////////////////////
    void ws_client_init(void);
    void ws_client_task(void);
    void ws_client_reset(void);
    //
    t_WS_HANDLER ws_client_open(char* url, WS_ON_OPEN_CALLBACK onOpen_cb, WS_ON_CLOSE_CALLBACK onClose_cb, WS_ON_MESSAGE_CALLBACK onMessage_cb, WS_ON_ERROR_CALLBACK onError_cb);
    int ws_client_send(t_WS_HANDLER wsh, char *strdata);
    int ws_client_close(t_WS_HANDLER wsh);

#ifdef	__cplusplus
}
#endif

#endif	/* WS_CLIENT_H */

