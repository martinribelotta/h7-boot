#include "stdio_serial.h"

#include "stm32h7xx_ll_usart.h"

int stdio_rx_not_empty(void)
{
    return LL_USART_IsActiveFlag_RXNE(USART1);
}

int stdio_getchar(void)
{
    return  stdio_rx_not_empty()? LL_USART_ReceiveData8(USART1) : -1;
}

void stdio_putchar(char c)
{
    while (!LL_USART_IsActiveFlag_TXE(USART1))
        ;
    LL_USART_TransmitData8(USART1, c);
}
