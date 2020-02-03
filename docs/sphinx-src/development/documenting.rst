.. Tizonia documentation

Documenting Tizonia
===================

We use `Doxygen <http://www.doxygen.nl/>`_ to document the C APIs and `Sphinx
<https://www.sphinx-doc.org/en/master/>`_ to produce our manuals and to give
everything a nicer look.

How To Build the Documentation
------------------------------

The documentation is built in 2 steps:

1. The Doxygen API docs are generated first.
2. The Sphinx docs are generated next. The Doxygen API documentation generated
   in the first step is consumed during this stage as they are embedded into
   the final Sphinx output.

.. code-block:: bash

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

   # Now change into the 'sphinx-src' dir and generate the Sphinx docs website
   # and the man page
   $ cd sphinx-src
   $ make html && make man

