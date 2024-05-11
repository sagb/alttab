/*
Helper functions specific to alttab.

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

#include "alttab.h"
#include "icon.h"
#include <signal.h>
#include <utlist.h>
#include <uthash.h>
extern Globals g;
//extern Display* dpy;
//extern int scr;
//extern Window root;

// PUBLIC

//
// diagnostic message
//
void msg(int lvl, const char *format, ...)
{
    if (g.debug > lvl) {
        if (lvl == -1) {
            fprintf(stderr, MSGPREFIX);
        }
        va_list ap;
        va_start(ap, format);
        vfprintf(stderr, format, ap);
        va_end(ap);
    }
}

//
// fatal exit with explanation
//
void die(const char *format, ...)
{
    va_list ap;
    fprintf(stderr, MSGPREFIX);
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    exit(1);
}

//
// signal handler
//
void sighandler(int signum)
{
    PermanentWindowInfo *pwi;
    int sln, in;
    switch (signum) {
        case SIGUSR1:
            fprintf(stderr, "debug information:\n");
            fprintf(stderr, "winlist: %d elements of %ld bytes, %ld total\n",
                    g.maxNdx, sizeof(WindowInfo), g.maxNdx*sizeof(WindowInfo) );
            DL_COUNT(g.sortlist, pwi, sln);
            fprintf(stderr, "sortlist: %d elements of %ld bytes, %ld total\n",
                    sln, sizeof(PermanentWindowInfo), sln*sizeof(PermanentWindowInfo) );
            in = HASH_COUNT(g.ic);
            fprintf(stderr, "icons: %d elements of %ld bytes, %ld total\n",
                    in, sizeof(icon_t), in*sizeof(icon_t) );
            fprintf(stderr, "option_wm: %d, option_desktop: %d, option_screen: %d\n",
                    g.option_wm, g.option_desktop, g.option_screen);
            fprintf(stderr, "option_font: '%s'\n", g.option_font);
            fprintf(stderr, "option_tileW/H: %dx%d, option_iconW/H: %dx%d\n",
                    g.option_tileW, g.option_tileH, g.option_iconW, g.option_iconH);
            fprintf(stderr, "option_vp_mode: %d, vp: [%dx%d+%d+%d], has_randr: %d\n",
                    g.option_vp_mode, g.vp.w, g.vp.h, g.vp.x, g.vp.y, g.has_randr);
            fprintf(stderr, "option_positioning: %d, option_posX/Y: %d, %d\n",
                g.option_positioning, g.option_posX, g.option_posY);
            fprintf(stderr, "option_iconSrc: %d, option_theme: '%s'\n",
                g.option_iconSrc, g.option_theme);
            fprintf(stderr, "ewmh: wmname '%s', tslf %d, -1du %d\n",
                g.ewmh.wmname, g.ewmh.try_stacking_list_first, g.ewmh.minus1_desktop_unusable);
            break;
    }
}

