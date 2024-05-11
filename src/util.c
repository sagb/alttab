/*
Helper functions.

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

#include "util.h"
#include <sys/types.h>
#include <sys/wait.h>
extern Display *dpy;
extern int scr;
extern Window root;

XErrorEvent *ee_ignored;
bool ee_complain;

// PUBLIC:

//
// get all possible modifiers
//
unsigned int getOffendingModifiersMask()
{
    unsigned int lockmask[3];   // num=0 scroll=1 caps=2
    int i;
    XModifierKeymap *map;
    KeyCode numlock, scrolllock;
    static int masks[8] = {
        ShiftMask, LockMask, ControlMask,
        Mod1Mask, Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
    };

    memset(lockmask, 0, sizeof(unsigned int) * 3);
    numlock = XKeysymToKeycode(dpy, XK_Num_Lock);
    scrolllock = XKeysymToKeycode(dpy, XK_Scroll_Lock);
    map = XGetModifierMapping(dpy);
    if (map != NULL && map->max_keypermod > 0) {
        for (i = 0; i < 8 * map->max_keypermod; i++) {
            if (numlock != 0 && map->modifiermap[i] == numlock)
                lockmask[0] = masks[i / map->max_keypermod];
            else if (scrolllock != 0 && map->modifiermap[i] == scrolllock)
                lockmask[1] = masks[i / map->max_keypermod];
        }
    }
    lockmask[2] = LockMask;
    if (map)
        XFreeModifiermap(map);
    return (lockmask[0] | lockmask[1] | lockmask[2]);
}

//
// for ignoring X errors
// https://tronche.com/gui/x/xlib/event-handling/protocol-errors/default-handlers.html#XErrorEvent
//
int zeroErrorHandler(Display * dpy_our, XErrorEvent * theEvent)
{
    ee_ignored = theEvent;
#define EM 512
    char etext[EM];
    if (ee_complain) {
        memset(etext, '\0', EM);
        XGetErrorText(dpy_our, theEvent->error_code, etext, EM);
        fprintf(stderr, "Unexpected X Error: %s\n", etext);
        fprintf(stderr,
                "Error details: request_code=%hhu minor_code=%hhu resourceid=%lu\n",
                theEvent->request_code, theEvent->minor_code,
                theEvent->resourceid);
    }
    return 0;
}

//
// grab/ungrab including all modifier combinations
// returns 1 if success, 0 if fail (f.e., BadAccess).
// Grabs are not restored on failure.
//
int changeKeygrab(Window win, bool grab, KeyCode keycode,
                  unsigned int modmask, unsigned int ignored_modmask)
{
    int debug = 1;
    unsigned int mask = 0;

    while (mask <= ignored_modmask) {
        if (mask & ~(ignored_modmask)) {
            ++mask;
            continue;
        }
        if (grab) {
            if (debug > 1) {
                fprintf(stderr, "grabbing %d with mask %d\n",
                        keycode, (modmask | mask));
            }
            ee_ignored = NULL;  // to detect X error event
            ee_complain = false;    // our handler will not croak
            XGrabKey(dpy, keycode, modmask | mask, win, True,
                     GrabModeAsync, GrabModeAsync);
            XSync(dpy, False);  // for error to appear
            if (ee_ignored) {
                ee_complain = true;
                return 0;       // signal caller that XGrabKey failed
            }
        } else {
            if (debug > 1) {
                fprintf(stderr, "ungrabbing %d with mask %d\n",
                        keycode, (modmask | mask));
            }
            XUngrabKey(dpy, keycode, modmask | mask, win);
        }
        ++mask;
    }

    ee_complain = true;
    return 1;
}

/*
//
// register interest in KeyRelease events for the window
// and its children recursively
//
void setSelectInput(Window win, int reg)
{
	Window root, parent;
	Window *children;
	unsigned int nchildren, i;

	ee_complain = false;

    // warning: this overwrites any previous mask,
    // find other calls to XSelectInput
	if (XSelectInput
	    (dpy, win,
	     reg ? KeyReleaseMask | SubstructureNotifyMask : 0) != BadWindow
	    && XQueryTree(dpy, win, &root, &parent, &children,
			  &nchildren) != 0) {
		for (i = 0; i < nchildren; ++i)
			setSelectInput(children[i], reg);
		if (nchildren > 0 && children)
			XFree(children);
	}

	ee_complain = true;
}
*/

