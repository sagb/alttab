<!-- This file is solely for github front page.
     For distribution, use doc/README instead. -->
![Default options, raw X11](doc/screenshots/alttab-default-rawx.png?raw=true)

![Low DPI](doc/screenshots/alttab-high.png?raw=true)

[screenshot options](doc/screenshots/screenshots.md)

[![chat on freenode](https://img.shields.io/badge/chat-on%20freenode-brightgreen.svg)](https://webchat.freenode.net/?channels=%23alttab)

**alttab** is X11 window switcher designed for minimalistic window managers
or standalone X11 session.
```
  alttab  [-w N] [-d N] [-sc N] [-mk str] [-kk str] [-bk str] [-mm N] [-bm N] 
  [-t NxM] [-i NxM] [-vp str] [-p str] [-s N] [-theme name] [-bg color] 
  [-fg color] [-frame color] [-font name] [-v|-vv]
```
(see man page for details)
<!-- ronn page has elements invalid for github markdown, don't link to it -->

Unlike task switchers integrated in most simple window managers (WM) or
dmenu-like switchers, alttab features visual interface and convenient
tactile behaviour: press modifier (Alt) - multiple switch with
a key (Tab) - release modifier.
Also, it's lightweight and depends only on basic X11 libs, conforming
to the usage of lightweight WM.

# Installation
```
git clone https://github.com/sagb/alttab.git
cd alttab
./configure && sudo make install
```
See [INSTALL.md](INSTALL.md) for details and [doc/wm-setup.md](doc/wm-setup.md)
for window manager settings.

Usually it should run fully functional without any argument: `alttab`.  

# See also

[no-wm](https://github.com/patrickhaller/no-wm): use X11 without a window manager  
   
   
alttab (C) Alexander Kulak &lt;sa-dev AT rainbow POINT by&gt; 2016-2018

