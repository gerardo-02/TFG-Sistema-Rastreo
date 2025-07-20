/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    filename.h

  @Summary
    Brief description of the file.

  @Description
    Describe the purpose of this file.
 */
/* ************************************************************************** */

#ifndef _SIMCOM_H    /* Guard against multiple inclusion */
#define _SIMCOM_H

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

#include "simcom/ipv4_helper.h"

/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif

    //Particular logging
#include "../log.h"
#define SIMCOM_MSG(f, ...)         _l(f, ##__VA_ARGS__)
#define SIMCOM_DBG(f, ...)         _d(f, ##__VA_ARGS__)
    //Particular pins for pwrkey, reset, status, etc...
#include "peripheral/gpio/plib_gpio.h"
#define SIMCOM_STATUS_ON()     (SIMCOM_STATUS_Get()==0)
#define SIMCOM_PWRKEY_HIGH()   SIMCOM_POWER_Clear()
#define SIMCOM_PWRKEY_LOW()    SIMCOM_POWER_Set()
#define SIMCOM_RESET_HIGH()    SIMCOM_RST_Clear()
#define SIMCOM_RESET_LOW()     SIMCOM_RST_Set()
    //Paricular UART
#include "peripheral/uart/plib_uart5.h"
#define SIMCOM_UART_RECEIVER_CALL_BACK(callback,context)   UART5_ReadCallbackRegister (callback,context);UART5_ReadThresholdSet(1);UART5_ReadNotificationEnable(true,true)
#define SIMCOM_UART_READ_COUNT_GET()                       UART5_ReadCountGet ()
#define SIMCOM_UART_READ(buffer,size)                      UART5_Read (buffer, size)
#define SIMCOM_UART_WRITE_COUNT_GET()                      UART5_WriteCountGet ()
#define SIMCOM_UART_WRITE_FREE()                           UART5_WriteFreeBufferCountGet()
#define SIMCOM_UART_WRITE(buffer,size)                     UART5_Write (buffer,size)
#define SIMCOM_UART_ERROR_GET()                            UART5_ErrorGet ()    

    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Constants                                                         */
    /* ************************************************************************** */
    /* ************************************************************************** */
