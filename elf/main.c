#include <stdint.h>

/* Simple delay */
void delay(volatile uint32_t count)
{
    while (count--) __asm__("nop");
}

int main(void)
{
    /* Example infinite loop */
    while (1)
    {
        delay(100000);
    }
}