#ifndef SOWM_H
#define SOWM_H

#include <X11/Xlib.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define for_win for (client *c = list; c; c = c->next) \
                    if (c == list && c != list->prev ? (c = c->next, 1) : 1)

#define mod_clean(mask) ((mask) & ~(LockMask | (numlock)) & \
                         (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))

typedef union {
    const char **com;
    const int    i;
} Arg;

typedef struct {
    unsigned int mod;
    KeySym       keysym;
    void       (*function)(const Arg);
    const Arg    arg;
} key;

typedef struct client {
    struct client *next, *prev;
    Window         w;
    int            wx, wy;     
    unsigned int   ww, wh;
    int            f;         
    int            cx, cy;   
} client;

void button_press(XEvent *e);
void button_release(XEvent *e);
void configure_request(XEvent *e);
void key_press(XEvent *e);
void map_request(XEvent *e);
void mapping_notify(XEvent *e);
void notify_destroy(XEvent *e);
void notify_enter(XEvent *e);
void notify_motion(XEvent *e);

void input_grab(Window root);
void run(const Arg arg);
void win_add(Window w);
void win_del(Window w);
void win_focus(client *c);
void win_move(const Arg arg);
void win_kill(const Arg arg);
void win_center(const Arg arg);
void win_fs(const Arg arg);
void win_cycle(const Arg arg);
void win_size(Window w, int *x, int *y, unsigned int *width, unsigned int *height);

void pan_by(int dx, int dy);
void pan_by_key(const Arg arg);

static int xerror(Display *d, XErrorEvent *e) { (void)d; (void)e; return 0; }

#endif