//
// execv program and read its stdout
//
int execAndReadStdout(char *exe, char *args[], char *buf, int bufsize)
{
    int link[2];
    pid_t pid;
    ssize_t rb;

    if (pipe(link) == -1) {
        perror("pipe");
        return 0;
    }
    if ((pid = fork()) == -1) {
        perror("fork");
        return 0;
    }
    if (pid == 0) {
        dup2(link[1], STDOUT_FILENO);
        close(link[0]);
        close(link[1]);
        execv(exe, args);
        perror("execv");
        return 0;
    } else {
        close(link[1]);
        if (buf != NULL) {
            rb = read(link[0], buf, bufsize);
            if (rb == -1)
                *buf = '\0';
        }
        close(link[0]);
        wait(NULL);
    }
    return 1;
}

//
// Fit the src/src_mask drawable into dst,
// centering and preserving aspect ratio
// Slow but extension-independent version
// 1=success 0=fail
//
int pixmapFitGeneric(Drawable src, Pixmap src_mask, Drawable dst,
                     unsigned int srcW, unsigned int srcH,
                     unsigned int dstWscal, unsigned int dstHscal,
                     unsigned int dstWoff, unsigned int dstHoff)
{
    unsigned long valuemask = 0;
    XGCValues values;
    GC gc = XCreateGC(dpy, root, valuemask, &values);
    if (!gc) {
        fprintf(stderr, "pixmapScale: can't create GC\n");
        return 0;
    }
    XImage *srci = XGetImage(dpy, src, 0, 0, srcW, srcH,
                             0xffffffff, XYPixmap);
    if (!srci) {
        fprintf(stderr, "XGetImage failed\n");
        XFreeGC(dpy, gc);
        return 0;
    }
    XImage *maski;
    if (src_mask != 0) {
        maski = XGetImage(dpy, src_mask, 0, 0, srcW, srcH,
                          0xffffffff, XYPixmap);
        if (!maski) {
            fprintf(stderr, "XGetImage failed for the mask\n");
            XFreeGC(dpy, gc);
            return 0;
        }
    }
    unsigned long pixel;
    int x, y;
    int re = 0;
    for (x = 0; x < dstWscal; x++) {
        for (y = 0; y < dstHscal; y++) {
            if (src_mask != 0
                && XGetPixel(maski, x * srcW / dstWscal,
                             y * srcH / dstHscal) == 0)
                continue;
            pixel = XGetPixel(srci, x * srcW / dstWscal, y * srcH / dstHscal);
            if (!XSetForeground(dpy, gc, pixel))
                re++;
            if (!XDrawPoint(dpy, dst, gc, x + dstWoff, y + dstHoff))
                re++;
        }
    }
    if (re > 0) {
        fprintf(stderr, "something failed during scaling %d times\n", re);
        XDestroyImage(srci);
        XFreeGC(dpy, gc);
        return 0;
    }
    XDestroyImage(srci);
    XFreeGC(dpy, gc);
    return 1;
}

