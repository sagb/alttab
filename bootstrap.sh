#!/bin/sh

# This file is part of alttab program.

# Update autotools stuff and documentation.
# To be run by maintainers and packagers.

set +e

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

project=`dirname $0`

cd "$project"
autoreconf -vi $ac_flag

cd doc
if [ alttab.1.ronn -nt alttab.1 -o "$force" \= "yes" ] ; then
    ronn --roff alttab.1.ronn
    MANWIDTH=80 man -l alttab.1 > alttab.1.txt
fi

