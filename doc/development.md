
repository, bug reporting
-------------------------

[On github](https://github.com/sagb/alttab).

docs
----

Man page and README are maintained as ronn(1) (markdown-like) file.
To update man page and README:

> cd doc  
> ronn alttab.1.ronn  
> man -l alttab.1 > alttab.1.txt

This is not included in makefiles to not require everybody 
to install ronn.

coding
------

Functions return 0 on failure, positive on success.  
Indent is four spaces:

/* vim:tabstop=4:shiftwidth=4:smarttab:expandtab:smartindent  
*/

