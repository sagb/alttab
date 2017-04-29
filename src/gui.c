/*
Draw and interface with our switcher window.

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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "alttab.h"
#include "util.h"
extern Globals g;

// PRIVATE

unsigned int tileW, tileH, iconW, iconH;
int scrNum;
int scrW, scrH;
Window uiwin;
int uiwinW, uiwinH, uiwinX, uiwinY;
Colormap colormap;
Visual *visual;
//Font fontLabel;  // Xft instead
XftFont *fontLabel;

//
// allocates GC
// type is: 
//   0: normal
//   1: for bg fill
//   2: for drawing frame
//
GC create_gc(Display * display, int screen_num, Window win, int type)
{
	GC gc;			/* handle of newly created GC.  */
	unsigned long valuemask = 0;	/* which values in 'values' to  */
	/* check when creating the GC.  */
	XGCValues values;	/* initial values for the GC.   */
	int line_style = LineSolid;
	int cap_style = CapButt;
	int join_style = JoinMiter;

	gc = XCreateGC(display, win, valuemask, &values);
	if (gc < 0) {
		fprintf(stderr, "can't create GC\n");
		return 0;
	}
	/* allocate foreground and background colors for this GC. */
	switch (type) {
	case 1:
		XSetForeground(display, gc, g.color[COLBG].xcolor.pixel);
		XSetBackground(display, gc, g.color[COLFG].xcolor.pixel);
		XSetLineAttributes(display, gc, FRAME_W, line_style, cap_style,
				   join_style);
		break;
	case 0:
		XSetForeground(display, gc, g.color[COLFG].xcolor.pixel);
		XSetBackground(display, gc, g.color[COLBG].xcolor.pixel);
		XSetLineAttributes(display, gc, 1, line_style, cap_style,
				   join_style);
		break;
	case 2:
		XSetForeground(display, gc, g.color[COLFRAME].xcolor.pixel);
		XSetBackground(display, gc, g.color[COLBG].xcolor.pixel);
		XSetLineAttributes(display, gc, FRAME_W, line_style, cap_style,
				   join_style);
		break;
	default:
		fprintf(stderr, "unknown GC type, not setting colors\n");
		break;
	}
	/* define the fill style for the GC. to be 'solid filling'. */
	XSetFillStyle(display, gc, FillSolid);
	return gc;
}

//
// single use helper for function below
//
void drawFr(Display * dpy, GC gc, int f)
{
	int d = XDrawRectangle(dpy, uiwin, gc,
			       f * (tileW + FRAME_W) + (FRAME_W / 2),
			       0 + (FRAME_W / 2),
			       tileW + FRAME_W, tileH + FRAME_W);
	if (!d) {
		fprintf(stderr, "can't draw frame\n");
	}
}

//
// draw selected and unselected frames around tiles
//
void framesRedraw(Display * dpy)
{
	int f;
	for (f = 0; f < g.maxNdx; f++) {
		if (f == g.selNdx)
			continue;	// skip
		drawFr(dpy, g.gcReverse, f);	// thick bg
		drawFr(dpy, g.gcDirect, f);	// thin frame
	}
// _after_ unselected draw selected, because they may overlap
	drawFr(dpy, g.gcFrame, g.selNdx);
}

// PUBLIC

