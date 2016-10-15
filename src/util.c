/*
Helper functions.

This file is part of alttab program.
alttab is Copyright (C) 2016, by respective author (sa).
It is free software; you can redistribute it and/or modify it under the terms of either:
a) the GNU General Public License as published by the Free Software Foundation; either version 1, or (at your option) any later version, or
b) the "Artistic License".
*/

#include "util.h"

// PUBLIC:

//
// get all possible modifiers
// per xbindkeys
//
unsigned int getOffendingModifiersMask (Display * dpy)
{
  unsigned int numlock_mask = 0;
  unsigned int scrolllock_mask = 0;
  unsigned int capslock_mask = 0;

  int i;
  XModifierKeymap *modmap;
  KeyCode nlock, slock;
  static int mask_table[8] = {
    ShiftMask, LockMask, ControlMask, Mod1Mask,
    Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
  };
  nlock = XKeysymToKeycode (dpy, XK_Num_Lock);
  slock = XKeysymToKeycode (dpy, XK_Scroll_Lock);
  /*
   * Find out the masks for the NumLock and ScrollLock modifiers,
   * so that we can bind the grabs for when they are enabled too.
   */
  modmap = XGetModifierMapping (dpy);
  if (modmap != NULL && modmap->max_keypermod > 0)
    {
      for (i = 0; i < 8 * modmap->max_keypermod; i++)
	{
	  if (modmap->modifiermap[i] == nlock && nlock != 0)
	    numlock_mask = mask_table[i / modmap->max_keypermod];
	  else if (modmap->modifiermap[i] == slock && slock != 0)
	    scrolllock_mask = mask_table[i / modmap->max_keypermod];
	}
    }
  capslock_mask = LockMask;
  if (modmap)
    XFreeModifiermap (modmap);
  return (numlock_mask | scrolllock_mask | capslock_mask);
}


//
// for ignoring X errors
// 

// https://tronche.com/gui/x/xlib/event-handling/protocol-errors/default-handlers.html#XErrorEvent

int zeroErrorHandler (Display *display, XErrorEvent *theEvent)
{
ee_ignored = theEvent;
#define EM 512
char etext[EM];
if (ee_complain) {
    memset (etext, '\0', EM);
    XGetErrorText (display, theEvent->error_code, etext, EM);
    fprintf(stderr, "Unexpected X Error: %s\n", etext);
}
return 0;
}


//
// grab/ungrab including all modifier combinations
// per metacity
// returns 1 if sucess, 0 if fail (f.e., BadAccess). Grabs are not restored on failure.
//
int changeKeygrab (
                Display*    dpy,
                Window      xwindow,
                bool        grab,
                KeyCode     keycode,
                unsigned int  modmask,
                unsigned int  ignored_modmask)
{
int debug=1;

// XGrabKey can generate BadAccess  -- ignored globally
//XErrorHandler hnd = (XErrorHandler)0;
//hnd = XSetErrorHandler (zeroErrorHandler);

unsigned int ignored_mask;
/* Grab keycode/modmask, together with
* all combinations of ignored modifiers.
* X provides no better way to do this.
*/
ignored_mask = 0;
while (ignored_mask <= ignored_modmask)
{
    if (ignored_mask & ~(ignored_modmask))
    {
        /* Not a combination of ignored modifiers
         * (it contains some non-ignored modifiers)
         */
        ++ignored_mask;
        continue;
    }
    if (grab) {
        if (debug>1) {fprintf (stderr, "grabbing %d with mask %d\n", keycode, (modmask | ignored_mask));}
        ee_ignored = NULL; // to detect X error event
        ee_complain = false; // our handler will not croak
        XGrabKey (dpy, keycode,
                modmask | ignored_mask,
                xwindow,
                True, // ?
                GrabModeAsync, GrabModeAsync);
        XSync (dpy,False); // for error to appear
        if (ee_ignored) {
            //XSetErrorHandler(hnd);
            ee_complain = true;
            return 0; // signal caller that XGrabKey failed
        }
    }
    else {
        if (debug>1) {fprintf (stderr, "ungrabbing %d with mask %d\n", keycode, (modmask | ignored_mask));}
        XUngrabKey (dpy, keycode,
                modmask | ignored_mask,
                xwindow);
    }
    ++ignored_mask;
}

ee_complain = true;
//XSetErrorHandler(hnd);
return 1;
}


