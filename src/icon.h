/*
icon.c definitions.

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

#ifndef ICON_H
#define ICON_H

#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <X11/Xlib.h>
//#include <X11/Xutil.h>
//#include <X11/Xft/Xft.h>
//#include <X11/extensions/Xrender.h>

//#define MAXPROPLEN  4096

//XErrorEvent *ee_ignored;
//bool ee_complain;

typedef struct {
    char *src_file; // NULL if not initialized or loaded from X window properties
    unsigned int src_w, src_h; // width/height of source (not resized) icon
    Pixmap drawable; // resized (ready to use)
    Pixmap mask;
    bool drawable_allocated; // we must free drawable (but not mask), because we created it
} icon_t;

typedef struct {
    char *app; // uthash key
    icon_t *icon;
} iconlist_t;


icon_t* initIcon();
void deleteIcon(icon_t* ic);

int addIconToList(iconlist_t* il, icon_t* ic);

#endif