//
// Fit the src/src_mask drawable into dst,
// centering and preserving aspect ratio
// XRender version
// 1=success 0=fail
// /usr/share/doc/libxrender-dev/libXrender.txt.gz
// https://cgit.freedesktop.org/xorg/proto/renderproto/plain/renderproto.txt
//
int pixmapFitXrender(Drawable src, Pixmap src_mask, Drawable dst,
                     unsigned int srcW, unsigned int srcH,
                     unsigned int dstWscal, unsigned int dstHscal,
                     unsigned int dstWoff, unsigned int dstHoff)
{
    Picture Psrc, Pmask, Pdst;
    XRenderPictFormat *format, *format_of_mask;
    XTransform transform;

    format = XRenderFindVisualFormat(dpy, DefaultVisual(dpy, scr));
    if (!format) {
        fprintf(stderr, "error, couldn't find valid Xrender format\n");
        return 0;
    }
    format_of_mask = XRenderFindStandardFormat(dpy, PictStandardA1);
    if (!format_of_mask) {
        fprintf(stderr, "error, couldn't find valid Xrender format for mask\n");
        return 0;
    }
    Psrc = XRenderCreatePicture(dpy, src, format, 0, NULL);
    Pmask =
        (src_mask != 0) ? XRenderCreatePicture(dpy, src_mask,
                                               format_of_mask, 0, NULL) : 0;
    Pdst = XRenderCreatePicture(dpy, dst, format, 0, NULL);
    XRenderSetPictureFilter(dpy, Psrc, FilterBilinear, 0, 0);
    if (Pmask != 0)
        XRenderSetPictureFilter(dpy, Pmask, FilterBilinear, 0, 0);
    transform.matrix[0][0] = (srcW << 16) / dstWscal;
    transform.matrix[0][1] = 0;
    transform.matrix[0][2] = 0;
    transform.matrix[1][0] = 0;
    transform.matrix[1][1] = (srcH << 16) / dstHscal;
    transform.matrix[1][2] = 0;
    transform.matrix[2][0] = 0;
    transform.matrix[2][1] = 0;
    transform.matrix[2][2] = XDoubleToFixed(1.0);
    XRenderSetPictureTransform(dpy, Psrc, &transform);
    if (Pmask != 0)
        XRenderSetPictureTransform(dpy, Pmask, &transform);
    XRenderComposite(dpy, PictOpOver, Psrc, Pmask, Pdst, 0, 0, 0, 0,
                     dstWoff, dstHoff, dstWscal, dstHscal);
    XRenderFreePicture(dpy, Psrc);
    XRenderFreePicture(dpy, Pdst);
    if (Pmask != 0)
        XRenderFreePicture(dpy, Pmask);

    return 1;
}

//
// Fit the src/src_mask drawable into dst,
// centering and preserving aspect ratio
// 1=success 0=fail
//
int pixmapFit(Drawable src, Pixmap src_mask, Drawable dst,
              unsigned int srcW, unsigned int srcH,
              unsigned int dstW, unsigned int dstH)
{
    int event_basep, error_basep;
    int32_t fWrat, fHrat;       // ratio * 65536
    int dstWscal, dstWoff, dstHscal, dstHoff;

    fWrat = (dstW << 16) / srcW;
    fHrat = (dstH << 16) / srcH;
    if (fWrat > fHrat) {
        dstWscal = ((srcW * fHrat) >> 16);
        if (abs(dstWscal - dstW) <= 1)  // suppress rounding errors
            dstWscal = dstW;
        dstWoff = (dstW - dstWscal) / 2;
        dstHscal = dstH;
        dstHoff = 0;
    } else {
        dstWscal = dstW;
        dstWoff = 0;
        dstHscal = ((srcH * fWrat) >> 16);
        if (abs(dstHscal - dstH) <= 1)
            dstHscal = dstH;
        dstHoff = (dstH - dstHscal) / 2;
    }

    return XRenderQueryExtension(dpy, &event_basep, &error_basep) == True ?
        pixmapFitXrender(src, src_mask, dst, srcW, srcH,
                         dstWscal, dstHscal, dstWoff,
                         dstHoff) : pixmapFitGeneric(src, src_mask, dst, srcW,
                                                     srcH, dstWscal,
                                                     dstHscal, dstWoff,
                                                     dstHoff);
}

//
// Return the number of utf8 code points in the buffer at s
//
size_t utf8len(char *s)
{
    size_t len = 0;
    for (; *s; ++s)
        if ((*s & 0xC0) != 0x80)
            ++len;
    return len;
}

//
// Return a pointer to the beginning of the pos'th utf8 codepoint
// in the buffer at s
//
char *utf8index(char *s, size_t pos)
{
    ++pos;
    for (; *s; ++s) {
        if ((*s & 0xC0) != 0x80)
            --pos;
        if (pos == 0)
            return s;
    }
    return NULL;
}

