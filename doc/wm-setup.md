
Setting up window manager for alttab
====================================

1. Disable WM's own Alt-Tab and any other applications which grab Alt-Tab
exclusively. Alternatively, let alttab use another shortcut, f.e. Left Ctrl
instead of Alt: `alttab -mm 4 -mk 0xffe3`.

2. Start alttab and see if it's usable at all: are windows recognized,
does alttab change focus? If no, then try to set `-w` option to the
following values: 1, 0, 3. If none works, then please report
the issue on github.

3. Autostart alttab _after_ window manager.


Tested window managers
----------------------

 WM/DE       | best value for `-w` option  | how to release grabbed Alt-Tab | autostart
------------ | --------------------------- | ------------------------------ | ---------
no WM        | 0 (set manually)            | doesn't grab                   | `alttab &` in ~/.xsession
ratpoison    | 2 (auto)                    | doesn't grab                   | `exec alttab` in ~/.ratpoisonrc
xmonad       | 1 (with plugin, see below)  | see "xmonad" section below     | 
dwm          | 1 (auto, partial support)   | ?                              | ?
i3           | 1 (auto)                    | doesn't grab                   | ?
evilwm       | 1 (auto)                    | ?                              | ?
twm          | 3 (default)                 | doesn't grab                   | ?
xfwm4/xfce   | 1 (auto)                    | see "xfce" section below       | 
metacity/MATE| 1 (auto)                    | see "MATE" section below       | 
jwm          | 1 (auto)                    | comment A-Tab entry in .jwmrc  | see "jwm" section below


xmonad
======

### keyboard shortcut
Xmonad is hungry for *Alt* key.
[Disable keybindings](http://xmonad.org/xmonad-docs/xmonad-contrib/XMonad-Doc-Extending.html#g:11) `(modMask,xK_Tab)` and `(modMask.|.shiftMask,xK_Tab)`
or [rebind modifier](https://wiki.haskell.org/Xmonad/Frequently_asked_questions#Rebinding_the_mod_key_.28Alt_conflicts_with_other_apps.3B_I_want_the_key.21.29) (neither method is tested, please report).

### inter-process communication
For setting window focus, [EWMH extension](http://xmonad.org/xmonad-docs/xmonad-contrib/XMonad-Hooks-EwmhDesktops.html) in xmonad is required (libghc-xmonad-contrib-dev), or xmonad will insist on its own focus (`alttab -w 0` will not work).
Also, there is no sane way to detect xmonad reliably without EWMH.

### startup
Use `startupHook` in xmonad.hs.


xfce
====

### keyboard shortcut
Applications -> Settings -> Settings Editor (not Manager) -> Channel: xfce4-keyboard-shortcuts -> in rigth pane disable all entries of `cycle_windows_key` or `cycle_reverse_windows_key`.

### startup
Applications -> Settings -> Session and Startup -> Application Autostart -> Add


MATE
====

### keyboard shortcut
System -> Preferences -> Hardware -> Keyboard Shortcuts -> disable Alt-Tab entries

### startup
System -> Control Center -> Startup Applications -> Add


jwm
===

Edit /etc/jwm/system.jwmrc or ~/.jwmrc:

### keyboard shortcut
Comment A-Tab entry.

### startup
Add/edit:

<StartupCommand>
    ...something else...
    alttab &
</StartupCommand>

