#include "app.h"
#include "vc3d.h"
#include <stdio.h>

BEGIN_C

static vc3d_t vc;

static void shape(int x, int y, int w, int h) {
//  printf("shape(%d,%d %dx%d)\n", x, y, w, h);
    vc3d_shape(vc, x, y, w, h);
}

static void paint(int x, int y, int w, int h) {
    printf("[%06d] paint(%d,%d %dx%d)\n", gettid(), x, y, w, h);
    vc3d_paint(vc, x, y, w, h);
}

static void input(input_event_t* e) {
    vc3d_input(vc, e);
    if (e->kind == INPUT_KEYBOARD && (e->flags & INPUT_KEYDOWN) && e->ch != 0) {
        if (32 <= e->ch && e->ch <= 127) { // otherwise %c or %lc debug output in Xcode gets confused
            printf("ch=%d 0x%04X '%c' key=%d 0x%02X ", e->ch, e->ch, e->ch, e->key, e->key);
        } else {
            printf("ch=%d 0x%04X key=%d 0x%02X ", e->ch, e->ch, e->key, e->key);
        }
        if (e->ch == 'f' || e->ch == 'F' || e->key == KEY_ESCAPE) {
            window_state.style ^= WINDOW_STYLE_FULLSCREEN;
        }
        if (e->ch == 'q' || e->ch == 'Q') {
            startup.quit();
        }
        if (e->ch == 'h' || e->ch == 'H') {
            window_state.style ^= WINDOW_STYLE_HIDDEN;
        }
        if (e->key == KEY_LEFT_ARROW)  { printf("\u21E6 "); }
        if (e->key == KEY_UP_ARROW)    { printf("\u21E7 "); }
        if (e->key == KEY_RIGHT_ARROW) { printf("\u21E8 "); }
        if (e->key == KEY_DOWN_ARROW)  { printf("\u21E9 "); }
        if (e->flags & INPUT_SHIFT)    { printf("SHIFT "); }
        if (e->flags & INPUT_ALT)      { printf("ALT "); }
        if (e->flags & INPUT_NUM)      { printf("NUM "); }
        if (e->flags & INPUT_CAPS)     { printf("CAPS "); }
        if (e->flags & INPUT_COMMAND)  { printf("COMMAND "); }
        if (e->flags & INPUT_CONTROL)  { printf("CONTROL "); }
        if (e->flags & INPUT_OPTION)   { printf("OPTION "); } // ATL == OPTION on Mac OSX keyboards
        printf("\n");
        if (e->flags & INPUT_SHIFT) {
            /* resize window */
            if (e->key == KEY_LEFT_ARROW)  { window_state.w--; }
            if (e->key == KEY_UP_ARROW)    { window_state.h--; }
            if (e->key == KEY_RIGHT_ARROW) { window_state.w++; }
            if (e->key == KEY_DOWN_ARROW)  { window_state.h++; }
        } else {
            /* move window */
            if (e->key == KEY_LEFT_ARROW)  { window_state.x--; }
            if (e->key == KEY_UP_ARROW)    { window_state.y++; }
            if (e->key == KEY_RIGHT_ARROW) { window_state.x++; }
            if (e->key == KEY_DOWN_ARROW)  { window_state.y--; }
        }
    } else if (e->kind == INPUT_MOUSE_MOVE) {
        printf("INPUT_MOUSE_MOVE %.1f %.1f\n", e->x, e->y);
    } else if (e->kind == INPUT_MOUSE_DRAG) {
        printf("INPUT_MOUSE_DRAG %.1f %.1f\n", e->x, e->y);
    } else if (e->kind == INPUT_MOUSE_DOWN) {
        printf("INPUT_MOUSE_DOWN %.1f %.1f\n", e->x, e->y);
    } else if (e->kind == INPUT_MOUSE_UP) {
        printf("INPUT_MOUSE_UP %.1f %.1f\n", e->x, e->y);
    } else if (e->kind == INPUT_MOUSE_DOUBLE_CLICK) {
        printf("INPUT_MOUSE_DOUBLE_CLICK %.1f %.1f\n", e->x, e->y);
    } else if (e->kind == INPUT_MOUSE_LONG_PRESS) {
        printf("INPUT_MOUSE_LONG_PRESS %.1f %.1f\n", e->x, e->y);
    } else if (e->kind == INPUT_TOUCH_PROXIMITY) {
        printf("INPUT_TOUCH_PROXIMITY %.1f %.1f\n", e->x, e->y);
    }
}

static void timer() {
//  printf("[%06d] timer %.6f\n", gettid(), app.time);
//  app.redraw(0, 0, window_state.w, window_state.h);
}

static void later_callback(void* that, void* message) {
    printf("[%06d] later_callback %.6f %s\n", gettid(), app.time, message);
    startup.later(2.5, null, "in 2.5 seconds", later_callback);
}

static void prefs() {
    printf("preferences\n");
    startup.later(0.5, null, "in 0.5 seconds", later_callback);
}

static int exits() {
    printf("about to quit\n");
    vc3d_destroy(vc);
    return EXIT_SUCCESS; // exit status (because sysexits.h is no posix)
}

void init(int argc, const char* argv[]) {
    app.input = input;
    app.shape = shape;
    app.paint = paint;
    app.timer = timer;
    app.prefs = prefs;
    app.exits = exits;
    //  window_state.state = WINDOW_STATE_FULLSCREEN;
    window_state.min_w = 800;
    window_state.min_h = 600;
    app.timer_frequency = 60; // Hz
    vc = vc3d_create(&app);
}

app_t app = {
    init,
    shape,
    paint,
    input,
    timer,
    prefs,
    exits,
};


END_C
