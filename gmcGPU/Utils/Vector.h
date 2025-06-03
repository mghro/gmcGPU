#pragma once

#define M_PI  (3.14159265358979323846)

struct SPt2
{
  float x;
  float y;
};

struct SPt3
{
  float x;
  float y;
  float z;
};

#define VEC3SCALE(A, s, B)  \
  B.x = s * A.x;            \
  B.y = s * A.y;            \
  B.z = s * A.z;

#define PVEC3SCALE(A, s, B) \
  B->x = s * A->x;          \
  B->y = s * A->y;          \
  B->z = s * A->z;

#define VEC3ADD(A, B, C)   \
  C.x = A.x + B.x;         \
  C.y = A.y + B.y;         \
  C.z = A.z + B.z;

#define PVEC3ADD(A, B, C)  \
  C->x = A->x + B->x;      \
  C->y = A->y + B->y;      \
  C->z = A->z + B->z;

#define VEC3ADDSCALE(A, s, B, C)  \
  C.x = A.x + s * B.x;            \
  C.y = A.y + s * B.y;            \
  C.z = A.z + s * B.z;

#define PVEC3ADDSCALE(A, s, B, C) \
  C->x = A->x + s * B->x;         \
  C->y = A->y + s * B->y;         \
  C->z = A->z + s * B->z;

#define VEC3SUB(A, B, C)   \
  C.x = A.x - B.x;         \
  C.y = A.y - B.y;         \
  C.z = A.z - B.z;

#define PVEC3SUB(A, B, C)  \
  C->x = A->x - B->x;      \
  C->y = A->y - B->y;      \
  C->z = A->z - B->z;

#define VEC3DOT(A, B, c)  \
  c = A.x * B.x + A.y * B.y + A.z * B.z;

#define PVEC3DOT(A, B, c) \
  c = A->x * B->x + A->y * B->y + A->z * B->z;

#define VEC3CALCNORM(A)   \
  sqrt(A.x * A.x + A.y * A.y + A.z * A.z);

#define PVEC3CALCNORM(A)  \
  sqrt(A->x * A->x + A->y * A->y + A->z * A->z);

#define VEC3NORMALIZE(A, norm) \
  norm = sqrt(A.x * A.x + A.y * A.y + A.z * A.z); \
  A.x /= norm;  \
  A.y /= norm;  \
  A.z /= norm;

#define PVEC3NORMALIZE(A, norm) \
  norm = sqrt(A->x * A->x + A->y * A->y + A->z * A->z); \
  A->x /= norm;  \
  A->y /= norm;  \
  A->z /= norm;

