
Setting up window manager for alttab
====================================

Basically two steps are required:

1. Autostart alttab _after_ window manager.
2. Disable WM's own Alt-Tab and any other applications which grab Alt-Tab exclusively.
Alternatively, let alttab use another shortcut, f.e. Left Ctrl instead of Alt:
`alttab -mm 4 -mk 0xffe3`.

Below are examples of these two steps for some window managers.

ratpoison
---------

1. Add to ~/.ratpoisonrc: `exec alttab`
2. Ratpoison doesn't grab Alt-Tab exclusively, so no additional configuration required.

xmonad
------

1. Use `startupHook` in xmonad.hs.
2. Xmonad is hungry for *Alt* key.
[Disable keybindings](http://xmonad.org/xmonad-docs/xmonad-contrib/XMonad-Doc-Extending.html#g:11) `(modMask,xK_Tab)` and `(modMask.|.shiftMask,xK_Tab)`
or [rebind modifier](https://wiki.haskell.org/Xmonad/Frequently_asked_questions#Rebinding_the_mod_key_.28Alt_conflicts_with_other_apps.3B_I_want_the_key.21.29) (neither method is tested, please report).

Also, for setting window focus, [EWMH extension](http://xmonad.org/xmonad-docs/xmonad-contrib/XMonad-Hooks-EwmhDesktops.html) in xmonad is required (libghc-xmonad-contrib-dev), or xmonad will insist on its own focus (`alttab -w 0` will not work).
Also, there is no sane way to detect xmonad reliably without EWMH.

xfce
----

1. Applications -> Settings -> Session and Startup -> Application Autostart -> Add
2. Applications -> Settings -> Settings Editor (not Manager) -> Channel: xfce4-keyboard-shortcuts -> in rigth pane disable all entries of `cycle_windows_key` or `cycle_reverse_windows_key`.

MATE
----

1. System -> Control Center -> Startup Applications -> Add
2. System -> Preferences -> Hardware -> Keyboard Shortcuts -> disable Alt-Tab entries

