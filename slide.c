#include <X11/Xlib.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/Xatom.h>
#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <limits.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "slide.h"

static client       *list = {0}, *cur;
static int          sw, sh, wx, wy, numlock = 0;
static unsigned int ww, wh;

static int          vx = 0, vy = 0;

static int          panning = 0;
static int          pan_start_x, pan_start_y;   
static int          pan_origin_vx, pan_origin_vy;

static Display      *d;
static XButtonEvent mouse;
static Window       root;

static Atom net_supported, net_wm_window_type, net_wm_window_type_dock,
            net_wm_strut, net_wm_strut_partial,
            net_supporting_wm_check, net_wm_name, ewmh_utf8_string;

static int strut[4] = {0, 0, 0, 0};

static void (*events[LASTEvent])(XEvent *e) = {
    [ButtonPress]      = button_press,
    [ButtonRelease]    = button_release,
    [ConfigureRequest] = configure_request,
    [KeyPress]         = key_press,
    [MapRequest]       = map_request,
    [MappingNotify]    = mapping_notify,
    [DestroyNotify]    = notify_destroy,
    [EnterNotify]      = notify_enter,
    [MotionNotify]     = notify_motion
};

#include "config.h"


void win_size(Window w, int *x, int *y, unsigned int *width, unsigned int *height) {
    unsigned int bw, depth;
    Window root_ret;
    XGetGeometry(d, w, &root_ret, x, y, width, height, &bw, &depth);
}

static int to_screen_x(int cx) { return cx - vx; }
static int to_screen_y(int cy) { return cy - vy; }

static void win_reposition(client *c) {
    XMoveWindow(d, c->w, to_screen_x(c->cx), to_screen_y(c->cy));
}

static void reproject_all(void) {
    for (client *c = list; c; ) {
        win_reposition(c);
        c = c->next;
        if (c == list) break;
    }
}


void pan_by(int dx, int dy) {
    vx += dx;
    vy += dy;
    reproject_all();
}

void pan_by_key(const Arg arg) {
    switch (arg.i) {
        case 0: pan_by(-PAN_STEP, 0); break; /* Left  */
        case 1: pan_by( PAN_STEP, 0); break; /* Right */
        case 2: pan_by(0, -PAN_STEP); break; /* Up    */
        case 3: pan_by(0,  PAN_STEP); break; /* Down  */
    }
}


void win_move(const Arg arg) {
    if (!cur || cur->f) return;

    switch (arg.i) {
        case 0: cur->cx -= WIN_MOVE_STEP; break;
        case 1: cur->cx += WIN_MOVE_STEP; break;
        case 2: cur->cy -= WIN_MOVE_STEP; break;
        case 3: cur->cy += WIN_MOVE_STEP; break;
    }
    win_reposition(cur);
}


void win_focus(client *c) {
    cur = c;
    XSetInputFocus(d, cur->w, RevertToParent, CurrentTime);
    XRaiseWindow(d, cur->w);
}


void notify_destroy(XEvent *e) {
    win_del(e->xdestroywindow.window);
    if (list) win_focus(list->prev);
}

void notify_enter(XEvent *e) {
    while (XCheckTypedEvent(d, EnterNotify, e));
    for (client *c = list; c; ) {
        if (c->w == e->xcrossing.window) { win_focus(c); break; }
        c = c->next;
        if (c == list) break;
    }
}

void notify_motion(XEvent *e) {
    while (XCheckTypedEvent(d, MotionNotify, e));

    /* Panning: Super+Shift+RightButton drag */
    if (panning) {
        int dx = e->xbutton.x_root - pan_start_x;
        int dy = e->xbutton.y_root - pan_start_y;
        vx = pan_origin_vx - dx;
        vy = pan_origin_vy - dy;
        reproject_all();
        return;
    }

    if (!mouse.subwindow || !cur || cur->f) return;

    int xd = e->xbutton.x_root - mouse.x_root;
    int yd = e->xbutton.y_root - mouse.y_root;

    if (mouse.button == 1) {
        cur->cx = wx + xd + vx;   
        cur->cy = wy + yd + vy;
        win_reposition(cur);
    } else if (mouse.button == 3 && !panning) {
        XMoveResizeWindow(d, mouse.subwindow,
            to_screen_x(cur->cx),
            to_screen_y(cur->cy),
            MAX(1, ww + xd),
            MAX(1, wh + yd));
    }
}

