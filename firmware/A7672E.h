/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    simcom.h

  @Summary
    Comunicación con el módulo SIMCOM A7672 mediante la stack USB.

  @Description
    Comunicación con el módulo SIMCOM A7672 mediante la stack USB.
 */
/* ************************************************************************** */

#ifndef _SIMCOM_H    /* Guard against multiple inclusion */
#define _SIMCOM_H


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

#include <stdint.h>
#include <stdbool.h>

#include "usb/usb_host_client_driver.h"

/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif


    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Constants                                                         */
    /* ************************************************************************** */
    /* ************************************************************************** */

#define AT_DISABLE_ECO              "ATE0\r"
#define AT_PRODUCT_INFORMATION      "ATI\r"
#define AT_SET_FULL_FUNCTIONALITY   "AT+CFUN=1\r"
#define AT_SIM_PIN_STATUS           "AT+CPIN?\r"
#define AT_MOBILE_NETWORK_STATUS    "AT+CREG?\r"
#define AT_GPRS_NETWORK_STATUS      "AT+CGREG?\r"
#define AT_EPS_NETWORK_STATUS       "AT+CEREG?\r"
#define AT_DEFINE_PDP_CONTEXT       "AT+CGDCONT=1,\"IP\",\"orangeworld\"\r"
#define AT_ACTIVATE_PDP_CONTEXT     "AT+CGACT=1,1\r"
#define AT_VERIFY_PDP_ADDRESS       "AT+CGPADDR=1\r"
#define AT_ENABLE_GNSS              "AT+CGNSSPWR=1\r"
#define AT_SET_GPS_MODE             "AT+CGNSSMODE=1\r"
#define AT_COLD_START_GPS           "AT+CGPSCOLD\r"
#define AT_WARM_START_GPS           "AT+CGPSWARM\r"
#define AT_HOT_START_GPS            "AT+CGPSHOT\r"
#define AT_START_GPS_TEST_MODE      "AT+CGPSFTM=1\r"
#define AT_GET_GPS_INFO             "AT+CGPSINFO\r"
#define AT_OPERATOR_LIST            "AT+COPS=?\r"
#define AT_START_SSL_SERVICE        "AT+CCHSTART\r"
#define AT_OPEN_SERVER_CONNECTION   "AT+CCHOPEN=0,\"simcomtest.serveo.net\",5000,1\r"
#define AT_HTTPS_INIT               "AT+HTTPINIT\r"
#define AT_HTTPS_SET_URL            "AT+HTTPPARA=\"URL\",\"http://simcomtest.serveo.net/posicion\"\r"
//#define AT_HTTPS_SET_URL            "AT+HTTPPARA=\"URL\",\"https://postman-echo.com/get\"\r"
#define AT_HTTP_SET_JSON_FORMAT     "AT+HTTPPARA=\"CONTENT\",\"application/json\"\r"
#define AT_HTTP_SEND_GET_REQUEST    "AT+HTTPACTION=0\r"
#define AT_HTTP_SEND_POST_REQUEST   "AT+HTTPACTION=1\r"
#define AT_HTTP_READ_HEADER         "AT+HTTPHEAD\r"
#define AT_HTTP_READ_RESPONSE       "AT+HTTPREAD=0,500\r"
#define AT_HTTP_INPUT_POST_DATA     "AT+HTTPDATA="       // Ojo, esta cadena debe ser completada
#define AT_HTTP_STOP_SERVICE        "AT+HTTPTERM\r"

    /* ************************************************************************** */
    /** Descriptive Constant Name

      @Summary
        Brief one-line summary of the constant.
    
      @Description
        Full description, explaining the purpose and usage of the constant.
        <p>
        Additional description in consecutive paragraphs separated by HTML 
        paragraph breaks, as necessary.
        <p>
        Type "JavaDoc" in the "How Do I?" IDE toolbar for more information on tags.
    
      @Remarks
        Any additional remarks
     */
