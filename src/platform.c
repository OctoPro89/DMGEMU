#include "platform.h"

keys keyboard;

#ifdef _POSIX_C_SOURCE

#if _POSIX_C_SOURCE >= 199309L
#include <time.h>  // nanosleep
#else
#include <unistd.h>  // usleep
#endif

void platform_sleep_ms(u64 ms) {
#if _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000 * 1000;
    nanosleep(&ts, 0);
#else
    if (ms >= 1000) {
        sleep(ms / 1000);
    }
    usleep((ms % 1000) * 1000);
#endif
}

f64 platform_get_elapsed_time_ms() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec * 1000.0 + now.tv_nsec * 0.000001;
}

#include <X11/Xlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glad/glad.h>
#include <GL/glx.h>

u8 running = true;
Display *display;
Window window;
GLXContext glContext;
Atom WM_DELETE_WINDOW;
XEvent event;
GLuint program;
GLuint vao, vbo;
GLuint texture;

typedef void (*glXSwapIntervalEXTProc)(Display*, GLXDrawable, int);

u8 framebuffer[FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT * 3]; // RGB software framebuffer

// Vertex + texture coordinate shader
const char *vertexShaderSource =
    "#version 130\n"
    "in vec2 position;\n"
    "in vec2 texCoord;\n"
    "out vec2 TexCoord;\n"
    "void main() {\n"
    "    TexCoord = texCoord;\n"
    "    gl_Position = vec4(position, 0.0, 1.0);\n"
    "}\n";

const char *fragmentShaderSource =
    "#version 130\n"
    "in vec2 TexCoord;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D screenTexture;\n"
    "void main() {\n"
    "    FragColor = texture(screenTexture, TexCoord);\n"
    "}\n";

void platform_put_pixel(i32 x, i32 y, u8 r, u8 g, u8 b) {
    if (x < 0 || x >= FRAMEBUFFER_WIDTH || y < 0 || y >= FRAMEBUFFER_HEIGHT) return;
    i32 index = (y * FRAMEBUFFER_WIDTH + x) * 3;
    framebuffer[index + 0] = r;
    framebuffer[index + 1] = g;
    framebuffer[index + 2] = b;
}

GLuint compileShader(GLenum type, const char *source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        fprintf(stderr, "Shader error: %s\n", log);
        exit(1);
    }
    return shader;
}

GLuint createShaderProgram() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

void setupQuad() {
    float quadVertices[] = {
        // positions   // tex coords
        -1.0f,  1.0f,   0.0f, 1.0f,
        -1.0f, -1.0f,   0.0f, 0.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
        -1.0f,  1.0f,   0.0f, 1.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
         1.0f,  1.0f,   1.0f, 1.0f
    };

    GLuint vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void setupTexture() {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, framebuffer);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // NES-style
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void updateTexture() {
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, framebuffer);
}

void platform_render() {
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);
    glBindVertexArray(vao);
    updateTexture();
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glXSwapBuffers(display, window);
}

void platform_shutdown() {
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(1, &texture);
    glDeleteProgram(program);
    glXMakeCurrent(display, None, NULL);
    glXDestroyContext(display, glContext);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
}

void platform_pump_messages() {
        while (XPending(display)) {
            XNextEvent(display, &event);

            switch (event.type) {
            case KeyPress: {
                // Filter out auto-repeat key presses
                if (XEventsQueued(display, QueuedAfterReading)) {
                    XEvent next_event;
                    XPeekEvent(display, &next_event);

                    if (next_event.type == KeyRelease &&
                        next_event.xkey.time == event.xkey.time &&
                        next_event.xkey.keycode == event.xkey.keycode) {
                        // This is an auto-repeat, skip it
                        break;
                    }
                }

                KeySym k = XLookupKeysym(&event.xkey, 0);

                switch (k) {
                    case XK_space: {
                        keyboard.space = true;
                        break;
                    }
                    case XK_Up: {
                        keyboard.up = true;
                        break;
                    }
                    case XK_Down: {
                        keyboard.down = true;
                        break;
                    }
                    case XK_Left: {
                        keyboard.left = true;
                        break;
                    }
                    case XK_Right: {
                        keyboard.right = true;
                        break;
                    }
                    case XK_x: {
                        keyboard.x = true;
                        break;
                    }
                    case XK_z: {
                        keyboard.z = true;
                        break;
                    }
                    case XK_f: {
                        keyboard.f = true;
                        break;
                    }
                    case XK_c: {
                        keyboard.c = true;
                        break;
                    }
                    case XK_u: {
                        keyboard.u = true;
                        break;
                    }
                    case XK_Return: {
                        keyboard.enter = true;
                        break;
                    }
                    case XK_Shift_R: {
                        keyboard.shift = true;
                        break;
                    }
                    }

                    break;
                }

                case KeyRelease: {
                    KeySym k = XLookupKeysym(&event.xkey, 0);

                    switch (k) {
                    case XK_space: {
                        keyboard.space = false;
                        break;
                    }
                    case XK_Up: {
                        keyboard.up = false;
                        break;
                    }
                    case XK_Down: {
                        keyboard.down = false;
                        break;
                    }
                    case XK_Left: {
                        keyboard.left = false;
                        break;
                    }
                    case XK_Right: {
                        keyboard.right = false;
                        break;
                    }
                    case XK_x: {
                        keyboard.x = false;
                        break;
                    }
                    case XK_z: {
                        keyboard.z = false;
                        break;
                    }
                    case XK_f: {
                        keyboard.f = false;
                        break;
                    }
                    case XK_c: {
                        keyboard.c = false;
                        break;
                    }
                    case XK_u: {
                        keyboard.u = false;
                        break;
                    }
                    case XK_Return: {
                        keyboard.enter = false;
                        break;
                    }
                    case XK_Shift_R: {
                        keyboard.shift = false;
                        break;
                    }
                    }
                }
                break;

                case ClientMessage: {
                    if ((Atom)event.xclient.data.l[0] == WM_DELETE_WINDOW) {
                        running = 0;
                    }

                    break;
                }
                break;
            }
        }
}

