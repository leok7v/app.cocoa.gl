#pragma once
#include "std.h"
#include "keyboard.h"

BEGIN_C

enum {
    /* window_state_t.style */
    WINDOW_STYLE_FULLSCREEN = 1, /* app can be in one of three states or */
    WINDOW_STYLE_MINIMIZED  = 2, /* app can be MINIMIZED|FULLSCREEN or MINIMIZED|NORMAL but not both */
    WINDOW_STYLE_NORMAL     = 4,
    WINDOW_STYLE_HIDDEN     = 8, /* any of the above states can be combined with hidden */
    
    /* mouse/touch input_event_t.kind */
    INPUT_MOUSE_MOVE         = 1,
    INPUT_MOUSE_DRAG         = 2,
    INPUT_MOUSE_DOWN         = 3,
    INPUT_MOUSE_UP           = 4, /* this is mouse button up and also `mouse click` */
    INPUT_MOUSE_DOUBLE_CLICK = 5, /* instead(!) of INPUT_MOUSE_UP */
    INPUT_MOUSE_LONG_PRESS   = 6, /* for most UI conventions same as INPUT_MOUSE_RIGHT_BUTTON INPUT_MOUSE_UP */
    INPUT_TOUCH_PROXIMITY    = 7, /* see event.z which is 0|1 for now */
    
    /* keyboard input_event_t.kind */
    INPUT_KEYBOARD           = 10, /* indicates keyboard input, last known mouse coordinates reported too */
    
    /* input_event_t.button indecies: */
    INPUT_MOUSE_LEFT_BUTTON  = 10, /* may be swapped for left handed mouse usage but still called 'left' */
    INPUT_MOUSE_OTHER_BUTTON = 11,
    INPUT_MOUSE_RIGHT_BUTTON = 12,
    INPUT_MOUSE_SCROLL_WHEEL = 13, /* event.y contains delta scroll */
    
    /* input_event_t.flags: */
    INPUT_REPEAT   = (1 << 19),
    INPUT_KEYDOWN  = (1 << 20),
    INPUT_KEYUP    = (1 << 21),
    INPUT_SHIFT    = (1 << 22), /* shift key is down */
    INPUT_CONTROL  = (1 << 23),
    INPUT_OPTION   = (1 << 24),
    INPUT_ALT      = (1 << 25),
    INPUT_COMMAND  = (1 << 26),
    INPUT_FN       = (1 << 27),
    INPUT_CAPS     = (1 << 28),
    INPUT_NUM      = (1 << 29),
    INPUT_HELP     = (1 << 30)
};

typedef struct input_event_s {
    int kind;
    int flags;
    float x; // mouse or primary touch coordinates
    float y;
    float z;
    int ch;
    int key;
    int button;
} input_event_t;

typedef struct app_s {
    void (*init)(int argc, const char* argv[]);
    void (*shape)(int x, int y, int width, int height); /* on OSX OpenGL does not have context here */
    void (*paint)(int x, int y, int w, int h);
    void (*input)(input_event_t* e);
    void (*timer)();
    void (*prefs)();  /* Settings|Preferences invoked by user or system */
    int  (*exits)();  /* "exit status" aka process exit code */
    double time;      /* time in seconds since application start */
    int timer_frequency; /* Hz number of timer() callbacks per second or zero */
} app_t;

typedef struct startup_s {
    void (*quit)(); /* application may call quit() to request exiting application dispatch loop. quits() will be called */
    void (*later)(double seconds, void* that, void* message, void (*callback)(void* _that, void* _message)); /* post a message */
    void (*redraw)(int x, int y, int w, int h); /* application may call redraw() to invalidate portion of the screen */
    void* (*map_resource)(const char* resource_name, int* size); // returns address and size of resource (naming system specific)
    void  (*unmap_resource)(void* a, int size); // unmaps the resource
    void  (*update)(); // update window_state, app.time and app.timer_frequency (also updated periodically)
} startup_t;

typedef struct window_state_s {
    int style;
    int x;     /* top left corner x in pixels in absolute monitors space coordinates */
    int y;     /* top left corner y in pixels in absolute monitors space coordinates */
    int w;     /* current width  of the window in pixels */
    int h;     /* current height of the window in pixels */
    int max_w; /* min/max in normal state */
    int max_h;
    int min_w; /* if min|max width|height are equal window is not resizable */
    int min_h;
} window_state_t;

extern app_t     app;
extern startup_t startup;
extern window_state_t window_state;

/* 
   Application can request its window to be moved or reshaped by changing its x/y and or width/height
   the window will be reshaped and moved in the next possible event dispatch looop.

   All application callbacks are called on event dispatch thread.

   input() will be called with decoded UNICODE or ASCII ch != 0, undecoded UNICODE-32 key, and
   combination of flags INPUT_SHIFT ... flags
 
   input() will be called mouse or touch-device x, y absolute screen coordinates and
   possible z proximity or pressure for touch sensors in hover over mode.
 
   input() will be called mouse buttons up and down with .button indicating the button 
   pressed see: INPUT_MOUSE_LEFT_BUTTON ...
   for touch device event.button will contain finger number touched down.
 
   timer() is called periodically if frequency > 0. timer with resolution > 1000Hz are not guaranteed by modern OSes.

   shape() is called when window is moved or resized by user, window manager (e.g. minimize all 
   or snap to top and bottom Windows) or by application request. Durring the shape() call the
   application state still has previous x, y, width, height.
   shape() is also called if window visibility changes (e.g. hidden)
 
   prefs() is called when user or system invokes Preferences or Settings action for the application.
   it is up to application to maintain 'prefs'-state till user exits it and paint visuals accordingly.
 
   if any of the aforementioned functions are null - they are not called at all.

   redraw() may be called by application to invalidate particular region in application coordinates.
 */

END_C
