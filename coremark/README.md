# coremark_stm32_hal
coremark for stm32 MCU, using hal library

steps:

1. with core_portme.h file, change ITERATIONS to an appropriate value.
2. with core_portme.c file, replace specific MCU platform header file, then change uart handle for communication.
3. in you project's main() function, add a function call to core_main(), following peripherals' initialization code.

enjoy!
