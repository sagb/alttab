
Dependencies
------------

Basic Xlib, Xft, Xrender, libpng libraries and uthash macros are required.
In Debian or Ubuntu:

> apt-get install libx11-dev libxmu-dev libxft-dev libxrender-dev libpng-dev uthash-dev

As maintainer or packager, also install autotools and ronn:

> apt-get install autoconf automake ruby-ronn


Installing alttab
-----------------

1. Download:

> git clone https://github.com/sagb/alttab.git

2. Maintainer or packager may want to update autotools stuff and refresh documentation with ronn:

> ./bootstrap.sh

3. Build:

> ./configure  
> make  
> make install

See README for usage notes.

