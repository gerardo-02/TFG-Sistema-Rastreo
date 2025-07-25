/* 
 * File:   at_responses.h
 * Author: jtrodriguez
 *
 * Created on 1 de marzo de 2021, 8:01
 */

#ifndef AT_RESPONSES_H
#define	AT_RESPONSES_H

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
#define AT_RES_STR_LEN(cmdStr)          (sizeof(cmdStr)-1)
#define AT_RESPONSE_MAX_ARGS            24
    // 2.3 Information responses
    // "OK" If the commands included in the command line are supported by the Module
    // and the subparameters are correct if presented, some information responses
    // will be retrieved by from the Module. Otherwise, the Module will report
    // "ERROR" or "+CME ERROR" or "+CMS ERROR" to Customer Application.
#define AT_RES_OK                       "OK"
#define AT_RES_ERROR                    "ERROR"
#define AT_RES_CME_ERROR                "+CME ERROR"
    //0 phone failure
    //1 no connection to phone
    //2 phone adaptor link reserved
    //3 operation not allowed
    //4 operation not supported
    //5 PH-SIM PIN required
    //6 PH-FSIM PIN required
    //7 PH-FSIM PUK required
#define CME_ERROR_SIM_NOT_INSERTED          10  //SIM not inserted
    //11 SIM PIN required
    //12 SIM PUK required
#define CME_ERROR_SIM_FAILURE               13  //SIM failure
    //14 SIM busy
    //15 SIM wrong
    //16 incorrect password
    //17 SIM PIN2 required
    //18 SIM PUK2 required
    //20 memory full
    //21 invalid index
    //22 not found
    //23 memory failure
    //24 text string too long
    //25 invalid characters in text string
    //26 dial string too long
    //27 invalid characters in dial string
    //30 no network service
    //31 network timeout
    //32 network not allowed - emergency calls only
    //40 network personalization PIN required
    //41 network personalization PUK required
    //42 network subset personalization PIN required
    //43 network subset personalization PUK required
    //44 service provider personalization PIN required
    //45 service provider personalization PUK required
    //46 corporate personalization PIN required
    //47 corporate personalization PUK required
    //100 Unknown
    //103 Illegal MESSAGE
    //106 Illegal ME
    //107 GPRS services not allowed
    //111 PLMN not allowed
    //112 Location area not allowed
    //113 Roaming not allowed in this location area
    //132 service option not supported
    //133 requested service option not subscribed
    //134 service option temporarily out of order
    //148 unspecified GPRS error
    //149 PDP authentication failure
    //150 invalid mobile class
    //257 network rejected request
    //258 retry operation
    //259 invalid deflected to number
    //260 deflected to own number
    //261 unknown subscriber
    //262 service not available
    //263 unknown class specified
    //264 unknown network message
    //273 minimum TFTS per PDP address violated
    //274 TFT precedence index not unique
    //275 invalid parameter combination 
#define AT_RES_CMS_ERROR                "+CMS ERROR"
    //300 ME failure
    //301 SMS service of ME reserved
    //302 Operation not allowed
    //303 Operation not supported
    //304 Invalid PDU mode parameter
    //305 Invalid text mode parameter
    //310 SIM not inserted
    //311 SIM PIN required
    //312 PH-SIM PIN required
    //313 SIM failure
    //314 SIM busy
    //315 SIM wrong
    //316 SIM PUK required
    //317 SIM PIN2 required
    //318 SIM PUK2 required
    //320 Memory failure
    //321 Invalid memory index
    //322 Memory full
    //330 SMSC address unknown
    //331 no network service
    //332 Network timeout
    //340 NO +CNMA ACK EXPECTED
    //341 Buffer overflow
    //342 SMS size more than expected
#define AT_RES_ERROR_CME_CMS_UNKNOWN    500 // 500 unknown error    
#define RECEIVE_HEADER                  "+RECEIVE"  // Receive header. Ver config (17.2.1 AT+CIPCCFG Configure parameters of socket)    
    // 4.2 +CPIN: <code>
