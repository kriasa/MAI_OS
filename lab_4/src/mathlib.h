#ifndef MATHLIB_H
#define MATHLIB_H

typedef float sin_integral_func(float a, float b, float e);
typedef float e_func(int x);

float sin_integral(float a, float b, float e);
float e(int x);

#endif