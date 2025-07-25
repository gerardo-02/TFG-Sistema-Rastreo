/* 
 * File:   at_commands.h
 * Author: jtrodriguez
 *
 * Created on 1 de marzo de 2021, 7:53
 */

#ifndef AT_COMMANDS_H
#define	AT_COMMANDS_H

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
#define AT_CMD_STR_LEN(cmdStr)                    (sizeof(cmdStr)-1)

    // 3 AT Commands According V.25TER

    // 3.16 ATE Enable command echo
#define AT_ENABLE_COMMAND_ECHO              "ATE"
#define ECHO_MODE_OFF                       "0"
#define ECHO_MODE_ON                        "1"    
#define AT_CMD_ECHO_OFF                     AT_ENABLE_COMMAND_ECHO ECHO_MODE_OFF "\r"

    // 4 AT Commands for Status Control

    // 4.2 AT+CPIN Enter PIN
    // Description
    // This command is used to send the ME a password which is necessary before
    // it can be operated (SIM PIN, SIM PUK, PH-SIM PIN, etc.). If the PIN is to
    // be entered twice, the TA shall automatically repeat the PIN. If no PIN
    // request is pending, no action is taken towards MT and an error message, 
    // +CME ERROR, is returned to TE.
    // If the PIN required is SIM PUK or SIM PUK2, the second pin is required. 
    // This second pin, <newpin>, is used to replace the old pin in the SIM.
#define AT_ENTER_PIN                        "AT+CPIN"
#define AT_CMD_READ_CPIN                    AT_ENTER_PIN "?\r" 
    // 4.3 AT+CICCID Read ICCID from SIM card
    // Description: This command is used to Read the ICCID from SIM card    
#define AT_READ_ICCID_FROM_SIM_CARD         "AT+CICCID"
#define AT_CMD_CICCID                       AT_READ_ICCID_FROM_SIM_CARD "\r"
    // 4.8 AT+CSQ Query signal quality
#define AT_QUERY_CSQ                        "AT+CSQ"
#define AT_CMD_QUERY_CSQ                    AT_QUERY_CSQ "\r"    
    // 4.9 AT+AUTOCSQ Set CSQ report
    // Description: This command is used to enable or disable automatic report CSQ
    // information, when automatic report enabled, the module reports CSQ
    // information every five seconds or only after <rssi> or <ber> is changed,
    // the format of automatic report is ?+CSQ: <rssi>,<ber>?.
#define AT_SET_CSQ_REPORT                   "AT+AUTOCSQ"
#define AUTOCSQ_AUTO_DISABLE                "0" //disable automatic report
#define AUTOCSQ_AUTO_ENABLE                 "1" //enable automatic report
#define AUTOCSQ_MODE_EVERY_5_SECONDS        "0" //CSQ automatic report every five seconds
#define AUTOCSQ_MODE_AUTOMATIC              "1" //CSQ automatic report only after <rssi> or <ber> is changed. NOTE: If the parameter of <mode> is omitted when executing write command, <mode> will be set to default value.
#define AT_CMD_AUTOCSQ_ENABLE               AT_SET_CSQ_REPORT "=" AUTOCSQ_AUTO_ENABLE "," AUTOCSQ_MODE_AUTOMATIC "\r"
    // 4.16 AT+CCLK Real time clock management
#define AT_REAL_TIME_CLOCK_MANAGEMENT       "AT+CCLK"
#define AT_CMD_READ_CCLK                    AT_REAL_TIME_CLOCK_MANAGEMENT "?\r"
    // 4.17 AT+CMEE Report mobile equipment error
    // Description: This command is used to disable or enable the use of result code
    // "+CME ERROR: <err>" or "+CMS ERROR: <err>" as an indication of an error
    // relating to the functionality of ME; when enabled, the format of <err> can be
    // set to numeric or verbose string.
