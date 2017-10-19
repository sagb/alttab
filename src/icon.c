/*
Icon object.

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

#define _GNU_SOURCE
#include <string.h>
//#include "util.h"
#include "alttab.h"
#include "pngd.h"
#include "icon.h"
extern Globals g;
extern Display* dpy;
extern int scr;
extern Window root;

// PUBLIC:

//
// icon constructor: safe defaults
//
icon_t* initIcon()
{
    icon_t *ic;
    
    ic = (icon_t*)malloc(sizeof(icon_t));
    if (ic==NULL)
            return NULL;

    ic->app[0]='\0';
    ic->src_path[0]='\0';
    ic->src_w = ic->src_h = 0;
    ic->drawable = ic->mask = 0;
    ic->drawable_allocated = false;

    return ic;
}

//
// icon destructor
//
void deleteIcon(icon_t* ic)
{
    if (ic->drawable_allocated) {
        XFreePixmap (dpy, ic->drawable);
    }
    free (ic);
}

//
// hash constructor
// 1=success
//
int initIconHash(icon_t** ihash)
{
    *ihash=NULL; // required by uthash
    updateIconsFromFile(ihash);
    return 1;
}

//
// load all icons
// (no pixmaps, just path and dimension)
//
int updateIconsFromFile(icon_t** ihash)
{
    const char* sysbase = "/usr/share/icons";
    char* home; const char* userbase = "/.icons";
    const char* theme = "/hicolor";
    char* icon_dirs[3];
    int ib, d_c, f_c;
    FTS *ftsp;
    FTSENT *p, *chp;
    int fts_options = FTS_COMFOLLOW | FTS_LOGICAL | FTS_NOCHDIR;
    //int rval = 0;
    icon_t *iiter, *tmp;

    icon_dirs[0] = (char*)malloc (strlen(sysbase) + strlen(theme) + 1);
    strcpy (icon_dirs[0], sysbase);
    strcat (icon_dirs[0], theme);
    home = getenv("HOME");
    icon_dirs[1] = (char*)malloc (strlen(home) + strlen(userbase) + strlen(theme) + 1);
    strcpy (icon_dirs[1], home);
    strcat (icon_dirs[1], userbase);
    strcat (icon_dirs[1], theme);
    icon_dirs[2] = NULL;
    if (g.debug>1) {
        for (ib=0; icon_dirs[ib]; ib++) {
            fprintf (stderr, "icon dir: %s\n", icon_dirs[ib]);
        }
    }

    if ((ftsp = fts_open(icon_dirs, fts_options, NULL)) == NULL) {
        warn("fts_open");
        return 0;
    }
    /* Initialize ftsp with as many icon_dirs as possible. */
    chp = fts_children(ftsp, 0);
    if (chp == NULL) {
        return 0;               /* no files to traverse */
    }
    d_c=f_c=0;
    while ((p = fts_read(ftsp)) != NULL) {
        switch (p->fts_info) {
            case FTS_D:
                //printf("d %s\n", p->fts_path);
                d_c++;
                break;
            case FTS_F:
                //fprintf(stderr, "f %s\n", p->fts_path);
                inspectIconFile (p);
                f_c++;
                break;
            default:
                break;
        }
    }
    fts_close(ftsp);
    if (g.debug>0) {
        fprintf (stderr, "icon dirs: %d, files: %d, apps: %d\n", d_c, f_c, HASH_COUNT(*ihash));
        if (g.debug>1) {
            HASH_ITER (hh, *ihash, iiter, tmp) {
                fprintf (stderr, "app \"%s\" [%s] (%dx%d)\n", iiter->app, iiter->src_path, iiter->src_w, iiter->src_h);
            }
        }
        // TODO: seems less than find -type f ?
    }
    return 1;
} // updateIconsFromFile

