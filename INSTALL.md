
Binaries
--------

* In Debian _sid_, _buster_ or their Ubuntu derivatives, install alttab from the official repository:

    ```
    apt install alttab
    ```

    Also, unofficial deb packages for snapshots and backports are available.
    Follow setup instructions [there](https://odd.systems/debian/).

* In FreeBSD, install [the port](https://www.freshports.org/x11/alttab/):

    ```
    cd /usr/ports/x11/alttab/ && make install clean
    ```

    or add the package:

    ```
    pkg install alttab
    ```

* In Arch Linux, alttab is available in [AUR](https://aur.archlinux.org/packages/?O=0&K=alttab).

* In Alpine Linux, alttab is in _aports/testing_ repository.

* In openSUSE, alttab is available in Tumbleweed and from the [X11:Utilities](https://build.opensuse.org/package/show/X11:Utilities/alttab) repository:

    ```
    zypper ar -f obs://X11:Utilities x11utils
    zypper ref
    zypper in alttab
    ```

Building from source
--------------------

1. Install build dependencies.
    Basic Xlib, Xft, Xrender, Xrandr, libpng, libxpm libraries
    and [uthash macros](http://troydhanson.github.io/uthash/) are required.
    In Debian or Ubuntu:

    ```
    apt install libx11-dev libxmu-dev libxft-dev libxrender-dev libxrandr-dev libpng-dev libxpm-dev uthash-dev
    ```

    Maintainer or packager may also install autotools and ronn:

    ```
    apt install autoconf automake ronn
    ```

2. Download:

    ```
    git clone https://github.com/sagb/alttab.git && cd alttab
    ```

3. Maintainer or packager may want to update autotools stuff and refresh documentation with ronn:

    ```
    ./bootstrap.sh
    ```

4. Build:

    ```
    ./configure  
    make  
    make install
    make check  # optional
    ```

### In OpenBSD (as of OpenBSD 7.4 amd64):

1. Install build dependencies.  
    `Xlib`, `Xft`, `Xrender`, `Xrandr` and `libxpm` come from [`xbase` file set](https://www.openbsd.org/faq/faq4.html#FilesNeeded)  
    `perl` is part of `base` file set.  
    In order to install others:

    ```
    # may omit autoconf-2.69 as it is pulled in as a dependency of automake-1.16.5
    pkg_add git png uthash automake-1.16.5
    ```
    
    If you intend to run `make check`, then also:
    
    ```
    pkg_add gawk
    ```

2. Download:

    ```
    git clone https://github.com/sagb/alttab.git && cd alttab
    ```

3. Update autotools stuff - **mandatory step for OpenBSD**:

    ```
    ./bootstrap.sh -f
    ```

4. Build:

    ```
    CPATH=/usr/local/include ./configure --mandir /usr/local/man
    CPATH=/usr/local/include make
    make install
    makewhatis /usr/local/man  # update mandoc.db with alttab.1
    make check  # optional
    ```

Usage notes
-----------

See `README.md`
