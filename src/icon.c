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

//#include "util.h"
#include "icon.h"

extern Display* dpy;

// PUBLIC:

//
// constructor: safe defaults
//
icon_t* initIcon()
{
    icon_t *ic;
    
    ic = (icon_t*)malloc(sizeof(icon_t));
    if (ic==NULL)
            return NULL;

    ic->src_file=NULL;
    ic->src_w = ic->src_h = 0;
    ic->drawable = ic->mask = 0;
    ic->drawable_allocated = false;

    return ic;
}

//
// destructor
//
void deleteIcon(icon_t* ic)
{
    if (ic->drawable_allocated) {
        XFreePixmap (dpy, ic->drawable);
    }
    free (ic);
}

//
// add icon to iconlist
// 1 if success
//
int addIconToList(iconlist_t* il, icon_t* ic)
{
	return 1;
}

