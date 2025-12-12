#define _DEFAULT_SOURCE
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "mathlib.h"

extern float sin_integral(float a, float b, float e);
extern float e(int x);

#define BUFFER_SIZE 4096

static void command_1(char **saveptr)
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
            float result = sin_integral(a, b, e_step);
            len = snprintf(buffer, sizeof(buffer),
                           "интеграл sin(x) результат= %.6f\n",
                           result);
        }
    }
    else
    {
        len = snprintf(buffer, sizeof(buffer), "недостаточно аргументов\n");
    }
    write(STDOUT_FILENO, buffer, len);
}

static void command_2(char **saveptr)
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
            float result = e(x);
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
    const char *prompt =
        "Статическая линковка с libmath1.so\n"
        "1 a b e / 2 x / q\n> ";

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
        case 1:
        {
            command_1(&saveptr);
            break;
        }
        case 2:
        {
            command_2(&saveptr);
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

    write(STDOUT_FILENO, "Exit\n", 6);
    return 0;
}