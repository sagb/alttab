#!/bin/sh

# Tests in headless X server.

# Copyright 2017-2024 Alexander Kulak.
# This file is part of alttab program.
#
# alttab is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# alttab is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with alttab.  If not, see <http://www.gnu.org/licenses/>.


ALTTAB=../src/alttab

install_software()
{
    packs="" ; progs=""
    for pair in "xdotool xdotool" "Xvfb xvfb" "xeyes x11-apps" "xprop x11-utils" "ps procps" ; do
        prog=`echo $pair | cut -d \  -f 1`
        pack=`echo $pair | cut -d \  -f 2`
        if ! which "$prog" >/dev/null ; then
            packs="${packs} ${pack}"
            progs="${progs} ${prog}"
        fi
    done
    if [ -n "$packs" ] ; then
        echo "Installing: $packs"
        icmd="sudo --non-interactive apt-get -y install $packs"
        $icmd || echo -e "Failed: $icmd\nThis script runs apt-get via passwordless sudo, otherwise install manually: $progs" >&2
        for prog in $progs ; do
            if ! which "$prog" >/dev/null ; then
                echo "Can't find $prog. Bail out!" >&2
                exit 1
            fi
        done
    fi
}

start_x()
{
    # not required, only to silent Xvfb
    sudo --non-interactive \
        sh -c "mkdir -p /tmp/.X11-unix ; chmod 1777 /tmp/.X11-unix" >/dev/null
    Xvfb :200 -ac &
    export xvfb=$!
    for ms100 in `seq 1 50` ; do
        sleep 0.1
        if DISPLAY=:200 xprop -root >/dev/null ; then
            return
        fi
    done
    echo "Can't recognize Xvfb in 5 seconds. Bail out!" >&2
    exit 1
}

stop_x()
{
    if [ -n "$xvfb" ] ; then
        kill -9 $xvfb
        rm -f /tmp/.X200-lock
        export xvfb=""
    fi
}

start_alttab()
{
    DISPLAY=:200 "$ALTTAB" -w 0 -vv &
    export alttab=$!
    sleep 1
}

stop_alttab()
{
    if [ -n "$alttab" ] ; then
        kill -9 $alttab
        export alttab=""
    fi
}

open_sample_windows()
{
    eyes=""
    for c in `seq 1 7` ; do
        DISPLAY=:200 xeyes &
        eyes="${eyes} $!"
    done
    export eyes
    sleep 0.5
}

close_sample_windows()
{
    if [ -n "$eyes" ] ; then
        kill -9 $eyes
        export eyes=""
    fi
}

alttab_simple_cycle()
{
    DISPLAY=:200
    xdotool keydown Alt_L
    for c in $(seq 1 20); do
        sleep 0.1
        xdotool key Tab
    done
    xdotool keyup Alt_L
}

check_alttab()
{
    stage="$1" ; comment="$2"
    sleep 0.5
    if ! ps -p "$alttab" >/dev/null ; then
        echo "Test ${stage} ($comment) Failed"
        exit 1
    else
        echo "Test ${stage} ($comment) Passed"
    fi
}

cleanup()
{
    close_sample_windows
    stop_alttab
    stop_x
}


# begin execution

trap cleanup EXIT
install_software
start_x
open_sample_windows
start_alttab
check_alttab 1 "start"
alttab_simple_cycle
check_alttab 2 "simple cycle, focus"
close_sample_windows
check_alttab 3 "unmap windows"
stop_alttab
stop_x

