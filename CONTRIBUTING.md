Contributing to alttab
======================

alttab is small, stable and maintained by one person. Every line merged
here is maintained forever, so the bar for code is high:

* **Bug fixes are welcome.** Keep them minimal: change only what is needed
  to fix the bug.
* **New features are accepted only if they are universal**: useful to many
  users of many window managers, not achievable with existing options, and
  not a matter of taste. Options that make alttab look or behave like
  a particular WM, theme or personal setup will be declined. Keep them
  in your fork.
* **Large changes are not welcome.** A feature that needs more than
  a few dozen lines in `src/` will most likely be declined, however well
  it is written.
* **Do not refactor.** Do not change function signatures, rename, move,
  reformat or "clean up" existing code unless your fix can't be done
  otherwise. If it can't, explain why in the PR.
* **One feature or fix per PR.**

If you are unsure whether a feature fits, open an issue first and
describe the use case. No code is needed to discuss an idea.

Every PR
--------

* Follows the coding rules in [doc/development.md](doc/development.md):
  `-Wall` clean, the `indent` style, no new `scan-build` warnings,
  `make check` passes.
* Updates `doc/alttab.1.ronn` if user-visible behaviour changes,
  and regenerates `doc/alttab.1` from it with `ronn --roff` if possible.
  `README.md` and `doc/README` are edited by hand to match.

Testing a new option
--------------------

THIS SECTION IS FOR NEW FEATURES ONLY. BUG FIXES DON'T NEED IT:
just describe how you tested the fix.

A new option interacts with all existing ones, and the maintainer can't
test every combination. Before submitting, test it yourself and write
the results in the PR.

Window managers, at least:

* `-w 0`: no WM (e.g. bare Xvfb)
* `-w 1`: one stacking WM with minimizing (e.g. Openbox, xfwm4)
  and one tiling WM (e.g. i3)
* `-w 3`: twm

`-w 2` (ratpoison) is optional, but welcome.

Option sets, each combined with your option:

1. no other options
2. `-d 1 -s 1 -b 1 -sortmin -min red -bw 3`
3. `-vertical -t 300x40 -i 24x24 -b 2`
4. `-t 48x48 -i 48x48 -fw 1` (icon fills the whole tile)
5. `-s 4 -e` (no icons, keep switcher)

Windows open during the test: one with a long non-ASCII title, one minimized,
one without an icon, and enough windows to overflow the screen width.
Also test with a single window.

In each case check forward, backward and wrap-around selection,
selecting by mouse, killing a client (`-dk`) and cancel.

If the change touches loading or drawing of icons, also test every icon
source `-s 0` to `-s 5` with windows whose icons come from:

* X11 window attributes: `_NET_WM_ICON` and legacy `WM_HINTS` pixmap,
  including a pixmap of depth other than 24
* files: PNG and XPM, in freedesktop theme directories and in
  `/usr/share/pixmaps`, with sizes smaller, equal and larger than `-i`
* no icon at all

Attach to the PR:

* screenshots of option sets 1-5 in one WM, and of anything that looks
  different in other WMs
* the leak summary that `ALTTAB_VALGRIND=1 make check` writes to
  `test/run-in-xvfb.log`, compared with the same run of master: no new leaks
* pixmap count of alttab in `xrestop` after the first and after the 20th
  cycle: it must not grow

Without these results the PR will not be reviewed.
