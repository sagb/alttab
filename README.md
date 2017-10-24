<!-- This file is solely for github front page.
     For distribution, use doc/README instead. -->
![Default options, raw X11](doc/screenshots/alttab-default-rawx.png?raw=true)

![Low DPI](doc/screenshots/alttab-high.png?raw=true)

[screenshot options](doc/screenshots/screenshots.md)

[![chat on freenode](https://img.shields.io/badge/chat-on%20freenode-brightgreen.svg)](https://webchat.freenode.net/?channels=%23alttab)

```
alttab is X11 window switcher designed for minimalistic window managers
or standalone X11 session.

  alttab  [-w N] [-mm N] [-bm N] [-mk N] [-kk N] [-t NxM] [-i NxM]
  [-s N] [-theme name] [-bg color] [-fg color] [-frame color]
  [-font name] [-v|-vv]

(see man page for details)

Unlike task switchers integrated in most simple window managers (WM) or
dmenu-like switchers, alttab features visual interface and convenient
tactile behaviour: press modifier (Alt) - multiple switch with
a key (Tab) - release modifier.
Also, it's lightweight and depends only on basic X11 libs, conforming
to the usage of lightweight WM.

Usage
-----

Usually it should run fully functional without any argument.
Start alttab after WM, to let it auto-recognize the WM. For examples, add
the following to ~/.ratpoisonrc:

  exec alttab

If there are no WM at all, then start alttab in ~/.xsession or elsewhere.

Source
------

Copyright 2017 Alexander Kulak <sa-dev AT rainbow POINT by>.
License: GPLv3 (see COPYING).
Repository: https://github.com/sagb/alttab
Chat: #alttab on Freenode


 -- Alexander Kulak <sa-dev@rainbow.by>  Fri, 28 Apr 2017 13:19:28 +0300
```

