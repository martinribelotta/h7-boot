#include "shell.h"

#include "stm32h7xx.h"
#include "ansi_esc.h"

#include <stdio.h>

static int cmd_info(int argc, const char *const *argv)
{
    printf("\n" ANSI_COLOR_CLEAR ANSI_FCOLOR_CYAN "** stm32h750 monitor **\n" ANSI_COLOR_CLEAR
           "Compiled at " ANSI_FCOLOR_GREEN __DATE__ ANSI_FCOLOR_MAGENTA " " __TIME__ ANSI_COLOR_CLEAR "\n"
           "with gcc version " ANSI_FCOLOR_YELLOW __VERSION__ ANSI_COLOR_CLEAR "\n");
    return 0;
}

static int cmd_reset(int argc, const char *const *argv)
{
    puts("Reset system");
    fflush(stdout);
    HAL_Delay(100);
    NVIC_SystemReset();
    while (1) {
    }
    return 0;
}

static SHELL_CMD(reset, "Reset board", cmd_reset);
static SHELL_CMD(info, "Show system info", cmd_info);
