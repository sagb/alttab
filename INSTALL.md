
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

* In Arch Linux, alttab is available in [AUR](https://aur.archlinux.org/packages/?O=0&K=alttab) (outdated version at Mar 2018).

* In Alpine Linux, alttab is in _aports/testing_ repository.


Building from source
--------------------

1. Install build dependencies.
    Basic Xlib, Xft, Xrender, Xrandr, libpng libraries
    and [uthash macros](http://troydhanson.github.io/uthash/) are required.
    In Debian or Ubuntu:

    ```
    apt install libx11-dev libxmu-dev libxft-dev libxrender-dev libxrandr-dev libpng-dev uthash-dev
    ```

    Maintainer or packager may also install autotools and ronn:

    ```
    apt install autoconf automake ruby-ronn
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
    ```
    See README for usage notes.