//
// Draw utf-8 string str on window/pixmap d,
// using *font and *xrcolor,
// splitting and cropping it to fit (x1,y1 - x1+width,y1+height) rectangle.
// Return 1 if ok.
//
int drawMultiLine(Drawable d, XftFont * font,
                  XftColor * xftcolor, char *str, unsigned int x1,
                  unsigned int y1, unsigned int width, unsigned int height)
{
    int debug = 0;
    XftDraw *xftdraw;
    XGlyphInfo ext;
#define M 2014
    char line[M];
    size_t line_clen, line_ulen;    // current line (substring) to draw
    size_t line_from_sym, line_to_sym;
    char *line_from_char, *line_to_char;    // and its "window" in str
    float approx_px_per_sym;
    int line_new_ulen;
    unsigned int x, y;          // current upper left corner at which XftDrawStringUtf8 will draw
    float line_interval = 0.3;
    int line_spacing_px;
    bool finish_after_draw = false;

    if ((*str) == '\0')
        return 1;

    xftdraw =
        XftDrawCreate(dpy, d, DefaultVisual(dpy, 0), DefaultColormap(dpy, scr));
//XftColorAllocValue (dpy,DefaultVisual(dpy,scr),DefaultColormap(dpy,scr),xrcolor,&xftcolor);

// once: calculate approx_px_per_sym and line_spacing_px
    XftTextExtentsUtf8(dpy, font, (unsigned char *)str, strlen(str), &ext);
    approx_px_per_sym = (float)ext.width / (float)utf8len(str);
    line_spacing_px = (float)ext.height * line_interval;

// cycle by lines
    line_to_sym = 0;
    x = x1;
    y = y1;
    do {
        // have line_to_sym from previous line
        // initialize new line with the rest of str; estimate px_per_sym
        /* ACTIVE CHANGE: */ line_from_sym = line_to_sym;
        line_from_char = utf8index(str, line_from_sym);
        strncpy(line, line_from_char, M);
        line[M - 1] = '\0';
        line_clen = strlen(line);
        line_ulen = utf8len(line);
        if (debug > 0) {
            fprintf(stderr, "NEW LINE: \"%s\" ulen=%zu clen=%zu\n",
                    line, line_ulen, line_clen);
        }
        // first approximation for line size
        line_new_ulen = (float)width / approx_px_per_sym;
        if (line_new_ulen >= line_ulen) {   // just draw the end of str and finish
            XftTextExtentsUtf8(dpy, font, (unsigned char *)line,
                               line_clen, &ext);
            finish_after_draw = true;
            goto Draw;
        }
        /* ACTIVE CHANGE: */
        line_ulen = line_new_ulen;
        line_to_sym = line_from_sym + line_ulen;    // try to cut here
        line_to_char = utf8index(str, line_to_sym);
        line[line_to_char - line_from_char] = '\0';
        line_clen = strlen(line);
        XftTextExtentsUtf8(dpy, font, (unsigned char *)line, line_clen, &ext);
        if (debug > 0) {
            fprintf(stderr,
                    "first cut approximation: \"%s\" ulen=%zu clen=%zu width=%d px, x2-x1=%d px\n",
                    line, line_ulen, line_clen, ext.width, width);
        }
        while (ext.width > width) {
            // decrease line by 1 utf symbol
            /* ACTIVE CHANGE: */ line_ulen--;
            line_to_sym--;
            line_to_char = utf8index(str, line_to_sym);
            line[line_to_char - line_from_char] = '\0';
            line_clen = strlen(line);
            XftTextExtentsUtf8(dpy, font, (unsigned char *)line,
                               line_clen, &ext);
            if (debug > 0) {
                fprintf(stderr,
                        "cut correction: \"%s\" ulen=%zu clen=%zu width=%d px, x2-x1=%d px\n",
                        line, line_ulen, line_clen, ext.width, width);
            }
        }
 Draw:
        if ((y + ext.height) > (y1 + height)) {
            if (debug > 0) {
                fprintf(stderr,
                        "y+ext.height=%d, y2=%d - breaking\n",
                        y + ext.height, (y1 + height));
            }
            break;
        }
        x += (width - ext.width) / 2;   // center
        XftDrawStringUtf8(xftdraw, xftcolor, font, x + ext.x, y + ext.y,
                          (unsigned char *)line, line_clen);
        if (debug > 0) {
            GC gc = DefaultGC(dpy, scr);
            XSetForeground(dpy, gc, WhitePixel(dpy, scr));
            XDrawRectangle(dpy, d, gc, x, y, ext.width, ext.height);
        }
        x = x1;
        y += ext.height + line_spacing_px;
    } while (!finish_after_draw);

    XftDrawDestroy(xftdraw);
    return 1;
}

