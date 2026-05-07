#include <stdint.h>

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

/* Vector table */
__attribute__((section(".isr_vector")))
void (* const vector_table[])(void) = {
    (void (*)(void))(&_estack), /* Initial SP */
    Reset_Handler,              /* Reset */
    Default_Handler,            /* NMI */
    Default_Handler,            /* HardFault */
};