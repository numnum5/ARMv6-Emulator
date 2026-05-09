#include <stdint.h>
#include <cassert>

class Registers
{

    public:

        uint32_t& operator[](int n);

        uint32_t operator[](int n) const;
    private:
        uint32_t regs[16];
        
};