/*
Icon object.

Copyright 2017-2019 Alexander Kulak.
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

#define _GNU_SOURCE
#include <string.h>
//#include "util.h"
#include "alttab.h"
#include "pngd.h"
#include "icon.h"
extern Globals g;
extern Display *dpy;
extern int scr;
extern Window root;

// PUBLIC:

//
// icon constructor: safe defaults
//
icon_t *initIcon()
{
    icon_t *ic;

    ic = (icon_t *) malloc(sizeof(icon_t));
    if (ic == NULL)
        return NULL;

    ic->app[0] = '\0';
    ic->src_path[0] = '\0';
    ic->src_w = ic->src_h = 0;
    ic->drawable = ic->mask = 0;
    ic->drawable_allocated = false;

    return ic;
}

//
// icon destructor
//
void deleteIcon(icon_t * ic)
{
    if (ic->drawable_allocated) {
        XFreePixmap(dpy, ic->drawable);
    }
    free(ic);
}

//
// hash constructor
// 1=success
//
int initIconHash(icon_t ** ihash)
{
    *ihash = NULL;              // required by uthash
    updateIconsFromFile(ihash);
    return 1;
}

//
// load all icons
// (no pixmaps, just path and dimension)
//
int updateIconsFromFile(icon_t ** ihash)
{
    int hd;
    char *id2;
    int id2len;
    char *icon_dirs[MAXICONDIRS];
    int d_c, f_c;
    FTS *ftsp;
    FTSENT *p, *chp;
    icon_t *iiter, *tmp;

    int fts_options = FTS_COMFOLLOW | FTS_LOGICAL | FTS_NOCHDIR;
    // XDG_DATA_DIRS ignored
    const char *icondir[] = {
        "/usr/share/icons",
        "/usr/local/share/icons",
        "~/.icons",
        "~/.local/share/icons",
        NULL
    };
    int idndx = 0;
    int theme_len = strlen(g.option_theme);
    char *home = getenv("HOME");
    int ret = 0;

    for (hd = 0; icondir[hd] != NULL; hd++) {
        id2len = strlen(icondir[hd]) + 1 + theme_len + 1;
        if (icondir[hd][0] == '~') {
            if (home == NULL)
                continue;
            id2len += strlen(home);
        }
        id2 = malloc(id2len);
        if (!id2)
            return 0;
        if (icondir[hd][0] == '~')
            snprintf(id2, id2len, "%s%s/%s", home, icondir[hd] + 1,
                     g.option_theme);
        else
            snprintf(id2, id2len, "%s/%s", icondir[hd], g.option_theme);
        id2[id2len - 1] = '\0';
        icon_dirs[idndx] = id2;
        idndx++;
    }
    icon_dirs[idndx] = NULL;
    if (g.debug > 1) {
        for (idndx = 0; icon_dirs[idndx] != NULL; idndx++)
            msg(1, "icon dir: %s\n", icon_dirs[idndx]);
    }

    if ((ftsp = fts_open(icon_dirs, fts_options, NULL)) == NULL) {
        warn("fts_open");
        goto out;
    }
    /* Initialize ftsp with as many icon_dirs as possible. */
    chp = fts_children(ftsp, 0);
    if (chp == NULL) {
        goto out;               /* no files to traverse */
    }
    d_c = f_c = 0;
    while ((p = fts_read(ftsp)) != NULL) {
        switch (p->fts_info) {
        case FTS_D:
            //printf("d %s\n", p->fts_path);
            d_c++;
            break;
        case FTS_F:
            //msg(1, "f %s\n", p->fts_path);
            inspectIconFile(p);
            f_c++;
            break;
        default:
            break;
        }
    }
    fts_close(ftsp);
    if (g.debug > 0) {
        msg(0, "icon dirs: %d, files: %d, apps: %d\n", d_c,
            f_c, HASH_COUNT(*ihash));
        if (g.debug > 1) {
            HASH_ITER(hh, *ihash, iiter, tmp) {
                msg(1, "app \"%s\" [%s] (%dx%d)\n",
                    iiter->app, iiter->src_path, iiter->src_w, iiter->src_h);
            }
        }
    }

    ret = 1;

out:
    for (idndx = 0; icon_dirs[idndx] != NULL; idndx++)
        free(icon_dirs[idndx]);

    return ret;
}                               // updateIconsFromFile

