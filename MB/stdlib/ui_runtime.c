#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

typedef enum { UI_RECT, UI_TEXT } UiCommandKind;
typedef struct {
    UiCommandKind kind;
    int32_t x, y, width, height;
    uint32_t color;
    char *text;
} UiCommand;

static UiCommand *commands;
static size_t command_count, command_capacity;

static void clear_commands(void) {
    for (size_t i = 0; i < command_count; ++i) free(commands[i].text);
    free(commands);
    commands = NULL;
    command_count = command_capacity = 0;
}

static int enqueue(UiCommand command) {
    if (command_count == command_capacity) {
        size_t next = command_capacity ? command_capacity * 2 : 32;
        UiCommand *grown = (UiCommand *)realloc(commands, next * sizeof(*grown));
        if (!grown) return 0;
        commands = grown;
        command_capacity = next;
    }
    commands[command_count++] = command;
    return 1;
}

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static HWND window_handle;
static const wchar_t window_class_name[] = L"MobdriveNativeWindow";

static wchar_t *to_wide(const char *utf8) {
    if (!utf8) utf8 = "";
    int count = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (count <= 0) return NULL;
    wchar_t *wide = (wchar_t *)malloc((size_t)count * sizeof(*wide));
    if (wide) MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide, count);
    return wide;
}

static void paint_commands(HDC dc) {
    RECT client;
    GetClientRect(window_handle, &client);
    FillRect(dc, &client, (HBRUSH)(COLOR_WINDOW + 1));
    SetBkMode(dc, TRANSPARENT);
    for (size_t i = 0; i < command_count; ++i) {
        UiCommand *cmd = &commands[i];
        if (cmd->kind == UI_RECT) {
            COLORREF color = RGB((cmd->color >> 16) & 255,
                                 (cmd->color >> 8) & 255,
                                 cmd->color & 255);
            HBRUSH brush = CreateSolidBrush(color);
            RECT rect = {cmd->x, cmd->y, cmd->x + cmd->width, cmd->y + cmd->height};
            if (brush) {
                FillRect(dc, &rect, brush);
                DeleteObject(brush);
            }
        } else {
            wchar_t *wide = to_wide(cmd->text);
            if (wide) {
                SetTextColor(dc, RGB((cmd->color >> 16) & 255,
                                     (cmd->color >> 8) & 255,
                                     cmd->color & 255));
                TextOutW(dc, cmd->x, cmd->y, wide, (int)wcslen(wide));
                free(wide);
            }
        }
    }
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    (void)wparam;
    (void)lparam;
    if (message == WM_PAINT) {
        PAINTSTRUCT paint;
        HDC dc = BeginPaint(hwnd, &paint);
        window_handle = hwnd;
        paint_commands(dc);
        EndPaint(hwnd, &paint);
        return 0;
    }
    if (message == WM_ERASEBKGND) return 1;
    if (message == WM_DESTROY) {
        window_handle = NULL;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wparam, lparam);
}

bool ui_window_open(const char *title, int32_t width, int32_t height) {
    if (window_handle) return 1;
    HINSTANCE instance = GetModuleHandleW(NULL);
    WNDCLASSW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = window_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(32512));
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = window_class_name;
    if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return 0;

    RECT bounds = {0, 0, width > 0 ? width : 800, height > 0 ? height : 600};
    AdjustWindowRect(&bounds, WS_OVERLAPPEDWINDOW, FALSE);
    wchar_t *wide_title = to_wide(title);
    if (!wide_title) return 0;
    HWND hwnd = CreateWindowExW(0, window_class_name, wide_title, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, bounds.right - bounds.left, bounds.bottom - bounds.top,
        NULL, NULL, instance, NULL);
    free(wide_title);
    if (!hwnd) return 0;
    window_handle = hwnd;
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    return 1;
}

