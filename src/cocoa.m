#import  <Cocoa/Cocoa.h>
#import  <OpenGL/gl.h>
#include <objc/runtime.h> // https://stackoverflow.com/questions/11319170/c-as-principal-class-or-a-cocoa-app-without-objc
#include <objc/message.h> // https://stackoverflow.com/questions/10289890/how-to-write-ios-app-purely-in-c#comment13239523_10289913
#include <assert.h>
#include <pthread.h>
#include <mach/thread_info.h>
#include <mach/mach_time.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>

#include "app.h"

#undef min
#undef max
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

// static uint64_t gettid() { uint64_t tid; pthread_threadid_np(null, &tid); return tid; }

static int64_t nanotime() {
    static int64_t time0;
    static mach_timebase_info_data_t tb;
    uint64_t t = mach_absolute_time();
    if (tb.denom == 0) {
        mach_timebase_info(&tb);
    }
    t = t * tb.numer / tb.denom;
    if (time0 == 0) { time0 = t; }
    return t - time0;
}

typedef struct app_back_s {
    float mouse_x;
    float mouse_y;
    float mouse_z;
    int window_state;
    int window_x;
    int window_y;
    int window_w;
    int window_h;
    int timer_frequency;
} app_back_t;

static app_t* app;
static app_back_t back;

@interface Window : NSWindow {
@public
    dispatch_source_t timer;
    NSOpenGLPixelFormat* pixel_format;
    NSOpenGLContext* context;
}

- (void) setTimer: (int) frequency;

@end

static bool state_diff(int mask) {
    return (back.window_state & mask) != (app->window_state & mask);
}

static void toggle_fullscreen(NSWindow* window) {
    bool app_full_screen = (app->window_state & WINDOW_STATE_FULLSCREEN) != 0;
    bool win_full_screen = (window.styleMask & NSFullScreenWindowMask) != 0;
    if (app_full_screen != win_full_screen) {
        dispatch_async(dispatch_get_main_queue(), ^{
            bool full_screen = (app->window_state & WINDOW_STATE_FULLSCREEN) != 0;
            window.hidesOnDeactivate = full_screen;
            [window toggleFullScreen: null]; // must be on the next dispatch otherwise update_state will be called from inside itself
            if (full_screen) {
                NSApplication.sharedApplication.presentationOptions |= NSApplicationPresentationAutoHideMenuBar;
            }
            if (!full_screen) {
                const NSApplicationPresentationOptions clear = NSApplicationPresentationAutoHideMenuBar | NSApplicationPresentationHideMenuBar | NSApplicationPresentationFullScreen;
                NSApplication.sharedApplication.presentationOptions &= ~(clear);
            }
        });
    }
}

static void hide_application(NSWindow* window, bool hide) {
    dispatch_async(dispatch_get_main_queue(), ^{
        if (hide) { // OSX hiding individual windows confuses Dock.app
            [NSApplication.sharedApplication hide: window];
        } else {
            [NSApplication.sharedApplication unhide: window];
        }
    });
}

static Window* create_window() {
    bool full_screen = (app->window_state & WINDOW_STATE_FULLSCREEN) != 0;
    int w = max(app->window_w, app->window_min_w);
    int h = max(app->window_h, app->window_min_h);
    NSRect r = full_screen ? NSScreen.mainScreen.frame : NSMakeRect(0, 0, w > 0 ? w : 640, h > 0 ? h : 480);
    NSWindowStyleMask sm = NSWindowStyleMaskBorderless|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable|NSWindowStyleMaskResizable|NSFullSizeContentViewWindowMask|NSTitledWindowMask;
    Window* window = [Window.alloc initWithContentRect: r styleMask: sm backing: NSBackingStoreBuffered defer: full_screen];
    window.opaque = true;
    window.hidesOnDeactivate = full_screen;
    window.acceptsMouseMovedEvents = true;
//  window.contentView = null;
    window.collectionBehavior |= NSWindowCollectionBehaviorFullScreenPrimary;
    window.titlebarAppearsTransparent  = true;
    window.titleVisibility = NSWindowTitleHidden;
    window.movable = true;
    if (app->window_min_w > 0 && app->window_min_h > 0) {
        window.contentMinSize = NSMakeSize(app->window_min_w, app->window_min_h);
    }
    if (app->window_max_w > 0 && app->window_max_h > 0) {
        window.contentMaxSize = NSMakeSize(app->window_max_w, app->window_max_h);
    }
    // not sure next two lines are absolutely necessary but leave it as insurance against future Apple changes:
    window.contentView.translatesAutoresizingMaskIntoConstraints = false;
    window.contentView.autoresizingMask = NSViewWidthSizable|NSViewHeightSizable;
    app->window = (__bridge void *)(window);
    NSRect frame = window.frame;
    app->window_x = frame.origin.x;
    app->window_y = frame.origin.y;
    app->window_w = frame.size.width;
    app->window_h = frame.size.height;
    if (full_screen) { toggle_fullscreen(window); }
    return window;
}

