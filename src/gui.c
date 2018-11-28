/*
Draw and interface with our switcher window.

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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
//#include <sys/time.h>
#include "alttab.h"
#include "util.h"
extern Globals g;
extern Display *dpy;
extern int scr;
extern Window root;

// PRIVATE

unsigned int tileW, tileH, iconW, iconH;
unsigned int visualTileW;
int lastPressedTile;
quad scrdim;
Window uiwin;
int uiwinW, uiwinH, uiwinX, uiwinY;
Colormap colormap;
Visual *visual;
//Font fontLabel;  // Xft instead
XftFont *fontLabel;
static int selNdx;                 // current (selected) item

//
// allocates GC
// type is: 
//   0: normal
//   1: for bg fill
//   2: for drawing frame
//
GC create_gc(int type)
{
    GC gc;                      /* handle of newly created GC.  */
    unsigned long valuemask = 0;    /* which values in 'values' to  */
    /* check when creating the GC.  */
    XGCValues values;           /* initial values for the GC.   */
    int line_style = LineSolid;
    int cap_style = CapButt;
    int join_style = JoinMiter;

    gc = XCreateGC(dpy, root, valuemask, &values);
    if (gc < 0) {
        msg(-1, "can't create GC\n");
        return 0;
    }
    /* allocate foreground and background colors for this GC. */
    switch (type) {
    case 1:
        XSetForeground(dpy, gc, g.color[COLBG].xcolor.pixel);
        XSetBackground(dpy, gc, g.color[COLFG].xcolor.pixel);
        XSetLineAttributes(dpy, gc, FRAME_W, line_style, cap_style, join_style);
        break;
    case 0:
        XSetForeground(dpy, gc, g.color[COLFG].xcolor.pixel);
        XSetBackground(dpy, gc, g.color[COLBG].xcolor.pixel);
        XSetLineAttributes(dpy, gc, 1, line_style, cap_style, join_style);
        break;
    case 2:
        XSetForeground(dpy, gc, g.color[COLFRAME].xcolor.pixel);
        XSetBackground(dpy, gc, g.color[COLBG].xcolor.pixel);
        XSetLineAttributes(dpy, gc, FRAME_W, line_style, cap_style, join_style);
        break;
    default:
        msg(-1, "unknown GC type, not setting colors\n");
        break;
    }
    /* define the fill style for the GC. to be 'solid filling'. */
    XSetFillStyle(dpy, gc, FillSolid);
    return gc;
}

//
// single use helper for function below
//
void drawFr(GC gc, int f)
{
    int d = XDrawRectangle(dpy, uiwin, gc,
                           f * (tileW + FRAME_W) + (FRAME_W / 2),
                           0 + (FRAME_W / 2),
                           tileW + FRAME_W, tileH + FRAME_W);
    if (!d) {
        msg(-1, "can't draw frame\n");
    }
}

//
// draw selected and unselected frames around tiles
//
void framesRedraw()
{
    int f;
    for (f = 0; f < g.maxNdx; f++) {
        if (f == selNdx)
            continue;           // skip
        drawFr(g.gcReverse, f); // thick bg
        drawFr(g.gcDirect, f);  // thin frame
    }
// _after_ unselected draw selected, because they may overlap
    drawFr(g.gcFrame, selNdx);
}

//
// given coordinates relative to our window,
// return the tile number or -1
//
int pointedTile(int x, int y)
{
    if (x < (FRAME_W / 2)
        || x > (uiwinW - (FRAME_W / 2))
        || y < 0 || y > uiwinH)
        return -1;
    return (x - (FRAME_W / 2)) / visualTileW;
}

