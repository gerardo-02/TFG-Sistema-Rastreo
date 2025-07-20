/* 
 * File:   sockets.h
 * Author: jtrodriguez
 *
 * Created on 10 de marzo de 2021, 14:30
 */

#ifndef SOCKETS_H
#define	SOCKETS_H


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

#ifdef	__cplusplus
extern "C" {
#endif

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
    typedef enum {
        SOCKET_SERVICE_STATE_INIT = 0,
        SOCKET_SERVICE_STATE_GET_STATUS,
        SOCKET_SERVICE_STATE_CLEAN_UP,
        SOCKET_SERVICE_STATE_IDLE,
        SOCKET_SERVICE_STATE_CLIENT_WAIT_OPEN,
        SOCKET_SERVICE_STATE_CLIENT_OPEN_ERROR,        
        SOCKET_SERVICE_STATE_CLIENT_DELIVER_TO_SOCKET,        
        //
        SOCKET_SERVICE_STATE_WAIT_RESPONSE,
    } t_SOCKET_SERVICE_STATES;

    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Interface Functions                                               */
    /* ************************************************************************** */
    /* ************************************************************************** */
    void sockets_service_time(void);
    void sockets_service_init(void);
    t_SOCKET_SERVICE_STATES sockets_service_tasks(void);
    int32_t sockets_send(uint8_t index, uint8_t *data, uint32_t dataLength, SIMCOM_SENT_CALLBACK cb);
    int32_t sockets_tcp_send(t_IPV4_ADDR ip, uint16_t port, uint8_t *data, uint32_t dataLength, SIMCOM_SENT_CALLBACK cb);

#ifdef	__cplusplus
}
#endif

#endif	/* SOCKETS_H */

