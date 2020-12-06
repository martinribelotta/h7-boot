#include "shell.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "quadspi.h"
#include "qspi.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"
#include "main.h"
#include "fatfs.h"

static void dumphex(const char *label, const uint8_t *ptr, size_t count)
{
    printf("%s:", label);
    for (size_t i = 0; i < count; i++) {
        if ((i % 16) == 0) {
            printf("\n  %04X ", i);
        }
        printf("%02X ", ptr[i]);
    }
    printf("\n");
}

static void fillrand(uint8_t *buf, size_t count)
{
    while (count--)
        *buf++ = rand();
}

static int testqspi(int argc, const char *const *argv)
{
    static uint8_t buf[32];
    static uint8_t buf2[32];
    // Uses noinit section to RAM DFF initialization on every power cycle
    static int seed __attribute__((section(".noinit")));
    srand(seed);
    seed = rand();

    uint8_t st;
    if (QSPI_GetStatus(&hqspi, &st) == QSPI_OK) {
        printf("QSPI status: 0x%02X\n", st);
    } else {
        puts("QSPI fail on get status");
    }

    MX_QUADSPI_Erase(0, sizeof(buf));
    fillrand(buf, sizeof(buf));
    dumphex("write buffer", buf, sizeof(buf));
    MX_QUADSPI_Write(buf, 0, sizeof(buf));
    MX_QUADSPI_Read(buf2, 0, sizeof(buf2));
    dumphex("read buffer", buf2, sizeof(buf2));
    int d = memcmp(buf, buf2, sizeof(buf));
    puts(d == 0 ? "Buffer equal" : "Buffer differ");
    return 0;
}

static int qspimmap(int argc, const char *const *argv)
{
    puts("Changing QSPI flash to memory map mode");
    puts(MX_QUADSPI_EnterMMAP() ? "Memory in MMAP mode" : "MMAP mode fail");
    return 0;
}

static int qspicmd(int argc, const char *const *argv)
{
    puts("Resetting QSPI flash to command mode");
    MX_QUADSPI_ExitMMAP();
    puts("QSPI Flash in command mode now");
    return 0;
}

static int qspir(int argc, const char *const *argv)
{
    if (argc < 3) {
        puts("usage: qspir <addr> <bytes>");
        return -1;
    }
    uint32_t addr, size;
    if (sscanf(argv[1], "%li", &addr) != 1) {
        printf("Cannot parse %s\n", argv[1]);
        return -1;
    }
    if (sscanf(argv[2], "%li", &size) != 1) {
        printf("Cannot parse %s\n", argv[2]);
        return -1;
    }
    uint8_t *buf = malloc(size);
    if (!buf) {
        perror("malloc buffer");
        return -2;
    }
    int ret = 0;
    uint32_t readed = MX_QUADSPI_Read(buf, addr, size);
    if (readed == -1) {
        puts("Fail reading data");
        ret = -2;
    } else {
        dumphex("Memory contents:", buf, readed);
    }
    free(buf);
    return ret;
}

static int qspicat(int argc, const char *const *argv)
{
    if (argc < 3) {
        puts("usage: qspir <addr> <bytes>");
        return -1;
    }
    uint32_t addr, size;
    if (sscanf(argv[1], "%li", &addr) != 1) {
        printf("Cannot parse %s\n", argv[1]);
        return -1;
    }
    if (sscanf(argv[2], "%li", &size) != 1) {
        printf("Cannot parse %s\n", argv[2]);
        return -1;
    }
    uint8_t *buf = malloc(size);
    if (!buf) {
        perror("malloc buffer");
        return -2;
    }
    int ret = 0;
    uint32_t readed = MX_QUADSPI_Read(buf, addr, size);
    if (readed == -1) {
        puts("Fail reading data");
        ret = -2;
    } else {
        fwrite(buf, readed, 1, stdout);
        printf("\n");
    }
    free(buf);
    return ret;
}

static uint32_t round_up_u32(uint32_t v, uint32_t n)
{
    v += n;
    v -= 1;
    v /= n;
    v *= n;
    return v;
}

static uint32_t round_down_u32(uint32_t v, uint32_t n)
{
    return v - (v % n);
}

static int qspiea(int argc, const char *const *argv)
{
    puts("Erasing entire flash");
    puts(MX_QUADSPI_EraseAll() ? "Erase Ok" : "Erase Fail");
    return 0;
}

static int qspie(int argc, const char *const *argv)
{
    puts("TODO: Implement me");
    if (argc < 3) {
        puts("usage: qspir <addr> <bytes>");
        return -1;
    }
    uint32_t addr, size;
    if (sscanf(argv[1], "%li", &addr) != 1) {
        printf("Cannot parse %s\n", argv[1]);
        return -1;
    }
    if (sscanf(argv[2], "%li", &size) != 1) {
        printf("Cannot parse %s\n", argv[2]);
        return -1;
    }
    uint32_t align = QSPI_EraseSize();
    printf("Aligining %ld bytes to %ld with start 0x%08lX\n", size, align, addr);
    size = round_up_u32(size, align);
    addr = round_down_u32(addr, align);
    printf("Erasing %ld bytes from 0x%08lX starting to 0x%08lX\n", size, addr, addr);
    uint32_t erased = MX_QUADSPI_Erase(addr, size);
    printf("Erased %ld bytes\n", erased);
    return 0;
}

