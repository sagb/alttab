/*
Interface with foreign windows common for all WMs.

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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <X11/Xatom.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <utlist.h>
//#include <sys/time.h>
#include "alttab.h"
#include "util.h"
extern Globals g;
extern Display *dpy;
extern int scr;
extern Window root;

// PRIVATE

//
// helper for windows' qsort
// CAUSES O(log(N)) OR EVEN WORSE! reintroduce winlist[]->order instead?
//
static int sort_by_order(const void *p1, const void *p2)
{
    PermanentWindowInfo *s;
    WindowInfo *w1 = (WindowInfo *) p1;
    WindowInfo *w2 = (WindowInfo *) p2;
    int r = 0;

    DL_FOREACH(g.sortlist, s) {
        if (s->id == w1->id) {
            r = (s->id == w2->id) ? 0 : -1;
            break;
        }
        if (s->id == w2->id) {
            r = 1;
            break;
        }
    }
    //msg(0, "cmp 0x%lx <-> 0x%lx = %d\n", w1->id, w2->id, r);
    return r;
}

//
// debug output of sortlist
//
static void print_sortlist()
{
    PermanentWindowInfo *s;
    msg(0, "sortlist:\n");
    DL_FOREACH(g.sortlist, s) {
        msg(0, "  0x%lx\n", s->id);
    }
}

//
// debug output of winlist
//
static void print_winlist()
{
    int wi, si;
    PermanentWindowInfo *s;

    if (g.winlist == NULL && g.maxNdx == 0)
        return;                 // safety
    msg(0, "winlist:\n");
    for (wi = 0; wi < g.maxNdx; wi++) {
        si = 0;
        DL_FOREACH(g.sortlist, s) {
            if (s == NULL)
                continue;       // safety
            if (s->id == g.winlist[wi].id)
                break;
            si++;
        }
        msg(0, "  %4d: 0x%lx  %s\n", si, g.winlist[wi].id, g.winlist[wi].name);
    }
}

// PUBLIC

//
// add Window to the head/tail of sortlist, if it's not in sortlist already
// if move==true and the item is already in sortlist, then move it to the head/tail
//
void addToSortlist(Window w, bool to_head, bool move)
{
    PermanentWindowInfo *s;
    bool was = false;
    bool add = false;

    DL_SEARCH_SCALAR(g.sortlist, s, id, w);
    if (s == NULL) {
        s = malloc(sizeof(PermanentWindowInfo));
        if (s == NULL)
            return;
        s->id = w;
        add = true;
    } else {
        was = true;
        if (move) {
            DL_DELETE(g.sortlist, s);
            add = true;
        }
    }
    if (add) {
        if (to_head) {
            DL_PREPEND(g.sortlist, s);
        } else {
            DL_APPEND(g.sortlist, s);
        }
    }
    if (add && !was) {
        // new window
        // register interest in events
        x_setCommonPropertiesForAnyWindow(w);
    }
}

//
// early initialization
// once per execution
//
int startupWintasks()
{
    long rootevmask = 0;

    g.sortlist = NULL;          // utlist head must be initialized to NULL
    g.ic = NULL;                // uthash too
    if (g.option_iconSrc != ISRC_RAM && g.option_iconSrc != ISRC_NONE) {
        initIconHash(&(g.ic));
    }
    // root: watching for _NET_ACTIVE_WINDOW
    if (g.option_wm == WM_EWMH) {
        g.naw = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", true);
        rootevmask |= PropertyChangeMask;
    }
    // warning: this overwrites any previous value.
    // note: x_setCommonPropertiesForAnyWindow does the similar thing
    // for any window other than root and uiwin
    if (rootevmask != 0) {
        XSelectInput(dpy, root, rootevmask);
    }

    switch (g.option_wm) {
    case WM_NO:
        return 1;
    case WM_RATPOISON:
        return rp_startupWintasks();
    case WM_EWMH:
        return 1;
    case WM_TWM:
        return 1;
    default:
        return 0;
    }
}

//
// search for suitable icon in NET_WM_ICON window property
// https://specifications.freedesktop.org/wm-spec/latest/ar01s05.html#_NET_WM_ICON
// if found, then
//   fill in "wi->icon_pixmap" and "wi->icon_mask"
//   and return 1,
// 0 otherwise.
//
#define best_w  pro[best-2]
#define best_h  pro[best-1]
//
int addIconFromProperty(WindowInfo * wi)
{
    long *pro;
    long unsigned n, nelem, best;
    unsigned int w, h;
    const char *NWI = "_NET_WM_ICON";
    uint32_t *image32;
    uint32_t fg;
    uint8_t alpha;
    int x, y;
    int bitmap_pad;
    int bytes_per_line;
    XImage *img;
    GC gc;

    if ((pro = (long *) get_x_property(wi->id, XA_CARDINAL,
                    (char*)NWI, &nelem)) == NULL) {
        msg(1, "Can't find %s (%lx, %s)\n", NWI, wi->id, wi->name);
        return 0;
    }
    nelem = nelem / sizeof(long);
    msg (1, "Found %lu elements in %s (%lx, %s)\n", nelem, NWI, wi->id, wi->name);
    best = 0;
    n = 0;
    while (n + 2 < nelem) {
        w = pro[n++];
        h = pro[n++];
        if (n + w*h > nelem || w == 0 || h == 0) {
            msg(1, "Skipping invalid %s icon: element=%lu/%lu w*h=%lux%lu\n", NWI, n, nelem, w, h);
            n += w*h;
            continue;
        }
        if (best == 0 || iconMatchBetter(w, h, best_w, best_h, false)) {
            best = n;
        }
        n += w*h;
    }
    if (best == 0) {
        msg(0, "%s found but no suitable icons in it\n", NWI);
        free(pro);
        return 0;
    }
    msg(1, "using %dx%d %s icon for %lx\n", w, h, NWI, wi->id);

    image32 = malloc(best_w * best_h * 4);
    CompositeConst cc = initCompositeConst(g.color[COLBG].xcolor.pixel);
    for (y = 0; y < best_h; y++) {
        for (x = 0; x < best_w; x++) {
            int ndx = y*best_w + x;
            // pro is ARGB by definition
            fg = pro[best + ndx] & 0x00ffffff;
            alpha = pro[best + ndx] >> 24;
            image32[ndx] = pixelComposite(fg, alpha, &cc);
        }
    }
    // result: image32

    bitmap_pad = (XDEPTH == 15 || XDEPTH == 16) ? 16 : 32;
    bytes_per_line = 0;
    img = XCreateImage(dpy, CopyFromParent, XDEPTH, ZPixmap, 0, (char*)image32, best_w, best_h, bitmap_pad, bytes_per_line);
    if (!img) {
        msg(0, "Can't XCreateImage, abort %s search\n", NWI);
        free(image32);
        free(pro);
        return 0;
    }
    wi->icon_drawable = XCreatePixmap(dpy, root, best_w, best_h, XDEPTH);
    gc = DefaultGC(dpy, scr);
    XPutImage(dpy, wi->icon_drawable, gc, img, 0, 0, 0, 0, best_w, best_h);
    wi->icon_mask = 0;
    wi->icon_allocated = true;
    wi->icon_w = best_w;
    wi->icon_h = best_h;
#ifdef ICON_DEBUG
    snprintf(wi->icon_src, MAXNAMESZ, "from %s", NWI);
#endif
    XFree(img);
    free(image32);
    free(pro);
    return 1;
}

//
// search for icon in WM hints of "wi".
// if found, then
//   fill in "wi->icon_pixmap" and "wi->icon_mask"
//   and return 1,
// 0 otherwise.
//
int addIconFromHints(WindowInfo * wi)
{
    XWMHints *hints;
    Pixmap hicon, hmask;

    hicon = hmask = 0;
    if ((hints = XGetWMHints(dpy, wi->id))) {
        msg(1,
            "IconPixmapHint: %ld, icon_pixmap: %lu, IconMaskHint: %ld, icon_mask: %lu, IconWindowHint: %ld, icon_window: %lu\n",
            hints->flags & IconPixmapHint,
            hints->icon_pixmap, hints->flags & IconMaskHint,
            hints->icon_mask, hints->flags & IconWindowHint,
            hints->icon_window);
        if ((hints->flags & IconWindowHint) &
            (!(hints->flags & IconPixmapHint))) {
            msg(0, "icon_window without icon_pixmap in hints, ignoring\n"); // not usable in xterm?
        }
        hicon = (hints->flags & IconPixmapHint) ? hints->icon_pixmap : 0;
//            ((hints->flags & IconPixmapHint) ?  hints->icon_pixmap : (
//            (hints->flags & IconWindowHint) ?  hints->icon_window : 0));
        hmask = (hints->flags & IconMaskHint) ? hints->icon_mask : 0;
        XFree(hints);
        if (hicon)
            msg(0, "no icon in WM hints (%s)\n", wi->name);
    } else {
        msg(0, "no WM hints (%s)\n", wi->name);
    }
    if (hicon == 0)
        return 0;
    wi->icon_drawable = hicon;
    if (hmask != 0)
        wi->icon_mask = hmask;
#ifdef ICON_DEBUG
    strcpy(wi->icon_src, "from WM hints");
#endif
    return 1;
}

//
// search for "wi" application class in PNG hash.
// if found, then
//   if program options don't request size comparison
//   OR png size match better, then
//     fill in "wi->icon_pixmap" and "wi->icon_mask"
//     and return 1
// return 0 otherwise.
// slow disk operations possible.
//
int addIconFromFiles(WindowInfo * wi)
{
    char *appclass, *tryclass, *s;
    long unsigned int class_size;
    icon_t *ic;
    int ret = 0;

    appclass = get_x_property(wi->id, XA_STRING, "WM_CLASS", &class_size);
    if (appclass) {
        for (tryclass = appclass; tryclass - appclass < class_size;
             tryclass += (strlen(tryclass) + 1)) {
            s = tryclass;
            while ((s = strchr(s, '/')) != NULL) *s++ = '_';
            ic = lookupIcon(tryclass);
            if (ic && (
                (g.option_iconSrc != ISRC_SIZE 
                        && g.option_iconSrc != ISRC_SIZE2)
                 || (g.option_iconSrc == ISRC_SIZE 
                        && iconMatchBetter(
                                    ic->src_w, ic->src_h,
                                    wi->icon_w, wi->icon_h,
                                    false))
                 || (g.option_iconSrc == ISRC_SIZE2 
                        && iconMatchBetter(
                                    ic->src_w, ic->src_h,
                                    wi->icon_w, wi->icon_h,
                                    true))
                )) {
                msg(0, "using file icon for %s\n", tryclass);
                if (ic->drawable == None) {
                    msg(1, "loading content for %s\n", ic->app);
                    if (loadIconContent(ic) == 0) {
                        msg(-1, "can't load file icon content: %s\n", ic->src_path);
                        continue;
                    }
                }
                // for the case when icon was already found in window props
                if (wi->icon_allocated) {
                    XFreePixmap(dpy, wi->icon_drawable);
                    /*
                    if (wi->icon_mask != None) {
                       XFreePixmap(dpy, wi->icon_mask);
                    }
                    */
                    wi->icon_allocated = false;
                }
                wi->icon_drawable = ic->drawable;
                wi->icon_mask = ic->mask;
