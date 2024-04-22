Why alttab doesn't show the icon or shows an unexpected icon?
=============================================================

First, read the description of the `-s` option in `man alttab`.  
If the issue is still unclear:

Build alttab with `ICON_DEBUG=1` and start it with `-v` option:

```shell
./configure CFLAGS="-DICON_DEBUG=1"
make
src/alttab -v (...other options)
```

Make the switcher appear, and then find the following fragment in the alttab
stderr output:

```
got N windows
0: 1c0000e (lvl 0, icon 65011716 (32x32)): window name 0
   <source of icon>
1: 3c00003 (lvl 0, icon 65011737 (32x32)): window name 1
   <source of icon>
...
```

The source of the icon should help you figure out what's happening.  
If still in trouble, [file a bug report](https://github.com/sagb/alttab/issues).
