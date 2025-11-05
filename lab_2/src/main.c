#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <semaphore.h> 
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

sem_t thread_limit_sem;

char *generate_long_text(size_t multiplier, size_t *text_len_out) {
    const char *base_pattern = "AgctGCTGT";
    size_t base_len = strlen(base_pattern);
    size_t total_len = base_len * multiplier;
    char *text = malloc(total_len + 1);
    
    if (text == NULL) { *text_len_out = 0; return NULL; }
    for (size_t i = 0; i < multiplier; i++) {
        for (size_t j = 0; j < base_len; j++) {
            text[i * base_len + j] = base_pattern[j];
        }
    }
    text[total_len] = '\0';
    *text_len_out = total_len;
    return text;
}

typedef struct {
    const char *text;
    const char *pattern;
    size_t pattern_len;
    size_t start_idx;
    size_t end_idx;
} ThreadArgs;

size_t naive_search_range_clean(const char *text, size_t pattern_len,
                                         const char *pattern, size_t start_pos, size_t end_pos) {
    size_t local_count = 0;
    
    for (size_t i = start_pos; i < end_pos; i++) {
        if (i + pattern_len > end_pos) break;
        bool found = true;
        for (size_t j = 0; j < pattern_len; j++) {
            if (text[i + j] != pattern[j]) {
                found = false;
                break;
            }
        }
        if (found) {
            local_count++;
        }
    }
    return local_count;
}

void join_and_sum(pthread_t *threads, size_t n, size_t *final_count) {
    for (size_t i = 0; i < n; i++) {
        size_t *local_count_ptr;
        if (pthread_join(threads[i], (void **)&local_count_ptr) != 0) {
            perror("Ошибка pthread_join");
        }
        if (local_count_ptr) {
            *final_count += *local_count_ptr;
            free(local_count_ptr);
        }
    }
}

double sequential_version_clean(const char *text, const char *pattern, size_t *final_count) {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    size_t text_len = strlen(text);
    size_t pattern_len = strlen(pattern);

    size_t count = naive_search_range_clean(text, pattern_len, pattern, 0, text_len);

    *final_count = count;

    clock_gettime(CLOCK_MONOTONIC, &end);
    return (end.tv_sec - start.tv_sec) * 1000.0 +
           (end.tv_nsec - start.tv_nsec) / 1000000.0;
}

void *worker_thread_clean(void *_args) {
    ThreadArgs *args = (ThreadArgs *)_args;

    if (sem_wait(&thread_limit_sem) != 0) {
        perror("Ошибка sem_wait в потоке");
        return NULL; 
    }
    
    size_t *result_count = malloc(sizeof(size_t));
    if (result_count == NULL) {
        perror("malloc result error");
        sem_post(&thread_limit_sem); 
        return NULL;
    }
    
    *result_count = naive_search_range_clean(args->text, args->pattern_len, args->pattern,
                                             args->start_idx, args->end_idx);
    if (sem_post(&thread_limit_sem) != 0) {
        perror("Ошибка sem_post в потоке");
    }
    return (void *)result_count;
}

double parallel_version_clean(const char *text, const char *pattern,
                              size_t K_threads, size_t L_limit, size_t *final_count) {
    struct timespec start, end;
    size_t initial_value;
    if (L_limit > 0) {
        initial_value = L_limit;
    } else {
        initial_value = 1;
    }

    if (sem_init(&thread_limit_sem, 0, initial_value) != 0) {
        perror("Ошибка инициализации семафора");
        return -1.0;
    }

    size_t text_len = strlen(text);
    size_t pattern_len = strlen(pattern);

    size_t chunk_size = text_len / K_threads;

    pthread_t *threads = malloc(K_threads * sizeof(pthread_t));
    ThreadArgs *thread_args = malloc(K_threads * sizeof(ThreadArgs));
    if (!threads || !thread_args) {
        perror("Ошибка выделения памяти");
        sem_destroy(&thread_limit_sem);
        free(threads);
        free(thread_args);
        return -1.0;
    }

    *final_count = 0;

    clock_gettime(CLOCK_MONOTONIC, &start);

    for (size_t i = 0; i < K_threads; i++) {
        size_t start_idx = i * chunk_size;
        size_t end_idx = (i == K_threads - 1) ? text_len : (i + 1) * chunk_size + pattern_len - 1;
        if (end_idx > text_len) end_idx = text_len;

        thread_args[i] = (ThreadArgs){
            .text = text, .pattern = pattern, .pattern_len = pattern_len,
            .start_idx = start_idx, .end_idx = end_idx
        };

        if (pthread_create(&threads[i], NULL, worker_thread_clean, &thread_args[i]) != 0) {
            perror("Ошибка создания потока.");
            join_and_sum(threads, i, final_count);
            sem_destroy(&thread_limit_sem);
            free(thread_args);
            free(threads);
            return -1.0;
        }
    }

    join_and_sum(threads, K_threads, final_count);
    clock_gettime(CLOCK_MONOTONIC, &end);

    sem_destroy(&thread_limit_sem);
    free(thread_args);
    free(threads);
    return (end.tv_sec - start.tv_sec) * 1000.0 +
           (end.tv_nsec - start.tv_nsec) / 1000000.0;
}


int main(int argc, char **argv) {
    if (argc != 4) { 
        fprintf(stderr, "Использование: %s <образец> <число_потоков_N> <лимит_потоков_L>\n", argv[0]);
        return EXIT_FAILURE;
    }
    size_t text_len = 0;
    char *text = generate_long_text(700000, &text_len); 
    
    if (text == NULL || text_len == 0) return EXIT_FAILURE;

    char *pattern = argv[1]; 
    size_t K_threads = atoi(argv[2]); 
    size_t L_limit = atoi(argv[3]); 
    
    if (K_threads == 0 || L_limit == 0 || K_threads < L_limit || strlen(pattern) == 0) {
        fprintf(stderr, "Ошибка: N, L должны быть > 0, N >= L, и паттерн не может быть пустым.\n");
        free(text);
        return EXIT_FAILURE;
    }

    size_t expected_count = 0;
    double seq_time_clean = sequential_version_clean(text, pattern, &expected_count);
    
    size_t par_count = 0;
    double par_time_clean = parallel_version_clean(text, pattern, K_threads, L_limit, &par_count);

    if (par_time_clean < 0.0) {
        fprintf(stderr, "\n=== ОШИБКА ВЫПОЛНЕНИЯ ПАРАЛЛЕЛЬНОЙ ВЕРСИИ. РЕСУРСЫ ОСВОБОЖДЕНЫ. ===\n");
        free(text);
        return EXIT_FAILURE;
    }

    printf("Образец: '%s'\n", pattern);
    printf("T1: %.5f мс\n", seq_time_clean);
    printf("Tn: %.5f мс\n", par_time_clean);
    if (expected_count == par_count) {
        printf("Найдено %zu\n", expected_count);
    } else {
        printf("ОШИБКА: Sequential %zu != Parallel %zu.\n", expected_count, par_count);
    }
    free(text);
    return EXIT_SUCCESS;
}
