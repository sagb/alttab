/*
Interface with EWMH-compatible window managers.

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
extern Display* dpy;
extern int scr;
extern Window root;

// PUBLIC

//
// return the name of EWMH-compatible WM
// or NULL if not found
//
char *ewmh_getWmName()
{

	Window *chld_win;
	char *r;
	Atom utf8string;

	chld_win = (Window *) NULL;
	if (!  (chld_win = (Window *) get_x_property(root, XA_WINDOW, "_NET_SUPPORTING_WM_CHECK", NULL))) {
		if (!  (chld_win = (Window *) get_x_property(root, XA_CARDINAL, "_WIN_SUPPORTING_WM_CHECK", NULL))) {
			return (char *)NULL;
		}
	}

	r = (char *)NULL;
	utf8string = XInternAtom(dpy, "UTF8_STRING", False);
	if (!
	    (r =
	     get_x_property(*chld_win, utf8string, "_NET_WM_NAME",
			    NULL))) {
		(r =
		 get_x_property(*chld_win, XA_STRING, "_NET_WM_NAME",
				NULL));
	}

	free(chld_win);
	return r;
}

//
// initialize winlist/startNdx
// return 1 if ok
//
int ewmh_initWinlist()
{
	Window *client_list;
	unsigned long client_list_size;
	int i;
	Window aw;
	char *awp;
	unsigned long sz;
	char *title;

	aw = (Window) 0;

	if ((awp = get_x_property(root, XA_WINDOW, "_NET_ACTIVE_WINDOW", &sz))) {
		aw = *((Window *) awp);
		free(awp);
	} else {
		if (g.debug > 0)
			fprintf(stderr, "can't obtain _NET_ACTIVE_WINDOW\n");
		// not mandatory
	}

	if ((client_list =
	     (Window *) get_x_property(root, XA_WINDOW, "_NET_CLIENT_LIST",
				       &client_list_size)) == NULL) {
		if ((client_list =
		     (Window *) get_x_property(root, XA_CARDINAL,
					       "_WIN_CLIENT_LIST",
					       &client_list_size)) == NULL) {
			fprintf(stderr, "can't get client list\n");
			return 0;
		}
	}

	for (i = 0; i < client_list_size / sizeof(Window); i++) {
		Window w = client_list[i];

		// build title
		char *wmn1 = get_x_property(w, XA_STRING, "WM_NAME", NULL);
		Atom utf8str = XInternAtom(dpy, "UTF8_STRING", False);
		char *wmn2 =
		    get_x_property(w, utf8str, "_NET_WM_NAME", NULL);
		title = wmn2 ? strdup(wmn2) : (wmn1 ? strdup(wmn1) : NULL);
		free(wmn1);
		free(wmn2);

		addWindowInfo(w, 0, 0, title);
		if (w == aw) {
			g.startNdx = i;
		}
	}

	if (g.debug > 1) {
		fprintf(stderr, "ewmh active window: %lu index: %d name: %s\n",
			aw, g.startNdx, g.winlist[g.startNdx].name);
	}

	return 1;
}

//
// focus window in EWMH WM
//
int ewmh_setFocus(int winNdx)
{
	Window win = g.winlist[winNdx].id;
	XEvent evt;
	long rn_mask = SubstructureRedirectMask | SubstructureNotifyMask;

	evt.xclient.window = win;
	evt.xclient.type = ClientMessage;
	evt.xclient.message_type =
	    XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	evt.xclient.serial = 0;
	evt.xclient.send_event = True;
	evt.xclient.format = 32;
	if (!XSendEvent(dpy, root, False, rn_mask, &evt)) {
		fprintf(stderr, "ewmh_activate_window: can't send xevent\n");
	}

	XMapRaised(dpy, win);
	return 1;
}
