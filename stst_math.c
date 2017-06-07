#define _USE_MATH_DEFINES
#include <math.h>
#include <stdint.h>

#define GL_UNSIGNED_SHORT_4_4_4_4 0x8033

typedef int8_t   int8;
typedef int16_t  int16;
typedef int32_t  int32;
typedef int64_t  int64;
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef float    float32;
typedef double   float64;

#define true 1
#define false 0

#define kilobyte 1000 
#define megabyte 1000000

#define uint16_max 65535
#define uint16_min 0
#define int16_max 32768
#define int16_min -32768

int16 abs_int16(int16 n)
{
    int16 result = ((n >> 15) ^ n) + ((uint16)n >> 15);
    return result;
}

int32 sign_float32(float32 n)
{
    int32 result = (0.f < n) - (n < 0.f);
    return result;
}

#define Msign_int16(n) ((n) >> 15)
#define Msign_int32(n) ((n) >> 31)

#define Mvec_def(type) \
typedef union \
{ \
    struct \
    { \
        type x; \
        type y; \
        type z; \
        type w; \
    }; \
    struct \
    { \
        type r; \
        type g; \
        type b; \
        type a; \
    }; \
} vec4_##type; \
\
typedef union \
{ \
    struct \
    { \
        type x; \
        type y; \
        type z; \
    }; \
    struct \
    { \
        type u; \
        type v; \
        type w; \
    }; \
    struct \
    { \
        type r; \
        type g; \
        type b; \
    }; \
} vec3_##type; \
\
typedef union \
{ \
    struct \
    { \
        type x; \
        type y; \
    }; \
    struct \
    { \
        type u; \
        type v; \
    }; \
    struct \
    { \
        type r; \
        type g; \
    }; \
} vec2_##type; \
\
vec4_##type make_vec4_##type(type x, type y, type z, type w) \
{ \
    vec4_##type result = { x, y, z, w }; \
    return result; \
} \
vec4_##type add_vec4_##type(vec4_##type v1, vec4_##type v2) \
{ \
    vec4_##type result = { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w }; \
    return result; \
} \
vec4_##type sub_vec4_##type(vec4_##type v1, vec4_##type v2) \
{ \
    vec4_##type result = { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w }; \
    return result; \
} \
type dot_vec4_##type(vec4_##type v1, vec4_##type v2) \
{ \
    type result = v1.x * v2.x + v1.y * v2.y + v1.z + v2.z + v1.w * v2.w; \
    return result; \
} \
\
vec3_##type make_vec3_##type(type x, type y, type z) \
{ \
    vec3_##type result = { x, y, z }; \
    return result; \
} \
vec3_##type add_vec3_##type(vec3_##type v1, vec3_##type v2) \
{ \
    vec3_##type result = { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z }; \
    return result; \
} \
vec3_##type sub_vec3_##type(vec3_##type v1, vec3_##type v2) \
{ \
    vec3_##type result = { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z }; \
    return result; \
} \
type dot_vec3_##type(vec3_##type v1, vec3_##type v2) \
{ \
    type result = v1.x * v2.x + v1.y * v2.y + v1.z + v2.z; \
    return result; \
} \
\
vec2_##type make_vec2_##type(type x, type y) \
{ \
    vec2_##type result = { x, y }; \
    return result; \
} \
vec2_##type add_vec2_##type(vec2_##type v1, vec2_##type v2) \
{ \
    vec2_##type result = { v1.x + v2.x, v1.y + v2.y }; \
    return result; \
} \
vec2_##type sub_vec2_##type(vec2_##type v1, vec2_##type v2) \
{ \
    vec2_##type result = { v1.x - v2.x, v1.y - v2.y }; \
    return result; \
} \
type dot_vec2_##type(vec2_##type v1, vec2_##type v2) \
{ \
    type result = v1.x * v2.x + v1.y * v2.y; \
    return result; \
}

Mvec_def(uint8);
Mvec_def(float32);

typedef vec2_float32 vec2f;
#define make_vec2f make_vec2_float32
#define add_vec2f add_vec2_float32
#define sub_vec2f sub_vec2_float32
#define dot_vec2f dot_vec2_float32

typedef vec3_float32 vec3f;
#define make_vec3f make_vec3_float32
#define add_vec3f add_vec3_float32
#define sub_vec3f sub_vec3_float32
#define dot_vec3f dot_vec3_float32

typedef vec4_float32 vec4f;
#define make_vec4f make_vec4_float32
#define add_vec4f add_vec4_float32
#define sub_vec4f sub_vec4_float32
#define dot_vec4f dot_vec4_float32

typedef vec3_uint8 vec3u;
#define make_vec3u make_vec3_uint8
#define add_vec3u add_vec3_uint8
#define sub_vec3u sub_vec3_uint8
#define dot_vec3u dot_vec3_uint8

typedef vec4_uint8 vec4u;
#define make_vec4u make_vec4_uint8
#define add_vec4u add_vec4_uint8
#define sub_vec4u sub_vec4_uint8
#define dot_vec4u dot_vec4_uint8

typedef union 
{
    float32 m[3][3];
    struct 
    {
        float32 
            m11, m12, m13,
            m21, m22, m23,
            m31, m32, m33; 
    };
} mat3;

