/* 
 * File:   com.h
 * Author: jtrodriguez
 *
 * Created on 24 de febrero de 2021, 12:01
 */

#ifndef COM_H
#define	COM_H

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
#define COMMAND_ACEPTED             0
#define COMMAND_BUSY                -1 
#define COMMAND_RES_ENTER_MSG       1
#define COMMAND_RES_OK              0
#define COMMAND_RES_ERROR           -1
#define COMMAND_RES_CME_CMS_ERROR   -2   
#define COMMAND_RES_TIMEOUT         -4    

    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Data types & Definitions                                          */
    /* ************************************************************************** */

    /* ************************************************************************** */
    typedef enum {
        COM_STATE_INIT = 0,
        COM_STATE_WAIT,
        COM_STATE_READY,
    } t_COM_STATES;

    typedef struct {
        int32_t res;
        int32_t err;
        void *local;
    } t_COM_COMMAND_CALLBACK_CTX;
    typedef void (*COM_COMMAND_CALLBACK)(t_COM_COMMAND_CALLBACK_CTX *ctx);

    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Interface Functions                                               */
    /* ************************************************************************** */
    /* ************************************************************************** */
    void com_time(void);
    void com_register_uart_call_backs(void); //Solo inicializa lo esencial para que no se bloquee el canal de comunicacion (UART) entre modulo y uC
    void com_init(void);
    t_COM_STATES com_tasks(void);
    int32_t com_command(char *command, uint32_t commandLength, COM_COMMAND_CALLBACK cb, void *localCtx);
    int32_t com_write_message(uint8_t *msg, uint32_t msgLength, COM_COMMAND_CALLBACK cb, void *localCtx);

#ifdef	__cplusplus
}
#endif

#endif	/* COM_H */

