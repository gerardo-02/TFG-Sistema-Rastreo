#ifndef WEB_SOCKET_H
#define	WEB_SOCKET_H

//typedef enum {
//    WS_MESSAGE_TYPE_CLIENT_RESPONSE,
//    WS_MESSAGE_TYPE_CLIENT_CHECK_ACCESO,
//    WS_MESSAGE_TYPE_CLIENT_EVENT,
//} t_WS_MESSAGE_TYPE;

#ifdef	__cplusplus
extern "C" {
#endif

    typedef void (*WS_MESSAGE_CALLBACK)(char *data, void *ctx);

    void web_socket_pusher_init(void);
    void web_socket_pusher_set_url(char *url);
    void web_socket_pusher_task(void);
    void web_socket_pusher_reset(void);
    //    int web_socket_query(t_WS_MESSAGE_TYPE type, char *data);
    int web_socket_pusher_event_register(char* event, WS_MESSAGE_CALLBACK cb, void *ctx);
    int web_socket_pusher_event_unregister(int registerEventId);

#ifdef	__cplusplus
}
#endif

#endif	/* WEB_SOCKET_H */

