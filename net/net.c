#include <stddef.h>

#include "definitions.h"

#include "log.h"
#include "drv/pmz_time.h"
#include "drv/drv_dmem.h"
//#include "tcp_server.h"
//#include "../devices/com_external_readers.h"
#include "server/telnet_server.h"
//#include "tcp_client.h"
//#include "web_socket.h"
//#include "commands.h"

#include "net.h"
#include "server/tcp_server.h"
#include "client/tcp_client.h"
#include "client/ws_client.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */
#define NET_CFG_IPV4_HOST_NAME_STR_LENGTH   16
#define NET_CFG_IPV4_MAC_STR_LENGTH         18
#define NET_CFG_IPV4_IP_STR_LENGTH          16

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Data Types                                                        */
/* ************************************************************************** */

/* ************************************************************************** */
//typedef struct
//{
//  char hostName[NET_CFG_IPV4_HOST_NAME_STR_LENGTH];
//  char mac[NET_CFG_IPV4_MAC_STR_LENGTH];
//  char ip[NET_CFG_IPV4_IP_STR_LENGTH];
//  char mask[NET_CFG_IPV4_IP_STR_LENGTH];
//  char gw[NET_CFG_IPV4_IP_STR_LENGTH];
//  char dns1[NET_CFG_IPV4_IP_STR_LENGTH];
//  char dns2[NET_CFG_IPV4_IP_STR_LENGTH];
//} t_NET_CFG;

typedef enum
{
  IFACE_STATE_INIT = 0,
  IFACE_STATE_DOWN,
  IFACE_STATE_UP,
  IFACE_STATE_LINK,
  IFACE_STATE_ERROR,
} t_IFACE_STATES;

typedef struct
{
  t_IFACE_STATES state;
  TCPIP_NET_HANDLE handle;
  IPV4_ADDR ipAddr;
} t_IFACE_DATA;

typedef struct
{
  t_NET_STATES state;
  int nIfaces;
  t_IFACE_DATA *iface;
} t_NET_DATA;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */
static t_NET_DATA netData;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

static void cb_ntp (TCPIP_SNTP_EVENT e, const void* eParam)
{
  TCPIP_SNTP_EVENT_TIME_DATA *data = (TCPIP_SNTP_EVENT_TIME_DATA*) eParam;
  time_t utc, local;

  switch (e)
    {
    case TCPIP_SNTP_EVENT_NONE:
      _l ("(NET-NTP) Invalid event success\n");
      break;
    case TCPIP_SNTP_EVENT_TSTAMP_OK:
      utc = (time_t) data->tUnixSeconds;
      local = pmz_time_utc_to_local (utc);
      _l ("(NET-NTP) (UTC)   %s", ctime (&utc));
      _l ("(NET-NTP) (LOCAL) %s\r", ctime (&local));
      pmz_time_set (&utc, true);
      break;
    case TCPIP_SNTP_EVENT_DNS_ERROR:
      _l ("(NET-NTP) NTP server name DNS failed\n");
      break;
    case TCPIP_SNTP_EVENT_IF_ERROR:
      _l ("(NET-NTP) Could not select a valid NTP interface\n");
      break;
    case TCPIP_SNTP_EVENT_SKT_ERROR:
      _l ("(NET-NTP) could not bind the socket to the NTP interface or timeout on the socket TX\n");
      break;
    case TCPIP_SNTP_EVENT_SERVER_TMO:
      _l ("(NET-NTP) no reply received from the NTP server\n");
      break;
    case TCPIP_SNTP_EVENT_VER_ERROR:
      _l ("(NET-NTP) the server response has a wrong version\n");
      break;
    case TCPIP_SNTP_EVENT_TSTAMP_ERROR:
      _l ("(NET-NTP) the server response had wrong timestamp\n");
      break;
    case TCPIP_SNTP_EVENT_KOD_ERROR:
      _l ("(NET-NTP) the server replied with a ¡Kiss O' Death! code\n");
      break;
    case TCPIP_SNTP_EVENT_SYNC_ERROR:
      _l ("(NET-NTP) the server response has a synchronization error\n");
      break;
    default:
      _l ("(NET-NTP) Event unknown: %d\n", e);
      break;
    }
}

