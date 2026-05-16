#include "semihosting.h"

static void uart_putc(char c)
{
    UART_DR = (uint32_t)c;
}

static void uart_puts(const char* s)
{
    while (*s)
    {
        uart_putc(*s++);
    }
}

static void uart_put_uint(uint32_t value)
{
    char buffer[10];
    int i = 0;

    if (value == 0)
    {
        uart_putc('0');
        return;
    }

    while (value > 0)
    {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i > 0)
    {
        uart_putc(buffer[--i]);
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