#include <stdio.h>

// Declare a buffer for user input of size 2048
static char input[2048];

int main(int argc, char** argv) {

    // Print Version and Exit Information
    puts("lis v0.1");
    puts("Press ^C to Exit\n");

    while(1) {
        fputs("> ", stdout);
        fgets(input, 2048, stdin);
        printf("< %s", input);
    }

    return 0;
}