mat3 mult_mat3(mat3 a, mat3 b)
{
    mat3 result;
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            result.m[i][j] = 
                a.m[i][0]*b.m[0][j]
              + a.m[i][1]*b.m[1][j]
              + a.m[i][2]*b.m[2][j];
        }
    }
    return result;
}

mat3 mult_mat3_float32(mat3 a, float32 b)
{
    mat3 result;
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            result.m[i][j] = a.m[i][j] * b;
        }
    }
    return result;
}

mat3 transpose_mat3(mat3 m)
{
    mat3 result;
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            result.m[i][j] = m.m[j][i];
        }
    }
    return result;
}

float32 determinant_mat3(mat3 m)
{
    float32 result = 
        m.m11 * (m.m22*m.m33 - m.m23*m.m32)
      - m.m12 * (m.m21*m.m33 - m.m23*m.m31)
      + m.m13 * (m.m21*m.m32 - m.m22*m.m31);
    return result;
}

mat3 inverse_mat3(mat3 m)
{
    mat3 result;
    result.m11 = m.m22*m.m33 - m.m23*m.m32;
    result.m12 = m.m13*m.m32 - m.m12*m.m33;
    result.m13 = m.m12*m.m23 - m.m13*m.m22;
    result.m21 = m.m23*m.m31 - m.m21*m.m33;
    result.m22 = m.m11*m.m33 - m.m13*m.m31;
    result.m23 = m.m13*m.m21 - m.m11*m.m23;
    result.m31 = m.m21*m.m32 - m.m22*m.m31;
    result.m32 = m.m12*m.m31 - m.m11*m.m32;
    result.m33 = m.m11*m.m22 - m.m12*m.m21;
    result = mult_mat3_float32(result, 1 / determinant_mat3(m));
    return result;
}

mat3 make_identity_mat3()
{
    mat3 result =
    {
        1, 0, 0,
        0, 1, 0,
        0, 0, 1,
    };
    return result;
}

mat3 make_scale_mat3(vec3f s)
{
    mat3 result = 
    {
        s.x,   0,   0,
          0, s.y,   0,
          0,   0, s.z,
    };
    return result;
}

typedef union 
{
    float32 m[4][4];
    struct 
    {
        float32 
            m11, m12, m13, m14,
            m21, m22, m23, m24,
            m31, m32, m33, m34,
            m41, m42, m43, m44;
    };
} mat4;

mat4 mult_mat4(mat4 a, mat4 b)
{
    mat4 result;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            result.m[i][j] = 
                a.m[i][0]*b.m[0][j]
              + a.m[i][1]*b.m[1][j]
              + a.m[i][2]*b.m[2][j]
              + a.m[i][3]*b.m[3][j];
        }
    }
    return result;
}

mat4 make_identity_mat4()
{
    mat4 result =
    {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    return result;
}

mat4 make_translate_mat4(vec3f t)
{
    mat4 result =
    {
        1, 0, 0, t.x,
        0, 1, 0, t.y,
        0, 0, 1, t.z,
        0, 0, 0, 1
    };
    return result;
}

mat4 make_scale_mat4(vec3f s)
{
    mat4 result = 
    {
        s.x,   0,   0,  0,
          0, s.y,   0,  0,
          0,   0, s.z,  0,
          0,   0,   0,  1
    };
    return result;
}

mat4 make_rotate_mat4(vec3f axis, float32 angle)
{
    float32 cos_angle = (float32)cos(angle);
    float32 sin_angle = (float32)sin(angle);

    mat4 result;

    result.m11 = cos_angle + (1 - cos_angle)*axis.x*axis.x;
    result.m12 = (1 - cos_angle)*axis.x*axis.y - axis.z*sin_angle;
    result.m13 = (1 - cos_angle)*axis.x*axis.z + axis.y*sin_angle;
    result.m14 = 0;

    result.m21 = (1 - cos_angle)*axis.x*axis.y + axis.z*sin_angle;
    result.m22 = cos_angle + (1 - cos_angle)*axis.y*axis.y;
    result.m23 = (1 - cos_angle)*axis.y*axis.z - axis.z*sin_angle;
    result.m24 = 0;

    result.m31 = (1 - cos_angle)*axis.x*axis.z - axis.y*sin_angle;
    result.m32 = (1 - cos_angle)*axis.y*axis.z + axis.x*sin_angle;
    result.m33 = cos_angle + (1 - cos_angle)*axis.z*axis.z;
    result.m34 = 0;

    result.m41 = 0;
    result.m42 = 0;
    result.m43 = 0;
    result.m44 = 1;

    return result;
}

mat4 make_persp_proj_mat4(float32 fov, float32 ar, float32 nearz, float32 farz)
{
    mat4 result = { 0 };
    float32 f = 1 / (float32)tan(fov / 2);
    result.m11 = f / ar;
    result.m22 = f;
    result.m33 = (farz + nearz) / (nearz - farz);
    result.m34 = (2 * farz*nearz) / (nearz - farz);
    result.m43 = -1;
    return result;
}

mat3 make_mat3_from_mat4(mat4 m)
{
    mat3 result = 
    {
        m.m11, m.m12, m.m13,
        m.m21, m.m22, m.m23,
        m.m31, m.m32, m.m33
    };
    return result;
}

