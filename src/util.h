/*
util.c definitions.

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

#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <X11/extensions/Xrender.h>

#define MAXPROPLEN  4096

XErrorEvent *ee_ignored;
bool ee_complain;

unsigned int getOffendingModifiersMask();
int changeKeygrab(Window win, bool grab, KeyCode keycode,
		  unsigned int modmask, unsigned int ignored_modmask);
int zeroErrorHandler(Display * dpy_our, XErrorEvent * theEvent);
void setSelectInput(Window win, int reg);

int execAndReadStdout(char *exe, char *args[], char *buf, int bufsize);

int pixmapFitGeneric(Drawable src, Pixmap src_mask, Drawable dst, unsigned int srcW,
		     unsigned int srcH, unsigned int dstWscaled,
		     unsigned int dstHscaled, unsigned int dstWoffset,
		     unsigned int dstHoffset);
int pixmapFitXrender(Drawable src, Pixmap src_mask, Drawable dst, unsigned int srcW,
		     unsigned int srcH, unsigned int dstWscaled,
		     unsigned int dstHscaled, unsigned int dstWoffset,
		     unsigned int dstHoffset);
int pixmapFit(Drawable src, Pixmap src_mask, Drawable dst, unsigned int srcW,
	      unsigned int srcH, unsigned int dstW, unsigned int dstH);

size_t utf8len(char *s);
char *utf8index(char *s, size_t pos);

int drawMultiLine(Drawable d, XftFont * font, XftColor * xftcolor, char *str, unsigned int x1,
		  unsigned int y1, unsigned int width, unsigned int height);
int drawMultiLine_test();

Bool predproc_true(Display * display, XEvent * event, char *arg);

char *get_x_property(Window win, Atom prop_type, char *prop_name,
		     unsigned long *prop_size);

#endif