/* ************************************************************************** */
/* ************************************************************************** */
// Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */
#define SET_NET_STATE(newState)             netData.state=newState

static void reset_all (void)
{
  tcp_server_reset ();
  tcp_client_reset ();
  ws_client_reset ();
  //  netData.ntpSync = false;
  //  TCPIP_SNTP_Disable ();
  //  _l ("(NET) NTP service disabled.\n");
}

//void net_cfg_get_default (t_NET_CFG *nc)
//{
//  strcpy (nc->hostName, TCPIP_NETWORK_DEFAULT_HOST_NAME_IDX0);
//  strcpy (nc->mac, TCPIP_NETWORK_DEFAULT_MAC_ADDR_IDX0);
//  strcpy (nc->ip, TCPIP_NETWORK_DEFAULT_IP_ADDRESS_IDX0);
//  strcpy (nc->mask, TCPIP_NETWORK_DEFAULT_IP_MASK_IDX0);
//  strcpy (nc->gw, TCPIP_NETWORK_DEFAULT_GATEWAY_IDX0);
//  strcpy (nc->dns1, TCPIP_NETWORK_DEFAULT_DNS_IDX0);
//  strcpy (nc->dns1, TCPIP_NETWORK_DEFAULT_SECOND_DNS_IDX0);
//}
//
//void net_cfg_get_current (t_NET_CFG *nc)
//{
//  TCPIP_NET_HANDLE netH;
//  IPV4_ADDR addr;
//
//  netH = TCPIP_STACK_NetHandleGet ("eth0");
//  if (netH)
//    {
//      strncpy (nc->hostName, TCPIP_STACK_NetBIOSName (netH), NET_CFG_IPV4_HOST_NAME_STR_LENGTH - 1);
//      addr.Val = TCPIP_STACK_NetAddress (netH);
//      TCPIP_Helper_IPAddressToString (&addr, nc->ip, NET_CFG_IPV4_IP_STR_LENGTH);
//      addr.Val = TCPIP_STACK_NetMask (netH);
//      TCPIP_Helper_IPAddressToString (&addr, nc->mask, NET_CFG_IPV4_IP_STR_LENGTH);
//      addr.Val = TCPIP_STACK_NetAddressGateway (netH);
//      TCPIP_Helper_IPAddressToString (&addr, nc->gw, NET_CFG_IPV4_IP_STR_LENGTH);
//      addr.Val = TCPIP_STACK_NetAddressDnsPrimary (netH);
//      TCPIP_Helper_IPAddressToString (&addr, nc->dns1, NET_CFG_IPV4_IP_STR_LENGTH);
//      //addr.Val = TCPIP_STACK_NetAddressDnsSecond(netH);
//      //TCPIP_Helper_IPAddressToString (&addr, nc->, NET_CFG_IPV4_IP_STR_LENGTH);
//      TCPIP_Helper_MACAddressToString ((TCPIP_MAC_ADDR*) TCPIP_STACK_NetAddressMac (netH), nc->mac, NET_CFG_IPV4_MAC_STR_LENGTH);
//    }
//  else
//    {
//      _l ("(NET CFG) Get current config failed!\n");
//    }
//}
//
//void net_cfg_start (t_NET_CFG *nc)
//{
//  TCPIP_NET_HANDLE netH;
//  TCPIP_NETWORK_CONFIG netConfig;
//
//  netConfig.interface = "ETHMAC";
//  netConfig.hostName = nc->hostName;
//  netConfig.macAddr = nc->mac;
//  netConfig.ipAddr = nc->ip;
//  netConfig.ipMask = nc->mask;
//  netConfig.gateway = nc->gw;
//  netConfig.priDNS = nc->dns1;
//  netConfig.secondDNS = nc->dns2;
//  netConfig.powerMode = "full"; //down
//  netConfig.startFlags = (TCPIP_NETWORK_CONFIG_DNS_CLIENT_ON | TCPIP_NETWORK_CONFIG_IP_STATIC);
//  netConfig.pMacObject = &DRV_ETHMAC_PIC32MACObject;
//
//  netH = TCPIP_STACK_NetHandleGet ("eth0");
//  if (!TCPIP_STACK_NetUp (netH, &netConfig))
//    {
//      _l ("(NET CFG) Set UP failed!\n");
//    }
//  else
//    {
//      _l ("(NET CFG) Set UP net:\n Host: %s\n MAC: %s\n Net: %s, %s\n GW: %s\n DNS1: %s, DNS2: %s\n", netConfig.hostName, netConfig.macAddr, netConfig.ipAddr, netConfig.ipMask, netConfig.gateway, netConfig.priDNS, netConfig.secondDNS);
//    }
//}

