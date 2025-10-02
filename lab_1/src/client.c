#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h> 

#define MAX_LINE_LEN 4096

ssize_t read_line(int fd, char *buffer, size_t max_len) {
    ssize_t total_read = 0;
    char c;

    while (total_read < max_len - 1 && read(fd, &c, 1) == 1) {
        if (c == '\n') {
            break; 
        }
        buffer[total_read++] = c;
    }
    if (total_read == 0 && c != '\n'){
         return -1; 
    }
    buffer[total_read] = '\0';
    return total_read;
}

int main() {
    char filename1[256], filename2[256];

    write(STDOUT_FILENO, "Enter the file name for child1: ", 32);//получения названия 1txt
    ssize_t r1 = read(STDIN_FILENO, filename1, sizeof(filename1) - 1);
    if (r1 <= 0){
         exit(EXIT_FAILURE);
    }
    if (filename1[r1 - 1] == '\n') {
        filename1[r1 - 1] = '\0';
    }

    write(STDOUT_FILENO, "Enter the file name for child2: ", 32);//получения названия 2txt
    ssize_t r2 = read(STDIN_FILENO, filename2, sizeof(filename2) - 1);
    if (r2 <= 0) {
        exit(EXIT_FAILURE);
    }
    if (filename2[r2 - 1] == '\n'){
        filename2[r2 - 1] = '\0';
    }

    int pipe1[2], pipe2[2];//создали каналы
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("pipe creation failed");
        exit(EXIT_FAILURE);
    }

    pid_t pid1 = fork();//процесс для первого
    if (pid1 == -1) { 
        perror("fork child1"); 
        exit(EXIT_FAILURE); 
    }
    if (pid1 == 0) {
        close(pipe1[1]);
        dup2(pipe1[0], STDIN_FILENO);
        close(pipe1[0]);
        close(pipe2[0]); 
        close(pipe2[1]);

        char *args[] = {"./server", filename1, NULL}; 
        execv("./server", args);
        perror("execv child1 failed");
        exit(EXIT_FAILURE);
    }

    pid_t pid2 = fork();//процесс для второго
    if (pid2 == -1) {
         perror("fork child2"); 
         exit(EXIT_FAILURE); 
    }
    if (pid2 == 0) {
        close(pipe2[1]);
        dup2(pipe2[0], STDIN_FILENO);
        close(pipe2[0]);
        close(pipe1[0]); 
        close(pipe1[1]);

        char *args[] = {"./server", filename2, NULL}; 
        execv("./server", args);
        perror("execv child2 failed");
        exit(EXIT_FAILURE);
    }

    close(pipe1[0]);
    close(pipe2[0]);
    int wfd1 = pipe1[1];
    int wfd2 = pipe2[1];

    char line[MAX_LINE_LEN];
    ssize_t len;
    write(STDOUT_FILENO, "Enter the lines:\n", 17);
    while ((len = read_line(STDIN_FILENO, line, sizeof(line))) != -1) {
        if (len == 0){
             break;
        }
        int target_fd;
        if (len > 10) { //фильтрация
            target_fd = wfd2;
        } else {
            target_fd = wfd1;
        }
        if (write(target_fd, line, len) == -1 || write(target_fd, "\n", 1) == -1) {
            perror("write to pipe failed");
            break;
        }
    }

    close(wfd1); 
    close(wfd2); 

    waitpid(pid1, NULL, 0); 
    waitpid(pid2, NULL, 0);

    write(STDOUT_FILENO, "Parent: all children finished.\n", 31);
    return 0;
}