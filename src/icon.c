/*
Icon object.

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
    ic->drawable = ic->mask = None;
    ic->drawable_allocated = false;
    ic->ext = ICON_EXT_UNKNOWN;
    ic->dir = ICON_DIR_FREEDESKTOP;

    return ic;
}

//
// icon destructor
//
void deleteIcon(icon_t * ic)
{
    if (ic->drawable_allocated) {
        XFreePixmap(dpy, ic->drawable);
        /*
        if (ic->mask != None) {
            XFreePixmap(dpy, ic->mask);
        }
        */
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
// build array of icon directories
// return the number of elements or 0
//
int allocIconDirs(char ** icon_dirs)
{
    int hd;
    char *id2;
    int id2len;
    const char *icondir[] = {
        "/usr/share/icons",
        "/usr/local/share/icons",
        "~/.icons",
        "~/.local/share/icons",
        "/usr/share/pixmaps",
        "~/.local/share/pixmaps",
        NULL
    };
    int idndx = 0;
    int theme_len = strlen(g.option_theme);
    char *home = getenv("HOME");
    char *xdgdd = getenv("XDG_DATA_DIRS");
    bool legacy;
    char *str1;
    char *xdg;
    char *saveptr1;
    int j;
    char *k;
    int idsd;

    for (hd = 0; icondir[hd] != NULL; hd++) {
        legacy = (strstr (icondir[hd], "pixmap") != NULL);
        id2len = strlen(icondir[hd]) + 1;
        if (!legacy)
            id2len += theme_len + 1;
        if (icondir[hd][0] == '~') {
            if (home == NULL)
                continue;
            id2len += strlen(home);
        }
        id2 = malloc(id2len);
        if (!id2)
            return 0;
        if (icondir[hd][0] == '~')
            snprintf(id2, id2len, "%s%s", home, icondir[hd] + 1);
        else
            snprintf(id2, id2len, "%s", icondir[hd]);
        if (!legacy) {
            strcat(id2, "/");
            strncat(id2, g.option_theme, theme_len);
        }
        id2[id2len - 1] = '\0';
        icon_dirs[idndx] = id2;
        idndx++;
    }

    if (xdgdd != NULL) {
        for (j = 1, str1 = xdgdd; ; j++, str1 = NULL) {
            xdg = strtok_r(str1, ":", &saveptr1);
            if (xdg == NULL)
                break;
            msg(1, "xdg dir %d: %s\n", j, xdg);
            // strip trailing "/"'s
            for (k = xdg + strlen(xdg) - 1; *k == '/'; k--) {
                *k = '\0';
            }
            id2len = strlen(xdg) + strlen("/icons/") + theme_len + 1;
            id2 = malloc(id2len);
            if (!id2)
                return 0;
            snprintf(id2, id2len, "%s/icons/%s", xdg, g.option_theme);
            id2[id2len - 1] = '\0';
            // search for duplicates
            for (idsd = 0; idsd < idndx; idsd++) {
                if (strncmp (icon_dirs[idsd], id2, id2len) == 0) {
                    msg(1, "skip duplicate icon dir: %s\n", id2);
                    free(id2); id2 = NULL;
                    break;
                }
            }
            if (id2 != NULL) {
                icon_dirs[idndx] = id2;
                idndx++;
            }
        }
    }

    icon_dirs[idndx] = NULL;
    if (g.debug > 1) {
        for (idndx = 0; icon_dirs[idndx] != NULL; idndx++)
            msg(1, "icon dir: %s\n", icon_dirs[idndx]);
    }
    return idndx;
}

//
// free icon_dirs
//
void destroyIconDirs(char ** icon_dirs)
{
    int idndx;
    for (idndx = 0; icon_dirs[idndx] != NULL; idndx++)
        free(icon_dirs[idndx]);
}

