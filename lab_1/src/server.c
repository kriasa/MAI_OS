#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h> 

#define MAX_LEN 4096


void remove_vowels(char *s)
{
    char *src = s, *dst = s;
    while (*src) {
        char c = *src;
        if (!(c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U' || c == 'y' || c == 'Y')) {
            *dst++ = c;
        }
        src++;
    }
    *dst = '\0';
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        const char msg[] = "usage: server filename\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }


    int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open file failed");
        exit(EXIT_FAILURE);
    }

    char buf[MAX_LEN];
    ssize_t n;

    while ((n = read(STDIN_FILENO, buf, sizeof(buf) - 1)) > 0) {

        if (buf[n - 1] == '\n') {
            buf[n - 1] = '\0';
        } else {
            buf[n] = '\0';
        }
        
        remove_vowels(buf);
        size_t len = strlen(buf);

        if (write(fd, buf, len) == -1 || write(fd, "\n", 1) == -1) {
            perror("write file failed");
        }

        if (write(STDOUT_FILENO, buf, len) == -1 || write(STDOUT_FILENO, "\n", 1) == -1) {
            perror("write stdout failed");
        }
    }

    if (n == -1) {
        perror("read stdin failed");
    }

    close(fd);
    return 0;
}