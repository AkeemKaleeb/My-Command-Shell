#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <glob.h>
#include <sys/wait.h>

#define MAX_INPUT 1024                  // Current Max input size, can be reallocated
#define TOKEN_DELIMITER " \t\r\n\a"     // Characters that divide command line arguments

// Built in function to exit the program with a given exit status
int exitShell(int exitStatus) {
    exit(exitStatus);
}

// Function to read the user input to be used by the program
char* readLine() {
    char* input = malloc(MAX_INPUT * sizeof(char*));            // Allocate space for input
    if(!input) {                                                // If space wasn't allocated, report an error
        fprintf(stderr, "Error allocating read line space\n");
        exit(EXIT_FAILURE);
    }

    int bytes_read = read(STDIN_FILENO, input, MAX_INPUT);      // read() buffer
    if (bytes_read < 0) {                                       // If there is a problem reading the input buffer, print error
        fprintf(stderr, "Error reading the command line\n");
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
        fprintf(stderr, "Error allocating space for tokens\n");
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
                fprintf(stderr, "Error reallocating token space\n");
                exitShell(EXIT_FAILURE);
            }
        }           

        token = strtok(NULL, TOKEN_DELIMITER);      // Nullify the final position increment
    }

    tokens[position] = NULL;                        // Last position increment holds no token
    return tokens;
}

// Function to handle wildcard (*) using glob
void wildCard(const char *pattern) {
    // Use glob to find matching files
    glob_t globResult;
    if (glob(pattern, 0, NULL, &globResult) == 0) {
        for (size_t j = 0; j < globResult.gl_pathc; j++) {
            printf("%s\n", globResult.gl_pathv[j]); // Print matching filenames, will need to change this ending functionality to change the argument list and replace the original token with the found matching names
        }
        globfree(&globResult);
    } else {
        fprintf(stderr, "glob: Error in glob function\n");
    }
}

int executeCommand(char **args) {
    for(int i = 0; args[i] != NULL; i++) {
        if(strcmp(args[i], "exit") == 0){
            //printing out any arguments exit receives
            for (int j = i + 1; args[j] != NULL; j++) {
                printf("%s ", args[j]);
            }
            printf("\n");
            exitShell(EXIT_SUCCESS);
        }
        else if (strcmp(args[i], "cd") == 0){
            //no directory specified
            if (args[i+1] == NULL){
                fprintf(stderr, "cd: No directory specified\n");
            }
            //argc is greater than 2, 
            else if (args[i+2] != NULL){
                fprintf(stderr, "cd: Too many arguments\n");
            }
                int currentDir = chdir(args[i+1]);
                if (currentDir != 0){
                    fprintf(stderr, "cd: No such file or directory\n");
                }
            return true;
        }
        else if (strcmp(args[i], "pwd") == 0){
            if (args[i+1] != NULL) {
                fprintf(stderr, "pwd: Too many arguments\n");
            }
            else {
                char* cwd;
                char buffer[MAX_INPUT];

                cwd = getcwd(buffer, MAX_INPUT);
                if (cwd != NULL){
                    printf("%s\n", cwd);
                }
                else {
                    fprintf(stderr, "pwd: Could not retrieve current directory pathname\n");
                }
            }
            return true;
        }
        else if (strcmp(args[i], "which") == 0){
            //no argument given to which
            if (args[i+1] == NULL){
                fprintf(stderr, "which: no program name specified\n");
            }
            //argc is greater than 2, 
            else if (args[i+2] != NULL){
                fprintf(stderr, "which: Too many arguments\n");
            }
            else {
                char *programName = args[i+1];
                char *pathEnvVar = getenv("PATH");
                char *pathNode = strtok(pathEnvVar, ":");
                int exists = 0;
                
                while (pathNode != NULL) {
                    char fullPath[MAX_INPUT];
                    snprintf(fullPath, MAX_INPUT, "%s/%s", pathNode, programName);
                    if(access(fullPath, F_OK) == 0) {
                        printf("%s\n", fullPath);
                        exists = 1;
                        break;
                    }
                    pathNode = strtok(NULL, ":");
                }
                if (!exists){
                    fprintf(stderr, "which: %s not found in PATH \n", programName);
                }
            }
            return true;
        }
        else if (strchr(args[i], '*') != NULL) {
            char *fullToken = args[i];
            char *asteriskPosition = strchr(fullToken, '*');
            if (asteriskPosition != NULL && strchr(asteriskPosition + 1, '/') == NULL) {    //checks that asterisk is in the last segment of path or filename
                if (strchr(args[i], '/') != NULL) {                             // given a pathname not just a file
                    char *fileName = strrchr(fullToken, '/') + 1;               // get filename part which should be right after last slash ('/'), strrchr() returns pointer to last occurence
                    char directoryPath[MAX_INPUT];                              // creating only directoryPath without filename/pattern
                    size_t pathLength = strlen(fullToken) - strlen(fileName);
                    strncpy(directoryPath, fullToken, pathLength);              // creating just the directoryPath
                    directoryPath[pathLength] = '\0';                           // null terminating the path

                    char originalDir[MAX_INPUT];
                    getcwd(originalDir, MAX_INPUT);             // get current working directory
                    chdir(directoryPath);                       // change working directory to directory specified in directoryPath 
                    wildCard(fileName);                         // glob current working directory for pattern)
                    chdir(originalDir);                         // change back to original directory
                    
                }
                else { // just a file name with an asterisk
                    wildCard(fullToken);
                }

            }
            else {
                fprintf(stderr, "Error: wildcard character must be last segment of pathname\n");
                exit(EXIT_FAILURE);
            }
            return true;
        } 
        // Not a built in command, check for other commands
        char *pathEnvVar = getenv("PATH");                  // Get environment path
        char *pathNode = strtok(pathEnvVar, ":");           // Tokenize the path into separate nodes
        bool exists = false;

        while(pathNode != NULL) {                           // See if program exists
            char fullPath[MAX_INPUT];                       // Construct path 
            snprintf(fullPath, MAX_INPUT, "%s/%s", pathNode, args[0]);      
            if(access(fullPath, F_OK) == 0) {               // If path exists and is executable
                exists = 1;
                execv(fullPath, args);                      // Execute the program with the provided args
                fprintf(stderr, "Error executing command\n");
                exitShell(EXIT_FAILURE);
            }
            pathNode = strtok(NULL, ":");
        }

        if(!exists) {
            fprintf(stderr, "Command not found: %s\n", args[0]);
        }
    }

    return true;
}

