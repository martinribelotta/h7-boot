#ifndef __STDIO_SERIAL_H__
#define __STDIO_SERIAL_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int stdio_rx_not_empty(void);
extern int stdio_getchar(void);
extern void stdio_putchar(char c);

#ifdef __cplusplus
}
#endif

#endif // __STDIO_SERIAL_H__
