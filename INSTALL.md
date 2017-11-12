
Binaries
--------

* Unofficial packages for Debian are available.
Follow setup instructions [there](https://odd.systems/debian/).
Pin their repository to low priority to prefer official packages in the future.

* In FreeBSD, install [the port](https://www.freshports.org/x11/alttab/):

> pkg install alttab


Building from source
--------------------

1. Install build dependencies.
Basic Xlib, Xft, Xrender, libpng libraries and uthash macros are required.
In Debian or Ubuntu:

> apt-get install libx11-dev libxmu-dev libxft-dev libxrender-dev libpng-dev uthash-dev

Maintainer or packager may also install autotools and ronn:

> apt-get install autoconf automake ruby-ronn

2. Download:

> git clone https://github.com/sagb/alttab.git

3. Maintainer or packager may want to update autotools stuff and refresh documentation with ronn:

> ./bootstrap.sh

4. Build:

> ./configure  
> make  
> make install

See README for usage notes.

