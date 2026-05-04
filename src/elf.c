

int _start(void)
{
    int result;

    __asm volatile (
        "mov r0, #5\n"
        "mov r1, #7\n"
        "add r2, r0, r1\n"
        : "=r"(result)
        :
        : "r0", "r1", "r2"
    );

    while (1);
}