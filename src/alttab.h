/*
Global includes.

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

#ifndef ALTTAB_H
#define ALTTAB_H

#define MAXNAMESZ   256
#define MAXPATHSZ   8200
#define MAXRPOUT    8200
#define MAXWINDOWS  1024

// as there are no files involved, we can die at any time
// BUT? "it is a good idea to free all the pixmaps that your program created before exiting from the program, pixmaps are stored in the server, if they are not freed they could remain allocated after the program exits"
#define die(e)      do { fprintf(stderr, "%s\n", e); exit(1); } while (0);
#define die2(e,f)   do { fprintf(stderr, "%s%s\n", e, f); exit(1); } while (0);

#define XWINNAME    "alttab"
#define XRMAPPNAME  XWINNAME
#define XCLASSNAME  XWINNAME
#define XCLASS      "AltTab"
#define DEFTILEW    112
#define DEFTILEH    128
#define DEFTILE     "112x128"
#define DEFICONW    32
#define DEFICONH    32
#define DEFICON     "32x32"
#define DEFPOS      "center"
#define DEFTHEME    "hicolor"
#define FRAME_W     8
//#define DEFFONT   "xft:DejaVu Sans Condensed-10"
#define DEFFONT     "xft:sans-10"

#define COLBG       0
#define COLFG       1
#define COLFRAME    2
#define NCOLORS     3
#define DEFCOLBG    "black"
#define DEFCOLFG    "grey"
#define DEFCOLFRAME "#a0abab"

#define XDEPTH      24		// TODO: get rid of this

#define DEFMODMASK  Mod1Mask
#define DEFBACKMASK ShiftMask
#define DEFMODKS    XK_Alt_L
#define DEFKEYKS    XK_Tab

#include "icon.h"

typedef struct {
	Window id;
	int wm_id;		// wm's internal window id, when WM has it (ratpoison)
	char name[MAXNAMESZ];
	int reclevel;
	Pixmap icon_drawable;	// Window or Pixmap
    Pixmap icon_mask;
	unsigned int icon_w, icon_h;
	bool icon_allocated;	// we must free icon, because we created it (placeholder or depth conversion)
	Pixmap tile;		// ready to display. w/h are all equal and defined in gui.c
	int order;		// in sort stack, kept in sync with g.sortlist
// this constant can't be 0, 1, -1, MAXINT, 
// because WMs set it to these values incoherently
#define DESKTOP_UNKNOWN 0xdead
    unsigned long desktop;
} WindowInfo;

typedef struct {
	//char name[MAXNAMESZ];
	char *name;
	XColor xcolor;
	XftColor xftcolor;
	//XRenderColor xrendercolor;
} Color;

typedef struct {
    char *wmname;
    bool try_stacking_list_first;
    bool minus1_desktop_unusable;
} EwmhFeatures;

typedef struct {
	int debug;
	bool uiShowHasRun;	// means: 1. window is ready to Expose, 2. need to call uiHide to free X stuff
	WindowInfo *winlist;
	int maxNdx;		// number of items in list above
	int selNdx;		// current (selected) item
	int startNdx;		// current item at start of uiShow (current window before setFocus)
	Window sortlist[MAXWINDOWS];	// auxiliary list for sorting
	// display-wide, for all groups/desktops
	// unlike g.winlist, survives uiHide
	// for each uiShow, g.winlist[].order is initialized using this list
	int sortNdx;		// number of elements in list above
	// option_* are initialized from command line arguments or X resources or defaults
	int option_max_reclevel;	// max reclevel. -1 is "everything"
#define WM_MIN          0
#define WM_NO           0
#define WM_EWMH         1
#define WM_RATPOISON    2
#define WM_TWM          3
#define WM_MAX          3
	int option_wm;
#define DESK_MIN        0
#define DESK_CURRENT    0
#define DESK_ALL        1
#define DESK_NOSPECIAL  2
#define DESK_MAX        2
#define DESK_DEFAULT    0
    int option_desktop;
	char *option_font;
	int option_tileW, option_tileH;
	int option_iconW, option_iconH;
#define POS_CENTER      0
#define POS_NONE        1
#define POS_SPECIFIC    2
    int option_positioning;
    int option_posX, option_posY;
#define ISRC_MIN        0
#define ISRC_RAM        0
#define ISRC_FALLBACK   1
#define ISRC_SIZE       2
#define ISRC_FILES      3
#define ISRC_MAX        3
#define ISRC_DEFAULT    2
    int option_iconSrc;
    char *option_theme;
	unsigned int option_modMask, option_backMask;
	KeyCode option_modCode, option_keyCode;
	Color color[NCOLORS];
	GC gcDirect, gcReverse, gcFrame;	// used in both gui.c and win.c
	unsigned int ignored_modmask;
    icon_t *ic;  // cache of all icons
    EwmhFeatures ewmh;  // guessed by ewmh_detectFeatures
    Atom naw;  // _NET_ACTIVE_WINDOW
} Globals;

// gui
int startupGUItasks();
int uiShow(bool direction);
void uiExpose();
int uiHide();
int uiNextWindow();
int uiPrevWindow();
int uiSelectWindow(int ndx);
void uiButtonEvent(XButtonEvent e);
Window getUiwin();

// windows
int startupWintasks();
int addIconFromHints (WindowInfo* wi);
int addIconFromFiles (WindowInfo* wi);
int addWindowInfo(Window win, int reclevel, int wm_id, unsigned long desktop, char *wm_name);
int initWinlist(bool direction);
void freeWinlist();
int setFocus(int winNdx);
int rp_startupWintasks();
int x_initWindowsInfoRecursive(Window win, int reclevel);
int rp_initWinlist();
int x_setFocus(int wndx);
int rp_setFocus(int winNdx);
int execAndReadStdout(char *exe, char *args[], char *buf, int bufsize);
int pulloutWindowToTop(int winNdx);
void winPropChangeEvent(XPropertyEvent e);

/* EWHM */
bool ewmh_detectFeatures(EwmhFeatures *e);
Window ewmh_getActiveWindow();
int ewmh_initWinlist();
int ewmh_setFocus(int winNdx, Window fwin); // fwin used if non-zero
unsigned long ewmh_getCurrentDesktop();
unsigned long ewmh_getDesktopOfWindow(Window w);
bool ewmh_skipWindowInTaskbar(Window w);

#endif
