#include "shell.h"

#include <stdio.h>
#include <string.h>

extern const shell_cmd_t __shell_commands_start;
extern const shell_cmd_t __shell_commands_end;

int shell_execute(int argc, const char *const *argv)
{
    if (argc == 0) {
        return 0;
    }
    const shell_cmd_t *ptr = &__shell_commands_start;
    while (ptr < &__shell_commands_end) {
        if (strcmp(ptr->cmd, argv[0]) == 0) {
            return ptr->func(argc, argv);
        }
        ptr++;
    }
    puts("Unknown command");
    return -1;
}

static int shell_help(int argc, const char *const *argv)
{
    puts("Command list:");
    const shell_cmd_t *ptr = &__shell_commands_start;
    while (ptr < &__shell_commands_end) {
        printf("%-30s %s\n", ptr->cmd, ptr->help);
        ptr++;
    }
    return 0;
}

static SHELL_CMD(help, "Show command list", shell_help);
