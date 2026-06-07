#include <stdint.h>
#include "semihosting.h"

#define UART_DR  (*(volatile uint32_t*)0x40000000)
#define SYST_CSR (*(volatile uint32_t*)0xE000E010)
#define SYST_RVR (*(volatile uint32_t*)0xE000E014)
#define SYST_CVR (*(volatile uint32_t*)0xE000E018)

uint16_t add(uint16_t a)
{
    uart_putc('a');
    uart_putc('a');
    uart_putc('a');
    uart_putc('b');
    uart_putc('\n');
    return a + 1;
}

int main(void)
{
  // SYST_RVR = 100;
  //   SYST_CVR = 0;
  //   SYST_CSR = 7;
    // add(15);

    asm volatile("svc #0");
    uart_putc('a');
    uart_putc('a');
    while(1);
}

