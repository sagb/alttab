/*
Interface with Ratpoison window manager.

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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "alttab.h"
#include "util.h"
extern Globals g;

// PRIVATE

#define RP_MAXGROUPS    64

static char *ratpoison_cmd;

//
// process ratpoison window_group, adding its windows
// to winlist/sortlist.
// window_group must be selected in advance.
//
static int rp_add_windows_in_group(int current_group, int window_group)
{
    char *args[] = { "ratpoison", "-c", "windows %n %i %s %t", NULL };
    char buf[MAXRPOUT];
    char *rpse = "bad token while parsing ratpoison output: ";

    bzero(buf, MAXRPOUT);
    if (!execAndReadStdout(ratpoison_cmd, args, buf, MAXRPOUT)) {
        msg(-1, "can't exec ratpoison\n");
        return 0;
    }
    msg(1, "windows in current rp group:\n%s", buf);
    char *rest = buf;
    char *tok;

    if (strstr(buf, "No managed windows")) {
        // no windows at all.
        return 0;
    } else {
        while ((tok = strsep(&rest, "\r\n")) && (*tok)) {
            // parse single line
            char *rest2 = tok;
            char *tok2;
            char *endptr;
            if (!((tok2 = strsep(&rest2, " \t")) && (*tok2)))
                die(rpse, rest2);
            int wm_id = strtol(tok2, &endptr, 10);
            if (endptr == tok2)
                die(rpse, tok2);
            if (!((tok2 = strsep(&rest2, " \t")) && (*tok2)))
                die(rpse, rest2);
            int win = strtol(tok2, &endptr, 10);
            if (endptr == tok2)
                die(rpse, tok2);
            if (!((tok2 = strsep(&rest2, " \t")) && (*tok2)))
                die(rpse, rest2);
            switch (*tok2) {
            case '*':
                // pull to head
                // g.winlist isn't here yet!
                addToSortlist(win, true, true);
                break;
            case '+':
            case '-':
                break;          // use?
            default:
                break;
            }
            if (common_skipWindow(win, current_group, window_group))
                return 0;
            // the rest of string is name
            addWindowInfo(win, 0, wm_id, window_group, rest2);
        }
    }
    return 1;
}

// PUBLIC

//
// early initialization in ratpoison
//
int rp_startupWintasks()
{

// search for ratpoison executable for later execv
    ratpoison_cmd = (char *)malloc(MAXPATHSZ);  // we don't free it, hopefully run only once
    FILE *fp;
    char *fr;
// search in PATH on startup only,
// then execv() for speed
    if ((fp = popen("which ratpoison", "r"))) {
        fr = fgets(ratpoison_cmd, MAXPATHSZ, fp);
        if (fr == NULL) {
            msg(-1, "can't find ratpoison executable\n");
            return 0;
        }
        pclose(fp);
    }
    if (strlen(ratpoison_cmd) < 2) {
        msg(-1, "can't find ratpoison executable\n");
        return 0;
    } else {
        ratpoison_cmd[strcspn(ratpoison_cmd, "\r\n")] = 0;
        msg(0, "ratpoison: %s\n", ratpoison_cmd);
    }

// insert myself in "unmanaged windows" list
// this is not required, as soon as ratpoison obeys DIALOG window type,
// but without "unmanage" it draws ugly additional frame around our window.
    char *ua[] = { "ratpoison", "-c", "unmanage", NULL };
    char *uains[] = { "ratpoison", "-c", "unmanage " XWINNAME, NULL };
    char buf[MAXRPOUT];
    memset(buf, '\0', MAXRPOUT);
    if (!execAndReadStdout(ratpoison_cmd, ua, buf, MAXRPOUT)) {
        msg(-1, "can't get unmanaged window list from ratpoison\n");
        return 1;               // not fatal
    }
    msg(1, "ratpoison reports unmanaged:\n%s", buf);
    if (!strstr(buf, XWINNAME)) {
        msg(0, "registering in ratpoison unmanaged list\n");
        //memset (buf, '\0', MAXRPOUT);
        if (!execAndReadStdout(ratpoison_cmd, uains, buf, MAXRPOUT)) {
            msg(-1, "can't register in ratpoison unmanaged window list\n");
            return 1;           // not fatal
        }
    } else {
        msg(0, "our window is already in ratpoison unmanaged list\n");
    }

    return 1;
}

//
// initialize winlist/update sortlist from ratpoison output
//
int rp_initWinlist()
{
#define  fallback    { msg(-1, "using current rp group\n") ; \
    return rp_add_windows_in_group(DESKTOP_UNKNOWN, DESKTOP_UNKNOWN); }
#define  arg2len  19
#define  intlen   10
    char arg2[arg2len];         // fits 'groups', 'gselect N'
    char *args[] = { "ratpoison", "-c", arg2, NULL };
    char buf[MAXRPOUT];
    int gr[RP_MAXGROUPS];
    int ngr, gri;               // numbers from 'groups' output
    int w_group;                // window group
    int c_group = DESKTOP_UNKNOWN;  // current group
    char *endptr;
    char *rest;
    char *tok;

    g.maxNdx = 0;
    if (g.option_desktop == DESK_CURRENT)
        return rp_add_windows_in_group(DESKTOP_UNKNOWN, DESKTOP_UNKNOWN);

    // discover rp groups
    strncpy(arg2, "groups", arg2len);
    bzero(buf, MAXRPOUT);
    if (!execAndReadStdout(ratpoison_cmd, args, buf, MAXRPOUT))
        fallback rest = buf;
    for (ngr = 0;
         ((tok = strsep(&rest, "\r\n")) && (*tok) && ngr < RP_MAXGROUPS);
         ngr++) {
        // parse single line
        gr[ngr] = strtol(tok, &endptr, 10);
        if (endptr == tok) {
            msg(-1, "no group number detected in rp output\n");
        fallback}
        if (*endptr == '*')
            c_group = gr[ngr];
        // the rest of line is group name, skip
    }
    if (ngr < 1) {
        msg(0, "no ratpoison (rpws) groups detected\n");
    fallback}
    msg(0, "number of ratpoison (rpws) groups: %d, current: %d\n",
        ngr, c_group);

    // cycle through rp groups
    for (gri = 0; gri < ngr; gri++) {
        w_group = gr[gri];
        msg(0, "processing rp group %d\n", w_group);
        snprintf(arg2, arg2len, "gselect %d", w_group);
        if (!execAndReadStdout(ratpoison_cmd, args, NULL, 0))
            continue;
        rp_add_windows_in_group(c_group, w_group);
    }
    return 1;
}

//
// focus window in ratpoison
//
int rp_setFocus(int winNdx)
{
    char selarg[64];
    char *args[] = { "ratpoison", "-c", selarg, NULL };
    char buf[MAXRPOUT];

    // don't skip changing group if it's current, because
    // rp_initWinlist left current group inconsistent
    if (g.winlist[winNdx].desktop != DESKTOP_UNKNOWN) {
        snprintf(selarg, 63, "gselect %lu", g.winlist[winNdx].desktop);
        execAndReadStdout(ratpoison_cmd, args, buf, MAXRPOUT);
        // ignore possible failure
    }
    snprintf(selarg, 63, "select %d", g.winlist[winNdx].wm_id);
    if (!execAndReadStdout(ratpoison_cmd, args, buf, MAXRPOUT))
        return 0;

    return 1;
}