static void update_state(NSWindow* window) {
    if (state_diff(WINDOW_STATE_FULLSCREEN)) {
        back.window_state = app->window_state; // in case update_state will be called recursively
        toggle_fullscreen(window);
    }
    if (state_diff(WINDOW_STATE_HIDDEN)) {
        back.window_state = app->window_state; // in case update_state will be called recursively
        hide_application(window, (app->window_state & WINDOW_STATE_HIDDEN) != 0);
    }
    back.window_state = app->window_state;
    if (app->window_x != back.window_x || app->window_y != back.window_y || app->window_w != back.window_w || app->window_h != back.window_h) {
        window.contentSize = NSMakeSize(app->window_w, app->window_h);
        [window setFrame: NSMakeRect(app->window_x, app->window_y, app->window_w, app->window_h) display: true animate: true];
        back.window_x = app->window_x;
        back.window_y = app->window_y;
        back.window_w = app->window_w;
        back.window_h = app->window_h;
    }
    if (app->timer_frequency != back.timer_frequency) {
        [(Window*)window setTimer: app->timer_frequency];
        back.timer_frequency = app->timer_frequency;
    }
    app->time = nanotime() / 1000000.0;
}

static void redraw(int x, int y, int w, int h) {
    if (app->window != null) {
        NSWindow* w = (__bridge NSWindow*)app->window;
        w.contentView.needsDisplay = true;
        w.viewsNeedDisplay = true;
    }
}

static void reshape(NSWindow* window) {
    NSRect frame = window.frame;
    // shape() still has previous window position and size in `app` when called
    app->shape(frame.origin.x, frame.origin.y, frame.size.width, frame.size.height);
    back.window_x = app->window_x = frame.origin.x;
    back.window_y = app->window_y = frame.origin.y;
    back.window_w = app->window_w = frame.size.width;
    back.window_h = app->window_h = frame.size.height;
}

static int translate_type_to_button_number(int buttonNumber) {
    int button = 0;
    switch (buttonNumber) {
        case NSLeftMouseUp:
        case NSLeftMouseDown:   button = INPUT_MOUSE_LEFT_BUTTON;  break;
        case NSRightMouseUp:
        case NSRightMouseDown:  button = INPUT_MOUSE_RIGHT_BUTTON; break;
        case NSOtherMouseUp:
        case NSOtherMouseDown:  button = INPUT_MOUSE_OTHER_BUTTON; break;
        case NSScrollWheel:     button = INPUT_MOUSE_SCROLL_WHEEL; break;
        case NSTabletProximity: button = INPUT_TOUCH_PROXIMITY;    break;
        default: break; /* nothing */
    }
    return button;
}

static int translate_modifier_flags(NSEvent* e) {
    NSEventModifierFlags mf = e.modifierFlags;
    int flags = 0;
    if (NSEventModifierFlagCapsLock & mf)   { flags |= INPUT_CAPS; }
    if (NSEventModifierFlagShift & mf)      { flags |= INPUT_SHIFT; }
    if (NSEventModifierFlagControl & mf)    { flags |= INPUT_CONTROL; }
    if (NSEventModifierFlagOption & mf)     { flags |= INPUT_OPTION; }
    if (NSEventModifierFlagCommand & mf)    { flags |= INPUT_COMMAND; }
    if (NSEventModifierFlagNumericPad & mf) { flags |= INPUT_NUM; }
    if (NSEventModifierFlagFunction & mf)   { flags |= INPUT_FN; }
    if (NSEventModifierFlagHelp & mf)       { flags |= INPUT_HELP; }
    if (e.isARepeat) { flags |= INPUT_REPEAT; }
    return flags;
}

static void fill_mouse_coordinates(input_event_t* ie) {
    ie->x = back.mouse_x;
    ie->y = back.mouse_y;
    ie->z = back.mouse_z;
}

static void keyboad_input(NSWindow* window, NSEvent* e, int mask) {
    unichar ch = [[e charactersIgnoringModifiers] characterAtIndex:0]; // 16 bits
    input_event_t ie = {0};
    ie.kind = INPUT_KEYBOARD;
    ie.flags = mask | translate_modifier_flags(e);
    ie.ch = (int)ch;
    ie.key = (int)e.keyCode;
    fill_mouse_coordinates(&ie);
    app->input(&ie);
    update_state(window);
}

