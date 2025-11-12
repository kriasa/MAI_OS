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
#include <time.h>

#define SHM_SIZE 8192
#define BUFFER_SIZE 4096
#define MAX_LINE_LEN 4096

struct shared_data {
    uint32_t length1;
    char data1[BUFFER_SIZE];
    uint32_t length2;  
    char data2[BUFFER_SIZE];
};

ssize_t read_line(int fd, char *buffer, size_t max_len) {
    ssize_t total_read = 0;
    char c;

    while (total_read < max_len - 1 && read(fd, &c, 1) == 1) {
        if (c == '\n') {
            break;
        }
        buffer[total_read++] = c;
    }
    if (total_read == 0) {
        return -1;
    }
    buffer[total_read] = '\0';
    return total_read;
}

int main() {
    char shm_name[64];
    char sem_access_name[64];
    char sem_avail1_name[64];
    char sem_avail2_name[64];
    
    int pid = getpid();
    snprintf(shm_name, sizeof(shm_name), "/lab_shm_%d", pid);
    snprintf(sem_access_name, sizeof(sem_access_name), "/lab_access_%d", pid);
    snprintf(sem_avail1_name, sizeof(sem_avail1_name), "/lab_avail1_%d", pid);
    snprintf(sem_avail2_name, sizeof(sem_avail2_name), "/lab_avail2_%d", pid);

    int shm_fd = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate failed");
        shm_unlink(shm_name);
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    struct shared_data *shm_buf = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_buf == MAP_FAILED) {
        perror("mmap failed");
        shm_unlink(shm_name);
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
    memset(shm_buf, 0, sizeof(struct shared_data));

    sem_t *sem_access = sem_open(sem_access_name, O_CREAT | O_EXCL, 0666, 1);
    sem_t *sem_avail1 = sem_open(sem_avail1_name, O_CREAT | O_EXCL, 0666, 0);
    sem_t *sem_avail2 = sem_open(sem_avail2_name, O_CREAT | O_EXCL, 0666, 0);
    
    if (sem_access == SEM_FAILED || sem_avail1 == SEM_FAILED || sem_avail2 == SEM_FAILED) {
        perror("sem_open failed");
        goto cleanup;
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork child1 failed");
        goto cleanup;
    }

    if (pid1 == 0) {
        char client_id_str[2] = "1";
        char *args[] = {"client", shm_name, sem_access_name, sem_avail1_name, client_id_str, NULL};
        execv("client", args);
        perror("execv child1 failed");
        exit(EXIT_FAILURE);
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork child2 failed");
        goto cleanup;
    }

    if (pid2 == 0) {
        char client_id_str[2] = "2";
        char *args[] = {"client", shm_name, sem_access_name, sem_avail2_name, client_id_str, NULL};
        execv("client", args);
        perror("execv child2 failed");
        exit(EXIT_FAILURE);
    }

    sleep(1);

    printf("Enter lines (empty line to finish):\n");
    char line[MAX_LINE_LEN];
    ssize_t len;

    while ((len = read_line(STDIN_FILENO, line, sizeof(line))) != -1) {
        if (len == 0) {
            break;
        }

        if (sem_wait(sem_access) == -1) {
            perror("sem_wait access failed");
            break;
        }

        uint32_t *target_length;
        char *target_data;
        sem_t *target_sem_avail;

        if (len > 10) {
            target_length = &shm_buf->length2;
            target_data = shm_buf->data2;
            target_sem_avail = sem_avail2;
        } else {
            target_length = &shm_buf->length1;
            target_data = shm_buf->data1;
            target_sem_avail = sem_avail1;
        }

        memcpy(target_data, line, len);
        *target_length = len;

        if (sem_post(sem_access) == -1) {
            perror("sem_post access failed");
            break;
        }

        if (sem_post(target_sem_avail) == -1) {
            perror("sem_post avail failed");
            break;
        }
    }
    if (sem_wait(sem_access) == -1) {
        perror("sem_wait access failed");
    } else {
        shm_buf->length1 = UINT32_MAX;
        shm_buf->length2 = UINT32_MAX;
        sem_post(sem_access);
        sem_post(sem_avail1);
        sem_post(sem_avail2);
    }

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    printf("Processing complete. Results saved to child1_output.txt and child2_output.txt\n");

cleanup:
    if (sem_access != SEM_FAILED) {
        sem_close(sem_access);
        sem_unlink(sem_access_name);
    }
    if (sem_avail1 != SEM_FAILED) {
        sem_close(sem_avail1);
        sem_unlink(sem_avail1_name);
    }
    if (sem_avail2 != SEM_FAILED) {
        sem_close(sem_avail2);
        sem_unlink(sem_avail2_name);
    }
    munmap(shm_buf, SHM_SIZE);
    shm_unlink(shm_name);
    close(shm_fd);

    return 0;
}