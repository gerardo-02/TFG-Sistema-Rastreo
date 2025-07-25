#include <stdio.h>

#include "log.h"

void dump_data (char *rawBytes, size_t nBytes, t_DUMP_MODES mode)
{
  int i;
  size_t rBuffSize;
  char rBuff[5];

  SYS_CONSOLE_Write (SYS_CONSOLE_DEFAULT_INSTANCE, "[DUMP]\r\n", 7);
  for (i = 0; i < nBytes; i++)
    {
      switch (mode)
        {
        case DUMP_MODE_HEX:
          rBuffSize = snprintf (rBuff, sizeof (rBuff), " %02hhX", (unsigned char) rawBytes[i]);
          SYS_CONSOLE_Write (SYS_CONSOLE_DEFAULT_INSTANCE, rBuff, rBuffSize);
          break;
        case DUMP_MODE_CHAR:
          SYS_CONSOLE_Write (SYS_CONSOLE_DEFAULT_INSTANCE, &rawBytes[i], 1);
          break;
        case DUMP_MODE_MIX:
          if ((rawBytes[i] < ' ') || rawBytes[i] > '~')
            {
              rBuffSize = snprintf (rBuff, sizeof (rBuff), "{%02hhX}", (unsigned char) rawBytes[i]);
              SYS_CONSOLE_Write (SYS_CONSOLE_DEFAULT_INSTANCE, rBuff, rBuffSize);
            }
          else
            {
              SYS_CONSOLE_Write (SYS_CONSOLE_DEFAULT_INSTANCE, &rawBytes[i], 1);
            }
        default:
          break;
        }
    }
  SYS_CONSOLE_Write (SYS_CONSOLE_DEFAULT_INSTANCE, "\r\n[END DUMP]\r\n", 12);
}