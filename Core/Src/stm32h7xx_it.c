/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    stm32h7xx_it.c
 * @brief   Interrupt Service Routines.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32h7xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define printf iprintf

/* Bit masking. */
#define CHECK_BIT(REG, POS) ((REG) & (1u << (POS)))

/* Hard Fault Status Register. */
#define FORCED ((uint8_t)30u)
#define VECTTBL ((uint8_t)1u)

/* MemManage Fault Status Register (MMFSR; 0-7 bits in CFSR). */
#define MMARVALID ((uint8_t)7u)
#define MLSPERR ((uint8_t)5u) /**< Only on ARM Cortex-M4F. */
#define MSTKERR ((uint8_t)4u)
#define MUNSTKERR ((uint8_t)3u)
#define DACCVIOL ((uint8_t)1u)
#define IACCVIOL ((uint8_t)0u)

/* Bus Fault Status Register (BFSR; 8-15 bits in CFSR). */
#define BFARVALID ((uint8_t)15u)
#define LSPERR ((uint8_t)13u) /**< Only on ARM Cortex-M4F. */
#define STKERR ((uint8_t)12u)
#define UNSTKERR ((uint8_t)11u)
#define IMPRECISERR ((uint8_t)10u)
#define PRECISERR ((uint8_t)9u)
#define IBUSERR ((uint8_t)8u)

