#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "log.h"
#include "drv/drv_dmem.h"

#include "http_request.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */
#define REQUEST_FIXED_LENGHT    2048

const char boundary[] = "---011000010111000001101001";

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Local Functions                                                   */
/* ************************************************************************** */

/* ************************************************************************** */
static t_HTTP_REQUEST_HANDLE *create_request_handle (size_t length)
{
  t_HTTP_REQUEST_HANDLE *hReq;

  hReq = (t_HTTP_REQUEST_HANDLE*) dmem_create (sizeof (t_HTTP_REQUEST_HANDLE));
  if (hReq)
    {
      hReq->length = 0;
      hReq->raw = (char*) dmem_create (length);
      if (!hReq->raw)
        {
          dmem_release (hReq);
          hReq = NULL;
        }
    }

  return hReq;
}

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */
t_HTTP_REQUEST_HANDLE *http_request_init (t_HTTP_METHODS method, char *host, int port, char *path, char *query)
{
  t_HTTP_REQUEST_HANDLE *hReq = NULL;

  switch (method)
    {
    case HTTP_METHOD_POST:
      hReq = create_request_handle (REQUEST_FIXED_LENGHT);
      if (hReq)
        {
          memcpy (hReq->raw, "POST", 4);
          hReq->length = 4;
        }
      break;
    case HTTP_METHOD_GET:
      hReq = create_request_handle (REQUEST_FIXED_LENGHT);
      if (hReq)
        {
          memcpy (hReq->raw, "GET", 3);
          hReq->length = 3;
        }
      break;
    default:
      break;
    }

  if (hReq)
    {
      if (port == 80)
        {
          hReq->length += snprintf (hReq->raw + hReq->length, REQUEST_FIXED_LENGHT - hReq->length,
                                    " /%s HTTP/1.1\r\n"
                                    "Host: %s\r\n"
                                    //"user-agent: pmxAtis\r\n"
                                    "\r\n"
                                    , path, host);
        }
      else
        {
          hReq->length += snprintf (hReq->raw + hReq->length, REQUEST_FIXED_LENGHT - hReq->length,
                                    " /%s HTTP/1.1\r\n"
                                    "Host: %s:%d\r\n"
                                    //"user-agent: pmxAtis\r\n"
                                    "\r\n"
                                    , path, host, port);
        }
    }
  else
    {
      _l ("(HTTP REQUEST) Init handle error!\n");
    }

  return hReq;
}

int http_request_add_generic (t_HTTP_REQUEST_HANDLE *rh, char *label, char *content)
{
  rh->length -= 2; //para sobreescribir el "\r\n" del final. Porque lo volvere a poner al final otra vez.
  rh->length += snprintf (rh->raw + rh->length, REQUEST_FIXED_LENGHT - rh->length,
                          "%s: %s\r\n\r\n"
                          , label, content);
  return 0;
}

int http_request_add_accept (t_HTTP_REQUEST_HANDLE *rh, char *accept)
{
  rh->length -= 2; //para sobreescribir el "\r\n" del final. Porque lo volvere a poner al final otra vez.
  rh->length += snprintf (rh->raw + rh->length, REQUEST_FIXED_LENGHT - rh->length,
                          "accept: %s\r\n\r\n"
                          , accept);
  return 0;
}

int http_request_add_bearer_token (t_HTTP_REQUEST_HANDLE *rh, char *token)
{
  rh->length -= 2; //para sobreescribir el "\r\n" del final. Porque lo volvere a poner al final otra vez.
  rh->length += snprintf (rh->raw + rh->length, REQUEST_FIXED_LENGHT - rh->length,
                          "authorization: Bearer "
                          "%s\r\n\r\n"
                          , token);
  return 0;
}

int http_request_add_content_user_pass (t_HTTP_REQUEST_HANDLE *rh, char *user, char *pass)
{
  uint32_t contentLength;

  rh->length -= 2; //para sobreescribir el "\r\n" del final. Porque lo volvere a poner al final otra vez.
  contentLength = (strlen (boundary)*3) + strlen (user) + strlen (pass) + 116; //Es un fijo de la etiquetas y demas.
  rh->length += snprintf (rh->raw + rh->length, REQUEST_FIXED_LENGHT - rh->length,
                          "content-type: multipart/form-data; boundary=%s\r\n"
                          "content-length: %u\r\n"
                          "\r\n--%s\r\n"
                          "content-disposition: form-data; name=\"user\"\r\n"
                          "\r\n%s\r\n"
                          "--%s\r\n"
                          "content-disposition: form-data; name=\"password\"\r\n"
                          "\r\n%s\r\n"
                          "--%s--\r\n\r\n"
                          , boundary, contentLength, boundary, user, boundary, pass, boundary);
  return 0;
}

void http_request_delete (t_HTTP_REQUEST_HANDLE **handle)
{
  if (*handle)
    {
      if ((*handle)->raw)
        {
          dmem_release ((*handle)->raw);
        }
      dmem_release (*handle);
      *handle = NULL;
    }
}

/* ************************************************************** End of File */