static void mouse_input(NSEvent* e, int kind) {
    input_event_t ie = {0};
    ie.kind = kind;
    ie.x = back.mouse_x = e.locationInWindow.x;
    ie.y = back.mouse_y = e.locationInWindow.y;
    ie.z = back.mouse_z = e.pressure;
    ie.button = translate_type_to_button_number((int)e.type);
    app->input(&ie);
}

@implementation Window : NSWindow

- (instancetype) initWithContentRect: (NSRect) r styleMask:(NSWindowStyleMask) mask backing:(NSBackingStoreType) bst defer: (BOOL) flag {
    self = [super initWithContentRect: r styleMask: mask backing: bst defer: flag];
    if (self != null) {
        static NSOpenGLPixelFormatAttribute attrs[] = {
            NSOpenGLPFADoubleBuffer,
            NSOpenGLPFADepthSize, 32,
            // Must specify the 3.2 Core Profile to use OpenGL 3.2
            NSOpenGLPFAOpenGLProfile,
            NSOpenGLProfileVersion4_1Core, // NSOpenGLProfileVersion3_2Core,
            0
        };
        pixel_format  = [NSOpenGLPixelFormat.alloc initWithAttributes: attrs];
        context = [NSOpenGLContext.alloc initWithFormat: pixel_format shareContext: null];
        context.view = self.contentView;
        self.viewsNeedDisplay = true;
    }
    return self;
}

- (void) sendEvent: (NSEvent*) e { update_state(self); [super sendEvent: e]; }
- (BOOL) canBecomeKeyWindow { return true; } // https://stackoverflow.com/questions/11622255/keydown-not-being-called
- (BOOL) canBecomeMainWindow { return true; }
- (void) close { (void)super.close; app->stop(); }
//  calling super.keyDown will play a keyboar input refused sound
- (void) keyDown: (NSEvent*) e { keyboad_input(self, e, INPUT_KEYDOWN); }
- (void) keyUp: (NSEvent*) e { keyboad_input(self, e, INPUT_KEYUP); [super keyUp: e]; }
- (void) mouseDown: (NSEvent*) e { mouse_input(e, INPUT_MOUSE_DOWN); [super mouseDown: e]; }
- (void) mouseUp: (NSEvent*) e { mouse_input(e, INPUT_MOUSE_UP); [super mouseUp: e]; }
- (void) rightMouseDown: (NSEvent*) e { mouse_input(e, INPUT_MOUSE_DOWN); [super rightMouseDown: e]; }
- (void) rightMouseUp: (NSEvent*) e { mouse_input(e, INPUT_MOUSE_UP); [super rightMouseUp: e]; }
- (void) otherMouseDown: (NSEvent*) e { mouse_input(e, INPUT_MOUSE_DOWN); [super otherMouseDown: e]; }
- (void) otherMouseUp: (NSEvent*) e { mouse_input(e, INPUT_MOUSE_UP); [super otherMouseUp: e]; }
// mouse motion outside window is also captured on OSX:
- (void) mouseMoved: (NSEvent*) e { mouse_input(e, INPUT_MOUSE_MOVE); [super mouseMoved: e]; }
- (void) mouseDragged: (NSEvent*) e { mouse_input(e, INPUT_MOUSE_DRAG); [super mouseDragged: e]; }
- (void) setTimer: (int) frequency {
    assert(frequency > 0);
    uint64_t nanoseconds = frequency == 0 ? 0 : NSEC_PER_SEC / frequency;
    if (timer != null) {
        dispatch_cancel(timer);
    }
    timer = nanoseconds > 0 ? dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue()) : null;
    if (timer != null) {
        dispatch_source_set_event_handler(timer, ^{
            if (app->timer != null) { app->timer(); }
        });
        dispatch_source_set_timer(timer, dispatch_time(DISPATCH_TIME_NOW, nanoseconds), nanoseconds, 0);
        dispatch_resume(timer);
    }
}

- (void) displayIfNeeded {
    update_state(self);
    (void)context.makeCurrentContext;
    app->paint(0, 0, app->window_w, app->window_h);
    (void)context.flushBuffer;
    (void)NSOpenGLContext.clearCurrentContext;
}

@end

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate> {
    Window* window;
}
@end

@implementation AppDelegate : NSObject

- (id) init {
    if (self = [super init]) {
        window = create_window();
        window.delegate = self;
    }
    return self;
}