#define AT_REPORT_MOBILE_EQUIPMET_ERROR     "AT+CMEE"
#define CMEE_DISABLE                        "0" //Disable result code,i.e. only "ERROR" will be displayed
#define CMEE_ENABLE_NUMERIC_VALUE           "1" //Enable error result code with numeric values
#define CMEE_ENABLE_STRING_VALUE            "2" //Enable error result code with string values 
#define AT_CMD_CMEE_ENABLE                  AT_REPORT_MOBILE_EQUIPMET_ERROR "=" CMEE_ENABLE_NUMERIC_VALUE "\r"  

    // 5 AT Commands for Network

    // 5.1 AT+CREG Network registration
    // Description: This command is used to control the presentation of an unsolicited result code +CREG: <stat> when <n>=1 and there is a change in the ME network registration status, or code +CREG: <stat>[,<lac>,<ci>] when <n>=2 and there is a change of the network cell.
    // Read command returns the status of result code presentation and an integer <stat> which shows whether the network has currently indicated the registration of the ME. Location information elements <lac> and <ci> are returned only when <n>=2 and ME is registered in the network.    
#define AT_NETWORK_REGISTRATION             "AT+CREG"
#define AT_CMD_READ_CREG                    AT_NETWORK_REGISTRATION "?\r"  
    // 5.11 AT+CNMP Preferred mode selection
    // Description: This command is used to select or set the state of the mode
    // preference.
#define AT_PREFERRED_MODE_SELECTION         "AT+CNMP"
#define CNMP_MODE_AUTO                      "2"     //Automatic
#define CNMP_MODE_GSM_ONLY                  "13"    //GSM Only
#define CNMP_MODE_WCDMA_ONLY                "14"    //WCDMA Only
#define CNMP_MODE_LTE_ONLY                  "38"    //LTE Only
#define CNMP_MODE_TDSCDMA_ONLY              "59"    //TDS-CDMA Only
#define CNMP_MODE_CDMA_ONLY                 "9"     //CDMA Only
#define CNMP_MODE_EVDO_ONLY                 "10"    //EVDO Only 
#define CNMP_MODE_GSM_WCDMA_ONLY            "19"    //GSM+WCDMA Only
#define CNMP_MODE_CDMA_EVO_ONLY             "22"    //CDMA+EVDO Only
#define CNMP_MODE_ANY_LTE_MODE              "48"    //Any modes but LTE
#define CNMP_MODE_GSM_TDSCDMA_ONLY          "60"    //GSM+TDSCDMA Only
#define CNMP_MODE_GSM_TDSCDMA_LTE           "61"    //GSM+TDSCDMA+LTE
#define CNMP_MODE_GSM_WCDMA_TDSCDMA_ONLY    "63"    //GSM+WCDMA+TDSCDMA Only
#define CNMP_MODE_CDMA_EVDO_GSM_WCDMA_TDSCDMA_ONLY  "67"  //CDMA+EVDO+GSM+WCDMA+TDSCDMA Only
#define AT_CMD_CNMP_SET_AUTO                AT_PREFERRED_MODE_SELECTION "=" CNMP_MODE_AUTO "\r"  //(!Cuidado al usarlo, cuando se lanza esta config se pone en "No service" durante unos 20 segundos!!!)
#define AT_CMD_READ_CNMP                    AT_PREFERRED_MODE_SELECTION "?\r"
    // 5.15 AT+CPSI Inquiring UE system information
    // This command is used to return the UE system information.
#define AT_INQUIRING_UE_SYSTEM_INFORMATION  "AT+CPSI"
#define AT_CMD_READ_CPSI                    AT_INQUIRING_UE_SYSTEM_INFORMATION "?\r"
#define CPSI_TIME_SYSTEM_INFORMATION        "0" //Disable period information
#define AT_CMD_CPSI_INFO_PERIOD             AT_INQUIRING_UE_SYSTEM_INFORMATION "=" CPSI_TIME_SYSTEM_INFORMATION "\r"
    // 5.16 AT+CNSMOD Show network system mode
    // Description: This command is used to return the current network system mode.   
