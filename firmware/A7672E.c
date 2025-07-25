/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    simcom.c

  @Summary
    Comunicación con el módulo SIMCOM A7672 mediante la stack USB.

  @Description
    Comunicación con el módulo SIMCOM A7672 mediante la stack USB.
 */
/* ************************************************************************** */

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <stdio.h>
#include <string.h>


#include "A7672E.h"
#include "usb_A7672E.h"

#include "log.h"
#include "config/default/system/time/sys_time.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */

#define CLEAR_BUFF(buff)  memset((buff), 0, sizeof(buff))

#define TIMER_COMPLETE(timer)  timer == 1U
#define TIMER_DISABLE(timer)    atomicSetTimer(timer, 0)
#define TIMER_SET(timer, time)  timer = time

/** Descriptive Data Item Name

  @Summary
    Brief one-line summary of the data item.
    
  @Description
    Full description, explaining the purpose and usage of data item.
    <p>
    Additional description in consecutive paragraphs separated by HTML 
    paragraph breaks, as necessary.
    <p>
    Type "JavaDoc" in the "How Do I?" IDE toolbar for more information on tags.
    
  @Remarks
    Any additional remarks
 * */

volatile static uint32_t timer;
SYS_TIME_HANDLE resendATCommandTimerHandle;

bool pendingATCommandParse;
bool pendingResponse;

bool resendATCommand;

SIMCOM_OBJ simcomObj;

/* ************************************************************************** */
/* ************************************************************************** */
// Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */

//static void atomicSetTimer(uint32_t * timer, uint32_t time){
//    uint8_t intStatus = __builtin_disable_interrupts();  // Guarda y desactiva interrupciones
//    *timer = time;
//    if (intStatus) __builtin_enable_interrupts();
//}

static uint32_t IPStringToInt(char* ip, size_t size){
    uint8_t b0, b1, b2, b3 = 0;
    char* num;
    
    if (size < 7 || size > 15){
        return 0;
    }
    
    num = strtok(ip, ".");
    b0 = atoi(num);
    num = strtok(NULL, ".");
    b1 = atoi(num);
    num = strtok(NULL, ".");
    b2 = atoi(num);
    num = strtok(NULL, "\r");
    b3 = atoi(num);
//    _l("\r\nIP string: %s\t IP int: %d.%d.%d.%d\r\n", ip, b0, b1, b2, b3);
    return b0 | b1 | b2 | b3;
}

static void IPIntToString(uint32_t ip, char* ip_st){
    uint8_t b0, b1, b2, b3;
    
    b0 = (ip >> 24) & 0xFF;
    b1 = (ip >> 16) & 0xFF;
    b2 = (ip >> 8) & 0xFF;
    b3 = ip & 0xFF;
    sprintf(ip_st, "%d.%d.%d.%d", b0, b1, b2, b3);
}

static void updateGPSInfo(char *info){
    char* tok;
    tok = strtok(info, ":"); // +GPSINFO:
    tok = strtok(NULL, ".");
    simcomObj.GPSInfo.lat = (atoi(tok) * 10000); //Latitud
    tok = strtok(NULL, ",");
    simcomObj.GPSInfo.lat += (atoi(tok) / 100);
    tok = strtok(NULL, ",");
    if (tok[0] == 'S') simcomObj.GPSInfo.lat *= -1;
    
    tok = strtok(NULL, ".");
    simcomObj.GPSInfo.lon = (atoi(tok) * 10000);
    tok = strtok(NULL, ",");
    simcomObj.GPSInfo.lon += (atoi(tok) / 100);  // Longitud
    tok = strtok(NULL, ",");
    if (tok[0] == 'W') simcomObj.GPSInfo.lon *= -1;
    
    tok = strtok(NULL, ",");
    tok = strtok(NULL, ",");
    tok = strtok(NULL, ",");
    tok = strtok(NULL, ".");
    
    simcomObj.GPSInfo.speed = (atoi(tok) * 10);
    tok = strtok(NULL, ",");
    simcomObj.GPSInfo.speed += atoi(tok);
    
    tok = strtok(NULL, ",");
    
    simcomObj.GPSInfo.course = atoi(tok);
}