#define AT_RES_CPIN                     "+CPIN"
#define CPIN_CODE_READY                 "READY"         //ME is not pendin for any password
#define CPIN_CODE_SIM_PIN               "SIM PIN"       //ME is waiting SIM PIN to be given
#define CPIN_CODE_SIM_PUK               "SIM PUK"       //ME is waiting SIM PUK to be given
#define CPIN_CODE_PH_SIM_PIN            "PH-SIM PIN"    //ME is waiting phone-to-SIM card password to be given
#define CPIN_CODE_SIM_PIN2              "SIM PIN2"      //ME is waiting SIM PIN2 to be given
#define CPIN_CODE_SIM_PUK2              "SIM PUK2"      //ME is waiting SIM PUK2 to be given
#define CPIN_CODE_PH_NET_PIN            "PH-NET PIN"    //ME is waiting network personalization password to be given
    // 4.3 AT+CICCID Read ICCID from SIM card    
#define AT_RES_CICCID                   "+ICCID" 
    // SIMCARD
#define AT_RES_SIMCARD                  "+SIMCARD"  
    // 4.8 AT+CSQ Query signal quality    
#define AT_RES_CSQ                      "+CSQ" 
    // 4.16 AT+CCLK Real time clock management
#define AT_RES_CCLK                     "+CCLK"
    // 5.1 AT+CREG Network registration
#define AT_RES_CREG                     "+CREG"
#define CREG_STAT_NO_REG_NO_SEARCH      '0' //not registered, ME is not currently searching a new operator to register to
#define CREG_STAT_REG_HOME_NET          '1' //registered, home network
#define CREG_STAT_NO_REG_SEARCHING      '2' //not registered, but ME is currently searching a new operator to register to
#define CREG_STAT_REG_DENIED            '3' //registration denied
#define CREG_STAT_UNKNOWN               '4' //unknown
#define CREG_STAT_REG_ROAMING           '5' //registered, roaming
    // 5.15 AT+CPSI Inquiring UE system information
    // <System Mode>: System mode, values: ?NO SERVICE?, ?GSM?, ?WCDMA?, ?LTE?, ?TDS??
    // If module in LIMITED SERVICE state and +CNLSA command is set to 1, the system mode will display as ?GSM-LIMITED?, ?WCDMA-LIMITED??
    // <Operation Mode>: UE operation mode, values: ?Online?,?Offline?,?Factory Test Mode?,?Reset?, ?Low Power Mode?.    
#define AT_RES_CPSI                     "+CPSI"
#define CPSI_SYSMODE_NO_SERVICE         "NO SERVICE"
#define CPSI_SYSMODE_GSM                "GSM"
#define CPSI_SYSMODE_WCDMA              "WCDMA"
#define CPSI_SYSMODE_LTE                "LTE"
    // 5.16 AT+CNSMOD: <n>,<stat>
#define AT_RES_CNSMOD                   "+CNSMOD"
#define CNSMOD_N_NO_AUTO_REPORT         0   //disable auto report the network system mode information
#define CNSMOD_N_AUTO_REPORT            1   //auto report the network system mode information, command: +CNSMOD:<stat>    
#define CNSMOD_STAT_NO_SERVICE          0   //no service     
#define CNSMOD_STAT_GSM                 1   //GSM
#define CNSMOD_STAT_GPRS                2   //GPRS
#define CNSMOD_STAT_EGPRS               3   //EGPRS (EDGE)
#define CNSMOD_STAT_WCDMA               4   //WCDMA
#define CNSMOD_STAT_HSDPA               5   //HSDPA only(WCDMA)
#define CNSMOD_STAT_HSUPA               6   //HSUPA only(WCDMA)
#define CNSMOD_STAT_HSPA                7   //HSPA (HSDPA and HSUPA, WCDMA)
#define CNSMOD_STAT_LTE                 8   //LTE
#define CNSMOD_STAT_TDSCDMA             9   //TDS-CDMA
#define CNSMOD_STAT_TDSHSDPA            10  //TDS-HSDPA only
#define CNSMOD_STAT_TDSHSUPA            11  //TDS- HSUPA only
#define CNSMOD_STAT_TDSHSPA             12  //TDS- HSPA (HSDPA and HSUPA)
#define CNSMOD_STAT_CDMA                13  //CDMA
#define CNSMOD_STAT_EVDO                14  //EVDO
#define CNSMOD_STAT_HYBRID              15  //HYBRID (CDMA and EVDO)  
    // 5.25 AT+CTZR Time and time zone reporting. 
