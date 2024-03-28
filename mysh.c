#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT 1024                  // Current Max input size, can be reallocated
#define TOKEN_DELIMITER " \t\r\n\a"     // Characters that divide command line arguments

void exitShell(int exitStatus) {
    exit(exitStatus);
}

char* readLine() {
    char* input = malloc(MAX_INPUT * sizeof(char*));
    if(!input) {
        fprintf(stderr, "Error allocating read line space");
        exit(EXIT_FAILURE);
    }

    int bytes_read = read(STDIN_FILENO, input, MAX_INPUT);      // read() buffer
    if (bytes_read < 0) {                                       // If there is a problem reading the input buffer, print error
        fprintf(stderr, "Error reading the command line");
        exitShell(EXIT_FAILURE);            
    }
    
    input[bytes_read - 1] = '\0';           // Remove newline character at the end

    if (strcmp(input, "exit") == 0) {       // If the user inputs "exit" end the shell program loop, exiting the program
        exitShell(EXIT_SUCCESS);
    }

    return input;
}

char **tokenizeInput(char *input) {
    char** tokens = malloc(MAX_INPUT * sizeof(char*));          // Allocate space for tokens array
    if(!tokens) {
        fprintf(stderr, "Error allocating space for tokens");
        exitShell(EXIT_FAILURE);
    }

    char *token;
    int position = 0;
    int bufferSize = MAX_INPUT;

    token = strtok(input, TOKEN_DELIMITER);
    while(token != NULL) {
        tokens[position] = token;           // Assign each token to the array
        position++;                         // Increment the array

        if(position >= bufferSize){         // Arguments too long, reallocate space
            bufferSize += MAX_INPUT;
            tokens = realloc(tokens, bufferSize * sizeof(char*));

            if(!tokens) {              // Error reallocating space
                fprintf(stderr, "Error reallocating token space");
                exitShell(EXIT_FAILURE);
            }
        }           

        token = strtok(NULL, TOKEN_DELIMITER);
    }

    tokens[position] = NULL;                // Last position increment holds no token
    return tokens;
}

void myshLoop() {
    printf("Shell started: enjoy!\n");

    do {                        // Shell loop
        printf("mysh> ");       // Every new line receives shell "> "
        fflush(stdout);         // Text is output to the shell console

        char *line = readLine();       // Assign global input to user input
    
        char **args = tokenizeInput(line);       

        free(line);
        free(args);
    } while(true);
}

int main() {
    // preparation and initialization
    printf("Starting mysh, please wait...\n");

    // shell loop
    myshLoop();

    // cleanup and exiting
    printf("Exiting Shell. Have a nice day!\n");
    exitShell(EXIT_SUCCESS);

    return 0;
}