static void GPSInfoToString(char* info){
    sprintf(info, "{\"lat\":%d.%04d,\"lon\":%d.%04d,\"vel\":%d.%01d,\"dir\":%d}", 
            (simcomObj.GPSInfo.lat / 1000000), (simcomObj.GPSInfo.lat % 1000000),
            (simcomObj.GPSInfo.lon / 1000000), (simcomObj.GPSInfo.lon % 1000000),
            (simcomObj.GPSInfo.speed / 10), (simcomObj.GPSInfo.speed % 10),
            simcomObj.GPSInfo.course);
}

USB_HOST_SIMCOM_EVENT_RESPONSE simcomEventHandler(
    USB_HOST_SIMCOM_EVENT event,
    void * eventData,
    uintptr_t context){
    
    USB_HOST_SIMCOM_TRANSFER_COMPLETE_DATA data = *(USB_HOST_SIMCOM_TRANSFER_COMPLETE_DATA *)eventData;
    
    switch (event){
        case USB_HOST_SIMCOM_EVENT_AT_READ_COMPLETE:
//            _l("SE HAN LEIDO %d BYTES\r\n", data.length);
            pendingATCommandParse = true;
            simcomObj.flags.ATread = true;
            break;
        case USB_HOST_SIMCOM_EVENT_AT_WRITE_COMPLETE:
            pendingResponse = true;
            simcomObj.flags.ATwrite = true;
            break;
        case USB_HOST_SIMCOM_EVENT_AT_SERIAL_STATE_NOTIFICATION_RECEIVED:
            simcomObj.flags.ATserial = true;
            break;
        case USB_HOST_SIMCOM_EVENT_RNDIS_READ_COMPLETE:
            simcomObj.flags.RNDISread = true;
            break;
        case USB_HOST_SIMCOM_EVENT_RNDIS_WRITE_COMPLETE:
            simcomObj.flags.RNDISwrite = true;
            break;
        case USB_HOST_SIMCOM_EVENT_RNDIS_SERIAL_STATE_NOTIFICATION_RECEIVED:
            simcomObj.flags.RNDISserial = true;
            break;
        case USB_HOST_SIMCOM_EVENT_ECM_READ_COMPLETE:
            simcomObj.flags.ECMread = true;
            break;
        case USB_HOST_SIMCOM_EVENT_ECM_WRITE_COMPLETE:
            simcomObj.flags.ECMwrite = true;
            break;
        case USB_HOST_SIMCOM_EVENT_ECM_SERIAL_STATE_NOTIFICATION_RECEIVED:
            simcomObj.flags.ECMserial = true;
            break;
        case USB_HOST_SIMCOM_EVENT_DIAG_READ_COMPLETE:
            simcomObj.flags.DIAGread = true;
            break;
        case USB_HOST_SIMCOM_EVENT_DIAG_WRITE_COMPLETE:
            simcomObj.flags.DIAGwrite = true;
            break;
        case USB_HOST_SIMCOM_EVENT_DEVICE_DETACHED:
            break;
        default:
            break;
    }
    
    return (USB_HOST_SIMCOM_EVENT_RESPONE_NONE);
}

void resendATCommandCB(uintptr_t context){
    if (timer > 1U) timer--;
}

bool ATCommandParse(char * command, uint32_t size){
    
    bool result = true;
    uint32_t i = 0;
    
    if(command == NULL || command[0] == 0x00){
        result = false;
    }
    else{
        
//        _l("%s", command);
//        _dump(command, 32, DUMP_MODE_MIX);
        
        while (command[i] != 0x00 && i < size)  i++; // Encuentro el final del comando
        
        if (command[0] != 0x0D || command[1] != 0x0A || command[i-2] != 0x0D || command[i-1] !=0x0A){
            result = false;
        }
    }
    
    return result;
}



/* ************************************************************************** */
/* ************************************************************************** */
// Section: Interface Functions                                               */
/* ************************************************************************** */
/* ************************************************************************** */

// *****************************************************************************

/** 
  @Function
    int ExampleInterfaceFunctionName ( int param1, int param2 ) 

  @Summary
    Brief one-line description of the function.

  @Remarks
    Refer to the example_file.h interface header for function usage details.
 */
