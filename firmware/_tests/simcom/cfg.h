/* 
 * File:   cfg.h
 * Author: jtrodriguez
 *
 * Created on 1 de marzo de 2021, 9:01
 */

#ifndef CFG_H
#define	CFG_H

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
        CFG_STATE_INIT = 0,
        //
        CFG_STATE_AUTOCSQ,
        CFG_STATE_CMEE,
        CFG_STATE_CNMP_AUTO,
        CFG_STATE_CPSI,
        CFG_STATE_CNSMOD,
        CFG_STATE_CTZU,
        CFG_STATE_CTZR,
        CFG_STATE_CGEREP,
        CFG_STATE_APN,
        CFG_STATE_SELECT_APN,
        CFG_STATE_CIPCCFG,
        CFG_STATE_IPHEAD,
        CFG_STATE_IPSRIP,
        CFG_STATE_CGPSINFO,
        //
        CFG_STATE_OK,
        //
        CFG_STATE_WAIT_COMMAND_RESPONSE,
    } t_CFG_STATES;

    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Interface Functions                                               */
    /* ************************************************************************** */
    /* ************************************************************************** */
    void cfg_time(void);
    void cfg_init(void);
    t_CFG_STATES cfg_tasks(void);

#ifdef	__cplusplus
}
#endif

#endif	/* CFG_H */

