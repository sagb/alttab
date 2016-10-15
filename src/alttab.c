/*
Parsing options/resources, top-level keygrab functions and main().

This file is part of alttab program.
alttab is Copyright (C) 2016, by respective author (sa).
It is free software; you can redistribute it and/or modify it under the terms of either:
a) the GNU General Public License as published by the Free Software Foundation; either version 1, or (at your option) any later version, or
b) the "Artistic License".
*/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xft/Xft.h>
#include <stdbool.h>             
#include <stdio.h>               
#include <string.h>               
#include "alttab.h"
#include "util.h"

// PUBLIC

Globals g;

// PRIVATE

//
// help and exit
//
void helpexit()
{
fprintf (stderr, "alttab, the task switcher.\n\
Options:\n\
    -w N      window manager: 0=no, 1=ewmh-compatible, 2=ratpoison\n\
   -mm N      main modifier mask\n\
   -bm N      backward scroll modifier mask\n\
   -kk N      keysym of main modifier\n\
   -mk N      keysym of main key\n\
    -t NxM    tile geometry\n\
    -i NxM    icon geometry\n\
   -bg color  background color\n\
   -fg color  foreground color\n\
-frame color  active frame color\n\
 -font name   font name in the form xft:fontconfig_pattern\n\
  -v|-vv      verbose\n\
    -h        help\n\
See man alttab for details.\n");
exit (0);
}


