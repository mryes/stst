#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>
#include <windows.h>
#include <glad/glad.h>
#include <glad/glad_wgl.h>

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

#define array_length(array) (sizeof(array) / sizeof((array)[0]))
#define hasFlag(flags, flag) (((flags) & (flag)) != 0)

#include "stst_math.c"

struct Memory
{
    uint8 *buffer;
    uint64 size;
    uint64 used;
};

uint8 *alloc_push(struct Memory *memory, uint64 size)
{
    uint8 *result = memory->buffer;
    size = (size + 15) & ~0xF;
    memory->buffer += size;
    memory->used += size;
    assert(memory->used < memory->size);
    return result;
}

struct ReadFileResult
{
    enum
    {
        READ_FILE_SUCCESS,
        READ_FILE_OPEN_FAILED,
        READ_FILE_BUFFER_TOO_SMALL
    } status;
    char *buffer;
    int64 error;
    size_t size;
};
struct ReadFileResult read_file(const char *file_name, struct Memory *memory)
{
    struct ReadFileResult result;
    result.size = 0;
    FILE *file = fopen(file_name, "rb");
    if (!file)
    {
        result.status = READ_FILE_OPEN_FAILED;
        result.error = errno;
        return result;
    }
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    result.buffer = alloc_push(memory, file_size + 1);
    fseek(file, 0, SEEK_SET);
    fread(result.buffer, file_size, 1, file);
    fclose(file);
    result.status = READ_FILE_SUCCESS;
    result.size = file_size + 1;
    return result;
}

struct ReadFileResult read_string(const char *file_name, struct Memory *memory)
{
    struct ReadFileResult result = read_file(file_name, memory);
    if (result.status == READ_FILE_SUCCESS)
    {
        result.buffer[result.size] = 0;
    }
    return result;
}

struct Substring
{
    char *start;
    uint64 len;
};

void print_substr(struct Substring it)
{
    printf("%.*s", it.len, it.start);
}

int eq_substr(struct Substring a, struct Substring b)
{
    if (a.len != b.len)
    {
        return false;
    }
    return strncmp(a.start, b.start, a.len) == 0;
}

int eq_substr_str(struct Substring a, const char* b)
{
    if (a.len != strlen(b))
    {
        return false;
    }
    return strncmp(a.start, b, a.len) == 0;
}

struct Substring peek_word(char *c)
{
    struct Substring result;
    while (*c != 0 && isspace(*c))
    {
        c++;
    }
    result.start = c;
    result.len = 0;
    while (*c != 0 && !isspace(*c))
    {
        c++;
        result.len++;
    }
    return result;
}

struct Substring next_word(char** c)
{
    struct Substring result;
    while (**c != 0 && isspace(**c))
    {
        (*c)++;
    }
    result.start = *c;
    result.len = 0;
    while (**c != 0 && !isspace(**c))
    {
        (*c)++;
        result.len++;
    }
    return result;
}

struct AppState
{
   int8 renderer_sw; 
};

enum
{
    c_AttribLocationVertices = 0,
    c_AttribLocationColors = 1,
    c_AttribLocationNormals = 2,
    c_AttribLocationTexcoords = 3
};

enum
{
    f_MeshAttribVertices = 0x1,
    f_MeshAttribColors = 0x2,
    f_MeshAttribNormals = 0x4,
    f_MeshAttribTexcoords = 0x8
};
struct StoredMesh
{
    vec3f  *vertices;
    vec3f  *colors;
    vec3f  *normals;
    vec2f  *texcoords;
    uint32 *indices;
    uint32 num_vertices;
    uint32 num_colors;
    uint32 num_normals;
    uint32 num_texcoords;
    uint32 num_indices;
    uint8  attrib_flags;
};

struct Material
{
    vec3f object_color; 
    vec3f ambient_color;
    float ambient_amount;
    uint16 flags;
};

struct OpenGLMesh
{
    uint32 vertex_vbo;
    uint32 color_vbo;
    uint32 normal_vbo;
    uint32 texcoord_vbo;
    uint32 ebo;
    uint32 vao;
    struct StoredMesh *stored;
};

