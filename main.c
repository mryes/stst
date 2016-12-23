#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include <math.h>
#include <windows.h>
#include <GL/gl.h>

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
typedef int16_t  fixed16;
typedef fixed16  fixed16_6;
typedef int32_t  fixed32;

#define true 1
#define false 0

#define kilobyte 1000 
#define megabyte 1000000

#define uint16_max 65535
#define uint16_min 0
#define int16_max 32767
#define int16_min -32768

int16 abs_int16(int16 n)
{
    int16 result = ((n >> 15) ^ n) + ((uint16)n >> 15);
    return result;
}

#ifdef _MSC_VER
#define printf printf_msvc
int printf_msvc(const char *format, ...)
{
    char str[1024];
    va_list args;
    va_start(args, format);
    int result = vsnprintf(str, sizeof(str), format, args);
    va_end(args);
    OutputDebugStringA(str);
    return result;
}
#endif

#define Msign_int16(n) ((n) >> 15)
#define Msign_int32(n) ((n) >> 31)

fixed32 mul_fixed16(uint8 fraction_bits, fixed16 a, fixed16 b) \
{
    int32 result = (int32)a * b;
    result = result >> fraction_bits;
    return (fixed32)result; \
}
#define Mmul_fixed16_6(a, b) mul_fixed16(6, a, b)

void slow_print_fixed16(uint8 fraction_bits, fixed16 n)
{
    printf("%g\n", n / (float64)(1 << fraction_bits));
}
#define Mslow_print_fixed16_6(a, b) slow_print_fixed16(6, a, b)

#define Mfixed16(fraction_bits, integer, fraction) \
    ((fixed16)(((integer) << (fraction_bits)) | (fraction)))
#define Mfixed16_neg(fraction_bits, integer, fraction) \
    (~Mfixed16((fraction_bits), (integer), (fraction)) + 1)

fixed16 shift_down_to_fixed16(fixed32 f, int current_fraction_bits, int new_fraction_bits)
{
    fixed16 result = f >> (current_fraction_bits - new_fraction_bits);
    return result;
}

#define Mvec_def(type) \
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
} 

// The "special" vector functions are the ones that differ between types
// (in particular between fixed point types and all the other types)

#define Mvec_special_function_def(type) \
type dot_vec3_##type(vec3_##type v1, vec3_##type v2) \
{ \
    type result = v1.x * v2.x + v1.y * v2.y + v1.z + v2.z; \
    return result; \
} \
type dot_vec2_##type(vec2_##type v1, vec2_##type v2) \
{ \
    type result = v1.x * v2.x + v1.y * v2.y; \
    return result; \
}

#define Mvec_special_function_def_fixed16() \
fixed32 dot_vec3_fixed16(uint8 fraction_bits, vec3_fixed16 v1, vec3_fixed16 v2) \
{ \
    fixed32 result = \
        mul_fixed16(fraction_bits, v1.x, v2.x) + \
        mul_fixed16(fraction_bits, v1.y, v2.y) + \
        mul_fixed16(fraction_bits, v1.x, v2.x);  \
    return result; \
} \
fixed32 dot_vec2_fixed16(uint8 fraction_bits, vec2_fixed16 v1, vec2_fixed16 v2) \
{ \
    fixed32 result = \
        mul_fixed16(fraction_bits, v1.x, v2.x) + \
        mul_fixed16(fraction_bits, v1.y, v2.y);  \
    return result; \
}

#define dot_vec2_screen(v1, v2) dot_vec2_fixed16(6, v1, v2)

Mvec_def(uint8);
Mvec_special_function_def(uint8);
Mvec_def(fixed16);
Mvec_special_function_def_fixed16();
typedef vec2_fixed16 vec2_fixed16_6;
typedef vec3_fixed16 vec3_fixed16_6;

#define mul_screen mul_fixed16_6
#define screen_fraction_bits 6
#define Mscreen_coord(integer, fraction) Mfixed16(6, integer, fraction)

#define Mpixel_word(r, g, b) ((r & 15) << 12) | ((g & 15) << 8) | ((b & 15) << 4);

#define max_face_vertices 8

LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
        case WM_SETCURSOR:
        {
            static int8 cursor_hidden = false;
            WORD ht = LOWORD(lParam);
            if (ht == HTCLIENT && !cursor_hidden)
            {
                cursor_hidden = true;
                ShowCursor(false);
            }
            else if (ht != HTCLIENT && cursor_hidden)
            {
                cursor_hidden = false;
                ShowCursor(true);
            }
            break;
        }
    }
    return DefWindowProc(window, message, wParam, lParam);
}

