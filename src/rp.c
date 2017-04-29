/*
Interface with Ratpoison window manager.

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
extern Globals g;

// PRIVATE

char *ratpoison_cmd;

// PUBLIC

//
// early initialization in ratpoison
//
int rp_startupWintasks()
{

// search for ratpoison executable for later execv
	ratpoison_cmd = (char *)malloc(MAXPATHSZ);	// we don't free it, hopefully run only once
	FILE *fp;
    char *fr;
    char *errpr = "can't find ratpoison executable\n";
// search in PATH on startup only,
// then execv() for speed
	if ((fp = popen("which ratpoison", "r"))) {
		fr = fgets(ratpoison_cmd, MAXPATHSZ, fp);
        if (fr == NULL) {
		    fprintf(stderr, errpr);
		    return 0;
        }
		pclose(fp);
	}
	if (strlen(ratpoison_cmd) < 2) {
		fprintf(stderr, errpr);
		return 0;
	} else {
		ratpoison_cmd[strcspn(ratpoison_cmd, "\r\n")] = 0;
		if (g.debug > 0) {
			fprintf(stderr, "ratpoison: %s\n", ratpoison_cmd);
		}
	}

// insert myself in "unmanaged windows" list
// this is not required, as soon as ratpoison obeys DIALOG window type,
// but without "unmanage" it draws ugly additional frame around our window.
	char *ua[] = { "ratpoison", "-c", "unmanage", NULL };
	char *uains[] = { "ratpoison", "-c", "unmanage " XWINNAME, NULL };
	char buf[MAXRPOUT];
	memset(buf, '\0', MAXRPOUT);
	if (!execAndReadStdout(ratpoison_cmd, ua, buf, MAXRPOUT)) {
		fprintf(stderr,
			"can't get unmanaged window list from ratpoison\n");
		return 1;	// not fatal
	}
	if (g.debug > 1) {
		fprintf(stderr, "ratpoison reports unmanaged:\n%s", buf);
	}
	if (!strstr(buf, XWINNAME)) {
		if (g.debug > 0) {
			fprintf(stderr,
				"registering in ratpoison unmanaged list\n");
		}
		//memset (buf, '\0', MAXRPOUT);
		if (!execAndReadStdout(ratpoison_cmd, uains, buf, MAXRPOUT)) {
			fprintf(stderr,
				"can't register in ratpoison unmanaged window list\n");
			return 1;	// not fatal
		}
	} else {
		if (g.debug > 0) {
			fprintf(stderr,
				"our window is already in ratpoison unmanaged list\n");
		}
	}

	return 1;
}

//
// initialize winlist/startNdx/update sortlist from ratpoison output
//
int rp_initWinlist(Display * dpy)
{

	char *args[] = { "ratpoison", "-c", "windows %n %i %s %t", NULL };
	char buf[MAXRPOUT];
	char *rpse = "bad token while parsing ratpoison output: ";

	memset(buf, '\0', MAXRPOUT);
	g.maxNdx = 0;
	if (!execAndReadStdout(ratpoison_cmd, args, buf, MAXRPOUT)) {
		fprintf(stderr, "can't exec ratpoison\n");
		return 0;
	}
	if (g.debug > 1) {
		fprintf(stderr, "ratpoison reports windows:\n%s", buf);
	}
	char *rest = buf;
	char *tok;

	if (strstr(buf, "No managed windows")) {
		// no windows at all.
		return 0;
	} else {
		while ((tok = strsep(&rest, "\r\n")) && (*tok)) {
			if (g.debug > 1) {
				fprintf(stderr, "token: %s\n", tok);
			}
			// parse single line
			char *rest2 = tok;
			char *tok2;
			char *endptr;
			if (!((tok2 = strsep(&rest2, " \t")) && (*tok2)))
				die2(rpse, rest2);
			int wm_id = strtol(tok2, &endptr, 10);
			if (endptr == tok2)
				die2(rpse, tok2);
			if (!((tok2 = strsep(&rest2, " \t")) && (*tok2)))
				die2(rpse, rest2);
			int win = strtol(tok2, &endptr, 10);
			if (endptr == tok2)
				die2(rpse, tok2);
			if (!((tok2 = strsep(&rest2, " \t")) && (*tok2)))
				die2(rpse, rest2);
			switch (*tok2) {
			case '*':
				g.startNdx = g.maxNdx;
				break;
			case '+':
				break;	// use?
			case '-':
				break;
			default:
				break;
			}
			// the rest of string is name
			addWindowInfo(dpy, win, 0, wm_id, rest2);
		}
	}

	return 1;
}

//
// focus window in ratpoison
//
int rp_setFocus(int winNdx)
{
	char selarg[64];
	snprintf(selarg, 63, "select %d", g.winlist[winNdx].wm_id);
	char *args[] = { "ratpoison", "-c", selarg, NULL };
	char buf[MAXRPOUT];

	if (!execAndReadStdout(ratpoison_cmd, args, buf, MAXRPOUT))
		return 0;

	return 1;
}
