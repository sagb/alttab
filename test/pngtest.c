/*
 * Stub for possible future test facility.
 *
 * Copyright 2017 Alexander Kulak.
 * This file is part of alttab program.
 *
 * alttab is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * alttab is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with alttab.  If not, see <http://www.gnu.org/licenses/>.
 * */


#include "../src/pngd.h"

Display* dpy;
int scr;
Window root;

int main(int argc, char **argv)
{
    return !pngReadToDrawable_test ("/usr/share/icons/hicolor/48x48/apps/mkvmerge.png");
}

