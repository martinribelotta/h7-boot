#include "shell.h"

#include "sd_diskio.h"
#include "sdmmc.h"
#include "fatfs.h"

#include <stdio.h>
#include <stdlib.h>

static int cmd_ls(int argc, const char *const *argv)
{
    static DIR dir;

    printf("SD %s contents:\n", SDPath);
    if (f_mount(&SDFatFS, SDPath, 1) != FR_OK) {
        printf("Error mounting SD\r\n");
        return 0;
    }
    if (f_opendir(&dir, SDPath) == FR_OK) {
        static FILINFO inf;
        while (f_readdir(&dir, &inf) == FR_OK) {
            if (inf.fname[0] == 0)
                break;
            printf("  %s\r\n", inf.fname);
        }
    } else {
        printf("Fail to open SD\r\n");
    }
    if (f_mount(NULL, SDPath, 1) != FR_OK) {
        printf("Error unmounting SD\r\n");
    }
    return 0;
}

static SHELL_CMD(ls, "list filesystem", cmd_ls);