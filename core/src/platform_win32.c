#include "platform.h"
#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <string.h>
#include <glad/glad.h>

#pragma comment(lib, "opengl32.lib")

// --------------------------------------------------
// Internal Structures
// --------------------------------------------------

struct platform_window {
    HWND hwnd;
    HDC hdc;
    HGLRC glrc;

    int width, height;
    int framebuffer_width, framebuffer_height;
    uint32_t* framebuffer;

    GLuint texture_id, shader_program, vao, vbo;

    platform_key_callback key_cb;
    platform_mouse_button_callback mouse_button_cb;
    platform_mouse_move_callback mouse_move_cb;
    platform_resize_callback resize_cb;

    int mouse_x, mouse_y;
    int mouse_dx, mouse_dy;
    bool keys[PLATFORM_KEY_MAX];
    bool mouse_buttons[PLATFORM_MOUSE_BUTTON_MAX];
    bool should_close;
};

// Single global shared context
static HGLRC shared_glrc = NULL;
static HINSTANCE g_hInstance = NULL;

// Window list
#define MAX_WINDOWS 16
static platform_window* g_windows[MAX_WINDOWS];
static int g_window_count = 0;

// Shared rendering resources
static GLuint shared_shader_program = 0;
static GLuint shared_vao = 0;
static GLuint shared_vbo = 0;

// Forward declarations
LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static platform_key win32_to_platform_key(WPARAM wParam);
static void setup_window_gl();
static void setup_window_texture(platform_window* win);
static GLuint compile_shader(GLenum type, const char* src);

// --------------------------------------------------
// Platform Init / Shutdown
// --------------------------------------------------

bool platform_init() {
    g_hInstance = GetModuleHandle(NULL);

    WNDCLASSA wc = { 0 };
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = window_proc;
    wc.hInstance = g_hInstance;
    wc.lpszClassName = "PlatformWindowClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassA(&wc)) return false;
    return true;
}

void platform_shutdown() {
    for (int i = 0; i < g_window_count; ++i)
        platform_destroy_window(g_windows[i]);
    UnregisterClassA("PlatformWindowClass", g_hInstance);
}

// --------------------------------------------------
// Window Management
// --------------------------------------------------

platform_window* platform_create_window(const char* title, int width, int height, bool resizable, int framebuffer_width, int framebuffer_height) {
    if (g_window_count >= MAX_WINDOWS) return NULL;

    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!resizable) style &= ~WS_THICKFRAME;

    HWND hwnd = CreateWindowExA(
        0, "PlatformWindowClass", title, style,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, g_hInstance, NULL
    );
    if (!hwnd) return NULL;

    HDC hdc = GetDC(hwnd);

    PIXELFORMATDESCRIPTOR pfd = { 0 };
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    int pf = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pf, &pfd);

    HGLRC glrc = wglCreateContext(hdc);
    if (!glrc) return NULL;
    if (shared_glrc)
        wglShareLists(shared_glrc, glrc);
    else
        shared_glrc = glrc;

    wglMakeCurrent(hdc, glrc);

    if (!gladLoadGL()) {
        OutputDebugStringA("Failed to initialize GLAD\n");
        return NULL;
    }

    platform_window* win = (platform_window*)malloc(sizeof(platform_window));
    memset(win, 0, sizeof(platform_window));
    win->hwnd = hwnd;
    win->hdc = hdc;
    win->glrc = glrc;
    win->width = width;
    win->height = height;
    win->framebuffer = (uint32_t*)malloc(sizeof(uint32_t) * width * height);
    win->framebuffer_width = framebuffer_width;
    win->framebuffer_height = framebuffer_height;

    g_windows[g_window_count++] = win;

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    setup_window_gl(win);
    setup_window_texture(win);

    return win;
}

void platform_destroy_window(platform_window* window) {
    if (!window) return;
    for (int i = 0; i < g_window_count; ++i) {
        if (g_windows[i] == window) {
            memmove(&g_windows[i], &g_windows[i + 1], sizeof(platform_window*) * (g_window_count - i - 1));
            g_window_count--;
            break;
        }
    }

    if (window->glrc) wglDeleteContext(window->glrc);
    if (window->framebuffer) free(window->framebuffer);
    DestroyWindow(window->hwnd);
    free(window);
}

bool platform_window_should_close(platform_window* window) {
    return window->should_close;
}

// --------------------------------------------------
// Rendering
// --------------------------------------------------

static const char* vertex_shader_src =
"#version 120\n"
"attribute vec2 aPos;\n"
"attribute vec2 aTex;\n"
"varying vec2 vTex;\n"
"void main() { vTex = aTex; gl_Position = vec4(aPos, 0.0, 1.0); }";

