#include "storage_eeram.h"
#include "storage_flash.h"

#include "pmz_storage.h"

void storage_init (void)
{
  storage_eeram_init ();
  storage_flash_init ();
}

void storage_task (void)
{
  storage_eeram_task ();
  storage_flash_task ();
}
