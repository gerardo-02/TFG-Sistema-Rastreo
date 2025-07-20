#ifndef TCP_CLIENT_H
#define	TCP_CLIENT_H

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
#define TCP_CLIENT_MAX_SOCKETS          (3)

#define TCP_CLIENT_OP_SUCCESS           (0)
#define TCP_CLIENT_OP_ERROR             (-1)    
#define TCP_CLIENT_OP_BUSY              (-2)
#define TCP_CLIENT_OP_WORNG_PARAMETERS  (-3)
#define TCP_CLIENT_OP_WORNG_ID_CLIENT   (-4) 

    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Data types & Definitions                                          */
    /* ************************************************************************** */

    /* ************************************************************************** */
    typedef enum {
        CLIENT_EVENT_NOTHING = 0,
        CLIENT_EVENT_CONNECTED,
        CLIENT_EVENT_TX,
        CLIENT_EVENT_RX,
        CLIENT_EVENT_DISCONNECTED,
        CLIENT_EVENT_TIMEOUT,
        CLIENT_EVENT_ERROR,
    } t_CLIENT_EVENTS;

    typedef enum {
        TCP_CLIENT_STATE_IDLE,
        //
        TCP_CLIENT_STATE_DNS_RESOLVE_HOST_ADDRESS,
        TCP_CLIENT_STATE_DNS_WAIT,
        TCP_CLIENT_STATE_DNS_TIMEOUT,
        TCP_CLIENT_STATE_DNS_ERROR,
        //
        TCP_CLIENT_STATE_CONNECTION_INIT,
        TCP_CLIENT_STATE_CONNECTION_WAITING_FOR,
        TCP_CLIENT_STATE_CONNECTION_SSL_NEGOTIATION_WAIT,
        TCP_CLIENT_STATE_CONNECTION_STABLISHED,
        TCP_CLIENT_STATE_CONNECTION_TIMEOUT,
        TCP_CLIENT_STATE_CONNECTION_ERROR,
        TCP_CLIENT_STATE_CONNECTION_CLOSE,
        //TCP_CLIENT_STATE_CONNECTION_CLEAN_UP,
        //
        TCP_CLIENT_STATE_WAIT_FOR_RETRY,
    } t_TCP_CLIENT_STATES;

    typedef struct {
        int32_t sid;
        t_CLIENT_EVENTS event;
        uint16_t dataLength;
    } t_TCP_CLIENT_CONTEXT;
    typedef void (*TCP_CLIENT_CALLBACK)(t_TCP_CLIENT_CONTEXT *tcpCtx, void *appCtx);

    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Interface Functions                                               */
    /* ************************************************************************** */
    /* ************************************************************************** */
    void tcp_client_init(void);
    void tcp_client_task(void);
    void tcp_client_reset(void);
    t_TCP_CLIENT_STATES tcp_client_get_state(int32_t sid);
    // Host name or ip string format --> DNS
    int32_t tcp_client_open_host(char *host, TCP_PORT port, bool tls, TCP_CLIENT_CALLBACK cb, void *ctx);
    // Host IP directly
    int32_t tcp_client_open_ip(IPV4_ADDR ip, TCP_PORT port, bool tls, TCP_CLIENT_CALLBACK cb, void *ctx);
    int32_t tcp_client_send(int32_t sid, uint8_t *data, uint16_t dataLength);
    uint16_t tcp_client_peek_rx(int32_t sid, uint8_t *buffer, uint16_t requestLength);
    uint16_t tcp_client_get_rx(int32_t sid, uint8_t *buffer, uint16_t requestLength);
    int32_t tcp_client_close(int32_t sid);

#ifdef	__cplusplus
}
#endif

#endif	/* TCP_CLIENT_H */

/* ************************************************************** End of File */