u8 platform_open_window(i32 width, i32 height) {
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Failed to open X display\n");
        return false;
    }

    i32 screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    XVisualInfo vi;
    XMatchVisualInfo(display, screen, 24, TrueColor, &vi);

    XSetWindowAttributes swa;
    swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask;

    window = XCreateWindow(display, root, 0, 0, width, height, 0,
                           vi.depth, InputOutput, vi.visual, CWEventMask, &swa);
    XMapWindow(display, window);
    XFlush(display);

    glContext = glXCreateContext(display, &vi, NULL, GL_TRUE);
    glXMakeCurrent(display, window, glContext);

    WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1);

    if (!gladLoadGLLoader((GLADloadproc)glXGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return false;
    }

    glXSwapIntervalEXTProc glXSwapIntervalEXT = 
        (glXSwapIntervalEXTProc)glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalEXT");

    if (glXSwapIntervalEXT) {
        glXSwapIntervalEXT(display, glXGetCurrentDrawable(), 0); // 0 = disable VSync
        printf("VSync disabled\n");
    } else {
        printf("glXSwapIntervalEXT not supported! VSync enabled\n");
    }

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    program = createShaderProgram();

    setupQuad();
    setupTexture();

    return true;
}

keys platform_get_keys() {
    return keyboard;
}

u8 platform_should_run() {
    return running;
}

#endif // _POSIX_C_SOURCE

#ifdef _WIN64

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glad/glad.h>

#undef APIENTRY

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <timeapi.h>

#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "opengl32.lib")

HDC hDC;
HGLRC hRC;
HWND hWnd;
HINSTANCE hInstance;
GLuint program;
GLuint vao, vbo;
GLuint texture;
MSG msg;
u8 running = true;
LARGE_INTEGER qpc_frequency;

LPCSTR class_name = "NESEMUWINDOWCLASS";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

u8 framebuffer[FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT * 3]; // RGB software framebuffer

// Vertex + texture coordinate shader
const char* vertexShaderSource =
"#version 130\n"
"in vec2 position;\n"
"in vec2 texCoord;\n"
"out vec2 TexCoord;\n"
"void main() {\n"
"    TexCoord = texCoord;\n"
"    gl_Position = vec4(position, 0.0, 1.0);\n"
"}\n";

const char* fragmentShaderSource =
"#version 130\n"
"in vec2 TexCoord;\n"
"out vec4 FragColor;\n"
"uniform sampler2D screenTexture;\n"
"void main() {\n"
"    FragColor = texture(screenTexture, TexCoord);\n"
"}\n";

u8 SetupPixelFormat(HDC hdc) {
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, // Flags
        PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or color index.
        32,                   // Color depth
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,                   // Depth buffer
        8,                    // Stencil buffer
        0,                    // Auxiliary buffer
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };

    i32 pixelFormat = ChoosePixelFormat(hdc, &pfd);
    if (pixelFormat == 0) return false;

    return SetPixelFormat(hdc, pixelFormat, &pfd);
}

// Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CLOSE:
        case WM_DESTROY:
            running = false;
            return 0;
        case WM_KEYDOWN:
            switch (wParam) {
                case VK_SHIFT:
                    keyboard.shift = true;
                    break;
                case VK_RETURN:
                    keyboard.enter = true;
                    break;
                case VK_UP:
                    keyboard.up = true;
                    break;
                case VK_DOWN:
                    keyboard.down = true;
                    break;
                case VK_LEFT:
                    keyboard.left = true;
                    break;
                case VK_RIGHT:
                    keyboard.right = true;
                    break;
                case VK_SPACE:
                    keyboard.space = true;
                    break;
                case 'Z':
                    keyboard.z = true;
                    break;
                case 'X':
                    keyboard.x = true;
                    break;
                case 'C':
                    keyboard.c = true;
                    break;
                case 'F':
                    keyboard.f = true;
                    break;
                case 'U':
                    keyboard.u = true;
                    break;
            }
            break;
        case WM_KEYUP:
            switch (wParam) {
                case VK_SHIFT:
                    keyboard.shift = false;
                    break;
                case VK_RETURN:
                    keyboard.enter = false;
                    break;
                case VK_UP:
                    keyboard.up = false;
                    break;
                case VK_DOWN:
                    keyboard.down = false;
                    break;
                case VK_LEFT:
                    keyboard.left = false;
                    break;
                case VK_RIGHT:
                    keyboard.right = false;
                    break;
                case VK_SPACE:
                    keyboard.space = false;
                    break;
                case 'Z':
                    keyboard.z = false;
                    break;
                case 'X':
                    keyboard.x = false;
                    break;
                case 'C':
                    keyboard.c = false;
                    break;
                case 'F':
                    keyboard.f = false;
                    break;
                case 'U':
                    keyboard.u = false;
                    break;
            }
            break;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void platform_sleep_ms(u64 ms) {
    Sleep((DWORD)ms);
}

f64 platform_get_elapsed_time_ms() {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return ((f64)(now.QuadPart) * 1000.0) / (f64)(qpc_frequency.QuadPart);
}

void platform_put_pixel(i32 x, i32 y, u8 r, u8 g, u8 b) {
    // if (x < 0 || x >= FRAMEBUFFER_WIDTH || y < 0 || y >= FRAMEBUFFER_HEIGHT) return;
    i32 index = (y * FRAMEBUFFER_WIDTH + x) * 3;
    framebuffer[index + 0] = r;
    framebuffer[index + 1] = g;
    framebuffer[index + 2] = b;
}

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        fprintf(stderr, "Shader error: %s\n", log);
        exit(1);
    }
    return shader;
}

GLuint createShaderProgram() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

void setupQuad() {
    float quadVertices[] = {
        // positions   // tex coords
        -1.0f,  1.0f,   0.0f, 1.0f,
        -1.0f, -1.0f,   0.0f, 0.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
        -1.0f,  1.0f,   0.0f, 1.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
         1.0f,  1.0f,   1.0f, 1.0f
    };

    GLuint vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void setupTexture() {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, framebuffer);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // NES-style
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void updateTexture() {
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, framebuffer);
}

void platform_render() {
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);
    glBindVertexArray(vao);
    updateTexture();
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    SwapBuffers(hDC);
}

void platform_shutdown() {
    timeEndPeriod(1);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(1, &texture);
    glDeleteProgram(program);
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hRC);
    ReleaseDC(hWnd, hDC);
    DestroyWindow(hWnd);
}

u8 platform_should_run() {
    return running == 1;
}

void platform_pump_messages() {
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int interval);

u8 platform_open_window(i32 width, i32 height) {
    timeBeginPeriod(1);

    hInstance = GetModuleHandleA(NULL);

    WNDCLASSA wc = { 0 };
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = class_name;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "RegisterClass failed!", "Error", MB_OK);
        return false;
    }

    RECT rect = { 0, 0, width, height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    // rect now contains the total window size needed
    i32 windowWidth = rect.right - rect.left;
    i32 windowHeight = rect.bottom - rect.top;

    LPCSTR title = "GBEMU";

    hWnd = CreateWindowExA(
        0, class_name, title,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight,
        NULL, NULL, hInstance, NULL
    );

    if (!hWnd) {
        MessageBoxA(NULL, "CreateWindowExA failed!", "Error", MB_OK);
        return false;
    }

    hDC = GetDC(hWnd);

    if (!SetupPixelFormat(hDC)) {
        MessageBoxA(NULL, "Failed to set pixel format.", "Error", MB_OK);
        return false;
    }

    hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hRC);

    // Load GLAD
    if (!gladLoadGL()) {
        MessageBoxA(NULL, "Failed to initialize GLAD", "Error", MB_OK);
        return false;
    }

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glViewport(0, 0, width, height);

    program = createShaderProgram();

    setupQuad();
    setupTexture();

    QueryPerformanceFrequency(&qpc_frequency);

    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;
    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

    if (wglSwapIntervalEXT) {
        wglSwapIntervalEXT(0);  // Disable VSync
        printf("VSync disabled\n");
    }
    else {
        printf("wglSwapIntervalEXT not supported! VSync enabled\n");
    }

    return true;
}

keys platform_get_keys() {
    return keyboard;
}

#endif // _WIN64