static void iface_task (t_IFACE_DATA *iface)
{
  switch (iface->state)
    {
    case IFACE_STATE_INIT:
      _l ("(NET-IFACE) (%d) found! Net name: %s,  net bios name: %s\n", TCPIP_STACK_NetIndexGet (iface->handle), TCPIP_STACK_NetNameGet (iface->handle), TCPIP_STACK_NetBIOSName (iface->handle));
      iface->state = IFACE_STATE_DOWN;
      break;
    case IFACE_STATE_DOWN:
      if (TCPIP_STACK_NetIsUp (iface->handle))
        {
          _l ("(NET-IFACE) (%d) is up!\n", TCPIP_STACK_NetIndexGet (iface->handle));
          iface->state = IFACE_STATE_UP;
        }
      break;
    case IFACE_STATE_UP:
      if (!TCPIP_STACK_NetIsUp (iface->handle))
        {
          _l ("(NET-IFACE) (%d) is down!\n", TCPIP_STACK_NetIndexGet (iface->handle));
          iface->state = IFACE_STATE_DOWN;
        }
      else if (TCPIP_STACK_NetIsReady (iface->handle))
        {
          iface->ipAddr.Val = TCPIP_STACK_NetAddress (iface->handle);
          _l ("(NET-IFACE) (%d) Linked! Local IP address: %d.%d.%d.%d\n", TCPIP_STACK_NetIndexGet (iface->handle), iface->ipAddr.v[0], iface->ipAddr.v[1], iface->ipAddr.v[2], iface->ipAddr.v[3]);
          iface->state = IFACE_STATE_LINK;
        }
      break;
    case IFACE_STATE_LINK:
      if (!TCPIP_STACK_NetIsReady (iface->handle))
        {
          _l ("(NET-IFACE) (%d) No Link!\n", TCPIP_STACK_NetIndexGet (iface->handle));
          iface->state = IFACE_STATE_UP;
        }
      break;
    case IFACE_STATE_ERROR:
      break;
    }
}

/* ************************************************************************** */
/* ************************************************************************** */
// Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */
void net_init (void)
{
  //net_cfg_start (&ac.netCfg.net_ipv4);
  SET_NET_STATE (NET_STATE_INIT);
}

