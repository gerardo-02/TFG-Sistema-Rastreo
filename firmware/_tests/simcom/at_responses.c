/* 
 * File:   at_responses.c
 * Author: jtrodriguez
 *
 * Created on 2 de marzo de 2021, 9:17
 */

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "simcom.h"
#include "at_responses.h"
//#include "log_console.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */
#define HEADER_LIST_LENGTH          (sizeof(header_list)/sizeof(t_HEADER_LIST))

#define NO_ARGUMENTS_MANAGER        0
#define ARGUMENTS_MANAGER_1         1
#define ARGUMENTS_MANAGER_2         2

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Data types & Definitions                                          */
/* ************************************************************************** */

/* ************************************************************************** */
typedef struct
{
  char* header;
  uint32_t length;
  uint32_t argManager;
} t_HEADER_LIST;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Global data & variables                                           */
/* ************************************************************************** */
/* ************************************************************************** */
extern t_SIMCOM_DATA simcomData;

static const t_HEADER_LIST header_list[] = {
  {AT_RES_CCLK, AT_RES_STR_LEN (AT_RES_CCLK), ARGUMENTS_MANAGER_1},
  {AT_RES_CGEV, AT_RES_STR_LEN (AT_RES_CGEV), ARGUMENTS_MANAGER_1},
  {AT_RES_CGPS, AT_RES_STR_LEN (AT_RES_CGPS), ARGUMENTS_MANAGER_1},
  {AT_RES_CGPSINFO, AT_RES_STR_LEN (AT_RES_CGPSINFO), ARGUMENTS_MANAGER_1},
  {AT_RES_CGREG, AT_RES_STR_LEN (AT_RES_CGREG), ARGUMENTS_MANAGER_1},
  {AT_RES_CICCID, AT_RES_STR_LEN (AT_RES_CICCID), ARGUMENTS_MANAGER_1},
  {AT_RES_CIPCLOSE, AT_RES_STR_LEN (AT_RES_CIPCLOSE), ARGUMENTS_MANAGER_1},
  {AT_RES_CIPERROR, AT_RES_STR_LEN (AT_RES_CIPERROR), ARGUMENTS_MANAGER_1},
  {AT_RES_CIPEVENT, AT_RES_STR_LEN (AT_RES_CIPEVENT), ARGUMENTS_MANAGER_1},
  {AT_RES_CIPOPEN, AT_RES_STR_LEN (AT_RES_CIPOPEN), ARGUMENTS_MANAGER_1},
  {AT_RES_CIPSEND, AT_RES_STR_LEN (AT_RES_CIPSEND), ARGUMENTS_MANAGER_1},
  {AT_RES_CTZV, AT_RES_STR_LEN (AT_RES_CTZV), ARGUMENTS_MANAGER_1},
  {AT_RES_CLIENT, AT_RES_STR_LEN (AT_RES_CLIENT), ARGUMENTS_MANAGER_1},
  {AT_RES_CME_ERROR, AT_RES_STR_LEN (AT_RES_CME_ERROR), ARGUMENTS_MANAGER_1},
  {AT_RES_CMS_ERROR, AT_RES_STR_LEN (AT_RES_CMS_ERROR), ARGUMENTS_MANAGER_1},
  {AT_RES_CNSMOD, AT_RES_STR_LEN (AT_RES_CNSMOD), ARGUMENTS_MANAGER_1},
  {AT_RES_CPIN, AT_RES_STR_LEN (AT_RES_CPIN), ARGUMENTS_MANAGER_1},
  {AT_RES_CPSI, AT_RES_STR_LEN (AT_RES_CPSI), ARGUMENTS_MANAGER_1},
  {AT_RES_CREG, AT_RES_STR_LEN (AT_RES_CREG), ARGUMENTS_MANAGER_1},
  {AT_RES_CSQ, AT_RES_STR_LEN (AT_RES_CSQ), ARGUMENTS_MANAGER_1},
  {AT_RES_IP_ERROR, AT_RES_STR_LEN (AT_RES_IP_ERROR), ARGUMENTS_MANAGER_1},
  {AT_RES_IPADDR, AT_RES_STR_LEN (AT_RES_IPADDR), ARGUMENTS_MANAGER_1},
  {AT_RES_IPCLOSE, AT_RES_STR_LEN (AT_RES_IPCLOSE), ARGUMENTS_MANAGER_1},
  {AT_RES_NET_CLOSE, AT_RES_STR_LEN (AT_RES_NET_CLOSE), ARGUMENTS_MANAGER_1},
  {AT_RES_NET_OPEN, AT_RES_STR_LEN (AT_RES_NET_OPEN), ARGUMENTS_MANAGER_1},
  {RECEIVE_HEADER, AT_RES_STR_LEN (RECEIVE_HEADER), ARGUMENTS_MANAGER_2},
  {AT_RES_SERVERSTART, AT_RES_STR_LEN (AT_RES_SERVERSTART), ARGUMENTS_MANAGER_1},
  {AT_RES_SERVERSTOP, AT_RES_STR_LEN (AT_RES_SERVERSTOP), ARGUMENTS_MANAGER_1},
  {AT_RES_SIMCARD, AT_RES_STR_LEN (AT_RES_SIMCARD), ARGUMENTS_MANAGER_1},
  {AT_RES_ERROR, AT_RES_STR_LEN (AT_RES_ERROR), NO_ARGUMENTS_MANAGER},
  {AT_RES_OK, AT_RES_STR_LEN (AT_RES_OK), NO_ARGUMENTS_MANAGER},
};