#define EXAMPLE_CONSTANT 0


    // *****************************************************************************
    // *****************************************************************************
    // Section: Data Types
    // *****************************************************************************
    // *****************************************************************************
    
    typedef struct{
        int32_t lat; // Formato ddmm.mmmm multiplicado * 10000 para evitar decimales
        int32_t lon; // Idem. Formato dddmm.mmmm
        uint16_t course; 
        uint8_t speed; //1 decimal. En metros por segundo
        
    }SIMCOM_LAST_GPS_POSSITION;
    
    typedef struct{
        bool ATread;
        bool ATwrite;
        bool ATserial;
        
        bool RNDISread;
        bool RNDISwrite;
        bool RNDISserial;
        
        bool ECMread;
        bool ECMwrite;
        bool ECMserial;
        
        bool DIAGread;
        bool DIAGwrite;
    }SIMCOM_TRANSFER_FLAGS;
    
    /** SIMCOM_STATE

      @Summary
        Estado del módulo de comunicaciones SIMCOM A7672.
    
      @Description
        Estado del módulo de comunicaciones SIMCOM A7672.
     */
    typedef enum{
        SIMCOM_STATE_ERROR = -1,
        SIMCOM_STATE_WAITING_DRIVER_READY = 0,
        SIMCOM_STATE_NOT_READY,
        SIMCOM_STATE_DISABLING_ECO,
        SIMCOM_STATE_SETTING_FULL_FUNCTIONALITY,
        SIMCOM_STATE_GETTING_SIM_PIN_STATUS,
        SIMCOM_STATE_GETTING_MOBILE_NETWORK_STATUS,
        SIMCOM_STATE_GETTING_GPRS_NETWORK_STATUS,
        SIMCOM_STATE_GETTING_EPS_NETWORK_STATUS,
        SIMCOM_STATE_DEFINING_PDP_CONTEXT,
        SIMCOM_STATE_ACTIVATING_PDP_CONTEXT,
        SIMCOM_STATE_VERIFY_PDP_ADDRESS,
        SIMCOM_STATE_ENABLING_GNSS,
        SIMCOM_STATE_SET_GPS_MODE,
        SIMCOM_STATE_COLD_START_GPS,
        SIMCOM_STATE_START_GPS_TEST_MODE,
        SIMCOM_STATE_HTTPS_INIT,
        SIMCOM_STATE_HTTPS_SET_URL,
        SIMCOM_STATE_HTTPS_SET_JSON_FORMAT,
        SIMCOM_STATE_REQUEST_GPS_INFO,
        SIMCOM_STATE_HTTP_POST_DATA,
        SIMCOM_STATE_HTTP_SEND_GET_REQUEST,
        SIMCOM_STATE_HTTP_WAIT_GET_RESPONSE,
        SIMCOM_STATE_HTTP_SEND_POST_REQUEST,
        SIMCOM_STATE_HTTP_WAIT_POST_RESPONSE,
        SIMCOM_STATE_HTTP_READ_HEADER,
        SIMCOM_STATE_HTTP_READ_RESPONSE,
        SIMCOM_STATE_HTTP_STOP_SERVICE,
        SIMCOM_STATE_READY

    } SIMCOM_STATE;
    
     /** Descriptive Data Type Name

      @Summary
        Brief one-line summary of the data type.
    
      @Description
        Full description, explaining the purpose and usage of the data type.
     */
    
    typedef struct{
        SIMCOM_TRANSFER_FLAGS flags;
        
        SIMCOM_LAST_GPS_POSSITION GPSInfo;
        
        uint32_t assignedIP;
        
        SIMCOM_STATE state;
    }SIMCOM_OBJ;
    
    extern SIMCOM_OBJ simcomObj;

    /** Descriptive Data Type Name

      @Summary
        Brief one-line summary of the data type.
    
      @Description
        Full description, explaining the purpose and usage of the data type.
     */


    // *****************************************************************************
    // *****************************************************************************
    // Section: Interface Functions
    // *****************************************************************************
    // *****************************************************************************

    // *****************************************************************************

    int simcom_init(void);
    
    int simcom_tasks(void);


    /* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _EXAMPLE_FILE_NAME_H */

/* *****************************************************************************
 End of File
 */
