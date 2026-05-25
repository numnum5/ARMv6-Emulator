#include <stdint.h>
// #include "semihosting.h"

#define UART_DR  (*(volatile uint32_t*)0x40000000)
#define SYST_CSR (*(volatile uint32_t*)0xE000E010)
#define SYST_RVR (*(volatile uint32_t*)0xE000E014)
#define SYST_CVR (*(volatile uint32_t*)0xE000E018)

int main(void)
{
        // Enable Systick;
    // SYST_RVR = 100;
    // SYST_CVR = 0;
    // SYST_CSR = 7;

    // uart_puts("HELLO\n");

    hlprintf("Hello\n");  

    // uart_putc('@');

    // uart_putc('I');
    // uart_putc('I');
    // uart_putc('X');
    // UART_DR = 'Z';
    // UART_DR = 'I';
    // UART_DR = ' ';
    // UART_DR = 'F';
    // UART_DR = 'R';
    // UART_DR = 'O';
    // UART_DR = 'M';
    // UART_DR = ' ';
    // UART_DR = 'F';
    // UART_DR = 'I';
    // UART_DR = 'R';
    // UART_DR = 'M';    
    // UART_DR = 'W';
    // UART_DR = 'A';
    // UART_DR = 'R';
    // UART_DR = 'E';
    UART_DR = '\n';

    asm volatile(
        "ldr r0, =0xDEADBEEF\n"
    );

    // asm volatile(
    //     "svc #0\n"
    // );

    while(1);
}