int simcom_init(void) {
    resendATCommandTimerHandle = SYS_TIME_TimerCreate(0, SYS_TIME_MSToCount(1), resendATCommandCB, (uintptr_t) 0, SYS_TIME_PERIODIC);
    resendATCommand = false;
    pendingATCommandParse = false;
    pendingResponse = false;
    
    simcomObj.flags.ATread = true;  // Flags a true para indicar que se puede hacer cualquier operación
    simcomObj.flags.ATwrite = true;
    simcomObj.flags.ATserial = true;
    simcomObj.flags.RNDISread = true;  
    simcomObj.flags.RNDISwrite = true;
    simcomObj.flags.RNDISserial = true;
    simcomObj.flags.ECMread = true;  
    simcomObj.flags.ECMwrite = true;
    simcomObj.flags.ECMserial = true;
    simcomObj.flags.DIAGread = true;  
    simcomObj.flags.DIAGwrite = true;
    
    simcomObj.assignedIP = 0;
    
    simcomObj.GPSInfo.lat = 0;
    simcomObj.GPSInfo.lon = 0;
    simcomObj.GPSInfo.speed = 0;
    simcomObj.GPSInfo.course = 0;
    
    simcomObj.state = SIMCOM_STATE_WAITING_DRIVER_READY;
    return 0;
}