#define AT_SHOW_NETWORK_SYSTEM_MODE         "AT+CNSMOD"
#define CNSMOD_DISABLE_AUTO_REPORT          "0" //disable auto report the network system mode information
#define CNSMOD_ENABLE_AUTO_REPORT           "1" //auto report the network system mode information, command: +CNSMOD:<stat>
#define AT_CMD_CNSMOD_ENABLE_AUTO_REPORT    AT_SHOW_NETWORK_SYSTEM_MODE "=" CNSMOD_ENABLE_AUTO_REPORT "\r"
#define AT_CMD_READ_CNSMOD                  AT_SHOW_NETWORK_SYSTEM_MODE "?\r"    
    // 5.24 AT+CTZU Automatic time and time zone update
    // Description: This command is used to enable and disable automatic time and
    // time zone update via NITZ. For automatic time and time zone update is
    // enabled (+CTZU=1): If time zone is only received from network and it
    // isn?t equal to local time zone (AT+CCLK), time zone is updated
    // automatically, and real time clock is updated based on local time and the
    // difference between time zone from network and local time zone (Local time
    // zone must be valid). If Universal Time and time zone are received from
    // network, both time zone and real time clock is updated automatically, and
    // real time clock is based on Universal Time and time zone from network. 
#define AT_AUTOMATIC_TIME_ZONE_UPDATE       "AT+CTZU"
#define CTZU_DISABLE                        "0"
#define CTZU_ENABLE                         "1"
#define AT_CMD_CTZU_ENABLE                  AT_AUTOMATIC_TIME_ZONE_UPDATE "=" CTZU_ENABLE "\r"
    // 5.25 AT+CTZR Time and time zone reporting
    // Description: This command is used to enable and disable the time zone change
    // event reporting. If the reporting is enabled the MT returns the unsolicited
    // result code +CTZV: <tz>[,<time>][,<dst>]whenever the time zone is changed.
    // NOTE: The time zone reporting is not affected by the Automatic Time and Time
    // Zone command AT+CTZU.
#define AT_TIME_ZONE_REPORTING              "AT+CTZR"
#define CTZR_OFF                            "0"
#define CTZR_ON                             "1"
#define AT_CMD_CTZR_ON                      AT_TIME_ZONE_REPORTING "=" CTZR_ON "\r"   

    // 9 AT Commands for GPRS

    // 9.1 AT+CGREG GPRS network registration status
    // Description: This command controls the presentation of an unsolicited
    // result code ?+CGREG: <stat>? when <n>=1 and there is a change in the MT's
    // GPRS network registration status. The read command returns the status of
    // result code presentation and an integer <stat> which shows Whether the 
    // network has currently indicated the registration of the MT.
#define AT_GPRS_NETWORK_REGISTRATION        "AT+CGREG"
#define AT_CMD_READ_CGREG                   AT_GPRS_NETWORK_REGISTRATION "?\r"
    // 9.14 AT+CGEREP GPRS event reporting
#define AT_GPRS_EVENT_REPORTING             "AT+CGEREP"
#define GPRS_EVENT_MODE_0                   "0" // buffer unsolicited result codes in the MT; if MT result code buffer is full, the oldest ones can be discarded. No codes are forwarded to the TE.
#define GPRS_EVENT_MODE_1                   "1" // discard unsolicited result codes when MT-TE link is reserved (e.g. in on-line data mode); otherwise forward them directly to the TE.
#define GPRS_EVENT_MODE_2                   "2" // buffer unsolicited result codes in the MT when MT-TE link is reserved (e.g. in on-line data mode) and flush them to the TE when MT-TE link becomes available; otherwise forward them directly to the TE.
#define GPRS_EVENT_BFR_0                    "0" // MT buffer of unsolicited result codes defined within this command is cleared when <mode> 1 or 2 is entered.
#define GPRS_EVENT_BFR_1                    "1" // MT buffer of unsolicited result codes defined within this command is flushed to the TE when <mode> 1 or 2 is entered (OK response shall be given before flushing the codes).
#define AT_CMD_CGEREP_2_0                   AT_GPRS_EVENT_REPORTING "=" GPRS_EVENT_MODE_2 "," GPRS_EVENT_BFR_0 "\r"

    // 17 AT Commands for Internet Service

    // 17.1 Common    
    // 17.1.1 AT+CGSOCKCONT Define socket PDP context
#define AT_DEFINE_SOCKET_PDP_CONTEXT        "AT+CGSOCKCONT"
    // 17.1.2 AT+CSOCKSETPN Set active PDP context?s profile number
