#include <stdint.h>
#include "semihosting.h"

/* Linker symbols */
extern uint32_t _sdata, _edata, _sidata;
extern uint32_t _sbss, _ebss;
extern uint32_t _estack;

/* Function prototype */

int main(void);

/* Reset handler */
void Reset_Handler(void)
{
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;

    /* Copy .data from FLASH to RAM */
    while (dst < &_edata)
        *dst++ = *src++;

    /* Zero .bss */
    for (dst = &_sbss; dst < &_ebss; )
        *dst++ = 0;

    /* Call main */
    main();

    /* If main returns, loop forever */
    while (1);
}

/* Default handler */
void Default_Handler(void)
{
    while (1);
}


void SVC_Handler(void)
{
    UART_DR = '@';
    UART_DR = '@';
    while (1);
}

void PendSV_Handler(void)
{
    while (1);
}

void SysTick_Handler(void)
{
    UART_DR = 'S';
    UART_DR = 'Y';
    UART_DR = 'S';
    UART_DR = '\n';
    while (1);
}

/* Vector table */
/* Vector table */
__attribute__((section(".isr_vector")))
void (* const vector_table[])(void) = {
    (void (*)(void))(&_estack), /* Initial SP */
    Reset_Handler,              /* Reset */
    Default_Handler,            /* NMI */
    Default_Handler,            /* HardFault */

    0,                          /* MemManage (not used on M0) */
    0,                          /* BusFault  (not used on M0) */
    0,                          /* UsageFault (not used on M0) */
    0,
    0,
    0,
    0,

    SVC_Handler,                /* SVCall */
    0,                          /* DebugMonitor */
    0,                          /* Reserved */
    PendSV_Handler,             /* PendSV */
    SysTick_Handler             /* SysTick */
};