#include "shell.h"

#include "ff_gen_drv.h"
#include "sd_diskio.h"
#include "main.h"
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

static int cmd_load(int argc, const char *const *argv)
{
    if (argc < 3) {
        puts("usage: load <file> <addr>");
        return -1;
    }
    uint32_t addr;
    if (sscanf(argv[2], "%lx", &addr) != 1) {
        printf("Cannot decode %s\n", argv[2]);
        return -1;
    }
    static FATFS fs;
    if (f_mount(&fs, SDPath, 1) != FR_OK) {
        puts("SD not ready");
        return -1;
    }
    static FIL fd;
    if (f_open(&fd, argv[1], FA_READ) != FR_OK) {
        f_mount(NULL, SDPath, 1);
        printf("Error open file %s\n", argv[1]);
        return -1;
    }
    uint32_t readed = 0;
    void *ptr = (void *)addr;
    puts("Reading file...");
    while (readed < f_size(&fd)) {
        UINT chunk_readed;
        if (f_read(&fd, ptr, 1024, &chunk_readed) != FR_OK) {
            printf("Error reading file at %ld", readed);
            f_close(&fd);
            f_mount(NULL, SDPath, 1);
            return -2;
        }
        ptr += chunk_readed;
        readed += chunk_readed;
    }
    printf("Readed %ld bytes from %s\n", readed, argv[1]);
    f_close(&fd);
    f_mount(NULL, SDPath, 1);
    return 0;
}

static SHELL_CMD(ls, "list filesystem", cmd_ls);
static SHELL_CMD(load, "Load file in RAM", cmd_load);
