/*
Reading PNG into Drawable.

Copyright 2017-2025 Alexander Kulak.
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

#include <unistd.h>
#include <X11/Xutil.h>
#include "pngd.h"

extern Display *dpy;
extern int scr;
extern Window root;

//
// read png header
//
int pngInit(FILE * infile, TImage * img)
{
    png_structp png_ptr;
    png_infop info_ptr;
    png_uint_32 width, height;
    uint8_t sig[8];

    if (fread(sig, 1, 8, infile) == 0)
        return 0;
    if (!png_check_sig(sig, 8))
        return 0;
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr)
        return 0;
    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        return 0;
    }
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return 0;
    }
    png_init_io(png_ptr, infile);
    png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &(img->bit_depth),
                 &(img->color_type), NULL, NULL, NULL);
    img->width = width;
    img->height = height;
    img->png_ptr = png_ptr;
    img->info_ptr = info_ptr;

    return 1;
}

static void pngFree(TImage *img)
{
    png_destroy_read_struct(&img->png_ptr, &img->info_ptr, NULL);
    free(img->data);
}

//
// read png data
//
uint8_t *pngLoadData(TImage * img)
{
    double gamma;
    png_uint_32 i, rowbytes;

    png_bytepp row_ptrs = NULL;
    uint8_t *data = NULL;       // local
    static double exponent = 2.2;

    if (setjmp(png_jmpbuf(img->png_ptr))) {
        png_destroy_read_struct(&(img->png_ptr), &(img->info_ptr), NULL);
        return NULL;
    }
    if (img->color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_expand(img->png_ptr);
    if (img->color_type == PNG_COLOR_TYPE_GRAY && img->bit_depth < 8)
        png_set_expand(img->png_ptr);
    if (png_get_valid(img->png_ptr, img->info_ptr, PNG_INFO_tRNS))
        png_set_expand(img->png_ptr);
    if (img->bit_depth == 16)
        png_set_strip_16(img->png_ptr);
    if (img->color_type == PNG_COLOR_TYPE_GRAY ||
        img->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(img->png_ptr);
    if (png_get_gAMA(img->png_ptr, img->info_ptr, &gamma))
        png_set_gamma(img->png_ptr, exponent, gamma);

    png_read_update_info(img->png_ptr, img->info_ptr);
    img->rowbytes = rowbytes = png_get_rowbytes(img->png_ptr, img->info_ptr);
    img->channels = (int)png_get_channels(img->png_ptr, img->info_ptr);
    if ((data = (uint8_t *) malloc(rowbytes * (img->height))) == NULL) {
        png_destroy_read_struct(&img->png_ptr, &img->info_ptr, NULL);
        return NULL;
    }
    if ((row_ptrs =
         (png_bytepp) malloc((img->height) * sizeof(png_bytep))) == NULL) {
        png_destroy_read_struct(&(img->png_ptr), &(img->info_ptr), NULL);
        free(data);
        data = NULL;
        return NULL;
    }
    for (i = 0; i < img->height; ++i)
        row_ptrs[i] = data + i * rowbytes;

    png_read_image(img->png_ptr, row_ptrs);
    free(row_ptrs);
    row_ptrs = NULL;
    png_read_end(img->png_ptr, NULL);

    return data;
}

//
// combines img onto d
// using: intermediate ximage, visual, background
//
int pngDraw(TImage * img, Drawable d, XImage * ximage, Visual * visual,
            uint8_t bg_red, uint8_t bg_green, uint8_t bg_blue)
{
    uint8_t *src;
    char *dest;
    uint8_t r, g, b, a;
    uint32_t pixel;
    int RShift, GShift, BShift;
    uint32_t RMask, GMask, BMask;
    uint32_t red, green, blue;

    int xrowbytes = ximage->bytes_per_line;
    uint32_t i, row, lastrow = 0;
    GC gc = DefaultGC(dpy, scr);

    // TODO: replace this by initCompositeConst/pixelComposite from util
    RMask = visual->red_mask;
    GMask = visual->green_mask;
    BMask = visual->blue_mask;
    RShift = convert_msb(RMask) - 7;
    GShift = convert_msb(GMask) - 7;
    BShift = convert_msb(BMask) - 7;

    for (lastrow = row = 0; row < img->height; ++row) {
        src = img->data + row * img->rowbytes;
        dest = ximage->data + row * xrowbytes;
        if (img->channels == 3) {
            for (i = img->width; i > 0; --i) {
                red = *src++;
                green = *src++;
                blue = *src++;
                pixel = (red << RShift) | (green << GShift) | (blue << BShift);
                *dest++ = (char)((pixel >> 24) & 0xff);
                *dest++ = (char)((pixel >> 16) & 0xff);
                *dest++ = (char)((pixel >> 8) & 0xff);
                *dest++ = (char)(pixel & 0xff);
            }
        } else {                /* if (channels == 4) */

            for (i = img->width; i > 0; --i) {
                r = *src++;
                g = *src++;
                b = *src++;
                a = *src++;
                if (a == 255) {
                    red = r;
                    green = g;
                    blue = b;
                } else if (a == 0) {
                    red = bg_red;
                    green = bg_green;
                    blue = bg_blue;
                } else {
                    alpha_composite(red, r, a, bg_red);
                    alpha_composite(green, g, a, bg_green);
                    alpha_composite(blue, b, a, bg_blue);
                }
                //fprintf (stderr, "r=%d g=%d b=%d a=%d red=%d green=%d blue=%d RShift=%d GShift=%d BShift=%d\n", r, g, b, a, red, green, blue, RShift, GShift, BShift);
                pixel = (red << RShift) | (green << GShift) | (blue << BShift);
                *dest++ = (char)((pixel >> 24) & 0xff);
                *dest++ = (char)((pixel >> 16) & 0xff);
                *dest++ = (char)((pixel >> 8) & 0xff);
                *dest++ = (char)(pixel & 0xff);
            }
        }
        if (((row + 1) & 0xf) == 0) {
            XPutImage(dpy, d, gc, ximage, 0, (int)lastrow, 0,
                      (int)lastrow, img->width, 16);
            XFlush(dpy);
            lastrow = row + 1;
        }
    }

    if (lastrow < img->height) {
        XPutImage(dpy, d, gc, ximage, 0, (int)lastrow, 0,
                  (int)lastrow, img->width, img->height - lastrow);
    }

    return 1;
}

