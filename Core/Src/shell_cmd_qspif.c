#include "shell.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "quadspi.h"
#include "qspi.h"

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

static int testQSPI(int argc, const char *const *argv)
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

static int cmd(int argc, const char *const *argv)
{
    printf("TODO: Implement me\n");
    return 0;
}

static SHELL_CMD(qspi, "QSPI Flash commands", cmd);
static SHELL_CMD(testqspi, "Perform a QSPI test", testQSPI);
