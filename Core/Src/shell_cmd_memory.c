#include "shell.h"

#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>

static int extract_addr_size(int argc, const char *const *argv, uint32_t *addr, uint32_t *size)
{
    if (argc < 3) {
        printf("usage: %s <addr> <size>\n", argv[0]);
        return -1;
    }
    if (sscanf(argv[1], "%lx", addr) != 1) {
        printf("cannot decode %s\n", argv[1]);
        return -1;
    }
    if (sscanf(argv[2], "%ld", size) != 1) {
        printf("cannot decode %s\n", argv[2]);
        return -1;
    }
    return 0;
}

static int write_memory(int argc, const char *const *argv, size_t data_size)
{
    if (argc < 4) {
        printf("usage: %s <addr> <data0> <data1> ... <dataN>\n", argv[0]);
        return -1;
    }
    uint32_t addr;
    if (sscanf(argv[1], "%lx", &addr) != 1) {
        printf("cannot decode %s\n", argv[1]);
        return -1;
    }
    size_t size = argc - 3;
    uint32_t v;
    void *ptr = (void *)addr;
    for (size_t i = 0; i < size; i++) {
        const char *s = argv[3 + i];
        if (sscanf(s, "%lx", &v) != 1) {
            printf("cannot decode %s\n", s);
            return -1;
        }
        memcpy(ptr, &v, data_size);
    }
    return 0;
}

static int cmd_mdb(int argc, const char *const *argv)
{
    uint32_t addr, size;
    if (extract_addr_size(argc, argv, &addr, &size) != 0) {
        return -1;
    }
    const uint8_t *ptr = (const uint8_t *)addr;
    puts("Memory dump");
    for (uint32_t i = 0; i < size; i += sizeof(*ptr)) {
        if ((i % 16) == 0) {
            printf("\n%08lX ", addr + i);
        }
        printf("%02X ", ptr[i]);
    }
    printf("\n");
    return 0;
}

static int cmd_mdh(int argc, const char *const *argv)
{
    uint32_t addr, size;
    if (extract_addr_size(argc, argv, &addr, &size) != 0) {
        return -1;
    }
    const uint16_t *ptr = (const uint16_t *)addr;
    puts("Memory dump");
    for (uint32_t i = 0; i < size; i += sizeof(*ptr)) {
        if ((i % 16) == 0) {
            printf("\n%08lX: ", addr + i);
        }
        printf("%04X ", ptr[i]);
    }
    printf("\n");
    return 0;
}

static int cmd_mdw(int argc, const char *const *argv)
{
    uint32_t addr, size;
    if (extract_addr_size(argc, argv, &addr, &size) != 0) {
        return -1;
    }
    const uint32_t *ptr = (const uint32_t *)addr;
    puts("Memory dump");
    for (uint32_t i = 0; i < size; i += sizeof(*ptr)) {
        if ((i % 16) == 0) {
            printf("\n%08lX ", addr + i);
        }
        printf("%08lX ", ptr[i]);
    }
    printf("\n");
    return 0;
}

static int cmd_mwb(int argc, const char *const *argv)
{
    if (write_memory(argc, argv, sizeof(uint8_t)) != 0) {
        return -1;
    }
    return 0;
}

static int cmd_mwh(int argc, const char *const *argv)
{
    if (write_memory(argc, argv, sizeof(uint8_t)) != 0) {
        return -1;
    }
    return 0;
}

static int cmd_mww(int argc, const char *const *argv)
{
    if (write_memory(argc, argv, sizeof(uint8_t)) != 0) {
        return -1;
    }
    return 0;
}

static int cmd_cat(int argc, const char *const *argv)
{
    uint32_t addr, size;
    if (extract_addr_size(argc, argv, &addr, &size) != 0) {
        return -1;
    }
    fwrite((const void *)addr, size, 1, stdout);
    printf("\n");
    return 0;
}

static SHELL_CMD(mdb, "Memory dump bytes (8b)", cmd_mdb);
static SHELL_CMD(mdh, "Memory dump half (16b)", cmd_mdh);
static SHELL_CMD(mdw, "Memory dump word (32b)", cmd_mdw);
static SHELL_CMD(mwb, "Memory dump bytes (8b)", cmd_mwb);
static SHELL_CMD(mwh, "Memory dump half (16b)", cmd_mwh);
static SHELL_CMD(mww, "Memory dump word (32b)", cmd_mww);
static SHELL_CMD(cat, "Memory string dump", cmd_cat);
