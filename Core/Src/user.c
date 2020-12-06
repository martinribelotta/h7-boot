#include "microrl.h"
#include "shell.h"
#include "stdio_serial.h"

static microrl_t mrl;

static void microrl_print(const char *s)
{
    while (*s)
        stdio_putchar(*s++);
}

static void print_sysinfo(void)
{
    static const char *const fake_argv[] = {"info"};
    shell_execute(1, fake_argv);
}

void USER_setup(void)
{
    print_sysinfo();
    microrl_init(&mrl, microrl_print);
    microrl_set_execute_callback(&mrl, shell_execute);
}

void USER_loop(void)
{
    if (stdio_rx_not_empty()) {
        microrl_insert_char(&mrl, stdio_getchar());
    }
}
