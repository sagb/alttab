Binaries
--------

* In Debian or its derivatives, install alttab from the official repository:

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
    Cmake and basic Xlib, Xft, Xrender, Xrandr, libpng, libxpm libraries
    and [uthash macros](http://troydhanson.github.io/uthash/) are required.
    In Debian or Ubuntu:

    ```
    apt install cmake libx11-dev libxmu-dev libxft-dev libxrender-dev libxrandr-dev libpng-dev libxpm-dev uthash-dev
    ```

    If you need to regenerate man page, also install ronn:

    ```
    apt install ronn
    ```

2. Download:

    ```
    git clone https://github.com/sagb/alttab.git && cd alttab
    ```

3. Build, test, install:

    ```
    mkdir build
    cd build
    cmake ..
    cmake --build .
    ctest  # optional
    sudo cmake --build . --target install
    ```

### In OpenBSD (as of OpenBSD 7.4 amd64):

1. Install build dependencies.  
    `Xlib`, `Xft`, `Xrender`, `Xrandr` and `libxpm` come from [`xbase` file set](https://www.openbsd.org/faq/faq4.html#FilesNeeded)  
    `perl` is part of `base` file set.  

2. Download:

    ```
    git clone https://github.com/sagb/alttab.git && cd alttab
    ```

3. Build:

    Proceed with cmake as described above. Former autotools build required 
    CPATH=/usr/local/include , perhaps cmake also needs this. Please test and report.

Usage
-----

See README or man page.  
Usually it should run fully functional without any argument: `alttab`.  
