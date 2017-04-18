/*
Global includes.

This file is part of alttab program.
alttab is Copyright (C) 2016-2017, by respective author (sa).
It is free software; you can redistribute it and/or modify it under
the terms of either:
a) the GNU General Public License as published by the Free Software
Foundation; either version 1, or (at your option) any later version, or
b) the "Artistic License".
*/

#ifndef ALTTAB_H
#define ALTTAB_H

#define MAXNAMESZ   256
#define MAXPATHSZ   8200
#define MAXRPOUT    8200

// as there are no files involved, we can die at any time
// BUT? "it is a good idea to free all the pixmaps that your program created before exiting from the program, pixmaps are stored in the server, if they are not freed they could remain allocated after the program exits"
#define die(e)      do { fprintf(stderr, "%s\n", e); exit(1); } while (0);
#define die2(e,f)   do { fprintf(stderr, "%s%s\n", e, f); exit(1); } while (0);

#define XWINNAME    "alttab"
#define XRMAPPNAME  XWINNAME
#define DEFTILEW    112
#define DEFTILEH    128
#define DEFTILE     "112x128"
#define DEFICONW    16
#define DEFICONH    16
#define DEFICON     "16x16"
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

#define XDEPTH      24  // TODO: get rid of this

#define DEFMODMASK  Mod1Mask
#define DEFBACKMASK ShiftMask
#define DEFMODKS    XK_Alt_L
#define DEFKEYKS    XK_Tab

typedef struct {
    Window id;
    int wm_id;  // wm's internal window id, when WM has it (ratpoison)
    char name[MAXNAMESZ];
    int reclevel;
    Pixmap icon_drawable;  // Window or Pixmap
    unsigned int icon_w, icon_h;
    bool icon_allocated;  // we need to free icon, because we created it (placeholder or depth conversion)
    Pixmap tile;  // ready to display. w/h are all equal and defined in gui.c
} WindowInfo;

typedef struct {
    //char name[MAXNAMESZ];
    char* name;
    XColor xcolor;
    XftColor xftcolor;
    //XRenderColor xrendercolor;
} Color;

typedef struct {
    int debug;
    bool uiShowHasRun; // means: 1. window is ready to Expose, 2. need to call uiHide to free X stuff
    WindowInfo* winlist;
    int maxNdx; // number of items in list above
    int selNdx; // current item
    // option_* are initialized from command line arguments or X resources or defaults
    int option_max_reclevel; // max reclevel. -1 is "everything"
#define WM_MIN          0
#define WM_NO           0
#define WM_EWMH         1
#define WM_RATPOISON    2
#define WM_MAX          2
    int option_wm;
    char* option_font;
    int option_tileW, option_tileH;
    int option_iconW, option_iconH;
    unsigned int option_modMask, option_backMask;
    KeyCode option_modCode, option_keyCode;
    Color color[NCOLORS];
    GC gcDirect, gcReverse, gcFrame;  // used in both gui.c and win.c
    unsigned int ignored_modmask;
} Globals;

// gui
int startupGUItasks (Display* dpy, Window root);
int uiShow (Display* dpy, Window root, bool direction);
void uiExpose (Display* dpy, Window root);
int uiHide (Display* dpy, Window root);
int uiNextWindow (Display* dpy, Window root);
int uiPrevWindow (Display* dpy, Window root);

// windows
int startupWintasks (Display* dpy);
int addWindowInfo (Display* dpy, Window win, int reclevel, int wm_id, char* wm_name);
int initWinlist (Display* dpy, Window root, bool direction);
void freeWinlist (Display* dpy);
int setFocus (Display* dpy, int winNdx);
int rp_startupWintasks();
int x_initWindowsInfoRecursive (Display* dpy,Window win,int reclevel);
int rp_initWinlist (Display* dpy, bool direction);
int x_setFocus (Display* dpy, int wndx);
int rp_setFocus (int winNdx);
int execAndReadStdout(char* exe, char* args[], char* buf, int bufsize);

/* EWHM */
Window *ewmh_get_client_list (Display *disp, unsigned long *size);
int ewmh_client_msg(Display *disp, Window win, char *msg, 
        unsigned long data0, unsigned long data1, 
        unsigned long data2, unsigned long data3,
        unsigned long data4);
int ewmh_activate_window (Display *disp, Window win, 
        bool switch_desktop);
char *ewmh_get_window_title (Display *disp, Window win);
char *ewmh_get_property (Display *disp, Window win, 
        Atom xa_prop_type, char *prop_name, unsigned long *size);
Window ewmh_get_active_window(Display *dpy);
// our
char* ewmh_wmName (Display *disp, Window root);
int ewmh_initWinlist (Display* dpy, bool direction);
int ewmh_setFocus (Display* dpy, int winNdx);

#endif
