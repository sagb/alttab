#!/bin/sh

# This file is part of alttab program.

# Update autotools stuff and documentation.
# To be run by maintainers and packagers.

autoreconf=autoreconf
project="$(dirname "$0")"


if [ "$1" \= "-h" ] ; then
    echo "Update autotools stuff and documentation. To be run by maintainers and packagers." >&2
    echo "Use: $0 [-f]" >&2
    echo "  -f : force regenerate everything" >&2
    exit 1
fi

if [ "$1" \= "-f" ] ; then
    force=yes
    ac_flag="-f"
fi

if [ "$(uname)" '=' "OpenBSD" ] ; then
    _arc="$(find /usr/local/bin -name 'autoreconf-*' 2>/dev/null | sort | tail -n 1)"
    if [ -n "$_arc" ] ; then
        export autoreconf="$_arc"
        export AUTOCONF_VERSION="${_arc##*-}"
    fi
    _am="$(find /usr/local/bin -name 'automake-*' 2>/dev/null | sort | tail -n 1)"
    if [ -n "$_am" ] ; then
        export AUTOMAKE_VERSION="${_am##*-}"
    fi
fi

cd "$project" || { echo 'Error: Can not `cd '"$project"'`'; exit 1 ;}
"$autoreconf" -vi $ac_flag

if which ronn >/dev/null 2>&1 ; then
    cd doc || { echo 'Error: Can not `cd ''doc''`'; exit 1 ;}
    if [ alttab.1.ronn -nt alttab.1 ] || [ "$force" \= "yes" ] ; then
        ronn --roff alttab.1.ronn
    fi
fi