//
// register interest in KeyRelease events for the window
// and its children recursively
// https://stackoverflow.com/questions/39087079/detect-modifier-key-release-in-x11-root-window
//
void setSelectInput (Display* dpy, Window win, int reg)
{                                             
Window root, parent;                        
Window* children;                           
int nchildren, i;                           

//XErrorHandler hnd = (XErrorHandler) 0;   -- ignored globally
//hnd = XSetErrorHandler(zeroErrorHandler);
ee_complain = false;

if (XSelectInput (dpy, win, reg ? KeyReleaseMask|SubstructureNotifyMask : 0) != BadWindow && \
XQueryTree (dpy, win, &root, &parent, &children, &nchildren) != 0) {
for (i = 0; i < nchildren; ++i) {
  setSelectInput (dpy, children[i], reg);
}
if (nchildren>0 && children) {XFree(children);}
}

ee_complain = true;
//XSetErrorHandler(hnd);
}                 


//
// execv program and read its stdout
// http://stackoverflow.com/questions/7292642/grabbing-output-from-exec
//
int execAndReadStdout(char* exe, char* args[], char* buf, int bufsize)
{
int link[2];
pid_t pid;

if (pipe(link)==-1) {
    fprintf (stderr, "pipe\n");
    return 0;
}

if ((pid = fork()) == -1) {
    fprintf (stderr, "fork\n");
    return 0;
}

if(pid == 0) {

dup2 (link[1], STDOUT_FILENO);
close (link[0]);
close (link[1]);
execv (exe, args);
fprintf (stderr, "execv\n");
return 0;

} else {

close(link[1]);
int nbytes = read(link[0], buf, bufsize);
//printf("Output: (%.*s)\n", nbytes, buf);
wait(NULL);

}
return 1;
}


//
// Scale drawable from 0,0
// Return 1=success 0=fail
// TODO: for speed, scale image->image without X11 calls,
//   then transform to Pixmap
//
int pixmapScale (Display* dpy, int scrNum, Window win, 
        Drawable src, Drawable dst, 
        unsigned int srcW, unsigned int srcH, 
        unsigned int dstW, unsigned int dstH)
{
//if (srcW==dstW && srcH==dstH)  TODO: xcopyarea, though redundant
unsigned long valuemask = 0;
XGCValues values;
GC gc = XCreateGC (dpy, win, valuemask, &values);
if (!gc) {
    fprintf (stderr, "pixmapScale: can't create GC\n");
    return 0;
}
XImage* srci = XGetImage (dpy, src, 0, 0, srcW, srcH,
        0xffffffff, XYPixmap);
if (!srci) {
    fprintf (stderr, "XGetImage failed\n");
    XFreeGC (dpy, gc);
    return 0;
}
unsigned long pixel;
int x,y; int re=0;
for (x=0; x<dstW; x++) {
    for (y=0; y<dstH; y++) {
        pixel = XGetPixel (srci, x*srcW/dstW, y*srcH/dstH);
        if (! XSetForeground (dpy, gc, pixel)) re++;
        if (! XDrawPoint (dpy, dst, gc, x, y)) re++;
    }
}
if (re>0) {
    fprintf (stderr, "something failed during scaling %d times\n", re);
    XDestroyImage (srci);
    XFreeGC (dpy, gc);
    return 0;
}
XDestroyImage (srci);
XFreeGC (dpy, gc);
return 1;
}


//
// Return the number of utf8 code points in the buffer at s
// https://stackoverflow.com/questions/7298059/how-to-count-characters-in-a-unicode-string-in-c
//
size_t utf8len(char *s)
{
    size_t len = 0;
    for (; *s; ++s) if ((*s & 0xC0) != 0x80) ++len;
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
        if ((*s & 0xC0) != 0x80) --pos;
        if (pos == 0) return s;
    }
    return NULL;
}

