How to generate Tizonia's man page
----------------------------------

Tizonia's man page is generated using the Sphinx documentation system found in
the repo's top-level 'docs' directory.


   # Install the dependencies
   $ sudo apt install doxygen
   $ sudo -H pip3 install --upgrade sphinx breathe alabaster recommonmark

   # Clone the repo
   $ git clone https://github.com/tizonia/tizonia-openmax-il

   # Change into the 'docs' directory
   cd tizonia-openmax-il/docs

   # Configuration
   $ autoreconf -ifs && ./configure

   # Generate the Doxygen API docs
   $ make

   # Now change into the 'sphinx-src' dir and generate the man page
   $ cd sphinx-src
   $ make man

   # Now copy the man page generated under tizonia-openmax-il/docs/sphinx-src/_build/man/tizonia.1
   cp tizonia-openmax-il/docs/sphinx-src/_build/man/tizonia.1 tizonia-openmax-il/player/man/
