#include <stdint.h>

#define UART_DR (*(volatile unsigned int*)0x40000000)

int main(void)
{
    UART_DR = 'H';
    UART_DR = 'I';
    UART_DR = ' ';
    UART_DR = 'F';
    UART_DR = 'R';
    UART_DR = 'O';
    UART_DR = 'M';
    UART_DR = ' ';
    UART_DR = 'F';
    UART_DR = 'I';
    UART_DR = 'R';
    UART_DR = 'M';    
    UART_DR = 'W';
    UART_DR = 'A';
    UART_DR = 'R';
    UART_DR = 'E';
    UART_DR = '\n';

    asm volatile(
        "ldr r0, =0xDEADBEEF\n"
    );

    while(1);
}