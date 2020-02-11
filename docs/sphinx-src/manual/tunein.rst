TuneIn
======

TODO

CONFIGURATION
-------------

The Tizonia configuration file contains a block under the ``[tizonia]`` section
like the one pictured below (see also :ref:`tizonia-config-label`).

.. code-block:: bash

   # Tunein configuration
   # -------------------------------------------------------------------------
   # To avoid passing this information on the command line, uncomment and
   # configure here.
   #
   #
   # tunein.buffer_seconds = size of the audio buffer (in seconds) to use
   #                         while downloading streams. Default: 120.
   #                         Increase in case of cuts.


OPTIONS
-------

``--tunein-search arg``
    TuneIn global station/podcast search.

``--tunein-local arg``
    TuneIn 'local' category search.

``--tunein-music arg``
    TuneIn 'music' category search.

``--tunein-talk arg``
    TuneIn 'talk' category search.

``--tunein-sports arg``
    TuneIn 'sports' category search.

``--tunein-location arg``
    TuneIn 'location' category search.

``--tunein-podcasts arg``
    TuneIn 'podcasts' category search.

``--tunein-trending arg``
    TuneIn 'trending' category search.

``--tunein-type arg``
    Narrow down the search to specific type: 'stations', 'shows', or 'all' (default: all). Optional.

``--tunein-keywords arg``
    Additional keywords that may be used in conjunction with the TuneIn options. Optional (may be repeated).


EXAMPLES
--------

.. code-block:: bash
