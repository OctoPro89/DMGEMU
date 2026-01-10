#ifdef __linux__
#include "platform.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>

#include <alsa/asoundlib.h>
#include <pthread.h>
#include <sched.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <glad/glad.h>
#include <GL/glx.h>

#define AUDIO_CHANNELS 2
#define AUDIO_BUFFER_SAMPLES 48000

static snd_pcm_t* pcm_handle;
static pthread_t audio_thread;
static bool audio_running;

static float* audio_buffer;
static volatile uint32_t audio_read_pos;
static volatile uint32_t audio_write_pos;

static int g_audio_sample_rate;

static platform_key x11_to_platform_key(KeySym ks);

struct platform_window {
    Display* display;
    Window   window;
    GLXContext gl;

    Atom WM_DELETE_WINDOW;

    int width, height;
    int framebuffer_width, framebuffer_height;
    uint32_t* framebuffer;

    GLuint texture_id;
    GLuint shader_program;
    GLuint vao, vbo;

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

#define MAX_WINDOWS 16
static platform_window* g_windows[MAX_WINDOWS];
static int g_window_count = 0;

// typedef void PFNGLXSWAPINTERVALEXTPROC(Display *dpy, GLXDrawable drawable, int interval);
static PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT = NULL;

static GLXContext shared_gl = NULL;

bool platform_init() {
    return true;
}

void platform_shutdown() {
    for (int i = 0; i < g_window_count; ++i)
        platform_destroy_window(g_windows[i]);
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
        printf("%s\n", buf);
    }
    return shader;
}

static void setup_window_gl(platform_window* win) {
    if (!win) return;
    glXMakeCurrent(win->display, win->window, win->gl);
    glXSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddress("glXSwapIntervalEXT");

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
    glXMakeCurrent(win->display, win->window, win->gl);
    glGenTextures(1, &win->texture_id);
    glBindTexture(GL_TEXTURE_2D, win->texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, win->framebuffer_width, win->framebuffer_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, win->framebuffer);
}

void platform_set_vsync(bool enabled) {
    for (int i = 0; i < g_window_count; ++i) {
        glXMakeCurrent(g_windows[i]->display, g_windows[i]->window, g_windows[i]->gl);
        glXSwapIntervalEXT(g_windows[i]->display, g_windows[i]->window, enabled ? 1 : 0);
    }
}

platform_window* platform_create_window(
    const char* title,
    int width, int height,
    bool resizable,
    int framebuffer_width, int framebuffer_height
) {
    if (g_window_count >= MAX_WINDOWS) return NULL;

    Display* dpy = XOpenDisplay(NULL);
    if (!dpy) return NULL;

    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);

    static int visual_attribs[] = {
        GLX_RGBA,
        GLX_DOUBLEBUFFER,
        GLX_DEPTH_SIZE, 24,
        None
    };

    XVisualInfo* vi = glXChooseVisual(dpy, screen, visual_attribs);
    if (!vi) return NULL;

    XSetWindowAttributes swa = {0};
    swa.colormap = XCreateColormap(dpy, root, vi->visual, AllocNone);
    swa.event_mask =
        ExposureMask |
        KeyPressMask | KeyReleaseMask |
        ButtonPressMask | ButtonReleaseMask |
        PointerMotionMask |
        StructureNotifyMask;

    Window win = XCreateWindow(
        dpy, root,
        0, 0, width, height,
        0, vi->depth,
        InputOutput,
        vi->visual,
        CWColormap | CWEventMask,
        &swa
    );

    XStoreName(dpy, win, title);
    XMapWindow(dpy, win);

    GLXContext gl = glXCreateContext(dpy, vi, shared_gl, True);
    if (!shared_gl) shared_gl = gl;

    glXMakeCurrent(dpy, win, gl);

    if (!gladLoadGLLoader((GLADloadproc)glXGetProcAddress)) {
        fprintf(stderr, "Failed to init GLAD\n");
        return NULL;
    }

    platform_window* pw = calloc(1, sizeof(platform_window));
    pw->display = dpy;
    pw->window = win;
    pw->gl = gl;
    pw->width = width;
    pw->height = height;
    pw->framebuffer_width = framebuffer_width;
    pw->framebuffer_height = framebuffer_height;
    pw->framebuffer = malloc(sizeof(uint32_t) * framebuffer_width * framebuffer_height);

    pw->WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, win, &pw->WM_DELETE_WINDOW, 1);

    g_windows[g_window_count++] = pw;

    setup_window_gl(pw);
    setup_window_texture(pw);

    return pw;
}