// Function to execute pipelined commands
int executePipeline(char **args1, char **args2) {
    int pipefd[2];
    pid_t pid1, pid2;

    if(pipe(pipefd) < 0) {      // Create pipe
        fprintf(stderr, "Error opening pipe");
        exitShell(EXIT_FAILURE);
    }

    pid1 = fork();              // Create first fork process
    if(pid1 < 0) {              
        fprintf(stderr, "Error opening fork");
        exitShell(EXIT_FAILURE);
    }

    if(pid1 == 0) {                         // Child Process 1
        close(pipefd[0]);                   // Close unused read end
        dup2(pipefd[1], STDOUT_FILENO);     // Redirect stdout to pipe
        close(pipefd[1]);                   // Close write end of pipe

        executeCommand(args1);              // Execute commands before pipe character
    }

    pid2 = fork();              // Fork second process
    if(pid2 < 0) {              
        fprintf(stderr, "Error opening fork");
        exitShell(EXIT_FAILURE);
    }

    if(pid2 == 0) {                         // Child Process 2
        close(pipefd[1]);                   // Close unused write end
        dup2(pipefd[0], STDIN_FILENO);      // Redirect stdin to pipe
        close(pipefd[0]);                   // Close read end of pipe

        executeCommand(args2);              // Execute commands after pipe character
    }

    // Parent Process
    close(pipefd[0]);       // Close read end
    close(pipefd[1]);       // Close write end

    // Wait for child processes to finish
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return true;
}

// modified execute function with built in commands (cd, exit, which, pwd)
// Returns true if the program should continue running
int execute(char **args) {
    if(args[0] == NULL) {           // Empty command, continue
        return true;
    }

    // Pipelining Behvaior
    int pipeIndex = -1;
    for(int i = 0; args[i] != NULL; i++) {      // Check program for pipelines
        if(strcmp(args[i], "|") == 0) {
            pipeIndex = i;
            break;
        } 
    }

    if(pipeIndex != -1) {                       // Pipeline exists
        char **args1 = malloc((pipeIndex + 1) * sizeof(char*));
        char **args2 = malloc((MAX_INPUT - pipeIndex) * sizeof(char*));

        if(!args1 || !args2) {                  // Error Allocating space
            fprintf(stderr, "Error allocating memory for pipelined arguments\n");
            exitShell(EXIT_FAILURE);
        }

        for(int i = 0; i < pipeIndex; i++) {                        // Assign the first set of arguments
            args1[i] = args[i];
        }
        args1[pipeIndex] = NULL;                                    // Terminate Array

        for(int i = 0; args[pipeIndex + i + 1] != NULL; i++) {      // Assign the second set of arguments
            args2[i] = args[pipeIndex + i + 1];
        }
        args2[MAX_INPUT - pipeIndex - 1] = NULL;                    // Terminate Array

        executePipeline(args1, args2);
        free(args1);
        free(args2);
    }
    else {      // No Pipeline
        executeCommand(args);
    }
    return true;
}

// Shell Loop to handle running the program
void myshLoop(int argc, char** argv) {
    char *input;
    char **args;
    int status;

    if(argc == 2) {
        // Batch Mode
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
            fprintf(stderr, "Error reading batch file\n");
            close(batchFile);
            exit(EXIT_FAILURE);
        }

        close(batchFile);
    }
    else {                                  
        printf("Shell started: enjoy!\n");
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
}

// Main method to initialize the program, begin running the loop, and close the program
int main(int argc, char** argv) {
    if(argc > 2) {
        myshLoop(argc, argv);
    }
    if(argc == 2) {
        int fileCheck = open(argv[1], O_RDONLY);
        if(!fileCheck) {            // Check if the argument is a file directory
            fprintf(stderr, "Error reading file path\n");
            exit(EXIT_FAILURE);
        }

        close(fileCheck);
        myshLoop(argc, argv);
        exit(EXIT_SUCCESS);
    }
    else {                                  // Batch Mode, straight into it 
        // preparation and initialization
        printf("Starting mysh, please wait...\n");

        // shell loop
        myshLoop(argc, argv);

        // cleanup and exiting
        printf("Exiting Shell. Have a nice day!\n");
        exitShell(EXIT_SUCCESS);
    }
}
