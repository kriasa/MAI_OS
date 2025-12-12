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

    while (x < b)
    {
        float step = (x + e_step < b) ? e_step : (b - x);
        float mid_point = x + step / 2.0f;
        integral += step * sinf(mid_point);
        x += step;
    }

    return integral;
}

EXPORT float e(int x)
{
    if (x <= 0)
        return 0.0f;
    return powf(1.0f + 1.0f / (float)x, (float)x);
}