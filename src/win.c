/*
Interface with foreign windows common for all WMs.

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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <X11/Xatom.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <utlist.h>
//#include <sys/time.h>
#include "alttab.h"
#include "util.h"
extern Globals g;
extern Display *dpy;
extern int scr;
extern Window root;

// PRIVATE

//
// helper for windows' qsort
// CAUSES O(log(N)) OR EVEN WORSE! reintroduce winlist[]->order instead?
//
static int sort_by_order(const void *p1, const void *p2)
{
    PermanentWindowInfo *s;
    WindowInfo *w1 = (WindowInfo*) p1;
    WindowInfo *w2 = (WindowInfo*) p2;
    int r = 0;

    DL_FOREACH(g.sortlist, s) {
        if (s->id == w1->id ) {
            r = (s->id == w2->id) ? 0 : -1;
            break;
        }
        if (s->id == w2->id ) {
            r = 1;
            break;
        }
    }
    //fprintf (stderr, "cmp 0x%lx <-> 0x%lx = %d\n", w1->id, w2->id, r);
    return r;
}

//
// add Window to the head/tail of sortlist, if it's not in sortlist already
// if move==true and the item is already in sortlist, then move it to the head/tail
//
void add_to_sortlist(Window w, bool to_head, bool move)
{
    PermanentWindowInfo *s;
    bool add = false;

    DL_SEARCH_SCALAR (g.sortlist, s, id, w);
    if (s == NULL) {
        s = malloc (sizeof (PermanentWindowInfo));
        if (s == NULL) return;
        s->id = w;
        add = true;
    } else {
        if (move) {
            DL_DELETE (g.sortlist, s);
            add = true;
        }
    }
    if (add) {
        if (to_head) {
            DL_PREPEND (g.sortlist, s);
        } else {
            DL_APPEND (g.sortlist, s);
        }
        // for delete notification
        XSelectInput(dpy, w, StructureNotifyMask);
    }
}

//
// debug output of sortlist
//
void print_sortlist()
{
    PermanentWindowInfo *s;
    fprintf (stderr, "sortlist:\n");
    DL_FOREACH (g.sortlist, s) {
        fprintf(stderr, "  0x%lx\n", s->id);
    }
}

//
// debug output of winlist
//
void print_winlist()
{
    int wi, si;
    PermanentWindowInfo *s;

    if (g.winlist == NULL && g.maxNdx == 0) return; // safety
    fprintf (stderr, "winlist:\n");
    for (wi = 0; wi < g.maxNdx; wi++) {
        si = 0;
        DL_FOREACH (g.sortlist, s) {
            if (s == NULL) continue; // safety
            if (s->id == g.winlist[wi].id )
                break;
            si++;
        }
        fprintf(stderr, "  %4d: 0x%lx  %s\n", si,
                g.winlist[wi].id, g.winlist[wi].name);
    }
}

// PUBLIC

//
// early initialization
// once per execution
//
int startupWintasks()
{
	g.sortlist = NULL;  // utlist head must be initialized to NULL
	g.ic = NULL;  // uthash too
	if (g.option_iconSrc != ISRC_RAM) {
		g.ic = initIcon();
		initIconHash(&(g.ic));
	}
    // watching for _NET_ACTIVE_WINDOW
    XSelectInput(dpy, root, PropertyChangeMask);
    g.naw = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", false);
    if (g.naw == None && g.debug > 0) {
        fprintf (stderr, "no _NET_ACTIVE_WINDOW atom, window focus will not be tracked\n");
    }
	switch (g.option_wm) {
	case WM_NO:
		return 1;
	case WM_RATPOISON:
		return rp_startupWintasks();
	case WM_EWMH:
		return 1;
    case WM_TWM:
        return 1;
	default:
		return 0;
	}
}

//
// search for icon in WM hints of "wi".
// if found, then
//   fill in "wi->icon_pixmap" and "wi->icon_mask"
//   and return 1,
// 0 otherwise.
//
int addIconFromHints(WindowInfo * wi)
{
	XWMHints *hints;
	Pixmap hicon, hmask;

	hicon = hmask = 0;
	if ((hints = XGetWMHints(dpy, wi->id))) {
		if (g.debug > 1) {
			fprintf(stderr,
				"IconPixmapHint: %ld, icon_pixmap: %lu, IconMaskHint: %ld, icon_mask: %lu, IconWindowHint: %ld, icon_window: %lu\n",
				hints->flags & IconPixmapHint,
				hints->icon_pixmap, hints->flags & IconMaskHint,
				hints->icon_mask, hints->flags & IconWindowHint,
				hints->icon_window);
		}
		if ((hints->flags & IconWindowHint) &
		    (!(hints->flags & IconPixmapHint))) {
			if (g.debug > 0)
				fprintf(stderr, "icon_window without icon_pixmap in hints, ignoring\n");	// not usable in xterm?
		}
		hicon =
		    (hints->flags & IconPixmapHint) ? hints->icon_pixmap : 0;
//            ((hints->flags & IconPixmapHint) ?  hints->icon_pixmap : (
//            (hints->flags & IconWindowHint) ?  hints->icon_window : 0));
		hmask = (hints->flags & IconMaskHint) ? hints->icon_mask : 0;
		XFree(hints);
		if (hicon && (g.debug > 0))
			fprintf(stderr, "no icon in WM hints (%s)\n", wi->name);
	} else {
		if (g.debug > 0) {
			fprintf(stderr, "no WM hints (%s)\n", wi->name);
		}
	}
	if (hmask != 0)
		wi->icon_mask = hmask;
	if (hicon != 0) {
		wi->icon_drawable = hicon;
		return 1;
	}
	return 0;
}

//
// search for "wi" application class in PNG hash.
// if found, then
//   if program options don't request size comparison 
//   OR png size match better, then
//     fill in "wi->icon_pixmap" and "wi->icon_mask"
//     and return 1
// return 0 otherwise.
// slow disk operations possible.
//
int addIconFromFiles(WindowInfo * wi)
{
	char *appclass, *tryclass;
	long unsigned int class_size;
	icon_t *ic;

	appclass = get_x_property(wi->id, XA_STRING, "WM_CLASS", &class_size);
	if (appclass) {
		for (tryclass = appclass; tryclass - appclass < class_size;
		     tryclass += (strlen(tryclass) + 1)) {
			ic = lookupIcon(tryclass);
			if (ic &&
			    (g.option_iconSrc != ISRC_SIZE
			     || iconMatchBetter(ic->src_w, ic->src_h,
						wi->icon_w, wi->icon_h))
			    ) {
				if (g.debug > 0)
					fprintf(stderr,
						"using png icon for %s\n",
						tryclass);
				if (ic->drawable == None) {
					if (g.debug > 1)
						fprintf(stderr,
							"loading content for %s\n",
							ic->app);
					if (loadIconContent(ic) == 0) {
						fprintf(stderr,
							"can't load png icon content\n");
						continue;
					}
				}
				wi->icon_drawable = ic->drawable;
				wi->icon_mask = 0;
				return 1;
			}
		}
	} else {
		if (g.debug > 0)
			fprintf(stderr, "can't find WM_CLASS for \"%s\"\n",
				wi->name);
	}
	return 0;
}

//
// add single window info into g.winlist and fix g.sortlist
// used by x, rp, ...
// only dpy and win are mandatory
//
int addWindowInfo(Window win, int reclevel, int wm_id, unsigned long desktop, char *wm_name)
{
	if (!
	    (g.winlist =
	     realloc(g.winlist, (g.maxNdx + 1) * sizeof(WindowInfo))))
		return 0;
	g.winlist[g.maxNdx].id = win;
	g.winlist[g.maxNdx].wm_id = wm_id;

// 1. get name

	if (wm_name) {
		strncpy(g.winlist[g.maxNdx].name, wm_name, MAXNAMESZ);
	} else {
		unsigned char *wn;
		Atom prop = XInternAtom(dpy, "WM_NAME", false), type;
		int form;
		unsigned long remain, len;
		if (XGetWindowProperty(dpy, win, prop, 0, MAXNAMESZ, false,
				       AnyPropertyType, &type, &form, &len,
				       &remain, &wn) == Success && wn) {
			strncpy(g.winlist[g.maxNdx].name, (char *)wn,
				MAXNAMESZ);
			g.winlist[g.maxNdx].name[MAXNAMESZ - 1] = '\0';
			XFree(wn);
		} else {
			g.winlist[g.maxNdx].name[0] = '\0';
		}
	}			// guessing name without WM hints

// 2. icon

// options:
// * WM_HINTS: https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/wm-hints.html
// * load icons from files.
// * use full windows as icons. https://www.talisman.org/~erlkonig/misc/x11-composite-tutorial/
//      it's more sophisticated than icon_drawable=win, because hidden window contents aren't available.
// * understand hints->icon_window (twm concept, xterm).

	g.winlist[g.maxNdx].icon_drawable =
	    g.winlist[g.maxNdx].icon_mask =
	    g.winlist[g.maxNdx].icon_w = g.winlist[g.maxNdx].icon_h = 0;
	unsigned int icon_depth = 0;
	g.winlist[g.maxNdx].icon_allocated = false;

	// search for icon in hints or file hash
	int icon_in_hints = 0;
	int opt = g.option_iconSrc;
	if (opt != ISRC_FILES)
		icon_in_hints = addIconFromHints(&(g.winlist[g.maxNdx]));
	if ((opt == ISRC_FALLBACK && !icon_in_hints) ||
	    opt == ISRC_SIZE || opt == ISRC_FILES)
		addIconFromFiles(&(g.winlist[g.maxNdx]));

	// extract icon width/height/depth
	Window root_return;
	int x_return, y_return;
	unsigned int border_width_return;
	if (g.winlist[g.maxNdx].icon_drawable) {
		if (XGetGeometry(dpy, g.winlist[g.maxNdx].icon_drawable,
				 &root_return, &x_return, &y_return,
				 &(g.winlist[g.maxNdx].icon_w),
				 &(g.winlist[g.maxNdx].icon_h),
				 &border_width_return, &icon_depth) == 0) {
			if (g.debug > 0) {
				fprintf(stderr,
					"icon dimensions unknown (%s)\n",
					g.winlist[g.maxNdx].name);
			}
			// probably draw placeholder?
			g.winlist[g.maxNdx].icon_drawable = 0;
		} else {
			if (g.debug > 1) {
				fprintf(stderr, "depth=%d\n", icon_depth);
			}
		}
	}
// convert icon with different depth (currently 1 only) into default depth
	if (g.winlist[g.maxNdx].icon_drawable && icon_depth == 1) {
		if (g.debug > 0) {
			fprintf(stderr,
				"rebuilding icon from depth 1 to %d (%s)\n",
				XDEPTH, g.winlist[g.maxNdx].name);
		}
		Pixmap pswap =
		    XCreatePixmap(dpy, g.winlist[g.maxNdx].icon_drawable,
				  g.winlist[g.maxNdx].icon_w,
				  g.winlist[g.maxNdx].icon_h, XDEPTH);
		if (!pswap)
			die("can't create pixmap");
		// GC should be already prepared in uiShow
		if (!XCopyPlane
		    (dpy, g.winlist[g.maxNdx].icon_drawable, pswap, g.gcDirect,
		     0, 0, g.winlist[g.maxNdx].icon_w,
		     g.winlist[g.maxNdx].icon_h, 0, 0, 1))
			die("can't copy plane");	// plane #1?
		g.winlist[g.maxNdx].icon_drawable = pswap;
		g.winlist[g.maxNdx].icon_allocated = true;	// for subsequent free()
		icon_depth = XDEPTH;
	}
	if (g.winlist[g.maxNdx].icon_drawable && icon_depth != XDEPTH) {
		fprintf(stderr,
			"Can't handle icon depth other than %d or 1 (%d, %s). Please report this condition.\n",
			XDEPTH, icon_depth, g.winlist[g.maxNdx].name);
		g.winlist[g.maxNdx].icon_drawable = g.winlist[g.maxNdx].icon_w =
		    g.winlist[g.maxNdx].icon_h = 0;
	}
// 3. sort

    add_to_sortlist (win, false, false);

// 4. other window data

	g.winlist[g.maxNdx].reclevel = reclevel;
	g.winlist[g.maxNdx].desktop = desktop;
	g.maxNdx++;
	if (g.debug > 1) {
		fprintf(stderr, "window %d, id %lx added to list\n", g.maxNdx,
			win);
	}
	return 1;
}				// addWindowInfo()

//
// sets g.winlist, g.maxNdx, g.selNdx
// updates g.sortlist, g.sortNdx
// n.b.: in heavy WM, use _NET_CLIENT_LIST
// direction is direction of first press: with shift or without
//
int initWinlist(bool direction)
{
	int r;
	if (g.debug > 1) {
		fprintf(stderr, "before initWinlist");
        print_sortlist();
	}
	g.startNdx = 0;		// safe default
	switch (g.option_wm) {
	case WM_NO:
		r = x_initWindowsInfoRecursive(root, 0);	// note: direction/current window index aren't used
		break;
	case WM_RATPOISON:
		r = rp_initWinlist();
		break;
	case WM_EWMH:
		r = ewmh_initWinlist();
		break;
    case WM_TWM:
        r = x_initWindowsInfoRecursive(root, 0);
        break;
	default:
		r = 0;
		break;
	}
	if (g.maxNdx > 1)
        add_to_sortlist (g.winlist[g.startNdx].id, true, true); // pull to head

// sort winlist according to .order
    if (g.debug > 1) {
        fprintf(stderr, "startNdx=%d\n", g.startNdx);
        fprintf(stderr, "before qsort\n");
        print_winlist();
    }
    qsort(g.winlist, g.maxNdx, sizeof(WindowInfo), sort_by_order);
    if (g.debug > 1) {
        fprintf(stderr, "after qsort\n");
        print_winlist();
    }

	g.startNdx = 0;		// former pointer invalidated by qsort, brought to top
	g.selNdx = direction ?
	    ((g.startNdx < 1
	      || g.startNdx >=
	      g.maxNdx) ? (g.maxNdx - 1) : (g.startNdx - 1)) : ((g.startNdx < 0
								 || g.startNdx
								 >=
								 (g.maxNdx -
								  1)) ? 0 :
								g.startNdx + 1);
//if (g.selNdx<0 || g.selNdx>=g.maxNdx) { g.selNdx=0; } // just for case
	if (g.debug > 1) {
		fprintf(stderr,
			"initWinlist ret: number of items in winlist: %d, current (selected) item in winlist: %d, current item at start of uiShow (current window before setFocus): %d\n",
			g.maxNdx, g.selNdx, g.startNdx);
	}

	return r;
}

//
// counterpair for initWinlist
// frees icons and winlist, but not tiles, as they are allocated in gui.c
//
void freeWinlist()
{
	if (g.debug > 0) {
		fprintf(stderr, "destroying icons and winlist\n");
	}
	if (g.debug > 1) {
		fprintf(stderr, "before freeWinlist\n");
        print_sortlist();
	}
	int y;
	for (y = 0; y < g.maxNdx; y++) {
		if (g.winlist[y].icon_allocated)
			XFreePixmap(dpy, g.winlist[y].icon_drawable);
	}
	free(g.winlist);
}

//
// popup/focus this X window
//
int setFocus(int winNdx)
{
	int r;
	switch (g.option_wm) {
	case WM_NO:
        r = ewmh_setFocus(winNdx, 0);  // for WM which isn't identified as EWMH compatible but accepts setting focus (dwm)
		x_setFocus(winNdx);
		break;
	case WM_RATPOISON:
		r = rp_setFocus(winNdx);
		break;
	case WM_EWMH:
        r = ewmh_setFocus(winNdx, 0);
        // XSetInputFocus stuff.
        // skippy-xd does it and notes that "order is important".
        // fixes #28.
        // it must be protected by testing IsViewable in the same way
        // as in x.c, or BadMatch happens after switching desktops.
        XWindowAttributes att;
        XGetWindowAttributes(dpy, g.winlist[winNdx].id, &att);
        if (att.map_state == IsViewable)
            XSetInputFocus(dpy, g.winlist[winNdx].id, RevertToParent, CurrentTime);
		break;
    case WM_TWM:
        r = ewmh_setFocus(winNdx, 0);
        x_setFocus(winNdx);
        break;
	default:
		return 0;
	}
    // pull to head
    add_to_sortlist (g.winlist[winNdx].id, true, true);
	return r;
}

//
// event handler for PropertyChange
// most of the time called when winlist[] is not initialized
// currently it updates sortlist on _NET_ACTIVE_WINDOW change
// because it's a handler of frequent event,
// there are shortcuts to return as soon as possible
//
void winPropChangeEvent(XPropertyEvent e)
// man XPropertyEvent
{
    Window aw;
    // no _NET_ACTIVE_WINDOW atom, probably not EWMH?
    if (g.naw == None) return;
    // property change event not for root window?
    if (e.window != root) return;
    // root property other than _NET_ACTIVE_WINDOW changed?
    if (e.atom != g.naw) return;
    // don't check for wm==EWMH, because _NET_ACTIVE_WINDOW changed for sure
    aw = ewmh_getActiveWindow();
    // can't get active window
    if (!aw) return;
    // focus changed to our own old/current/zero window?
    if (aw == getUiwin()) return;
    // focus changed to window which is already top?
    if (g.sortlist != NULL && aw == g.sortlist->id) return;
    // is window hidden in WM?
    if (ewmh_skipWindowInTaskbar(aw)) return;
/*
    // the i3 sortlist bug is not here, see ewmh.c init_winlist instead
    //
    if (aw == g.last.prev) {
        struct timeval ctv;
        int usec_delta;
        gettimeofday(&ctv, NULL);
        usec_delta = (ctv.tv_sec - g.last.tv.tv_sec) * 1E6 
          + (ctv.tv_usec - g.last.tv.tv_usec);
        fprintf (stderr, "delta %d\n", usec_delta);
        if (usec_delta < 5E5) {  // half a second
            return;
        }
        fprintf (stderr, PREF"pulling 'prev' 0x%lx supressed\n", aw);
    }
    if (aw == g.last.to) {
        fprintf (stderr, PREF"pulling 'to' 0x%lx supressed\n", aw);
        return;
    }
*/
    // finally, add/pop window to the head of sortlist
    // unfortunately, on focus by alttab, this is fired twice:
    // 1) when alttab window gone, previous window becomes active,
    // 2) then alttab-focused window becomes active.
    if (g.debug>0)
        fprintf (stderr, 
            "wm new active window 0x%lx, pull to the head of sortlist\n",
            aw);
    add_to_sortlist (aw, true, true);
}

