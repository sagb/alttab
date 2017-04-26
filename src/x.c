/*
Interface with foreign windows in raw X11.

Copyright 2017 Alexander Kulak.
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
//#include <stdlib.h>
//#include <string.h>
#include "alttab.h"
extern Globals g;

// PRIVATE

//
// get window group leader
// copied from https://github.com/robx/skippy-xd/blob/master/wm.c
//
Window wm_get_group_leader(Display *dpy, Window window)
{
unsigned char *data;
int status, real_format;
Atom real_type;
unsigned long items_read, items_left;
Window leader = None;
Atom WM_CLIENT_LEADER;
WM_CLIENT_LEADER = XInternAtom(dpy, "WM_CLIENT_LEADER", 0);
status = XGetWindowProperty(dpy, window, WM_CLIENT_LEADER,
                  0, 1, False, XA_WINDOW, &real_type, &real_format,
                  &items_read, &items_left, &data);
if(status != Success)
{
    XWMHints *hints = XGetWMHints(dpy, window);
    if(! hints)
        return None;
    if(hints->flags & WindowGroupHint)
        leader = hints->window_group;
    return leader;
}
if(items_read)
    leader = ((Window*)data)[0];
XFree(data);
return leader;
}

// PUBLIC

//
// set winlist,maxNdx,startNdx recursively using raw Xlib
// first call should be (win=root, reclevel=0)
//
int x_initWindowsInfoRecursive (Display* dpy,Window win,int reclevel) {

Window root, parent;
Window* children;
unsigned int nchildren, i;
//Window leader;
XWindowAttributes wa;

// check if window is "leader" or no prop, skip otherwise
// caveat: in rp, gvim leader has no file name and icon
// this doesn't work in raw X: leader may be not viewable.
/*
leader = wm_get_group_leader (dpy, win);
if (g.debug>1) {fprintf (stderr, "win: %x leader: %x\n", win, leader);}
*/
// add viewable only
// caveat: in rp, skips anything except of visible window
// TODO: add an option for this in WMs too
XGetWindowAttributes (dpy, win, &wa);

// insert detailed window data in window list
//if ( ((!leader) || leader==win)  &&
//if ( (leader==win)  &&
if (
        wa.map_state == IsViewable  &&
        reclevel != 0 ) {
    addWindowInfo (dpy, win, reclevel, 0, NULL);
}

// skip children if max recursion level reached
if (g.option_max_reclevel != -1 && reclevel >= g.option_max_reclevel)
    return 1;

// recursion
if (XQueryTree (dpy, win, &root, &parent, &children, &nchildren)==0) {
    if (g.debug>0) {fprintf (stderr, "can't get window tree for %lu\n", win);}
    return 0;
}
for (i = 0; i < nchildren; ++i) {
    x_initWindowsInfoRecursive (dpy, children[i], reclevel+1);
}

if (nchildren>0 && children) {XFree(children);}
if (reclevel==0) { g.startNdx=0; }
return 1;
}

//
// set window focus in raw X
//
int x_setFocus (Display* dpy, int wndx)
{
Window w = g.winlist[wndx].id;

// TODO: 1. XWarpPointer

// 2. XRaiseWindow required, but doesn't make window "Viewable"
XRaiseWindow (dpy, w);

// 3. XSetInputFocus
// "The specified focus window must be viewable at the time 
// XSetInputFocus is called, or a BadMatch error results."
// This check is redundant: non-viewable windows isn't added to winlist in raw X anyway
XWindowAttributes att;
XGetWindowAttributes (dpy, w, &att);
if (att.map_state == IsViewable )
    XSetInputFocus (dpy, w, RevertToParent, CurrentTime);

return 1;
}

