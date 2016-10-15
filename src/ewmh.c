/*
Interface with EWMH-compatible window managers.

This file is part of alttab program.
alttab is Copyright (C) 2016, by respective author (sa).
It is free software; you can redistribute it and/or modify it under the terms of either:
a) the GNU General Public License as published by the Free Software Foundation; either version 1, or (at your option) any later version, or
b) the "Artistic License".

Parts are copied from wmctrl (c) Tomas Styblo <tripie@cpan.org>
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
//#include <locale.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
//#include <X11/cursorfont.h>
//#include <X11/Xmu/WinUtil.h>
//#include <glib.h>

#include <X11/Xft/Xft.h>
#include "alttab.h"
#include "util.h"
extern Globals g;

#define MAX_PROPERTY_VALUE_LEN 4096
//#define SELECT_WINDOW_MAGIC ":SELECT:"
//#define ACTIVE_WINDOW_MAGIC ":ACTIVE:"

#define p_verbose(...) if (g.debug>1) { \
    fprintf(stderr, __VA_ARGS__); \
}

// PUBLIC

int ewmh_client_msg(Display *disp, Window win, char *msg, /* {{{ */
        unsigned long data0, unsigned long data1, 
        unsigned long data2, unsigned long data3,
        unsigned long data4) {
    XEvent event;
    long mask = SubstructureRedirectMask | SubstructureNotifyMask;

    event.xclient.type = ClientMessage;
    event.xclient.serial = 0;
    event.xclient.send_event = True;
    event.xclient.message_type = XInternAtom(disp, msg, False);
    event.xclient.window = win;
    event.xclient.format = 32;
    event.xclient.data.l[0] = data0;
    event.xclient.data.l[1] = data1;
    event.xclient.data.l[2] = data2;
    event.xclient.data.l[3] = data3;
    event.xclient.data.l[4] = data4;
    
    if (XSendEvent(disp, DefaultRootWindow(disp), False, mask, &event)) {
        return EXIT_SUCCESS;
    }
    else {
        fprintf(stderr, "Cannot send %s event.\n", msg);
        return EXIT_FAILURE;
    }
}/*}}}*/


char *ewmh_get_property (Display *disp, Window win, /*{{{*/
        Atom xa_prop_type, char *prop_name, unsigned long *size) {
    Atom xa_prop_name;
    Atom xa_ret_type;
    int ret_format;
    unsigned long ret_nitems;
    unsigned long ret_bytes_after;
    unsigned long tmp_size;
    unsigned char *ret_prop;
    char *ret;

    xa_prop_name = XInternAtom(disp, prop_name, False);
    
    /* MAX_PROPERTY_VALUE_LEN / 4 explanation (XGetWindowProperty manpage):
     *
     * long_length = Specifies the length in 32-bit multiples of the
     *               data to be retrieved.
     *
     * NOTE:  see 
     * http://mail.gnome.org/archives/wm-spec-list/2003-March/msg00067.html
     * In particular:
     *
     * 	When the X window system was ported to 64-bit architectures, a
     * rather peculiar design decision was made. 32-bit quantities such
     * as Window IDs, atoms, etc, were kept as longs in the client side
     * APIs, even when long was changed to 64 bits.
     *
     */
    ee_complain = false;
    Status xsw = XGetWindowProperty(disp, win, xa_prop_name, 0, MAX_PROPERTY_VALUE_LEN / 4, False,
            xa_prop_type, &xa_ret_type, &ret_format,     
            &ret_nitems, &ret_bytes_after, &ret_prop);
    XSync (disp, False); // for error to "appear"
    ee_complain = true;

    if (xsw != Success) {
        p_verbose("Cannot get %s property.\n", prop_name);
        return NULL;
    }
  
    if (xa_ret_type != xa_prop_type) {
        p_verbose("Invalid type of %s property.\n", prop_name);
        XFree(ret_prop);
        return NULL;
    }

    /* null terminate the result to make string handling easier */
    tmp_size = (ret_format / 8) * ret_nitems;
    /* Correct 64 Architecture implementation of 32 bit data */
    if(ret_format==32) tmp_size *= sizeof(long)/4;
    ret = malloc(tmp_size + 1);
    memcpy(ret, ret_prop, tmp_size);
    ret[tmp_size] = '\0';

    if (size) {
        *size = tmp_size;
    }
    
    XFree(ret_prop);
    return ret;
}/*}}}*/


char *ewmh_get_window_title (Display *disp, Window win) {/*{{{*/
    char *title_utf8;
    char *wm_name;
    char *net_wm_name;

    wm_name = ewmh_get_property(disp, win, XA_STRING, "WM_NAME", NULL);
    net_wm_name = ewmh_get_property(disp, win, 
            XInternAtom(disp, "UTF8_STRING", False), "_NET_WM_NAME", NULL);

    if (net_wm_name) {
        title_utf8 = strdup(net_wm_name);
    }
    else {
        if (wm_name) {
            //title_utf8 = locale_to_utf8(wm_name, -1, NULL, NULL, NULL);  // TODO?
            title_utf8 = strdup (wm_name);
        }
        else {
            title_utf8 = NULL;
        }
    }

    free(wm_name);
    free(net_wm_name);
    
    return title_utf8;
}/*}}}*/