#ifdef ICON_DEBUG
                strncpy(wi->icon_src, ic->src_path, MAXNAMESZ);
#endif
                ret = 1;
                goto out;
            }
        }
    } else {
        msg(0, "can't find WM_CLASS for \"%s\"\n", wi->name);
    }
out:
    free(appclass);
    return ret;
}

//
// add single window info into g.winlist and fix g.sortlist
// used by x, rp, ...
// only dpy and win are mandatory
//
#define WI g.winlist[g.maxNdx]  // current WindowInfo
int addWindowInfo(Window win, int reclevel, int wm_id, unsigned long desktop,
                  char *wm_name)
{
    if (!(g.winlist = realloc(g.winlist, (g.maxNdx + 1) * sizeof(WindowInfo))))
        return 0;
    WI.id = win;
    WI.wm_id = wm_id;

// 1. get name

    if (wm_name) {
        strncpy(WI.name, wm_name, MAXNAMESZ-1);
    } else {
        // handle COMPOUND WM_NAME, see #177.
        XTextProperty text_prop;
        char **list = NULL;
        int count;
        if (XGetWMName(dpy, win, &text_prop) && text_prop.value) {
            // trying to interpret the name as a UTF-8
            if (Xutf8TextPropertyToTextList(dpy, &text_prop, &list, &count) >= Success && count > 0 && list) {
                strncpy(WI.name, list[0], MAXNAMESZ - 1);
                WI.name[MAXNAMESZ - 1] = '\0';
                XFreeStringList(list);
            } else {
                strncpy(WI.name, (char *)text_prop.value, MAXNAMESZ - 1);
                WI.name[MAXNAMESZ - 1] = '\0';
            }
            XFree(text_prop.value);
        } else {
            WI.name[0] = '\0';
        }
    }                           // guessing name without WM hints

// 2. icon

// options:
// * WM_HINTS: https://tronche.com/gui/x/xlib/ICC/client-to-window-manager/wm-hints.html
// * load icons from files.
// * use full windows as icons. https://www.talisman.org/~erlkonig/misc/x11-composite-tutorial/
//      it's more sophisticated than icon_drawable=win, because hidden window contents aren't available.
// * understand hints->icon_window (twm concept, xterm).

    WI.icon_drawable =
        WI.icon_mask =
        WI.icon_w = WI.icon_h = 0;
    unsigned int icon_depth = 0;
    WI.icon_allocated = false;
#ifdef ICON_DEBUG
    WI.icon_src[0] = '\0';
#endif

    // search for icon in window properties, hints or file hash
    int opt = g.option_iconSrc;
    int icon_in_x = 0;
    if (opt == ISRC_NONE)
        goto endIcon;
    if (opt != ISRC_FILES) {
        icon_in_x = addIconFromProperty(&(WI));
        if (!icon_in_x)
            icon_in_x = addIconFromHints(&(WI));
    }
    if ((opt == ISRC_FALLBACK && !icon_in_x) ||
        opt == ISRC_SIZE || opt == ISRC_SIZE2 || opt == ISRC_FILES)
        addIconFromFiles(&(WI));

    // extract icon width/height/depth
    Window root_return;
    int x_return, y_return;
    unsigned int border_width_return;
    if (WI.icon_drawable) {
        if (XGetGeometry(dpy, WI.icon_drawable,
                         &root_return, &x_return, &y_return,
                         &(WI.icon_w),
                         &(WI.icon_h),
                         &border_width_return, &icon_depth) == 0) {
            msg(0, "icon dimensions unknown (%s)\n", WI.name);
            // probably draw placeholder?
            WI.icon_drawable = 0;
        } else {
            msg(1, "depth=%d\n", icon_depth);
        }
    }
// convert icon with different depth (currently 1 only) into default depth
    if (WI.icon_drawable && icon_depth == 1) {
        msg(0,
            "rebuilding icon from depth %d to %d (%s)\n",
            icon_depth, XDEPTH, WI.name);
        Pixmap pswap = XCreatePixmap(dpy, WI.icon_drawable,
                                     WI.icon_w,
                                     WI.icon_h, XDEPTH);
        if (!pswap)
            die("can't create pixmap");
        // GC should be already prepared in uiShow
        if (!XCopyPlane
            (dpy, WI.icon_drawable, pswap, g.gcDirect,
             0, 0, WI.icon_w,
             WI.icon_h, 0, 0, 1))
            die("can't copy plane");    // plane #1?
        WI.icon_drawable = pswap;
        WI.icon_allocated = true;  // for subsequent free()
        icon_depth = XDEPTH;
    }
    if (WI.icon_drawable && icon_depth != XDEPTH) {
        msg(-1,
            "can't handle icon depth other than %d or 1 (%d, %s). Please report this condition.\n",
            XDEPTH, icon_depth, WI.name);
        WI.icon_drawable = WI.icon_w =
            WI.icon_h = 0;
    }
endIcon:

// 3. sort

    addToSortlist(win, false, false);

// 4. bottom line

    WI.bottom_line[0] = '\0';
    long unsigned int nws, *pid;
    char procd[32];
    int sr;
    struct stat st;
    struct passwd *gu;
    switch(g.option_bottom_line) {
        case BL_DESKTOP:
            if (desktop != DESKTOP_UNKNOWN)
                snprintf(WI.bottom_line, MAXNAMESZ, "%ld", desktop);
            else
                strncpy(WI.bottom_line, "?", 2);
            break;
        case BL_USER:
            pid = (long unsigned int*)get_x_property(WI.id,
                    XA_CARDINAL, "_NET_WM_PID", &nws);
            if (!pid) {
                strncpy(WI.bottom_line, "[no pid]", 9);
                break;
            }
            snprintf(procd, 32, "/proc/%ld", *pid);
            sr = stat(procd, &st);
            if (sr == -1) {
                strncpy(WI.bottom_line, "[no /proc]", 11);
                break;
            }
            gu = getpwuid(st.st_uid);
            if (gu == NULL) {
                strncpy(WI.bottom_line, "[no name]", 10);
                break;
            }
            snprintf(WI.bottom_line, MAXNAMESZ, "%s", gu->pw_name);
            break;
    }

// 5. other window data

    WI.reclevel = reclevel;
    WI.desktop = desktop;

    g.maxNdx++;
    msg(1, "window %d, id %lx added to list\n", g.maxNdx, win);
    return 1;
}                               // addWindowInfo()