void ui_event_loop(void) {
    MSG message;
    while (GetMessageW(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    clear_commands();
}

void ui_window_close(void) {
    if (window_handle) DestroyWindow(window_handle);
}

#elif defined(__linux__)
#include <X11/Xlib.h>
#include <X11/Xutil.h>

static Display *display;
static Window window_handle;
static GC graphics;
static Atom wm_delete_window;
static XFontSet font_set;

static unsigned long x11_color(uint32_t color) {
    return (unsigned long)(color & 0x00ffffffu);
}

static void paint_commands(void) {
    if (!display || !window_handle || !graphics) return;
    XSetForeground(display, graphics, WhitePixel(display, DefaultScreen(display)));
    XClearWindow(display, window_handle);
    for (size_t i = 0; i < command_count; ++i) {
        UiCommand *cmd = &commands[i];
        XSetForeground(display, graphics, x11_color(cmd->color));
        if (cmd->kind == UI_RECT) {
            if (cmd->width > 0 && cmd->height > 0)
                XFillRectangle(display, window_handle, graphics, cmd->x, cmd->y,
                               (unsigned int)cmd->width, (unsigned int)cmd->height);
        } else if (cmd->text) {
            if (font_set)
                Xutf8DrawString(display, window_handle, font_set, graphics,
                                cmd->x, cmd->y, cmd->text, (int)strlen(cmd->text));
            else
                XDrawString(display, window_handle, graphics, cmd->x, cmd->y,
                            cmd->text, (int)strlen(cmd->text));
        }
    }
    XFlush(display);
}

bool ui_window_open(const char *title, int32_t width, int32_t height) {
    if (display && window_handle) return true;
    display = XOpenDisplay(NULL);
    if (!display) {
        fputs("Mobdrive UI: cannot open X11 display (check DISPLAY and X server).\n", stderr);
        return false;
    }
    int screen = DefaultScreen(display);
    window_handle = XCreateSimpleWindow(display, RootWindow(display, screen),
        0, 0, (unsigned int)(width > 0 ? width : 800),
        (unsigned int)(height > 0 ? height : 600), 1,
        BlackPixel(display, screen), WhitePixel(display, screen));
    if (!window_handle) {
        XCloseDisplay(display);
        display = NULL;
        return false;
    }
    XStoreName(display, window_handle, title ? title : "Mobdrive");
    XSelectInput(display, window_handle, ExposureMask | StructureNotifyMask);
    wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window_handle, &wm_delete_window, 1);
    graphics = XCreateGC(display, window_handle, 0, NULL);
    char **missing = NULL;
    int missing_count = 0;
    char *default_string = NULL;
    font_set = XCreateFontSet(display, "-*-*-medium-r-normal--14-*-*-*-*-*-*-*",
                              &missing, &missing_count, &default_string);
    if (missing) XFreeStringList(missing);
    XMapWindow(display, window_handle);
    XFlush(display);
    return true;
}

void ui_event_loop(void) {
    if (!display || !window_handle) return;
    bool running = true;
    while (running) {
        XEvent event;
        XNextEvent(display, &event);
        if (event.type == Expose && event.xexpose.count == 0) paint_commands();
        else if (event.type == ClientMessage &&
                 (Atom)event.xclient.data.l[0] == wm_delete_window) running = false;
        else if (event.type == DestroyNotify) running = false;
    }
    if (font_set) XFreeFontSet(display, font_set);
    if (graphics) XFreeGC(display, graphics);
    if (window_handle) XDestroyWindow(display, window_handle);
    XCloseDisplay(display);
    display = NULL;
    window_handle = 0;
    graphics = NULL;
    font_set = NULL;
    clear_commands();
}

void ui_window_close(void) {
    if (display && window_handle) XDestroyWindow(display, window_handle);
    if (display) XFlush(display);
}

#else
bool ui_window_open(const char *title, int32_t width, int32_t height) {
    (void)title; (void)width; (void)height;
    fputs("Mobdrive UI: this platform has no native window backend.\n", stderr);
    return 0;
}
void ui_event_loop(void) {}
void ui_window_close(void) {}
#endif

void draw_rect(int32_t x, int32_t y, int32_t width, int32_t height, uint32_t color) {
    (void)enqueue((UiCommand){UI_RECT, x, y, width, height, color, NULL});
#ifdef _WIN32
    if (window_handle) InvalidateRect(window_handle, NULL, FALSE);
#elif defined(__linux__)
    if (display && window_handle) XClearArea(display, window_handle, 0, 0, 0, 0, True);
#endif
}

void draw_text_color(int32_t x, int32_t y, const char *text, uint32_t color) {
    char *copy = NULL;
    if (text) {
        size_t length = strlen(text) + 1;
        copy = (char *)malloc(length);
        if (!copy) return;
        memcpy(copy, text, length);
    }
    if (!enqueue((UiCommand){UI_TEXT, x, y, 0, 0, color, copy})) free(copy);
#ifdef _WIN32
    if (window_handle) InvalidateRect(window_handle, NULL, FALSE);
#elif defined(__linux__)
    if (display && window_handle) XClearArea(display, window_handle, 0, 0, 0, 0, True);
#endif
}

void draw_text(int32_t x, int32_t y, const char *text) {
    draw_text_color(x, y, text, 0x000000);
}

void ui_draw_rect(int32_t x, int32_t y, int32_t width, int32_t height, uint32_t color) {
    draw_rect(x, y, width, height, color);
}

void ui_draw_text(int32_t x, int32_t y, const char *text) {
    draw_text(x, y, text);
}
