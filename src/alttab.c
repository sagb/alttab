/*
Parsing options/resources, top-level keygrab functions and main().

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
#include <X11/Xresource.h>
#include <X11/Xft/Xft.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "alttab.h"
#include "util.h"

// PUBLIC

Globals g;
// globals common for alttab, util and icon
Display* dpy;
int scr;
Window root;

// PRIVATE

//
// help and exit
//
void helpexit()
{
	fprintf(stderr, "alttab, the task switcher.\n\
Options:\n\
    -w N      window manager: 0=no, 1=ewmh-compatible, 2=ratpoison, 3=old fashion\n\
    -d N      desktop: 0=current 1=all, 2=all but special\n\
   -sc N      screen: 0=current 1=all\n\
   -mm N      main modifier mask\n\
   -bm N      backward scroll modifier mask\n\
   -kk N      keysym of main modifier\n\
   -mk N      keysym of main key\n\
    -t NxM    tile geometry\n\
    -i NxM    icon geometry\n\
   -vp geo    switcher viewport: focus, pointer, total, WxH+X+Y\n\
    -p str    switcher position: center, none, +X+Y\n\
    -s N      icon source: 0=X11 only, 1=fallback to files, 2=best size, 3=files only\n\
-theme name   icon theme\n\
   -bg color  background color\n\
   -fg color  foreground color\n\
-frame color  active frame color\n\
 -font name   font name in the form xft:fontconfig_pattern\n\
  -v|-vv      verbose\n\
    -h        help\n\
See man alttab for details.\n");
	exit(0);
}

//
// removes argument from argv
//
void remove_arg(int *argc, char **argv, int argn)
{
    int i;
    for(i = argn; i < (*argc); ++i)
    argv[i] = argv[i+1];
    --(*argc);
}

//
// initialize globals based on executable agruments and Xresources
// return 1 if success, 0 otherwise
//
int use_args_and_xrm(int *argc, char **argv)
{
// set debug level early
	g.debug = 0;
	char *endptr;
// not using getopt() because of need for "-v" before Xrm
	int arg;
	for (arg = 0; arg < (*argc); arg++) {
		if ((strcmp(argv[arg], "-v") == 0)) {
			g.debug = 1;
            remove_arg(argc, argv, arg);
        }
        else if ((strcmp(argv[arg], "-vv") == 0)) {
			g.debug = 2;
            remove_arg(argc, argv, arg);
        }
        else if ((strcmp(argv[arg], "-h") == 0)) {
			helpexit();
            remove_arg(argc, argv, arg);
        }
	}
	if (g.debug > 0) {
		fprintf(stderr, "debug level %d\n", g.debug);
	}

    const char* inv = "invalid %s specified, using default\n";
    const char* nosym = "the specified %s keysym is not defined for any keycode, using default\n";

	XrmOptionDescRec xrmTable[] = {
		{"-w", "*windowmanager", XrmoptionSepArg, NULL} ,
		{"-d", "*desktops", XrmoptionSepArg, NULL} ,
        {"-sc", "*screens", XrmoptionSepArg, NULL} ,
		{"-mm", "*modifier.mask", XrmoptionSepArg, NULL} ,
		{"-bm", "*backscroll.mask", XrmoptionSepArg, NULL} ,
		{"-mk", "*modifier.keysym", XrmoptionSepArg, NULL} ,
		{"-kk", "*key.keysym", XrmoptionSepArg, NULL} ,
		{"-t", "*tile.geometry", XrmoptionSepArg, NULL} ,
		{"-i", "*icon.geometry", XrmoptionSepArg, NULL} ,
		{"-vp", "*viewport", XrmoptionSepArg, NULL} ,
		{"-p", "*position", XrmoptionSepArg, NULL} ,
		{"-s", "*icon.source", XrmoptionSepArg, NULL} ,
		{"-theme", "*theme", XrmoptionSepArg, NULL} ,
		{"-bg", "*background", XrmoptionSepArg, NULL} ,
		{"-fg", "*foreground", XrmoptionSepArg, NULL} ,
		{"-frame", "*framecolor", XrmoptionSepArg, NULL} ,
		{"-font", "*font", XrmoptionSepArg, NULL} ,
	};
	XrmDatabase db;
	XrmInitialize();
	char *rm = XResourceManagerString(dpy);
	char *empty = "";
	if (g.debug > 1) {
		fprintf(stderr, "resource manager: \"%s\"\n", rm);
	}
	if (!rm) {
		if (g.debug > 0) {
			fprintf(stderr,
				"can't get resource manager, using empty db\n");
		}
		//return 0;  // we can do it
		//db = XrmGetDatabase (dpy);
		rm = empty;
	}
	db = XrmGetStringDatabase(rm);
	if (!db) {
		fprintf(stderr, "can't get resource database\n");
		return 0;
	}
	XrmParseCommand(&db, xrmTable, sizeof(xrmTable) / sizeof(xrmTable[0]),
			XRMAPPNAME, argc, argv);
    if ((*argc) > 1) {
        fprintf (stderr, "unknown options or wrong arguments:");
        int uo; for (uo = 1; uo < (*argc); uo++) {
            fprintf (stderr, " \"%s\"", argv[uo]);
        }
        fprintf (stderr, ", use -h for help\n");
    }

	XrmValue v;
	char *type;

#define XRESOURCE_LOAD_STRING(NAME, DST, DEFAULT)           \
	XrmGetResource (db, XRMAPPNAME NAME, XCLASS NAME, &type, &v);         \
	if (v.addr != NULL && !strncmp ("String", type, 64))    \
		DST = v.addr;                                       \
    else                                                    \
        DST = DEFAULT;                                      \
    if (g.debug>1) {fprintf (stderr, NAME": %s\n", DST);}

// initializing g.option_wm

	endptr = NULL;
	char *wmindex = NULL;
    XRESOURCE_LOAD_STRING(".windowmanager", wmindex, NULL);
	if (wmindex)
		g.option_wm = strtol(wmindex, &endptr, 0);
	if ((endptr != NULL) && (*endptr == '\0') && (g.option_wm >= WM_MIN)
	    && (g.option_wm <= WM_MAX))
		goto wmDone;
	if (g.debug > 0) {
		fprintf(stderr, "no WM index or unknown, guessing\n");
	}
// EWMH?
	if (ewmh_detectFeatures(&(g.ewmh))) {
		if (g.debug > 0) {
			fprintf(stderr, "EWMH-compatible WM detected: %s\n",
				g.ewmh.wmname);
		}
		g.option_wm = WM_EWMH;
		goto wmDone;
	}
// ratpoison?
	Atom nwm_prop = XInternAtom(dpy, "_NET_WM_NAME", false), atype;
	unsigned char *nwm;
	int form;
	unsigned long remain, len;
	if (XGetWindowProperty(dpy, root, nwm_prop, 0, MAXNAMESZ, false,
			       AnyPropertyType, &atype, &form, &len, &remain,
			       &nwm) == Success && nwm) {
		if (g.debug > 0) {
			fprintf(stderr,
				"_NET_WM_NAME root property present: %s\n",
				nwm);
		}
		if (strstr((char *)nwm, "ratpoison") != NULL) {
			g.option_wm = WM_RATPOISON;
			XFree(nwm);
			goto wmDone;
		}
		XFree(nwm);
	}
	if (g.debug > 0) {
		fprintf(stderr, "unknown WM, using WM_TWM\n");
	}
	g.option_wm = WM_TWM;
 wmDone:
	if (g.debug > 0) {
		fprintf(stderr, "WM: %d\n", g.option_wm);
	}

	endptr = NULL;
	char *dsindex = NULL;
    int ds = DESK_DEFAULT;
    g.option_desktop = DESK_DEFAULT;
	XRESOURCE_LOAD_STRING(".desktops", dsindex, NULL);
	if (dsindex) {
        ds = strtol(dsindex, &endptr, 0);
        if (*dsindex != '\0' && *endptr == '\0') {
            if (ds >= DESK_MIN && ds <= DESK_MAX)
                g.option_desktop = ds;
            else
                fprintf (stderr, "desktops argument must be from %d to %d, using default\n", DESK_MIN, DESK_MAX);
        } else {
            fprintf (stderr, inv, "desktops");
        }
    }
	if (g.debug > 0)
		fprintf(stderr, "desktops: %d\n", g.option_desktop);

	endptr = NULL;
	char *scindex = NULL;
    int sc = SCR_DEFAULT;
    g.option_screen = SCR_DEFAULT;
	XRESOURCE_LOAD_STRING(".screens", scindex, NULL);
	if (scindex) {
        sc = strtol(scindex, &endptr, 0);
        if (*scindex != '\0' && *endptr == '\0') {
            if (sc >= SCR_MIN && sc <= SCR_MAX)
                g.option_screen = sc;
            else
                fprintf (stderr, 
                  "screens argument must be from %d to %d, using default\n", 
                  SCR_MIN, SCR_MAX);
        } else {
            fprintf (stderr, inv, "screens");
        }
    }
	if (g.debug > 0)
		fprintf(stderr, "screens: %d\n", g.option_screen);

	unsigned int defaultModMask = DEFMODMASK;
	unsigned int defaultBackMask = DEFBACKMASK;
	KeySym defaultModSym = DEFMODKS;
	KeySym defaultKeySym = DEFKEYKS;
	char *s;
	KeySym ksym = defaultModSym;
	unsigned int mask = defaultModMask;

    g.option_modMask = defaultModMask;
	endptr = s = NULL;
	XRESOURCE_LOAD_STRING(".modifier.mask", s, NULL);
	if (s) {
		mask = strtol(s, &endptr, 0);
        if (*s != '\0' && *endptr == '\0')
            g.option_modMask = mask;
        else
            fprintf (stderr, inv, "modifier mask");
    }

    g.option_backMask = defaultBackMask;
	endptr = s = NULL;
	XRESOURCE_LOAD_STRING(".backscroll.mask", s, NULL);
	if (s) {
		mask = strtol(s, &endptr, 0);
        if (*s != '\0' && *endptr == '\0')
            g.option_backMask = mask;
        else
            fprintf (stderr, inv, "backscroll mask");
    }

    g.option_modCode = XKeysymToKeycode(dpy, defaultModSym);
	endptr = s = NULL;
	XRESOURCE_LOAD_STRING(".modifier.keysym", s, NULL);
	if (s) {
		ksym = strtol(s, &endptr, 0);
        if (*s != '\0' && *endptr == '\0') {
            g.option_modCode = XKeysymToKeycode(dpy, ksym);
            if (g.option_modCode == 0) {
                fprintf (stderr, nosym, "modifier");
                g.option_modCode = XKeysymToKeycode(dpy, defaultModSym);
            }
        } else {
            fprintf (stderr, inv, "modifier keysym");
        }
    }

    g.option_keyCode = XKeysymToKeycode(dpy, defaultKeySym);
	endptr = s = NULL;
	XRESOURCE_LOAD_STRING(".key.keysym", s, NULL);
	if (s) {
		ksym = strtol(s, &endptr, 0);
        if (*s != '\0' && *endptr == '\0') {
            g.option_keyCode = XKeysymToKeycode(dpy, ksym);
            if (g.option_keyCode == 0) {
                fprintf (stderr, nosym, "main key");
                g.option_keyCode = XKeysymToKeycode(dpy, defaultKeySym);
            }
        } else {
            fprintf (stderr, inv, "main key keysym");
        }
    }

	if (g.debug > 0) {
		fprintf(stderr,
			"modMask %d, backMask %d, modCode %d, keyCode %d\n",
			g.option_modMask, g.option_backMask, g.option_modCode,
			g.option_keyCode);
	}

	char *gtile, *gicon, *gview, *gpos;
	int x, y;
	unsigned int w, h;
	int xpg;

    g.option_tileW = DEFTILEW;
    g.option_tileH = DEFTILEH;
	char *defaultTileGeo = DEFTILE;
	XRESOURCE_LOAD_STRING(".tile.geometry", gtile,
			      defaultTileGeo);
    if (gtile) {
    	xpg = XParseGeometry(gtile, &x, &y, &w, &h);
        if (xpg & WidthValue)
        	g.option_tileW = w;
        else
            fprintf (stderr, inv, "tile width");
        if (xpg & HeightValue)
        	g.option_tileH = h;
        else
            fprintf (stderr, inv, "tile height");
    }

    g.option_iconW = DEFICONW;
    g.option_iconH = DEFICONH;
	char *defaultIconGeo = DEFICON;
	XRESOURCE_LOAD_STRING(".icon.geometry", gicon,
			      defaultIconGeo);
    if (gicon) {
    	xpg = XParseGeometry(gicon, &x, &y, &w, &h);
        if (xpg & WidthValue)
        	g.option_iconW = w;
        else
            fprintf (stderr, inv, "icon width");
        if (xpg & HeightValue)
        	g.option_iconH = h;
        else
            fprintf (stderr, inv, "icon height");
    }

	if (g.debug > 0) {
		fprintf(stderr, "%dx%d tile, %dx%d icon\n",
			g.option_tileW, g.option_tileH, g.option_iconW,
			g.option_iconH);
	}

    bzero (&(g.option_vp), sizeof(g.option_vp));
    g.option_vp_mode = VP_DEFAULT;
	char *defaultViewGeo = DEFVP;
	XRESOURCE_LOAD_STRING(".viewport", gview,
			      defaultViewGeo);
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
                fprintf (stderr, inv, "viewport");
                g.option_vp_mode = VP_DEFAULT;
            }
        }
    }
	if (g.debug > 0) {
		fprintf(stderr, "viewport: mode %d, %dx%d+%d+%d\n",
            g.option_vp_mode,
            g.option_vp.w, g.option_vp.h,
            g.option_vp.x, g.option_vp.y);
	}

    g.option_positioning = POS_DEFAULT;
    g.option_posX = 0;
    g.option_posY = 0;
	char *defaultPosStr = DEFPOS;
	XRESOURCE_LOAD_STRING(".position", gpos,
			      defaultPosStr);
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
                fprintf (stderr, inv, "position");
                g.option_positioning = POS_DEFAULT;
            }
        }
    }
	if (g.debug > 0) {
		fprintf(stderr, "positioning policy: %d, position: +%d+%d\n",
			g.option_positioning, g.option_posX, g.option_posY);
	}

	endptr = NULL;
	char *isrcindex = NULL;
    int isrc = ISRC_DEFAULT;
    g.option_iconSrc = ISRC_DEFAULT;
	XRESOURCE_LOAD_STRING(".icon.source", isrcindex, NULL);
	if (isrcindex) {
        isrc = strtol(isrcindex, &endptr, 0);
        if (*isrcindex != '\0' && *endptr == '\0') {
            if (isrc >= ISRC_MIN && isrc <= ISRC_MAX)
                g.option_iconSrc = isrc;
            else
                fprintf (stderr, "icon source argument must be from %d to %d, using default\n", ISRC_MIN, ISRC_MAX);
        } else {
            fprintf (stderr, inv, "icon source");
        }
    }
	if (g.debug > 0)
		fprintf(stderr, "icon source: %d\n", g.option_iconSrc);

	char *defaultTheme = DEFTHEME;
    g.option_theme = NULL;
	XRESOURCE_LOAD_STRING(".theme", g.option_theme, defaultTheme);
    if (!g.option_theme)
		g.option_theme = defaultTheme;
	if (g.debug > 0)
		fprintf(stderr, "icon theme: %s\n", g.option_theme);

	char *defaultColorBG = DEFCOLBG;
	XRESOURCE_LOAD_STRING(".background", g.color[COLBG].name,
			      defaultColorBG);
	char *defaultColorFG = DEFCOLFG;
	XRESOURCE_LOAD_STRING(".foreground", g.color[COLFG].name,
			      defaultColorFG);
	char *defaultColorFrame = DEFCOLFRAME;
	XRESOURCE_LOAD_STRING(".framecolor", g.color[COLFRAME].name,
			      defaultColorFrame);

	char *defaultFont = DEFFONT;
	XRESOURCE_LOAD_STRING(".font", g.option_font, defaultFont);
	if ((strncmp(g.option_font, "xft:", 4) == 0)
	    && (*(g.option_font + 4) != '\0')) {
		g.option_font += 4;
	} else {
        fprintf(stderr,
                "invalid font: \"%s\", using default: %s\n",
                g.option_font, defaultFont);
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
int grabAllKeys(bool grabUngrab)
{
	g.ignored_modmask = getOffendingModifiersMask(dpy);	// or 0 for g.debug
	char *grabhint =
	    "Error while (un)grabbing key 0x%x with mask 0x%x/0x%x.\nProbably other program already grabbed this combination.\nCheck: xdotool keydown alt+Tab; xdotool key XF86LogGrabInfo; xdotool keyup Tab; sleep 1; xdotool keyup alt; tail /var/log/Xorg.0.log\nOr try Ctrl-Tab instead of Alt-Tab:  alttab -mm 4 -mk 0xffe3\n";
// attempt XF86Ungrab? probably too invasive
	if (!changeKeygrab
	    (root, grabUngrab, g.option_keyCode, g.option_modMask,
	     g.ignored_modmask)) {
		fprintf(stderr, grabhint, g.option_keyCode,
            g.option_modMask, g.ignored_modmask);
		exit(1);
	}
	if (!changeKeygrab
	    (root, grabUngrab, g.option_keyCode,
	     g.option_modMask | g.option_backMask, g.ignored_modmask)) {
		fprintf(stderr, grabhint, g.option_keyCode,
			g.option_modMask | g.option_backMask, g.ignored_modmask);
		exit(1);
	}
	return 1;
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
    XErrorHandler hnd = XSetErrorHandler (zeroErrorHandler); // for entire program
    if (hnd) ;; // make -Wunused happy

	if (!use_args_and_xrm(&argc, argv))
		die("use_args_and_xrm failed");
	if (!startupWintasks())
		die("startupWintasks failed");
	if (!startupGUItasks())
		die("startupGUItasks failed");

	grabAllKeys(true);
	g.uiShowHasRun = false;

	struct timespec nanots;
	nanots.tv_sec = 0;
	nanots.tv_nsec = 1E7;
	char keys_pressed[32];
	int octet = g.option_modCode / 8;
	int kmask = 1 << (g.option_modCode - octet * 8);

	while (true) {
		memset(&(ev.xkey), 0, sizeof(ev.xkey));

		if (g.uiShowHasRun) {
			// poll: lag and consume cpu, but necessary because of bug #1 and #2
			nanosleep(&nanots, NULL);
			XQueryKeymap(dpy, keys_pressed);
			if (!(keys_pressed[octet] & kmask)) {	// Alt released
				uiHide();
				continue;
			}
			if (!XCheckIfEvent(dpy, &ev, *predproc_true, NULL))
				continue;
		} else {
			// event: immediate, when we don't care about Alt release
			XNextEvent(dpy, &ev);
		}

		switch (ev.type) {
		case KeyPress:
			if (g.debug > 1) {
				fprintf(stderr, "Press %lx: %d-%d\n",
					ev.xkey.window, ev.xkey.state,
					ev.xkey.keycode);
			}
			if (!
			    ((ev.xkey.state & g.option_modMask)
			     && ev.xkey.keycode == g.option_keyCode)) {
				break;
			}	// safety redundance
			if (!g.uiShowHasRun) {
				uiShow((ev.xkey.state & g.option_backMask));
			} else {
				if (ev.xkey.state & g.option_backMask) {
					uiPrevWindow();
				} else {
					uiNextWindow();
				}
			}
			break;

		case KeyRelease:
			if (g.debug > 1) {
				fprintf(stderr, "Release %lx: %d-%d\n",
					ev.xkey.window, ev.xkey.state,
					ev.xkey.keycode);
			}
			// interested only in "final" release
			if (!
			    ((ev.xkey.state & g.option_modMask)
			     && ev.xkey.keycode == g.option_modCode
			     && g.uiShowHasRun)) {
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

		default:
			if (g.debug > 1) {
				fprintf(stderr, "Event type %d\n", ev.type);
			}
			break;
		}

	}

// this is probably never reached
	grabAllKeys(false);
// not restoring error handler
	XCloseDisplay(dpy);
	return 0;
}				// main
