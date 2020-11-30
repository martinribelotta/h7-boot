#include "shell.h"

#include "sd_diskio.h"
#include "sdmmc.h"
#include "fatfs.h"

#include <stdio.h>
#include <stdlib.h>

static int cmd_ls(int argc, const char *const *argv)
{
    static DIR dir;
    static FATFS fs;
    printf("SD %s contents:\n", SDPath);
    if (f_mount(&fs, SDPath, 1) != FR_OK) {
        puts("Error mounting SD");
        return -1;
    }
    if (f_opendir(&dir, SDPath) == FR_OK) {
        static FILINFO inf;
        while (f_readdir(&dir, &inf) == FR_OK) {
            if (inf.fname[0] == 0)
                break;
            printf("  %s\n", inf.fname);
        }
    } else {
        puts("Fail to open SD");
        return -1;
    }
    if (f_mount(NULL, SDPath, 1) != FR_OK) {
        puts("Error unmounting SD\n");
        return -1;
    }
    return 0;
}

static SHELL_CMD(ls, "list filesystem", cmd_ls);
