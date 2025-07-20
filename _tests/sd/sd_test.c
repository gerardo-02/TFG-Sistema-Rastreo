/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "system/fs/sys_fs.h"
#include "system/fs/sys_fs_media_manager.h"
#include "log.h"
#include "sd_test.h"

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Constants                                                         */
/* ************************************************************************** */
/* ************************************************************************** */
#define SDCARD_MOUNT_NAME    "/mnt/sd"
#define SDCARD_DEV_NAME      "/dev/mmcblka1"  //Este nombre parece tener algo de misterio. Si pones "/dev/mmcblka" sin el "1" no va!!!. ?¿?¿?¿
#define SDCARD_FILE_NAME     "FILE_TOO_LONG_NAME_EXAMPLE_123.txt"
#define SDCARD_DIR_NAME      "Dir"

#define APP_DATA_LEN         512

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Data types & Definitions                                          */
/* ************************************************************************** */

/* ************************************************************************** */
typedef enum
{
  APP_WAIT_FOR_SD = 0,
  //
  APP_IDLE_SD_PRESENT,
  //
  APP_MOUNT_DISK,
  /* Set the current drive */
  APP_SET_CURRENT_DRIVE,
  /* Create directory */
  APP_CREATE_DIRECTORY,
  /* The app opens the file to write */
  APP_OPEN_FOR_WRITE_FILE,
  /* The app reads from a file and writes to another file */
  APP_WRITE_TO_FILE,
  /* The app opens the file to read */
  APP_OPEN_FOR_READ_FILE,
  /* The app closes the file*/
  APP_CLOSE_FILE,
  /* The app unmounts the disk */
  APP_UNMOUNT_DISK,
  /* An app error has occurred */
  APP_ERROR
} APP_STATES;

typedef struct
{
  /* SYS_FS File handle for file */
  SYS_FS_HANDLE fileHandle;
  /* Application's current state */
  APP_STATES state;
  int32_t nBytesRead;
} APP_DATA;

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Global data & variables                                           */
/* ************************************************************************** */
/* ************************************************************************** */
static APP_DATA appData;
static CACHE_ALIGN uint8_t readWriteBuffer[APP_DATA_LEN];

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


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Interface Functions                                               */
/* ************************************************************************** */

/* ************************************************************************** */
void sd_test_initialize (void)
{
  appData.state = APP_WAIT_FOR_SD;
}