struct OpenGLShader
{
    uint32 id;
    int32 u_model;
    int32 u_normal_model;
    int32 u_view;
    int32 u_proj;
    int32 u_object_color;
    int32 u_ambient_color;
    int32 u_ambient_amount;
    int32 u_light_pos;
    int32 u_light_color;
};

void load_ply(
    const char *file_name, struct StoredMesh *stored_mesh,
    struct Memory *perm_section, struct Memory *temp_section)
{
    uint64 temp_section_last = temp_section->used;

    struct ReadFileResult read_file_result = read_string(file_name, temp_section);
    if (read_file_result.status != READ_FILE_SUCCESS)
    {
        printf("Couldn't load PLY %s. Status: %i, Error: %i.\n", 
            file_name, read_file_result.status, strerror(read_file_result.error));
        exit(-1);
    }
    char *c = read_file_result.buffer;

    while (!eq_substr_str(next_word(&c), "element")) {};

    uint64 num_verts = 0;
    if (eq_substr_str(next_word(&c), "vertex"))
    {
        num_verts = atoi(next_word(&c).start);
        stored_mesh->num_vertices = num_verts;
    }

    uint8 attribs[8];
    uint8 num_attribs = 0;
    attribs[num_attribs++] = f_MeshAttribVertices;
    stored_mesh->attrib_flags |= f_MeshAttribVertices;

    while (!eq_substr_str(peek_word(c), "element"))
    {
        struct Substring w1 = next_word(&c);
        struct Substring w2 = next_word(&c);
        struct Substring w3 = next_word(&c);
        if (eq_substr_str(w1, "property") 
        &&  eq_substr_str(w2, "float") 
        &&  eq_substr_str(w3, "nx"))
        {
            attribs[num_attribs++] = f_MeshAttribNormals;
            stored_mesh->attrib_flags |= f_MeshAttribNormals;
        }
        if (eq_substr_str(w1, "property") 
        &&  eq_substr_str(w2, "float") 
        &&  eq_substr_str(w3, "s"))
        {
            attribs[num_attribs++] = f_MeshAttribTexcoords;
            stored_mesh->attrib_flags |= f_MeshAttribTexcoords;
        }
        if (eq_substr_str(w1, "property") 
        &&  eq_substr_str(w2, "uchar") 
        &&  eq_substr_str(w3, "red"))
        {
            attribs[num_attribs++] = f_MeshAttribColors;
            stored_mesh->attrib_flags |= f_MeshAttribColors;
        }
    };

    next_word(&c);

    uint64 num_faces = 0;
    if (eq_substr_str(next_word(&c), "face"))
    {
        num_faces = atoi(next_word(&c).start);
    }

    while (!eq_substr_str(next_word(&c), "end_header")) {};

    if (hasFlag(stored_mesh->attrib_flags, f_MeshAttribVertices))
    {
        stored_mesh->vertices = (vec3f*)alloc_push(
            perm_section, num_verts * sizeof(stored_mesh->vertices[0]) * 3);
    }
    if (hasFlag(stored_mesh->attrib_flags, f_MeshAttribColors))
    {
        stored_mesh->colors = (vec3f*)alloc_push(
            perm_section, num_verts * sizeof(stored_mesh->colors[0]) * 3);
    }
    if (hasFlag(stored_mesh->attrib_flags, f_MeshAttribNormals))
    {
        stored_mesh->normals = (vec3f*)alloc_push(
            perm_section, num_verts * sizeof(stored_mesh->normals[0]) * 3);
    }
    if (hasFlag(stored_mesh->attrib_flags, f_MeshAttribTexcoords))
    {
        stored_mesh->texcoords = (vec2f*)alloc_push(
            perm_section, num_verts * sizeof(stored_mesh->texcoords[0]) * 2);
    }

    uint64 indices_allocated = num_faces * 4;
    stored_mesh->indices = (uint32*)alloc_push(
        perm_section, indices_allocated * sizeof(uint32));

