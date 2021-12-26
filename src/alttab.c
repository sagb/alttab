/*
Parsing options/resources, top-level keygrab functions and main().

Copyright 2017-2021 Alexander Kulak.
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
#include <X11/Xresource.h>
#include <X11/Xft/Xft.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "alttab.h"
#include "util.h"
#include "config.h"

// PUBLIC

Globals g;
// globals common for alttab, util and icon
Display *dpy;
int scr;
Window root;

// PRIVATE
static XrmDatabase db;

//
// help and exit
//
static void helpexit()
{
    msg(-1, "the task switcher, v%s\n\
Options:\n\
    -w N      window manager: 0=no, 1=ewmh-compatible, 2=ratpoison, 3=old fashion\n\
    -d N      desktop: 0=current 1=all, 2=all but special, 3=all but current\n\
   -sc N      screen: 0=current 1=all\n\
   -kk str    keysym of main key\n\
   -mk str    keysym of main modifier\n\
   -bk str    keysym of backscroll modifier\n\
   -pk str    keysym of 'prev' key\n\
   -nk str    keysym of 'next' key\n\
   -mm N      (obsoleted) main modifier mask\n\
   -bm N      (obsoleted) backward scroll modifier mask\n\
    -t NxM    tile geometry\n\
    -i NxM    icon geometry\n\
   -vp str    switcher viewport: focus, pointer, total, WxH+X+Y\n\
    -p str    switcher position: center, none, +X+Y\n\
    -s N      icon source: 0=X11 only, 1=fallback to files, 2=best size, 3=files only\n\
-theme name   icon theme\n\
   -bg color  background color\n\
   -fg color  foreground color\n\
-frame color  active frame color\n\
-inact color  inactive frame color\n\
   -bc color  extra border color\n\
   -bw N      extra border width\n\
 -font name   font name in the form xft:fontconfig_pattern\n\
 -vertical    verticat layout\n\
    -e        keep switcher after keys release\n\
  -v|-vv      verbose\n\
    -h        help\n\
See man alttab for details.\n", PACKAGE_VERSION);
    exit(0);
}

//
// initialize globals based on executable agruments and Xresources
// return 1 if success, 0 otherwise
// on fatal failure, calls die/exit
//
static int use_args_and_xrm(int *argc, char **argv)
{
// set debug level early
    g.debug = 0;
    char *errmsg;
    int ksi;
    KeyCode BC;
    unsigned int wmindex, dsindex, scindex, isrc;
    char *gtile, *gicon, *gview, *gpos;
    int x, y;
    unsigned int w, h, bw;
    int xpg;
    char *s;
    char *rm;
    char *empty = "";
    int uo;
    Atom nwm_prop, atype;
    unsigned char *nwm;
    int form;
    unsigned long remain, len;
    XrmOptionDescRec xrmTable[] = {
        {"-w", "*windowmanager", XrmoptionSepArg, NULL},
        {"-d", "*desktops", XrmoptionSepArg, NULL},
        {"-sc", "*screens", XrmoptionSepArg, NULL},
        {"-mm", "*modifier.mask", XrmoptionSepArg, NULL},
        {"-bm", "*backscroll.mask", XrmoptionSepArg, NULL},
        {"-mk", "*modifier.keysym", XrmoptionSepArg, NULL},
        {"-kk", "*key.keysym", XrmoptionSepArg, NULL},
        {"-bk", "*backscroll.keysym", XrmoptionSepArg, NULL},
        {"-pk", "*prevkey.keysym", XrmoptionSepArg, NULL},
        {"-nk", "*nextkey.keysym", XrmoptionSepArg, NULL},
        {"-ck", "*cancelkey.keysym", XrmoptionSepArg, NULL},
        {"-t", "*tile.geometry", XrmoptionSepArg, NULL},
        {"-i", "*icon.geometry", XrmoptionSepArg, NULL},
        {"-vp", "*viewport", XrmoptionSepArg, NULL},
        {"-p", "*position", XrmoptionSepArg, NULL},
        {"-s", "*icon.source", XrmoptionSepArg, NULL},
        {"-theme", "*theme", XrmoptionSepArg, NULL},
        {"-bg", "*background", XrmoptionSepArg, NULL},
        {"-fg", "*foreground", XrmoptionSepArg, NULL},
        {"-frame", "*framecolor", XrmoptionSepArg, NULL},
        {"-inact", "*inactcolor", XrmoptionSepArg, NULL},
        {"-bc", "*bordercolor", XrmoptionSepArg, NULL},
        {"-bw", "*borderwidth", XrmoptionSepArg, NULL},
        {"-font", "*font", XrmoptionSepArg, NULL},
        {"-vertical", "*vertical", XrmoptionIsArg, NULL},
        {"-e", "*keep", XrmoptionIsArg, NULL}
    };
    const char *inv = "invalid %s, use -h for help\n";
    const char *rmb = "can't figure out modmask from keycode 0x%x\n";

// not using getopt() because of need for "-v" before Xrm
    int arg;
    for (arg = 0; arg < (*argc); arg++) {
        if ((strcmp(argv[arg], "-v") == 0)) {
            g.debug = 1;
            remove_arg(argc, argv, arg);
        } else if ((strcmp(argv[arg], "-vv") == 0)) {
            g.debug = 2;
            remove_arg(argc, argv, arg);
        } else if ((strcmp(argv[arg], "-h") == 0)) {
            helpexit();
            remove_arg(argc, argv, arg);
        }
    }
    msg(0, "%s\n", PACKAGE_STRING);
    msg(0, "debug level %d\n", g.debug);

    XrmInitialize();
    rm = XResourceManagerString(dpy);
    msg(1, "resource manager: \"%s\"\n", rm);
    if (!rm) {
        msg(0, "can't get resource manager, using empty db\n");
        //return 0;  // we can do it
        //db = XrmGetDatabase (dpy);
        rm = empty;
    }
    db = XrmGetStringDatabase(rm);
    if (!db) {
        msg(-1, "can't get resource database\n");
        return 0;
    }
    XrmParseCommand(&db, xrmTable, sizeof(xrmTable) / sizeof(xrmTable[0]),
                    XRMAPPNAME, argc, argv);
    if ((*argc) > 1) {
        g.debug = 1;
        msg(-1, "unknown options or wrong arguments:");
        for (uo = 1; uo < (*argc); uo++) {
            msg(0, " \"%s\"", argv[uo]);
        }
        msg(0, ", use -h for help\n");
        exit(1);
    }

    switch (xresource_load_int(&db, XRMAPPNAME, "windowmanager", &wmindex)) {
    case 1:
        if (wmindex >= WM_MIN && wmindex <= WM_MAX) {
            g.option_wm = wmindex;
            goto wmDone;
        } else {
            die(inv, "windowmanager argument range");
        }
        break;
    case 0:
        msg(0, "no WM index or unknown, guessing\n");
        break;
    case -1:
        die(inv, "windowmanager argument");
        break;
    }
// EWMH?
    if (ewmh_detectFeatures(&(g.ewmh))) {
        msg(0, "EWMH-compatible WM detected: %s\n", g.ewmh.wmname);
        g.option_wm = WM_EWMH;
        goto wmDone;
    }
// ratpoison?
    nwm_prop = XInternAtom(dpy, "_NET_WM_NAME", false);
    if (XGetWindowProperty(dpy, root, nwm_prop, 0, MAXNAMESZ, false,
                           AnyPropertyType, &atype, &form, &len, &remain,
                           &nwm) == Success && nwm) {
        msg(0, "_NET_WM_NAME root property present: %s\n", nwm);
        if (strstr((char *)nwm, "ratpoison") != NULL) {
            g.option_wm = WM_RATPOISON;
            XFree(nwm);
            goto wmDone;
        }
        XFree(nwm);
    }
    msg(0, "unknown WM, using WM_TWM\n");
    g.option_wm = WM_TWM;
 wmDone:
    msg(0, "WM: %d\n", g.option_wm);

    switch (xresource_load_int(&db, XRMAPPNAME, "desktops", &dsindex)) {
    case 1:
        if (dsindex >= DESK_MIN && dsindex <= DESK_MAX)
            g.option_desktop = dsindex;
        else
            die(inv, "desktops argument range");
        break;
    case 0:
        g.option_desktop = DESK_DEFAULT;
        break;
    case -1:
        die(inv, "desktops argument");
        break;
    }
    msg(0, "desktops: %d\n", g.option_desktop);

    switch (xresource_load_int(&db, XRMAPPNAME, "screens", &scindex)) {
    case 1:
        if (scindex >= SCR_MIN && scindex <= SCR_MAX)
            g.option_screen = scindex;
        else
            die(inv, "screens argument range");
        break;
    case 0:
        g.option_screen = SCR_DEFAULT;
        break;
    case -1:
        die(inv, "screens argument");
        break;
    }
    msg(0, "screens: %d\n", g.option_screen);

#define  MC  g.option_modCode
#define  KC  g.option_keyCode
#define  prevC  g.option_prevCode
#define  nextC  g.option_nextCode
#define  cancelC  g.option_cancelCode
#define  GMM  g.option_modMask
#define  GBM  g.option_backMask

    ksi = ksym_option_to_keycode(&db, XRMAPPNAME, "modifier", &errmsg);
    if (ksi == -1)
        die("%s\n", errmsg);
    MC = ksi != 0 ? ksi : XKeysymToKeycode(dpy, DEFMODKS);

    ksi = ksym_option_to_keycode(&db, XRMAPPNAME, "key", &errmsg);
    if (ksi == -1)
        die("%s\n", errmsg);
    KC = ksi != 0 ? ksi : XKeysymToKeycode(dpy, DEFKEYKS);

    ksi = ksym_option_to_keycode(&db, XRMAPPNAME, "prevkey", &errmsg);
    if (ksi == -1)
        die("%s\n", errmsg);
    prevC = ksi != 0 ? ksi : XKeysymToKeycode(dpy, DEFPREVKEYKS);

    ksi = ksym_option_to_keycode(&db, XRMAPPNAME, "nextkey", &errmsg);
    if (ksi == -1)
        die("%s\n", errmsg);
    nextC = ksi != 0 ? ksi : XKeysymToKeycode(dpy, DEFNEXTKEYKS);

    ksi = ksym_option_to_keycode(&db, XRMAPPNAME, "cancelkey", &errmsg);
    if (ksi == -1)
        die("%s\n", errmsg);
    cancelC = ksi != 0 ? ksi : XKeysymToKeycode(dpy, DEFCANCELKS);

    switch (xresource_load_int(&db, XRMAPPNAME, "modifier.mask", &(GMM))) {
    case 1:
        msg(-1,
            "Using obsoleted -mm option or modifier.mask resource, see man page for upgrade\n");
        break;
    case 0:
        GMM = keycode_to_modmask(MC);
        if (GMM == 0)
            die(rmb, MC);
        break;
    case -1:
        die(inv, "modifier mask");
        break;
    }

    switch (xresource_load_int(&db, XRMAPPNAME, "backscroll.mask", &(GBM))) {
    case 1:
        msg(-1,
            "Using obsoleted -bm option or backscroll.mask resource, see man page for upgrade\n");
        break;
    case 0:
        BC = ksym_option_to_keycode(&db, XRMAPPNAME, "backscroll", &errmsg);
        if (BC != 0) {
            GBM = keycode_to_modmask(BC);
            if (GBM == 0)
                die(rmb, BC);
        } else {
            GBM = DEFBACKMASK;
        }
        break;
    case -1:
        die(inv, "backscroll mask");
        break;
    }

    msg(0, "modMask %d, backMask %d, modCode %d, keyCode %d\n",
        GMM, GBM, MC, KC);

    g.option_tileW = DEFTILEW;
    g.option_tileH = DEFTILEH;
    gtile = xresource_load_string(&db, XRMAPPNAME, "tile.geometry");
    if (gtile != NULL) {
        xpg = XParseGeometry(gtile, &x, &y, &w, &h);
        if (xpg & WidthValue)
            g.option_tileW = w;
        else
            die(inv, "tile width");
        if (xpg & HeightValue)
            g.option_tileH = h;
        else
            die(inv, "tile height");
    }

    g.option_iconW = DEFICONW;
    g.option_iconH = DEFICONH;
    gicon = xresource_load_string(&db, XRMAPPNAME, "icon.geometry");
    if (gicon) {
        xpg = XParseGeometry(gicon, &x, &y, &w, &h);
        if (xpg & WidthValue)
            g.option_iconW = w;
        else
            die(inv, "icon width");
        if (xpg & HeightValue)
            g.option_iconH = h;
        else
            die(inv, "icon height");
    }

    msg(0, "%dx%d tile, %dx%d icon\n",
        g.option_tileW, g.option_tileH, g.option_iconW, g.option_iconH);

    switch (xresource_load_int(&db, XRMAPPNAME, "borderwidth", &bw)) {
    case 1:
        if (bw >= BORDER_MIN)
          g.option_borderW = bw;
        else
          die(inv, "bw argument range");
        break;
    case 0:
        g.option_borderW = DEFBORDERW;
        break;
    case -1:
        die(inv, "bw argument");
        break;
    }
    msg(0, "bw: %d\n", g.option_borderW);

    bzero(&(g.option_vp), sizeof(g.option_vp));
    g.option_vp_mode = VP_DEFAULT;
    gview = xresource_load_string(&db, XRMAPPNAME, "viewport");
    if (gview) {
        if (strncmp(gview, "focus", 6) == 0) {
            g.option_vp_mode = VP_FOCUS;
        } else if (strncmp(gview, "pointer", 8) == 0) {
            g.option_vp_mode = VP_POINTER;
        } else if (strncmp(gview, "total", 6) == 0) {
            g.option_vp_mode = VP_TOTAL;
        } else {
            g.option_vp_mode = VP_SPECIFIC;
            xpg = XParseGeometry(gview, &x, &y, &w, &h);
            if (xpg & (XValue | YValue | WidthValue | HeightValue)) {
                g.option_vp.w = w;
                g.option_vp.h = h;
                g.option_vp.x = x;
                g.option_vp.y = y;
            } else {
                die(inv, "viewport");
            }
        }
    }
    msg(0, "viewport: mode %d, %dx%d+%d+%d\n",
        g.option_vp_mode,
        g.option_vp.w, g.option_vp.h, g.option_vp.x, g.option_vp.y);

    g.option_positioning = POS_DEFAULT;
    g.option_posX = 0;
    g.option_posY = 0;
    gpos = xresource_load_string(&db, XRMAPPNAME, "position");
    if (gpos) {
        if (strncmp(gpos, "center", 7) == 0) {
            g.option_positioning = POS_CENTER;
        } else if (strncmp(gpos, "none", 5) == 0) {
            g.option_positioning = POS_NONE;
        } else {
            g.option_positioning = POS_SPECIFIC;
            xpg = XParseGeometry(gpos, &x, &y, &w, &h);
            if (xpg & (XValue | YValue)) {
                g.option_posX = x;
                g.option_posY = y;
            } else {
                die(inv, "position");
            }
        }
    }
    msg(0, "positioning policy: %d, position: +%d+%d\n",
        g.option_positioning, g.option_posX, g.option_posY);

    g.option_iconSrc = ISRC_DEFAULT;
    switch (xresource_load_int(&db, XRMAPPNAME, "icon.source", &isrc)) {
    case 1:
        if (isrc >= ISRC_MIN && isrc <= ISRC_MAX)
            g.option_iconSrc = isrc;
        else
            die("icon source argument must be from %d to %d\n",
                ISRC_MIN, ISRC_MAX);
        break;
    case 0:
        g.option_iconSrc = ISRC_DEFAULT;
        break;
    case -1:
        die(inv, "icon source");
        break;
    }
    msg(0, "icon source: %d\n", g.option_iconSrc);

    s = xresource_load_string(&db, XRMAPPNAME, "theme");
    g.option_theme = s ? s : DEFTHEME;
    msg(0, "icon theme: %s\n", g.option_theme);

    s = xresource_load_string(&db, XRMAPPNAME, "background");
    g.color[COLBG].name = s ? s : DEFCOLBG;
    s = xresource_load_string(&db, XRMAPPNAME, "foreground");
    g.color[COLFG].name = s ? s : DEFCOLFG;
    s = xresource_load_string(&db, XRMAPPNAME, "framecolor");
    g.color[COLFRAME].name = s ? s : DEFCOLFRAME;
    s = xresource_load_string(&db, XRMAPPNAME, "inactcolor");
    g.color[COLINACT].name = s ? s : DEFCOLINACT;
    s = xresource_load_string(&db, XRMAPPNAME, "bordercolor");
    g.color[COLBORDER].name = s ? s : DEFCOLBORDER;

    s = xresource_load_string(&db, XRMAPPNAME, "font");
    if (s) {
        if ((strncmp(s, "xft:", 4) == 0)
            && (*(s + 4) != '\0')) {
            g.option_font = s + 4;
        } else {
            // resource may indeed be valid but non-xft
            msg(-1, "invalid font: %s, using default: %s\n", s, DEFFONT);
            g.option_font = DEFFONT + 4;
        }
    } else {
        g.option_font = DEFFONT + 4;
    }

    s = xresource_load_string(&db, XRMAPPNAME, "vertical");
    g.option_vertical = (s != NULL);
    msg(0, "vertical: %d\n", g.option_vertical);

// max recursion for searching windows
// -1 is "everything"
// in raw X this returns too much windows, "1" is probably sufficient
// no need for an option
    g.option_max_reclevel = (g.option_wm == WM_NO) ? 1 : -1;

    s = xresource_load_string(&db, XRMAPPNAME, "keep");
    g.option_keep_ui = (s != NULL);
    msg(0, "keep_ui: %d\n", g.option_keep_ui);

    return 1;
}

//
// grab Alt-Tab and Alt-Shift-Tab
// note: exit() on failure
//
static int grabKeysAtStartup(bool grabUngrab)
{
    g.ignored_modmask = getOffendingModifiersMask(dpy); // or 0 for g.debug
    char *grabhint =
        "Error while (un)grabbing key 0x%x with mask 0x%x/0x%x.\nProbably other program already grabbed this combination.\nCheck: xdotool keydown alt+Tab; xdotool key XF86LogGrabInfo; xdotool keyup Tab; sleep 1; xdotool keyup alt\nand then look for active device grabs in /var/log/Xorg.0.log\nOr try Ctrl-Tab instead of Alt-Tab:  alttab -mk Control_L\n";
// attempt XF86Ungrab? probably too invasive
    if (!changeKeygrab
        (root, grabUngrab, g.option_keyCode, g.option_modMask,
         g.ignored_modmask)) {
        die(grabhint, g.option_keyCode, g.option_modMask, g.ignored_modmask);
    }
    if (!changeKeygrab
        (root, grabUngrab, g.option_keyCode,
         g.option_modMask | g.option_backMask, g.ignored_modmask)) {
        die(grabhint, g.option_keyCode,
            g.option_modMask | g.option_backMask, g.ignored_modmask);
    }

    return 1;
}

//
// Returns 0 if not an extra prev/next keycode, 1 if extra prev keycode, and 2 if extra next keycode.
//
static int isPrevNextKey(unsigned int keycode)
{
    if (keycode == g.option_prevCode) {
        return 1;
    }
    if (keycode == g.option_nextCode) {
        return 2;
    }
    // if here then is neither
    return 0;
}


int main(int argc, char **argv)
{

    XEvent ev;
    dpy = XOpenDisplay(NULL);
    if (!dpy)
        die("can't open display");
    scr = DefaultScreen(dpy);
    root = DefaultRootWindow(dpy);

    ee_complain = true;
    //hnd = (XErrorHandler)0;
    XErrorHandler hnd = XSetErrorHandler(zeroErrorHandler); // for entire program
    if (hnd) ;;                 // make -Wunused happy

    signal(SIGUSR1, sighandler);

    if (!use_args_and_xrm(&argc, argv))
        die("use_args_and_xrm failed");
    if (!startupWintasks())
        die("startupWintasks failed");
    if (!startupGUItasks())
        die("startupGUItasks failed");

    grabKeysAtStartup(true);
    g.uiShowHasRun = false;

    struct timespec nanots;
    nanots.tv_sec = 0;
    nanots.tv_nsec = 1E7;
    char keys_pressed[32];
    int octet = g.option_modCode / 8;
    int kmask = 1 << (g.option_modCode - octet * 8);

    while (true) {
        memset(&(ev.xkey), 0, sizeof(ev.xkey));

        if (g.uiShowHasRun && ! g.option_keep_ui) {
            // poll: lag and consume cpu, but necessary because of bug #1 and #2
            XQueryKeymap(dpy, keys_pressed);
            if (!(keys_pressed[octet] & kmask)) {   // Alt released
                uiHide();
                continue;
            }
            if (!XCheckIfEvent(dpy, &ev, *predproc_true, NULL)) {
                nanosleep(&nanots, NULL);
                continue;
            }
        } else {
            // event: immediate, when we don't care about Alt release
            XNextEvent(dpy, &ev);
        }

        switch (ev.type) {
        case KeyPress:
            msg(1, "Press %lx: %d-%d\n",
                ev.xkey.window, ev.xkey.state, ev.xkey.keycode);
            if (ev.xkey.state & g.option_modMask) {  // alt
                if (ev.xkey.keycode == g.option_keyCode) {  // tab
                    // additional check, see #97
                    XQueryKeymap(dpy, keys_pressed);
                    if (!(keys_pressed[octet] & kmask)) {
                        msg(1, "Wrong modifier, skip event\n");
                        continue;
                    }
                    if (!g.uiShowHasRun) {
                        uiShow((ev.xkey.state & g.option_backMask));
                    } else {
                        if (ev.xkey.state & g.option_backMask) {
                            uiPrevWindow();
                        } else {
                            uiNextWindow();
                        }
                    }
                } else if (ev.xkey.keycode == g.option_cancelCode) { // escape
                    // additional check, see #97
                    XQueryKeymap(dpy, keys_pressed);
                    if (!(keys_pressed[octet] & kmask)) {
                        msg(1, "Wrong modifier, skip event\n");
                        continue;
                    }
                    uiSelectWindow(0);
                } else {  // non-tab
                    switch (isPrevNextKey(ev.xkey.keycode)) {
                    case 1:
                        uiPrevWindow();
                        break;
                    case 2:
                        uiNextWindow();
                        break;
                    }
                }
            }
            break;

        case KeyRelease:
            msg(1, "Release %lx: %d-%d\n",
                ev.xkey.window, ev.xkey.state, ev.xkey.keycode);
            // interested only in "final" release
            if (!((ev.xkey.state & g.option_modMask)
                  && ev.xkey.keycode == g.option_modCode && g.uiShowHasRun)) {
                break;
            }
            if (g.option_keep_ui) {
                break;
            }
            uiHide();
            break;

        case Expose:
            if (g.uiShowHasRun) {
                uiExpose();
            }
            break;

        case ButtonPress:
        case ButtonRelease:
            uiButtonEvent(ev.xbutton);
            break;

        case PropertyNotify:
            winPropChangeEvent(ev.xproperty);
            break;

        case DestroyNotify:
            winDestroyEvent(ev.xdestroywindow);
            break;

        case FocusIn:
            winFocusChangeEvent(ev.xfocus);
            break;

        default:
            msg(1, "Event type %d\n", ev.type);
            break;
        }

    }

// this is probably never reached
    shutdownWin();
    shutdownGUI();
    XrmDestroyDatabase(db);
    grabKeysAtStartup(false);
// not restoring error handler
    XCloseDisplay(dpy);
    return 0;
} // main