//
// initialize globals based on executable agruments and Xresources
// return 1 if success, 0 otherwise
//
int use_args_and_xrm (Display* dpy, Window root, int argc, char ** argv)
{
// set debug level early
g.debug=0;
char* endptr;
// not using getopt() because of need for "-v" before Xrm
int arg; for (arg=0; arg<argc; arg++) {
    if ((strcmp(argv[arg], "-v")==0))
        g.debug=1;
    else if ((strcmp(argv[arg], "-vv")==0))
        g.debug=2;
    else if ((strcmp(argv[arg], "-h")==0))
        helpexit();
}
if (g.debug>0) {fprintf (stderr, "debug level %d\n", g.debug);}

XrmOptionDescRec xrmTable[] = {
	{"-w", "*windowmanager", XrmoptionSepArg, NULL},
	{"-mm", "*modifier.mask", XrmoptionSepArg, NULL},
	{"-bm", "*backscroll.mask", XrmoptionSepArg, NULL},
	{"-mk", "*modifier.keysym", XrmoptionSepArg, NULL},
	{"-kk", "*key.keysym", XrmoptionSepArg, NULL},
	{"-t", "*tile.geometry", XrmoptionSepArg, NULL},
	{"-i", "*icon.geometry", XrmoptionSepArg, NULL},
	{"-bg", "*background", XrmoptionSepArg, NULL},
	{"-fg", "*foreground", XrmoptionSepArg, NULL},
	{"-frame", "*framecolor", XrmoptionSepArg, NULL},
	{"-font", "*font", XrmoptionSepArg, NULL},
};
XrmDatabase db;
XrmInitialize();
char* rm = XResourceManagerString (dpy);
char* empty = "";
if (g.debug>1) {fprintf (stderr, "resource manager: \"%s\"\n", rm);}
if (!rm) {
    if (g.debug>0) {fprintf (stderr, "can't get resource manager, using empty db\n");}
    //return 0;  // we can do it
    //db = XrmGetDatabase (dpy);
    rm = empty;
}
db = XrmGetStringDatabase (rm);
if (!db) {
    fprintf (stderr, "can't get resource database\n");
    return 0;
}
XrmParseCommand (&db, xrmTable, sizeof(xrmTable)/sizeof(xrmTable[0]), XRMAPPNAME, &argc, argv);

XrmValue v;
char* type;

#define XRESOURCE_LOAD_STRING(NAME, DST, DEFAULT)           \
	XrmGetResource (db, NAME, "String", &type, &v);         \
	if (v.addr != NULL && !strncmp ("String", type, 64))    \
		DST = v.addr;                                       \
    else                                                    \
        DST = DEFAULT;                                      \
    if (g.debug>1) {fprintf (stderr, NAME": %s\n", DST);}

// initializing g.option_wm

endptr = NULL;
char* wmindex = NULL;
XRESOURCE_LOAD_STRING(XRMAPPNAME".windowmanager", wmindex, NULL);
if (wmindex) g.option_wm = strtol (wmindex, &endptr, 0);
if ((endptr!=NULL) && (*endptr=='\0') && (g.option_wm>=WM_MIN) && (g.option_wm<=WM_MAX)) goto wmDone;
if (g.debug>0) {fprintf (stderr, "no WM index or unknown, guessing\n");}
// EWMH?
char* ewmn = ewmh_wmName (dpy, root);
if (ewmn!=NULL) {
    if (g.debug>0) {fprintf(stderr, "EWMH-compatible WM detected: %s\n", ewmn);}
    g.option_wm = WM_EWMH;
    goto wmDone;
}
// ratpoison?
Atom nwm_prop = XInternAtom (dpy, "_NET_WM_NAME", false), atype;
unsigned char* nwm;
int form; unsigned long remain, len;
if (XGetWindowProperty (dpy, root, nwm_prop, 0, MAXNAMESZ, false,
        AnyPropertyType, &atype, &form, &len, &remain, &nwm
        ) == Success && nwm) {
    if (g.debug>0) {fprintf (stderr, "_NET_WM_NAME root property present: %s\n", nwm);}
    if (strstr (nwm, "ratpoison")!=NULL) {
        g.option_wm = WM_RATPOISON;
        XFree (nwm);
        goto wmDone;
    }
    XFree (nwm);
}
if (g.debug>0) {fprintf (stderr, "unknown WM, using raw X\n");}
g.option_wm = WM_NO;
wmDone:
if (g.debug>0) {fprintf (stderr, "WM: %d\n", g.option_wm);}

unsigned int defaultModMask = DEFMODMASK;
unsigned int defaultBackMask = DEFBACKMASK;
KeySym defaultModSym = DEFMODKS;
KeySym defaultKeySym = DEFKEYKS;
char *s; KeySym ksym; unsigned int mask;
// TODO: thorough validation?
endptr = s = NULL;
XRESOURCE_LOAD_STRING(XRMAPPNAME".modifier.mask", s, NULL);
if (s) mask = strtol (s, &endptr, 0);
g.option_modMask = ((endptr==NULL) || (*endptr!='\0')) ? defaultModMask : mask;
endptr = s = NULL;
XRESOURCE_LOAD_STRING(XRMAPPNAME".backscroll.mask", s, NULL);
if (s) mask = strtol (s, &endptr, 0);
g.option_backMask = ((endptr==NULL) || (*endptr!='\0')) ? defaultBackMask : mask;
endptr = s = NULL;
XRESOURCE_LOAD_STRING(XRMAPPNAME".modifier.keysym", s, NULL);
if (s) ksym = strtol (s, &endptr, 0);
if ((endptr==NULL) || (*endptr!='\0')) ksym = defaultModSym;
g.option_modCode = XKeysymToKeycode (dpy, ksym);
endptr = s = NULL;
XRESOURCE_LOAD_STRING(XRMAPPNAME".key.keysym", s, NULL);
if (s) ksym = strtol (s, &endptr, 0);
if ((endptr==NULL) || (*endptr!='\0')) ksym = defaultKeySym;
g.option_keyCode = XKeysymToKeycode (dpy, ksym);
if (g.debug>0) {fprintf (stderr, "modMask %d, backMask %d, modCode %d, keyCode %d\n", 
        g.option_modMask, g.option_backMask, g.option_modCode, g.option_keyCode);}

char* defaultTileGeo = DEFTILE;
char *gtile, *gicon;
int x, y; unsigned int w, h;
int xpg;
XRESOURCE_LOAD_STRING(XRMAPPNAME".tile.geometry", gtile, defaultTileGeo);
xpg = XParseGeometry (gtile, &x, &y, &w, &h);
g.option_tileW = (xpg & WidthValue) ? w : DEFTILEW;
g.option_tileH = (xpg & HeightValue) ? h : DEFTILEH;

char* defaultIconGeo = DEFICON;
XRESOURCE_LOAD_STRING(XRMAPPNAME".icon.geometry", gicon, defaultIconGeo);
xpg = XParseGeometry (gicon, &x, &y, &w, &h);
g.option_iconW = (xpg & WidthValue) ? w : DEFICONW;
g.option_iconH = (xpg & HeightValue) ? h : DEFICONH;
if (g.debug>0) {fprintf (stderr, "%dx%d tile, %dx%d icon\n", 
        g.option_tileW, g.option_tileH, g.option_iconW, g.option_iconH);}

char* defaultColorBG = DEFCOLBG;
XRESOURCE_LOAD_STRING(XRMAPPNAME".background", g.color[COLBG].name, defaultColorBG);
char* defaultColorFG = DEFCOLFG;
XRESOURCE_LOAD_STRING(XRMAPPNAME".foreground", g.color[COLFG].name, defaultColorFG);
char* defaultColorFrame = DEFCOLFRAME;
XRESOURCE_LOAD_STRING(XRMAPPNAME".framecolor", g.color[COLFRAME].name, defaultColorFrame);

char* defaultFont = DEFFONT;
XRESOURCE_LOAD_STRING(XRMAPPNAME".font", g.option_font, defaultFont);
if ((strncmp(g.option_font, "xft:", 4)==0) && (*(g.option_font+4)!='\0')) {
    g.option_font += 4;
} else {
    if (g.debug>0) {fprintf (stderr, "invalid font: \"%s\", using default: %s\n", g.option_font, defaultFont);}
    g.option_font = defaultFont + 4;
}

// max recursion for searching windows
// -1 is "everything" 
// in raw X this returns too much windows, "1" is probably sufficient
// no need for an option
g.option_max_reclevel = (g.option_wm == WM_NO) ? 1 : -1;

return 1;
}