//
// DestroyNotify handler
// removes the window from sortlist
//
void winDestroyEvent(XDestroyWindowEvent e)
// man XDestroyWindowEvent
{
    PermanentWindowInfo *s;

    DL_SEARCH_SCALAR (g.sortlist, s, id, e.window);
    if (s != NULL) {
        if (g.debug>1) {
            fprintf (stderr, "0x%lx found in sortlist, removing\n", e.window);
        }
        DL_DELETE (g.sortlist, s);
    }
}

//
// common filter before adding window to winlist
// it checks: desktop, screen
// depending on global options and dimensions
//
bool common_skipWindow(Window w, 
    unsigned long current_desktop, unsigned long window_desktop)
{
    quad wq; // window's absolute coordinates

    if (g.option_desktop == DESK_CURRENT
            && current_desktop != window_desktop 
            && current_desktop != DESKTOP_UNKNOWN 
            && window_desktop != DESKTOP_UNKNOWN) {
        if (g.debug > 1) {
            fprintf (stderr, 
                    "window not on active desktop, skipped (window's %ld, current %ld)\n", 
                    window_desktop, current_desktop);
        }
        return true;
    }
    if (g.option_desktop == DESK_NOSPECIAL
            && window_desktop == -1) {
        if (g.debug > 1) {
            fprintf (stderr, "window on -1 desktop, skipped\n");
        }
        return true;
    }
    if (g.option_desktop == DESK_NOCURRENT && 
            (window_desktop == current_desktop ||
             window_desktop == -1)) {
        if (g.debug > 1) {
            fprintf (stderr, "window on current or -1 desktop, skipped\n");
        }
        return true;
    }

    if (g.option_screen == SCR_CURRENT) {
        if (! get_absolute_coordinates(w, &wq)) {
            fprintf (stderr, 
                    "can't get coordinates of window 0x%lx, included anyway\n", w);
        } else {
            if (! rectangles_cross(g.vp, wq)) {
                if (g.debug > 1) {
                    fprintf (stderr, 
                            "window's area doesn't cross with current screen, skipped\n");
                }
                return true;
            }
        }
    }

    return false;
}

