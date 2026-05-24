#include <stdint.h>
#include <stdarg.h>
#define UART_DR (*(volatile uint32_t*)0x40000000)

void uart_putc(char c);
void uart_puts(const char* s);
static void uart_put_uint(uint32_t value);
static void uart_put_hex(uint32_t value);
void hlprintf(const char* fmt, ...);