#define AT_RES_CTZV                     "+CTZV" //+CTZV: <tz>[,<time>][,<dst>]
    // 9.1 AT+CGREG GPRS network registration status
#define AT_RES_CGREG                    "+CGREG"
#define CGREG_STAT_NO_REG_NO_SEARCH     '0' //not registered, ME is not currently searching a new operator to register to
#define CGREG_STAT_REG_HOME_NET         '1' //registered, home network
#define CGREG_STAT_NO_REG_SEARCHING     '2' //not registered, but ME is currently searching a new operator to register to
#define CGREG_STAT_REG_DENIED           '3' //registration denied
#define CGREG_STAT_UNKNOWN              '4' //unknown
#define CGREG_STAT_REG_ROAMING          '5' //registered, roaming  
    // 9.14 AT+CGEREP GPRS event reporting
    // The following unsolicited result codes and the corresponding events are defined:
    //+CGEV: REJECT <PDP_type>, <PDP_addr>. A network request for PDP context activation occurred when the MT was unable to report it to the TE with a +CRING unsolicited result code and was automatically rejected.
    //+CGEV: NW REACT <PDP_type>, <PDP_addr>, [<cid>]. The network has requested a context reactivation. The <cid> that was used to reactivate the context is provided if known to the MT.
    //+CGEV: NW DEACT <PDP_type>, <PDP_addr>, [<cid>]. The network has forced a context deactivation. The <cid> that was used to activate the context is provided if known to the MT.
    //+CGEV: ME DEACT <PDP_type>, <PDP_addr>, [<cid>]. The mobile equipment has forced a context deactivation. The <cid> that was used to activate the context is provided if known to the MT.
    //+CGEV: NW DETACH. The network has forced a Packet Domain detach. This implies that all active contexts have been deactivated. These are not reported separately.
    //+CGEV: ME DETACH. The mobile equipment has forced a Packet Domain detach. This implies that all active contexts have been deactivated. These are not reported separately.
    //+CGEV: NW CLASS <class>. The network has forced a change of MS class. The highest available class is reported (see AT+CGCLASS).
    //+CGEV: ME CLASS <class>. The mobile equipment has forced a change of MS class. The highest available class is reported (see AT+CGCLASS).    
#define AT_RES_CGEV                     "+CGEV"
    // 17 AT Commands for Internet Service  
    // 17.2.7 AT+NETOPEN Open socket
#define AT_RES_NET_OPEN                 "+NETOPEN"
#define NETOPEN_NET_STATE_CLOSE         '0'
#define NETOPEN_NET_STATE_OPEN          '1'
    // 17.2.8 AT+NETCLOSE Close socket
#define AT_RES_NET_CLOSE                "+NETCLOSE" //+NETCLOSE: <err>    
    // 17.2.9 AT+IPADDR Inquire socket PDP address
#define AT_RES_IPADDR                   "+IPADDR"
    // 17.2.11 AT+SERVERSTART Startup TCP server
#define AT_RES_SERVERSTART              "+SERVERSTART"
    // 17.2.12 AT+SERVERSTOP Stop TCP server
#define AT_RES_SERVERSTOP               "+SERVERSTOP"
    // 17.2.13 AT+CIPOPEN Establish connection in multi-socket mode
#define AT_RES_CIPOPEN                  "+CIPOPEN" 
    // 17.2.14 AT+CIPSEND Send data through TCP or UDP connection
#define AT_RES_CIPSEND                  "+CIPSEND"
    // 17.2.16 AT+CIPCLOSE Close TCP or UDP socket
#define AT_RES_CIPCLOSE                 "+CIPCLOSE"
    // 17.2.18 Information elements related to TCP/IP
    // The following table lists information elements which may be reported.
    // +CIPEVENT: NETWORK CLOSED UNEXPECTEDLY
    // Network is closed for network error(Out of service, etc). When this event
    // happens, user?s application needs to check and close all opened sockets,
    // and then uses AT+NETCLOSE to release the network library if AT+NETOPEN?
    // shows the network library is still opened.
#define AT_RES_CIPEVENT                 "+CIPEVENT"
    // +IPCLOSE: <client_index>, <close_reason>
    // Socket is closed passively.
    // <client_index> is the link number.
    // <close_reason>:
    // 0 - Closed by local, active
    // 1 - Closed by remote, passive
    // 2 - Closed for sending timeout
