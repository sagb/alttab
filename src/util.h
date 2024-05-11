/*
util.c definitions.

Copyright 2017-2024 Alexander Kulak.
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
#include <X11/Xresource.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

#define MAXPROPLEN  4096
#define MAXPROPBIG  128000 * sizeof(long)  // for icon array, for example
#define ERRLEN      2048

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

extern XErrorEvent *ee_ignored;
extern bool ee_complain;

// values for pixel composite transformations
// which are constant per entire image
typedef struct {
    int RShift, GShift, BShift;
    uint32_t RMask, GMask, BMask;
    unsigned long bg;
    uint8_t bg_r, bg_g, bg_b;
} CompositeConst;

#define alpha_composite(composite, fg, alpha, bg) {                            \
    uint16_t shiftarg = ((uint16_t)(fg) * (uint16_t)(alpha) +                  \
        (uint16_t)(bg) * (uint16_t)(255 - (uint16_t)(alpha)) + (uint16_t)128); \
    (composite) = (uint8_t)((shiftarg + (shiftarg >> 8)) >> 8);                \
}


unsigned int getOffendingModifiersMask();
int changeKeygrab(Window win, bool grab, KeyCode keycode,
                  unsigned int modmask, unsigned int ignored_modmask);
int zeroErrorHandler(Display * dpy_our, XErrorEvent * theEvent);
//void setSelectInput(Window win, int reg);

int execAndReadStdout(char *exe, char *args[], char *buf, int bufsize);

int pixmapFitGeneric(Drawable src, Pixmap src_mask, Drawable dst,
                     unsigned int srcW, unsigned int srcH,
                     unsigned int dstWscaled, unsigned int dstHscaled,
                     unsigned int dstWoffset, unsigned int dstHoffset);
int pixmapFitXrender(Drawable src, Pixmap src_mask, Drawable dst,
                     unsigned int srcW, unsigned int srcH,
                     unsigned int dstWscaled, unsigned int dstHscaled,
                     unsigned int dstWoffset, unsigned int dstHoffset);
int pixmapFit(Drawable src, Pixmap src_mask, Drawable dst, unsigned int srcW,
              unsigned int srcH, unsigned int dstW, unsigned int dstH);

size_t utf8len(char *s);
char *utf8index(char *s, size_t pos);

int drawMultiLine(Drawable d, XftFont * font, XftColor * xftcolor, char *str,
                  unsigned int x1, unsigned int y1, unsigned int width,
                  unsigned int height);
int drawMultiLine_test();
int drawSingleLine(Drawable d, XftFont * font, XftColor * xftcolor, char *str,
                  unsigned int x1, unsigned int y1, unsigned int width,
                  unsigned int height);

Bool predproc_true(Display * display, XEvent * event, char *arg);

char *get_x_property(Window win, Atom prop_type, char *prop_name,
                     unsigned long *prop_size);
char *get_x_property_alt(Window win,
			 Atom prop_type1, char *prop_name1,
			 Atom prop_type2, char *prop_name2,
			 unsigned long *prop_size);

bool rectangles_cross(quad a, quad b);
bool get_absolute_coordinates(Window w, quad * q);

void remove_arg(int *argc, char **argv, int argn);
char *xresource_load_string(XrmDatabase * db, const char *appname, char *name);
int xresource_load_int(XrmDatabase * db, const char *appname, char *name,
                       unsigned int *ret);
int ksym_option_to_keycode(XrmDatabase * db, const char *appname,
                           const char *name, char **errmsg);
unsigned int keycode_to_modmask(KeyCode kc);

int convert_msb(uint32_t in);
CompositeConst initCompositeConst(unsigned long bg);
uint32_t pixelComposite(uint32_t fg, uint8_t a, CompositeConst *cc);

#endif