static RESPONSE_CALL_BACK responseCallBack[HEADER_LIST_LENGTH];
static t_RESPONSE_CALL_BACK_CONTEXT responseCtx;
static t_SIMCOM_RECEIVE_CALL_BACK_CONTEXT receiveCtx;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Callback Functions                                                */
/* ************************************************************************** */

/* ************************************************************************** */


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Local Functions                                                   */
/* ************************************************************************** */

/* ************************************************************************** */
static uint32_t response_parse_context (char* response, t_RESPONSE_CALL_BACK_CONTEXT *ctx)
{
  ctx->nArgs = 0;
  ctx->responseHeader = response = strtok (response, " ");
  while (response != NULL)
    {
      response = strtok (NULL, "\",");
      if (response)
        {
          ctx->arg[ctx->nArgs++] = response;
        }
    }

  return ctx->nArgs;
}

static uint32_t receive_parse_context (char* receive, t_SIMCOM_RECEIVE_CALL_BACK_CONTEXT *ctx)
{
  receive = strtok (receive, ",");
  if (receive)
    {
      receive = strtok (NULL, ",");
      if (receive)
        {
          ctx->socketIndex = atoi (receive);
          receive = strtok (NULL, ",");
          if (receive)
            {
              ctx->length = atoi (receive);
              ctx->message = receive + (strlen (receive) + 1);
            }
        }
    }

  return ctx->length;
}

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */
void response_init (void)
{
  uint32_t i;
  for (i = 0; i < HEADER_LIST_LENGTH; i++)
    {
      responseCallBack[i] = NULL;
    }
//  logf ("(SIMCOM-RESPONSES) Init. (%d) Callbacks initialized\n", HEADER_LIST_LENGTH);
}

int32_t response_header_search_index (char *header)
{
  uint32_t i = 0;
  bool finded = false;

  //TODO: Mejorar la eficiencia de esta busqueda!!!
  while ((i < HEADER_LIST_LENGTH) && !finded)
    {
      if (memcmp (header, header_list[i].header, header_list[i].length) == 0)
        {
          finded = true;
        }
      else
        {
          i++;
        }
    }
  return i;
}

void response_manage (char* response)
{
  uint32_t i;

  i = response_header_search_index (response);
  if (i < HEADER_LIST_LENGTH)
    {
      switch (header_list[i].argManager)
        {
        case NO_ARGUMENTS_MANAGER:
          if (responseCallBack[i])
            {
              responseCtx.nArgs = 0;
              responseCtx.responseHeader = response;
              responseCallBack[i](&responseCtx);
            }
          break;
        case ARGUMENTS_MANAGER_1:
          if (responseCallBack[i])
            {
              response_parse_context (response, &responseCtx);
              // <editor-fold defaultstate="collapsed" desc="DEBUG, QUITAR!!!">
//              uint32_t a;
//              for (a = 0; a < responseCtx.nArgs; a++)
//                {
//                  dbgf ("  %d: %s\n", a, responseCtx.arg[a]);
//                }
              // </editor-fold>                
              responseCallBack[i](&responseCtx);
            }
          break;
        case ARGUMENTS_MANAGER_2:
          if (simcomData.cfg.receiveCallBack)
            {
              receive_parse_context (response, &receiveCtx);
              if (receiveCtx.length)
                {
                  // <editor-fold defaultstate="collapsed" desc="DEBUG, QUITAR!!!">
//                  uint32_t a;
//                  dbgf ("_message (%d), from (%d): ", receiveCtx.length, receiveCtx.socketIndex);
//                  for (a = 0; a < receiveCtx.length; a++)
//                    {
//                      dbgf (" %02X", *(receiveCtx.message + a));
//                    }
//                  dbgf ("\n");
                  // </editor-fold>      
                  simcomData.cfg.receiveCallBack (&receiveCtx);
                }
            }
          else
            {
//              logf ("(SIMCOM-RESPONSE) WARNING! Receive call-back NOT configured.\n");
            }
          break;
        default:
          // Arguments manager not defined!
          break;
        }
    }
  else
    {
//      logf ("Unknown message: %s\n", response);
    }
}

uint32_t response_set_call_back (char* responseHeader, RESPONSE_CALL_BACK cb)
{
  uint32_t i;

  i = response_header_search_index (responseHeader);
  if (i < HEADER_LIST_LENGTH)
    {
//      logf ("(AT RESPONSE) Set call back for %s. (%d)\n", responseHeader, i);
      responseCallBack[i] = cb;
    }

  return i;
}

/* ************************************************************** End of File */
