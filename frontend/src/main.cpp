#include <XGUI/xgui.h>
#include <../src/Window/Win32Window.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

extern "C" {
    #include <cart.h>
    #include <bus.h>
    #include <lcd.h>
    #include <gamepad.h>
}

char state_char = 0;
bool shift = false;
bool enter = false;
bool control = false;
bool left = false;
bool right = false;
bool backspace = false;
bool del = false;

const bool* m_keys = NULL;

u32 gl_texture = 0;
u8* software_framebuffer;

const char* open_file_dialog(const char* filter) {
    static char filepath[MAX_PATH] = { 0 };  // static so we can return pointer

    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;  // or pass your window handle here
    ofn.lpstrFile = filepath;
    ofn.nMaxFile = sizeof(filepath);

    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    *filepath = '\0'; // reset

    if (GetOpenFileNameA(&ofn)) {
        return filepath;  // success
    }
    else {
        return NULL; // user cancelled or error
    }
}

wchar_t TranslateKeyToChar(UINT vkCode)
{
    BYTE keyboardState[256];
    GetKeyboardState(keyboardState); // get current state (shift, caps, etc.)

    keyboardState[VK_CONTROL] = false;
    keyboardState[VK_LCONTROL] = false;
    keyboardState[VK_RCONTROL] = false;

    wchar_t buff[5] = { 0 };
    int ret = ToUnicode(vkCode, MapVirtualKey(vkCode, MAPVK_VK_TO_VSC), keyboardState, buff, 4, 0);
    if (ret > 0)
        return buff[0];
    return 0;
}

void cb(int vkCode, bool isPressed)
{
    bool ctrl = m_keys[VK_CONTROL] || m_keys[VK_LCONTROL] || m_keys[VK_RCONTROL];
    bool shft = m_keys[VK_SHIFT] || m_keys[VK_LSHIFT] || m_keys[VK_RSHIFT];

    control = ctrl;
    shft = shft;
    enter = m_keys[VK_RETURN];
    left = m_keys[VK_LEFT];
    right = m_keys[VK_RIGHT];
    del = m_keys[VK_DELETE];
    backspace = m_keys[VK_BACK];

    // Ignore non-printable or control keys for text input
    if (!isPressed) return;

    wchar_t ch = TranslateKeyToChar(vkCode);
    if (ch != 0)
    {
        char c = (char)ch;
        state_char = c;
    }
}

static void render() {
    for (int y = 0; y < PPU_YRES; ++y) {
        for (int x = 0; x < PPU_XRES; ++x) {
            u64 index = (y * PPU_XRES + x) * 3;
            software_framebuffer[index] =  ppu_global->video_buffer[y * PPU_XRES + x];
            software_framebuffer[index + 1] = ppu_global->video_buffer[y * PPU_XRES + x];
            software_framebuffer[index + 2] = ppu_global->video_buffer[y * PPU_XRES + x];
        }
    }
}

static void create_texture() {
    glGenTextures(1, &gl_texture);
    glBindTexture(GL_TEXTURE_2D, gl_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, PPU_XRES, PPU_YRES, 0, GL_RGB, GL_UNSIGNED_BYTE, software_framebuffer);
}

static void update_texture() {
    glBindTexture(GL_TEXTURE_2D, gl_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, PPU_XRES, PPU_YRES, GL_RGB, GL_UNSIGNED_BYTE, software_framebuffer);
}

static void destroy_texture() {
    glDeleteTextures(1, &gl_texture);
}

