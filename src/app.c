#include "app.h"
#include "vc3d.h"
#include <stdio.h>

BEGIN_C

static app_t app;
static vc3d_t vc;

static void shape(int x, int y, int w, int h) {
    printf("shape(%d,%d %dx%d)\n", x, y, w, h);
    vc3d_shape(vc, x, y, w, h);
}

static void paint(int x, int y, int w, int h) {
//  printf("paint(%d,%d %dx%d)\n", x, y, w, h);
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
        if (e->ch == 'f' || e->ch == 'F') {
            app.window_state ^= WINDOW_STATE_FULLSCREEN;
        }
        if (e->ch == 'q' || e->ch == 'Q') {
            app.stop();
        }
        if (e->ch == 'h' || e->ch == 'H') {
            app.window_state ^= WINDOW_STATE_HIDDEN;
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
            if (e->key == KEY_LEFT_ARROW)  { app.window_w--; }
            if (e->key == KEY_UP_ARROW)    { app.window_h--; }
            if (e->key == KEY_RIGHT_ARROW) { app.window_w++; }
            if (e->key == KEY_DOWN_ARROW)  { app.window_h++; }
        } else {
            /* move window */
            if (e->key == KEY_LEFT_ARROW)  { app.window_x--; }
            if (e->key == KEY_UP_ARROW)    { app.window_y++; }
            if (e->key == KEY_RIGHT_ARROW) { app.window_x++; }
            if (e->key == KEY_DOWN_ARROW)  { app.window_y--; }
        }
    }
}

static void timer() {
//  printf("timer %.1f\n", app.time);
    app.redraw(0, 0, app.window_w, app.window_h);
}

static void prefs() {
    printf("preferences\n");
}

static int quits() {
    printf("about to quit\n");
    vc3d_destroy(vc);
    return 0;
}

app_t* run(int argc, const char* argv[]) {
    app.input = input;
    app.shape = shape;
    app.paint = paint;
    app.timer = timer;
    app.prefs = prefs;
    app.quits = quits;
    app.window_min_w = 640;
    app.window_min_h = 480;
    app.timer_frequency = 60;
    vc = vc3d_create(&app);
    return &app;
}

END_C
