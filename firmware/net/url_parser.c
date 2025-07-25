#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include "drv/drv_dmem.h"

#include "url_parser.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Data types & Definitions                                          */
/* ************************************************************************** */
/* ************************************************************************** */


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Local Functions                                                   */
/* ************************************************************************** */

/* ************************************************************************** */
static __inline__ int _is_scheme_char (int c)
{
  return (!isalpha (c) && '+' != c && '-' != c && '.' != c) ? 0 : 1;
}

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */
t_URL_DATA *url_parse (char *url)
{
  t_URL_DATA *purl;
  const char *tmpstr;
  const char *curstr;
  uint16_t len;
  uint16_t i;
  uint8_t userpass_flag;
  uint8_t bracket_flag;

  /* Allocate the parsed url storage */
  purl = (t_URL_DATA*) dmem_create (sizeof (t_URL_DATA));
  if (NULL == purl)
      return NULL;
  
  memset (purl, 0, sizeof(t_URL_DATA));
  curstr = url;

  /*
   * <scheme>:<scheme-specific-part>
   * <scheme> := [a-z\+\-\.]+
   *             upper case = lower case for resiliency
   */
  /* Read scheme */
  tmpstr = strchr (curstr, ':');
  if (NULL == tmpstr)
    {
      /* Not found the character */
      url_parse_free (purl);
      return NULL;
    }
  /* Get the scheme length */
  len = tmpstr - curstr;
  /* Check restrictions */
  for (i = 0; i < len; i++)
    {
      if (!_is_scheme_char (curstr[i]))
        {
          /* Invalid format */
          url_parse_free (purl);
          return NULL;
        }
    }
  /* Copy the scheme to the storage */
  purl->scheme = dmem_create (sizeof (char) * (len + 1));
  if (NULL == purl->scheme)
    {
      url_parse_free (purl);
      return NULL;
    }
  (void) strncpy (purl->scheme, curstr, len);
  purl->scheme[len] = '\0';
  /* Make the character to lower if it is upper case. */
  for (i = 0; i < len; i++)
    {
      purl->scheme[i] = tolower (purl->scheme[i]);
    }
  /* Skip ':' */
  tmpstr++;
  curstr = tmpstr;

  /*
   * //<user>:<password>@<host>:<port>/<url-path>
   * Any ":", "@" and "/" must be encoded.
   */
  /* Eat "//" */
  for (i = 0; i < 2; i++)
    {
      if ('/' != *curstr)
        {
          url_parse_free (purl);
          return NULL;
        }
      curstr++;
    }

  /* Check if the user (and password) are specified. */
  userpass_flag = 0;
  tmpstr = curstr;
  while ('\0' != *tmpstr)
    {
      if ('@' == *tmpstr)
        {
          /* Username and password are specified */
          userpass_flag = 1;
          break;
        }
      else if ('/' == *tmpstr)
        {
          /* End of <host>:<port> specification */
          userpass_flag = 0;
          break;
        }
      tmpstr++;
    }

  /* User and password specification */
  tmpstr = curstr;
  if (userpass_flag)
    {
      /* Read username */
      while ('\0' != *tmpstr && ':' != *tmpstr && '@' != *tmpstr)
        {
          tmpstr++;
        }
      len = tmpstr - curstr;
      purl->username = dmem_create (sizeof (char) * (len + 1));
      if (NULL == purl->username)
        {
          url_parse_free (purl);
          return NULL;
        }
      (void) strncpy (purl->username, curstr, len);
      purl->username[len] = '\0';
      /* Proceed current pointer */
      curstr = tmpstr;
      if (':' == *curstr)
        {
          /* Skip ':' */
          curstr++;
          /* Read password */
          tmpstr = curstr;
          while ('\0' != *tmpstr && '@' != *tmpstr)
            {
              tmpstr++;
            }
          len = tmpstr - curstr;
          purl->password = dmem_create (sizeof (char) * (len + 1));
          if (NULL == purl->password)
            {
              url_parse_free (purl);
              return NULL;
            }
          (void) strncpy (purl->password, curstr, len);
          purl->password[len] = '\0';
          curstr = tmpstr;
        }
      /* Skip '@' */
      if ('@' != *curstr)
        {
          url_parse_free (purl);
          return NULL;
        }
      curstr++;
    }

  if ('[' == *curstr)
    {
      bracket_flag = 1;
    }
  else
    {
      bracket_flag = 0;
    }
  /* Proceed on by delimiters with reading host */
  tmpstr = curstr;
  while ('\0' != *tmpstr)
    {
      if (bracket_flag && ']' == *tmpstr)
        {
          /* End of IPv6 address. */
          tmpstr++;
          break;
        }
      else if (!bracket_flag && (':' == *tmpstr || '/' == *tmpstr))
        {
          /* Port number is specified. */
          break;
        }
      tmpstr++;
    }
  len = tmpstr - curstr;
  purl->host = dmem_create (sizeof (char) * (len + 1));
  if (NULL == purl->host || len <= 0)
    {
      url_parse_free (purl);
      return NULL;
    }
  (void) strncpy (purl->host, curstr, len);
  purl->host[len] = '\0';
  curstr = tmpstr;

  /* Is port number specified? */
  if (':' == *curstr)
    {
      curstr++;
      /* Read port number */
      tmpstr = curstr;
      while ('\0' != *tmpstr && '/' != *tmpstr)
        {
          tmpstr++;
        }
      len = tmpstr - curstr;
      purl->port = dmem_create (sizeof (char) * (len + 1));
      if (NULL == purl->port)
        {
          url_parse_free (purl);
          return NULL;
        }
      (void) strncpy (purl->port, curstr, len);
      purl->port[len] = '\0';
      curstr = tmpstr;
    }

  /* End of the string */
  if ('\0' == *curstr)
    {
      return purl;
    }

  /* Skip '/' */
  if ('/' != *curstr)
    {
      url_parse_free (purl);
      return NULL;
    }
  curstr++;

  /* Parse path */
  tmpstr = curstr;
  while ('\0' != *tmpstr && '#' != *tmpstr && '?' != *tmpstr)
    {
      tmpstr++;
    }
  len = tmpstr - curstr;
  purl->path = dmem_create (sizeof (char) * (len + 1));
  if (NULL == purl->path)
    {
      url_parse_free (purl);
      return NULL;
    }
  (void) strncpy (purl->path, curstr, len);
  purl->path[len] = '\0';
  curstr = tmpstr;

  /* Is query specified? */
  if ('?' == *curstr)
    {
      /* Skip '?' */
      curstr++;
      /* Read query */
      tmpstr = curstr;
      while ('\0' != *tmpstr && '#' != *tmpstr)
        {
          tmpstr++;
        }
      len = tmpstr - curstr;
      purl->query = dmem_create (sizeof (char) * (len + 1));
      if (NULL == purl->query)
        {
          url_parse_free (purl);
          return NULL;
        }
      (void) strncpy (purl->query, curstr, len);
      purl->query[len] = '\0';
      curstr = tmpstr;
    }

  /* Is fragment specified? */
  if ('#' == *curstr)
    {
      /* Skip '#' */
      curstr++;
      /* Read fragment */
      tmpstr = curstr;
      while ('\0' != *tmpstr)
        {
          tmpstr++;
        }
      len = tmpstr - curstr;
      purl->fragment = dmem_create (sizeof (char) * (len + 1));
      if (NULL == purl->fragment)
        {
          url_parse_free (purl);
          return NULL;
        }
      (void) strncpy (purl->fragment, curstr, len);
      purl->fragment[len] = '\0';
      curstr = tmpstr;
    }

  return purl;
}

/* Free memory of parsed url */
void url_parse_free (t_URL_DATA *purl)
{
  if (purl)
    {
      if (purl->scheme) dmem_release (purl->scheme);
      if (purl->host) dmem_release (purl->host);
      if (purl->port) dmem_release (purl->port);
      if (purl->path) dmem_release (purl->path);
      if (purl->query) dmem_release (purl->query);
      if (purl->fragment) dmem_release (purl->fragment);
      if (purl->username) dmem_release (purl->username);
      if (purl->password) dmem_release (purl->password);
      dmem_release (purl);
    }
}

/* ************************************************************** End of File */
