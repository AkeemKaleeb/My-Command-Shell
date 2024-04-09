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

bool previousStatus = false;

typedef struct {
    char **data;
    unsigned length;
    unsigned capacity;
} arraylist_t;

void initialize(arraylist_t *L, unsigned size) {
    L->data = malloc(size * sizeof(char*));
    L->length = 0;
    L->capacity = size;
}

void destroy(arraylist_t *L) {
    for(int i = 0; i < L->length; i++) {
        free(L->data[i]);
    }
    free(L->data);
}

void addArgList(arraylist_t *L, const char* item) {
    if(item == NULL) {
        return;
    }
    if (L->length == L->capacity) {
        L->capacity *= 2;
        char **temp = realloc(L->data, L->capacity * sizeof(char*));
        if (!temp) {
            fprintf(stderr, "Error: Reallocation failed\n");
            exit(EXIT_FAILURE);
        }
        L->data = temp;
    }

    L->data[L->length] = malloc(strlen(item) + 1);
    if (!L->data[L->length]) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    strcpy(L->data[L->length], item);

    L->length++;
}

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
    
    if(position == 0) {
        free(tokens);
        tokens = NULL;
    }
    return tokens;
}

// Function to handle wildcard (*) using glob
void wildCard(const char *pattern, char *originalToken, arraylist_t cmdArgList) {
    // Use glob to find matching files
    glob_t globResult;
    if (glob(pattern, 0, NULL, &globResult) == 0) {
        for (size_t j = 0; j < globResult.gl_pathc; j++) {
            addArgList(&cmdArgList, globResult.gl_pathv[j]); // adds matching file names to argument list
        }
        globfree(&globResult);
    }
    else {
        addArgList(&cmdArgList, originalToken);
    }
}

