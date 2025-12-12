#include "mathlib.h"
#include <math.h>

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

EXPORT float sin_integral(float a, float b, float e_step)
{
    if (e_step <= 0.0f || b <= a)
        return 0.0f;

    float integral = 0.0f;
    float x = a;
    float prev_sin = sinf(x);

    while (x < b)
    {
        float step = (x + e_step < b) ? e_step : (b - x);
        float next_x = x + step;
        float next_sin = sinf(next_x);
        integral += (prev_sin + next_sin) * step / 2.0f;

        x = next_x;
        prev_sin = next_sin;
    }

    return integral;
}

EXPORT float e(int x)
{
    if (x <= 0)
        return 0.0f;

    float result = 1.0f;
    float factorial = 1.0f;

    for (int n = 1; n <= x; n++)
    {
        factorial *= n;
        result += 1.0f / factorial;
    }

    return result;
}