//
// check if the file has better icon for given app than in g.ic
// return 1 if this icon is used, 0 otherwise
//
int inspectIconFile(FTSENT * pe)
{
    char *point;
    char *fname;
    char app[MAXAPPLEN];
    int applen;
    char *xchar;
    char *dim;
    int dimlen;
    char sx[5];
    char sy[5];
    int ix, iy;
    icon_t *ic;
    char *suff;
    int sfxn;
    int tl;
    char *generic_suffixes[] = { "-color", NULL };

    // skip non-apps icons
    if (strcmp(pe->fts_parent->fts_name, "apps") != 0)
        return 0;

    // guess app
    fname = pe->fts_name;
    point = rindex(fname, '.');
    if (point == NULL)
        return 0;               // no extension?
    if (strcmp(point + 1, "png") != 0)
        return 0;               // going to load png only TODO: mention in doc
    applen = point - fname > (MAXAPPLEN - 1) ? (MAXAPPLEN - 1) : point - fname;
    strncpy(app, fname, applen);
    app[applen] = '\0';
    for (tl = 0; app[tl] != '\0'; tl++)
        app[tl] = tolower(app[tl]);

    // sort of generalization
    sfxn = 0;
    while (generic_suffixes[sfxn] != NULL) {
        suff = strcasestr(app, generic_suffixes[sfxn]);
        if (suff != NULL) {
            *suff = '\0';
        }
        sfxn++;
    }

    // guess dimensions
    dim = pe->fts_parent->fts_parent->fts_name;
    dimlen = pe->fts_parent->fts_parent->fts_namelen;
    xchar = index(dim, 'x');
    if (xchar == NULL)
        return 0;               // unknown dimensions
    strncpy(sx, dim, (xchar - dim));
    sx[xchar - dim] = '\0';
    ix = atoi(sx);
    strncpy(sy, xchar + 1, dim + dimlen - xchar);
    sy[dim + dimlen - xchar - 1] = '\0';
    iy = atoi(sy);

    // is app already in hash?
    HASH_FIND_STR(g.ic, app, ic);
    if (ic == NULL) {
        ic = initIcon();
        strncpy(ic->app, app, MAXAPPLEN);
        strncpy(ic->src_path, pe->fts_path, MAXICONPATHLEN-1);
        ic->src_w = ix;
        ic->src_h = iy;
        HASH_ADD_STR(g.ic, app, ic);
    } else {
        // we already have icon with dimensions: ic->src_w, h
        // new candidate: ix, iy
        // best value: g.option_iconW, H
        // should we replace the icon?
        if (iconMatchBetter(ix, iy, ic->src_w, ic->src_h)) {
            strncpy(ic->src_path, pe->fts_path, MAXICONPATHLEN-1);
            ic->src_w = ix;
            ic->src_h = iy;
        }
    }

    return 1;
}                               // inspectIconFile

//
// update drawable
//
int loadIconContent(icon_t * ic)
{

    if (!ic->drawable_allocated) {
        ic->drawable = XCreatePixmap(dpy, root, ic->src_w, ic->src_h, XDEPTH);
        if (ic->drawable == None) {
            msg(-1, "can't create pixmap for png icon\n");
            return 0;
        }
        ic->drawable_allocated = true;
    }

    if (pngReadToDrawable
        (ic->src_path, ic->drawable, g.color[COLBG].xcolor.red,
         g.color[COLBG].xcolor.green, g.color[COLBG].xcolor.blue) == 0) {
        msg(-1, "can't read png to drawable\n");
        return 0;
    }

    return 1;
}

//
// search app icon in hash,
// return icon or NULL if not found
//
icon_t *lookupIcon(char *app)
{
    icon_t *ic;
    char appl[MAXAPPLEN];
    int l;

    for (l = 0; (*(app + l)) != '\0' && l < MAXAPPLEN; l++)
        appl[l] = tolower(*(app + l));
    appl[l] = '\0';

    HASH_FIND_STR(g.ic, appl, ic);
    return ic;
}

//
// check if new width/height better match icon size option
// assuming square icons
//
bool iconMatchBetter(int new_w, int new_h, int old_w, int old_h)
{
    int hasdiff, newdiff;

    hasdiff = old_h - g.option_iconH;
    newdiff = new_h - g.option_iconH;
    return
        (hasdiff >= 0) ? ((newdiff < 0) ? false : ((newdiff <
                                                    hasdiff) ? true : false)
        ) : ((newdiff >= 0) ? true : ((newdiff > hasdiff) ? true : false)
        );
}

void deleteIconHash(icon_t **ihash)
{
    icon_t *iiter, *tmp;

    HASH_ITER(hh, *ihash, iiter, tmp) {
        HASH_DEL(*ihash, iiter);
        if (iiter != NULL)
            deleteIcon(iiter);
    }
}
