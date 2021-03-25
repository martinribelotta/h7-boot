#ifndef __RCC_H__
#define __RCC_H__


#include <stm32h7xx_hal.h>

#define RCC_SECTION __attribute__((section(".rcc_init"), used))

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rcc_init_s {
    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;
} rcc_init_t;

extern void rcc_initialize(const rcc_init_t *config);

#ifdef __cplusplus
}
#endif

#endif // __RCC_H__
