#include <unistd.h>
#include <sys/wait.h>
#include <iostream>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
   
        execlp("arm-none-eabi-gcc",
            "arm-none-eabi-gcc",
            "-mcpu=cortex-m0",
            "-mthumb",
            "-ffreestanding",
            "-nostdlib",
            "main.c",
            "-o",
            "main.elf",
            NULL);
        perror("execlp failed");

        return 1;
    }
    else if (pid > 0) {

        int status;
        waitpid(pid, &status, 0);

        std::cout << "Compilation finished\n";
    }
    else {
        perror("fork failed");
    }

    return 0;
}