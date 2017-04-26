
Dependencies
------------

Basic Xlib and Xft libraries are required.
In Debian or Ubuntu:

> apt-get install libx11-dev libxmu-dev libxft-dev

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

4. Run:

> alttab

Usually it should be fully functional without any argument.
Start alttab after WM, to let it auto-recognize the WM. For examples, add the following to ~/.ratpoisonrc:

exec alttab

If there are no WM at all, then start alttab in ~/.xsession or elsewhere.