//
// early initialization
// called once per execution
// TODO: counterpair for freeing X resources, 
//   even if called once per execution:
/*
int p; for (p=0; p<NCOLORS; p++) {
        XftColorFree (dpy,DefaultVisual(dpy,0),DefaultColormap(dpy,0),g.color[p].xftcolor);
            // XFreeColors ?
}
if (g.gcDirect) XFreeGC (dpy, g.gcDirect);
if (g.gcReverse) XFreeGC (dpy, g.gcReverse);
if (g.gcFrame) XFreeGC (dpy, g.gcFrame);
*/
int startupGUItasks(Display * dpy, Window root)
{

	scrNum = DefaultScreen(dpy);
	scrW = DisplayWidth(dpy, scrNum);
	scrH = DisplayHeight(dpy, scrNum);

// colors
	colormap = DefaultColormap(dpy, scrNum);
	visual = DefaultVisual(dpy, scrNum);
	if (g.debug > 0) {
		fprintf(stderr, "early allocating colors\n");
	}
	srand(time(NULL));
	int p;
	for (p = 0; p < NCOLORS; p++) {
		if (g.color[p].name[0]) {
			if (strncmp(g.color[p].name, "_rnd_", 5) == 0) {
				// replace in-place: 8 chars is sufficient for #rrggbb
				char r[3];
				short int rc;
				for (rc = 0; rc <= 2; rc++) {
					r[rc] = rand() / (RAND_MAX / 0x80);
				}
				if (strncmp(g.color[p].name, "_rnd_low", 8) ==
				    0) {
					(void)snprintf(g.color[p].name, 8,
						       "#%.2hhx%.2hhx%.2hhx",
						       r[0], r[1], r[2]);
					g.color[p].name[7] = '\0';
				} else
				    if (strncmp(g.color[p].name, "_rnd_high", 9)
					== 0) {
					(void)snprintf(g.color[p].name, 9,
						       "#%.2hhx%.2hhx%.2hhx",
						       r[0] + 0x80, r[0] + 0x80,
						       r[1] + 0x80);
					g.color[p].name[7] = '\0';
				}
				if (g.debug > 1)
					fprintf(stderr,
						"color generated: %s, RAND_MAX=%d\n",
						g.color[p].name, RAND_MAX);
			}
			if (!XAllocNamedColor(dpy,
					      colormap,
					      g.color[p].name,
					      &(g.color[p].xcolor),
					      &(g.color[p].xcolor)))
				die2("failed to allocate X color: ",
				     g.color[p].name);
			if (!XftColorAllocName
			    (dpy, visual, colormap, g.color[p].name,
			     &(g.color[p].xftcolor)))
				die2("failed to allocate Xft color: ",
				     g.color[p].name);
		}
	}

	if (g.debug > 0) {
		fprintf(stderr, "early opening font\n");
	}
//fontLabel = XLoadFont (dpy, LABELFONT);  // using Xft instead
	fontLabel = XftFontOpenName(dpy, scrNum, g.option_font);
	if (!fontLabel) {
		fprintf(stderr, "can't allocate font: %s", g.option_font);
	}
// having colors, GC may be built
// they are required early for addWindow when transforming icon depth
	if (g.debug > 0) {
		fprintf(stderr, "early building GCs\n");
	}
	g.gcDirect = create_gc(dpy, scrNum, root, 0);
	g.gcReverse = create_gc(dpy, scrNum, root, 1);
	g.gcFrame = create_gc(dpy, scrNum, root, 2);

	return 1;
}