void key_press(XEvent *e) {
    KeySym keysym = XkbKeycodeToKeysym(d, e->xkey.keycode, 0, 0);

    for (unsigned int i = 0; i < sizeof(keys)/sizeof(*keys); ++i)
        if (keys[i].keysym == keysym &&
            mod_clean(keys[i].mod) == mod_clean(e->xkey.state))
            keys[i].function(keys[i].arg);
}

void button_press(XEvent *e) {
    if (e->xbutton.button == 3 &&
        (mod_clean(e->xbutton.state) == mod_clean(MOD | ShiftMask))) {
        panning      = 1;
        pan_start_x  = e->xbutton.x_root;
        pan_start_y  = e->xbutton.y_root;
        pan_origin_vx = vx;
        pan_origin_vy = vy;
        XGrabPointer(d, root, True,
            ButtonReleaseMask | PointerMotionMask,
            GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
        return;
    }

    if (!e->xbutton.subwindow) return;

    win_size(e->xbutton.subwindow, &wx, &wy, &ww, &wh);
    XRaiseWindow(d, e->xbutton.subwindow);
    mouse = e->xbutton;

    for (client *c = list; c; ) {
        if (c->w == e->xbutton.subwindow) { win_focus(c); break; }
        c = c->next;
        if (c == list) break;
    }
}

void button_release(XEvent *e) {
    (void)e;
    if (panning) {
        panning = 0;
        XUngrabPointer(d, CurrentTime);
        return;
    }
    mouse.subwindow = 0;
}


void win_add(Window w) {
    client *c;

    if (!(c = (client *) calloc(1, sizeof(client)))) exit(1);

    c->w = w;

    if (list) {
        list->prev->next = c;
        c->prev          = list->prev;
        list->prev       = c;
        c->next          = list;
    } else {
        list       = c;
        list->prev = list->next = list;
    }
}

void win_del(Window w) {
    client *x = 0;

    for (client *c = list; c; ) {
        if (c->w == w) { x = c; break; }
        c = c->next;
        if (c == list) break;
    }

    if (!list || !x)  return;
    if (x->prev == x) list = 0;
    if (list == x)    list = x->next;
    if (x->next)      x->next->prev = x->prev;
    if (x->prev)      x->prev->next = x->next;

    free(x);
}


void win_kill(const Arg arg) {
    (void)arg;
    if (cur) XKillClient(d, cur->w);
}

void win_center(const Arg arg) {
    (void)arg;
    if (!cur) return;

    win_size(cur->w, &(int){0}, &(int){0}, &ww, &wh);
    cur->cx = vx + (sw - (int)ww) / 2;
    cur->cy = vy + (sh - (int)wh) / 2;
    win_reposition(cur);
}

void win_fs(const Arg arg) {
    (void)arg;
    if (!cur) return;

    if ((cur->f = cur->f ? 0 : 1)) {
        win_size(cur->w, &cur->wx, &cur->wy, &cur->ww, &cur->wh);
        XMoveResizeWindow(d, cur->w, 0, 0, sw, sh);
    } else {
        XMoveResizeWindow(d, cur->w,
            to_screen_x(cur->cx), to_screen_y(cur->cy),
            cur->ww, cur->wh);
    }
}

static void viewport_center_on(client *c) {
    unsigned int cw, ch;
    win_size(c->w, &(int){0}, &(int){0}, &cw, &ch);
    vx = c->cx - (sw - (int)cw) / 2;
    vy = c->cy - (sh - (int)ch) / 2;
    reproject_all();
}

void win_cycle(const Arg arg) {
    if (!cur || list->next == list) return;
    client *c = arg.i ? cur->prev : cur->next;
    if (!c) return;
    win_focus(c);
    viewport_center_on(cur);
}
void configure_request(XEvent *e) {
    XConfigureRequestEvent *ev = &e->xconfigurerequest;

    XConfigureWindow(d, ev->window, ev->value_mask, &(XWindowChanges) {
        .x          = ev->x,
        .y          = ev->y,
        .width      = ev->width,
        .height     = ev->height,
        .sibling    = ev->above,
        .stack_mode = ev->detail
    });
}

static int win_is_dock(Window w) {
    Atom type;
    int fmt;
    unsigned long n, extra;
    unsigned char *data = NULL;

    if (XGetWindowProperty(d, w, net_wm_window_type, 0, 1, False, XA_ATOM,
            &type, &fmt, &n, &extra, &data) == Success && data) {
        int dock = (*(Atom *)data == net_wm_window_type_dock);
        XFree(data);
        return dock;
    }
    return 0;
}

static void update_struts(Window w) {
    Atom type;
    int fmt;
    unsigned long n, extra;
    unsigned char *data = NULL;

    if (XGetWindowProperty(d, w, net_wm_strut_partial, 0, 12, False, XA_CARDINAL,
            &type, &fmt, &n, &extra, &data) == Success && data && n >= 4) {
        long *s = (long *)data;
        strut[0] = MAX(strut[0], (int)s[0]);
        strut[1] = MAX(strut[1], (int)s[1]);
        strut[2] = MAX(strut[2], (int)s[2]);
        strut[3] = MAX(strut[3], (int)s[3]);
        XFree(data);
        return;
    }
    if (data) XFree(data);

    if (XGetWindowProperty(d, w, net_wm_strut, 0, 4, False, XA_CARDINAL,
            &type, &fmt, &n, &extra, &data) == Success && data && n == 4) {
        long *s = (long *)data;
        strut[0] = MAX(strut[0], (int)s[0]);
        strut[1] = MAX(strut[1], (int)s[1]);
        strut[2] = MAX(strut[2], (int)s[2]);
        strut[3] = MAX(strut[3], (int)s[3]);
        XFree(data);
    }
}

void map_request(XEvent *e) {
    Window w = e->xmaprequest.window;

    if (win_is_dock(w)) {
        update_struts(w);
        XMapWindow(d, w);
        return;
    }

    XSelectInput(d, w, StructureNotifyMask | EnterWindowMask);
    win_size(w, &wx, &wy, &ww, &wh);
    win_add(w);
    cur = list->prev;

    if (wx + wy == 0) {
        int cx_root, cy_root, dummy;
        unsigned int udummy;
        Window wdummy;
        XQueryPointer(d, root, &wdummy, &wdummy, &cx_root, &cy_root,
                      &dummy, &dummy, &udummy);
        win_size(w, &dummy, &dummy, &ww, &wh);

        int ax = strut[0], ay = strut[2];
        int aw = sw - strut[0] - strut[1];
        int ah = sh - strut[2] - strut[3];

        int sx = cx_root - (int)ww / 2;
        int sy = cy_root - (int)wh / 2;
        if (sx < ax)                  sx = ax;
        if (sy < ay)                  sy = ay;
        if (sx + (int)ww > ax + aw)   sx = ax + aw - (int)ww;
        if (sy + (int)wh > ay + ah)   sy = ay + ah - (int)wh;

        cur->cx = sx + vx;
        cur->cy = sy + vy;
        XMoveWindow(d, w, sx, sy);
    } else {
        cur->cx = wx + vx;
        cur->cy = wy + vy;
    }

    XMapWindow(d, w);
    win_focus(list->prev);
}

void mapping_notify(XEvent *e) {
    XMappingEvent *ev = &e->xmapping;

    if (ev->request == MappingKeyboard || ev->request == MappingModifier) {
        XRefreshKeyboardMapping(ev);
        input_grab(root);
    }
}

void run(const Arg arg) {
    if (fork()) return;
    if (d) close(ConnectionNumber(d));
    setsid();
    execvp((char*)arg.com[0], (char**)arg.com);
}

void input_grab(Window root) {
    unsigned int i, j, modifiers[] = {0, LockMask, numlock, numlock|LockMask};
    XModifierKeymap *modmap = XGetModifierMapping(d);
    KeyCode code;

    for (i = 0; i < 8; i++)
        for (int k = 0; k < modmap->max_keypermod; k++)
            if (modmap->modifiermap[i * modmap->max_keypermod + k]
                == XKeysymToKeycode(d, 0xff7f))
                numlock = (1 << i);

    XUngrabKey(d, AnyKey, AnyModifier, root);

    for (i = 0; i < sizeof(keys)/sizeof(*keys); i++)
        if ((code = XKeysymToKeycode(d, keys[i].keysym)))
            for (j = 0; j < sizeof(modifiers)/sizeof(*modifiers); j++)
                XGrabKey(d, code, keys[i].mod | modifiers[j], root,
                         True, GrabModeAsync, GrabModeAsync);

    for (i = 1; i < 4; i += 2)
        for (j = 0; j < sizeof(modifiers)/sizeof(*modifiers); j++) {
            XGrabButton(d, i, MOD | modifiers[j], root, True,
                ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
                GrabModeAsync, GrabModeAsync, 0, 0);
            XGrabButton(d, 3, MOD | ShiftMask | modifiers[j], root, True,
                ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
                GrabModeAsync, GrabModeAsync, 0, 0);
        }

    XFreeModifiermap(modmap);
}


int main(void) {
    XEvent ev;

    if (!(d = XOpenDisplay(0))) exit(1);

    signal(SIGCHLD, SIG_IGN);
    XSetErrorHandler(xerror);

    int s  = DefaultScreen(d);
    root   = RootWindow(d, s);
    sw     = XDisplayWidth(d, s);
    sh     = XDisplayHeight(d, s);

    net_supporting_wm_check  = XInternAtom(d, "_NET_SUPPORTING_WM_CHECK",  False);
    net_wm_name              = XInternAtom(d, "_NET_WM_NAME",              False);
    ewmh_utf8_string         = XInternAtom(d, "UTF8_STRING",               False);
    net_supported            = XInternAtom(d, "_NET_SUPPORTED",            False);
    net_wm_window_type       = XInternAtom(d, "_NET_WM_WINDOW_TYPE",       False);
    net_wm_window_type_dock  = XInternAtom(d, "_NET_WM_WINDOW_TYPE_DOCK",  False);
    net_wm_strut             = XInternAtom(d, "_NET_WM_STRUT",             False);
    net_wm_strut_partial     = XInternAtom(d, "_NET_WM_STRUT_PARTIAL",     False);

    Window wmcheck = XCreateSimpleWindow(d, root, 0, 0, 1, 1, 0, 0, 0);
    XChangeProperty(d, root,    net_supporting_wm_check, XA_WINDOW, 32,
        PropModeReplace, (unsigned char *)&wmcheck, 1);
    XChangeProperty(d, wmcheck, net_supporting_wm_check, XA_WINDOW, 32,
        PropModeReplace, (unsigned char *)&wmcheck, 1);
    XChangeProperty(d, wmcheck, net_wm_name, ewmh_utf8_string, 8,
        PropModeReplace, (unsigned char *)"slide", 5);

    Atom supported[] = {
        net_supporting_wm_check,
        net_wm_name,
        net_wm_window_type,
        net_wm_window_type_dock,
        net_wm_strut,
        net_wm_strut_partial,
    };
    XChangeProperty(d, root, net_supported, XA_ATOM, 32,
        PropModeReplace, (unsigned char *)supported,
        sizeof(supported) / sizeof(Atom));

    XSelectInput(d,  root, SubstructureRedirectMask);
    Cursor cursor = XcursorLibraryLoadCursor(d, "left_ptr");
    XDefineCursor(d, root, cursor);
    input_grab(root);

    while (1 && !XNextEvent(d, &ev))
        if (events[ev.type]) events[ev.type](&ev);
}
