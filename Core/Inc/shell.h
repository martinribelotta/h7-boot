#ifndef __SHELL_H__
#define __SHELL_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef int(shell_func_t)(int argc, const char *const *argv);

typedef struct {
    const char *cmd;
    const char *help;
    shell_func_t *func;
} shell_cmd_t;

#define SHELL_CMD_ATTR __attribute__((used, section(".shell_commands")))

#define SHELL_CMD(name, help, func) const shell_cmd_t shell_cmd_##name SHELL_CMD_ATTR = {#name, help, func}

extern int shell_execute(int argc, const char *const *argv);

#ifdef __cplusplus
}
#endif

#endif // __SHELL_H__
