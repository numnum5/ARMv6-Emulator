#include "semihosting.h"

void uart_putc(char c)
{
    UART_DR = c;
}

void uart_puts(const char* s)
{
    while (*s)
    {
        uart_putc(*s++);
    }
}

static void uart_put_uint(uint32_t value)
{
    static const uint32_t divisors[] =
    {
        1000000000,
        100000000,
        10000000,
        1000000,
        100000,
        10000,
        1000,
        100,
        10,
        1
    };

    int started = 0;

    for (int i = 0; i < 10; i++)
    {
        uint32_t divisor = divisors[i];
        uint32_t digit = 0;

        while (value >= divisor)
        {
            value -= divisor;
            digit++;
        }

        if (digit || started || divisor == 1)
        {
            uart_putc('0' + digit);
            started = 1;
        }
    }
}

static void uart_put_hex(uint32_t value)
{
    uart_puts("0x");

    for (int i = 7; i >= 0; i--)
    {
        uint32_t nibble = (value >> (i * 4)) & 0xF;

        if (nibble < 10)
            uart_putc('0' + nibble);
        else
            uart_putc('A' + (nibble - 10));
    }
}

void hlprintf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    while (*fmt)
    {
        if (*fmt != '%')
        {
            uart_putc(*fmt++);
            continue;
        }

        fmt++;

        switch (*fmt)
        {
            case 'c':
            {
                char c = (char)va_arg(args, int);
                uart_putc(c);
                break;
            }

            case 's':
            {
                const char* s = va_arg(args, const char*);
                uart_puts(s);
                break;
            }

            case 'u':
            {
                uint32_t v = va_arg(args, uint32_t);
                uart_put_uint(v);
                break;
            }

            case 'x':
            {
                uint32_t v = va_arg(args, uint32_t);
                uart_put_hex(v);
                break;
            }

            case '%':
            {
                uart_putc('%');
                break;
            }

            default:
            {
                uart_putc('?');
                break;
            }
        }

        fmt++;
    }

    va_end(args);
}