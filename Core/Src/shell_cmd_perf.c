#include "shell.h"

#include <stm32h7xx.h>

#include <stdio.h>

static inline void delay(long loops)
{
    do
        asm volatile("" ::: "memory");
    while (--loops);
}

static unsigned long calcbogo(unsigned long loops)
{
    unsigned long ticks = HAL_GetTick();
    delay(loops);
    ticks = HAL_GetTick() - ticks;
    if (ticks < 100) {
        return 0;
    }
    return loops * 1000ULL / ticks;
}

static int cmd_bogomips(int argc, const char *const *argv)
{
    for (unsigned long loops = 1 << 20; loops; loops <<= 1) {
        unsigned long lps = calcbogo(loops);
        if (lps) {
            unsigned long intPart = lps / 1000000;
            unsigned long fracPart = lps % 1000000;
            printf("Loops per sec: %lu - %lu.%02lu BogoMips\n", lps, intPart, fracPart);
            return 0;
        }
    }
    puts("Cannot calculate bogoMIPS");
    return -1;
}

static int cmd_coremark(int argc, const char *const *argv)
{
    extern int core_main(void);
    puts("Running coremark");
    return core_main();
}

static SHELL_CMD(coremark, "Perform Coremark test", cmd_coremark);
static SHELL_CMD(bogomips, "Perform bogomisp test", cmd_bogomips);
