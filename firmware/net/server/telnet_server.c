#include <stdint.h>

#include "tcpip/tcpip.h"
#include "tcpip/telnet.h"

#include "log.h"

#include "telnet_server.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */
#define TELNET_SERVER_MAX_USER_SIZE   24
#define TELNET_SERVER_MAX_PASS_SIZE   24  

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Data types & Definitions                                          */
/* ************************************************************************** */

/* ************************************************************************** */
typedef struct
{
  char user[TELNET_SERVER_MAX_USER_SIZE];
  char pass[TELNET_SERVER_MAX_PASS_SIZE];
} t_TELNET_SERVER_USER_PASS;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Global data & variables                                           */
/* ************************************************************************** */
/* ************************************************************************** */
static TCPIP_TELNET_HANDLE telnetHandle;
static t_TELNET_SERVER_USER_PASS up;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Callback Functions                                                */
/* ************************************************************************** */

/* ************************************************************************** */
static bool authHandler (const char* user, const char* password, const TCPIP_TELNET_CONN_INFO* pInfo, const void* hParam)
{
  t_TELNET_SERVER_USER_PASS *userPass = (t_TELNET_SERVER_USER_PASS*) hParam;

  //  if (pInfo)
  //    {
  //      _l ("(TELNET-SERVER) Incomming connection. "
  //                         "id=%d, pSk=%d, tSk=%d, sta=%d, lip=%d, lp=%d, rip=%d, rp=%d\n"
  //                         , pInfo->connIx, pInfo->presSkt, pInfo->tcpSkt, pInfo->state, pInfo->tcpInfo.localIPaddress.v4Add.Val, pInfo->tcpInfo.localPort, pInfo->tcpInfo.remoteIPaddress.v4Add.Val, pInfo->tcpInfo.remotePort);
  //      //WARNING: la estructura pInfo no trae NA!!!! sera que esta el telnet en el framework hecho una KAKA!!??
  //    }
  if (hParam)
    {
      if ((memcmp (user, userPass->user, strlen (userPass->user)) == 0) && (memcmp (password, userPass->pass, strlen (userPass->pass)) == 0))
        {
          _l ("(TELNET-SERVER) User: %s, ACCESS OK\n", user);
          return true;
        }
      else
        {
          _l ("(TELNET-SERVER) User: %s, Pass: %s. ACCESS FAIL!\n", user, password);
          return false;
        }
    }
  else
    {
      _l ("(TELNET-SERVER) CALL-BACK ERROR. ACCESS FAIL!\n");
      return false;
    }
}

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Local Functions                                                   */
/* ************************************************************************** */
/* ************************************************************************** */


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */
void telnet_server_start (uint16_t port, char *user, char *pass)
{
  memset (&up, 0, sizeof (t_TELNET_SERVER_USER_PASS));
  strncpy (up.user, user, TELNET_SERVER_MAX_USER_SIZE - 1);
  strncpy (up.pass, pass, TELNET_SERVER_MAX_PASS_SIZE - 1);

  // TODO: habria que poder modificar el puerto...
  telnetHandle = TCPIP_TELNET_AuthenticationRegister (authHandler, &up);
  if (!telnetHandle)
    {
      _l ("(TELNET-SERVER) Error creating telnet port.\n");
    }
  else
    {
      _l ("(TELNET-SERVER) Initialized\n");
    }

}

/* ************************************************************** End of File */
