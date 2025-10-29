
#ifndef PERLIN_H
#define PERLIN_H

#include <math.h>
#include <stdlib.h>
#ifndef PERLINDEF
#define PERLINDEF // Functions defined as 'extern' by default (implicit
                  // specifiers)
#endif

#define PERM_SIZE 256

static int p[PERM_SIZE * 2];

// Fade function (smoothing)
static inline float fade(float t) {
  return t * t * t * (t * (t * 6 - 15) + 10);
}

// Linear interpolation
static inline float lerp(float a, float b, float t) { return a + t * (b - a); }

// Gradient function
static inline float grad(int hash, float x, float y) {
  int h = hash & 7; // 8 gradient directions
  float u = h < 4 ? x : y;
  float v = h < 4 ? y : x;
  return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}

// Initialize permutation table with a given seed
PERLINDEF void perlin_init(int seed) {
  int i;
  srand(seed);
  for (i = 0; i < PERM_SIZE; i++)
    p[i] = i;
  for (i = 0; i < PERM_SIZE; i++) {
    int j = rand() % PERM_SIZE;
    int tmp = p[i];
    p[i] = p[j];
    p[j] = tmp;
  }
  for (i = 0; i < PERM_SIZE; i++)
    p[PERM_SIZE + i] = p[i];
}

// 2D Perlin noise function
float perlin2D(float x, float y) {
  int X = (int)floor(x) & 255;
  int Y = (int)floor(y) & 255;
  x -= floor(x);
  y -= floor(y);

  float u = fade(x);
  float v = fade(y);

  int aa = p[p[X] + Y];
  int ab = p[p[X] + Y + 1];
  int ba = p[p[X + 1] + Y];
  int bb = p[p[X + 1] + Y + 1];

  float gradAA = grad(aa, x, y);
  float gradBA = grad(ba, x - 1, y);
  float gradAB = grad(ab, x, y - 1);
  float gradBB = grad(bb, x - 1, y - 1);

  float lerpX1 = lerp(gradAA, gradBA, u);
  float lerpX2 = lerp(gradAB, gradBB, u);
  return lerp(lerpX1, lerpX2, v);
}

// Optional: fractal noise (octaves)
PERLINDEF float perlin2D_octaves(float x, float y, int octaves,
                                 float persistence) {
  float total = 0;
  float frequency = 1;
  float amplitude = 1;
  float maxValue = 0;

  for (int i = 0; i < octaves; i++) {
    total += perlin2D(x * frequency, y * frequency) * amplitude;
    maxValue += amplitude;
    amplitude *= persistence;
    frequency *= 2;
  }

  return total / maxValue;
}

#endif // PERLIN_H