static void input() {
    if (m_keys[VK_RETURN]) gamepad_global.controller.start = true;
    if (m_keys[VK_TAB]) gamepad_global.controller.select = true;
    if (m_keys['Z']) gamepad_global.controller.a = true;
    if (m_keys['X']) gamepad_global.controller.b = true;

    if (!m_keys[VK_RETURN]) gamepad_global.controller.start = false;
    if (!m_keys[VK_TAB]) gamepad_global.controller.select = false;
    if (!m_keys['Z']) gamepad_global.controller.a = false;
    if (!m_keys['X']) gamepad_global.controller.b = false;

    if (m_keys[VK_LEFT]) gamepad_global.controller.left = true;
    if (m_keys[VK_RIGHT]) gamepad_global.controller.right = true;
    if (m_keys[VK_UP]) gamepad_global.controller.up = true;
    if (m_keys[VK_DOWN]) gamepad_global.controller.down = true;

    if (!m_keys[VK_LEFT]) gamepad_global.controller.left = false;
    if (!m_keys[VK_RIGHT]) gamepad_global.controller.right = false;
    if (!m_keys[VK_UP]) gamepad_global.controller.up = false;
    if (!m_keys[VK_DOWN]) gamepad_global.controller.down = false;
}

static void load_rom(const char* fp) {
    if (fp != NULL) {
        lcd_unload();
        bus_unload();
        timer_unload();
        ppu_unload();
        cpu_unload();
        cart_unload();

        cart_open(fp);
        cpu_init(cart_global.rom, cart_global.rom_size);
        ppu_init();
        timer_init();
        bus_init();
    }
}

static void save_ram(const char* fp) { if (fp != NULL) cart_battery_save(fp); }
static void load_ram(const char* fp) { if (fp != NULL) cart_battery_load(fp);  }

