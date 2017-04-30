
repository, bug reporting
-------------------------

[On github](https://github.com/sagb/alttab).

maintainer script
-----------------

To rebuild autotools stuff use bootstrap.sh.

Also, this script updates man page and README which are maintained 
as ronn(1) (markdown-like) file.
This is not included in makefiles to not require casual user 
to install ronn.

coding
------

Functions return 0 on failure, positive on success.  
No strict requirements are imposed, but for the time present, the code
adheres to the following:

* Satisfy `-Wall` compiler option
* Use Linux coding style (indent -linux)
* Pass Clang Static Analyzer check (scan-build make) without warnings

/* vim:tabstop=4:shiftwidth=4:smarttab:expandtab:smartindent  
*/