int simcom_tasks(void) {
    
    USB_HOST_SIMCOM_TRANSFER_HANDLE readTransferHandle;
    USB_HOST_SIMCOM_TRANSFER_HANDLE writeTransferHandle; 

    static char buffRecv[512];
    static char postData[64];
    char postCmd[20];
    char* tok;
    
    switch(simcomObj.state){
        
        case SIMCOM_STATE_WAITING_DRIVER_READY:
            if (USB_HOST_SIMCOM_IsInReadyState()){
                USB_HOST_SIMCOM_EventHandlerSet(simcomEventHandler, 0); // Cambiar luego a primer estado de la maquina
                SYS_TIME_TimerStart(resendATCommandTimerHandle);
                CLEAR_BUFF(buffRecv);
                simcomObj.state = SIMCOM_STATE_NOT_READY;
            }
            break;
            
        case SIMCOM_STATE_NOT_READY: 
            
            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATread = false;
            }
            
            if (pendingATCommandParse){
                if (ATCommandParse(buffRecv, sizeof(buffRecv)) && strcmp(buffRecv, "\r\n*ATREADY: 1\r\n") == 0){
                    TIMER_SET(timer, 15000);  // Desde que se recibe este mensaje, escucharemos durante 15 segundos antes de empezar a mandar comandos
                }
                CLEAR_BUFF(buffRecv);
                pendingATCommandParse = false;
            }
            
            if (TIMER_COMPLETE(timer)){
                simcomObj.state = SIMCOM_STATE_DISABLING_ECO;
            }
            
            break;
            
        case SIMCOM_STATE_DISABLING_ECO:
            
            if (simcomObj.flags.ATwrite && !pendingResponse && USB_HOST_SIMCOM_AT_Write(&writeTransferHandle, AT_DISABLE_ECO, strlen(AT_DISABLE_ECO)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATwrite = false;
            }
            
            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATread = false;
            }
            
            if (pendingATCommandParse){
                if (ATCommandParse(buffRecv, sizeof(buffRecv))){
                    if (strcmp(buffRecv, "\r\nOK\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_SETTING_FULL_FUNCTIONALITY;
                    }
                    else if(strcmp(buffRecv, "\r\nERROR\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_ERROR;
                    }
                    
                }
                CLEAR_BUFF(buffRecv);
                pendingATCommandParse = false;
            }
            
            break;
            
        case SIMCOM_STATE_SETTING_FULL_FUNCTIONALITY:
            
            if (simcomObj.flags.ATwrite && !pendingResponse && USB_HOST_SIMCOM_AT_Write(&writeTransferHandle, AT_SET_FULL_FUNCTIONALITY, strlen(AT_SET_FULL_FUNCTIONALITY)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATwrite = false;
            }
            
            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATread = false;
            }
            
            if (pendingATCommandParse){
                if (ATCommandParse(buffRecv, sizeof(buffRecv))){
                    if (strcmp(buffRecv, "\r\nOK\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_GETTING_SIM_PIN_STATUS;
                    }
                    else if(strcmp(buffRecv, "\r\nERROR\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_ERROR;
                    }
                    
                }
                CLEAR_BUFF(buffRecv);
                pendingATCommandParse = false;
            }
            
            break;
            
        case SIMCOM_STATE_GETTING_SIM_PIN_STATUS:
            
            if (simcomObj.flags.ATwrite && !pendingResponse && USB_HOST_SIMCOM_AT_Write(&writeTransferHandle, AT_SIM_PIN_STATUS, strlen(AT_SIM_PIN_STATUS)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATwrite = false;
            }
            
            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATread = false;
            }
            
            if (pendingATCommandParse){
                if (ATCommandParse(buffRecv, sizeof(buffRecv))){
                    if (strcmp(buffRecv, "\r\nOK\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_GETTING_MOBILE_NETWORK_STATUS;
                    }
                    else if(strcmp(buffRecv, "\r\nERROR\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_ERROR;
                    }
                    
                }
                CLEAR_BUFF(buffRecv);
                pendingATCommandParse = false;
            }
            
            break;
            
        case SIMCOM_STATE_GETTING_MOBILE_NETWORK_STATUS:
            
            if (simcomObj.flags.ATwrite && !pendingResponse && USB_HOST_SIMCOM_AT_Write(&writeTransferHandle, AT_MOBILE_NETWORK_STATUS, strlen(AT_MOBILE_NETWORK_STATUS)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATwrite = false;
            }
            
            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATread = false;
            }
            
            if (pendingATCommandParse){
                if (ATCommandParse(buffRecv, sizeof(buffRecv))){
                    if (strcmp(buffRecv, "\r\nOK\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_GETTING_GPRS_NETWORK_STATUS;
                    }
                    else if(strcmp(buffRecv, "\r\nERROR\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_ERROR;
                    }
                    
                }
                CLEAR_BUFF(buffRecv);
                pendingATCommandParse = false;
            }
            
            break;
            
        case SIMCOM_STATE_GETTING_GPRS_NETWORK_STATUS:
            if (simcomObj.flags.ATwrite && !pendingResponse && USB_HOST_SIMCOM_AT_Write(&writeTransferHandle, AT_GPRS_NETWORK_STATUS, strlen(AT_GPRS_NETWORK_STATUS)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATwrite = false;
            }
            
            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATread = false;
            }
            
            if (pendingATCommandParse){
                if (ATCommandParse(buffRecv, sizeof(buffRecv))){
                    if (strcmp(buffRecv, "\r\nOK\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_GETTING_EPS_NETWORK_STATUS;
                    }
                    else if(strcmp(buffRecv, "\r\nERROR\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_ERROR;
                    }
                    
                }
                CLEAR_BUFF(buffRecv);
                pendingATCommandParse = false;
            }
            break;
            
        case SIMCOM_STATE_GETTING_EPS_NETWORK_STATUS:
            if (simcomObj.flags.ATwrite && !pendingResponse && USB_HOST_SIMCOM_AT_Write(&writeTransferHandle, AT_EPS_NETWORK_STATUS, strlen(AT_EPS_NETWORK_STATUS)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATwrite = false;
            }
            
            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATread = false;
            }
            
            if (pendingATCommandParse){
                if (ATCommandParse(buffRecv, sizeof(buffRecv))){
                    if (strcmp(buffRecv, "\r\nOK\r\n") == 0 || strcmp(buffRecv, "\r\nERROR\r\n") == 0){  // Estar conectado a la red EPS no es estrictamente necesario
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_DEFINING_PDP_CONTEXT;
                    }
                }
                CLEAR_BUFF(buffRecv);
                pendingATCommandParse = false;
            }
            break;
            
        case SIMCOM_STATE_DEFINING_PDP_CONTEXT:
            if (simcomObj.flags.ATwrite && !pendingResponse && USB_HOST_SIMCOM_AT_Write(&writeTransferHandle, AT_DEFINE_PDP_CONTEXT, strlen(AT_DEFINE_PDP_CONTEXT)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATwrite = false;
            }
            
            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATread = false;
            }
            
            if (pendingATCommandParse){
                if (ATCommandParse(buffRecv, sizeof(buffRecv))){
                    if (strcmp(buffRecv, "\r\nOK\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_ACTIVATING_PDP_CONTEXT;
                    }
                    else if(strcmp(buffRecv, "\r\nERROR\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_ERROR;
                    }
                    
                }
                CLEAR_BUFF(buffRecv);
                pendingATCommandParse = false;
            }
            break;
            
        case SIMCOM_STATE_ACTIVATING_PDP_CONTEXT:
            if (simcomObj.flags.ATwrite && !pendingResponse && USB_HOST_SIMCOM_AT_Write(&writeTransferHandle, AT_ACTIVATE_PDP_CONTEXT, strlen(AT_ACTIVATE_PDP_CONTEXT)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATwrite = false;
            }
            
            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATread = false;
            }
            
            if (pendingATCommandParse){
                if (ATCommandParse(buffRecv, sizeof(buffRecv))){
                    if (strcmp(buffRecv, "\r\nOK\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_VERIFY_PDP_ADDRESS;
                    }
                    else if(strcmp(buffRecv, "\r\nERROR\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_ERROR;
                    }
                    
                }
                CLEAR_BUFF(buffRecv);
                pendingATCommandParse = false;
            }
            break;
            
        case SIMCOM_STATE_VERIFY_PDP_ADDRESS:
            if (simcomObj.flags.ATwrite && !pendingResponse && USB_HOST_SIMCOM_AT_Write(&writeTransferHandle, AT_VERIFY_PDP_ADDRESS, strlen(AT_VERIFY_PDP_ADDRESS)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATwrite = false;
            }
            
            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATread = false;
            }
            
            if (pendingATCommandParse){
                if (ATCommandParse(buffRecv, sizeof(buffRecv))){
                    if (strcmp(buffRecv, "\r\nOK\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_HTTPS_INIT;
                    }
                    else if(strncmp(buffRecv, "\r\n+CGPADDR:", 11) == 0){
                        tok = strtok(buffRecv, ",");
                        tok = strtok(NULL, ","); // 2 veces para obtener el final de la cadena
                        simcomObj.assignedIP = IPStringToInt(tok, strlen(tok) - 2); //-2 porque \r\n no deben ser contados por la función
                    }
                    else if(strcmp(buffRecv, "\r\nERROR\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_ERROR;
                    }
                    
                }
                CLEAR_BUFF(buffRecv);
                pendingATCommandParse = false;
            }
            break;
            
//        case SIMCOM_STATE_ENABLING_GNSS:
//            if (simcomObj.flags.ATwrite && !pendingResponse && USB_HOST_SIMCOM_AT_Write(&writeTransferHandle, AT_ENABLE_GNSS, strlen(AT_ENABLE_GNSS)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
//                simcomObj.flags.ATwrite = false;
//            }
//            
//            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
//                simcomObj.flags.ATread = false;
//            }
//            
//            if (pendingATCommandParse){
//                if (ATCommandParse(buffRecv, sizeof(buffRecv))){
//                    if (strcmp(buffRecv, "\r\n+CGNSSPWR: READY!\r\n") == 0){
//                        pendingResponse = false;
//                        simcomObj.state = SIMCOM_STATE_SET_GPS_MODE;
//                    }
//                    else if(strcmp(buffRecv, "\r\nERROR\r\n") == 0){
//                        pendingResponse = false;
//                        simcomObj.state = SIMCOM_STATE_ERROR;
//                    }
//                    
//                }
//                CLEAR_BUFF(buffRecv);
//                pendingATCommandParse = false;
//            }
//            break;
//            
//        case SIMCOM_STATE_SET_GPS_MODE:
//            if (simcomObj.flags.ATwrite && !pendingResponse && USB_HOST_SIMCOM_AT_Write(&writeTransferHandle, AT_SET_GPS_MODE, strlen(AT_SET_GPS_MODE)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
//                simcomObj.flags.ATwrite = false;
//            }
//            
//            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
//                simcomObj.flags.ATread = false;
//            }
//            
//            if (pendingATCommandParse){
//                if (ATCommandParse(buffRecv, sizeof(buffRecv))){
//                    if (strcmp(buffRecv, "\r\nOK\r\n") == 0){
//                        pendingResponse = false;
//                        simcomObj.state = SIMCOM_STATE_COLD_START_GPS;
//                    }
//                    else if(strcmp(buffRecv, "\r\nERROR\r\n") == 0){
//                        pendingResponse = false;
//                        simcomObj.state = SIMCOM_STATE_ERROR;
//                    }
//                    
//                }
//                CLEAR_BUFF(buffRecv);
//                pendingATCommandParse = false;
//            }
//            break;
//            
//        case SIMCOM_STATE_COLD_START_GPS:
//            if (simcomObj.flags.ATwrite && !pendingResponse && USB_HOST_SIMCOM_AT_Write(&writeTransferHandle, AT_COLD_START_GPS, strlen(AT_COLD_START_GPS)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
//                simcomObj.flags.ATwrite = false;
//            }
//            
//            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
//                simcomObj.flags.ATread = false;
//            }
//            
//            if (pendingATCommandParse){
//                if (ATCommandParse(buffRecv, sizeof(buffRecv))){
//                    if (strcmp(buffRecv, "\r\nOK\r\n") == 0){
//                        pendingResponse = false;
//                        simcomObj.state = SIMCOM_STATE_START_GPS_TEST_MODE;
//                    }
//                    else if(strcmp(buffRecv, "\r\nERROR\r\n") == 0){
//                        pendingResponse = false;
//                        simcomObj.state = SIMCOM_STATE_ERROR;
//                    }
//                    
//                }
//                CLEAR_BUFF(buffRecv);
//                pendingATCommandParse = false;
//            }
//            break;
//            
//        case SIMCOM_STATE_START_GPS_TEST_MODE:
//            if (simcomObj.flags.ATwrite && !pendingResponse && USB_HOST_SIMCOM_AT_Write(&writeTransferHandle, AT_START_GPS_TEST_MODE, strlen(AT_START_GPS_TEST_MODE)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
//                simcomObj.flags.ATwrite = false;
//            }
//            
//            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
//                simcomObj.flags.ATread = false;
//            }
//            
//            if (pendingATCommandParse){
//                if (ATCommandParse(buffRecv, sizeof(buffRecv))){
//                    if (strcmp(buffRecv, "\r\nOK\r\n") == 0){
////                        pendingResponse = false;
////                        simcomObj.state = SIMCOM_STATE_READY;
//                    }
//                    else if(strcmp(buffRecv, "\r\nERROR\r\n") == 0){
//                        pendingResponse = false;
//                        simcomObj.state = SIMCOM_STATE_ERROR;
//                    }
//                    
//                }
//                CLEAR_BUFF(buffRecv);
//                pendingATCommandParse = false;
//            }
//            break;
            
        case SIMCOM_STATE_HTTPS_INIT:
            
            if (simcomObj.flags.ATwrite && !pendingResponse && USB_HOST_SIMCOM_AT_Write(&writeTransferHandle, AT_HTTPS_INIT, strlen(AT_HTTPS_INIT)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATwrite = false;
            }
            
            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATread = false;
            }
            
            if (pendingATCommandParse){
                if (ATCommandParse(buffRecv, sizeof(buffRecv))){
                    if (strcmp(buffRecv, "\r\nOK\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_HTTPS_SET_URL;
                    }
                    else if(strcmp(buffRecv, "\r\nERROR\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_ERROR;
                    }
                    
                }
                CLEAR_BUFF(buffRecv);
                pendingATCommandParse = false;
            }
            break;
            
        case SIMCOM_STATE_HTTPS_SET_URL:
            
            if (simcomObj.flags.ATwrite && !pendingResponse && USB_HOST_SIMCOM_AT_Write(&writeTransferHandle, AT_HTTPS_SET_URL, strlen(AT_HTTPS_SET_URL)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATwrite = false;
            }
            
            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATread = false;
            }
            
            if (pendingATCommandParse){
                if (ATCommandParse(buffRecv, sizeof(buffRecv))){
                    if (strcmp(buffRecv, "\r\nOK\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_HTTPS_SET_JSON_FORMAT;
                    }
                    else if(strcmp(buffRecv, "\r\nERROR\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_ERROR;
                    }
                    
                }
                CLEAR_BUFF(buffRecv);
                pendingATCommandParse = false;
            }
            break;
            
        case SIMCOM_STATE_HTTPS_SET_JSON_FORMAT:
            
            if (simcomObj.flags.ATwrite && !pendingResponse && USB_HOST_SIMCOM_AT_Write(&writeTransferHandle, AT_HTTP_SET_JSON_FORMAT, strlen(AT_HTTP_SET_JSON_FORMAT)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATwrite = false;
            }
            
            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATread = false;
            }
            
            if (pendingATCommandParse){
                if (ATCommandParse(buffRecv, sizeof(buffRecv))){
                    if (strcmp(buffRecv, "\r\nOK\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_REQUEST_GPS_INFO;
                    }
                    else if(strcmp(buffRecv, "\r\nERROR\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_ERROR;
                    }
                    
                }
                CLEAR_BUFF(buffRecv);
                pendingATCommandParse = false;
            }
            break;
            
        case SIMCOM_STATE_REQUEST_GPS_INFO:
            
//          No enviamos nada porque este es un modulo A7672E sin GPS
//            if (simcomObj.flags.ATwrite && !pendingResponse && USB_HOST_SIMCOM_AT_Write(&writeTransferHandle, AT_GET_GPS_INFO, strlen(AT_GET_GPS_INFO)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
//                simcomObj.flags.ATwrite = false;
//            }

//          Logicamente, tampoco hay nada que leer            
//            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
//                simcomObj.flags.ATread = false;
//            }
            
            CLEAR_BUFF(postData);
//            sprintf(buffRecv, "\r\n+CGPSINFO:3113.343286,N,12121.234064,E,250311,072809.3,44.1,15.3,145\r\n");
            sprintf(buffRecv, "\r\n+CGPSINFO:3113.%d86,N,12121.%d64,E,250311,072809.3,44.1,%d.3,%d\r\n", rand() % 10000, rand() % 10000, rand() % 20, rand() % 360);
            pendingATCommandParse = true;
            
            if (pendingATCommandParse){
                if (ATCommandParse(buffRecv, sizeof(buffRecv))){
                    updateGPSInfo(buffRecv);
                    GPSInfoToString(postData);
//                    _l("%s\r\n", postData);
                    simcomObj.state = SIMCOM_STATE_HTTP_POST_DATA;
                    
                }
                CLEAR_BUFF(buffRecv);
                pendingATCommandParse = false;
            }
            break;
            
        case SIMCOM_STATE_HTTP_POST_DATA:
            
            CLEAR_BUFF(postCmd);
            sprintf(postCmd, AT_HTTP_INPUT_POST_DATA"%d,10\r", strlen(postData));
            
            if (simcomObj.flags.ATwrite && !pendingResponse && USB_HOST_SIMCOM_AT_Write(&writeTransferHandle, postCmd, strlen(postCmd)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATwrite = false;
            }
            
            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATread = false;
            }
            
            if (pendingATCommandParse){
                if (ATCommandParse(buffRecv, sizeof(buffRecv))){
                    if (strcmp(buffRecv, "\r\nDOWNLOAD\r\n") == 0){
                        
                        if (simcomObj.flags.ATwrite && USB_HOST_SIMCOM_AT_Write(&writeTransferHandle, postData, strlen(postData)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                            simcomObj.flags.ATwrite = false;
                        }
                    }
                    else if(strcmp(buffRecv, "\r\nOK\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_HTTP_SEND_POST_REQUEST;
                    }
                    else if(strcmp(buffRecv, "\r\nERROR\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_ERROR;
                    }
                    
                }
                CLEAR_BUFF(buffRecv);
                pendingATCommandParse = false;
            }
            break;
            
        case SIMCOM_STATE_HTTP_SEND_POST_REQUEST:
            
            if (simcomObj.flags.ATwrite && !pendingResponse && USB_HOST_SIMCOM_AT_Write(&writeTransferHandle, AT_HTTP_SEND_POST_REQUEST, strlen(AT_HTTP_SEND_POST_REQUEST)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATwrite = false;
            }
            
            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATread = false;
            }
            
            if (pendingATCommandParse){
                if (ATCommandParse(buffRecv, sizeof(buffRecv))){
                    if (strcmp(buffRecv, "\r\nOK\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_HTTP_WAIT_POST_RESPONSE;
                    }
                    else if(strcmp(buffRecv, "\r\nERROR\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_ERROR;
                    }
                    
                }
                CLEAR_BUFF(buffRecv);
                pendingATCommandParse = false;
            }
            break;
            
        case SIMCOM_STATE_HTTP_WAIT_POST_RESPONSE:
            
            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATread = false;
            }
            
            if (pendingATCommandParse){
                if (ATCommandParse(buffRecv, sizeof(buffRecv))){
                    if (strncmp(buffRecv, "\r\n+HTTPACTION: 1", 16) == 0){
                        pendingResponse = false;
//                        simcomObj.state = SIMCOM_STATE_HTTP_READ_HEADER;
                        simcomObj.state = SIMCOM_STATE_HTTP_STOP_SERVICE;
                    }
                    else if(strcmp(buffRecv, "\r\nERROR\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_ERROR;
                    }
                    
                }
                CLEAR_BUFF(buffRecv);
                pendingATCommandParse = false;
            }
            break;
            
        case SIMCOM_STATE_HTTP_READ_HEADER:
            
            if (simcomObj.flags.ATwrite && !pendingResponse && USB_HOST_SIMCOM_AT_Write(&writeTransferHandle, AT_HTTP_READ_HEADER, strlen(AT_HTTP_READ_HEADER)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATwrite = false;
            }
            
            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATread = false;
            }
            
            if (pendingATCommandParse){
                if (ATCommandParse(buffRecv, sizeof(buffRecv))){
                    if (strcmp(buffRecv, "\r\nOK\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_HTTP_READ_RESPONSE;
                    }
                    else if(strcmp(buffRecv, "\r\nERROR\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_ERROR;
                    }
                    
                }
                CLEAR_BUFF(buffRecv);
                pendingATCommandParse = false;
            }
            break;
            
        case SIMCOM_STATE_HTTP_READ_RESPONSE:
            
            if (simcomObj.flags.ATwrite && !pendingResponse && USB_HOST_SIMCOM_AT_Write(&writeTransferHandle, AT_HTTP_READ_RESPONSE, strlen(AT_HTTP_READ_RESPONSE)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATwrite = false;
            }
            
            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATread = false;
            }
            
            if (pendingATCommandParse){
                if (ATCommandParse(buffRecv, sizeof(buffRecv))){
                    if (strcmp(buffRecv, "\r\n+HTTPREAD: 0\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_HTTP_STOP_SERVICE;
                    }
                    else if(strcmp(buffRecv, "\r\nERROR\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_ERROR;
                    }
                    
                }
                CLEAR_BUFF(buffRecv);
                pendingATCommandParse = false;
            }
            break;
            
        case SIMCOM_STATE_HTTP_STOP_SERVICE:
            
            if (simcomObj.flags.ATwrite && !pendingResponse && USB_HOST_SIMCOM_AT_Write(&writeTransferHandle, AT_HTTP_STOP_SERVICE, strlen(AT_HTTP_STOP_SERVICE)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATwrite = false;
            }
            
            if (simcomObj.flags.ATread && USB_HOST_SIMCOM_AT_Read(&readTransferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
                simcomObj.flags.ATread = false;
            }
            
            if (pendingATCommandParse){
                if (ATCommandParse(buffRecv, sizeof(buffRecv))){
                    if (strcmp(buffRecv, "\r\nOK\r\n") == 0){
                        TIMER_SET(timer,10000);
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_READY;
                    }
                    else if(strcmp(buffRecv, "\r\nERROR\r\n") == 0){
                        pendingResponse = false;
                        simcomObj.state = SIMCOM_STATE_ERROR;
                    }
                    
                }
                CLEAR_BUFF(buffRecv);
                pendingATCommandParse = false;
            }
            break;
            
        case SIMCOM_STATE_READY:
            
            if (TIMER_COMPLETE(timer)){
                simcomObj.state = SIMCOM_STATE_HTTPS_INIT;
            }
            
            break;
            
        case SIMCOM_STATE_ERROR:
//            if (USB_HOST_SIMCOM_AT_Read(&transferHandle, buffRecv, sizeof(buffRecv)) == USB_HOST_SIMCOM_RESULT_SUCCESS){
//                _d("\r\n%s\r\n", buffRecv);
//                _dump(buffRecv, sizeof(buffRecv), DUMP_MODE_HEX);
//            }
            break;
    }
    return 0;
}


/* *****************************************************************************
 End of File
 */