/*
Interface with foreign windows common for all WMs.

This file is part of alttab program.
alttab is Copyright (C) 2016, by respective author (sa).
It is free software; you can redistribute it and/or modify it under the terms of either:
a) the GNU General Public License as published by the Free Software Foundation; either version 1, or (at your option) any later version, or
b) the "Artistic License".
*/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <X11/Xatom.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "alttab.h"
extern Globals g;

// PUBLIC

//
// early initialization
//
int startupWintasks (Display* dpy) {
switch (g.option_wm) {
  case WM_NO:
    return 1;
  case WM_RATPOISON:
    return rp_startupWintasks();
  case WM_EWMH:
    return 1;
  default:
    return 0;
}
}

//
// add single window info into g.winlist (used by x, rp, ...)
// only dpy and win are mandatory
//
int addWindowInfo (Display* dpy, Window win, int reclevel, int wm_id, char* wm_name) {

    if (! (g.winlist = realloc (g.winlist, (g.maxNdx+1)*sizeof(WindowInfo))) )
        return 0;
    g.winlist[g.maxNdx].id = win;
    g.winlist[g.maxNdx].wm_id = wm_id;

    // 1. get name

    if (wm_name) {
        strncpy (g.winlist[g.maxNdx].name, wm_name, MAXNAMESZ);
    } else {

        unsigned char* wn;
/*
        // not sufficient:
        // http://stackoverflow.com/questions/9364668/xlib-window-name-problems
        // TODO: moreover, AnyPropertyType is not sufficient too
        if (XFetchName (dpy, win, &wn) && wn) {
        strncpy (g.winlist[g.maxNdx].name, wn, MAXNAMESZ);
        g.winlist[g.maxNdx].name[MAXNAMESZ-1]='\0';
        XFree (wn);
*/
        Atom prop = XInternAtom (dpy, "WM_NAME", false), type;
        int form;
        unsigned long remain, len;
        if (XGetWindowProperty (dpy, win, prop, 0, MAXNAMESZ, false,
                AnyPropertyType, &type, &form, &len, &remain, &wn
                ) == Success && wn) {
            strncpy (g.winlist[g.maxNdx].name, wn, MAXNAMESZ);
            g.winlist[g.maxNdx].name[MAXNAMESZ-1]='\0';
            XFree (wn); // TODO: free another vars too?
        } else {
            g.winlist[g.maxNdx].name[0] = '\0';
        }
    } // guessing name without WM hints

    // 2. icon

    // WM_HINTS: https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/wm-hints.html
    // TODO by priority: 
    //    option to use full windows as icons. https://www.talisman.org/~erlkonig/misc/x11-composite-tutorial/
    //    _NET_WM_ICON: http://unix.stackexchange.com/questions/48860/how-to-dump-the-icon-of-a-running-x-program
    //    understand hints->icon_window. for xterm, it seems not usable.
    XWMHints* hints;
    g.winlist[g.maxNdx].icon_drawable = 
    g.winlist[g.maxNdx].icon_w = 
    g.winlist[g.maxNdx].icon_h = 0;
    int icon_depth = 0;
    g.winlist[g.maxNdx].icon_allocated = false;
    if (hints = XGetWMHints (dpy, win)) {
        if (g.debug>1) { fprintf (stderr, "IconPixmapHint: %d, icon_pixmap: %d, IconMaskHint: %d, icon_mask: %d, IconWindowHint: %d, icon_window: %d\n", hints->flags & IconPixmapHint, hints->icon_pixmap, hints->flags & IconMaskHint, hints->icon_mask, hints->flags & IconWindowHint, hints->icon_window); }
        if ( (hints->flags & IconWindowHint)  & (! (hints->flags & IconPixmapHint)) ) {
            if (g.debug>0) {fprintf (stderr, "attention: icon_window without icon_pixmap in hints, ignoring it assuming it's not usable, like in xterm\n");}
        }
        g.winlist[g.maxNdx].icon_drawable = 
            (hints->flags & IconPixmapHint) ?  hints->icon_pixmap : 0;
//            ((hints->flags & IconPixmapHint) ?  hints->icon_pixmap : (
//            (hints->flags & IconWindowHint) ?  hints->icon_window : 0
//            );
        XFree (hints);
        // extract icon width/height
        Window root_return; int x_return, y_return;
        unsigned int border_width_return;
        if (g.winlist[g.maxNdx].icon_drawable) {
            if (XGetGeometry (dpy, g.winlist[g.maxNdx].icon_drawable, 
                  &root_return, &x_return, &y_return, 
                  &(g.winlist[g.maxNdx].icon_w), 
                  &(g.winlist[g.maxNdx].icon_h),
                  &border_width_return, &icon_depth) ==0 ) {
                if (g.debug>0) {fprintf (stderr, "icon dimensions unknown (%s)\n", g.winlist[g.maxNdx].name);}
                // TODO: placeholder?
                g.winlist[g.maxNdx].icon_drawable = 0;
            } else {
                if (g.debug>1) {fprintf (stderr, "depth=%d\n", icon_depth);}
            }
        } else {
            if (g.debug>0) {fprintf (stderr, "no icon in WM hints (%s)\n", g.winlist[g.maxNdx].name);}
        }
    } else {
        if (g.debug>0) {fprintf (stderr, "no WM hints (%s)\n", g.winlist[g.maxNdx].name);}
    }
    // convert icon with different depth (currently 1 only) into default depth
    if (g.winlist[g.maxNdx].icon_drawable && icon_depth==1) {
        if (g.debug>0) {fprintf (stderr, "rebuilding icon from depth 1 to %d (%s)\n", XDEPTH, g.winlist[g.maxNdx].name);}
        Pixmap pswap = XCreatePixmap (dpy, g.winlist[g.maxNdx].icon_drawable, 
                g.winlist[g.maxNdx].icon_w, g.winlist[g.maxNdx].icon_h, XDEPTH);
        if (!pswap) die ("can't create pixmap");
        // GC should be already prepared in uiShow
        if (!XCopyPlane (dpy, g.winlist[g.maxNdx].icon_drawable, pswap, g.gcDirect,
                0,0, g.winlist[g.maxNdx].icon_w, g.winlist[g.maxNdx].icon_h, 
                0,0,  1)) die ("can't copy plane"); // plane #1?
        g.winlist[g.maxNdx].icon_drawable = pswap;
        g.winlist[g.maxNdx].icon_allocated = true; // for subsequent free()
        icon_depth=XDEPTH;
    }
    if (g.winlist[g.maxNdx].icon_drawable && icon_depth!=XDEPTH) {
        fprintf (stderr, "Can't handle icon depth other than %d or 1 (%d, %s). Please report this condition.\n", XDEPTH, icon_depth, g.winlist[g.maxNdx].name);
        g.winlist[g.maxNdx].icon_drawable = 
        g.winlist[g.maxNdx].icon_w =
        g.winlist[g.maxNdx].icon_h = 0;
    }

    // 3. other window data

    g.winlist[g.maxNdx].reclevel = reclevel;
    g.maxNdx++;
    if (g.debug>1) {fprintf (stderr, "window %d, id %x added to list\n", g.maxNdx, win);}
    return 1;
}

