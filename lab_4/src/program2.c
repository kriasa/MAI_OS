#define _DEFAULT_SOURCE
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>
#include "mathlib.h"

#define LIB1_PATH "./libmath1.so"
#define LIB2_PATH "./libmath2.so"
#define BUFFER_SIZE 4096

static float sin_integral_stub(float a, float b, float e)
{
    (void)a;
    (void)b;
    (void)e;
    const char msg[] = "библиотека не загружена или функция не найдена\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
    return 0.0f;
}
static float e_stub(int x)
{
    (void)x;
    const char msg[] = "библиотека не загружена или функция не найдена\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
    return 0.0f;
}

static int load_library(const char *library_path, void **library_handle,
                        sin_integral_func **sin_impl_ptr, e_func **e_impl_ptr)
{

    if (*library_handle != NULL)
    {
        dlclose(*library_handle);
        *library_handle = NULL;
    }

    void *handle = dlopen(library_path, RTLD_LAZY);
    if (handle == NULL)
    {
        const char msg[] = "ошибка загрузки библиотеки: ";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        write(STDERR_FILENO, dlerror(), strlen(dlerror()));
        write(STDERR_FILENO, "\n", 1);

        *sin_impl_ptr = sin_integral_stub;
        *e_impl_ptr = e_stub;
        return 0;
    }

    *sin_impl_ptr = (sin_integral_func *)dlsym(handle, "sin_integral");
    *e_impl_ptr = (e_func *)dlsym(handle, "e");

    if (*sin_impl_ptr == NULL || *e_impl_ptr == NULL)
    {
        const char msg[] = "не удалось найти функции в библиотеке\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);

        dlclose(handle);
        *library_handle = NULL;
        *sin_impl_ptr = sin_integral_stub;
        *e_impl_ptr = e_stub;
        return 0;
    }

    *library_handle = handle;

    char msg[256];
    int length = snprintf(msg, sizeof(msg), "библиотека '%s' загружена\n", library_path);
    write(STDOUT_FILENO, msg, length);
    return 1;
}

static void command_0(const char **current_lib_ptr, void **library,
                      sin_integral_func **sin_impl_ptr, e_func **e_impl_ptr)
{

    const char *new_lib_path;
    if (strcmp(*current_lib_ptr, LIB1_PATH) == 0)
    {
        new_lib_path = LIB2_PATH;
    }
    else
    {
        new_lib_path = LIB1_PATH;
    }

    if (load_library(new_lib_path, library, sin_impl_ptr, e_impl_ptr))
    {
        *current_lib_ptr = new_lib_path;
    }
}

static void command_1(sin_integral_func sin_integral_impl, char **saveptr)
{
    char *arg1 = strtok_r(NULL, " \t\n", saveptr);
    char *arg2 = strtok_r(NULL, " \t\n", saveptr);
    char *arg3 = strtok_r(NULL, " \t\n", saveptr);

    int len = 0;
    char buffer[256];

    if (arg1 && arg2 && arg3)
    {
        float a = atof(arg1);
        float b = atof(arg2);
        float e_step = atof(arg3);

        if (e_step <= 0)
        {
            len = snprintf(buffer, sizeof(buffer), "ошибка значения\n");
        }
        else
        {
            float result = sin_integral_impl(a, b, e_step);
            len = snprintf(buffer, sizeof(buffer),
                           "интеграл sin(x) результат = %.6f\n", result);
        }
    }
    else
    {
        len = snprintf(buffer, sizeof(buffer), "недостаточно аргументов\n");
    }
    write(STDOUT_FILENO, buffer, len);
}

static void command_2(e_func e_impl, char **saveptr)
{
    char *arg1 = strtok_r(NULL, " \t\n", saveptr);

    int len = 0;
    char buffer[256];

    if (arg1)
    {
        int x = atoi(arg1);

        if (x <= 0)
        {
            len = snprintf(buffer, sizeof(buffer), "ошибка значения\n");
        }
        else
        {
            float result = e_impl(x);
            len = snprintf(buffer, sizeof(buffer),
                           "e результат = %.10f\n", result);
        }
    }
    else
    {
        len = snprintf(buffer, sizeof(buffer), "недостаточно аргументов\n");
    }
    write(STDOUT_FILENO, buffer, len);
}

int main(void)
{
    void *library_handle = NULL;
    sin_integral_func *sin_integral_impl = NULL;
    e_func *e_impl = NULL;
    const char *current_lib_path = LIB1_PATH;

    if (!load_library(current_lib_path, &library_handle, &sin_integral_impl, &e_impl))
    {
        write(STDERR_FILENO, "не удалось загрузить библиотеку\n", 61);
        return 1;
    }

    const char *prompt =
        "Динамическая загрузка\n"
        " 0 (switch) / 1 a b e / 2 x / q\n> ";

    write(STDOUT_FILENO, prompt, strlen(prompt));

    int bytes_read = 0;
    char buffer[BUFFER_SIZE];

    while ((bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE - 1)) > 0)
    {
        buffer[bytes_read] = '\0';
        for (int i = 0; i < bytes_read; i++)
        {
            if (buffer[i] == '\n' || buffer[i] == '\r')
            {
                buffer[i] = '\0';
                break;
            }
        }

        char *saveptr;
        char *token = strtok_r(buffer, " \t\n", &saveptr);

        if (token == NULL)
        {
            write(STDOUT_FILENO, "> ", 2);
            continue;
        }

        if (strcmp(token, "q") == 0)
            break;

        int cmd = atoi(token);

        switch (cmd)
        {
        case 0:
        {
            command_0(&current_lib_path, &library_handle, &sin_integral_impl, &e_impl);
            break;
        }
        case 1:
        {
            command_1(sin_integral_impl, &saveptr);
            break;
        }
        case 2:
        {
            command_2(e_impl, &saveptr);
            break;
        }
        default:
        {
            const char msg[] = "неизвестная команда\n";
            write(STDOUT_FILENO, msg, strlen(msg));
        }
        }
        write(STDOUT_FILENO, "> ", 2);
    }

    if (library_handle != NULL)
    {
        dlclose(library_handle);
    }
    write(STDOUT_FILENO, "Exit\n", 5);
    return 0;
}