static void __initWinlist(void)
{
    free(g.winlist);
    g.winlist = NULL;
    g.maxNdx = 0;
}


//
// sets g.winlist, g.maxNdx
// updates g.sortlist, g.sortNdx
// n.b.: in heavy WM, use _NET_CLIENT_LIST
// direction is direction of first press: with shift or without
//
int initWinlist(void)
{
    int r;
    if (g.debug > 1) {
        msg(1, "before initWinlist\n");
        print_sortlist();
    }
    switch (g.option_wm) {
    case WM_NO:
        r = x_initWindowsInfoRecursive(root, 0);    // note: direction/current window index aren't used
        break;
    case WM_RATPOISON:
        r = rp_initWinlist();
        break;
    case WM_EWMH:
        r = ewmh_initWinlist();
        break;
    case WM_TWM:
        r = x_initWindowsInfoRecursive(root, 0);
        break;
    default:
        r = 0;
        break;
    }

    if (!r)
        __initWinlist();

// sort winlist according to .order
    if (g.debug > 1) {
        msg(1, "before qsort\n");
        print_sortlist();
        print_winlist();
    }
    qsort(g.winlist, g.maxNdx, sizeof(WindowInfo), sort_by_order);
    if (g.debug > 1) {
        msg(1, "after qsort\n");
        print_winlist();
    }

    msg(1, "initWinlist ret: number of items in winlist: %d\n", g.maxNdx);

    return r;
}