//
// Draw utf-8 string str on window/pixmap d, 
// using *font and *xrcolor,
// splitting and cropping it to fit (x1,y1 - x1+width,y1+height) rectangle.
// Return 1 if ok.
//
int drawMultiLine (Display* dpy, Drawable d, XftFont* font, XftColor* xftcolor, char* str, unsigned int x1, unsigned int y1, unsigned int width, unsigned int height)
{
int debug = 0;
XftDraw* xftdraw;
XGlyphInfo ext;
#define M 2014
char line[M]; size_t line_clen, line_ulen; // current line (substring) to draw
size_t line_from_sym, line_to_sym; char *line_from_char, *line_to_char;  // and its "window" in str
float approx_px_per_sym;
int line_new_ulen;
unsigned int x, y; // current upper left corner at which XftDrawStringUtf8 will draw
float line_interval = 0.3; int line_spacing_px;
bool finish_after_draw = false;

if ((*str)=='\0') return 1;

xftdraw = XftDrawCreate (dpy, d, DefaultVisual(dpy,0), DefaultColormap(dpy,0));
//XftColorAllocValue (dpy,DefaultVisual(dpy,0),DefaultColormap(dpy,0),xrcolor,&xftcolor);

// once: calculate approx_px_per_sym and line_spacing_px
XftTextExtentsUtf8 (dpy, font, str, strlen(str), &ext);
approx_px_per_sym = (float)ext.width / (float)utf8len(str);
line_spacing_px = (float)ext.height * line_interval;

// cycle by lines
line_to_sym = 0;
x = x1; y = y1;
do {
    // have line_to_sym from previous line
    // initialize new line with the rest of str; estimate px_per_sym
    /* ACTIVE CHANGE: */ line_from_sym = line_to_sym;
    line_from_char = utf8index (str,line_from_sym);
    strncpy (line, line_from_char, M); line[M-1]='\0';
    line_clen = strlen (line);
    line_ulen = utf8len (line);
    if (debug>0) {fprintf (stderr, "NEW LINE: \"%s\" ulen=%d clen=%d\n", line, line_ulen, line_clen);}
    // first approximation for line size
    line_new_ulen = (float)width / approx_px_per_sym;
    if (line_new_ulen >= line_ulen) {  // just draw the end of str and finish
        XftTextExtentsUtf8 (dpy, font, line, line_clen, &ext);
        finish_after_draw = true;
        goto Draw;
    }
    /* ACTIVE CHANGE: */ line_ulen = line_new_ulen;
    line_to_sym = line_from_sym + line_ulen; // try to cut here
    line_to_char = utf8index (str,line_to_sym);
    line[line_to_char-line_from_char]='\0';
    line_clen = strlen (line);
    XftTextExtentsUtf8 (dpy, font, line, line_clen, &ext);
    if (debug>0) {fprintf (stderr, "first cut approximation: \"%s\" ulen=%d clen=%d width=%d px, x2-x1=%d px\n", line, line_ulen, line_clen, ext.width, width);}
    while (ext.width > width) {
        // decrease line by 1 utf symbol
        /* ACTIVE CHANGE: */ line_ulen--;
        line_to_sym--;
        line_to_char = utf8index (str,line_to_sym);
        line[line_to_char-line_from_char]='\0';
        line_clen = strlen (line);
        XftTextExtentsUtf8 (dpy, font, line, line_clen, &ext);
        if (debug>0) {fprintf (stderr, "cut correction: \"%s\" ulen=%d clen=%d width=%d px, x2-x1=%d px\n", line, line_ulen, line_clen, ext.width, width);}
    }
    Draw:
    if ((y+ext.height) > (y1+height)) {
        if (debug>0) {fprintf(stderr, "y+ext.height=%d, y2=%d - breaking\n", y+ext.height, (y1+height));}
        break;
    }
    x += (width - ext.width) / 2; // center
    XftDrawStringUtf8 (xftdraw, xftcolor, font, x+ext.x, y+ext.y, line, line_clen);
    if (debug>0) {
        GC gc = DefaultGC (dpy,0);
        XSetForeground (dpy, gc, WhitePixel(dpy,0));
        XDrawRectangle (dpy, d, gc, x, y, ext.width, ext.height);
    }
    x = x1;
    y += ext.height + line_spacing_px;
} while (!finish_after_draw);

// this function doesn't allocate it anymore
//XftColorFree (dpy,DefaultVisual(dpy,0),DefaultColormap(dpy,0),xftcolor);
XftDrawDestroy (xftdraw);
return 1;
}

//
// Test for previous function. Use:
// int main() { return drawMultiLine_test() ? 0 : 1; }
// g++ XftTest.cc -lX11 -lXft `pkg-config --cflags freetype2`
//

int drawMultiLine_test ()
{
Display* dpy = XOpenDisplay(0);;
Window win = XCreateSimpleWindow (dpy,DefaultRootWindow(dpy),0,0,1000,600,0,0,0);
XMapWindow (dpy, win);

/*
XRenderColor xrcolor;
xrcolor.red  =65535;
xrcolor.green=0;
xrcolor.blue =0;
xrcolor.alpha=65535;
*/
XftColor xftcolor;
XftColorAllocName (dpy, DefaultVisual(dpy,0), DefaultColormap(dpy,0), "blue", &xftcolor);

XftFont* font = NULL;
font = XftFontOpenName(dpy,0,"DejaVu Sans Condensed-50");
//font = XftFontOpenName(dpy,0,"Arial-24");
if (!font) return 0;

char* line = "Rdfr Пример drawMultiLine ^&*♘#^% лала дофываор ghj 12345698675 56273 hagsdfgh hgfa shgdffg jhHG g 238476 y jlk3(* &^78  8734 76 83756 87 * hgjGHJHG876 i47^ t76 8768i";

GC gc = DefaultGC (dpy,0);
XSetForeground (dpy, gc, WhitePixel(dpy,0));
XDrawRectangle (dpy, win, gc, 100,100,500,400);
int r = drawMultiLine (dpy, win, font, &xftcolor, line, 100,100,500,400);

XFlush(dpy);
sleep(2);

XftFontClose (dpy, font);
return r;
}