//
// load all icons
// (no pixmaps, just path and dimension)
//
int updateIconsFromFile(icon_t ** ihash)
{
    char *icon_dirs[MAXICONDIRS];
    int d_c, f_c;
    FTS *ftsp;
    FTSENT *p, *chp;
    icon_t *iiter, *tmp;
    int fts_options = FTS_COMFOLLOW | FTS_LOGICAL | FTS_NOCHDIR;
    int ret = 0;

    if (allocIconDirs(icon_dirs) <= 0) {
        goto out;
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
            inspectIconMeta(p);
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
    destroyIconDirs(icon_dirs);
    return ret;
}   // updateIconsFromFile

//
// check if the file has better icon for given app than in g.ic
// return 1 if this icon is used, 0 otherwise
//
int inspectIconMeta(FTSENT * pe)
{
    char *point;
    char *fname;
    char app[MAXAPPLEN];
    int applen;
    char *xchar;
    char *uchar;
    char *endptr;
    char *dim;
    int dimlen;
    char sx[MAXICONDIMLEN];
    char sy[MAXICONDIMLEN];
    int sx_size, sy_size;
    int ix, iy;
    icon_t *ic;
    char *suff;
    int sfxn;
    int tl;
    char *lds;
    char *digit;
    char *minus;
    char *generic_suffixes[] = { "-color", NULL };
    char *legacy_dim_suffixes[] = { "16", "24", "32", "48", "64", NULL };
    int dir = ICON_DIR_FREEDESKTOP;
    int ext = ICON_EXT_UNKNOWN;
    const char *special_fail_1 = "failed to interpret %s as app_WWxHH at %s\n";

    fname = pe->fts_name;
    point = strrchr(fname, '.');
    if (point == NULL)
        return 0;               // no extension?
    if (strcmp(point + 1, "png") == 0) {
        ext = ICON_EXT_PNG;
    } else if (strcmp(point + 1, "xpm") == 0) {
        ext = ICON_EXT_XPM;
    }
    if (ext == ICON_EXT_UNKNOWN)
        return 0;

    if (strstr(pe->fts_parent->fts_name, "pixmap") != NULL)
        dir = ICON_DIR_LEGACY;

    // skip non-apps freedesktop icons
    if (dir == ICON_DIR_FREEDESKTOP && strcmp(pe->fts_parent->fts_name, "apps") != 0)
        return 0;

    // guess app
    applen = point - fname > (MAXAPPLEN - 1) ? (MAXAPPLEN - 1) : point - fname;
    strncpy(app, fname, applen);
    app[applen] = '\0';
    for (tl = 0; app[tl] != '\0'; tl++)
        app[tl] = tolower(app[tl]);

    // guess dimensions by directory
    if (dir == ICON_DIR_FREEDESKTOP) {
        dim = pe->fts_parent->fts_parent->fts_name;
        dimlen = pe->fts_parent->fts_parent->fts_namelen;
        xchar = strchr(dim, 'x');
        if (xchar == NULL)
            return 0;               // unknown dimensions
        sx_size = xchar - dim;
        if (sx_size > MAXICONDIMLEN - 1)
            return 0;
        strncpy(sx, dim, sx_size);
        sx[sx_size] = '\0';
        ix = atoi(sx);
        sy_size = dim + dimlen - xchar;
        if (sy_size > MAXICONDIMLEN - 1)
            return 0;
        strncpy(sy, xchar + 1, sy_size);
        sy[sy_size] = '\0';
        iy = atoi(sy);
    } else {
        // icon other than a priory known dimensions has lowest priority
        ix = iy = 1;
    }

    // drop known suffix
    sfxn = 0;
    while (generic_suffixes[sfxn] != NULL) {
        suff = strcasestr(app, generic_suffixes[sfxn]);
        if (suff != NULL) {
            *suff = '\0';
        }
        sfxn++;
    }

    // app_48x48 legacy xpm
    if (dir == ICON_DIR_LEGACY) {
        uchar = strrchr(app, '_');
        xchar = strrchr(app, 'x');
        if (xchar != NULL && uchar != NULL && xchar > uchar) {
            sx_size = xchar - uchar - 1;
            if (sx_size > MAXICONDIMLEN - 1) {
                msg (0, special_fail_1, app, "WW");
                ix = 0;
                goto end_special_1;
            }
            strncpy(sx, uchar+1, sx_size);
            sx[sx_size] = '\0';
            ix = strtol(sx, &endptr, 10);
            if (!(*sx != '\0' && *endptr == '\0')) {
                msg (0, special_fail_1, app, "WW");
                ix = 0;
                goto end_special_1;
            }
            sy_size = app + strlen(app) - xchar;
            if (sy_size > MAXICONDIMLEN - 1) {
                msg (0, special_fail_1, app, "HH");
                iy = 0;
                goto end_special_1;
            }
            strncpy(sy, xchar + 1, sy_size);
            sy[sy_size] = '\0';
            iy = strtol(sy, &endptr, 10);
            if (!(*sy != '\0' && *endptr == '\0')) {
                msg (0, special_fail_1, app, "HH");
                iy = 0;
                goto end_special_1;
            }
            *uchar = '\0';
        }
    }
end_special_1:

    // app48 and app-48 legacy xpm
    if (dir == ICON_DIR_LEGACY) {
        digit = app + strlen(app) - 2;
        minus = app + strlen(app) - 3; // trick for gcc-10 pseudo-intelligence
        if (digit > app + 1) {
            sfxn = 0;
            while ((lds = legacy_dim_suffixes[sfxn]) != 0) {
                if (strncasecmp(digit, lds, 3) == 0) {
                    ix = iy = strtol(lds, &endptr, 10);
                    if (*(minus) == '-') {
                        *(minus) = '\0';
                    } else {
                        *digit = '\0';
                    }
                    break;
                }
                sfxn++;
            }
        }
    }

    // is app already in hash?
    HASH_FIND_STR(g.ic, app, ic);
    if (ic == NULL) {
        ic = initIcon();
        if (ic == NULL)
            return 0;
        strncpy(ic->app, app, MAXAPPLEN);
        strncpy(ic->src_path, pe->fts_path, MAXICONPATHLEN-1);
        ic->src_w = ix;
        ic->src_h = iy;
        ic->ext = ext;
        ic->dir = dir;
        HASH_ADD_STR(g.ic, app, ic);
    } else {
        // we already have icon with dimensions: ic->src_w, h
        // new candidate: ix, iy
        // best value: g.option_iconW, H
        // should we replace the icon?
        if (iconMatchBetter(ix, iy, ic->src_w, ic->src_h, false)) {
            strncpy(ic->src_path, pe->fts_path, MAXICONPATHLEN-1);
            ic->src_w = ix;
            ic->src_h = iy;
            ic->ext = ext;
            ic->dir = dir;
        }
    }

    return 1;
} // inspectIconMeta

//
// update Drawable from png file
//
int loadIconContentPNG(icon_t * ic)
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
        msg(-1, "can't read png to drawable: %s\n", ic->src_path);
        return 0;
    }

    return 1;
}

