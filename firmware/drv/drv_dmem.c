#include <string.h>

#include "log.h"

#include "drv_dmem.h"

#if defined(DMEM_DEBUG)
static int aaa = 0;
static int fff = 0;
#endif

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */

#if defined(DMEM_DEBUG)

void *dmem_create (size_t size)
{
  void *r = malloc (size);
  if (r)
    {
      aaa++;
      _d ("(D-MEM): create=%X, size=%d (%d)\n", r, (int) size, aaa - fff);
    }
  else
    {
      _d ("(D-MEM): create fail. Unable to create mem for: %d\n", (int) size);
    }

  return r;
}
#endif

void *dmem_create_w_data (uint8_t *data, size_t size)
{
  void *r = malloc (size);
  if (r)
    {
#if defined(DMEM_DEBUG)
      aaa++;
      _d ("(D-MEM): create_data=%X, size=%d (%d)\n", r, (int) size, aaa - fff);
#endif
      memcpy (r, data, size); //Copy data
    }
  else
    {
#if defined(DMEM_DEBUG)
      _d ("(D-MEM): create_data fail. Unable to create_data mem for: %d\n", (int) size);
#endif
    }

  return r;
}

#if defined(DMEM_DEBUG)

void *dmem_extend (void *ptr, size_t newSize)
{
  void *r = realloc (ptr, newSize);
  if (r)
    {
      _d ("(D-MEM): extend=%X, size=%d, before=%X\n", r, (int) newSize, ptr);
    }
  else
    {
      _d ("(D-MEM): extend fail. Unable to extend mem for: %d\n", (int) newSize);
    }

  return r;
}
#endif

#if defined(DMEM_DEBUG)

void dmem_release (void *ptr)
{
  if (ptr)
    {
      fff++;
      _d ("(D-MEM): release=%X (%d)\n", (int) ptr, aaa - fff);
      free (ptr);
    }
  else
    {
      _d ("(D-MEM): Unable to release NULL pointer\n");
    }
}
#endif