//
// check if the file has better icon for given app than we have in g.ic
// return 1 if this icon is used, 0 otherwise
//
int inspectIconFile (FTSENT* pe)
{
    char* point; char* fname; char app[MAXAPPLEN]; int applen;
    char* xchar; char* dim; int dimlen; char sx[5]; char sy[5]; int ix, iy;
    icon_t *ic;
    int hasdiff, newdiff, replace;
    char* generic_suffixes[] = { "-color", "-esr", "-im6", NULL };
    char* suff; int sfxn;
    int tl;

    // skip non-apps icons
    if (strcmp (pe->fts_parent->fts_name, "apps") != 0)
        return 0;

    // guess app
    fname = pe->fts_name;
    point = rindex (fname, '.');
    if (point==NULL)
        return 0; // no extension?
    if (strcmp (point+1, "png") != 0)
        return 0; // going to load png only TODO: mention in doc
    applen = point-fname > (MAXAPPLEN-1) ? (MAXAPPLEN-1) : point-fname;
    strncpy (app, fname, applen);
    app[applen]='\0';
    for (tl=0; app[tl]!='\0'; tl++) 
        app[tl] = tolower (app[tl]);

    // sort of generalization
    point = index (app, '.');
    if (point!=NULL)
        *point = '\0';
    sfxn=0; while (generic_suffixes[sfxn] != NULL) {
        suff = strcasestr (app, generic_suffixes[sfxn]);
        if (suff!=NULL) {
            *suff = '\0';
        }
        sfxn++;
    }

    // guess dimensions
    dim = pe->fts_parent->fts_parent->fts_name;
    dimlen = pe->fts_parent->fts_parent->fts_namelen;
    xchar = index (dim, 'x');
    if (xchar==NULL)
        return 0; // unknown dimensions
    strncpy (sx, dim, (xchar-dim));
    sx[xchar-dim]='\0';
    ix = atoi(sx);
    strncpy (sy, xchar+1, dim+dimlen-xchar);
    sy[dim+dimlen-xchar-1]='\0';
    iy = atoi(sy);

    // is app already in hash?
    HASH_FIND_STR (g.ic, app, ic);
    if (ic==NULL) {
        ic = initIcon();
        strncpy (ic->app, app, MAXAPPLEN);
        strncpy (ic->src_path, pe->fts_path, MAXICONPATHLEN);
        ic->src_w = ix;
        ic->src_h = iy;
        HASH_ADD_STR (g.ic, app, ic);
    } else {
        // we already have icon with dimensions: ic->src_w, h
        // new candidate: ix, iy
        // best value: g.option_iconW, H
        // should we replace the icon?
        // assuming square icons
        hasdiff = ic->src_h - g.option_iconH;
        newdiff = iy - g.option_iconH;
        replace = 
            (hasdiff >= 0) ? (
                (newdiff < 0) ? 0 : (
                    (newdiff < hasdiff) ? 1 : 0
                )
            ) : (
                (newdiff >= 0) ? 1 : (
                    (newdiff > hasdiff) ? 1 : 0
                )
            );
        if (replace==1) {
            strncpy (ic->src_path, pe->fts_path, MAXICONPATHLEN);
            ic->src_w = ix;
            ic->src_h = iy;
        }
     }

    return 1;
} // inspectIconFile 

//
// update drawable
//
int loadIconContent(icon_t* ic) {

    if (!ic->drawable_allocated) {
        ic->drawable = XCreatePixmap(dpy, root, ic->src_w, ic->src_h, XDEPTH);
        if (ic->drawable==None) {
            fprintf (stderr, "can't create pixmap for png icon\n");
            return 0;
        }
        ic->drawable_allocated=true;
    }

    if (pngReadToDrawable(ic->src_path, ic->drawable)==0) {
        fprintf (stderr, "can't read png to drawable\n");
        return 0;
    }

    return 1;
} // loadIconContent


//
// search app icon in hash,
// load pixmap if necessary,
// return ready to use icon or NULL if not found
//
icon_t* lookupIcon(char* app)
{
    icon_t* ic;
    char appl[MAXAPPLEN];
    int l;

    for (l=0; (*(app+l))!='\0' && l<MAXAPPLEN; l++)
        appl[l] = tolower (*(app+l));
    appl[l] = '\0';

    HASH_FIND_STR (g.ic, appl, ic);
    if (ic != NULL) {
        // app is in hash
        if (ic->drawable == None) {
            if (g.debug>1)
                fprintf (stderr, "lookupIcon: loading content for %s\n", ic->app);
            if (loadIconContent (ic) == 0) {
                fprintf (stderr, "can't load png icon content\n");
                return NULL;
            }
        }
    }
    return ic;
} // lookupIcon


