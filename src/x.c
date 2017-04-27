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
#include "alttab.h"
#include "util.h"
extern Globals g;

// PRIVATE

//
// get window group leader
// to be used later. rewritten, not tested.
//
Window x_get_leader(Display * dpy, Window win)
{
	Window *retprop;
	XWMHints *h;
	Window leader = None;

	retprop =
	    (Window *) get_x_property(dpy, win, XA_WINDOW, "WM_CLIENT_LEADER",
				      NULL);
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

// PUBLIC

//
// set winlist,maxNdx,startNdx recursively using raw Xlib
// first call should be (win=root, reclevel=0)
//
int x_initWindowsInfoRecursive(Display * dpy, Window win, int reclevel)
{

	Window root, parent;
	Window *children;
	unsigned int nchildren, i;
//Window leader;
	XWindowAttributes wa;

// check if window is "leader" or no prop, skip otherwise
// caveat: in rp, gvim leader has no file name and icon
// this doesn't work in raw X: leader may be not viewable.
/*
leader = x_get_leader (dpy, win);
if (g.debug>1) {fprintf (stderr, "win: %x leader: %x\n", win, leader);}
*/
// add viewable only
// caveat: in rp, skips anything except of visible window
// probably add an option for this in WMs too?
	XGetWindowAttributes(dpy, win, &wa);

// insert detailed window data in window list
//if ( ((!leader) || leader==win)  &&
//if ( (leader==win)  &&
	if (wa.map_state == IsViewable && reclevel != 0) {
		addWindowInfo(dpy, win, reclevel, 0, NULL);
	}
// skip children if max recursion level reached
	if (g.option_max_reclevel != -1 && reclevel >= g.option_max_reclevel)
		return 1;

// recursion
	if (XQueryTree(dpy, win, &root, &parent, &children, &nchildren) == 0) {
		if (g.debug > 0) {
			fprintf(stderr, "can't get window tree for %lu\n", win);
		}
		return 0;
	}
	for (i = 0; i < nchildren; ++i) {
		x_initWindowsInfoRecursive(dpy, children[i], reclevel + 1);
	}

	if (nchildren > 0 && children) {
		XFree(children);
	}
	if (reclevel == 0) {
		g.startNdx = 0;
	}
	return 1;
}

//
// set window focus in raw X
//
int x_setFocus(Display * dpy, int wndx)
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