//
// combine widgets into wi->tile
// for uiShow()
//
void prepareTile(WindowInfo * wi)
{
    wi->tile = XCreatePixmap(dpy, root, tileW, tileH, XDEPTH);
    if (!wi->tile)
        die("can't create tile");
    int fr = XFillRectangle(dpy, wi->tile, g.gcReverse, 0, 0,
                            tileW, tileH);
    if (!fr) {
        msg(-1, "can't fill tile\n");
    }
    // mini-window content could be drawn here,
    // but there is no backing store of windows 
    // in my simple environments (as reported by xwininfo)
    //
    // place icons
    if (wi->icon_drawable) {
        if (wi->icon_w == iconW && wi->icon_h == iconH) {
            // direct copy
            msg(1, "copying icon onto tile\n");
            // prepare special GC to copy icon, with clip mask if icon_mask present
            unsigned long ic_valuemask = 0;
            XGCValues ic_values;
            GC ic_gc = XCreateGC(dpy, root, ic_valuemask,
                                 &ic_values);
            if (ic_gc < 0) {
                msg(-1, "can't create GC to draw icon\n");
                goto endIcon;
            }
            if (wi->icon_mask != 0) {
                XSetClipMask(dpy, ic_gc, wi->icon_mask);
            }
            int or = XCopyArea(dpy,
                               wi->icon_drawable,
                               wi->tile,
                               ic_gc, 0, 0,
                               wi->icon_w, wi->icon_h,  // src
                               0, 0);   // dst
            if (!or) {
                msg(-1, "can't copy icon to tile\n");
            }
            XFreeGC(dpy, ic_gc);
        } else {
            // scale
            msg(1, "scaling icon onto tile\n");
            int sc = pixmapFit(wi->icon_drawable,
                               wi->icon_mask,
                               wi->tile,
                               wi->icon_w,
                               wi->icon_h,
                               iconW, iconH);
            if (!sc) {
                msg(-1, "can't scale icon to tile\n");
            }
        }
    } else {
        // draw placeholder or standalone icons from some WM
        GC gcL = create_gc(0);  // GC for thin line
        if (!gcL) {
            msg(-1, "can't create gcL\n");
        } else {
            XSetLineAttributes(dpy, gcL, 1, LineSolid, CapButt, JoinMiter);
            //XSetForeground (dpy, gcL, pixel);
            int pr = XDrawRectangle(dpy, wi->tile, gcL,
                                    0, 0, iconW, iconH);
            if (!pr) {
                msg(-1, "can't draw placeholder\n");
            }
            XFreeGC(dpy, gcL);
        }
    }
 endIcon:
    // draw labels
    if (wi->name && fontLabel) {
        int dr = drawMultiLine(wi->tile, fontLabel,
                               &(g.color[COLFG].xftcolor),
                               wi->name,
                               0, (iconH + 5), tileW,
                               (tileH - iconH - 5));
        if (dr != 1) {
            msg(-1, "can't draw label\n");
        }
    }
}                               // prepareTile

// PUBLIC

//
// early initialization
// called once per execution
// mostly initializes g.*
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
int startupGUItasks()
{
// if viewport is not fixed, then initialize vp* at every show
    if (g.option_vp_mode == VP_SPECIFIC) {
        g.vp = g.option_vp;
    }
    g.has_randr = randrAvailable();
// colors
    colormap = DefaultColormap(dpy, scr);
    visual = DefaultVisual(dpy, scr);
    msg(0, "early allocating colors\n");
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
                if (strncmp(g.color[p].name, "_rnd_low", 8) == 0) {
                    (void)snprintf(g.color[p].name, 8,
                                   "#%.2hhx%.2hhx%.2hhx", r[0], r[1], r[2]);
                    g.color[p].name[7] = '\0';
                } else if (strncmp(g.color[p].name, "_rnd_high", 9)
                           == 0) {
                    (void)snprintf(g.color[p].name, 9,
                                   "#%.2hhx%.2hhx%.2hhx",
                                   r[0] + 0x80, r[0] + 0x80, r[1] + 0x80);
                    g.color[p].name[7] = '\0';
                }
                msg(1,
                    "color generated: %s, RAND_MAX=%d\n",
                    g.color[p].name, RAND_MAX);
            }
            if (!XAllocNamedColor(dpy,
                                  colormap,
                                  g.color[p].name,
                                  &(g.color[p].xcolor), &(g.color[p].xcolor)))
                die("failed to allocate X color: ", g.color[p].name);
            if (!XftColorAllocName
                (dpy, visual, colormap, g.color[p].name,
                 &(g.color[p].xftcolor)))
                die("failed to allocate Xft color: ", g.color[p].name);
        }
    }

    msg(0, "early opening font\n");
