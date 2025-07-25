/* 
 * File:   drv_mcp7940n.h
 * Author: JT
 *
 * Created on 10 de agosto de 2020, 10:13
 */

#ifndef DRV_MCP7940N_H
#define	DRV_MCP7940N_H

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include "time.h"

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
typedef enum
{
    MCP7940N_STATE_INIT = 0,
    /* IDLE */        
    MCP7940N_STATE_IDLE,
    /* REFRESH REGISTER */
    MCP7940N_STATE_REFRESH_DATA,
    MCP7940N_STATE_CHECK_ON,
    MCP7940N_STATE_TURN_ON,        
    /* UPDATE REGISTER */
    MCP7940N_STATE_REFRESH_FOR_UPDATE,              
    MCP7940N_STATE_UPDATING,             
    /* ERROR */
    MCP7940N_STATE_ERROR
}t_MCP7940N_STATES;

typedef enum
{
    MCP7940N_RES_SUCCESS = 0,
    MCP7940N_RES_NOT_READY,        
}t_MCP7940N_RESPONSES;
typedef struct
{
    t_MCP7940N_RESPONSES res;
    struct tm *ts;
}t_MCP7940N_CALL_BACK_CONTEXT;
typedef void(*DRV_MCP7940N_CALL_BACK)(t_MCP7940N_CALL_BACK_CONTEXT *ctx);

    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Interface Functions                                               */
    /* ************************************************************************** */
    /* ************************************************************************** */
int32_t drv_mcp7940n_initialize(void);
void drv_mcp7940n_tasks(void);

t_MCP7940N_STATES mcp7940n_get_state(void);
int32_t mcp7940n_get_ts(DRV_MCP7940N_CALL_BACK cb);
int32_t mcp7940n_set_ts(struct tm *ts);

#ifdef	__cplusplus
}
#endif

#endif	/* DRV_MCP7940N_H */

/* ************************************************************** End of File */
