#include "registers.hpp"

uint32_t& Registers::operator[](int n)
{
    assert(n >= 0 && n < 16);

    return regs[n];
}

uint32_t Registers::operator[](int n) const
{
    assert(n >= 0 && n <= 15);

    if (n == 15)
    {
        return this->regs[15] + 4;
    }
    else
    {
        return this->regs[n];
    }
}