//
// counterpair for initWinlist
// frees icons and winlist, but not tiles, as they are allocated in gui.c
//
void freeWinlist()
{
    msg(0, "destroying icons and winlist\n");
    if (g.debug > 1) {
        msg(1, "before freeWinlist\n");
        print_sortlist();
    }
    int y;
    for (y = 0; y < g.maxNdx; y++) {
        if (g.winlist[y].icon_allocated)
            XFreePixmap(dpy, g.winlist[y].icon_drawable);
    }
    __initWinlist();
}

//
// popup/focus this X window
//
int setFocus(int winNdx)
{
    int r;
    switch (g.option_wm) {
    case WM_NO:
        r = ewmh_setFocus(winNdx, 0);   // for WM which isn't identified as EWMH compatible but accepts setting focus (dwm)
        x_setFocus(winNdx);
        break;
    case WM_RATPOISON:
        r = rp_setFocus(winNdx);
        break;
    case WM_EWMH:
        r = ewmh_setFocus(winNdx, 0);
        // XSetInputFocus stuff.
        // skippy-xd does it and notes that "order is important".
        // fixes #28.
        // it must be protected by testing IsViewable in the same way
        // as in x.c, or BadMatch happens after switching desktops.
        XWindowAttributes att;
        XGetWindowAttributes(dpy, g.winlist[winNdx].id, &att);
        if (att.map_state == IsViewable)
            XSetInputFocus(dpy, g.winlist[winNdx].id, RevertToParent,
                           CurrentTime);
        break;
    case WM_TWM:
        r = ewmh_setFocus(winNdx, 0);
        x_setFocus(winNdx);
        break;
    default:
        return 0;
    }
    // pull to head
    addToSortlist(g.winlist[winNdx].id, true, true);
    return r;
}