//
// called on alt-tab keypress to draw popup
// build g.winlist
// create our window
// sets g.uiShowHasRun (if set, then call uiHide to free X stuff)
// returns 1 if our window is ready to Expose, 0 otherwise
// direction is direction of first press: with shift or without
//
int uiShow(Display * dpy, Window root, bool direction)
{
	if (g.debug > 0) {
		fprintf(stderr, "preparing ui\n");
	}
// init X junk early, because depth conversion in addWindow requires GC

	g.uiShowHasRun = true;	// begin allocations

// GC initialized earlier, now able to init winlist

	g.winlist = NULL;
	g.maxNdx = 0;
	if (!initWinlist(dpy, root, direction)) {
		if (g.debug > 0) {
			fprintf(stderr,
				"initWinlist failed, skipping ui initialization\n");
		}
		g.winlist = NULL;
		g.maxNdx = 0;
		return 0;
	}
	if (!g.winlist) {
		if (g.debug > 0) {
			fprintf(stderr,
				"winlist doesn't exist, skipping ui initialization\n");
		}
		return 0;
	}
	if (g.maxNdx < 1) {
		if (g.debug > 0) {
			fprintf(stderr,
				"number of windows < 1, skipping ui initialization\n");
		}
		return 0;
	}
	if (g.debug > 0) {
		fprintf(stderr, "got %d windows\n", g.maxNdx);
		int i;
		for (i = 0; i < g.maxNdx; i++) {
			fprintf(stderr,
				"%d: %lx (lvl %d, icon %lu (%dx%d)): %s\n", i,
				g.winlist[i].id, g.winlist[i].reclevel,
				g.winlist[i].icon_drawable, g.winlist[i].icon_w,
				g.winlist[i].icon_h, g.winlist[i].name);
		}
	}
// have winlist, now back to uiwin stuff
// calculate dimensions
	tileW = g.option_tileW, tileH = g.option_tileH;
	iconW = g.option_iconW;
	iconH = g.option_iconH;
	float rt = 1.0;
// tiles may be smaller if they don't fit screen
	uiwinW = (tileW + FRAME_W) * g.maxNdx + FRAME_W;
	if (uiwinW > scrW) {
		rt = (float)scrW / (float)uiwinW;
		uiwinW = scrW;
		tileW = tileW * rt;
		tileH = tileH * rt;
	}
// icon may be smaller if it doesn't fit tile
	if (iconW > tileW) {
		rt = (float)tileW / (float)iconW;
		iconW = tileW;
		iconH = rt * iconH;
	}
	if (iconH > tileH) {
		rt = (float)tileH / (float)iconH;
		iconH = tileH;
		iconW = rt * iconW;
	}
	uiwinH = tileH + 2 * FRAME_W;
	uiwinX = (scrW - uiwinW) / 2;
	uiwinY = (scrH - uiwinH) / 2;
	if (g.debug > 0) {
		fprintf(stderr, "tile w=%d h=%d win w=%d h=%d\n", tileW, tileH,
			uiwinW, uiwinH);
	}
// prepare tiles

	int m;
	for (m = 0; m < g.maxNdx; m++) {
		if (!g.winlist)
			die("no winlist in uiShow. this shouldn't happen, please report.");
		g.winlist[m].tile =
		    XCreatePixmap(dpy, root, tileW, tileH, XDEPTH);
		if (!g.winlist[m].tile)
			die("can't create tile");
		int fr =
		    XFillRectangle(dpy, g.winlist[m].tile, g.gcReverse, 0, 0,
				   tileW, tileH);
		if (!fr) {
			fprintf(stderr, "can't fill tile\n");
		}
		// place icons
		if (g.winlist[m].icon_drawable) {
			if (g.winlist[m].icon_w == iconW &&
			    g.winlist[m].icon_h == iconH) {
				// direct copy
				// TODO: preserve x/y ratio instead of scaling (xcalc)
				if (g.debug > 1) {
					fprintf(stderr, "%d: copying icon\n",
						m);
				}
				int or = XCopyArea(dpy,
						   g.winlist[m].icon_drawable,
						   g.winlist[m].tile,
						   g.gcDirect, 0, 0,
						   g.winlist[m].icon_w, g.winlist[m].icon_h,	// src
						   0, 0);	// dst
				if (!or) {
					fprintf(stderr,
						"can't copy icon to tile\n");
				}
			} else {
				// scale
				if (g.debug > 1) {
					fprintf(stderr, "%d: scaling icon\n",
						m);
				}
				int sc = pixmapScale(dpy, scrNum, root,
						     g.winlist[m].icon_drawable,
						     g.winlist[m].tile,
						     g.winlist[m].icon_w,
						     g.winlist[m].icon_h,
						     iconW, iconH);
				if (!sc) {
					fprintf(stderr,
						"can't scale icon to tile\n");
				}
			}
		} else {
			// draw placeholder or standalone icons from some WM
			GC gcL = create_gc(dpy, scrNum, root, 0);	// GC for thin line
			if (!gcL) {
				fprintf(stderr, "can't create gcL\n");
			} else {
				XSetLineAttributes(dpy, gcL, 1, LineSolid,
						   CapButt, JoinMiter);
				//XSetForeground (dpy, gcL, pixel);
				int pr =
				    XDrawRectangle(dpy, g.winlist[m].tile, gcL,
						   0, 0, iconW, iconH);
				if (!pr) {
					fprintf(stderr,
						"can't draw placeholder\n");
				}
				XFreeGC(dpy, gcL);
			}
		}
		// draw labels
		if (g.winlist[m].name && fontLabel) {
			int dr =
			    drawMultiLine(dpy, g.winlist[m].tile, fontLabel,
					  &(g.color[COLFG].xftcolor),
					  g.winlist[m].name,
					  0, (iconH + 5), tileW,
					  (tileH - iconH - 5));
			if (dr != 1) {
				fprintf(stderr, "can't draw label\n");
			}
		}
	}
	if (g.debug > 0) {
		fprintf(stderr, "prepared %d tiles\n", m);
	}
	if (fontLabel)
		XftFontClose(dpy, fontLabel);

// prepare our window
	uiwin = XCreateSimpleWindow(dpy, root,
				    uiwinX, uiwinY,
				    uiwinW, uiwinH,
				    0, g.color[COLFRAME].xcolor.pixel,
				    g.color[COLBG].xcolor.pixel);
	if (uiwin <= 0)
		die("can't create window");
	if (g.debug > 0) {
		fprintf(stderr, "our window is %lu\n", uiwin);
	}
// set properties of our window
	XStoreName(dpy, uiwin, XWINNAME);
	XSelectInput(dpy, uiwin, ExposureMask | KeyPressMask | KeyReleaseMask);
// set window type so that WM will hopefully not resize it
// before mapping: https://specifications.freedesktop.org/wm-spec/1.3/ar01s05.html
	Atom at = XInternAtom(dpy, "ATOM", True);
	Atom wt = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	Atom td = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	if (at && wt && td)
		XChangeProperty(dpy, uiwin, wt, at, 32, PropModeReplace,
				(unsigned char *)(&td), 1);
// xmonad ignores _NET_WM_WINDOW_TYPE_DIALOG but obeys WM_TRANSIENT_FOR
	XSetTransientForHint(dpy, uiwin, uiwin);

	XMapWindow(dpy, uiwin);
	return 1;
}

