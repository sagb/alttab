
Using alttab in xmonad
======================

For setting window focus, [EWMH extension](http://xmonad.org/xmonad-docs/xmonad-contrib/XMonad-Hooks-EwmhDesktops.html) in xmonad is required (libghc-xmonad-contrib-dev), or xmonad will insist on its own focus (`alttab -w 0` will not work).
Also, there is no sane way to detect xmonad reliably without EWMH.

Xmonad is hungry for *Alt* key.
[Disable keybindings](http://xmonad.org/xmonad-docs/xmonad-contrib/XMonad-Doc-Extending.html#g:11) (modMask,xK\_Tab) and (modMask.|.shiftMask,xK\_Tab)
or [rebind modifier](https://wiki.haskell.org/Xmonad/Frequently_asked_questions#Rebinding_the_mod_key_.28Alt_conflicts_with_other_apps.3B_I_want_the_key.21.29) (both workarounds are not tested, please report).
Alternatively, use Left Ctrl instead of Alt: `alttab -mm 4 -mk 0xffe3`.

Don't forget to run alttab _after_ WM, f.e., using startupHook in xmonad.hs.

