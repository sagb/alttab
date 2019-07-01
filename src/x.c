/*
Interface with foreign windows in raw X11.

Copyright 2017-2019 Alexander Kulak.
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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <X11/Xatom.h>
#include <stdbool.h>
#include <stdio.h>
#include "alttab.h"
#include "util.h"
extern Globals g;
extern Display *dpy;
extern int scr;
extern Window root;

// PRIVATE

// max recursion for searching windows
// -1 is "everything"
// in raw X this returns too much windows, "1" is probably sufficient
// no need for an option
static int max_reclevel;

//
// get window group leader
// to be used later. rewritten, not tested.
//
Window x_get_leader(Window win)
{
    Window *retprop;
    XWMHints *h;
    Window leader = None;

    retprop =
        (Window *) get_x_property(win, XA_WINDOW, "WM_CLIENT_LEADER", NULL);
    if (retprop != NULL) {
        leader = retprop[0];
        XFree(retprop);
    } else {
        if (!(h = XGetWMHints(dpy, win)))
            return None;
        if (h->flags & WindowGroupHint)
            leader = h->window_group;
    }
    return leader;
}

//
// set winlist,maxNdx recursively using raw Xlib
// first call should be (win=root, reclevel=0)
//
static int x_initWindowsInfoRecursive(Window win, int reclevel)
{

    Window root, parent;
    Window *children;
    unsigned int nchildren, i;
//    Window leader;
    XWindowAttributes wa;
    char *winname;

// check if window is "leader" or no prop, skip otherwise
// caveat: in rp, gvim leader has no file name and icon
// this doesn't work in raw X: leader may be not viewable.
// doesn't work in twm either: qutebrowser has meaningless leader.
/*
    leader = 0;
    if (g.option_wm == WM_TWM) {
        leader = x_get_leader (win);
        msg(1, "win: 0x%lx leader: 0x%lx\n", win, leader);
    }
*/
// in non-twm, add viewable only
// caveat: in rp, skips anything except of visible window
// probably add an option for this in WMs too?
    wa.map_state = 0;
    if (g.option_wm != WM_TWM)
        XGetWindowAttributes(dpy, win, &wa);

// in twm-like, add only windows with a name
    winname = NULL;
    if (g.option_wm == WM_TWM) {
        winname = get_x_property(win, XA_STRING, "WM_NAME", NULL);
    }
// insert detailed window data in window list
    if ((g.option_wm == WM_TWM || wa.map_state == IsViewable)
        && reclevel != 0 && (g.option_wm != WM_TWM || winname != NULL)
//            && (g.option_wm != WM_TWM || leader == win)
        && !common_skipWindow(win, DESKTOP_UNKNOWN, DESKTOP_UNKNOWN)
        ) {
        addWindowInfo(win, reclevel, 0, DESKTOP_UNKNOWN, winname);
    }
// skip children if max recursion level reached
    if (max_reclevel != -1 && reclevel >= max_reclevel)
        return 1;

// recursion
    if (XQueryTree(dpy, win, &root, &parent, &children, &nchildren) == 0) {
        msg(0, "can't get window tree for 0x%lx\n", win);
        return 0;
    }
    for (i = 0; i < nchildren; ++i) {
        x_initWindowsInfoRecursive(children[i], reclevel + 1);
    }

    if (nchildren > 0 && children) {
        XFree(children);
    }
    return 1;
}

//
// set window focus in raw X
//
static int x_setFocus(int wndx)
{
    Window w = g.winlist[wndx].id;

// 1. XWarpPointer
// If such WMs would be discovered that prevent our focus
// AND set their own focus via the pointer only.

// 2. XRaiseWindow required, but doesn't make window "Viewable"
    XRaiseWindow(dpy, w);

// 3. XSetInputFocus
// "The specified focus window must be viewable at the time
// XSetInputFocus is called, or a BadMatch error results."
// This check is redundant: non-viewable windows isn't added to winlist in raw X anyway
    XWindowAttributes att;
    XGetWindowAttributes(dpy, w, &att);
    if (att.map_state == IsViewable)
        XSetInputFocus(dpy, w, RevertToParent, CurrentTime);

    return 1;
}

static int xSetFocus(int idx)
{
    int r;

    // for WM which isn't identified as EWMH compatible
    // but accepts setting focus (dwm)
    r = ewmh_setFocus(idx, 0);
    x_setFocus(idx);

    return r;
}

static int xStartup(void)
{
    max_reclevel = 1;
    return 1;
}

static int twmStartup(void)
{
    max_reclevel = -1;
    return 1;
}

// PUBLIC

struct WmOps WmNoOps = {
    .startup = xStartup,
    .winlist = x_initWindowsInfoRecursive,
    .setFocus = xSetFocus,
};

struct WmOps WmTwmOps = {
    .startup = twmStartup,
    .winlist = x_initWindowsInfoRecursive,
    .setFocus = xSetFocus,
};
