#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <stdint.h>

#define SHM_SIZE 8192
#define BUFFER_SIZE 4096

struct shared_data {
    uint32_t length1;
    char data1[BUFFER_SIZE];
    uint32_t length2;  
    char data2[BUFFER_SIZE];
};

void remove_vowels(char *s) {
    char *src = s, *dst = s;
    while (*src) {
        char c = *src;
        if (!(c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || 
              c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U' || 
              c == 'y' || c == 'Y')) {
            *dst++ = c;
        }
        src++;
    }
    *dst = '\0';
}

int main(int argc, char **argv) {
    if (argc != 5) {
        const char msg[] = "Usage: client <shm_name> <sem_access_name> <sem_avail_name> <client_id>\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    const char *shm_name = argv[1];
    const char *sem_access_name = argv[2];
    const char *sem_avail_name = argv[3];
    int client_id = atoi(argv[4]);

    int shm_fd = shm_open(shm_name, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    struct shared_data *shm_buf = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_buf == MAP_FAILED) {
        perror("mmap failed");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    sem_t *sem_access = sem_open(sem_access_name, O_RDWR);
    sem_t *sem_avail = sem_open(sem_avail_name, O_RDWR);
    
    if (sem_access == SEM_FAILED || sem_avail == SEM_FAILED) {
        perror("sem_open failed");
        munmap(shm_buf, SHM_SIZE);
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    char filename[256];
    snprintf(filename, sizeof(filename), "child%d_output.txt", client_id);
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open file failed");
        sem_close(sem_access);
        sem_close(sem_avail);
        munmap(shm_buf, SHM_SIZE);
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
    int running = 1;
    while (running) {
        if (sem_wait(sem_avail) == -1) {
            perror("sem_wait avail failed");
            break;
        }

        if (sem_wait(sem_access) == -1) {
            perror("sem_wait access failed");
            break;
        }

        uint32_t *length = (client_id == 1) ? &shm_buf->length1 : &shm_buf->length2;
        char *data = (client_id == 1) ? shm_buf->data1 : shm_buf->data2;

        if (*length == UINT32_MAX) {
            running = 0;
        } else if (*length > 0) {
            char buffer[BUFFER_SIZE];
            memcpy(buffer, data, *length);
            buffer[*length] = '\0';
            
            remove_vowels(buffer);

            if (write(fd, buffer, strlen(buffer)) == -1 || write(fd, "\n", 1) == -1) {
                perror("write to file failed");
            }
            
            *length = 0;
        }

        if (sem_post(sem_access) == -1) {
            perror("sem_post access failed");
            break;
        }
    }

    close(fd);
    sem_close(sem_access);
    sem_close(sem_avail);
    munmap(shm_buf, SHM_SIZE);
    close(shm_fd);

    return 0;
}