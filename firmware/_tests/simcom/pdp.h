/* 
 * File:   pdp.h
 * Author: jtrodriguez
 *
 * Created on 9 de marzo de 2021, 11:04
 */

#ifndef PDP_H
#define	PDP_H

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
        PDP_STATE_INIT = 0,
        PDP_STATE_TEST_OPEN_NET,
        PDP_STATE_CFG_TCP_SENDMODE,
        PDP_STATE_CFG_TCP_TIMEOUTS,                
        PDP_STATE_CFG_TCP_MODE,                
        PDP_STATE_OPEN_NET,
        PDP_STATE_GET_LOCAL_IP,
        PDP_STATE_NET_OPENED,
        //
        PDP_STATE_WAIT_RESPONSE,
    } t_PDP_STATES;

    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Interface Functions                                               */
    /* ************************************************************************** */
    /* ************************************************************************** */
    void pdp_time(void);
    void pdp_init(void);
    t_PDP_STATES pdp_tasks(void);

#ifdef	__cplusplus
}
#endif

#endif	/* PDP_H */

