
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
Satisfy `-Wall` compiler option.
Indent is four spaces:

/* vim:tabstop=4:shiftwidth=4:smarttab:expandtab:smartindent  
*/