Window ewmh_get_active_window(Display *disp) {/*{{{*/
    char *prop;
    unsigned long size;
    Window ret = (Window)0;
    
    prop = ewmh_get_property(disp, DefaultRootWindow(disp), XA_WINDOW, 
                        "_NET_ACTIVE_WINDOW", &size);
    if (prop) {
        ret = *((Window*)prop);
        free(prop);
    }

    return(ret);
}/*}}}*/


int ewmh_activate_window (Display *disp, Window win, /* {{{ */
        bool switch_desktop) {
    unsigned long *desktop;

    /* desktop ID */
    if ((desktop = (unsigned long *)ewmh_get_property(disp, win,
            XA_CARDINAL, "_NET_WM_DESKTOP", NULL)) == NULL) {
        if ((desktop = (unsigned long *)ewmh_get_property(disp, win,
                XA_CARDINAL, "_WIN_WORKSPACE", NULL)) == NULL) {
            p_verbose("Cannot find desktop ID of the window.\n");
        }
    }

    if (switch_desktop && desktop) {
        if (ewmh_client_msg(disp, DefaultRootWindow(disp), 
                    "_NET_CURRENT_DESKTOP", 
                    *desktop, 0, 0, 0, 0) != EXIT_SUCCESS) {
            p_verbose("Cannot switch desktop.\n");
        }
        free(desktop);
    }

    ewmh_client_msg(disp, win, "_NET_ACTIVE_WINDOW", 
            0, 0, 0, 0, 0);
    XMapRaised(disp, win);

    return EXIT_SUCCESS;
}/*}}}*/


Window *ewmh_get_client_list (Display *disp, unsigned long *size) {/*{{{*/
    Window *client_list;

    if ((client_list = (Window *)ewmh_get_property(disp, DefaultRootWindow(disp), 
                    XA_WINDOW, "_NET_CLIENT_LIST", size)) == NULL) {
        if ((client_list = (Window *)ewmh_get_property(disp, DefaultRootWindow(disp), 
                        XA_CARDINAL, "_WIN_CLIENT_LIST", size)) == NULL) {
            fputs("Cannot get client list properties. \n"
                  "(_NET_CLIENT_LIST or _WIN_CLIENT_LIST)"
                  "\n", stderr);
            return NULL;
        }
    }

    return client_list;
}/*}}}*/


//
// ----------- MODIFIED functions from wmctrl --------------
//

//
// return the name of EWMH-compatible WM
// or NULL if not found
//
char* ewmh_wmName (Display *disp, Window root) {/*{{{*/
    Window *sup_window = NULL;
    char *wm_name = NULL;
    bool name_is_utf8 = true;
    
    if (! (sup_window = (Window *)ewmh_get_property(disp, root,
                    XA_WINDOW, "_NET_SUPPORTING_WM_CHECK", NULL))) {
        if (! (sup_window = (Window *)ewmh_get_property(disp, root,
                        XA_CARDINAL, "_WIN_SUPPORTING_WM_CHECK", NULL))) {
            p_verbose("Cannot get window manager info properties.\n"
                  "(_NET_SUPPORTING_WM_CHECK or _WIN_SUPPORTING_WM_CHECK)\n");
            return NULL;
        }
    }

    /* WM_NAME */
    if (! (wm_name = ewmh_get_property(disp, *sup_window,
            XInternAtom(disp, "UTF8_STRING", False), "_NET_WM_NAME", NULL))) {
        name_is_utf8 = false;
        if (! (wm_name = ewmh_get_property(disp, *sup_window,
                XA_STRING, "_NET_WM_NAME", NULL))) {
            p_verbose("Cannot get name of the window manager (_NET_WM_NAME).\n");
        }
    }
    //name_out = get_output_str(wm_name, name_is_utf8); // or provide utf, as wmctrl?
    
    free(sup_window);
    //free(wm_name); // TODO: leak ignored
    
    return wm_name;
}/*}}}*/

//
// ----------------- our own ewmh functions ---------------
//

//
// initialize winlist/selNdx
// return 1 if ok
//
int ewmh_initWinlist (Display* dpy, bool direction)
{

Window *client_list;
unsigned long client_list_size;
int i;

Window aw = ewmh_get_active_window (dpy);
int awi=-1; // its index in list

if ((client_list = ewmh_get_client_list (dpy, &client_list_size)) == NULL) {
    return 0;
}
for (i = 0; i < client_list_size / sizeof(Window); i++) {
    Window w = client_list[i];
    char *title_utf8 = ewmh_get_window_title (dpy, w); /* UTF8 */
    //char *title_out = get_output_str(title_utf8, true); // TODO: merge this from wmctrl?
    char *title_out = title_utf8;
    addWindowInfo (dpy, w, 0, 0, title_out);
    if (w == aw) { awi = i; }
}

if (g.debug>1) {fprintf(stderr, "ewmh active window: %d index: %d name: %s\n", aw, awi, g.winlist[awi].name);}
g.selNdx = direction ? 
    ( (awi<1 || awi>=g.maxNdx) ? (g.maxNdx-1) : (awi-1) ) :
    ( (awi<0 || awi>=(g.maxNdx-1)) ? 0 : awi+1 );

return 1;
}

//
// focus window in EWMH WM
//
int ewmh_setFocus (Display* dpy, int winNdx)
{
return (ewmh_activate_window (dpy, g.winlist[winNdx].id, true) == EXIT_SUCCESS) ? 1 : 0;
}

