#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>


// Declare a buffer for user input of size 2048

int main(int argc, char** argv) {

    // Print Version and Exit Information
    puts("lis v0.1.1");
    puts("Press ^C to Exit\n");

    while(1) {
        char* input = readline("> ");
        add_history(input);
        printf("< %s\n", input);
        free (input);
    }

    return 0;
}