#define SIMCOM_MAX_MESSAGE_LENGTH           1500    //Esto es una limitacion del moudulo de SimCom    
#define SIMCOM_CARD_ICCID_LENGTH            20
#define SIMCOM_MAX_APN_LENGTH               127
#define SIMCOM_MAX_TCP_SOCKETS              10
#define SIMCOM_MAX_TCP_SERVERS              4
#define SIMCOM_GPS_MODE_OFF                 0
#define SIMCOM_GPS_MODE_ON                  1
#define SIMCOM_GPS_MODE_ON_UNTIL_GET_POS    2

    // *****************************************************************************
    // *****************************************************************************
    // Section: Data Types
    // *****************************************************************************
    // *****************************************************************************

    //CONFIG

    typedef struct {
        uint8_t year;
        uint8_t mounth;
        uint8_t day;
        uint8_t dayWeek; //Sin uso
        uint8_t hour;
        uint8_t minute;
        uint8_t second;
        int8_t gmt;
    } t_SIMCOM_CLOCK_SYNC_CALL_BACK_CONTEXT;
    typedef void (*SIMCOM_CLOCK_SYNC_CALL_BACK)(t_SIMCOM_CLOCK_SYNC_CALL_BACK_CONTEXT*);

    typedef struct {
        char *message;
        uint32_t length;
        uint32_t socketIndex;
    } t_SIMCOM_RECEIVE_CALL_BACK_CONTEXT;
    typedef void (*SIMCOM_RECEIVE_CALL_BACK)(t_SIMCOM_RECEIVE_CALL_BACK_CONTEXT*);

    typedef struct {
        // DDM coordinates: Degrees, decimal minutes
        uint8_t latD; // Latitude degrees
        char latO; // Latitude orientation
        uint8_t logD; // Longitude degrees
        char logO; // Longitude orientation        
        float latDM; // Latitude decimal minutes    
        float logDM; // Longitude decimal minutes    
        // Date & UTC time
        char date[7]; //ddmmyy
        char utcTime[9]; //hhmmss.s
        // MSL Altitude. Unit is meters
        float alt;
        // Speed over ground. Unit is knots. (knots = nudos)
        float speed;
        // Course degrees
        uint16_t course; //?
    } t_SIMCOM_GPS_POSITION_INFO; // 19.2 AT+CGPSINFO Get GPS fixed position information
    typedef void(*SIMCOM_GPS_UPDATE_CALL_BACK)(t_SIMCOM_GPS_POSITION_INFO*);

    typedef struct {
        uint8_t gpsMode;
        char apn[SIMCOM_MAX_APN_LENGTH + 1];
        uint16_t serverPort[SIMCOM_MAX_TCP_SERVERS];
        SIMCOM_CLOCK_SYNC_CALL_BACK clockSyncCallBack;
        SIMCOM_RECEIVE_CALL_BACK receiveCallBack;
        SIMCOM_GPS_UPDATE_CALL_BACK gpsUpdateCallBack;
    } t_SIMCOM_CFG_DATA;

    //STATUS

    typedef enum {
        SIMCOM_SOCKET_STATE_CLOSE = 0,
        SIMCOM_SOCKET_STATE_OPEN,
        SIMCOM_SOCKET_STATE_FAIL,
    } t_SIMCOM_SOCKET_STATES;

    typedef struct {
        t_SIMCOM_SOCKET_STATES state;
        int8_t server_index;
    } t_SIMCOM_SOCKET;

    typedef enum {
        SIMCOM_SERVER_STATE_CLOSE = 0,
        SIMCOM_SERVER_STATE_LISTEN,
        SIMCOM_SERVER_STATE_FAIL,
    } t_SIMCOM_SERVER_STATES;

    typedef struct {
        t_SIMCOM_SERVER_STATES state;
        uint16_t port;
    } t_SIMCOM_SERVER;

    typedef struct {
        unsigned simCardReady : 1;
        unsigned iccid_updated : 1;
        unsigned rssi_updated : 1;
        unsigned networkRegistered : 1;
        unsigned netOpen : 1;
        unsigned localIpUpdated : 1;
        unsigned gpsSessionActive : 1;
        unsigned gpsUdapte : 1;
        unsigned : 24;
        char iccid[SIMCOM_CARD_ICCID_LENGTH + 1];
        int32_t rssi_dbm;
        t_SIMCOM_GPS_POSITION_INFO gpsInfo;
        uint32_t networkSystemMode;
        t_IPV4_ADDR localIp;
        t_SIMCOM_SOCKET socket[SIMCOM_MAX_TCP_SOCKETS];
        t_SIMCOM_SERVER server[SIMCOM_MAX_TCP_SERVERS];
    } t_SIMCOM_STATUS_DATA;

    //DATA

    typedef enum {
        SIMCOM_OPER_SUCCESS = 0,
        SIMCOM_OPER_ERROR,
    } t_SIMCOM_OPERATION_RESULT;

    typedef struct {
        t_SIMCOM_CFG_DATA cfg;
        t_SIMCOM_STATUS_DATA status;
    } t_SIMCOM_DATA;

    typedef struct {
        t_SIMCOM_OPERATION_RESULT op_res;
        void *local;
    } t_SIMCOM_SENT_CALLBACK_CTX;
    typedef void (*SIMCOM_SENT_CALLBACK)(t_SIMCOM_SENT_CALLBACK_CTX*);

    // *****************************************************************************
    // *****************************************************************************
    // Section: Interface Functions
    // *****************************************************************************
    // *****************************************************************************
    void simcom_init(void);
    void simcom_tasks(void);
    bool simcom_module_ready(void);
    void simcom_reset_module(void);
    void simcom_hard_reset_module(void);
    t_SIMCOM_STATUS_DATA simcom_get_status(void);
    int32_t simcom_call_back_register_clock_sync(SIMCOM_CLOCK_SYNC_CALL_BACK cb);
    int32_t simcom_set_apn(char *apn);
    int32_t simcom_setup_server(uint32_t index, uint16_t port);
    int32_t simcom_call_back_register_receive(SIMCOM_RECEIVE_CALL_BACK cb);
    int32_t simcom_response(uint8_t socketIndex, uint8_t *data, uint32_t dataLength, SIMCOM_SENT_CALLBACK cb);
    int32_t simcom_send(t_IPV4_ADDR ip, uint16_t port, uint8_t *data, uint32_t dataLength, SIMCOM_SENT_CALLBACK cb);
    int32_t simcom_set_gps_mode(uint8_t mode);
    int32_t simcom_call_back_register_gps_update(SIMCOM_GPS_UPDATE_CALL_BACK cb);

    /****/
    unsigned int simcom_dbgprint(char *buff, unsigned short bufflen);

    /* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _SIMCOM_H */

/* ************************************************************** End of File */