    for (int i = 0; i<num_verts; i++)
    {
        for (int a = 0; a < num_attribs; a++)
        {
            if (attribs[a] == f_MeshAttribVertices)
            {
                stored_mesh->vertices[i].x = (float32)atof(next_word(&c).start);
                stored_mesh->vertices[i].y = (float32)atof(next_word(&c).start);
                stored_mesh->vertices[i].z = (float32)atof(next_word(&c).start);
            }
            else if (attribs[a] == f_MeshAttribTexcoords)
            {
                stored_mesh->texcoords[i].u = (float32)atof(next_word(&c).start);
                stored_mesh->texcoords[i].v = (float32)atof(next_word(&c).start);
            }
            else if (attribs[a] == f_MeshAttribNormals)
            {
                stored_mesh->normals[i].x = (float32)atof(next_word(&c).start);
                stored_mesh->normals[i].y = (float32)atof(next_word(&c).start);
                stored_mesh->normals[i].z = (float32)atof(next_word(&c).start);
            }
            else if (attribs[a] == f_MeshAttribColors)
            {
                stored_mesh->colors[i].r = (float32)(atof(next_word(&c).start) / 255);
                stored_mesh->colors[i].g = (float32)(atof(next_word(&c).start) / 255);
                stored_mesh->colors[i].b = (float32)(atof(next_word(&c).start) / 255);
            }
        }
    }

    uint64 num_indices = 0;
    for (int f = 0; f<num_faces; f++)
    {
        uint32 points = atoi(next_word(&c).start) - 3;
        if (points < 0 || points > 1) continue; // only allow 3 or 4 vertex faces, for now(?)
        uint32 first = atoi(next_word(&c).start);
        stored_mesh->indices[num_indices++] = first;
        stored_mesh->indices[num_indices++] = atoi(next_word(&c).start);
        uint32 last = atoi(next_word(&c).start);
        stored_mesh->indices[num_indices++] = last;
        // begin triangle fan
        for (int p = 0; p<points; p++)
        {
            stored_mesh->indices[num_indices++] = first;
            stored_mesh->indices[num_indices++] = last;
            last = atoi(next_word(&c).start);
            stored_mesh->indices[num_indices++] = last;
        }
    }

    if (num_indices < indices_allocated)
    {
        uint64 extra_indices = indices_allocated - num_indices;
        perm_section->used -= extra_indices * sizeof(uint32);
    }

    stored_mesh->num_indices = num_indices;

    temp_section->used = temp_section_last;
}

struct OpenGLMesh load_opengl_mesh(struct StoredMesh *stored_mesh)
{
    struct OpenGLMesh result = { 0 };
    result.stored = stored_mesh;
    glGenVertexArrays(1, &result.vao);
    glBindVertexArray(result.vao);
    if (hasFlag(stored_mesh->attrib_flags, f_MeshAttribVertices))
    {
        glGenBuffers(1, &result.vertex_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, result.vertex_vbo);
        glBufferData(
            GL_ARRAY_BUFFER, 
            stored_mesh->num_vertices * 3 * sizeof(float32),
            stored_mesh->vertices,
            GL_STATIC_DRAW);
        glVertexAttribPointer(c_AttribLocationVertices, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float32), (void*)0);
        glEnableVertexAttribArray(c_AttribLocationVertices);
    }
    if (hasFlag(stored_mesh->attrib_flags, f_MeshAttribColors))
    {
        glGenBuffers(1, &result.color_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, result.color_vbo);
        glBufferData(
            GL_ARRAY_BUFFER, 
            stored_mesh->num_vertices * 3 * sizeof(float32),
            stored_mesh->colors,
            GL_STATIC_DRAW);
        glVertexAttribPointer(c_AttribLocationColors, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float32), (void*)0);
        glEnableVertexAttribArray(c_AttribLocationColors);
    }
    if (hasFlag(stored_mesh->attrib_flags, f_MeshAttribNormals))
    {
        glGenBuffers(1, &result.normal_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, result.normal_vbo);
        glBufferData(
            GL_ARRAY_BUFFER, 
            stored_mesh->num_vertices * 3 * sizeof(float32),
            stored_mesh->normals,
            GL_STATIC_DRAW);
        glVertexAttribPointer(c_AttribLocationNormals, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float32), (void*)0);
        glEnableVertexAttribArray(c_AttribLocationNormals);
    }
    if (hasFlag(stored_mesh->attrib_flags, f_MeshAttribTexcoords))
    {
        glGenBuffers(1, &result.texcoord_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, result.texcoord_vbo);
        glBufferData(
            GL_ARRAY_BUFFER, 
            stored_mesh->num_vertices * 2 * sizeof(float32),
            stored_mesh->texcoords,
            GL_STATIC_DRAW);
        glVertexAttribPointer(c_AttribLocationTexcoords, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float32), (void*)0);
        glEnableVertexAttribArray(c_AttribLocationTexcoords);
    }
    glGenBuffers(1, &result.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        stored_mesh->num_indices * sizeof(uint32),
        stored_mesh->indices,
        GL_STATIC_DRAW);
    return result;
}