//
// Test for previous function. Use:
// int main() { return drawMultiLine_test() ? 0 : 1; }
// g++ XftTest.cc -lX11 -lXft `pkg-config --cflags freetype2`
// not used in alttab
//
int drawMultiLine_test()
{
    // define these as globals in caller
    dpy = XOpenDisplay(NULL);
    scr = DefaultScreen(dpy);
    root = RootWindow(dpy, scr);

    Window win = XCreateSimpleWindow(dpy, root, 0, 0, 1000, 600, 0,
                                     0, 0);
    XMapWindow(dpy, win);

    XftColor xftcolor;
    XftColorAllocName(dpy, DefaultVisual(dpy, 0), DefaultColormap(dpy, 0),
                      "blue", &xftcolor);

    XftFont *font = NULL;
    font = XftFontOpenName(dpy, 0, "DejaVu Sans Condensed-50");
//font = XftFontOpenName(dpy,0,"Arial-24");
    if (!font)
        return 0;

    char line[104];
    line[0] = '\0';
    char *add = "example of drawMultiLine ";
    int sc;
    for (sc = 0; sc < 4; sc++)
        strncat(line, add, 26);

    GC gc = DefaultGC(dpy, 0);
    XSetForeground(dpy, gc, WhitePixel(dpy, 0));
    XDrawRectangle(dpy, win, gc, 100, 100, 500, 400);
    int r = drawMultiLine(win, font, &xftcolor, line, 100, 100, 500, 400);

    XFlush(dpy);
    sleep(2);

    XftFontClose(dpy, font);
    return r;
}

//
// Draw single utf-8 string str on window/pixmap d,
// using *font and *xftcolor, at (x1,y1,width*height).
// Return 1 if ok.
//
int drawSingleLine(Drawable d, XftFont * font,
            XftColor * xftcolor, char *str, unsigned int x1,
            unsigned int y1, unsigned int width, unsigned int height)
{
    int debug = 0;
    XftDraw *xftdraw;
    int line_clen;

    if ((*str) == '\0')
        return 1;
    xftdraw = XftDrawCreate(dpy, d, DefaultVisual(dpy, 0), 
            DefaultColormap(dpy, scr));
    line_clen = strlen(str);
    XftDrawStringUtf8(xftdraw, xftcolor, font, x1, y1+height,
            (unsigned char *)str, line_clen);
    if (debug > 0) {
        GC gc = DefaultGC(dpy, scr);
        XSetForeground(dpy, gc, WhitePixel(dpy, scr));
        XDrawRectangle(dpy, d, gc, x1, y1, x1 + width, y1 + height);
    }
    return 1;
}

//
// check procedure for XCheckIfEvent which always returns true
//
Bool predproc_true(Display * display, XEvent * event, char *arg)
{
    return (True);
}