//
// draw file on d
//
int pngReadToDrawable(char *pngpath, Drawable d, uint8_t bg_red,
                      uint8_t bg_green, uint8_t bg_blue)
{
    int debug = 0;

    FILE *infile;
    Visual *visual;
    TImage img;
    uint8_t *xdata;
    int pad;
    XImage *ximage;
    int depth;
    int ret;

    img.data = NULL;
    img.png_ptr = NULL;
    img.info_ptr = NULL;
    visual = DefaultVisual(dpy, scr);
    depth = DisplayPlanes(dpy, scr);

    if (!(depth == 24 || depth == 32)) {
        fprintf(stderr, "X11 depth must be 24 or 32, we have %d\n", depth);
        return 0;
    }
    if (!(infile = fopen(pngpath, "rb"))) {
        fprintf(stderr, "can't open [%s]\n", pngpath);
        return 0;
    }
    if ((pngInit(infile, &img)) != 1) {
        fprintf(stderr, "error reading png header\n");
        return 0;
    }
    img.data = pngLoadData(&img);
    fclose(infile);
    if (!img.data || img.width == 0 || img.height == 0) {
        fprintf(stderr, "error loading png data\n");
        pngFree(&img);
        return 0;
    }
    if (debug > 0)
        fprintf(stderr, "read %dx%d png, %d channels\n", img.width,
                img.height, img.channels);
    xdata = (uint8_t *) malloc(4 * img.width * img.height);
    pad = 32;
    if (!xdata) {
        fprintf(stderr, "xdata malloc error\n");
        pngFree(&img);
        return 0;
    }
    ximage =
        XCreateImage(dpy, visual, depth, ZPixmap, 0, (char *)xdata,
                     img.width, img.height, pad, 0);
    if (!ximage) {
        fprintf(stderr, "error creating ximage\n");
        free(xdata);
        pngFree(&img);
        return 0;
    }
    ximage->byte_order = MSBFirst;

    ret = pngDraw(&img, d, ximage, visual, bg_red, bg_green, bg_blue);
    pngFree(&img);
    XDestroyImage(ximage);
    return ret;
}

//
// standalone test for pngReadToDrawable (see test/)
//
int pngReadToDrawable_test(char *pngfile)
{
    Window p;
    XEvent e;

    // define these as globals in caller
    dpy = XOpenDisplay(NULL);
    scr = DefaultScreen(dpy);
    root = RootWindow(dpy, scr);

    p = XCreateSimpleWindow(dpy, root, 0, 0, 600, 600, 0,
                            WhitePixel(dpy, scr), WhitePixel(dpy, scr));
    if (p == None) {
        fprintf(stderr, "error creating main window\n");
        return 0;
    }
    XSelectInput(dpy, p, ExposureMask | KeyPressMask | KeyReleaseMask);
    XMapWindow(dpy, p);
    do
        XNextEvent(dpy, &e);
    while (e.type != Expose || e.xexpose.count);
    XFlush(dpy);

    if (pngReadToDrawable(pngfile, p, 255, 255, 255) != 1) {
        fprintf(stderr, "can't read png to drawadle\n");
        return 0;
    }

    sleep(2);
    return 1;
}
