#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define MAX_INPUT 1024                  // Current Max input size, can be reallocated
#define TOKEN_DELIMITER " \t\r\n\a"     // Characters that divide command line arguments

bool isBatch = false;

// Built in function to exit the program with a given exit status
int exitShell(int exitStatus) {
    if(!isBatch) {
        printf("Exiting Shell. Have a nice day!\n");
    }
    exit(exitStatus);
}

// Function to read the user input to be used by the program
char* readLine() {
    char* input = malloc(MAX_INPUT * sizeof(char*));            // Allocate space for input
    if(!input) {                                                // If space wasn't allocated, report an error
        fprintf(stderr, "Error allocating read line space");
        exit(EXIT_FAILURE);
    }

    int bytes_read = read(STDIN_FILENO, input, MAX_INPUT);      // read() buffer
    if (bytes_read < 0) {                                       // If there is a problem reading the input buffer, print error
        fprintf(stderr, "Error reading the command line");
        exitShell(EXIT_FAILURE);            
    }
    
    input[bytes_read - 1] = '\0';           // Remove newline character at the end

    return input;
}

// Function to take an input line and tokenize each argument by white space
// Returns an array of these tokenized arguments
char **tokenizeInput(char *input) {
    char** tokens = malloc(MAX_INPUT * sizeof(char*));          // Allocate space for tokens array
    if(!tokens) {                                               // If space was not allocated, report an error
        fprintf(stderr, "Error allocating space for tokens");
        exitShell(EXIT_FAILURE);
    }

    char *token;                            // Individual token
    int position = 0;                       // Incremental counter
    int bufferSize = MAX_INPUT;             // Reallocatable buffer size

    token = strtok(input, TOKEN_DELIMITER); // Tokenize the inputs
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

        token = strtok(NULL, TOKEN_DELIMITER);      // Nullify the final position increment
    }

    tokens[position] = NULL;                        // Last position increment holds no token
    return tokens;
}

// Function to execute any built in commands
// Returns true if the program should continue running
int execute(char **args) {
    if(args[0] == NULL) {           // Empty command, continue
        return true;
    }

    for(int i = 0; i < sizeof(args); i++) {
        if(strcmp(args[i], "exit") == 0){
            exitShell(EXIT_SUCCESS);
        }
    }

    return true;
}

// Shell Loop to handle running the program
void myshLoop() {
    printf("Shell started: enjoy!\n");
    char *input;
    char **args;
    int status;

    do {                        // Shell loop
        printf("mysh> ");       // Every new line receives shell "> "
        fflush(stdout);         // Text is output to the shell console

        input = readLine();              // Read user input from terminal
        args = tokenizeInput(input);     // Organize input line to an array of arguments
        status = execute(args);

        free(input);                     // Free previous data
        free(args);
    } while(status);
}

// Main method to initialize the program, begin running the loop, and close the program
int main(int argc, char** argv) {

    if(argc > 2) {
        printf("Usage: ./mysh \"Filepath\"\n");
    }
    else if(argc == 2) {
        // Batch Mode
        printf("Entering Batch Mode, Filepath: %s\n", (char*)argv[1]);
        isBatch = true;
        int batchFile = open(argv[1], O_RDONLY);       // Open file to read commands
        if(batchFile == -1) {                           // Check if file exists
            fprintf(stderr, "Error opening batch file\n");
            exit(EXIT_FAILURE);
        }

        char line[MAX_INPUT];
        int bytesRead;
        while((bytesRead = read(batchFile, line, sizeof(line))) > 0) {
            char **args = tokenizeInput(line);
            execute(args);
            free(args);
        }

        if(bytesRead == -1) {
            fprintf(stderr, "Error reading batch file");
            close(batchFile);
            exit(EXIT_FAILURE);
        }

        close(batchFile);
        exit(EXIT_SUCCESS);
    }
    else {
        // Shell Mode
        if(isatty(STDIN_FILENO)) {
            // preparation and initialization
            printf("Starting mysh, please wait...\n");
            isBatch = false;

            // shell loop
            myshLoop();

            // cleanup and exiting
            printf("Exiting Shell. Have a nice day!\n");
            exitShell(EXIT_SUCCESS);
        }
        else {
            fprintf(stderr, "Error: No file provided for batch mode.\n");
            exit(EXIT_FAILURE);
        }
       

        return 0;
    }
}