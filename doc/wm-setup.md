
Setting up window manager for alttab
====================================

1. Disable WM's own Alt-Tab and any other applications which grab Alt-Tab
exclusively. Alternatively, let alttab use another shortcut, f.e. Left Ctrl
instead of Alt: `alttab -mk Control_L`.

2. Start alttab and check is it usable at all: are windows recognized,
does alttab change focus? If no, then try to set `-w` option to the
following values: 1, 0, 3. If none works, then please report
the issue on github.

3. Autostart alttab _after_ window manager.


Tested window managers
----------------------

See [here](https://github.com/sagb/alttab/issues/33) how much alttab enhances 
each WM.


 WM/DE       | best value for `-w` option  | how to release grabbed Alt-Tab | autostart
------------ | --------------------------- | ------------------------------ | ---------
no WM        | 0 (set manually)            | doesn't grab                   | `alttab &` in ~/.xsession
ratpoison    | 2 (auto)                    | doesn't grab                   | `exec alttab` in ~/.ratpoisonrc
xmonad       | 1 (with plugin, see below)  | see "xmonad" section below     | 
dwm          | 1 (auto, partial support)   | ?                              | ?
i3           | 1 (auto)                    | doesn't grab                   | `exec_always alttab` in ~/.i3/config
evilwm       | 1 (auto)                    | ?                              | ?
twm          | 3 (default)                 | doesn't grab                   | ?
xfwm4/xfce   | 1 (auto)                    | see "xfce" section below       | 
metacity/MATE| 1 (auto)                    | see "MATE" section below       | 
jwm          | 1 (auto)                    | comment A-Tab entry in .jwmrc  | see "jwm" section below
openbox      | 1 (auto)                    | see "openbox" section below    |
fluxbox      | 1 (auto)                    | see "fluxbox" section below    | see "fluxbox" section below
icewm        | 1 (auto)                    | ?                              | ?
matchbox     | 1 (auto, partial support)   | doesn't grab                   | ?
enlightenment| 1 (auto)                    | ?                              | ?
blackbox     | 1 (auto)                    | doesn't grab                   | ?
window maker | 1 (auto, partial support)   | doesn't grab                   | ?
flwm         | 3 (default)                 | ?                              | ?
xpra (non-wm)| 1 (auto, partial support)   | doesn't grab                   | ?
cwm          | 1 (auto) (issue #35)        | ?                              | ?
wm2          | 3 (default)                 | doesn't grab                   | ?
aewm         | 1 (auto)                    | doesn't grab                   | ?
afterstep    | 3 (manually, issue #38)     | doesn't grab                   | ?
fvwm         | 1 (auto)                    | ?                              | ?
ctwm         | 1 (auto, see issue #39)     | doesn't grab                   | ?
lwm          | 1 (auto, issue #40)         | doesn't grab                   | ?
sawfish      | 1 (auto)                    | doesn't grab                   | ?
awesome      | 1 (auto)                    | doesn't grab                   | ?
bspwm        | 1 (auto, see #109, #152)    | doesn't grab                   | `alttab &` in ~/.config/bspwm/bspwmrc


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
Applications -> Settings -> Settings Editor (not Manager) -> Channel: xfce4-keyboard-shortcuts -> in right pane disable all entries of `cycle_windows_key` or `cycle_reverse_windows_key`.

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

Edit `/etc/jwm/system.jwmrc` or `~/.jwmrc`:

### keyboard shortcut
Comment A-Tab entry.

### startup
Add/edit:

    <StartupCommand>
        ...something else...
        alttab &
    </StartupCommand>

Also, "nomove" option may be added to the alttab group in .jwmrc to prevent
moving of alttab window (issue #31).


openbox
=======

### keyboard shortcut
Disable `A-Tab` and `A-S-Tab` keybinds in `/etc/xdg/openbox/rc.xml`

### startup
Add `alltab &` to `/etc/xdg/openbox/autostart`


fluxbox
=======

### keyboard shortcut
Disable/comment built-in non-GUI Alt-Tab in /etc/X11/fluxbox/keys or ~/.fluxbox/keys:

    #Mod1 Tab :NextWindow {groups} (workspace=[current])
    #Mod1 Shift Tab :PrevWindow {groups} (workspace=[current])

### startup
To start alttab _after_ fluxbox, in `~/.fluxbox/startup` replace `exec fluxbox` with:

    fluxbox &
    fbpid=$!
    sleep 3
    {
        alttab &
        # ...other apps to run after fluxbox startup...
    } &
    wait $fbpid

Details [here](http://fluxbox-wiki.org/Editing_the_startup_file.html).