static const char* fragment_shader_src =
"#version 120\n"
"uniform sampler2D uTexture;\n"
"varying vec2 vTex;\n"
"void main() { gl_FragColor = texture2D(uTexture, vTex); }";

static GLuint compile_shader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char buf[512];
        glGetShaderInfoLog(shader, 512, NULL, buf);
        OutputDebugStringA(buf);
    }
    return shader;
}

static void setup_window_gl(platform_window* win) {
    if (!win) return;
    wglMakeCurrent(win->hdc, win->glrc);

    // Compile shaders (vertex + fragment)
    GLuint vert = compile_shader(GL_VERTEX_SHADER, vertex_shader_src);
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_src);
    win->shader_program = glCreateProgram();
    glAttachShader(win->shader_program, vert);
    glAttachShader(win->shader_program, frag);
    glBindAttribLocation(win->shader_program, 0, "aPos");
    glBindAttribLocation(win->shader_program, 1, "aTex");
    glLinkProgram(win->shader_program);
    glDeleteShader(vert);
    glDeleteShader(frag);

    // Setup VAO/VBO
    float vertices[] = {
        -1, -1, 0, 1,
         3, -1, 2, 1,
        -1,  3, 0, -1
    };
    glGenVertexArrays(1, &win->vao);
    glGenBuffers(1, &win->vbo);
    glBindVertexArray(win->vao);
    glBindBuffer(GL_ARRAY_BUFFER, win->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));

    // Setup per-window texture
    glGenTextures(1, &win->texture_id);
    glBindTexture(GL_TEXTURE_2D, win->texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, win->framebuffer_width, win->framebuffer_height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, win->framebuffer);
}


static void setup_window_texture(platform_window* win) {
    if (!win) return;
    wglMakeCurrent(win->hdc, win->glrc);

    glGenTextures(1, &win->texture_id);
    glBindTexture(GL_TEXTURE_2D, win->texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, win->framebuffer_width, win->framebuffer_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, win->framebuffer);
}

