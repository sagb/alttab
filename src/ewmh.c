/*
Interface with EWMH-compatible window managers.
Note: _WIN fallbacks are not part of EWMH or ICCCM, but kept here anyway.

Copyright 2017-2024 Alexander Kulak.
This file is part of alttab program.

alttab is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

alttab is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with alttab.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <X11/Xft/Xft.h>
#include "alttab.h"
#include "util.h"
extern Globals g;
extern Display *dpy;
extern int scr;
extern Window root;

// PRIVATE

//
// returns ptr to EWMH client list and client_list_size
// or NULL
//
static Window *ewmh_get_client_list(unsigned long *client_list_size)
{
    Window *client_list;

    if (g.ewmh.try_stacking_list_first) {
        if ((client_list =
             (Window *) get_x_property(root, XA_WINDOW,
                                       "_NET_CLIENT_LIST_STACKING",
                                       client_list_size)) != NULL) {
            msg(1, "ewmh found stacking window list\n");
            return client_list;
        } else {
            g.ewmh.try_stacking_list_first = false;
        }
    }

    client_list =
        (Window *) get_x_property_alt(root,
                                      XA_WINDOW, "_NET_CLIENT_LIST",
                                      XA_CARDINAL, "_WIN_CLIENT_LIST",
                                      client_list_size);

    return client_list;
}

static int ewmh_send_wm_evt(Window w, char *atom, unsigned long edata[])
{
    XEvent evt;
    long rn_mask = SubstructureRedirectMask | SubstructureNotifyMask;
    evt.xclient.window = w;
    evt.xclient.type = ClientMessage;
    evt.xclient.message_type = XInternAtom(dpy, atom, False);
    evt.xclient.serial = 0;
    evt.xclient.send_event = True;
    evt.xclient.format = 32;
    //memset (&(evt.xclient.data.l[1]), 0, 4*sizeof(evt.xclient.data.l[1]));
    int ei;
    for (ei = 0; ei < 5; ei++)
        evt.xclient.data.l[ei] = edata[ei];
    if (!XSendEvent(dpy, root, False, rn_mask, &evt)) {
        msg(-1, "can't send %s xevent\n", atom);
        return 0;
    }
    return 1;
}

static int ewmh_switch_desktop(unsigned long desktop)
{
    int evr, elapsed;
    unsigned long edata[] = { desktop, CurrentTime, 0, 0, 0 };
    if (desktop == -1 && g.ewmh.minus1_desktop_unusable)
        return 0;
    msg(1, "ewmh switching desktop to %ld\n", desktop);
    evr = ewmh_send_wm_evt(root, "_NET_CURRENT_DESKTOP", edata);
    if (evr == 0)
        return 0;
    // wait for WM (#45)
#define WM_POLL_TIMEOUT   200000    // 200 ms
#define WM_POLL_INTERVAL   10000
    for (elapsed = 0;
         elapsed < WM_POLL_TIMEOUT
         && ewmh_getCurrentDesktop() != desktop; elapsed += WM_POLL_INTERVAL) {
        msg(1, "usleep %d\n", WM_POLL_INTERVAL);
        usleep(WM_POLL_INTERVAL);
    }
    return evr;
}

static int ewmh_switch_window(unsigned long window)
{
    unsigned long edata[] = { 2, CurrentTime, 0, 0, 0 };
    msg(1, "ewmh switching window to 0x%lx\n", window);
    return ewmh_send_wm_evt(window, "_NET_ACTIVE_WINDOW", edata);
}

// PUBLIC

//
// initialize EwmhFeatures
// return true if usable at all
//
bool ewmh_detectFeatures(EwmhFeatures * e)
{
    Window *chld_win;
    char *r;
    Atom utf8string;
    char *default_wm_name = "unknown_ewmh_compatible";
    unsigned long client_list_size;
    Window *client_list;

    // This function is used in alttab for detection of EWMH compatibility.
    // But there are WM (dwm) that support EWMH subset required for alttab
    // except of _NET_SUPPORTING_WM_CHECK/_WIN_SUPPORTING_WM_CHECK detection.
    // We detect those WM as EWMH-compatible and return their name
    // as "unknown_ewmh_compatible".

    bzero(e, sizeof(EwmhFeatures));
    e->try_stacking_list_first = true;

    // first, detect necessary feature: client list
    // also, this resets try_stacking_list_first if necessary
    client_list = ewmh_get_client_list(&client_list_size);
    if (client_list == NULL) {
        // WM is not usable in EWMH mode
        return false;
    }
    free(client_list);

    // then, guess/devise WM name
    chld_win = (Window *)get_x_property_alt(root,
                                            XA_WINDOW, "_NET_SUPPORTING_WM_CHECK",
                                            XA_CARDINAL, "_WIN_SUPPORTING_WM_CHECK",
                                            NULL);
    if (!chld_win) {
        e->wmname = default_wm_name;
        return true;
    }

    utf8string = XInternAtom(dpy, "UTF8_STRING", False);

    r = get_x_property_alt(*chld_win,
                       utf8string, "_NET_WM_NAME",
                       XA_STRING, "_NET_WM_NAME", NULL);
    free(chld_win);

    e->wmname = (r != NULL) ? r : default_wm_name;

    // special workarounds
    if (strncmp(e->wmname, "CWM", 4) == 0)
        e->minus1_desktop_unusable = true;

    return true;
}

//
// active window or 0
//
Window ewmh_getActiveWindow()
{
    Window w = (Window) 0;
    char *awp;
    unsigned long sz;
    if ((awp = get_x_property(root, XA_WINDOW, "_NET_ACTIVE_WINDOW", &sz))) {
        w = *((Window *) awp);
        free(awp);
    } else {
        msg(0, "can't obtain _NET_ACTIVE_WINDOW\n");
    }
    return w;
}

//
// initialize winlist, correcting sortlist
// return 1 if ok
//
int ewmh_initWinlist()
{
    Window *client_list;
    unsigned long client_list_size;
    int i;
    Window aw;
    char *title;
    unsigned long current_desktop, window_desktop;

    current_desktop = ewmh_getCurrentDesktop();

    aw = ewmh_getActiveWindow();
    if (!aw) {
        msg(0, "can't obtain _NET_ACTIVE_WINDOW\n");
        // continue anyway
    }

    client_list = ewmh_get_client_list(&client_list_size);
    if (client_list == NULL) {
        msg(-1, "can't get client list\n");
        return 0;
    }

    for (i = 0; i < client_list_size / sizeof(Window); i++) {
        Window w = client_list[i];

        if (ewmh_skipWindowInTaskbar(w))
            continue;

        window_desktop = ewmh_getDesktopOfWindow(w);
        if (common_skipWindow(w, current_desktop, window_desktop))
            continue;

        // build title
        char *wmn1 = get_x_property(w, XA_STRING, "WM_NAME", NULL);
        Atom utf8str = XInternAtom(dpy, "UTF8_STRING", False);
        char *wmn2 = get_x_property(w, utf8str, "_NET_WM_NAME", NULL);
        title = wmn2 ? strdup(wmn2) : (wmn1 ? strdup(wmn1) : NULL);
        free(wmn1);
        free(wmn2);

        addWindowInfo(w, 0, 0, window_desktop, title);
        if (w == aw) {
            addToSortlist(w, true, true);   // pull to head
        }
        free(title);
    }

    // TODO: BUG? sometimes i3 returns previous active window,
    // which breaks sortlist
    // no more startNdx, can't output this
    //msg(1, "ewmh active window: %lu name: %s\n",
    //  aw, (g.winlist ? g.winlist[g.startNdx].name : "null"));

    free(client_list);
    return 1;
}

//
// focus window in EWMH WM
// fwin used if non-zero, winNdx otherwise
//
int ewmh_setFocus(int winNdx, Window fwin)
{
    Window win = (fwin != 0) ? fwin : g.winlist[winNdx].id;
    msg(1, "ewmh_setFocus win 0x%lx\n", win);
    if (fwin == 0 && g.option_desktop != DESK_CURRENT) {
        unsigned long wdesk = g.winlist[winNdx].desktop;
        unsigned long cdesk = ewmh_getCurrentDesktop();
        msg(1, "ewmh_setFocus fwin 0x%lx opt %d wdesk %lu cdesk %lu\n",
            fwin, g.option_desktop, wdesk, cdesk);
        if (cdesk != wdesk && wdesk != DESKTOP_UNKNOWN) {
            ewmh_switch_desktop(wdesk);
        }
    }
    ewmh_switch_window(win);
    XMapRaised(dpy, win);
    return 1;
}

static unsigned long ewmh_getDesktopFromProp(Window w, char *prop1, char *prop2)
{
    unsigned long *d;
    unsigned long propsize;
    unsigned long ret = DESKTOP_UNKNOWN;

    d = (unsigned long *)get_x_property_alt(w,
                                            XA_CARDINAL, prop1,
                                            XA_CARDINAL, prop2, &propsize);
    if (d && (propsize > 0))
        ret = *d;

    free(d);
    return ret;
}

//
// get current desktop in EWMH WM
//
unsigned long ewmh_getCurrentDesktop()
{
    return ewmh_getDesktopFromProp(root, "_NET_CURRENT_DESKTOP", "_WIN_WORKSPACE");
}

//
// get desktop of window w in EWMH WM
//
unsigned long ewmh_getDesktopOfWindow(Window w)
{
    return ewmh_getDesktopFromProp(w, "_NET_WM_DESKTOP", "_WIN_WORKSPACE");
}

//
// does window have _NET_WM_STATE_SKIP_TASKBAR
//
bool ewmh_skipWindowInTaskbar(Window w)
{
    Atom *state;
    long unsigned int state_propsize;
    Atom a_skip_tb;
    int i;
    bool ret = false;

    if (g.option_no_skip_taskbar)
        return false;
    state =
        (Atom *) get_x_property(w, XA_ATOM, "_NET_WM_STATE", &state_propsize);
    if (state == NULL || state_propsize == 0) {
        msg(1, "%lx: no _NET_WM_STATE property\n", w);
        goto out;
    }
    a_skip_tb = XInternAtom(dpy, "_NET_WM_STATE_SKIP_TASKBAR", True);
    for (i = 0; i < state_propsize / sizeof(Atom); i++) {
        if (state[i] == a_skip_tb) {
            msg(1, "%lx: _NET_WM_STATE_SKIP_TASKBAR found\n", w);
            ret = true;
            goto out;
        }
    }

 out:
    free(state);
    return ret;
}