#define AT_ACTIVE_PDP_CONTEXT_NUMBER        "AT+CSOCKSETPN"
#define AT_CMD_SELECT_PDP_1                 AT_ACTIVE_PDP_CONTEXT_NUMBER "=1\r"
    // 17.2 TCP/UDP
    // 17.2.1 AT+CIPCCFG Configure parameters of socket
#define AT_CONFIGURE_SOCKET_PARAMETERS      "AT+CIPCCFG"    //[<NmRetry>][,[<DelayTm>][,[<Ack>][,[<errMode>][,]<HeaderType>][,[[<AsyncMode>][,[<TimeoutVal>]]]]]]]]
#define AT_CMD_CIPCCFG                      AT_CONFIGURE_SOCKET_PARAMETERS "=3,500,0,1,1,0,500\r"
    // 17.2.2 AT+CIPSENDMODE Select sending mode
#define AT_SENDING_MODE                     "AT+CIPSENDMODE"
#define CIPSENDMODE_0                       "0" //sending without waiting peer TCP ACK mode
#define CIPSENDMODE_1                       "1" //sending wait peer TCP ACK mode
#define AT_CMD_CIPSENDMODE_1                AT_SENDING_MODE "=" CIPSENDMODE_1 "\r"
    // 17.2.3 AT+CIPTIMEOUT Set TCP/IP timeout value
#define AT_TCP_TIMEOUTS                     "AT+CIPTIMEOUT" //[<netopen_timeout>][, [<cipopen_timeout>][, [<cipsend_timeout>]]]
#define AT_CMD_CIPTIMEOUT                   AT_TCP_TIMEOUTS "=30000,20000,40000\r"
    // 17.2.4 AT+CIPHEAD Add an IP head when receiving data
#define AT_ADD_IP_HEAD_RECEIVING_DATA       "AT+CIPHEAD"
#define CIPHEAD_MODE_0                      "0" // not add IP header
#define CIPHEAD_MODE_1                      "1" // add IP header, the format is "+IPD(data length)"    
#define AT_CMD_CIPHEAD_MODE_1               AT_ADD_IP_HEAD_RECEIVING_DATA "=" CIPHEAD_MODE_1 "\r"
    // 17.2.5 AT+CIPSRIP Show Remote IP address and Port
#define AT_SHOW_REMOTE_IP_ADDRESS           "AT+CIPSRIP"
#define CIPSRIP_MODE_0                      "0" // do not show the prompt
#define CIPSRIP_MODE_1                      "1" // show the prompt,the format is as follows: "RECV FROM:<IP ADDRESS>:<PORT>"
#define AT_CMD_CIPSRIP_MODE_0               AT_SHOW_REMOTE_IP_ADDRESS "=" CIPSRIP_MODE_0 "\r"    
    // 17.2.6 AT+CIPMODE Select TCP/IP application mode
#define AT_TCP_APPLICATION_MODE             "AT+CIPMODE"
#define CIPMODE_0                           "0" //Non transparent mode
#define CIPMODE_1                           "1" //Transparent mode
#define AT_CMD_CIPMODE_0                    AT_TCP_APPLICATION_MODE "=" CIPMODE_0 "\r"
    // 17.2.7 AT+NETOPEN Open socket
    // NOTE: The test command and the write command of AT+NETOPEN is reserved 
    // for being compatible with old TCP/IP command set, and the old TCP/IP 
    // command set is not recommended to be used any longer.
#define AT_OPEN_SOCKET                      "AT+NETOPEN"
#define AT_CMD_READ_NETOPEN                 AT_OPEN_SOCKET "?\r"
#define AT_CMD_EXEC_NETOPEN                 AT_OPEN_SOCKET "\r"
    // 17.2.8 AT+NETCLOSE Close socket    
    // Description: This command closes network. Before calling this command, 
    // all opened sockets must be closed first.
#define AT_CLOSE_SOCKET                     "AT+NETCLOSE"
#define AT_CMD_READ_NETCLOSE                AT_CLOSE_SOCKET "?\r"
#define AT_CMD_EXEC_NETCLOSE                AT_CLOSE_SOCKET "\r"    
    // 17.2.9 AT+IPADDR Inquire socket PDP address
