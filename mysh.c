#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define MAX_INPUT 1024

int main() {
    char input[MAX_INPUT];
    ssize_t bytes_read;

    while (1) {
        printf("> ");
        fflush(stdout);

        bytes_read = read(STDIN_FILENO, input, MAX_INPUT);
        if (bytes_read < 0) {
            perror("read");
            return 1;
        }

        // Remove newline character at the end
        input[bytes_read - 1] = '\0';

        if (strcmp(input, "exit") == 0) {
            break;
        }
    }

    return 0;
}