- (void) applicationWillFinishLaunching:(NSNotification *)notification {
    window.title = NSProcessInfo.processInfo.processName;
    [window cascadeTopLeftFromPoint: NSMakePoint(20,20)];
}

- (void) applicationDidFinishLaunching:(NSNotification *)notification { [window makeKeyAndOrderFront: self]; }

- (void) preferences: (nullable id)sender { if (app->prefs != null) { app->prefs(); } }

- (void) hide: (nullable id)sender { hide_application(window, true); }

// On OSX windowDidMove, windowDidResize cannot call reshape(); here because there is no glContext

- (void) windowDidMove: (NSNotification*) n { reshape(window); }

- (void) windowDidResize: (NSNotification*) n {
    [window->context makeCurrentContext]; // in case reshape() needs to do OpenGL calls
    [window->context update]; // this is very important - resize does not work without it!
    reshape(window); // see: https://lists.apple.com/archives/cocoa-dev/2006/Oct/msg01512.html
    [NSOpenGLContext clearCurrentContext];
}

- (void) applicationDidHide: (NSNotification*) n {
    app->window_state |= WINDOW_STATE_HIDDEN;
    back.window_state |= WINDOW_STATE_HIDDEN;
    update_state(window);
}

- (void) applicationDidUnhide: (NSNotification*) n {
    app->window_state &= ~WINDOW_STATE_HIDDEN;
    back.window_state &= ~WINDOW_STATE_HIDDEN;
    update_state(window);
}

- (void) applicationDidBecomeActive: (NSNotification*) n {
    app->window_state &= ~WINDOW_STATE_HIDDEN;
    back.window_state &= ~WINDOW_STATE_HIDDEN;
    update_state(window);
    hide_application(window, false);
}

- (BOOL) applicationShouldTerminateAfterLastWindowClosed: (NSApplication*) sender { return true; }
- (BOOL) acceptsFirstResponder { return true; } /* receive key events */

/* flagsChanged must be in NSWindowDelegate orherwise Cocoa assert on invalid state */
- (void) flagsChanged: (NSEvent*) e { keyboad_input(window, e, 0); [self flagsChanged: e]; }

@end

static void later(void* that, void* message, void (*callback)(void* _that, void* _message)) {
    dispatch_async(dispatch_get_main_queue(), ^{
        callback(that, message);
    });
}

static void stop() {
    [NSApplication.sharedApplication stop: NSApplication.sharedApplication];
}

static void* map_resource(const char* resource_name, int* size) {
    NSString* name = [NSString stringWithUTF8String: resource_name];
    NSString* path = [NSBundle.mainBundle pathForResource: name.stringByDeletingPathExtension ofType: name.pathExtension];
    void* a = null;
    if (path != null) {
        int fd = open(path.UTF8String, O_RDONLY);
        if (fd > 0) {
            struct stat s = {0};
            if (fstat(fd, &s) == 0) {
                a = mmap(0, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
                if (a != null) {
                    *size = (int)s.st_size;
                }
            }
            close(fd);
        }
        return a;
    }
    return null;
}

static void unmap_resource(void* a, int size) {
    munmap(a, size);
}

static void menu_add_item(NSMenu* submenu, NSString* title, SEL callback, NSString* key) {
    [submenu addItem: [NSMenuItem.alloc initWithTitle: title action: callback keyEquivalent: key]];
}

int main(int argc, const char* argv[]) {
    (void)nanotime(); // initialize time0
    app = run(argc, argv);
    if (app == null) { return 0; }
    app->stop = stop;
    app->later = later;
    app->redraw = redraw;
    app->map_resource = map_resource;
    app->unmap_resource = unmap_resource;
    NSApplication* a = NSApplication.sharedApplication;
    a.activationPolicy = NSApplicationActivationPolicyRegular;
    NSMenuItem* i = NSMenuItem.new;
    i.submenu = NSMenu.new;
//  i.submenu.autoenablesItems = false;
    NSString* quit = [@"Quit " stringByAppendingString: NSProcessInfo.processInfo.processName];
    menu_add_item(i.submenu, @"Preferences...", @selector(preferences:), @",");
    menu_add_item(i.submenu, @"Hide", @selector(hide:), @"H");
    menu_add_item(i.submenu, @"Enter Full Screen", @selector(toggleFullScreen:), @"F");
    menu_add_item(i.submenu, quit, @selector(stop:), @"Q");
    a.mainMenu = NSMenu.new;
    [a.mainMenu addItem: i];
    AppDelegate* d = AppDelegate.new;
    a.delegate = d;
    (void)a.run;
    int exit_status = app->exits != null ? app->exits() : 0;
    return exit_status;
}