#define AT_INQUIRE_SOCKET_PDP_ADDRESS       "AT+IPADDR"
#define AT_CMD_EXEC_IPADDR                  AT_INQUIRE_SOCKET_PDP_ADDRESS "\r"
    // 17.2.11 AT+SERVERSTART Startup TCP server
    // Description: This command starts up TCP server, and the server can
    // receive the request of TCP client. After the command executes
    // successfully, an unsolicited result code is returned when a client tries
    // to connect with module and module accepts request. The unsolicited result
    // code is +CLIENT: < link_num >,<server_index>,<client_IP>:<port>.
#define AT_STARTUP_TCP_SERVER               "AT+SERVERSTART"    
#define AT_CMD_READ_SERVERSTART             AT_STARTUP_TCP_SERVER "?\r"
    // 17.2.12 AT+SERVERSTOP Stop TCP server
    // Description: This command stops TCP server. Before stopping a TCP server,
    // all sockets with <server_index> equals to the closing TCP server index
    // must be closed first.
#define AT_STOP_TCP_SERVER                  "AT+SERVERSTOP"    
    // 17.2.13 AT+CIPOPEN Establish connection in multi-socket mode
    // Description: This command is used to establish a connection with TCP 
    // server and UDP server, The sum of all of connections is 10
#define AT_SOCKET_CONNECTION                "AT+CIPOPEN"
#define AT_CMD_READ_CIPOPEN                 AT_SOCKET_CONNECTION "?\r"
#define AT_CMD_OPEN_SOCKET                  AT_SOCKET_CONNECTION    //AT+CIPOPEN=<link_num>,?TCP?,<serverIP>,<serverPort>
    // 17.2.14 AT+CIPSEND Send data through TCP or UDP connection
    // Description: This command is used to send data to remote side. The 
    // <length> field can be empty, when it is empty, Each <Ctrl+Z> character
    // present in the data should be coded as <ETX><Ctrl+Z>. Each <ESC> 
    // character present in the data should be coded as <ETX><ESC>. Each <ETX>
    // character will be coded as <ETX><ETX>. Single <Ctrl+Z> means end of the
    // input data. Single <ESC> is used to cancel the sending. <ETX> is 0x03, 
    // and <Ctrl+Z> is 0x1A, <ESC> is 0x1B.
#define AT_SOCKET_SEND                      "AT+CIPSEND"
#define AT_CMD_CIPSEND                      AT_SOCKET_SEND  //AT+CIPSEND=<link_num>,[<length>]<CR>data for send
    // 17.2.16 AT+CIPCLOSE Close TCP or UDP socket
#define AT_CLOSE_TCP_SOCKET                 "AT+CIPCLOSE"
#define AT_CMD_CLOSE_SOCKET                 AT_CLOSE_TCP_SOCKET //AT+CIPCLOSE=<link_num>
    // 19 AT Commands for GPS
    // 19.1 AT+CGPS Start/Stop GPS session
    // Description: This command is used to start or stop GPS session.
#define AT_START_STOP_GPS_SESSION           "AT+CGPS"
#define AT_CMD_GET_GPS_SESSION_STATE        AT_START_STOP_GPS_SESSION "?\r"
#define AT_CMD_START_GPS_SESSION            AT_START_STOP_GPS_SESSION "=1,1\r"
#define AT_CMD_STOP_GPS_SESSION             AT_START_STOP_GPS_SESSION "=0,1\r"
    // 19.2 AT+CGPSINFO Get GPS fixed position information
    // Description: This command is used to get current position information.
    // AT+CGPSINFO=<time>
    // <time>: The range is 0-255, unit is second, after set <time> will report the GPS information every the seconds.
#define AT_GET_GPS_FIXED_POSITION_INFO      "AT+CGPSINFO"
#define AT_CMD_EXEC_CGPSINFO                AT_GET_GPS_FIXED_POSITION_INFO "\r"

    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Data types & Definitions                                          */
    /* ************************************************************************** */
    /* ************************************************************************** */


    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Interface Functions                                               */
    /* ************************************************************************** */
    /* ************************************************************************** */


#ifdef	__cplusplus
}
#endif

#endif	/* AT_COMMANDS_H */