//
// Expose event callback
// redraw our window
//
void uiExpose(Display * dpy, Window root)
{
	if (g.debug > 0) {
		fprintf(stderr, "expose ui\n");
	}
// icons
	int j;
	for (j = 0; j < g.maxNdx; j++) {
		if (g.winlist[j].tile) {
			if (g.debug > 1) {
				fprintf(stderr, "copying tile %d to canvas\n",
					j);
			}
			//XSync (dpy, false);
			int r = XCopyArea(dpy, g.winlist[j].tile, uiwin,
					  g.gcDirect, 0, 0, tileW, tileH,	// src
					  j * (tileW + FRAME_W) + FRAME_W, FRAME_W);	// dst
			//XSync (dpy, false);
			if (g.debug > 1) {
				fprintf(stderr, "XCopyArea returned %d\n", r);
			}
		}
	}
// frame
	framesRedraw(dpy);
}

//
// remove ui and switch to chosen window
//
int uiHide(Display * dpy, Window root)
{
	if (g.winlist) {
		if (g.debug > 0) {
			fprintf(stderr, "changing window focus to %lu\n",
				g.winlist[g.selNdx].id);
		}
		setFocus(dpy, g.selNdx);	// before winlist destruction!
	}
	if (g.debug > 0) {
		fprintf(stderr, "destroying our window and tiles\n");
	}
	if (uiwin) {
		XUnmapWindow(dpy, uiwin);
		XDestroyWindow(dpy, uiwin);
		uiwin = 0;
	}
	int y;
	for (y = 0; y < g.maxNdx; y++) {
		if (g.winlist && g.winlist[y].tile) {
			XFreePixmap(dpy, g.winlist[y].tile);
			g.winlist[y].tile = 0;
		}
	}
	if (g.winlist) {
		freeWinlist(dpy);
	}
	g.uiShowHasRun = false;
	return 1;
}

//
// select next item in g.winlist
//
int uiNextWindow(Display * dpy, Window root)
{
	if (!uiwin)
		return 0;	// kb events may trigger it even when no window drawn yet
	g.selNdx++;
	if (g.selNdx >= g.maxNdx)
		g.selNdx = 0;
	if (g.debug > 0) {
		fprintf(stderr, "item %d\n", g.selNdx);
	}
	framesRedraw(dpy);
	return 1;
}

//
// select previous item in g.winlist
//
int uiPrevWindow(Display * dpy, Window root)
{
	if (!uiwin)
		return 0;	// kb events may trigger it even when no window drawn yet
	g.selNdx--;
	if (g.selNdx < 0)
		g.selNdx = g.maxNdx - 1;
	if (g.debug > 0) {
		fprintf(stderr, "item %d\n", g.selNdx);
	}
	framesRedraw(dpy);
	return 1;
}