#define AT_RES_IPCLOSE                  "+IPCLOSE"  
    // +CLIENT: < link_num >,<server_index>,<client_IP>:<port>
    // TCP server accepted a new socket client, the index is <link_num>, the TCP
    // server index is <server_index>. The peer IP address is <client_IP>, the
    // peer port is <port>.
#define AT_RES_CLIENT                   "+CLIENT"
    // 17.2.19 Unsolicited TCP/IP command <err> Codes
#define US_TCP_ERR_OPERATION_SUCCESS        0   //operation succeeded
#define US_TCP_ERR_NETWORK_FAILURE          1   //Network failure
#define US_TCP_ERR_NETWORK_NOT_OPEN         2   //Network not opened
#define US_TCP_ERR_WORNG_PARAMETER          3   //Wrong parameter
#define US_TCP_ERR_OPERATION_NOT_SUPPORTED  4   //Operation not supported
#define US_TCP_ERR_FAILED_CREATE_SOCKET     5   //Failed to create socket
#define US_TCP_ERR_FAILED_BIND_SOCKET       6   //Failed to bind socket
#define US_TCP_ERR_SERVER_ALREADY_LISTEN    7   //TCP server is already listening
#define US_TCP_ERR_BUSY                     8   //Busy
#define US_TCP_ERR_SOCKETS_OPENED           9   //Sockets opened
#define US_TCP_ERR_TIMEOUT                  10  //Timeout
#define US_TCP_ERR_DNS_PARSE_FAILED         11  //DNS parse failed for AT+CIPOPEN
#define US_TCP_ERR_UNKNOWN                  255 //Unknown error
    // +CIPERROR: <err>
#define AT_RES_CIPERROR                 "+CIPERROR"
    // +IP ERROR: <error message>
#define AT_RES_IP_ERROR                 "+IP ERROR"
    // 19 AT Commands for GPS
    // 19.1 AT+CGPS Start/Stop GPS session
    // +CGPS: <on/off>,<mode>
    // <on/off>: 0 ? stop GPS session, 1 ? start GPS session
#define GPS_RES_SESSION_STOP            '0'    
#define GPS_RES_SESSION_START           '1'
    // <mode>: 1 ? standalone mode, 2 ? UE-based mode, 3 ? UE-assisted mode
#define AT_RES_CGPS                     "+CGPS:"    //En esta he dejado los ':' para que no haya ambiguedad con +CGPSINFO
    // 19.2 AT+CGPSINFO Get GPS fixed position information
    // +CGPSINFO: [<lat>],[<N/S>],[<log>],[<E/W>],[<date>],[<UTC time>],[<alt>],[<speed>],[<course>]    
    // <lat>: Latitude of current position. Output format is ddmm.mmmmmm
    // <N/S>: N/S Indicator, N=north or S=south
    // <log>: Longitude of current position. Output format is dddmm.mmmmmm
    // <E/W>: E/W Indicator, E=east or W=west
    // <date>: Date. Output format is ddmmyy
    // <UTC time>: UTC Time. Output format is hhmmss.s
    // <alt>: MSL Altitude. Unit is meters.
    // <speed>: Speed Over Ground. Unit is knots.
    // <course>: Course. Degrees.
#define AT_RES_CGPSINFO                 "+CGPSINFO"
    
    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Data types & Definitions                                          */
    /* ************************************************************************** */

    /* ************************************************************************** */
    typedef struct {
        char *responseHeader;
        uint32_t nArgs;
        char *arg[AT_RESPONSE_MAX_ARGS];
    } t_RESPONSE_CALL_BACK_CONTEXT;
    typedef void (*RESPONSE_CALL_BACK)(t_RESPONSE_CALL_BACK_CONTEXT *ctx);

    /* ************************************************************************** */
    /* ************************************************************************** */
    /* Section: Interface Functions                                               */
    /* ************************************************************************** */
    /* ************************************************************************** */
    void response_init(void);
    void response_manage(char* response);
    void receive_manage(char* receive);
    uint32_t response_set_call_back(char* responseHeader, RESPONSE_CALL_BACK cb);

#ifdef	__cplusplus
}
#endif

#endif	/* AT_RESPONSES_H */

