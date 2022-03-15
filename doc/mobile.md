Alttab on mobile devices
========================

Alttab displays on every device that can run X11.

It's trivial with Linux phone (Pinephone, Librem, Pro1-X, Volla).

With Android, alttab runs fine in Linux chroot
displaying in [XServer XSDL](https://play.google.com/store/apps/details?id=x.org.server).


Touchscreen control
-------------------

Using `-e` option, an external program can pop up the switcher:

```shell
alttab -e &
xdotool key alt+Tab
```

This is particularly useful with Linux phone and 
[gesture daemon](https://git.sr.ht/~mil/lisgd).
To call alttab by swipe to the right at the left screen edge:

```shell
alttab -mk Control_L -e &
lisgd \
    -g '1,LR,L,*,/usr/bin/xdotool key ctrl+Tab' \
    ...other lisgd options... &
```

It looks like this: [2.3M mp4 video](screenshots/alttab-mobile.mp4).

