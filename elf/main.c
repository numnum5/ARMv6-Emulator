#include <stdint.h>

#define UART_DR (*(volatile unsigned int*)0x40000000)

int main(void)
{
    UART_DR = 'H';
    UART_DR = 'i';
    UART_DR = '\n';

    asm volatile(
        "ldr r0, =0xDEADBEEF\n"
    );

    while(1);
}