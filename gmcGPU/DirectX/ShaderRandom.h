#ifndef _SHADER_
#pragma once
#endif

#define INITRAND(initValue) \
uint gmcRand = initValue;   \

#define NEWRAND(value)                 \
gmcRand = gmcRand * 103515247 + 12343; \
value = ((unsigned int)(gmcRand / 65536) % 32768) / 32768.0;

#define INITSEEDS128(seed0, seed1, seed2, seed3) \
  unsigned int seedsInit[4];  \
  unsigned int t128Seed;      \
  unsigned int sum128Seed;    \
  seedsInit[0] = seed0;       \
  seedsInit[1] = seed1;       \
  seedsInit[2] = seed2;       \
  seedsInit[3] = seed3;

#define NEWSEED128(seed)                                          \
  sum128Seed = seedsInit[0] + seedsInit[3];                       \
  seed = ((sum128Seed << 7) | (sum128Seed >> 25)) + seedsInit[0]; \
  t128Seed = seedsInit[1] << 9;     \
  seedsInit[2] ^= seedsInit[0];     \
  seedsInit[3] ^= seedsInit[1];     \
  seedsInit[1] ^= seedsInit[2];     \
  seedsInit[0] ^= seedsInit[3];     \
  seedsInit[2] ^= t128Seed;         \
  seedsInit[3] = (seedsInit[3] << 11) | (seedsInit[3] >> 21);

#define INITRAND128(seedStart)   \
  unsigned int valInt;           \
  unsigned int t128;             \
  unsigned int sum128;           \
  float scale = 2.32830644e-10;  \
  unsigned int seedCurrent[4];   \
  seedCurrent[0] = seedStart[0]; \
  seedCurrent[1] = seedStart[1]; \
  seedCurrent[2] = seedStart[2]; \
  seedCurrent[3] = seedStart[3];

#define NEWRAND128(value)                                     \
  sum128 = seedCurrent[0] + seedCurrent[3];                   \
  valInt = ((sum128 << 7) | (sum128 >> 25)) + seedCurrent[0]; \
  value = valInt * scale;                                     \
  t128 = seedCurrent[1] << 9;           \
  seedCurrent[2] ^= seedCurrent[0];     \
  seedCurrent[3] ^= seedCurrent[1];     \
  seedCurrent[1] ^= seedCurrent[2];     \
  seedCurrent[0] ^= seedCurrent[3];     \
  seedCurrent[2] ^= t128;               \
  seedCurrent[3] = (seedCurrent[3] << 11) | (seedCurrent[3] >> 21);

#define STORESEED128(seedStore)  \
  seedStore[0] = seedCurrent[0]; \
  seedStore[1] = seedCurrent[1]; \
  seedStore[2] = seedCurrent[2]; \
  seedStore[3] = seedCurrent[3];

#define INITRAND128_1(seedStart)   \
  unsigned int valInt;            \
  unsigned int t128;              \
  unsigned int sum128;            \
  float scale = 2.32830644e-10;   \
  unsigned int seedCurrent1[4];   \
  seedCurrent1[0] = seedStart[0]; \
  seedCurrent1[1] = seedStart[1]; \
  seedCurrent1[2] = seedStart[2]; \
  seedCurrent1[3] = seedStart[3];

#define NEWRAND128_1(value)                                      \
  sum128 = seedCurrent1[0] + seedCurrent1[3];                   \
  valInt = ((sum128 << 7) | (sum128 >> 25)) + seedCurrent1[0]; \
  value = valInt * scale;                                      \
  t128 = seedCurrent1[1] << 9;            \
  seedCurrent1[2] ^= seedCurrent1[0];     \
  seedCurrent1[3] ^= seedCurrent1[1];     \
  seedCurrent1[1] ^= seedCurrent1[2];     \
  seedCurrent1[0] ^= seedCurrent1[3];     \
  seedCurrent1[2] ^= t128;                \
  seedCurrent1[3] = (seedCurrent1[3] << 11) | (seedCurrent1[3] >> 21);

#define STORESEED128_1(seedStore)   \
  seedStore[0] = seedCurrent1[0]; \
  seedStore[1] = seedCurrent1[1]; \
  seedStore[2] = seedCurrent1[2]; \
  seedStore[3] = seedCurrent1[3];




