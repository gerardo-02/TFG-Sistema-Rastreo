#ifndef TCP_SERVER_H
#define	TCP_SERVER_H

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <stdint.h>
#include "tcpip/tcpip.h"
#include "tcpip/tcp.h"

#ifdef	__cplusplus
extern "C" {
#endif

    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Constants                                                         */
    /* ************************************************************************** */
    /* ************************************************************************** */
#define TCP_SERVER_MAX_SOCKETS          (3)

#define TCP_SERVER_OP_SUCCESS           (0)
#define TCP_SERVER_OP_ERROR             (-1)    
#define TCP_SERVER_OP_BUSY              (-2)
#define TCP_SERVER_OP_WORNG_PARAMETERS  (-3)
#define TCP_SERVER_OP_WORNG_ID_CLIENT   (-4) 

    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Data types & Definitions                                          */
    /* ************************************************************************** */

    /* ************************************************************************** */
    typedef enum {
        SERVER_EVENT_CONNECTION_STABLISHED,
        SERVER_EVENT_MESSAGE_SENT,
        SERVER_EVENT_DISCONNECTED,
        SERVER_EVENT_CLOSE,
        SERVER_EVENT_ERROR,
    } t_SERVER_EVENTS;

    typedef struct {
        int32_t serverIndex;
        t_SERVER_EVENTS event;
    } t_TCP_SERVER_EVENT_CALLBACK_CONTEXT;
    typedef void (*TCP_SERVER_EVENT_CALLBACK)(t_TCP_SERVER_EVENT_CALLBACK_CONTEXT *ctx);
    typedef uint16_t(*TCP_SERVER_RX_PROCESSOR)(int32_t serverIndex);

    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Interface Functions                                               */
    /* ************************************************************************** */
    /* ************************************************************************** */
    void tcp_server_init(void);
    void tcp_server_task(void);
    void tcp_server_reset(void);
    int32_t tcp_server_config(TCP_PORT listenPort, TCP_SERVER_RX_PROCESSOR rxP, TCP_SERVER_EVENT_CALLBACK cb);
    uint16_t tcp_server_get_rx_length(int32_t serverIndex);
    uint16_t tcp_server_pull_rx_data(int32_t serverIndex, uint8_t* buffer, uint16_t count);
    void tcp_server_force_disconnect(int32_t serverIndex);
    int32_t tcp_server_send(int32_t serverIndex, uint8_t *data, uint16_t dataLength);
#ifdef	__cplusplus
}
#endif

#endif	/* TCP_SERVER_H */

/* ************************************************************** End of File */
