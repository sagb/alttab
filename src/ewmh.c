/*
Interface with EWMH-compatible window managers.
Note: _WIN fallbacks are not part of EWMH or ICCCM, but kept here anyway.

Copyright 2017-2018 Alexander Kulak.
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

// PRIVATE

//
// returns ptr to EWMH client list and client_list_size
// or NULL
//
Window *ewmh_get_client_list(unsigned long *client_list_size)
{
    Window *client_list;

	if (g.ewmh.try_stacking_list_first) {
        if ((client_list =
	     (Window *) get_x_property(root, XA_WINDOW, "_NET_CLIENT_LIST_STACKING",
				       client_list_size)) != NULL) {
            if (g.debug>1) {
                fprintf(stderr, "ewmh found stacking window list\n");
            }
            return client_list;
        } else {
            g.ewmh.try_stacking_list_first = false;
        }
    }
	if ((client_list =
	     (Window *) get_x_property(root, XA_WINDOW, "_NET_CLIENT_LIST",
			       client_list_size)) != NULL)
        return client_list;
    if ((client_list =
	     (Window *) get_x_property(root, XA_CARDINAL, "_WIN_CLIENT_LIST",
			       client_list_size)) != NULL)
		return client_list;
    return 0;
}

int ewmh_send_wm_evt(Window w, char *atom, unsigned long edata[])
{
    XEvent evt;
    long rn_mask = SubstructureRedirectMask | SubstructureNotifyMask;
    evt.xclient.window = w;
    evt.xclient.type = ClientMessage;
    evt.xclient.message_type =
        XInternAtom(dpy, atom, False);
    evt.xclient.serial = 0;
    evt.xclient.send_event = True;
    evt.xclient.format = 32;
    //memset (&(evt.xclient.data.l[1]), 0, 4*sizeof(evt.xclient.data.l[1]));
    int ei; for (ei = 0; ei < 5; ei++)
        evt.xclient.data.l[ei] = edata[ei];
    if (!XSendEvent(dpy, root, False, rn_mask, &evt)) {
        fprintf(stderr, "can't send %s xevent\n", atom);
        return 0;
    }
    return 1;
}

int ewmh_switch_desktop(unsigned long desktop)
{
    int evr, elapsed;
	unsigned long edata[] = {desktop, CurrentTime, 0,0,0};
    if (desktop == -1 && g.ewmh.minus1_desktop_unusable)
        return 0;
    if (g.debug>1) {
        fprintf(stderr, "ewmh switching desktop to %ld\n", desktop);
    }
    evr = ewmh_send_wm_evt(root, "_NET_CURRENT_DESKTOP", edata);
    if (evr == 0)
        return 0;
    // wait for WM (#45)
#define WM_POLL_TIMEOUT   200000  // 200 ms
#define WM_POLL_INTERVAL   10000
    for (elapsed = 0;
            elapsed < WM_POLL_TIMEOUT
            && ewmh_getCurrentDesktop() != desktop;
            elapsed += WM_POLL_INTERVAL) {
        if (g.debug>1) {
            fprintf(stderr, "usleep %d\n", WM_POLL_INTERVAL);
        }
        usleep(WM_POLL_INTERVAL);
    }
    return evr;
}

int ewmh_switch_window(unsigned long window)
{
    unsigned long edata[] = {2, CurrentTime, 0,0,0};
    if (g.debug>1) {
        fprintf(stderr, "ewmh switching window to 0x%lx\n", window);
    }
    return
        ewmh_send_wm_evt(window, "_NET_ACTIVE_WINDOW", edata);
}

// PUBLIC

//
// initialize EwmhFeatures
// return true if usable at all
//
bool ewmh_detectFeatures(EwmhFeatures *e)
{
	Window *chld_win;
	char *r;
	Atom utf8string;
    char *default_wm_name = "unknown_ewmh_compatible";
    unsigned long client_list_size;

    // This function is used in alttab for detection of EWMH compatibility.
    // But there are WM (dwm) that support EWMH subset required for alttab
    // except of _NET_SUPPORTING_WM_CHECK/_WIN_SUPPORTING_WM_CHECK detection.
    // We detect those WM as EWMH-compatible and return their name
    // as "unknown_ewmh_compatible".

    bzero (e, sizeof(EwmhFeatures));
    e->try_stacking_list_first = true;

    // first, detect necessary feature: client list
    // also, this resets try_stacking_list_first if necessary
    if (ewmh_get_client_list(&client_list_size) == NULL) {
        // WM is not usable in EWMH mode
        return false;
    }

    // then, guess/devise WM name
	chld_win = (Window *) NULL;
	if (!  (chld_win = (Window *) get_x_property(root, XA_WINDOW, "_NET_SUPPORTING_WM_CHECK", NULL))) {
		if (!  (chld_win = (Window *) get_x_property(root, XA_CARDINAL, "_WIN_SUPPORTING_WM_CHECK", NULL))) {
			e->wmname = default_wm_name;
            return true;
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
	if (chld_win!=NULL)
        free(chld_win);

    e->wmname = (r!=NULL) ? r : default_wm_name;

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
		if (g.debug > 0)
			fprintf(stderr, "can't obtain _NET_ACTIVE_WINDOW\n");
	}
    return w;
}

//
// initialize winlist/startNdx
// return 1 if ok
//
int ewmh_initWinlist(quad screen)
{
	Window *client_list;
	unsigned long client_list_size;
	int i;
	Window aw;
	char *title;
    unsigned long current_desktop, window_desktop;
    quad wq; // window's absolute coordinates

    current_desktop = ewmh_getCurrentDesktop();

    aw = ewmh_getActiveWindow();
	if (!aw && g.debug > 0) {
        fprintf(stderr, "can't obtain _NET_ACTIVE_WINDOW\n");
        // continue anyway
    }

    if ((client_list = ewmh_get_client_list(&client_list_size)) == NULL) {
        fprintf(stderr, "can't get client list\n");
        return 0;
    }

	for (i = 0; i < client_list_size / sizeof(Window); i++) {
		Window w = client_list[i];

        if (ewmh_skipWindowInTaskbar(w)) {
	        if (g.debug > 1) {
                fprintf (stderr, "window %lx has \"skip on taskbar\" property, skipped\n", w);
            }
            continue;
        }

        window_desktop = ewmh_getDesktopOfWindow(w);
        if (g.option_desktop == DESK_CURRENT
                && current_desktop != window_desktop 
                && current_desktop != DESKTOP_UNKNOWN 
                && window_desktop != DESKTOP_UNKNOWN) {
	        if (g.debug > 1) {
                fprintf (stderr, 
                  "window not on active desktop, skipped (window's %ld, current %ld)\n", 
                  window_desktop, current_desktop);
            }
            continue;
        }
        if (g.option_desktop == DESK_NOSPECIAL
                && window_desktop == -1) {
	        if (g.debug > 1) {
                fprintf (stderr, "window on -1 desktop, skipped\n");
            }
            continue;
        }
        if (g.option_desktop == DESK_NOCURRENT && 
                (window_desktop == current_desktop ||
                 window_desktop == -1)) {
	        if (g.debug > 1) {
                fprintf (stderr, "window on current or -1 desktop, skipped\n");
            }
            continue;
        }

        if (g.option_screen == SCR_CURRENT) {
            if (! get_absolute_coordinates(w, &wq)) {
                fprintf (stderr, 
                  "can't get coordinates of window 0x%lx, included anyway\n", w);
            } else {
                if (! rectangles_cross(screen, wq)) {
                    if (g.debug > 1) {
                        fprintf (stderr, 
                          "window's area doesn't cross with current screen, skipped\n");
                    }
                    continue;
                }
            }
        }

		// build title
		char *wmn1 = get_x_property(w, XA_STRING, "WM_NAME", NULL);
		Atom utf8str = XInternAtom(dpy, "UTF8_STRING", False);
		char *wmn2 =
		    get_x_property(w, utf8str, "_NET_WM_NAME", NULL);
		title = wmn2 ? strdup(wmn2) : (wmn1 ? strdup(wmn1) : NULL);
		free(wmn1);
		free(wmn2);

		addWindowInfo(w, 0, 0, window_desktop, title);
		if (w == aw) {
			g.startNdx = g.maxNdx-1;
		}
	}

    // TODO: BUG? sometimes i3 returns previous active window,
    // which breaks sortlist
	if (g.debug > 1) {
		fprintf(stderr, "ewmh active window: %lu index: %d name: %s\n",
			aw, g.startNdx, (g.winlist ? g.winlist[g.startNdx].name : "null"));
	}

	return 1;
}

//
// focus window in EWMH WM
// fwin used if non-zero, winNdx otherwise
//
int ewmh_setFocus(int winNdx, Window fwin)
{
    Window win = (fwin != 0) ? fwin : g.winlist[winNdx].id;
    if (g.debug>1) {
        fprintf(stderr, "ewmh_setFocus win 0x%lx\n", win);
    }
    if (fwin == 0 && g.option_desktop != DESK_CURRENT) {
        unsigned long wdesk = g.winlist[winNdx].desktop;
        unsigned long cdesk = ewmh_getCurrentDesktop();
        if (g.debug>1) {
            fprintf(stderr, "ewmh_setFocus fwin 0x%lx opt %d wdesk %lu cdesk %lu\n",
                   fwin, g.option_desktop, wdesk, cdesk);
        }
        if (cdesk != wdesk && wdesk != DESKTOP_UNKNOWN) {
            ewmh_switch_desktop(wdesk);
        }
    }
	ewmh_switch_window(win);
	XMapRaised(dpy, win);
	return 1;
}

//
// get current desktop in EWMH WM
//
unsigned long ewmh_getCurrentDesktop()
{
    unsigned long *cd;
    unsigned long propsize;
    cd = (unsigned long*)get_x_property (root, XA_CARDINAL,
            "_NET_CURRENT_DESKTOP", &propsize);
    if (!cd)
        cd = (unsigned long*)get_x_property (root, XA_CARDINAL,
                "_WIN_WORKSPACE", &propsize);
    return (cd && (propsize>0)) ? *cd : DESKTOP_UNKNOWN;
}

//
// get desktop of window w in EWMH WM
//
unsigned long ewmh_getDesktopOfWindow(Window w)
{
    unsigned long *d;
    unsigned long propsize;
    d = (unsigned long*)get_x_property (w, XA_CARDINAL,
            "_NET_WM_DESKTOP", &propsize);
    if (!d)
        d = (unsigned long*)get_x_property (w, XA_CARDINAL,
                "_WIN_WORKSPACE", &propsize);
    return (d && (propsize>0)) ? *d : DESKTOP_UNKNOWN;
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

    state = (Atom*)get_x_property(w, XA_ATOM, "_NET_WM_STATE", &state_propsize);
    if (state == NULL || state_propsize == 0) {
        if (g.debug>1) {
            fprintf (stderr, "%lx: no _NET_WM_STATE property\n", w);
        }
        return false;
    }
    a_skip_tb = XInternAtom(dpy, "_NET_WM_STATE_SKIP_TASKBAR", True);
	for (i = 0; i < state_propsize / sizeof(Atom); i++) {
        if (state[i] == a_skip_tb) {
            if (g.debug>1) {
                fprintf (stderr, "%lx: _NET_WM_STATE_SKIP_TASKBAR found\n", w);
            }
            return true;
        }
    }
    return false;
}

