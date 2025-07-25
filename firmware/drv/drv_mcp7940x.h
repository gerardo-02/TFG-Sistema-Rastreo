#ifndef DRV_MCP7940X_H
#define	DRV_MCP7940X_H

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <time.h>

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
    MCP7940X_STATE_INIT = 0,
    /* IDLE */        
    MCP7940X_STATE_IDLE,
    /* REFRESH REGISTER */
    MCP7940X_STATE_REFRESH_DATA,
    MCP7940X_STATE_CHECK_ON,
    MCP7940X_STATE_TURN_ON,        
    /* UPDATE REGISTER */
    MCP7940X_STATE_REFRESH_FOR_UPDATE,              
    MCP7940X_STATE_UPDATING,             
    /* ERROR */
    MCP7940X_STATE_ERROR
}t_MCP7940X_STATES;

typedef enum
{
    MCP7940X_RES_SUCCESS = 0,
    MCP7940X_RES_NOT_READY,        
}t_MCP7940X_RESPONSES;
typedef struct
{
    t_MCP7940X_RESPONSES res;
    time_t ts;
    bool reliable;
}t_MCP7940X_CALL_BACK_CONTEXT;
typedef void(*DRV_MCP7940X_CALL_BACK)(t_MCP7940X_CALL_BACK_CONTEXT *ctx);

    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Interface Functions                                               */
    /* ************************************************************************** */
    /* ************************************************************************** */
int32_t drv_mcp7940x_init(void);
void drv_mcp7940x_tasks(void);

t_MCP7940X_STATES mcp7940x_get_state(void);
int32_t mcp7940x_get_ts(DRV_MCP7940X_CALL_BACK cb);
int32_t mcp7940x_set_ts(struct tm *ts);

#ifdef	__cplusplus
}
#endif

#endif	/* DRV_MCP7940X_H */

/* ************************************************************** End of File */