t_NET_STATES net_tasks (void)
{
  int i;

  switch (netData.state)
    {
    case NET_STATE_INIT:
      //      TCPIP_SNTP_Disable ();
      //      //      netData.ntpSync = false;
      //      _l ("(NET) NTP service disabled.\n");
      //      tcp_server_init ();
      //      com_external_readers_init ();
      //      telnet_initialize (0, ac.sysCfg.reg.telnetLogin.user, ac.sysCfg.reg.telnetLogin.pass);
      //      tcp_client_init ();
      //      web_socket_init ();
      //      command_processor_init ();
      netData.nIfaces = 0;
      netData.iface = NULL;
      SET_NET_STATE (NET_STATE_WAIT_STACK_INIT);
      break;
    case NET_STATE_WAIT_STACK_INIT:
      switch (TCPIP_STACK_Status (sysObj.tcpip))
        {
        case SYS_STATUS_ERROR_EXTENDED:
        case SYS_STATUS_ERROR:
        case SYS_STATUS_READY_EXTENDED:
          _l ("(NET) TCP/IP Stack initialization failed (%d)!!\n", (int) TCPIP_STACK_Status (sysObj.tcpip));
          SET_NET_STATE (NET_STATE_ERROR);
          break;
        case SYS_STATUS_UNINITIALIZED:
        case SYS_STATUS_BUSY:
          break;
        case SYS_STATUS_READY:
          telnet_server_start (23, "atis", "4t1s");
          tcp_server_init ();
          tcp_client_init ();
          ws_client_init ();
          //ifaces
          netData.nIfaces = TCPIP_STACK_NumberOfNetworksGet ();
          if (netData.nIfaces)
            {
              _l ("(NET) Ifaces: %d\n", netData.nIfaces);
              netData.iface = dmem_create (sizeof (t_IFACE_DATA) * netData.nIfaces);
              if (netData.iface)
                {
                  for (i = 0; i < netData.nIfaces; i++)
                    {
                      netData.iface[i].handle = TCPIP_STACK_IndexToNet (i);
                      netData.iface[i].state = IFACE_STATE_INIT;
                    }
                  //              if (TCPIP_SNTP_ConnectionParamSet (netData.iface.handle, TCPIP_NTP_DEFAULT_CONNECTION_TYPE, ac.sysCfg.reg.hostNtp) == SNTP_RES_OK)
                  //                {
                  //                  _l ("(NET) NTP host set to %s\n", ac.sysCfg.reg.hostNtp);
                  //                }
                  //              else
                  //                {
                  //                  _l ("(NET) NTP host setting error. Default server is (%s)\n", TCPIP_NTP_SERVER);
                  //                }
                  TCPIP_SNTP_HandlerRegister (cb_ntp);
                  //              SET_TCP_STATE (NET_STATE_WAIT_FOR_LINK);
                  SET_NET_STATE (NET_STATE_READY);
                }
              else
                {
                  _l ("(NET) Ifaces data error. HALT!\n");
                  SET_NET_STATE (NET_STATE_HALT);
                }
            }
          break;
        }
      break;
      //    case NET_STATE_WAIT_FOR_LINK: // Wait for ethernet link...
      //      if (TCPIP_STACK_NetIsReady (netData.iface.handle))
      //        {
      //          netData.iface.ipAddr.Val = TCPIP_STACK_NetAddress (netData.iface.handle);
      //          _l ("(NET) Link. Local IP address: %d.%d.%d.%d (READY)\n", netData.iface.ipAddr.v[0], netData.iface.ipAddr.v[1], netData.iface.ipAddr.v[2], netData.iface.ipAddr.v[3]);
      //          TCPIP_SNTP_Enable ();
      //          _l ("(NET) NTP service enabled.\n");
      //          SET_TCP_STATE (NET_STATE_READY);
      //        }
      //      break;
    case NET_STATE_READY:
      //      if (!TCPIP_STACK_NetIsReady (netData.iface.handle))
      //        {
      //          _l ("(NET) No link. Closing...\n");
      //          reset_all ();
      //          SET_TCP_STATE (NET_STATE_WAIT_FOR_LINK);
      //        }
      for (i = 0; i < netData.nIfaces; i++)
        iface_task (&netData.iface[i]);
      //tasks
      tcp_server_task ();
      tcp_client_task ();
      ws_client_task ();
      break;
    case NET_STATE_ERROR:
      _l ("(NET) STATE ERROR. Halt!\n");
      reset_all ();
      //      _l ("(NET) NTP service disabled.\n");
      SET_NET_STATE (NET_STATE_HALT);
    case NET_STATE_HALT:
      break;
    }

  return netData.state;
}

t_NET_STATES net_get_state (void)
{
  return netData.state;
}

int net_set_ntp_host (char *ntpHost)
{
  //  if (TCPIP_SNTP_ConnectionParamSet (netData.iface.handle, IP_ADDRESS_TYPE_IPV4, ntpHost) == SNTP_RES_OK)
  //    {
  //      _l ("(NET-NTP) Server set to %s\n", ntpHost);
  //      TCPIP_SNTP_ConnectionInitiate ();
  return 0;
  //    }
  //  else
  //    {
  //      _l ("(NET-NTP) ERROR. Server could not set to %s\n", ntpHost);
  //      return -1; //error
  //    }
}

/* ************************************************************** End of File */