void platform_destroy_window(platform_window* w) {
    if (!w) return;

    for (int i = 0; i < g_window_count; ++i) {
        if (g_windows[i] == w) {
            memmove(&g_windows[i], &g_windows[i + 1],
                    sizeof(platform_window*) * (g_window_count - i - 1));
            g_window_count--;
            break;
        }
    }

    glXMakeCurrent(w->display, None, NULL);
    glXDestroyContext(w->display, w->gl);
    XDestroyWindow(w->display, w->window);
    XCloseDisplay(w->display);

    free(w->framebuffer);
    free(w);
}

void platform_poll_events() {
    for (int i = 0; i < g_window_count; ++i) {
        platform_window* w = g_windows[i];
        w->mouse_dx = w->mouse_dy = 0;

        while (XPending(w->display)) {
            XEvent e;
            XNextEvent(w->display, &e);

            switch (e.type) {
            case ClientMessage:
                if ((Atom)e.xclient.data.l[0] == w->WM_DELETE_WINDOW)
                    w->should_close = true;
                break;

            case KeyPress:
            case KeyRelease: {
                bool down = (e.type == KeyPress);
                platform_key key = x11_to_platform_key(
                    XLookupKeysym(&e.xkey, 0)
                );
                if (key != PLATFORM_KEY_UNKNOWN) {
                    w->keys[key] = down;
                    if (w->key_cb) w->key_cb(w, key, down);
                }
            } break;

            case ButtonPress:
            case ButtonRelease: {
                bool down = (e.type == ButtonPress);
                platform_mouse_button btn;

                if (e.xbutton.button == Button1) btn = PLATFORM_MOUSE_BUTTON_LEFT;
                else if (e.xbutton.button == Button3) btn = PLATFORM_MOUSE_BUTTON_RIGHT;
                else if (e.xbutton.button == Button2) btn = PLATFORM_MOUSE_BUTTON_MIDDLE;
                else break;

                w->mouse_buttons[btn] = down;
                if (w->mouse_button_cb)
                    w->mouse_button_cb(w, btn, down);
            } break;

            case MotionNotify: {
                int x = e.xmotion.x;
                int y = e.xmotion.y;
                w->mouse_dx += x - w->mouse_x;
                w->mouse_dy += y - w->mouse_y;
                w->mouse_x = x;
                w->mouse_y = y;
                if (w->mouse_move_cb)
                    w->mouse_move_cb(w, x, y);
            } break;

            case ConfigureNotify:
                w->width = e.xconfigure.width;
                w->height = e.xconfigure.height;
                if (w->resize_cb)
                    w->resize_cb(w, w->width, w->height);
                break;
            }
        }
    }
}

bool platform_key_down(platform_key key) {
    for (int i = 0; i < g_window_count; ++i)
        if (g_windows[i]->keys[key]) return true;
    return false;
}

bool platform_mouse_button_down(platform_mouse_button b) {
    for (int i = 0; i < g_window_count; ++i)
        if (g_windows[i]->mouse_buttons[b]) return true;
    return false;
}

void platform_get_mouse_pos(platform_window* w, int* x, int* y) {
    if (!w) return;
    if (x) *x = w->mouse_x;
    if (y) *y = w->mouse_y;
}

void platform_get_mouse_delta(platform_window* w, int* dx, int* dy) {
    if (!w) return;
    if (dx) *dx = w->mouse_dx;
    if (dy) *dy = w->mouse_dy;
    w->mouse_dx = w->mouse_dy = 0;
}

static platform_key x11_to_platform_key(KeySym ks) {
    if (ks >= XK_A && ks <= XK_Z) return PLATFORM_KEY_A + (ks - XK_A);
    if (ks >= XK_a && ks <= XK_z) return PLATFORM_KEY_A + (ks - XK_a);
    if (ks >= XK_0 && ks <= XK_9) return PLATFORM_KEY_0 + (ks - XK_0);

    switch (ks) {
    case XK_Escape: return PLATFORM_KEY_ESCAPE;
    case XK_space: return PLATFORM_KEY_SPACE;
    case XK_Return: return PLATFORM_KEY_ENTER;
    case XK_Tab: return PLATFORM_KEY_TAB;
    case XK_BackSpace: return PLATFORM_KEY_BACKSPACE;
    case XK_Left: return PLATFORM_KEY_LEFT;
    case XK_Right: return PLATFORM_KEY_RIGHT;
    case XK_Up: return PLATFORM_KEY_UP;
    case XK_Down: return PLATFORM_KEY_DOWN;
    default: return PLATFORM_KEY_UNKNOWN;
    }
}