//
// obtain X Window property
// prop_size is returned in bytes
//
char *get_x_property(Window win, Atom prop_type, char *prop_name,
                     unsigned long *prop_size)
{

    int prop_ret_fmt;
    unsigned long n_prop_ret_items, ret_bytes_after, size;
    unsigned char *ret_prop;
    char *r;
    Atom prop_name_x, prop_ret_type_x;
    int max_prop_len;

    int debug = 0;

    prop_name_x = XInternAtom(dpy, prop_name, False);
    max_prop_len = strstr(prop_name, "ICON") == NULL ? MAXPROPLEN : MAXPROPBIG;

    ee_complain = false;
    Status propstatus =
        XGetWindowProperty(dpy, win, prop_name_x, 0, max_prop_len / 4, False,
                           prop_type, &prop_ret_type_x, &prop_ret_fmt,
                           &n_prop_ret_items, &ret_bytes_after, &ret_prop);
    XSync(dpy, False);          // for error to "appear"
    ee_complain = true;

    if (propstatus != Success && debug > 0) {
        fprintf(stderr,
                "get_x_property: XGetWindowProperty failed (win %ld, prop %s)\n",
                win, prop_name);
        return (char *)NULL;
    }

    if (prop_type != prop_ret_type_x) {
        // this diagnostic may cause BadAtom
        if (debug > 1)
            fprintf(stderr,
                    "get_x_property: prop type doesn't match (win %ld, prop %s, requested: %s, obtained: %s)\n",
                    win, prop_name,
                    (prop_type == 0) ? "0" : XGetAtomName(dpy,
                                                          prop_type),
                    (prop_ret_type_x == 0) ? "0" : XGetAtomName(dpy,
                                                                prop_ret_type_x));
        XFree(ret_prop);
        return (char *)NULL;
    }

    size = (prop_ret_fmt / 8) * n_prop_ret_items;
    if (prop_ret_fmt == 32) {
        size *= sizeof(long) / 4;
    }
    r = malloc(size + 1);
    memcpy(r, ret_prop, size);
    r[size] = '\0';
    if (prop_size) {
        *prop_size = size;
    }

    XFree(ret_prop);
    return r;
}

char *get_x_property_alt(Window win,
			 Atom prop_type1, char *prop_name1,
			 Atom prop_type2, char *prop_name2,
			 unsigned long *prop_size)
{
    char *p;

    p = get_x_property(win, prop_type1, prop_name1, prop_size);
    if (p != NULL)
        return p;

    p = get_x_property(win, prop_type2, prop_name2, prop_size);
    return p;
}

//
// do rectangles cross?
//
bool rectangles_cross(quad a, quad b)
{
    return !(a.x >= (b.x + b.w) ||
             (a.x + a.w) <= b.x || a.y >= (b.y + b.h) || (a.y + a.h) <= b.y);
}

//
// coordinates relative to root
//
bool get_absolute_coordinates(Window w, quad * q)
{
    Window child;
    XWindowAttributes wa;
    int x, y;
    if (XTranslateCoordinates(dpy, w, root, 0, 0, &x, &y, &child) == False)
        return false;
    if (XGetWindowAttributes(dpy, w, &wa) == 0)
        return false;
    q->x = x;
    q->y = y;
    q->w = wa.width;
    q->h = wa.height;
    return true;
}

//
// removes argument from argv
//
void remove_arg(int *argc, char **argv, int argn)
{
    int i;
    for (i = argn; i < (*argc); ++i)
        argv[i] = argv[i + 1];
    --(*argc);
}

//
// return resource/option `appname.name'
// or NULL if not specified
//
char *xresource_load_string(XrmDatabase * db, const char *appname, char *name)
{
    char xappname[MAXNAMESZ];
    XrmValue v;
    char *type;

    snprintf(xappname, MAXNAMESZ, "%s.%s", appname, name);
    XrmGetResource(*db, xappname, xappname, &type, &v);
    return (v.addr != NULL && !strncmp("String", type, 6)) ? v.addr : NULL;
}

//
// search for resource/option `appname.name'
// return:
//  1: ok, result in *ret
//  0: not specified
// -1: conversion error
//
int xresource_load_int(XrmDatabase * db, const char *appname, char *name,
                       unsigned int *ret)
{
    unsigned int r;
    char *endptr;
    char *s = xresource_load_string(db, appname, name);

    if (s != NULL) {
        r = strtol(s, &endptr, 0);
        if (*s != '\0' && *endptr == '\0') {
            *ret = r;
            return 1;
        } else {
            return -1;
        }
    } else {
        return 0;
    }
    return 0;
}