void sd_test_task (void)
{
  switch (appData.state)
    {
    case APP_WAIT_FOR_SD:
      if (SYS_FS_MEDIA_MANAGER_MediaStatusGet (SDCARD_DEV_NAME))
        {
          _l ("(SD-SPI) SD inserted!\n");
          appData.state = APP_IDLE_SD_PRESENT;
        }
      break;
    case APP_IDLE_SD_PRESENT:
      if (!SYS_FS_MEDIA_MANAGER_MediaStatusGet (SDCARD_DEV_NAME))
        {
          _l ("(SD-SPI) No SD!\n");
          appData.state = APP_WAIT_FOR_SD;
        }
      break;
    case APP_MOUNT_DISK:
      _l ("(SD) Try to mount: Device name = %s, Mount name = %s\n", SDCARD_DEV_NAME, SDCARD_MOUNT_NAME);
      if (SYS_FS_Mount (SDCARD_DEV_NAME, SDCARD_MOUNT_NAME, FAT, 0, NULL) != 0)
        {
          /* The disk could not be mounted. Try mounting again until success. */
          _l ("(SD) Mount failure. Mounted?...\n");
          appData.state = APP_UNMOUNT_DISK;
        }
      else
        {
          /* Mount was successful. Unmount the disk, for testing. */
          _l ("(SD) Mount was successful.\n");
          appData.state = APP_SET_CURRENT_DRIVE;
        }
      break;
    case APP_SET_CURRENT_DRIVE:
      _l ("(SD) Set current drive: Mount name = %s\n", SDCARD_MOUNT_NAME);
      if (SYS_FS_CurrentDriveSet (SDCARD_MOUNT_NAME) == SYS_FS_RES_FAILURE)
        {
          /* Error while setting current drive */
          _l ("(SD) Set drive failure. Test end!\n");
          appData.state = APP_ERROR;
        }
      else
        {
          /* Open a file for reading. */
          _l ("(SD) Set drive successful.\n");
          appData.state = APP_CREATE_DIRECTORY;
        }
      break;
    case APP_CREATE_DIRECTORY:
      _l ("(SD) Make directory: Directory name = %s\n", SDCARD_DIR_NAME);
      if (SYS_FS_DirectoryMake (SDCARD_DIR_NAME) == SYS_FS_RES_FAILURE)
        {
          /* Error while creating a new drive */
          _l ("(SD) Make directory failure. Test end!\n");
          appData.state = APP_ERROR;
        }
      else
        {
          /* Open a second file for writing. */
          _l ("(SD) Make directory successful!\n");
          appData.state = APP_OPEN_FOR_WRITE_FILE;
        }
      break;
    case APP_OPEN_FOR_WRITE_FILE:
      /* Open a second file inside "Dir" */
      _l ("(SD) Open file for write: File name = %s\n", SDCARD_DIR_NAME"/"SDCARD_FILE_NAME);
      appData.fileHandle = SYS_FS_FileOpen (SDCARD_DIR_NAME"/"SDCARD_FILE_NAME, (SYS_FS_FILE_OPEN_WRITE));
      if (appData.fileHandle == SYS_FS_HANDLE_INVALID)
        {
          /* Could not open the file. Error out*/
          _l ("(SD) Open file for write failure. Test end!\n");
          appData.state = APP_ERROR;
        }
      else
        {
          /* Read from one file and write to another file */
          _l ("(SD) Open file for write successful!\n");
          appData.state = APP_WRITE_TO_FILE;
        }
      break;
    case APP_WRITE_TO_FILE:
      appData.nBytesRead = snprintf ((char*) readWriteBuffer, APP_DATA_LEN, "Test write into a SD file!");
      _l ("(SD) Try to write into SD: %s\n", readWriteBuffer);
      if (SYS_FS_FileWrite (appData.fileHandle, (const void *) readWriteBuffer, appData.nBytesRead) == -1)
        {
          /* Write was not successful. Close the file and error out.*/
          _l ("(SD) Write file failure. Test end!\n");
          appData.state = APP_ERROR;
        }
      else
        {
          _l ("(SD) Write file successful!\n");
          appData.state = APP_OPEN_FOR_READ_FILE;
        }
      SYS_FS_FileClose (appData.fileHandle);
      break;
    case APP_OPEN_FOR_READ_FILE:
      _l ("(SD) Open file for read: File name = %s\n", SDCARD_DIR_NAME"/"SDCARD_FILE_NAME);
      appData.fileHandle = SYS_FS_FileOpen (SDCARD_DIR_NAME"/"SDCARD_FILE_NAME, (SYS_FS_FILE_OPEN_READ));
      if (appData.fileHandle == SYS_FS_HANDLE_INVALID)
        {
          /* Could not open the file. Error out*/
          _l ("(SD) Open file for read failure. Test end!\n");
          appData.state = APP_ERROR;
        }
      else
        {
          _l ("(SD) Open file for read successful!\n");
          _l ("(SD) Reading data from %s\n", SDCARD_DIR_NAME"/"SDCARD_FILE_NAME);
          appData.nBytesRead = SYS_FS_FileRead (appData.fileHandle, (void *) readWriteBuffer, APP_DATA_LEN);
          if (appData.nBytesRead == -1)
            {
              /* There was an error while reading the file. Close the file and error out. */
              _l ("(SD) Reading failure. Test end!\n");
              SYS_FS_FileClose (appData.fileHandle);
              appData.state = APP_ERROR;
            }
          else
            {
              _l ("(SD) Data readed:\n");
              _dump (readWriteBuffer, appData.nBytesRead, DUMP_MODE_MIX);
              appData.state = APP_CLOSE_FILE;
            }
        }
      break;
    case APP_CLOSE_FILE:
      /* Close file */
      SYS_FS_FileClose (appData.fileHandle);
      /* The test was successful. Lets idle. */
      _l ("(SD) File closed. Complete test successful!\n");
      appData.state = APP_UNMOUNT_DISK;
      break;
    case APP_UNMOUNT_DISK:
      _l ("(SD) Try to unmount: Mount name = %s\n", SDCARD_MOUNT_NAME);
      if (SYS_FS_Unmount (SDCARD_MOUNT_NAME) != 0)
        {
          /* The disk could not be un mounted. Try un mounting again untill success. */
          _l ("(SD) Unmount failure. Test end!\n");
          appData.state = APP_ERROR;
        }
      else
        {
          /* UnMount was successful. Mount the disk again */
          _l ("(SD) Unmount was successful. End test.\n");
          appData.state = APP_IDLE_SD_PRESENT;
        }
      break;
      //
    case APP_ERROR:
      /* The application comes here when the demo has failed. */
      appData.state = APP_WAIT_FOR_SD;
      break;
    default:
      break;
    }
}

void sd_test_testing (void)
{
  switch (appData.state)
    {
    case APP_IDLE_SD_PRESENT:
      _l ("(SD) Initializing test...\n");
      appData.state = APP_MOUNT_DISK;
      break;
    default:
      _l ("(SD) System busy. Test can't be initiated\n");
      break;
    }
}
/* ************************************************************** End of File */