//
// sets g.winlist, g.maxNdx, g.selNdx
// n.b.: in heavy WM, use _NET_CLIENT_LIST:
// http://stackoverflow.com/questions/1201179/how-to-identify-top-level-x11-windows-using-xlib
// direction is direction of first press: with shift or without
//
int initWinlist (Display* dpy, Window root, bool direction)
{
int r;
switch (g.option_wm) {
  case WM_NO:
    r = x_initWindowsInfoRecursive (dpy, root, 0); // note: direction/current window index aren't used
    break;
  case WM_RATPOISON:
    r = rp_initWinlist (dpy, direction);
    break;
  case WM_EWMH:
    r = ewmh_initWinlist (dpy, direction);
    break;
  default:
    r = 0;
    break;
}
if (g.selNdx<0 || g.selNdx>=g.maxNdx) { g.selNdx=0; } // just for case
return r;
}

//
// counterpair for initWinlist
// frees icons and winlist, but not tiles, as they are allocated in gui.c
//
void freeWinlist (Display* dpy)
{
if (g.debug>0) {fprintf (stderr, "destroying icons and winlist\n");}
int y; for (y=0; y<g.maxNdx; y++) {
    if (g.winlist[y].icon_allocated)
        XFreePixmap (dpy, g.winlist[y].icon_drawable);
}
free (g.winlist);
}

//
// popup/focus this X window
//
int setFocus (Display* dpy, int winNdx)
{
int r;
switch (g.option_wm) {
  case WM_NO:
    r = ewmh_setFocus (dpy, winNdx); // for WM which isn't identified as EWMH compatible but accepts setting focus (dwm)
    r = x_setFocus (dpy, winNdx);
    break;
  case WM_RATPOISON:
    r = rp_setFocus (winNdx);
    break;
  case WM_EWMH:
    r = ewmh_setFocus (dpy, winNdx);
    // skippy-xd does this and notes that "order is important"
    // allow in trouble
    //XSetInputFocus (dpy, g.winlist[winNdx].id, RevertToParent, CurrentTime);
    break;
  default:
    return 0;
}
return r;
}