//fontLabel = XLoadFont (dpy, LABELFONT);  // using Xft instead
    fontLabel = XftFontOpenName(dpy, scr, g.option_font);
    if (!fontLabel) {
        msg(-1,
            "can't allocate font: %s\ncheck installed fontconfig fonts: fc-list\n",
            g.option_font);
    }
// having colors, GC may be built
// they are required early for addWindow when transforming icon depth
    msg(0, "early building GCs\n");
    g.gcDirect = create_gc(0);
    g.gcReverse = create_gc(1);
    g.gcFrame = create_gc(2);

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
int uiShow(bool direction)
{
    msg(0, "preparing ui\n");
    g.uiShowHasRun = true;      // begin allocations
// screen-related stuff is not at startup but here,
// because screen configuration may get changed at runtime
// moreover, DisplayWidth/Height aren't changed without
// reconnecting to X server, that's why root geometry is used
    XWindowAttributes ra;
    if (XGetWindowAttributes(dpy, root, &ra) != 0) {
        scrdim.x = ra.x;
        scrdim.y = ra.y;
        scrdim.w = ra.width;
        scrdim.h = ra.height;
    } else {
        msg(-1, "can't get root window attributes, using screen dimensions\n");
        scrdim.x = scrdim.y = 0;
        scrdim.w = DisplayWidth(dpy, scr);
        scrdim.h = DisplayHeight(dpy, scr);
    }
// caculate viewport.
#define VPM  g.option_vp_mode
    switch (VPM) {
    case VP_SPECIFIC:
        // initialized at startup instead
        break;
    case VP_TOTAL:
        g.vp = scrdim;
        break;
    case VP_FOCUS:
    case VP_POINTER:
        if (g.has_randr) {
            bool multihead;
            if (!randrGetViewport(&(g.vp), &multihead)) {
                msg(0,
                    "can't obtain viewport from randr, using default screen\n");
                g.vp = scrdim;
            }
            if (!multihead) {
                msg(0,
                    "randr reports single head, using default screen instead\n");
                g.vp = scrdim;
            }
        } else {
            msg(0, "no randr, using default screen as viewport\n");
            g.vp = scrdim;
        }
        break;
    default:
        msg(-1, "unknown viewport mode, using default screen\n");
        g.vp = scrdim;
    }

    XClassHint class_h = { XCLASSNAME, XCLASS };

// to init winlist, the following must be initialized:
// GC,
// g.vp (for SCR_CURRENT)

    g.winlist = NULL;
    g.maxNdx = 0;
    if (!initWinlist()) {
        msg(0, "initWinlist failed, skipping ui initialization\n");
        g.winlist = NULL;
        g.maxNdx = 0;
        return 0;
    }

    if (!g.winlist) {
        msg(0, "winlist doesn't exist, skipping ui initialization\n");
        return 0;
    }
    if (g.maxNdx < 1) {
        msg(0, "number of windows < 1, skipping ui initialization\n");
        return 0;
    }

    selNdx = direction ? (g.maxNdx - 1) : ((0 >= (g.maxNdx - 1)) ? 0 : 1);
//if (selNdx<0 || selNdx>=g.maxNdx) { selNdx=0; } // just for case
    msg(1, "Current (selected) item in winlist: %d\n", selNdx);

    if (g.debug > 0) {
        msg(0, "got %d windows\n", g.maxNdx);
        int i;
        for (i = 0; i < g.maxNdx; i++) {
            msg(0,
                "%d: %lx (lvl %d, icon %lu (%dx%d)): %s\n", i,
                g.winlist[i].id, g.winlist[i].reclevel,
                g.winlist[i].icon_drawable, g.winlist[i].icon_w,
                g.winlist[i].icon_h, g.winlist[i].name);
        }
    }
// have winlist, now back to uiwin stuff
// calculate dimensions
    tileW = g.option_tileW;
    tileH = g.option_tileH;
    iconW = g.option_iconW;
    iconH = g.option_iconH;
    float rt = 1.0;
// for subsequent calculation of width(s), use 'avail_w'
// instead of g.vp.w, because they don't match for POS_SPECIFIC
    int avail_w = g.vp.w;
    if (g.option_positioning == POS_SPECIFIC)
        avail_w -= g.option_posX;
// tiles may be smaller if they don't fit viewport
    uiwinW = (tileW + FRAME_W) * g.maxNdx + FRAME_W;
    if (uiwinW > avail_w) {
        int frames = FRAME_W * g.maxNdx + FRAME_W;
        rt = ((float)(avail_w - frames)) / ((float)(tileW * g.maxNdx));
        tileW = (float)tileW *rt;
        tileH = (float)tileH *rt;
        uiwinW = tileW * g.maxNdx + frames;
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
    if (g.option_positioning == POS_CENTER) {
        uiwinX = (g.vp.w - uiwinW) / 2 + g.vp.x;
        uiwinY = (g.vp.h - uiwinH) / 2 + g.vp.y;
    } else {
        uiwinX = g.option_posX + g.vp.x;
        uiwinY = g.option_posY + g.vp.y;
    }
    visualTileW = (uiwinW - FRAME_W) / g.maxNdx;
    if (g.debug > 0) {
        msg(0, "tile w=%d h=%d\n", tileW, tileH);
        msg(0, "uiwin %dx%d +%d+%d", uiwinW, uiwinH, uiwinX, uiwinY);
        if (g.debug > 1) {
            int nscr, si;
            Screen *s;
            nscr = ScreenCount(dpy);
            msg(0, ", %d screen(s):", nscr);
            for (si = 0; si < nscr; ++si) {
                s = ScreenOfDisplay(dpy, si);
                msg(0, " [%dx%d]", s->width, s->height);
            }
            msg(0, ", viewport %dx%d+%d+%d, avail_w %d",
                g.vp.w, g.vp.h, g.vp.x, g.vp.y, avail_w);
        }
        msg(0, "\n");
    }
// prepare tiles

    if (!g.winlist) {
        die("no winlist in uiShow. this shouldn't happen, please report.");
    }
    int m;
    for (m = 0; m < g.maxNdx; m++) {
        prepareTile(&(g.winlist[m]));
    }
    msg(0, "prepared %d tiles\n", m);
    if (fontLabel)
        XftFontClose(dpy, fontLabel);

// prepare our window
    unsigned long valuemask = CWBackPixel | CWBorderPixel | CWOverrideRedirect;
    XSetWindowAttributes attributes;
    attributes.background_pixel = g.color[COLBG].xcolor.pixel;
    attributes.border_pixel = g.color[COLFRAME].xcolor.pixel;
    attributes.override_redirect = 1;
    uiwin = XCreateWindow(dpy, root, uiwinX, uiwinY, uiwinW, uiwinH, 0, // border_width
                          CopyFromParent,   // depth
                          InputOutput,  // class
                          CopyFromParent,   // visual
                          valuemask, &attributes);
    if (uiwin <= 0)
        die("can't create window");
    msg(0, "our window is 0x%lx\n", uiwin);

// set properties of our window
    XStoreName(dpy, uiwin, XWINNAME);
    XSetClassHint(dpy, uiwin, &class_h);
// warning: this overwrites any previous value.
// note: x_setCommonPropertiesForAnyWindow does the same thing for any window
    XSelectInput(dpy, uiwin, ExposureMask | KeyPressMask | KeyReleaseMask
                 | ButtonPressMask | ButtonReleaseMask);
// set window type so that WM will hopefully not resize it
// before mapping: https://specifications.freedesktop.org/wm-spec/1.3/ar01s05.html
    Atom at = XInternAtom(dpy, "ATOM", True);
    Atom wt = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    Atom td = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    if (at && wt && td)
        XChangeProperty(dpy, uiwin, wt, at, 32, PropModeReplace,
                        (unsigned char *)(&td), 1);
// disable appearance in taskbar
    Atom st = XInternAtom(dpy, "_NET_WM_STATE", True);
    Atom sk = XInternAtom(dpy, "_NET_WM_STATE_SKIP_TASKBAR", True); // there is also PAGER
    if (at && st && sk)
        XChangeProperty(dpy, uiwin, st, at, 32, PropModeReplace,
                        (unsigned char *)(&sk), 1);
// xmonad ignores _NET_WM_WINDOW_TYPE_DIALOG but obeys WM_TRANSIENT_FOR
    XSetTransientForHint(dpy, uiwin, uiwin);
// disable window title and borders. works in xfwm4.
#define PROP_MOTIF_WM_HINTS_ELEMENTS 5
#define MWM_HINTS_DECORATIONS (1L << 1)
    struct {
        unsigned long flags;
        unsigned long functions;
        unsigned long decorations;
        long inputMode;
        unsigned long status;
    } hints = {
    MWM_HINTS_DECORATIONS, 0, 0,};
    Atom ma = XInternAtom(dpy, "_MOTIF_WM_HINTS", False);
    if (ma) {
        XChangeProperty(dpy, uiwin, ma, ma, 32, PropModeReplace,
                        (unsigned char *)&hints, PROP_MOTIF_WM_HINTS_ELEMENTS);
    }

    XMapWindow(dpy, uiwin);

    // positioning and size hints.
    // centering required in JWM.
    // should really perform centering when 
    //  viewport == wm screen. how would we know the latter?
    long sflags;
    sflags =
        USPosition | USSize | PPosition | PSize | PMinSize | PMaxSize |
        PBaseSize;
    // gravity: https://tronche.com/gui/x/xlib/window/attributes/gravity.html
    if (g.option_positioning != POS_NONE)
        sflags |= PWinGravity;
    XSizeHints uiwinSizeHints = { sflags,
        uiwinX, uiwinY,         // obsoleted
        uiwinW, uiwinH,         // obsoleted
        uiwinW, uiwinH,
        uiwinW, uiwinH,
        0, 0,
        {0, 0}
        , {0, 0}
        ,
        uiwinW, uiwinH,
        (g.option_positioning == POS_CENTER
         //&& g.option_vp_mode != VP_SPECIFIC) ? CenterGravity : ForgetGravity };
         && g.option_vp_mode != VP_SPECIFIC) ? CenterGravity : StaticGravity
    };
    XSetWMNormalHints(dpy, uiwin, &uiwinSizeHints);

    return 1;
}

//
// Expose event callback
// redraw our window
//
void uiExpose()
{
    msg(0, "expose ui\n");
// if WM moved uiwin, here is the place
// where we first see 'bad' absolute coordinates.
// try to correct them.
    quad uwq;
    if (get_absolute_coordinates(uiwin, &uwq)) {
// debug for #54
        msg(1, "attr abs at expose: %dx%d +%d+%d\n",
            uwq.w, uwq.h, uwq.x, uwq.y);
        int xdiff = uwq.x - uiwinX;
        int ydiff = uwq.y - uiwinY;
        if (abs(xdiff) > FRAME_W / 2 || abs(ydiff) > FRAME_W / 2) {
            msg(1, "WM moved uiwin too far, trying to correct\n");
            XMoveWindow(dpy, uiwin, uiwinX, uiwinY);
        }
        if (uwq.w != uiwinW || uwq.h != uiwinH) {
            // WM resized our window, like
            // floating_maximum_size in #54.
            // there is little can be done here,
            // so just complain.
            msg(-1,
                "switcher window resized, expect bugs. Please configure WM to not interfere with alttab window size, for example, disable 'floating_maximum_size' in i3\n");
        }
    }
// icons
    int j;
    for (j = 0; j < g.maxNdx; j++) {
        if (g.winlist[j].tile) {
            msg(1, "copying tile %d to canvas\n", j);
            //XSync (dpy, false);
            int r = XCopyArea(dpy, g.winlist[j].tile, uiwin,
                              g.gcDirect, 0, 0, tileW, tileH,   // src
                              j * (tileW + FRAME_W) + FRAME_W, FRAME_W);    // dst
            //XSync (dpy, false);
            msg(1, "XCopyArea returned %d\n", r);
        }
    }
// frame
    framesRedraw();
}

//
// remove ui and switch to chosen window
//
int uiHide()
{
    // order is important: to set focus in Metacity,
    // our window must be destroyed first
    if (uiwin) {
        msg(0, "destroying our window\n");
        XUnmapWindow(dpy, uiwin);
        XDestroyWindow(dpy, uiwin);
        uiwin = 0;
    }
    if (g.winlist) {
        msg(0, "changing focus to 0x%lx\n", g.winlist[selNdx].id);
        /*
           // save the switch moment for detecting
           // subsequent false focus event from WM
           gettimeofday(&(g.last.tv), NULL);
           g.last.prev = g.winlist[g.startNdx].id;
           g.last.to = g.winlist[selNdx].id;
         */
        setFocus(selNdx);     // before winlist destruction!
    }
    msg(0, "destroying tiles\n");
    int y;
    for (y = 0; y < g.maxNdx; y++) {
        if (g.winlist && g.winlist[y].tile) {
            XFreePixmap(dpy, g.winlist[y].tile);
            g.winlist[y].tile = 0;
        }
    }
    if (g.winlist) {
        freeWinlist();
    }
    g.uiShowHasRun = false;
    return 1;
}

//
// select next item in g.winlist
//
int uiNextWindow()
{
    if (!uiwin)
        return 0;               // kb events may trigger it even when no window drawn yet
    selNdx++;
    if (selNdx >= g.maxNdx)
        selNdx = 0;
    msg(0, "item %d\n", selNdx);
    framesRedraw();
    return 1;
}

//
// select previous item in g.winlist
//
int uiPrevWindow()
{
    if (!uiwin)
        return 0;               // kb events may trigger it even when no window drawn yet
    selNdx--;
    if (selNdx < 0)
        selNdx = g.maxNdx - 1;
    msg(0, "item %d\n", selNdx);
    framesRedraw();
    return 1;
}

//
// select item in g.winlist
//
int uiSelectWindow(int ndx)
{
    if (!uiwin)
        return 0;               // kb events may trigger it even when no window drawn yet
    if (ndx < 0 || ndx >= g.maxNdx) {
        return 0;
    }
    selNdx = ndx;
    msg(0, "item %d\n", selNdx);
    framesRedraw();
    return 1;
}

//
// mouse press/release handler
//
void uiButtonEvent(XButtonEvent e)
{
    if (!uiwin)
        return;
    if (e.type == ButtonPress) {
        switch (e.button) {
        case 1:
            lastPressedTile = pointedTile(e.x, e.y);
            if (lastPressedTile != -1)
                uiSelectWindow(lastPressedTile);
            break;
        case 4:
            uiPrevWindow();
            break;
        case 5:
            uiNextWindow();
            break;
        }
    }
    if (e.type == ButtonRelease && e.button == 1) {
        if (lastPressedTile != -1 && lastPressedTile == pointedTile(e.x, e.y))
            uiHide();
    }
}

//
// our window
//
Window getUiwin()
{
    return uiwin;
}