int executeCommand(char **args, arraylist_t cmdArgList) {
    for(int i = 0; args[i] != NULL; i++) {
        if(strcmp(args[i], "exit") == 0){
            //printing out any arguments exit receives
            addArgList(&cmdArgList, args[i]); //adds "exit" to argument list
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
                previousStatus = false;
            }
            //argc is greater than 2, 
            else if (args[i+2] != NULL){
                fprintf(stderr, "cd: Too many arguments\n");
                previousStatus = false;
            }    
            else {
                addArgList(&cmdArgList, args[i]); //adds "cd" to argument list
                addArgList(&cmdArgList, args[i+1]); //adds directoryName to change to, to argument list
                int currentDir = chdir(args[i+1]);
                if (currentDir != 0){
                    fprintf(stderr, "cd: No such file or directory\n");
                    previousStatus = false;
                }
                else {
                    previousStatus = true;
                }
                return true;
            }
        }
        else if (strcmp(args[i], "pwd") == 0){
            addArgList(&cmdArgList, args[i]); // adds "pwd" to argument list
            if (args[i+1] != NULL) {
                fprintf(stderr, "pwd: Too many arguments\n");
                previousStatus = false;
            }
            else {
                char* cwd;
                char buffer[MAX_INPUT];

                cwd = getcwd(buffer, MAX_INPUT);
                if (cwd != NULL){
                    printf("%s\n", cwd);
                    previousStatus = true;
                }
                else {
                    fprintf(stderr, "pwd: Could not retrieve current directory pathname\n");
                    previousStatus = false;
                }
            }
            return true;
        }
        else if (strcmp(args[i], "which") == 0){
            addArgList(&cmdArgList, args[i]); // adds "which" to argument list
            //no argument given to which
            if (args[i+1] == NULL){
                fprintf(stderr, "which: no program name specified\n");
                previousStatus = false;
            }
            //argc is greater than 2, 
            else if (args[i+2] != NULL){
                fprintf(stderr, "which: Too many arguments\n");
                previousStatus = false;
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
                    previousStatus = false;
                }
                else {
                    previousStatus = true;
                }
            }
            return true;
        }
        else if (strchr(args[i], '*') != NULL) {
            char *fullToken = args[i];
            char *asteriskPosition = strchr(fullToken, '*');
            if (asteriskPosition != NULL && strchr(asteriskPosition + 1, '/') == NULL) {    //checks that asterisk is in the last segment of path or filename
                if (strchr(args[i], '/') != NULL) {                             // given a pathname not just a file name
                    char *fileName = strrchr(fullToken, '/') + 1;               // get filename part which should be right after last slash ('/'), strrchr() returns pointer to last occurence
                    char directoryPath[MAX_INPUT];                              // creating only directoryPath without filename/pattern
                    size_t pathLength = strlen(fullToken) - strlen(fileName);
                    strncpy(directoryPath, fullToken, pathLength);              // creating just the directoryPath
                    directoryPath[pathLength] = '\0';                           // null terminating the path

                    char originalDir[MAX_INPUT];
                    getcwd(originalDir, MAX_INPUT);             // get current working directory
                    chdir(directoryPath);                       // change working directory to directory specified in directoryPath 
                    wildCard(fileName, args[i], cmdArgList);    // glob current working directory for pattern
                    chdir(originalDir);                         // change back to original directory
                    
                }
                else { // just a file name with an asterisk
                    wildCard(fullToken, args[i], cmdArgList); // passing cmdArglist to populate with matching files or original token
                }
                previousStatus = true;
            }
            else {
                fprintf(stderr, "Error: wildcard character must be last segment of pathname\n");
                previousStatus = false;
                exit(EXIT_FAILURE);
            }
            return true; //not sure we need this because wildcard token may not be last token
        } 
        else if ((strchr(args[i], '<') != NULL) || (strchr(args[i], '>') != NULL)) {
            // Check for redirection
            int inputRedirection = 0; // Flag for input redirection
            int outputRedirection = 0; // Flag for output redirection
            char *inputFile = NULL; // Input file path
            char *outputFile = NULL; // Output file path
            if (strchr(args[i], '<') != NULL) {
                inputRedirection = 1;
                if (args[i + 1] != NULL && args[i + 1][0] != '\0') { // there is a space between redirection flag and file path
                    inputFile = args[i + 1]; // Get input file path
                } else {
                    // If no space after '<', get input file from the current argument
                    if (args[i][1] != '\0') {
                        inputFile = args[i] + 1; // Start from the character immediately after '<'
                    }
                }
                char *flagPosition1 = strchr(args[i], '<');
                *flagPosition1 = '\0'; //replaces redirection symbol with NULL

            } else if (strchr(args[i], '>') != NULL) {
                // Output redirection
                outputRedirection = 1;
                if (args[i + 1] != NULL && args[i + 1][0] != '\0') { // there is a space between redirection flag and file path
                    outputFile = args[i + 1]; // Get ouput file path
                } else {
                    // If no space after '>', get output file from the current argument
                    if (args[i][1] != '\0') {
                        outputFile = args[i] + 1; // Start from the character immediately after '<'
                    }
                }
                char *flagPosition2 = strchr(args[i], '>');
                *flagPosition2 = '\0'; //replaces redirection symbol with NULL
            }

            // open file for redirection
            int fd;
            // Input redirection
            if (inputRedirection) {
                fd = open(inputFile, O_RDONLY);
                if (fd == -1) {
                    perror("open");
                    return 1;
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }

            // Output redirection
            if (outputRedirection) {
                fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0640);
                if (fd == -1) {
                    perror("open");
                    return 1;
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            // Execute the command
            // may have to append to cmdArgList all the arguments that come after the redirection file name,
            execv(args[0], cmdArgList.data); //execute's the first command in the command-line, and passes in the argument list
            perror("execv"); // Print error message if execv fails to succeed
            return 1; // Return 1 exit status, not sure if we need this because redirection tokens may not be last tokens in command line
            // should continue reading tokens after this I believe

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
                pid_t pid = fork();                     // Fork a new process
                if (pid < 0) {
                    fprintf(stderr, "Error forking process\n");
                    previousStatus = false;
                    exit(EXIT_FAILURE);
                } else if (pid == 0) {
                    // Child process
                    execv(fullPath, args);              // Execute the command
                    fprintf(stderr, "execv");
                    previousStatus = false;
                    exit(EXIT_FAILURE);
                } else {
                    // Parent process
                    int status;
                    waitpid(pid, &status, 0);           // Wait for the child process to complete
                    previousStatus = true;
                    return true;                        // Return true to continue running the shell
                }
            }
            pathNode = strtok(NULL, ":");
        }
        addArgList(&cmdArgList, args[i]); // adds command/file to argument list
        if(!exists) {
            fprintf(stderr, "Command not found: %s\n", args[0]);
            previousStatus = false;
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

        arraylist_t argList1;
        int length1 = 0;
        while (args1[length1] != NULL) {
            length1++;
        }
        initialize(&argList1, length1);                // creates argument list for first set of commands and passes it to executeCommand to be populated
        executeCommand(args1, argList1);              // Execute commands before pipe character
        destroy(&argList1);
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

        arraylist_t argList2;
        int length2 = 0;
        while (args1[length2] != NULL) {
            length2++;
        }
        initialize(&argList2, length2);                // creates argument list for second set of commands
        executeCommand(args2, argList2);              // Execute commands after pipe character and passes it to executeCommand to be populated
        destroy(&argList2);
    }

    // Parent Process
    close(pipefd[0]);       // Close read end
    close(pipefd[1]);       // Close write end

    // Wait for child processes to finish
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return true;
}