//
// return keycode corresponding to resource/option `appname.name.keysym'
//  0: not specified
// -1: bad value, errmsg is set (caller must free)
//
int ksym_option_to_keycode(XrmDatabase * db, const char *appname,
                           const char *name, char **errmsg)
{
    char *endptr, *opt;
    KeySym ksym;
    char xresr[MAXNAMESZ];
    KeyCode retcode = 0;

    endptr = opt = NULL;
    snprintf(xresr, MAXNAMESZ, "%s.keysym", name);
    xresr[MAXNAMESZ - 1] = '\0';
    opt = xresource_load_string(db, appname, xresr);
    if (opt) {
        ksym = XStringToKeysym(opt);
        if (ksym == NoSymbol) {
            ksym = strtol(opt, &endptr, 0);
            if (!(*opt != '\0' && *endptr == '\0'))
                ksym = NoSymbol;
        }
        if (ksym != NoSymbol) {
            retcode = XKeysymToKeycode(dpy, ksym);
            if (retcode == 0) {
                *errmsg = malloc(ERRLEN);
                snprintf(*errmsg, ERRLEN,
                         "the specified %s keysym is not defined for any keycode",
                         name);
                return -1;
            }
            return retcode;
        } else {
            *errmsg = malloc(ERRLEN);
            snprintf(*errmsg, ERRLEN, "invalid %s keysym", name);
            return -1;
        }
    }
    return 0;
}

//
// scan modifier table,
// return first modifier corresponding to given keycode,
// in the form of modmask,
// or zero if not found
//
unsigned int keycode_to_modmask(KeyCode kc)
{
    int mi, ksi;
    KeyCode tkc;
    XModifierKeymap *xmk = XGetModifierMapping(dpy);
    unsigned int ret = 0;

    for (mi = 0; mi < 8; mi++) {
        for (ksi = 0; ksi < xmk->max_keypermod; ksi++) {
            tkc = (xmk->modifiermap)[xmk->max_keypermod * mi + ksi];
            if (tkc == kc) {
                ret = (1 << mi);
                goto out;
            }
        }
    }

out:
    XFreeModifiermap(xmk);
    return ret;
}

//
// initCompositeConst helper
//
int convert_msb(uint32_t in)
{
    int out;
    for (out = 31; out >= 0; --out) {
        if (in & 0x80000000L)
            break;
        in <<= 1;
    }
    return out;
}

//
// prepare composition transformations
//
CompositeConst initCompositeConst(unsigned long bg)
{
    CompositeConst cc;
    Visual *visual = DefaultVisual(dpy, scr);
    cc.RMask = visual->red_mask;
    cc.GMask = visual->green_mask;
    cc.BMask = visual->blue_mask;
    cc.RShift = convert_msb(cc.RMask) - 7;
    cc.GShift = convert_msb(cc.GMask) - 7;
    cc.BShift = convert_msb(cc.BMask) - 7;
    cc.bg = bg;
    cc.bg_r = (bg >> cc.RShift) & 0xff;
    cc.bg_g = (bg >> cc.GShift) & 0xff;
    cc.bg_b = (bg >> cc.BShift) & 0xff;
    return cc;
}

//
// compose "fg" pixel with background (from "cc") using alpha "a"
//
uint32_t pixelComposite(uint32_t fg, uint8_t a, CompositeConst *cc)
{
    uint32_t ret;
    uint8_t r, g, b;
    if (a == 0) {
        ret = cc->bg;
    } else if (a == 255) {
        ret = fg;
    } else {
        alpha_composite(r, (fg >> 16) & 0xff, a, cc->bg_r);
        alpha_composite(g, (fg >> 8) & 0xff, a, cc->bg_g);
        alpha_composite(b, fg & 0xff, a, cc->bg_b);
        ret = (r << cc->RShift) | (g << cc->GShift) | (b << cc->BShift);
    }
    return ret;
}

