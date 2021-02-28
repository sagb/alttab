
Stability
---------

Alttab is considered stable.

Repository, bug reporting
-------------------------

[On github](https://github.com/sagb/alttab).

Maintainer script
-----------------

To rebuild autotools stuff use bootstrap.sh.

Also, this script updates man page and README which are maintained 
as ronn(1) (markdown-like) file.
This is not included in makefiles to not require casual user 
to install ronn.

Coding
------

Functions return 0 on failure, positive on success.  
No strict requirements are imposed, but for the time present, the code
adheres to the following:

* Satisfy `-Wall` compiler option
* Use the following indent: `indent -nbad -bap -nbc -bbo -hnl -br -brs -c33 -cd33 -ncdb -ce -ci4 -cli0 -d0 -di1 -nfc1 -i4 -ip0 -l80 -lp -npcs -nprs -npsl -sai -saf -saw -ncs -nsc -sob -nfca -cp33 -ss -ts4 -il1 -nut`
* Pass Clang Static Analyzer check (scan-build make) without warnings, except of those about uthash internals and obvious false-positives

The only global variables are: g, dpy, scr, root, ee\_complain.

X error handler doesn't abort on error. To disable even the error message,
temporary set ee\_complain to false.

Test suite
----------

`make check` should work.

/* vim:tabstop=4:shiftwidth=4:smarttab:expandtab:smartindent  
*/