//
// event handler for PropertyChange
// most of the time called when winlist[] is not initialized
// currently it updates sortlist on _NET_ACTIVE_WINDOW change
// because it's a handler of frequent event,
// there are shortcuts to return as soon as possible
//
void winPropChangeEvent(XPropertyEvent e)
// man XPropertyEvent
{
    Window aw;
    // no _NET_ACTIVE_WINDOW atom, probably not EWMH?
    if (g.naw == None)
        return;
    // property change event not for root window?
    if (e.window != root)
        return;
    // root property other than _NET_ACTIVE_WINDOW changed?
    if (e.atom != g.naw)
        return;
    // don't check for wm==EWMH, because _NET_ACTIVE_WINDOW changed for sure
    aw = ewmh_getActiveWindow();
    // can't get active window
    if (!aw)
        return;
    // focus changed to our own old/current/zero window?
    if (aw == getUiwin())
        return;
    // focus changed to window which is already top?
    if (g.sortlist != NULL && aw == g.sortlist->id)
        return;
    // is window hidden in WM?
    if (ewmh_skipWindowInTaskbar(aw))
        return;
/*
    // the i3 sortlist bug is not here, see ewmh.c init_winlist instead
    //
    if (aw == g.last.prev) {
        struct timeval ctv;
        int usec_delta;
        gettimeofday(&ctv, NULL);
        usec_delta = (ctv.tv_sec - g.last.tv.tv_sec) * 1E6
          + (ctv.tv_usec - g.last.tv.tv_usec);
        msg(0, "delta %d\n", usec_delta);
        if (usec_delta < 5E5) {  // half a second
            return;
        }
        msg(0, PREF"pulling 'prev' 0x%lx suppressed\n", aw);
    }
    if (aw == g.last.to) {
        msg(0, PREF"pulling 'to' 0x%lx suppressed\n", aw);
        return;
    }
*/
    // finally, add/pop window to the head of sortlist
    // unfortunately, on focus by alttab, this is fired twice:
    // 1) when alttab window gone, previous window becomes active,
    // 2) then alttab-focused window becomes active.
    msg(0,
        "event PropertyChange: 0x%lx active, pull to the head of sortlist\n",
        aw);
    addToSortlist(aw, true, true);
}

