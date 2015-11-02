.. Tizonia documentation


Developer Notes / Coding Style
==============================

General Philosophy
------------------

* `GNU Style <http://en.wikipedia.org/wiki/Indent_style#GNU_style>`_.
* Line length is generally limited to 80 characters.
* Each file starts with a header containing the short license text.


Formatting the Source Code
--------------------------

* clang-format is used and configuration files exist (.clang-format) in various sub-projects:

  * In general, C++ code follows a Google C++/Allman stylet (e.g. the tizonia
    command-line application, and the tizrmd daemon).

  * C-based libraries, like libtizonia, libtizcore, libtizplatform, etc, follow
    the GNU style.


Indentation
-----------

* Indentation is always done using spaces, tabs are never used.
* One level of indentation is 2 characters long.

Naming Variables, Functions, and Files
--------------------------------------
TODO



