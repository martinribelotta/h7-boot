#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <_ansi.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <reent.h>

extern int stdio_getchar(void);
extern void stdio_putchar(char c);

_ssize_t _read_r(struct _reent *r, int fd, void *buf, size_t nbytes)
{
    if (fd != STDIN_FILENO) {
        __errno_r(r) = ENODEV;
        return -1;
    }

    _ssize_t readed = 0;

    while (readed < nbytes) {
        int c = stdio_getchar();
        if (c == -1) {
            break;
        }
        readed++;
    }

    return readed;
}

_ssize_t _write_r(struct _reent *r, int fd, const void *buf, size_t nbytes)
{
    if (fd != STDOUT_FILENO && fd != STDERR_FILENO) {
        __errno_r(r) = ENODEV;
        return -1;
    }

    const uint8_t *ptr = buf;
    _ssize_t writed = 0;
    while (writed < nbytes) {
        stdio_putchar(ptr[writed++]);
    }

    return writed;
}