/* Usage Fault Status Register (BFSR; 16-25 bits in CFSR). */
#define DIVBYZERO ((uint8_t)25u) /**< Has to be enabled in CCR. */
#define UNALIGNED ((uint8_t)24u) /**< Has to be enabled in CCR. */
#define NOCP ((uint8_t)19u)
#define INVPC ((uint8_t)18u)
#define INVSTATE ((uint8_t)17u)
#define UNDEFINSTR ((uint8_t)16u)
/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern HCD_HandleTypeDef hhcd_USB_OTG_HS;
/* USER CODE BEGIN EV */
void ReportHardFault(uint32_t *stack_frame, uint32_t exc)
{
    uint32_t r0 = stack_frame[0];
    uint32_t r1 = stack_frame[1];
    uint32_t r2 = stack_frame[2];
    uint32_t r3 = stack_frame[3];
    uint32_t r12 = stack_frame[4];
    uint32_t lr = stack_frame[5];
    uint32_t pc = stack_frame[6];
    uint32_t psr = stack_frame[7];
    uint32_t hfsr = SCB->HFSR;
    uint32_t cfsr = SCB->CFSR;
    uint32_t mmar = SCB->MMFAR;
    uint32_t bfar = SCB->BFAR;
    uint32_t afsr = SCB->AFSR;

    printf("\n!!!Hard Fault detected!!!\n");

    printf("\nStack frame:\n");
    printf("R0 :        0x%08lX\n", r0);
    printf("R1 :        0x%08lX\n", r1);
    printf("R2 :        0x%08lX\n", r2);
    printf("R3 :        0x%08lX\n", r3);
    printf("R12:        0x%08lX\n", r12);
    printf("LR :        0x%08lX\n", lr);
    printf("PC :        0x%08lX\n", pc);
    printf("PSR:        0x%08lX\n", psr);

    printf("\nFault status:\n");
    printf("HFSR:       0x%08lX\n", hfsr);
    printf("CFSR:       0x%08lX\n", cfsr);
    printf("MMAR:       0x%08lX\n", mmar);
    printf("BFAR:       0x%08lX\n", bfar);
    printf("AFSR:       0x%08lX\n", afsr);

    printf("\nOther:\n");
    printf("EXC_RETURN: 0x%08lX\n", exc);

    printf("\nDetails of the fault status:\n");
    printf("Hard fault status:\n");
    if (CHECK_BIT(hfsr, FORCED)) {
        printf(" - Forced Hard fault.\n");
    }
    if (CHECK_BIT(hfsr, VECTTBL)) {
        printf(" - Bus fault on vector table read.\n");
    }
    printf("MemManage fault status:\n");
    if (CHECK_BIT(cfsr, MMARVALID)) {
        printf(" - MMAR holds a valid address: 0x%08lX\n", mmar);
    } else {
        printf(" - MMAR holds an invalid address.\n");
    }
    if (CHECK_BIT(cfsr, MLSPERR)) {
        printf(" - Fault occurred during floating-point lazy state preservation.\n");
    }
    if (CHECK_BIT(cfsr, MSTKERR)) {
        printf(" - Stacking has caused an access violation.\n");
    }
    if (CHECK_BIT(cfsr, MUNSTKERR)) {
        printf(" - Unstacking has caused an access violation.\n");
    }
    if (CHECK_BIT(cfsr, DACCVIOL)) {
        printf(" - Load or store at a location that does not permit the operation.\n");
    }
    if (CHECK_BIT(cfsr, IACCVIOL)) {
        printf(" - Instruction fetch from a location that does not permit execution.\n");
    }
    printf("Bus fault status:\n");
    if (CHECK_BIT(cfsr, BFARVALID)) {
        printf(" - BFAR holds a valid address: 0x%08lX\n", bfar);
    } else {
        printf(" - BFAR holds an invalid address.\n");
    }
    if (CHECK_BIT(cfsr, LSPERR)) {
        printf(" - Fault occurred during floating-point lazy state preservation.\n");
    }
    if (CHECK_BIT(cfsr, STKERR)) {
        printf(" - Stacking has caused a Bus fault.\n");
    }
    if (CHECK_BIT(cfsr, UNSTKERR)) {
        printf(" - Unstacking has caused a Bus fault.\n");
    }
    if (CHECK_BIT(cfsr, IMPRECISERR)) {
        printf(" - Data bus error has occurred, but the return address in the stack is not related to the fault.\n");
    }
    if (CHECK_BIT(cfsr, PRECISERR)) {
        printf(" - Data bus error has occurred, and the return address points to the instruction that caused the "
               "fault.\n");
    }
    if (CHECK_BIT(cfsr, IBUSERR)) {
        printf(" - Instruction bus error.\n");
    }
    printf("Usage fault status:\n");
    if (CHECK_BIT(cfsr, DIVBYZERO)) {
        printf(" - The processor has executed an SDIV or UDIV instruction with a divisor of 0.\n");
    }
    if (CHECK_BIT(cfsr, UNALIGNED)) {
        printf(" - The processor has made an unaligned memory access.\n");
    }
    if (CHECK_BIT(cfsr, NOCP)) {
        printf(" - Attempted to access a coprocessor.\n");
    }
    if (CHECK_BIT(cfsr, INVPC)) {
        printf(" - Illegal attempt to load of EXC_RETURN to the PC.\n");
    }
    if (CHECK_BIT(cfsr, INVSTATE)) {
        printf(" - Attempted to execute an instruction that makes illegal use of the EPSR.\n");
    }
    if (CHECK_BIT(cfsr, INVSTATE)) {
        printf(" - The processor has attempted to execute an undefined instruction.\n");
    }
    printf("End Of report\r\n");

    __DSB();
    __ISB();
    /* Breakpoint. */
    __asm volatile("BKPT #0");

    HAL_NVIC_SystemReset();

    /* Infinite loop to stop the execution. */
    while (1)
        ;
}

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */

  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */
    __asm__ volatile("TST    LR, #0b0100;      "
                     "ITE    EQ;               "
                     "MRSEQ  R0, MSP;          "
                     "MRSNE  R0, PSP;          "
                     "MOV    R1, LR;           "
                     "B      ReportHardFault;  ");
  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32H7xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32h7xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles USB On The Go HS global interrupt.
  */
void OTG_HS_IRQHandler(void)
{
  /* USER CODE BEGIN OTG_HS_IRQn 0 */

  /* USER CODE END OTG_HS_IRQn 0 */
  HAL_HCD_IRQHandler(&hhcd_USB_OTG_HS);
  /* USER CODE BEGIN OTG_HS_IRQn 1 */

  /* USER CODE END OTG_HS_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