int main(int argc, char* argv[]) {
    cart_open(argc == 2 ? argv[1] : "C:/users/vince/downloads/tetris.gb");
    cpu_init(cart_global.rom, cart_global.rom_size);
    ppu_init();
    timer_init();
    bus_init();

    software_framebuffer = (u8*)malloc(sizeof(u8) * PPU_XRES * PPU_YRES * 3);

    Win32Window window = Win32Window("DMGEMU Frontend", 800, 600);
    window.Show();
    window.SetKeyCallback(cb);
    if (!window.SetupGLContext())
    {
        MessageBoxA(NULL, "Failed to initialize OpenGL", "", MB_OK);
        return -1;
    }

    xgui::init();

    auto& ctx = xgui::Context::get();

    xgui::Style style{};
    style.corner_radius = 10.0f;

    ctx.style = style;

    u64 prev_frame = 0;

    create_texture();

    ctx.dock_points.push_back({
        0.5f, 0.5f,
        1.0f, 1.0f,
        "Center Dock"
    });

    while (!window.ShouldClose())
    {
        m_keys = window.GetKeyboardState();

        window.PollEvents();
        
        input();
        while (prev_frame == ppu_global->current_frame) {
            bus_step();
        }
        prev_frame = ppu_global->current_frame;
        render();

        // Process input
        xgui::InputState input{};
        input.mouse_down[0] = window.IsMouseButtonDown(0);
        input.mouse_down[1] = window.IsMouseButtonDown(1);
        input.mouse_down[2] = window.IsMouseButtonDown(2);

        int x = 0, y = 0;
        window.GetMousePosition(x, y);
        input.mouse_pos.x = (float)x;
        input.mouse_pos.y = (float)y;

        input.input_char = control ? 0 : state_char;
        input.key_ctrl = control;
        input.key_enter = enter;
        input.key_shift = shift;
        input.key_left = left;
        input.key_right = right;
        input.key_c = state_char == 'c';
        input.key_x = state_char == 'x';
        input.key_v = state_char == 'v';
        input.key_a = state_char == 'a';
        input.key_delete = del;
        input.key_backspace = backspace;

        // Begin frame
        xgui::beginFrame(input);

        static bool fileMenuOpen = false;
        f32 file_x_off = 0.0f;
        static bool gameMenuOpen = false;
        f32 game_x_off = 5.0f;
        static bool infoMenuOpen = false;
        f32 info_x_off = 10.0f;

        xgui::menuBar(20.0f);
        if (xgui::menuBarItem("File", &file_x_off)) {
            infoMenuOpen = false;
            gameMenuOpen = false;
            fileMenuOpen = !fileMenuOpen;
        }

        if (xgui::menuBarItem("Game", &game_x_off)) {
            infoMenuOpen = false;
            fileMenuOpen = false;
            gameMenuOpen = !gameMenuOpen;
        }

        if (xgui::menuBarItem("Info", &info_x_off)) {
            gameMenuOpen = false;
            fileMenuOpen = false;
            infoMenuOpen = !infoMenuOpen;
        }

        if (fileMenuOpen) {
            static xgui::MenuItem menuitems[] = {
                { "Load ROM" },
                { "Exit" }
            };

            int selected = xgui::menu(menuitems, _countof(menuitems), file_x_off, ctx.current_menu_bar.height);
            if (selected != -1) {
                int parent = XGUI_MENUBAR_PARENT(selected);

                if (parent == 0) load_rom(open_file_dialog("DMG Game Boy ROMs (*.gb;)\0*.gb;\0All Files\0*.*\0"));
                if (parent == 1) window.NotifyClose();

                fileMenuOpen = false;
            }
        }

        if (gameMenuOpen) {
            static xgui::MenuItem menuitems[] = {
                { "Save Game" },
                { "Load Game" }
            };

            int selected = xgui::menu(menuitems, _countof(menuitems), game_x_off, ctx.current_menu_bar.height);
            if (selected != -1) {
                int parent = XGUI_MENUBAR_PARENT(selected);

                if (parent == 0) save_ram(open_file_dialog("DMGEMU Save Files (*.save;)\0*.save;\0All Files\0*.*\0"));
                if (parent == 1) load_ram(open_file_dialog("DMGEMU Save Files (*.save;)\0*.save;\0All Files\0*.*\0"));

                gameMenuOpen = false;
            }
        }

        xgui::WindowedUILayout wlayout{};

        if (infoMenuOpen)
        {
            xgui::beginWindow("Info", 300.0f, 300.0f, 400.0f, 400.0f);

            xgui::bringCurrentWindowToFront();

            wlayout.UIbegin(0.8f, 20.0f);
            wlayout.UItext("[Emulator Core]:", 5.0f);
            wlayout.UItext("DMGEMU ver 1.0 ", 5.0f);
            wlayout.UItext("[UI rendering library]:", 5.0f);
            wlayout.UItext("XGUI ver 1.0", 5.0f);
            wlayout.UItext("[Rendering API]:", 5.0f);
            wlayout.UItext("OpenGL 4.5 Desktop", 5.0f);
            wlayout.UItext("Software by OctoDev89", 5.0f);
            if (wlayout.UIbutton("Close", 20.0f)) {
                infoMenuOpen = false;
            }
            wlayout.UIend();

            xgui::endWindow();
        }

        xgui::beginWindow("Emulator", 350.0f, 350.0f, (f32)PPU_XRES * 3.0f + 50.0f, (f32)PPU_YRES * 3.0f + 50.0f);

        wlayout.UIbegin(0.8f, 20.0f);
        const f32 image_ratio = (f32)PPU_XRES / (f32)PPU_YRES;
        f32 window_ratio = (f32)ctx.current_window->rect.w / (f32)ctx.current_window->rect.h;

        f32 new_width, new_height;

        if (window_ratio > image_ratio) {
            new_height = ctx.current_window->rect.h;
            new_width = new_height * image_ratio;
        }
        else {
            new_width = ctx.current_window->rect.w;
            new_height = new_width / image_ratio;
        }

        xgui::imageView(gl_texture, ctx.current_window->rect.x + (ctx.current_window->rect.w * 0.5f), ctx.current_window->rect.y + ((ctx.current_window->rect.h + 20.0f) * 0.5f), new_width - 25.0f, new_height - 25.0f);

        wlayout.UIend();

        xgui::endWindow();

        // End frame
        xgui::endFrame();

        state_char = 0;
        shift = false;
        enter = false;
        control = false;
        left = false;
        right = false;
        backspace = false;

        if (ctx.active_id == 0 && ctx.input.mouse_down[0]) fileMenuOpen = false;

        update_texture();
        window.SwapDC();
    }

    lcd_unload();
    bus_unload();
    timer_unload();
    ppu_unload();
    cpu_unload();
    cart_unload();
    destroy_texture();
    free(software_framebuffer);
    xgui::shutdown();

    _CrtDumpMemoryLeaks();
    return 0;
}