/*
Helper functions.

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

#include "util.h"
#include <sys/types.h>
#include <sys/wait.h>

// PUBLIC:

//
// get all possible modifiers
//
unsigned int getOffendingModifiersMask(Display * dpy)
{
	unsigned int lockmask[3];	// num=0 scroll=1 caps=2
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
			else if (scrolllock != 0
				 && map->modifiermap[i] == scrolllock)
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
int zeroErrorHandler(Display * display, XErrorEvent * theEvent)
{
	ee_ignored = theEvent;
#define EM 512
	char etext[EM];
	if (ee_complain) {
		memset(etext, '\0', EM);
		XGetErrorText(display, theEvent->error_code, etext, EM);
		fprintf(stderr, "Unexpected X Error: %s\n", etext);
	}
	return 0;
}

//
// grab/ungrab including all modifier combinations
// returns 1 if sucess, 0 if fail (f.e., BadAccess).
// Grabs are not restored on failure.
//
int changeKeygrab(Display * dpy, Window win, bool grab, KeyCode keycode,
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
			ee_ignored = NULL;	// to detect X error event
			ee_complain = false;	// our handler will not croak
			XGrabKey(dpy, keycode, modmask | mask, win, True,
				 GrabModeAsync, GrabModeAsync);
			XSync(dpy, False);	// for error to appear
			if (ee_ignored) {
				ee_complain = true;
				return 0;	// signal caller that XGrabKey failed
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

//
// register interest in KeyRelease events for the window
// and its children recursively
//
void setSelectInput(Display * dpy, Window win, int reg)
{
	Window root, parent;
	Window *children;
	unsigned int nchildren, i;

	ee_complain = false;

	if (XSelectInput
	    (dpy, win,
	     reg ? KeyReleaseMask | SubstructureNotifyMask : 0) != BadWindow
	    && XQueryTree(dpy, win, &root, &parent, &children,
			  &nchildren) != 0) {
		for (i = 0; i < nchildren; ++i)
			setSelectInput(dpy, children[i], reg);
		if (nchildren > 0 && children)
			XFree(children);
	}

	ee_complain = true;
}

//
// execv program and read its stdout
//
int execAndReadStdout(char *exe, char *args[], char *buf, int bufsize)
{
	int link[2];
	pid_t pid;

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
		(void)read(link[0], buf, bufsize);
//printf("Output: (%.*s)\n", nbytes, buf);
		close(link[0]);
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
int pixmapScale(Display * dpy, int scrNum, Window win,
		Drawable src, Drawable dst,
		unsigned int srcW, unsigned int srcH,
		unsigned int dstW, unsigned int dstH)
{
//if (srcW==dstW && srcH==dstH)  TODO: xcopyarea, though redundant
	unsigned long valuemask = 0;
	XGCValues values;
	GC gc = XCreateGC(dpy, win, valuemask, &values);
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
	unsigned long pixel;
	int x, y;
	int re = 0;
	for (x = 0; x < dstW; x++) {
		for (y = 0; y < dstH; y++) {
			pixel =
			    XGetPixel(srci, x * srcW / dstW, y * srcH / dstH);
			if (!XSetForeground(dpy, gc, pixel))
				re++;
			if (!XDrawPoint(dpy, dst, gc, x, y))
				re++;
		}
	}
	if (re > 0) {
		fprintf(stderr, "something failed during scaling %d times\n",
			re);
		XDestroyImage(srci);
		XFreeGC(dpy, gc);
		return 0;
	}
	XDestroyImage(srci);
	XFreeGC(dpy, gc);
	return 1;
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
int drawMultiLine(Display * dpy, Drawable d, XftFont * font,
		  XftColor * xftcolor, char *str, unsigned int x1,
		  unsigned int y1, unsigned int width, unsigned int height)
{
	int debug = 0;
	XftDraw *xftdraw;
	XGlyphInfo ext;
#define M 2014
	char line[M];
	size_t line_clen, line_ulen;	// current line (substring) to draw
	size_t line_from_sym, line_to_sym;
	char *line_from_char, *line_to_char;	// and its "window" in str
	float approx_px_per_sym;
	int line_new_ulen;
	unsigned int x, y;	// current upper left corner at which XftDrawStringUtf8 will draw
	float line_interval = 0.3;
	int line_spacing_px;
	bool finish_after_draw = false;

	if ((*str) == '\0')
		return 1;

	xftdraw =
	    XftDrawCreate(dpy, d, DefaultVisual(dpy, 0),
			  DefaultColormap(dpy, 0));
//XftColorAllocValue (dpy,DefaultVisual(dpy,0),DefaultColormap(dpy,0),xrcolor,&xftcolor);

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
		if (line_new_ulen >= line_ulen) {	// just draw the end of str and finish
			XftTextExtentsUtf8(dpy, font, (unsigned char *)line,
					   line_clen, &ext);
			finish_after_draw = true;
			goto Draw;
		}
		/* ACTIVE CHANGE: */
		line_ulen = line_new_ulen;
		line_to_sym = line_from_sym + line_ulen;	// try to cut here
		line_to_char = utf8index(str, line_to_sym);
		line[line_to_char - line_from_char] = '\0';
		line_clen = strlen(line);
		XftTextExtentsUtf8(dpy, font, (unsigned char *)line, line_clen,
				   &ext);
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
					line, line_ulen, line_clen, ext.width,
					width);
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
		x += (width - ext.width) / 2;	// center
		XftDrawStringUtf8(xftdraw, xftcolor, font, x + ext.x, y + ext.y,
				  (unsigned char *)line, line_clen);
		if (debug > 0) {
			GC gc = DefaultGC(dpy, 0);
			XSetForeground(dpy, gc, WhitePixel(dpy, 0));
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
	Display *dpy = XOpenDisplay(0);;
	Window win =
	    XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1000, 600, 0,
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
		strncat(line, add, 25);

	GC gc = DefaultGC(dpy, 0);
	XSetForeground(dpy, gc, WhitePixel(dpy, 0));
	XDrawRectangle(dpy, win, gc, 100, 100, 500, 400);
	int r =
	    drawMultiLine(dpy, win, font, &xftcolor, line, 100, 100, 500, 400);

	XFlush(dpy);
	sleep(2);

	XftFontClose(dpy, font);
	return r;
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
//
char *get_x_property(Display * dpy, Window win, Atom prop_type, char *prop_name,
		     unsigned long *prop_size)
{

	int prop_ret_fmt;
	unsigned long n_prop_ret_items, ret_bytes_after, size;
	unsigned char *ret_prop;
	char *r;
	Atom prop_name_x, prop_ret_type_x;

	int debug = 0;

	prop_name_x = XInternAtom(dpy, prop_name, False);

	ee_complain = false;
	Status propstatus =
	    XGetWindowProperty(dpy, win, prop_name_x, 0, MAXPROPLEN / 4, False,
			       prop_type, &prop_ret_type_x, &prop_ret_fmt,
			       &n_prop_ret_items, &ret_bytes_after, &ret_prop);
	XSync(dpy, False);	// for error to "appear"
	ee_complain = true;

	if (propstatus != Success) {
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
