/*
Interface to XRANDR

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

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include "alttab.h"
#include "util.h"
extern Globals g;
extern Display *dpy;
extern int scr;
extern Window root;

// Documentation:
// https://cgit.freedesktop.org/xorg/proto/randrproto/tree/randrproto.txt

// version requirements:
// RRGetScreenResourcesCurrent (1.3)
#define MAJ_REQ 1
#define MIN_REQ 3

// PRIVATE

// current output geometry list
// we alloc/free them only here

//
// update outs, return nout or 0
//
static int randr_update_outputs(Window w, quad ** outs)
{
    XRRScreenResources *scr_res;
    XRROutputInfo *out_info;
    XRRCrtcInfo *crtc_info;
    int nout;
    int out;                    // as returned by randr; may include inactive

    scr_res = XRRGetScreenResourcesCurrent(dpy, w);
    if (scr_res == NULL)
        return 0;
    nout = 0;
    (*outs) = NULL;
    for (out = 0; out < scr_res->noutput; out++) {
        out_info = XRRGetOutputInfo(dpy, scr_res, scr_res->outputs[out]);
        if (out_info == NULL
            || out_info->connection != RR_Connected || out_info->crtc == 0) {
            XRRFreeOutputInfo(out_info); // does it neen NULL check?
            continue;
        }
        crtc_info = XRRGetCrtcInfo(dpy, scr_res, out_info->crtc);
        if (crtc_info == NULL)
            continue;
        (*outs) = realloc((*outs), (nout + 1) * sizeof(quad));
        if ((*outs) == NULL)
            return 0; // would be nice to free Infos
        (*outs)[nout].x = crtc_info->x;
        (*outs)[nout].y = crtc_info->y;
        (*outs)[nout].w = crtc_info->width;
        (*outs)[nout].h = crtc_info->height;
        msg(0,
            "output %d (%d by randr) crtc: +%d+%d %dx%d noutput %d npossible %d\n",
            nout, out,
            crtc_info->x, crtc_info->y, crtc_info->width, crtc_info->height,
            crtc_info->noutput, crtc_info->npossible);
        XRRFreeCrtcInfo(crtc_info);
        XRRFreeOutputInfo(out_info);
        nout++;
    }
    XRRFreeScreenResources(scr_res);
    return nout;
}

//
// return currently focused window and its geometry,
// or focus point (depending on vp_mode).
// res and fw must be allocated by caller.
//
static bool x_get_activity_area(quad * res, Window * fw)
{
#define VPM  g.option_vp_mode
    int rtr;
    Window root_ret, child_ret;
    int root_x, root_y, win_x, win_y;
    unsigned int mask_ret;

    *fw = 0;

    if (VPM == VP_FOCUS) {
        if (XGetInputFocus(dpy, fw, &rtr) == 0 || (*fw) == 0) {
            msg(-1, "can't recognize focused window\n");
            return false;
        }
        if (!get_absolute_coordinates(*fw, res)) {
            msg(-1, "can't get window 0x%lx absolute coordinates\n", (*fw));
            return false;
        }
        msg(0, "focus in window 0x%lx (%dx%d +%d+%d)\n",
            (*fw), res->w, res->h, res->x, res->y);
        return true;
    }

    if (VPM == VP_POINTER) {
        if (!XQueryPointer(dpy, root, &root_ret, &child_ret,
                           &root_x, &root_y, &win_x, &win_y, &mask_ret)) {
            // pointer is not on the same screen as root
            return false;
        }
        res->x = win_x;
        res->y = win_y;
        res->w = res->h = 1;
        *fw = root;
        msg(0, "pointer: +%d+%d\n", res->x, res->y);
        return true;
    }
    // unknown vp_mode
    return false;
}

// PUBLIC

//
// check for randr at runtime
// assuming (for now) it's available at build time
//
bool randrAvailable()
{
    int maj, min;
    bool ok;
    Status s = XRRQueryVersion(dpy, &maj, &min);
    if (s != 0) {
        ok = ((maj == MAJ_REQ && min >= MIN_REQ) || (maj > MAJ_REQ)
            );
    } else {
        ok = false;
    }
    if (g.debug > 0) {
        if (s == 0)
            msg(0, "randr not available\n");
        else
            msg(0, "randr v. %d.%d available (%ssufficient)\n",
                maj, min, ok ? "" : "not ");
    }
    return ok;
}

//
// return best viewport from randr 'point of view'
// of false if not found.
// res must be allocated by caller.
//
bool randrGetViewport(quad * res, bool * multihead)
{
    Window fw = 0;
    quad aq;                    // 'activity area': focused window geometry or pointer point
    quad *oq = NULL;            // outputs geometries
    int o, no;
    int x1, x2, y1, y2, area;
    quad lq;                    // largest cross-section at 1st stage
    int largest_cross_area = 0; // its square
    int best_1_stage_output = -1;
    int best_2_stage_output = -1;
    int smallest_2_stage_area;

    //no = randr_update_outputs(fw != 0 ? fw : root, &oq); // no fw here
    no = randr_update_outputs(root, &oq);
    if (no < 1) {
        msg(0, "randr didn't detect any output\n");
        *multihead = false;
        free(oq);
        return false;
    }
    if (no == 1) {
        msg(0, "using single randr output as viewport\n");
        *res = oq[0];
        *multihead = false;
        free(oq);
        return true;
    }

    *multihead = true;

    if (!x_get_activity_area(&aq, &fw)) {
        msg(0, "failed to detect activity area, using first randr output\n");
        *res = oq[0];
        free(oq);
        return true;
    }

    for (o = 0; o < no; o++) {
        if (rectangles_cross(aq, oq[o])) {
            // this output has a common cross-section with activity area,
            // now find this cross-section.
            x1 = aq.x > oq[o].x ? aq.x : oq[o].x;
            x2 = (aq.x + aq.w) <
                (oq[o].x + oq[o].w) ? (aq.x + aq.w) : (oq[o].x + oq[o].w);
            y1 = aq.y > oq[o].y ? aq.y : oq[o].y;
            y2 = (aq.y + aq.h) <
                (oq[o].y + oq[o].h) ? (aq.y + aq.h) : (oq[o].y + oq[o].h);
            area = (x2 - x1) * (y2 - y1);
            msg(0, "output %d cross-section: %d\n", o, area);
            if (area > largest_cross_area) {
                best_1_stage_output = o;
                lq.x = x1;
                lq.y = y1;
                lq.w = x2 - x1;
                lq.h = y2 - y1;
                largest_cross_area = area;
            }
            /*
               // disabled feature:
               // total area of CRTCs which have cross-section with focused window
               if (oq[o].x < res->x) res->x = oq[o].x;
               int right_diff = (oq[o].x + oq[o].w) - (res->x + res->w);
               if (right_diff > 0) res->w += right_diff;
               if (oq[o].y < res->y) res->y = oq[o].y;
               int bot_diff = (oq[o].y + oq[o].h) - (res->y + res->h);
               if (bot_diff > 0) res->h += bot_diff;
             */
        } else {
            msg(0, "output %d doesn't cross with activity area\n", o);
        }
    }                           // outputs

    if (best_1_stage_output == -1) {
        msg(0,
            "failed to find largest cross-section, using first randr output\n");
        *res = oq[0];
        free(oq);
        return true;
    }
    // if best cross-area is shared with some other monitor,
    // then smallest of these monitors is chosen.
    smallest_2_stage_area =
        oq[best_1_stage_output].w * oq[best_1_stage_output].h;
    for (o = 0; o < no; o++) {
        if (o == best_1_stage_output)
            continue;
        if (rectangles_cross(lq, oq[o])) {
            area = oq[o].w * oq[o].h;
            if (area < smallest_2_stage_area) {
                best_2_stage_output = o;
                smallest_2_stage_area = area;
            }
        }
    }

    if (best_2_stage_output == -1) {
        best_2_stage_output = best_1_stage_output;
    } else {
        msg(0,
            "found better (smallest output) candidate: %d\n",
            best_2_stage_output);
    }

    // the single result here is  best_2_stage_output

    *res = oq[best_2_stage_output];
    msg(0,
        "best viewport from randr: %dx%d +%d+%d\n",
        res->w, res->h, res->x, res->y);
    free(oq);
    return true;
}
