/*
util.c definitions.

This file is part of alttab program.
alttab is Copyright (C) 2016, by respective author (sa).
It is free software; you can redistribute it and/or modify it under the terms of either:
a) the GNU General Public License as published by the Free Software Foundation; either version 1, or (at your option) any later version, or
b) the "Artistic License".
*/

#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

// for ignoring X errors (see handler code and notes/xlib)
XErrorEvent* ee_ignored;
bool ee_complain;

unsigned int getOffendingModifiersMask (Display * dpy);
int changeKeygrab ( Display* dpy, Window xwindow, bool grab, KeyCode keycode, unsigned int modmask, unsigned int ignored_modmask);
int zeroErrorHandler (Display *display, XErrorEvent *theEvent);
void setSelectInput (Display* dpy, Window win, int reg);

int execAndReadStdout (char* exe, char* args[], char* buf, int bufsize);

int pixmapScale (Display* dpy, int scrNum, Window win, Drawable src, Drawable dst, unsigned int srcW, unsigned int srcH, unsigned int dstW, unsigned int dstH); 

size_t utf8len (char* s);
char* utf8index (char* s, size_t pos);

int drawMultiLine (Display* dpy, Drawable d, XftFont* font, XftColor* xftcolor, char* str, unsigned int x1, unsigned int y1, unsigned int width, unsigned int height);
int drawMultiLine_test ();

Bool predproc_true (Display* display, XEvent* event, char* arg);

#endif
