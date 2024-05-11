/*
pngd.c definitions.

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

#ifndef PNGD_H
#define PNGD_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <X11/Xlib.h>
#include <png.h>
#include "util.h"

// PUBLIC

typedef struct {
    png_structp png_ptr;
    png_infop info_ptr;
    png_uint_32 width, height;
    int bit_depth, color_type, channels;
    uint32_t rowbytes;
    uint8_t *data;
} TImage;

int pngInit(FILE * infile, TImage * img);
uint8_t *pngLoadData(TImage * img);
int convert_msb(uint32_t in);
int pngDraw(TImage * img, Drawable d, XImage * ximage, Visual * visual,
            uint8_t bg_red, uint8_t bg_green, uint8_t bg_blue);
int pngReadToDrawable(char *pngpath, Drawable d, uint8_t bg_red,
                      uint8_t bg_green, uint8_t bg_blue);
int pngReadToDrawable_test();

#endif