typedef struct 
{
    uint8 *buffer;
    uint64 size;
    uint64 used;
} Memory;

uint8 *push_alloc(Memory *memory, uint64 size)
{
    uint8 *result = memory->buffer;
    memory->buffer += size;
    memory->used += size;
    assert(memory->used < memory->size);
    return result;
}

typedef struct
{
    uint8 *data;
    uint32 size;
    int32 width;
    int32 height;
    uint8 pixel_size;
    uint32 pixel_num;
} Framebuffer;

void renderer_draw_face(Framebuffer *framebuffer, vec2_fixed16_6 *vertices, int num_vertices, vec3_uint8 color)
{
    vec2_fixed16 min_corner = { int16_max, int16_max };
    vec2_fixed16 max_corner = { int16_min, int16_min };
    for (int i=0; i<num_vertices; i++)
    {
        vec2_fixed16 p = vertices[i];
        if (p.x < min_corner.x)
        {
            min_corner.x = p.x;
        }
        if (p.y < min_corner.y)
        {
            min_corner.y = p.y;
        }
        if (p.x > max_corner.x)
        {
            max_corner.x = p.x;
        }
        if (p.y > max_corner.y)
        {
            max_corner.y = p.y;
        }
    }

    min_corner.x = min_corner.x >> screen_fraction_bits;
    min_corner.y = min_corner.y >> screen_fraction_bits;
    max_corner.x = (max_corner.x + (1 << (screen_fraction_bits-1))) >> screen_fraction_bits;
    max_corner.y = (max_corner.y + (1 << (screen_fraction_bits-1))) >> screen_fraction_bits;

    if (min_corner.x < 0)
    {
        min_corner.x = 0;
    }
    if (min_corner.x >= framebuffer->width)
    {
        min_corner.x = framebuffer->width - 1;
    }
    if (min_corner.y < 0)
    {
        min_corner.y = 0;
    }
    if (min_corner.y >= framebuffer->height)
    {
        min_corner.y = framebuffer->height - 1;
    }

    vec2_fixed16 perps[max_face_vertices];
    for (int i=0; i<num_vertices; i++)
    {
        int next = i + 1;
        if (next > 2) next = 0;
        vec2_fixed16 line = sub_vec2_fixed16(vertices[next], vertices[i]);
        perps[i].x = -line.y;
        perps[i].y =  line.x;
    }

    for (int16 y = min_corner.y; y <= max_corner.y; y++)
    {
        for (int16 x = min_corner.x; x <= max_corner.x; x++)
        {
            vec2_fixed16 pixel_vec_abs = make_vec2_fixed16(
                Mscreen_coord(x, 1 << (screen_fraction_bits-1)), 
                Mscreen_coord(y, 1 << (screen_fraction_bits-1)));
            int16 last_dp_sign;
            int fail = false;
            for (int i=0; i<num_vertices; i++)
            {
                vec2_fixed16 pixel_vec_rel = sub_vec2_fixed16(pixel_vec_abs, vertices[i]);
                fixed32 dp = dot_vec2_screen(pixel_vec_rel, perps[i]);
                int16 dp_sign = Msign_int32(dp);
                if (i > 0 && dp_sign != last_dp_sign)
                {
                    fail = true;
                    break;
                }
                last_dp_sign = dp_sign;
            }
            if (fail) continue;

            uint16 *pixel = (uint16*)framebuffer->data + (y * framebuffer->width + x);
            *pixel = Mpixel_word(color.r, color.g, color.b);
        }
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    uint32 screen_width = 384;
    uint32 screen_height = 256;
    uint8 window_scale = 3;
    uint32 window_display_width  = screen_width * window_scale; 
    uint32 window_display_height = screen_height * window_scale;

    WNDCLASS wc = {0};
    wc.lpfnWndProc = windowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "Arnold";
    RegisterClass(&wc);

    DWORD window_style = WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT window_rect;
    window_rect.left   = 0;
    window_rect.top    = 0;
    window_rect.right  = window_display_width;
    window_rect.bottom = window_display_height;
    AdjustWindowRectEx(&window_rect, window_style, 0, 0);

    HWND window = CreateWindowEx(
        0,
        wc.lpszClassName,
        "Window",
        window_style,
        30, 30,
        window_rect.right  - window_rect.left, 
        window_rect.bottom - window_rect.top,
        NULL,
        NULL,
        hInstance,
        NULL);
    if (!window)
    {
        DWORD error_code = GetLastError();
        printf("Error creating the window. Error code is: %u.\n", error_code);
        return -1;
    }

    HDC device_context = GetDC(window);
    PIXELFORMATDESCRIPTOR pixel_format = {0};
    pixel_format.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pixel_format.nVersion = 1;
    pixel_format.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pixel_format.iPixelType = PFD_TYPE_RGBA;
    pixel_format.cColorBits = 32;
    pixel_format.cAlphaBits = 8;
    pixel_format.iLayerType = PFD_MAIN_PLANE;
    int pixel_format_index = ChoosePixelFormat(device_context, &pixel_format);
    DescribePixelFormat(device_context, pixel_format_index, sizeof(PIXELFORMATDESCRIPTOR), &pixel_format);
    SetPixelFormat(device_context, pixel_format_index, &pixel_format);

    HGLRC render_context = wglCreateContext(device_context);
    if (!render_context)
    {
        DWORD error_code = GetLastError();
        printf("Error creating OpenGL context. Error code is: %u.\n", error_code);
        return -1;
    }
    if (!wglMakeCurrent(device_context, render_context))
    {
        DWORD error_code = GetLastError();
        printf("OpenGL initialization error. Error code is: %u.\n", error_code);
        return -1;
    }
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    Memory memory;
    memory.size = 3*megabyte;
    memory.used = 0;
    memory.buffer = VirtualAlloc(0, memory.size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!memory.buffer)
    {
        DWORD error_code = GetLastError();
        printf("Could not allocate. Error code is: %u. Bye.\n", error_code);
        return -1;
    }

    Framebuffer framebuffer;
    framebuffer.width = screen_width;
    framebuffer.height = screen_height;
    framebuffer.pixel_size = 2;
    framebuffer.pixel_num = framebuffer.width * framebuffer.height;
    framebuffer.size = framebuffer.pixel_num * framebuffer.pixel_size;
    framebuffer.data = push_alloc(&memory, framebuffer.size);

    printf("Memory size: %u.\nFramebuffer size: %u.\nMemory remaining after framebuffer allocation: %u.\n",
        (uint32)memory.size, framebuffer.size, memory.size - memory.used);

    uint16 goose = 0;
    srand((uint32)time(NULL));
    uint32 seed = rand();

    ShowWindow(window, nCmdShow);

    int running = true;
    while (running)
    {
        MSG msg = {0};
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            switch (msg.message)
            {
                case WM_QUIT: 
                {
                    running = false;
                    break;
                }
            }
        }

        goose++;

        for (uint16 y = 0; y < framebuffer.height; y++)
        {
            for (uint16 x = 0; x < framebuffer.width; x++)
            {
                uint16 *pixel = (uint16*)framebuffer.data + (y * framebuffer.width + x);
                *pixel = 0;
                *pixel = Mpixel_word(x, y, y / 16 + goose);
            }
        }

        /*
        for (uint32 p = 0; p < framebuffer.pixel_num; p++)
        {
            uint16 *pixel = (uint16*)framebuffer.data + p;
            *pixel = Mpixel_word(0, 0, 0);
        }
        */

        srand(seed);
        for (int i=0; i<1000; i++)
        {
            vec2_fixed16 points[3] = 
            {
                {
                    Mscreen_coord(rand() & 255 + 90, rand() & 64),
                    Mscreen_coord(rand() & 255, rand() & 64),
                },
                {
                    Mscreen_coord(rand() & 255 + 90, rand() & 64),
                    Mscreen_coord(rand() & 255, rand() & 64),
                },
                {
                    Mscreen_coord(rand() & 255 + 90, rand() & 64),
                    Mscreen_coord(rand() & 255, rand() & 64),
                }
            };

            renderer_draw_face(
                &framebuffer,
                points, 3,
                make_vec3_uint8(goose*i / 30, goose*i / 15, 15 - (goose*i / 15)));
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
            framebuffer.width, framebuffer.height,
            0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, framebuffer.data);
        glBegin(GL_TRIANGLES);
        glTexCoord2i(0, 0); glVertex2i(-1, -1);
        glTexCoord2i(1, 0); glVertex2i( 1, -1);
        glTexCoord2i(0, 1); glVertex2i(-1,  1);
        glTexCoord2i(1, 1); glVertex2i( 1,  1);
        glTexCoord2i(0, 1); glVertex2i(-1,  1);
        glTexCoord2i(1, 0); glVertex2i( 1, -1);
        glEnd();
        SwapBuffers(device_context);
    }

    return 0;
}