void platform_put_pixel(platform_window* window, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (!window || x < 0 || x >= window->framebuffer_width || y < 0 || y >= window->framebuffer_height) return;
    window->framebuffer[y * window->framebuffer_width + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
}

void platform_blit(platform_window* win) {
    if (!win) return;
    wglMakeCurrent(win->hdc, win->glrc);

    glViewport(0, 0, win->width, win->height);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindTexture(GL_TEXTURE_2D, win->texture_id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, win->framebuffer_width, win->framebuffer_height,
        GL_RGBA, GL_UNSIGNED_BYTE, win->framebuffer);

    glUseProgram(win->shader_program);
    glBindVertexArray(win->vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void platform_swap_buffers(platform_window* window) {
    if (!window) return;
    SwapBuffers(window->hdc);
}

void platform_render(platform_window* window) {
    platform_blit(window);
    platform_swap_buffers(window);
}

// --------------------------------------------------
// Input / Callbacks
// --------------------------------------------------

bool platform_key_down(platform_key key) {
    for (int i = 0; i < g_window_count; ++i)
        if (g_windows[i]->keys[key]) return true;
    return false;
}

bool platform_mouse_button_down(platform_mouse_button button) {
    for (int i = 0; i < g_window_count; ++i)
        if (g_windows[i]->mouse_buttons[button]) return true;
    return false;
}

void platform_get_mouse_pos(platform_window* window, int* x, int* y) {
    if (!window || !x || !y) return;
    *x = window->mouse_x;
    *y = window->mouse_y;
}

void platform_get_mouse_delta(platform_window* window, int* dx, int* dy) {
    if (!window || !dx || !dy) return;
    *dx = window->mouse_dx;
    *dy = window->mouse_dy;
    window->mouse_dx = window->mouse_dy = 0;
}

// --------------------------------------------------
// Callbacks registration
// --------------------------------------------------

void platform_set_key_callback(platform_window* window, platform_key_callback cb) { window->key_cb = cb; }
void platform_set_mouse_button_callback(platform_window* window, platform_mouse_button_callback cb) { window->mouse_button_cb = cb; }
void platform_set_mouse_move_callback(platform_window* window, platform_mouse_move_callback cb) { window->mouse_move_cb = cb; }
void platform_set_resize_callback(platform_window* window, platform_resize_callback cb) { window->resize_cb = cb; }

// --------------------------------------------------
// Event Loop
// --------------------------------------------------

void platform_poll_events() {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// --------------------------------------------------
// Win32 Window Proc
// --------------------------------------------------

LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    platform_window* win = NULL;
    for (int i = 0; i < g_window_count; ++i)
        if (g_windows[i]->hwnd == hwnd) { win = g_windows[i]; break; }

    switch (uMsg) {
    case WM_CLOSE: if (win) win->should_close = true; return 0;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP: {
        if (!win) break;
        bool down = (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN);
        platform_key key = win32_to_platform_key(wParam);
        if (key != PLATFORM_KEY_UNKNOWN) {
            win->keys[key] = down;
            if (win->key_cb) win->key_cb(win, key, down);
        }
        return 0;
    }

    case WM_LBUTTONDOWN: case WM_LBUTTONUP:
    case WM_RBUTTONDOWN: case WM_RBUTTONUP:
    case WM_MBUTTONDOWN: case WM_MBUTTONUP: {
        if (!win) break;
        platform_mouse_button button;
        bool down = (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN || uMsg == WM_MBUTTONDOWN);
        switch (uMsg) {
        case WM_LBUTTONDOWN: case WM_LBUTTONUP: button = PLATFORM_MOUSE_BUTTON_LEFT; break;
        case WM_RBUTTONDOWN: case WM_RBUTTONUP: button = PLATFORM_MOUSE_BUTTON_RIGHT; break;
        case WM_MBUTTONDOWN: case WM_MBUTTONUP: button = PLATFORM_MOUSE_BUTTON_MIDDLE; break;
        default: return 0;
        }
        win->mouse_buttons[button] = down;
        if (win->mouse_button_cb) win->mouse_button_cb(win, button, down);
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (!win) break;
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        win->mouse_dx += x - win->mouse_x;
        win->mouse_dy += y - win->mouse_y;
        win->mouse_x = x;
        win->mouse_y = y;
        if (win->mouse_move_cb) win->mouse_move_cb(win, x, y);
        return 0;
    }

    case WM_SIZE: {
        if (!win) break;
        win->width = LOWORD(lParam);
        win->height = HIWORD(lParam);
        if (win->resize_cb) win->resize_cb(win, win->width, win->height);
        return 0;
    }
    }

    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

// --------------------------------------------------
// Win32 -> Platform Key Mapping
// --------------------------------------------------

static platform_key win32_to_platform_key(WPARAM wParam) {
    switch (wParam) {
    case 'A': return PLATFORM_KEY_A; case 'B': return PLATFORM_KEY_B; case 'C': return PLATFORM_KEY_C;
    case 'D': return PLATFORM_KEY_D; case 'E': return PLATFORM_KEY_E; case 'F': return PLATFORM_KEY_F;
    case 'G': return PLATFORM_KEY_G; case 'H': return PLATFORM_KEY_H; case 'I': return PLATFORM_KEY_I;
    case 'J': return PLATFORM_KEY_J; case 'K': return PLATFORM_KEY_K; case 'L': return PLATFORM_KEY_L;
    case 'M': return PLATFORM_KEY_M; case 'N': return PLATFORM_KEY_N; case 'O': return PLATFORM_KEY_O;
    case 'P': return PLATFORM_KEY_P; case 'Q': return PLATFORM_KEY_Q; case 'R': return PLATFORM_KEY_R;
    case 'S': return PLATFORM_KEY_S; case 'T': return PLATFORM_KEY_T; case 'U': return PLATFORM_KEY_U;
    case 'V': return PLATFORM_KEY_V; case 'W': return PLATFORM_KEY_W; case 'X': return PLATFORM_KEY_X;
    case 'Y': return PLATFORM_KEY_Y; case 'Z': return PLATFORM_KEY_Z;
    case '0': return PLATFORM_KEY_0; case '1': return PLATFORM_KEY_1; case '2': return PLATFORM_KEY_2;
    case '3': return PLATFORM_KEY_3; case '4': return PLATFORM_KEY_4; case '5': return PLATFORM_KEY_5;
    case '6': return PLATFORM_KEY_6; case '7': return PLATFORM_KEY_7; case '8': return PLATFORM_KEY_8;
    case '9': return PLATFORM_KEY_9;
    case VK_ESCAPE: return PLATFORM_KEY_ESCAPE; case VK_SPACE: return PLATFORM_KEY_SPACE;
    case VK_RETURN: return PLATFORM_KEY_ENTER; case VK_TAB: return PLATFORM_KEY_TAB;
    case VK_BACK: return PLATFORM_KEY_BACKSPACE;
    case VK_LEFT: return PLATFORM_KEY_LEFT; case VK_RIGHT: return PLATFORM_KEY_RIGHT;
    case VK_UP: return PLATFORM_KEY_UP; case VK_DOWN: return PLATFORM_KEY_DOWN;
    default: return PLATFORM_KEY_UNKNOWN;
    }
}