void platform_put_pixel(platform_window* window, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (!window || x < 0 || x >= window->framebuffer_width || y < 0 || y >= window->framebuffer_height) return;
    window->framebuffer[y * window->framebuffer_width + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
}

void platform_blit(platform_window* win) {
    if (!win) return;
    glXMakeCurrent(win->display, win->window, win->gl);

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
    glXSwapBuffers(window->display, window->window);
}

void platform_render(platform_window* w) {
    platform_blit(w);
    platform_swap_buffers(w);
}

static void* platform_audio_thread(void* arg)
{
    /* Try to raise thread priority (best effort) */
    struct sched_param param = { .sched_priority = 20 };
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

    const uint32_t frames_per_chunk = 256;
    float local_buffer[256 * AUDIO_CHANNELS];

    while (audio_running) {
        uint32_t frames_written = 0;

        for (uint32_t i = 0; i < frames_per_chunk; ++i) {
            for (int ch = 0; ch < AUDIO_CHANNELS; ++ch) {
                if (audio_read_pos != audio_write_pos) {
                    local_buffer[i * AUDIO_CHANNELS + ch] =
                        audio_buffer[audio_read_pos];
                    audio_read_pos =
                        (audio_read_pos + 1) %
                        (AUDIO_BUFFER_SAMPLES * AUDIO_CHANNELS);
                } else {
                    local_buffer[i * AUDIO_CHANNELS + ch] = 0.0f;
                }
            }
        }

        while (frames_written < frames_per_chunk && audio_running) {
            snd_pcm_sframes_t r =
                snd_pcm_writei(
                    pcm_handle,
                    local_buffer + frames_written * AUDIO_CHANNELS,
                    frames_per_chunk - frames_written);

            if (r == -EPIPE) {
                /* Underrun */
                snd_pcm_prepare(pcm_handle);
                snd_pcm_start(pcm_handle);
            } else if (r < 0) {
                snd_pcm_prepare(pcm_handle);
                snd_pcm_start(pcm_handle);
            } else {
                frames_written += r;
            }
        }
    }

    return NULL;
}

bool platform_audio_init(int sample_rate)
{
    g_audio_sample_rate = sample_rate;

    if (snd_pcm_open(&pcm_handle, "default",
                     SND_PCM_STREAM_PLAYBACK, 0) < 0)
        return false;

    snd_pcm_hw_params_t* hw;
    snd_pcm_hw_params_alloca(&hw);

    snd_pcm_hw_params_any(pcm_handle, hw);
    snd_pcm_hw_params_set_access(
        pcm_handle, hw, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(
        pcm_handle, hw, SND_PCM_FORMAT_FLOAT_LE);
    snd_pcm_hw_params_set_channels(
        pcm_handle, hw, AUDIO_CHANNELS);
    snd_pcm_hw_params_set_rate(
        pcm_handle, hw, sample_rate, 0);

    /* ~5ms buffer */
    snd_pcm_uframes_t period_size = 256;
    snd_pcm_hw_params_set_period_size_near(pcm_handle, hw, &period_size, NULL);

    snd_pcm_uframes_t buffer_size = period_size * 4;
    snd_pcm_hw_params_set_buffer_size_near(pcm_handle, hw, &buffer_size);

    if (snd_pcm_hw_params(pcm_handle, hw) < 0)
        return false;

    snd_pcm_prepare(pcm_handle);
    snd_pcm_start(pcm_handle);

    audio_buffer = malloc(sizeof(float) *
                          AUDIO_BUFFER_SAMPLES *
                          AUDIO_CHANNELS);

    audio_read_pos = 0;
    audio_write_pos = 0;
    audio_running = true;

    if (pthread_create(&audio_thread, NULL,
                       platform_audio_thread, NULL) != 0)
        return false;

    return true;
}

void platform_audio_shutdown(void)
{
    audio_running = false;

    if (audio_thread)
        pthread_join(audio_thread, NULL);

    if (pcm_handle) {
        snd_pcm_drain(pcm_handle);
        snd_pcm_close(pcm_handle);
    }

    if (audio_buffer)
        free(audio_buffer);
}

void platform_audio_push(float left, float right)
{
    uint32_t next =
        (audio_write_pos + 2) %
        (AUDIO_BUFFER_SAMPLES * AUDIO_CHANNELS);

    if (next == audio_read_pos)
        return;

    audio_buffer[audio_write_pos] = left;
    audio_buffer[audio_write_pos + 1] = right;
    audio_write_pos = next;
}

#endif // __linux__