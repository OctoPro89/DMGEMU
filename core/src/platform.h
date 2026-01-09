#pragma once

#include <stdint.h>

#ifndef __cplusplus
    #ifndef bool
        #include <stdbool.h>
    #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

    // ----------------------------------------
    // Types & Handles
    // ----------------------------------------

    typedef struct platform_window platform_window;

    // ----------------------------------------
    // Key & Mouse Enums
    // ----------------------------------------

    typedef enum {
        PLATFORM_KEY_UNKNOWN = 0,
        PLATFORM_KEY_A,
        PLATFORM_KEY_B,
        PLATFORM_KEY_C,
        PLATFORM_KEY_D,
        PLATFORM_KEY_E,
        PLATFORM_KEY_F,
        PLATFORM_KEY_G,
        PLATFORM_KEY_H,
        PLATFORM_KEY_I,
        PLATFORM_KEY_J,
        PLATFORM_KEY_K,
        PLATFORM_KEY_L,
        PLATFORM_KEY_M,
        PLATFORM_KEY_N,
        PLATFORM_KEY_O,
        PLATFORM_KEY_P,
        PLATFORM_KEY_Q,
        PLATFORM_KEY_R,
        PLATFORM_KEY_S,
        PLATFORM_KEY_T,
        PLATFORM_KEY_U,
        PLATFORM_KEY_V,
        PLATFORM_KEY_W,
        PLATFORM_KEY_X,
        PLATFORM_KEY_Y,
        PLATFORM_KEY_Z,
        PLATFORM_KEY_0,
        PLATFORM_KEY_1,
        PLATFORM_KEY_2,
        PLATFORM_KEY_3,
        PLATFORM_KEY_4,
        PLATFORM_KEY_5,
        PLATFORM_KEY_6,
        PLATFORM_KEY_7,
        PLATFORM_KEY_8,
        PLATFORM_KEY_9,
        PLATFORM_KEY_ESCAPE,
        PLATFORM_KEY_SPACE,
        PLATFORM_KEY_ENTER,
        PLATFORM_KEY_TAB,
        PLATFORM_KEY_BACKSPACE,
        PLATFORM_KEY_LEFT,
        PLATFORM_KEY_RIGHT,
        PLATFORM_KEY_UP,
        PLATFORM_KEY_DOWN,
        PLATFORM_KEY_MAX
    } platform_key;

    typedef enum {
        PLATFORM_MOUSE_BUTTON_LEFT,
        PLATFORM_MOUSE_BUTTON_RIGHT,
        PLATFORM_MOUSE_BUTTON_MIDDLE,
        PLATFORM_MOUSE_BUTTON_MAX
    } platform_mouse_button;

    // ----------------------------------------
    // Callbacks
    // ----------------------------------------

    typedef void (*platform_key_callback)(platform_window*, platform_key key, bool down);
    typedef void (*platform_mouse_button_callback)(platform_window*, platform_mouse_button button, bool down);
    typedef void (*platform_mouse_move_callback)(platform_window*, int x, int y);
    typedef void (*platform_resize_callback)(platform_window*, int width, int height);

    // ----------------------------------------
    // Initialization / Shutdown
    // ----------------------------------------

    bool platform_init();
    void platform_shutdown();

    // ----------------------------------------
    // Window Management
    // ----------------------------------------

    platform_window* platform_create_window(const char* title, int width, int height, bool resizable, int framebuffer_width, int framebuffer_height);
    void platform_destroy_window(platform_window* window);
    void platform_poll_events();
    bool platform_window_should_close(platform_window* window);

    // Optional: set callbacks per window
    void platform_set_key_callback(platform_window* window, platform_key_callback cb);
    void platform_set_mouse_button_callback(platform_window* window, platform_mouse_button_callback cb);
    void platform_set_mouse_move_callback(platform_window* window, platform_mouse_move_callback cb);
    void platform_set_resize_callback(platform_window* window, platform_resize_callback cb);

    // ----------------------------------------
    // Rendering
    // ----------------------------------------
    
    // Sets vsync on all windows
    void platform_set_vsync(bool enabled);

    // Set a single pixel in the window's framebuffer
    void platform_put_pixel(platform_window* window, int x, int y, uint8_t r, uint8_t g, uint8_t b);

    // Upload framebuffer to OpenGL texture
    void platform_blit(platform_window* window);

    // Swap the OpenGL buffers
    void platform_swap_buffers(platform_window* window);

    // Blit and swap buffers
    void platform_render(platform_window* window);

    // ----------------------------------------
    // Input Polling
    // ----------------------------------------

    bool platform_key_down(platform_key key);
    bool platform_mouse_button_down(platform_mouse_button button);
    void platform_get_mouse_pos(platform_window* window, int* x, int* y);
    void platform_get_mouse_delta(platform_window* window, int* dx, int* dy);

    // ----------------------------------------
    // Audio
    // ----------------------------------------

    bool platform_audio_init(int sample_rate);
    void platform_audio_shutdown(void);

    // Push one stereo sample (floating-point -1.0 .. 1.0)
    void platform_audio_push(float left, float right);

#ifdef __cplusplus
}
#endif