void executeConditional(char **args1, char **args2, char* condition) {
    if(strcmp(condition, "then") == 0 && previousStatus == true) {
        arraylist_t argList1;
        int length1 = 0;
        while (args1[length1] != NULL) {
            length1++;
        }
        initialize(&argList1, length1);                // creates argument list for first set of commands and passes it to executeCommand to be populated
        executeCommand(args1, argList1);              // Execute commands before pipe character
        destroy(&argList1);
    }
    else if(strcmp(condition, "else") == 0 && previousStatus == false) {
        arraylist_t argList2;
        int length2 = 0;
        while (args2[length2] != NULL) {
            length2++;
        }
        initialize(&argList2, length2);                // creates argument list for first set of commands and passes it to executeCommand to be populated
        executeCommand(args2, argList2);              // Execute commands before pipe character
        destroy(&argList2);
    }
}

// modified execute function with built in commands (cd, exit, which, pwd)
// Returns true if the program should continue running
int execute(char **args, arraylist_t *cmdArgList) {
    if(args[0] == NULL) {           // Empty command, continue
        return true;
    }

    // Pipelining Behvaior
    int pipeIndex = -1;
    int conditionalIndex = -1;
    for(int i = 0; args[i] != NULL; i++) {      // Check program for pipelines
        if(strcmp(args[i], "|") == 0) {
            pipeIndex = i;
            break;
        } 
        else if(strcmp(args[i], "then") == 0 || strcmp(args[i], "else") == 0){
            conditionalIndex = i;
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
    else if(conditionalIndex != -1){
        char **args1 = malloc((conditionalIndex + 1) * sizeof(char*));
        char **args2 = malloc((MAX_INPUT - conditionalIndex) * sizeof(char*));

        if(!args1 || !args2) {                  // Error Allocating space
            fprintf(stderr, "Error allocating memory for conditional arguments\n");
            exitShell(EXIT_FAILURE);
        }

        for(int i = 0; i < conditionalIndex; i++) {                        // Assign the first set of arguments
            args1[i] = args[i];
        }
        args1[conditionalIndex] = NULL;                                    // Terminate Array

        for(int i = 0; args[conditionalIndex + i + 1] != NULL; i++) {      // Assign the second set of arguments
            args2[i] = args[conditionalIndex + i + 1];
        }
        args2[MAX_INPUT - conditionalIndex - 1] = NULL;                    // Terminate Array

        executeConditional(args1, args2, args[conditionalIndex]);
        free(args1);
        free(args2);
    }
    else {      // No Pipeline
        int tokenLength = 0;
        while(args[tokenLength] != NULL) {
            tokenLength++;
        }
        executeCommand(args, *cmdArgList);
    }
    return true;
}

// Shell Loop to handle running the program
void myshLoop(int argc, char** argv) {
    char *input;
    char **args;
    int status;

    arraylist_t cmdArgList;

    if(argc >= 2) {
        // Batch Mode
        initialize(&cmdArgList, MAX_INPUT);
        int batchFile = open(argv[1], O_RDONLY);        // Open file to read commands
        if(batchFile == -1) {                           // Check if file exists
            fprintf(stderr, "Error opening batch file\n");
            exit(EXIT_FAILURE);
        }

        char line[MAX_INPUT];
        int bytesRead;
        while((bytesRead = read(batchFile, line, sizeof(line))) > 0) {
            char **args = tokenizeInput(line);
            execute(args, &cmdArgList);
            free(args);
        }

        if(bytesRead == -1) {
            fprintf(stderr, "Error reading batch file\n");
            close(batchFile);
            exit(EXIT_FAILURE);
        }

        close(batchFile);
        destroy(&cmdArgList);
    }
    else {                                  
        printf("Shell started: enjoy!\n");
        do {                        // Shell loop
            printf("mysh> ");       // Every new line receives shell "> "
            fflush(stdout);         // Text is output to the shell console

            input = readLine();              // Read user input from terminal
            args = tokenizeInput(input);     // Organize input line to an array of arguments
            status = execute(args, &cmdArgList);

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
