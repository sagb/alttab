/*
Global includes.

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

#ifndef ALTTAB_H
#define ALTTAB_H

#define MAXPATHSZ   8200
#define MAXRPOUT    8200

// as there are no files involved, we can die at any time
// BUT? "it is a good idea to free all the pixmaps that your program created before exiting from the program, pixmaps are stored in the server, if they are not freed they could remain allocated after the program exits"

#define XWINNAME    "alttab"
#define XRMAPPNAME  XWINNAME
#define XCLASSNAME  XWINNAME
#define MSGPREFIX   "alttab: "
#define XCLASS      "AltTab"
#define DEFTILEW    112
#define DEFTILEH    128
#define DEFICONW    32
#define DEFICONH    32
#define DEFBORDERW  0
#define DEFTHEME    "hicolor"
#define FRAME_W     8
//#define DEFFONT   "xft:DejaVu Sans Condensed-10"
#define DEFFONT     "xft:sans-10"

#define COLBG       0
#define COLFG       1
#define COLFRAME    2
#define COLBORDER   3
#define COLINACT    4
#define NCOLORS     5
#define DEFCOLBG    "black"
#define DEFCOLFG    "grey"
#define DEFCOLFRAME "#a0abab"
#define DEFCOLBORDER "black"

// #define XDEPTH      24          // TODO: get rid of this
#define XDEPTH      30          // TODO: get rid of this

#define DEFMODMASK  Mod1Mask
#define DEFBACKMASK ShiftMask
#define DEFMODKS    XK_Alt_L
#define DEFKEYKS    XK_Tab
#define DEFPREVKEYKS    XK_VoidSymbol
#define DEFNEXTKEYKS    XK_VoidSymbol
#define DEFCANCELKS XK_Escape

#include "icon.h"

#ifndef COMTYPES
#define COMTYPES
typedef struct {
    int w;
    int h;
    int x;
    int y;
} quad;
#define MAXNAMESZ   256
#endif

typedef struct {
    Window id;
    int wm_id;                  // wm's internal window id, when WM has it (ratpoison)
    char name[MAXNAMESZ];
    int reclevel;
    Pixmap icon_drawable;       // Window or Pixmap
    Pixmap icon_mask;
    unsigned int icon_w, icon_h;
    bool icon_allocated;        // we must free icon, because we created it (placeholder or depth conversion)
    Pixmap tile;                // ready to display. w/h are all equal and defined in gui.c
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

// uthash doubly-linked list element
typedef struct PermanentWindowInfo {
    Window id;
    struct PermanentWindowInfo *next, *prev;
} PermanentWindowInfo;

/*
typedef struct SwitchMoment {
    Window prev;
    Window to;
    struct timeval tv;
} SwitchMoment;
*/

typedef struct {
    int debug;
    bool uiShowHasRun;          // means: 1. window is ready to Expose, 2. need to call uiHide to free X stuff
    WindowInfo *winlist;
    int maxNdx;                 // number of items in list above
    /* auxiliary list for sorting
     * head = recently focused
     * display-wide, for all groups/desktops
     * unlike g.winlist, survives uiHide */
    PermanentWindowInfo *sortlist;
    // option_* are initialized from command line arguments or X resources or defaults
    int option_max_reclevel;    // max reclevel. -1 is "everything"
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
#define DESK_NOCURRENT  3
#define DESK_MAX        3
#define DESK_DEFAULT    DESK_CURRENT
    int option_desktop;
#define SCR_MIN        0
#define SCR_CURRENT    0
#define SCR_ALL        1
#define SCR_MAX        1
#define SCR_DEFAULT    SCR_ALL
    int option_screen;
    char *option_font;
#define VERTICAL_DEFAULT false
    bool option_vertical;
    int option_tileW, option_tileH;
    int option_iconW, option_iconH;
#define BORDER_MIN      0
    int option_borderW;
#define VP_FOCUS        0
#define VP_POINTER      1
#define VP_TOTAL        2
#define VP_SPECIFIC     3
#define VP_DEFAULT      VP_FOCUS
    int option_vp_mode;
    quad option_vp;
    quad vp;
    bool has_randr;
#define POS_CENTER      0
#define POS_NONE        1
#define POS_SPECIFIC    2
#define POS_DEFAULT     POS_CENTER
    int option_positioning;
    int option_posX, option_posY;
#define ISRC_MIN        0
#define ISRC_RAM        0
#define ISRC_FALLBACK   1
#define ISRC_SIZE       2
#define ISRC_FILES      3
#define ISRC_MAX        3
#define ISRC_DEFAULT    ISRC_SIZE
    int option_iconSrc;
    char *option_theme;
    unsigned int option_modMask, option_backMask;
    KeyCode option_modCode, option_keyCode;
    KeyCode option_prevCode, option_nextCode;
    KeyCode option_cancelCode;
    Color color[NCOLORS];
    GC gcDirect, gcReverse, gcFrame;    // used in both gui.c and win.c
    unsigned int ignored_modmask;
    icon_t *ic;                 // cache of all icons
    EwmhFeatures ewmh;          // guessed by ewmh_detectFeatures
    Atom naw;                   // _NET_ACTIVE_WINDOW
//    SwitchMoment last; // for detecting false focus events from WM
    bool option_keep_ui;
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
void shutdownGUI(void);

// windows
int startupWintasks();
int addIconFromProperty(WindowInfo * wi);
int addIconFromHints(WindowInfo * wi);
int addIconFromFiles(WindowInfo * wi);
int addWindowInfo(Window win, int reclevel, int wm_id, unsigned long desktop,
                  char *wm_name);
int initWinlist(void);
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
void winDestroyEvent(XDestroyWindowEvent e);
void winFocusChangeEvent(XFocusChangeEvent e);
bool common_skipWindow(Window w, unsigned long current_desktop,
                       unsigned long window_desktop);
void x_setCommonPropertiesForAnyWindow(Window win);
void addToSortlist(Window w, bool to_head, bool move);
void shutdownWin(void);

/* EWHM */
bool ewmh_detectFeatures(EwmhFeatures * e);
Window ewmh_getActiveWindow();
int ewmh_initWinlist();
int ewmh_setFocus(int winNdx, Window fwin); // fwin used if non-zero
unsigned long ewmh_getCurrentDesktop();
unsigned long ewmh_getDesktopOfWindow(Window w);
bool ewmh_skipWindowInTaskbar(Window w);

/* RANDR */
bool randrAvailable();
bool randrGetViewport(quad * res, bool * multihead);

/* autil */
void die(const char *format, ...);
void msg(int lvl, const char *format, ...);
void sighandler(int signum);

#endif
