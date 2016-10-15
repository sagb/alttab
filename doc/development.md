
repo
----

docs
----

Man page is maintained as ronn(1) (markdown-like) file.
To produce man page:

ronn -r alttab.1.ronn

This is not included in makefiles to not require everybody 
to install ronn.

coding
------

Functions return 0 on failure, positive on success.
Indent is four spaces:

/* vim:tabstop=4:shiftwidth=4:smarttab:expandtab:smartindent
*/