//
// grab Alt-Tab and Alt
// note: exit() on failure
//
int grabAllKeys (Display* dpy, Window root, bool grabUngrab)
{
g.ignored_modmask = getOffendingModifiersMask (dpy); // or 0 for g.debug
char* grabhint = "Error while (un)grabbing key %d with mask %d.\nProbably other program already grabbed this combination.\nCheck: xdotool keydown alt+Tab; xdotool key XF86LogGrabInfo; xdotool keyup Tab; sleep 1; xdotool keyup alt; tail /var/log/Xorg.0.log\nOr try Ctrl-Tab instead of Alt-Tab:  -mm 4 -mk 0xffe3\n";
// TODO: attempt XF86Ungrab? probably too invasive
// TODO: error message doesn't count ignored_modmask
if (!changeKeygrab (dpy, root, grabUngrab, g.option_keyCode, g.option_modMask, g.ignored_modmask)) {
    fprintf (stderr, grabhint, g.option_keyCode, g.option_modMask);
    exit (1);
}
if (!changeKeygrab (dpy,root, grabUngrab, g.option_keyCode, g.option_modMask|g.option_backMask, g.ignored_modmask)) {
    fprintf (stderr, grabhint, g.option_keyCode, g.option_modMask|g.option_backMask);
    exit (1);
}
return 1;
}


int main (int argc, char ** argv)
{                 

XEvent ev;
Display* dpy = XOpenDisplay (NULL);
Window root = DefaultRootWindow (dpy);

ee_complain = true;
XErrorHandler hnd = (XErrorHandler)0;
hnd = XSetErrorHandler (zeroErrorHandler); // forever

if (!use_args_and_xrm (dpy, root, argc, argv))
    die("use_args_and_xrm failed");
if (!startupWintasks (dpy))
    die("startupWintasks failed");
if (!startupGUItasks (dpy, root))
    die("startupGUItasks failed");

setSelectInput (dpy, root, True);
grabAllKeys (dpy, root, true);
g.uiShowHasRun = false;

while(true)
{
    memset (&(ev.xkey), sizeof(ev.xkey), 0);
    XNextEvent(dpy, &ev);
    switch(ev.type)
    {
        case KeyPress:
            if (g.debug>1) {fprintf (stderr, "Press %x: %d-%d\n", ev.xkey.window, ev.xkey.state, ev.xkey.keycode);}
            if (! ((ev.xkey.state & g.option_modMask) && ev.xkey.keycode == g.option_keyCode) ) { break; } // safety redundance
            if (!g.uiShowHasRun) {
                uiShow (dpy, root, (ev.xkey.state & g.option_backMask));
            } else {
                if (ev.xkey.state & g.option_backMask) {
                    uiPrevWindow (dpy, root);
                } else {
                    uiNextWindow (dpy, root);
                }
            }
            break;

        case KeyRelease:
            if (g.debug>1) {fprintf (stderr, "Release %x: %d-%d\n", ev.xkey.window, ev.xkey.state, ev.xkey.keycode);}
            // interested only in "final" release
            if (! ((ev.xkey.state & g.option_modMask) && ev.xkey.keycode == g.option_modCode && g.uiShowHasRun) ) { break; }
            uiHide (dpy, root);
            break;

        case MapNotify:
            if (g.debug>1) {fprintf (stderr, "Mapped %x\n", ev.xmap.window);}
            setSelectInput (dpy, ev.xmap.window, True);
            break;

        case UnmapNotify:
            if (g.debug>1) {fprintf (stderr, "Unmapped %x\n", ev.xunmap.window);}
            setSelectInput (dpy, ev.xunmap.window, False);
            break;

        case Expose:
            if (g.uiShowHasRun) {
                uiExpose (dpy, root);
            }
            break;

        default:
            if (g.debug>1) {fprintf (stderr, "Event type %d\n", ev.type);}
            break;
    }

}

// this is probably never reached
grabAllKeys (dpy, root, false);
// not restoring error handler
XCloseDisplay (dpy);
return 0;
} // main