//
// DestroyNotify handler
// removes the window from sortlist
//
void winDestroyEvent(XDestroyWindowEvent e)
// man XDestroyWindowEvent
{
    PermanentWindowInfo *s;

    DL_SEARCH_SCALAR(g.sortlist, s, id, e.window);
    if (s != NULL) {
        msg(1,
            "event DestroyNotify: 0x%lx found in sortlist, removing\n",
            e.window);
        DL_DELETE(g.sortlist, s);
    }
}

//
// common filter before adding window to winlist
// it checks: desktop, screen
// depending on global options and dimensions
//
bool common_skipWindow(Window w,
                       unsigned long current_desktop,
                       unsigned long window_desktop)
{
    quad wq;                    // window's absolute coordinates

    if (g.option_desktop == DESK_CURRENT
        && current_desktop != window_desktop
        && current_desktop != DESKTOP_UNKNOWN
        && window_desktop != DESKTOP_UNKNOWN) {
        msg(1,
            "window not on active desktop, skipped (window's %ld, current %ld)\n",
            window_desktop, current_desktop);
        return true;
    }
    if (g.option_desktop == DESK_NOSPECIAL && window_desktop == -1) {
        msg(1, "window on -1 desktop, skipped\n");
        return true;
    }
    if (g.option_desktop == DESK_NOCURRENT &&
        (window_desktop == current_desktop || window_desktop == -1)) {
        msg(1, "window on current or -1 desktop, skipped\n");
        return true;
    }
    if (g.option_desktop == DESK_SPECIAL && window_desktop != -1) {
        msg(1, "window not on -1 desktop, skipped\n");
        return true;
    }
    // man page: -sc 0: Screen is defined according to -vp pointer or -vp focus.
    // assuming g.vp already calculated in gui.c
    if (g.option_screen == SCR_CURRENT &&
        (g.option_vp_mode == VP_POINTER || g.option_vp_mode == VP_FOCUS)) {
        if (!get_absolute_coordinates(w, &wq)) {
            msg(-1,
                "can't get coordinates of window 0x%lx, included anyway\n", w);
        } else {
            if (!rectangles_cross(g.vp, wq)) {
                msg(1,
                    "window's area doesn't cross with current screen, skipped\n");
                return true;
            }
        }
    }

    return false;
}

//
// focus change handler
// does the same as _NET_ACTIVE_WINDOW handler
// but in non-EWMH environment
//
void winFocusChangeEvent(XFocusChangeEvent e)
{
    Window w;
    // in non-EWMH only
    // probably should also maintain _NET_ACTIVE_WINDOW
    // support flag in EwmhFeatures
    if (g.option_wm == WM_EWMH)
        return;
    // focusIn only
    if (e.type != FocusIn)
        return;
    // skip Grab/Ungrab notification modes
    if (e.mode != NotifyNormal)
        return;
    // focus changed to our own old/current/zero window?
    w = e.window;
    if (w == getUiwin())
        return;
    // focus changed to window which is already top?
    if (g.sortlist != NULL && w == g.sortlist->id)
        return;

    msg(1, "event focusIn 0x%lx, pull to the head of sortlist\n", w);
    addToSortlist(w, true, true);
}

void shutdownWin()
{
    deleteIconHash(&g.ic);
}