//
// update Drawable from xpm file
//
int loadIconContentXPM(icon_t * ic)
{
    int ret;

    ret = (XpmReadFileToPixmap(dpy, root, ic->src_path, &(ic->drawable),
                &(ic->mask), NULL) == XpmSuccess) ? 1 : 0;
    if (ret == 1) {
        ic->drawable_allocated = true;
    } else {
        msg(-1, "can't read xpm to drawable: %s\n", ic->src_path);
    }

    return ret;
}

//
// update drawable
//
int loadIconContent(icon_t * ic)
{
    int ret;

    if (ic->ext == ICON_EXT_PNG) {
        ret = loadIconContentPNG(ic);
    } else if (ic->ext == ICON_EXT_XPM) {
        ret = loadIconContentXPM(ic);
    } else {
        msg(1, "unknown icon extension: %d\n", ic->ext);
        ret = 0;
    }
    return ret;
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
// if equal_prefer_new=true and sizes are equal, then prefer new icon
//
bool iconMatchBetter(int new_w, int new_h, int old_w, int old_h, bool equal_prefer_new)
{
    int hasdiff, newdiff;

    hasdiff = old_h - g.option_iconH;
    newdiff = new_h - g.option_iconH;
    if (hasdiff == newdiff && equal_prefer_new)
        return true;
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
