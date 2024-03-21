#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#define MAX_INPUT 1024      // Current Max input size, can be reallocated

void myshLoop() {

    char input[MAX_INPUT];      // Input text array
    int bytes_read;

    printf("Shell started: enjoy!\n");

    while (true) {              // Shell loop
        printf("> ");           // Every new line receives shell "> "
        fflush(stdout);         // Text is output to the shell console

        bytes_read = read(STDIN_FILENO, input, MAX_INPUT);      // read() buffer
        if (bytes_read < 0) {                                   // If there is a problem reading the input buffer, print error
            perror("Error reading the command line");
        }
        
        input[bytes_read - 1] = '\0';           // Remove newline character at the end

        if (strcmp(input, "exit") == 0) {       // If the user inputs "exit" end the shell program loop, exiting the program
            break;
        }
    }
}

int main() {
    // preparation and initialization
    printf("Starting mysh, please wait...\n");

    // shell loop
    myshLoop();

    // cleanup and exiting
    printf("Exiting Shell. Have a nice day!\n");

    return 0;
}