#define Mpixel_word(r, g, b) ((r & 15) << 12) | ((g & 15) << 8) | ((b & 15) << 4);

#define min_face_vertices 3
#define max_face_vertices 8

struct Framebuffer
{
    uint8 *data;
    uint32 size;
    int32 width;
    int32 height;
    uint8 pixel_size;
    uint32 pixel_num;
};

void renderer_draw_face(struct Framebuffer *framebuffer, int num_vertices, vec2f *vertices, vec3u *colors)
{
    assert(num_vertices >= min_face_vertices);
    assert(num_vertices <= max_face_vertices);

    vec2f min_corner = { INFINITY, INFINITY };
    vec2f max_corner = { -INFINITY, -INFINITY };
    for (int i=0; i<num_vertices; i++)
    {
        vec2f p = vertices[i];
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

    min_corner.x = (float32)floor(min_corner.x);
    min_corner.y = (float32)floor(min_corner.y);
    max_corner.x = (float32)ceil(max_corner.x);
    max_corner.y = (float32)ceil(max_corner.y);

    if (min_corner.x < 0)
    {
        min_corner.x = 0;
    }
    if (min_corner.x >= framebuffer->width)
    {
        min_corner.x = (float32)(framebuffer->width - 1);
    }
    if (min_corner.y < 0)
    {
        min_corner.y = 0;
    }
    if (min_corner.y >= framebuffer->height)
    {
        min_corner.y = (float32)(framebuffer->height - 1);
    }

    vec2f perps[min_face_vertices];
    vec2f lines[min_face_vertices];
    int vertex_indices[3] = { 0, 1, 2 };
    for (int vi=2; vi<num_vertices; vi++)
    {
        for (int i=0; i<min_face_vertices; i++)
        {
            int next = i + 1;
            if (next > 2) next = 0;
            lines[i] = sub_vec2_float32(vertices[vertex_indices[next]], vertices[vertex_indices[i]]);
            perps[i].x = -lines[i].y;
            perps[i].y =  lines[i].x;
        }

        for (int y = (int)min_corner.y; y <= (int)max_corner.y; y++)
        {
            for (int x = (int)min_corner.x; x <= (int)max_corner.x; x++)
            {
                vec2f pixel_vec_abs = make_vec2_float32(x + 0.5f, y + 0.5f);
                int16 last_dp_sign;
                int fail = false;
                for (int i=0; i<min_face_vertices; i++)
                {
                    vec2f pixel_vec_rel = 
                        sub_vec2_float32(pixel_vec_abs, vertices[vertex_indices[i]]);
                    float32 dp = dot_vec2_float32(pixel_vec_rel, perps[i]);
                    int16 dp_sign = sign_float32(dp);
                    if (i > 0 && dp_sign != last_dp_sign)
                    {
                        fail = true;
                        break;
                    }
                    last_dp_sign = dp_sign;
                }
                if (fail) continue;

                vec3_uint8 color = colors[0];
                uint16 *pixel = (uint16*)framebuffer->data + (y * framebuffer->width + x);
                *pixel = Mpixel_word(color.r, color.g, color.b);
            }
        }

        vertex_indices[1]++;
        vertex_indices[2]++;
    }
}

LRESULT CALLBACK WIN_window_proc_false(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(window, message, wParam, lParam);
}

LRESULT CALLBACK WIN_window_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
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

HWND WIN_create_window(WNDCLASS wc, uint32 display_width, uint32 display_height)
{
    DWORD window_style = WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT window_rect;
    window_rect.left = 0;
    window_rect.top = 0;
    window_rect.right = display_width;
    window_rect.bottom = display_height;
    AdjustWindowRectEx(&window_rect, window_style, 0, 0);

    HWND window = CreateWindowEx(
        0,
        wc.lpszClassName,
        "stst",
        window_style,
        30, 30,
        window_rect.right - window_rect.left,
        window_rect.bottom - window_rect.top,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    if (!window)
    {
        DWORD error_code = GetLastError();
        printf("Error creating the window. Error code is: %u.\n", error_code);
        exit(-1);
    }

    return window;
}

HGLRC WIN_create_opengl_context(HDC device_context)
{
    HGLRC render_context = wglCreateContext(device_context);
    if (!render_context)
    {
        DWORD error_code = GetLastError();
        printf("Error creating false OpenGL context. Error code is: %u.\n", error_code);
        exit(-1);
    }
    if (!wglMakeCurrent(device_context, render_context))
    {
        DWORD error_code = GetLastError();
        printf("OpenGL initialization error. Error code is: %u.\n", error_code);
        exit(-1);
    }
    return render_context;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    uint32 screen_width = 384;
    uint32 screen_height = 256;
    uint8 window_scale = 2;
    uint32 window_display_width = screen_width * window_scale;
    uint32 window_display_height = screen_height * window_scale;

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WIN_window_proc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "stst";
    RegisterClass(&wc);

    WNDCLASS wc_false = { 0 };
    wc_false.lpfnWndProc = WIN_window_proc_false;
    wc_false.hInstance = hInstance;
    wc_false.lpszClassName = "stst-false";
    RegisterClass(&wc_false);

    HWND window = WIN_create_window(wc_false, window_display_width, window_display_height);
    HDC device_context = GetDC(window);

    PIXELFORMATDESCRIPTOR pixel_format = { 0 };
    pixel_format.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pixel_format.nVersion = 1;
    pixel_format.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pixel_format.iPixelType = PFD_TYPE_RGBA;
    pixel_format.cAlphaBits = 8;
    pixel_format.cDepthBits = 24;
    pixel_format.cStencilBits = 8;
    pixel_format.iLayerType = PFD_MAIN_PLANE;
    int pixel_format_index = ChoosePixelFormat(device_context, &pixel_format);
    DescribePixelFormat(device_context, pixel_format_index, sizeof(PIXELFORMATDESCRIPTOR), &pixel_format);
    SetPixelFormat(device_context, pixel_format_index, &pixel_format);

    HGLRC render_context = WIN_create_opengl_context(device_context);

    if (!gladLoadGL())
    {
        printf("GLAD initialization error.\n");
        exit(-1);
    }
    if (!gladLoadWGL(device_context))
    {
        printf("GLAD initialization error.\n");
        exit(-1);
    }

    const int pixel_format_attributes[] =
    {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 32,
        WGL_DEPTH_BITS_ARB, 24,
        WGL_STENCIL_BITS_ARB, 8,
        WGL_SAMPLE_BUFFERS_ARB, 1,
        WGL_SAMPLES_ARB, 4,
        0,
    };

    uint32 num_formats;
    wglChoosePixelFormatARB(device_context, pixel_format_attributes, NULL, 1, &pixel_format_index, &num_formats);

    wglMakeCurrent(device_context, NULL);
    wglDeleteContext(render_context);
    DestroyWindow(window);

    window = WIN_create_window(wc, window_display_width, window_display_height);
    device_context = GetDC(window);

    DescribePixelFormat(device_context, pixel_format_index, sizeof(PIXELFORMATDESCRIPTOR), &pixel_format);
    SetPixelFormat(device_context, pixel_format_index, &pixel_format);

    render_context = WIN_create_opengl_context(device_context);

    glEnable(GL_TEXTURE_2D);

    struct Memory memory;
    memory.size = 50 * megabyte;
    memory.used = 0;
    memory.buffer = VirtualAlloc(0, memory.size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    assert((uintptr_t)memory.buffer % 16 == 0);
    if (!memory.buffer)
    {
        DWORD error_code = GetLastError();
        printf("Could not allocate. Error code is: %u. Bye.\n", error_code);
        return -1;
    }
    struct Memory perm_section;
    perm_section.size = 3 * megabyte;
    perm_section.used = 0;
    perm_section.buffer = alloc_push(&memory, perm_section.size);
    struct Memory temp_section;
    temp_section.size = 3 * megabyte;
    temp_section.used = 0;
    temp_section.buffer = alloc_push(&memory, temp_section.size);

    struct AppState *app_state = (struct AppState*)alloc_push(&perm_section, sizeof(struct AppState));
    app_state->renderer_sw = false;

    struct StoredMesh cube_mesh = { 0 };
    load_ply("assets/sphere_q2.ply", &cube_mesh, &perm_section, &temp_section);
    struct OpenGLMesh opengl_cube_mesh = load_opengl_mesh(&cube_mesh);

    struct OpenGLShader opengl_shader;

    glViewport(0, 0, window_display_width, window_display_height);
    glEnable(GL_DEPTH_TEST);

    uint32 vertex_shader;
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    struct ReadFileResult vertex_shader_src = read_string("assets/default.vertex", &temp_section);
    if (vertex_shader_src.status != READ_FILE_SUCCESS)
    {
        printf("Error loading vertex shader.\n");
        exit(-1);
    }
    glShaderSource(vertex_shader, 1, (const GLchar* const*)&vertex_shader_src.buffer, NULL);
    glCompileShader(vertex_shader);
    int successful;
    char info[512];
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &successful);
    if (!successful)
    {
        glGetShaderInfoLog(vertex_shader, 512, NULL, info);
        printf("Error compiling vertex shader:\n%s\n", info);
        exit(-1);
    }

    uint32 fragment_shader;
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    struct ReadFileResult fragment_shader_src = read_string("assets/default.fragment", &temp_section);
    if (fragment_shader_src.status != READ_FILE_SUCCESS)
    {
        printf("Error loading fragment shader.\n");
        exit(-1);
    }
    glShaderSource(fragment_shader, 1, (const GLchar* const*)&fragment_shader_src.buffer, NULL);
    glCompileShader(fragment_shader);
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &successful);
    if (!successful)
    {
        glGetShaderInfoLog(fragment_shader, 512, NULL, info);
        printf("Error compiling fragment shader.\n");
        exit(-1);
    }

    opengl_shader.id = glCreateProgram();
    glAttachShader(opengl_shader.id, vertex_shader);
    glAttachShader(opengl_shader.id, fragment_shader);
    glLinkProgram(opengl_shader.id);
    glGetProgramiv(opengl_shader.id, GL_LINK_STATUS, &successful);
    if (!successful)
    {
        glGetProgramInfoLog(opengl_shader.id, 512, NULL, info);
        printf("Error linking shader program.\n");
        exit(-1);
    }
    
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    opengl_shader.u_model = glGetUniformLocation(opengl_shader.id, "u_model");
    opengl_shader.u_normal_model = glGetUniformLocation(opengl_shader.id, "u_normal_model");
    opengl_shader.u_view = glGetUniformLocation(opengl_shader.id, "u_view");
    opengl_shader.u_proj = glGetUniformLocation(opengl_shader.id, "u_proj");
    opengl_shader.u_object_color = glGetUniformLocation(opengl_shader.id, "u_object_color");
    opengl_shader.u_ambient_color = glGetUniformLocation(opengl_shader.id, "u_ambient_color");
    opengl_shader.u_ambient_amount = glGetUniformLocation(opengl_shader.id, "u_ambient_amount");
    opengl_shader.u_light_pos = glGetUniformLocation(opengl_shader.id, "u_light_pos");
    opengl_shader.u_light_color = glGetUniformLocation(opengl_shader.id, "u_light_color");

    struct Framebuffer framebuffer;
    framebuffer.width = screen_width;
    framebuffer.height = screen_height;
    framebuffer.pixel_size = 2;
    framebuffer.pixel_num = framebuffer.width * framebuffer.height;
    framebuffer.size = framebuffer.pixel_num * framebuffer.pixel_size;
    framebuffer.data = alloc_push(&perm_section, framebuffer.size);

    LARGE_INTEGER perf_freq;
    QueryPerformanceFrequency(&perf_freq);

    srand((uint32)time(NULL));
    uint32 seed = rand();

    ShowWindow(window, nCmdShow);

    temp_section.used = 0;

    float32 goose = 0;

    int running = true;
    while (running)
    {
        LARGE_INTEGER perf_counter_start;
        QueryPerformanceCounter(&perf_counter_start);

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

        if (app_state->renderer_sw)
        {
            glBindTexture(GL_TEXTURE_2D, 1);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            for (uint32 p = 0; p < framebuffer.pixel_num; p++)
            {
                uint16 *pixel = (uint16*)framebuffer.data + p;
                *pixel = Mpixel_word(0, 255, 0);
            }

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
                framebuffer.width, framebuffer.height,
                0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, framebuffer.data);
            glBegin(GL_TRIANGLES);
            glTexCoord2i(0, 0); glVertex2i(-1, -1);
            glTexCoord2i(1, 0); glVertex2i(1, -1);
            glTexCoord2i(0, 1); glVertex2i(-1, 1);
            glTexCoord2i(1, 1); glVertex2i(1, 1);
            glTexCoord2i(0, 1); glVertex2i(-1, 1);
            glTexCoord2i(1, 0); glVertex2i(1, -1);
            glEnd();
        }
        else
        {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            mat4 model = mult_mat4(
                make_translate_mat4(make_vec3f(2*cos(goose), 0, -5 + 2*sin(goose))),
                make_rotate_mat4(make_vec3f(0, 1, 0), goose));
            mat3 normal_model = transpose_mat3(inverse_mat3(make_mat3_from_mat4(model)));
            mat4 view = make_identity_mat4();
            mat4 proj = make_persp_proj_mat4(0.7, (float32)screen_width/screen_height, 0.1, 50);
            goose += 0.005;
            if (goose > 2*M_PI)
            {
                goose -= 2*M_PI;
            }

            vec3f object_color = make_vec3f(1.0, 1.0, 1.0);
            vec3f ambient_color = make_vec3f(1, 0, 0);
            float32 ambient_amount = 0.3;
            vec3f light_pos = make_vec3f(0, 0, -5);
            vec3f light_color = make_vec3f(0, 1, 0);
            
            glUseProgram(opengl_shader.id);

            glUniformMatrix4fv(opengl_shader.u_model,1, GL_TRUE, (const GLfloat*)&model);
            glUniformMatrix3fv(opengl_shader.u_normal_model, 1, GL_TRUE, (const GLfloat*)&normal_model);
            glUniformMatrix4fv(opengl_shader.u_view, 1, GL_TRUE, (const GLfloat*)&view);
            glUniformMatrix4fv(opengl_shader.u_proj, 1, GL_TRUE, (const GLfloat*)&proj);
            glUniform3fv(opengl_shader.u_object_color, 1, (const GLfloat*)&object_color);
            glUniform3fv(opengl_shader.u_ambient_color, 1, (const GLfloat*)&ambient_color);
            glUniform1f(opengl_shader.u_ambient_amount, ambient_amount);
            glUniform3fv(opengl_shader.u_light_pos, 1, (const GLfloat*)&light_pos);
            glUniform3fv(opengl_shader.u_light_color, 1, (const GLfloat*)&light_color);

            glBindVertexArray(opengl_cube_mesh.vao);
            
            glDrawElements(GL_TRIANGLES, opengl_cube_mesh.stored->num_indices, GL_UNSIGNED_INT, 0);
        }

        SwapBuffers(device_context);

        /*
        LARGE_INTEGER perf_counter_end;
        QueryPerformanceCounter(&perf_counter_end);
        LARGE_INTEGER ticks_elapsed;
        ticks_elapsed.QuadPart = perf_counter_end.QuadPart - perf_counter_start.QuadPart;
        double seconds_elapsed = (double)ticks_elapsed.QuadPart / (double)perf_freq.QuadPart;
        printf("Seconds elapsed: %f, Ticks elapsed: %I64d, Ticks per second: %I64d\n",
            seconds_elapsed, ticks_elapsed, perf_freq.QuadPart);
        */
    }

    return 0;
}