static const char *fresult_to_string(FRESULT r)
{
    switch (r) {
    case FR_OK:
        return "Succeeded";
    case FR_DISK_ERR:
        return "A hard error occurred in the low level disk I/O layer";
    case FR_INT_ERR:
        return "Assertion failed";
    case FR_NOT_READY:
        return "The physical drive cannot work";
    case FR_NO_FILE:
        return "Could not find the file";
    case FR_NO_PATH:
        return "Could not find the path";
    case FR_INVALID_NAME:
        return "The path name format is invalid";
    case FR_DENIED:
        return "Access denied due to prohibited access or directory full";
    case FR_EXIST:
        return "Access denied due to prohibited access";
    case FR_INVALID_OBJECT:
        return "The file/directory object is invalid";
    case FR_WRITE_PROTECTED:
        return "The physical drive is write protected";
    case FR_INVALID_DRIVE:
        return "The logical drive number is invalid";
    case FR_NOT_ENABLED:
        return "The volume has no work area";
    case FR_NO_FILESYSTEM:
        return "There is no valid FAT volume";
    case FR_MKFS_ABORTED:
        return "The f_mkfs() aborted due to any problem";
    case FR_TIMEOUT:
        return "Could not get a grant to access the volume within defined "
               "period";
    case FR_LOCKED:
        return "The operation is rejected according to the file sharing policy";
    case FR_NOT_ENOUGH_CORE:
        return "LFN working buffer could not be allocated";
    case FR_TOO_MANY_OPEN_FILES:
        return "Number of open files > _FS_LOCK";
    case FR_INVALID_PARAMETER:
        return "Given parameter is invalid";
    default:
        return "Unknown error";
    }
}

static void print_fresult(const char *msg, FRESULT r)
{
    printf("%s: %s\n", msg, fresult_to_string(r));
}

static int qspicp(int argc, const char *const *argv)
{
    if (argc < 3) {
        puts("usage: qspicp <filename> <addr>");
        return -1;
    }
    uint32_t addr;
    if (sscanf(argv[2], "%li", &addr) != 1) {
        printf("Cannot decode address: %s\n", argv[2]);
        return -1;
    }
    FRESULT r;
    FATFS fs;
    if ((r = f_mount(&fs, SDPath, 1)) != FR_OK) {
        print_fresult("mounting filesystem", r);
        return -2;
    }
    FIL f;
    int ret = 0;
    if ((r = f_open(&f, argv[1], FA_READ)) != FR_OK) {
        ret = -2;
        print_fresult("open file", r);
        goto umount_out;
    }
    size_t bufsize = QSPI_EraseSize();
    uint8_t *cpbuf = malloc(bufsize);
    if (!cpbuf) {
        ret = -3;
        perror("malloc buffer");
        goto fclose_out;
    }

    size_t fileSize = f_size(&f);
    MX_QUADSPI_Erase(addr, fileSize);
    size_t writed = 0;
    while (writed < fileSize) {
        UINT readed;
        if ((r = f_read(&f, cpbuf, bufsize, &readed)) != FR_OK) {
            ret = -4;
            print_fresult("reading file", r);
            goto free_out;
        }
        size_t chunck_w = MX_QUADSPI_Write(cpbuf, addr, readed);
        if (chunck_w == -1) {
            ret = -5;
            printf("Error wiriting chunck at address 0x%08lX\n", addr);
            goto free_out;
        }
        addr += chunck_w;
        writed += chunck_w;
    }
    puts("End of write");
free_out:
    free(cpbuf);
fclose_out:
    f_close(&f);
umount_out:
    f_mount(NULL, SDPath, 1);
    return ret;
}

static SHELL_CMD(qspimmap, "Put QSPI in MMAP mode", qspimmap);
static SHELL_CMD(qspicmd, "Put QSPI in Command mode", qspicmd);
static SHELL_CMD(qspir, "Read QSPI memory: qspir <addr> <bytes>", qspir);
static SHELL_CMD(qspicat, "Dump ascii from QSPI: qspicat <addr> <n>", qspicat);
static SHELL_CMD(qspie, "Erase QSPI sectors: qspie <addr> <bytes>", qspie);
static SHELL_CMD(qspiea, "Erase All QSPI", qspiea);
static SHELL_CMD(testqspi, "Perform a QSPI test", testqspi);
static SHELL_CMD(qspicp, "Copy sd file to QSPI: qspicp <file> <addr>", qspicp);
