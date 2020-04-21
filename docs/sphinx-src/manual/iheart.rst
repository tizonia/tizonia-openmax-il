iHeart
======

User credentials are not need to stream radio stations from iHeart. Simply use
one of the command-line options provided to search and listeb to radio stations
from the iHeart Internet radio directory.

CONFIGURATION
-------------

The Tizonia configuration file contains a block under the ``[tizonia]`` section
like the one pictured below (see also :ref:`tizonia-config-label`).

.. code-block:: bash

   # iHeart configuration
   # -------------------------------------------------------------------------
   # To avoid passing this information on the command line, uncomment and
   # configure here.
   #
   #
   # iheart.buffer_seconds = size of the audio buffer (in seconds) to use
   #                         while downloading streams. Default: 120.
   #                         Increase in case of cuts.

``iheart.buffer_seconds``
  This is the minimum size of the audio buffer (in seconds) that Tizonia will
  use while downloading the audio streams. It may be increased in case of
  cuts, but usually not required. Default: 120.

OPTIONS
-------

``--iheart-search arg``
    iheart station search.

``--iheart-keywords arg``
    Additional keywords that may be used in conjunction with the iheart search option. Optional (may be repeated).


EXAMPLES
--------

.. code-block:: bash

   $ tizonia --iheart-search "top 40"

   $ tizonia --iheart-search "kiss"

   $ tizonia --iheart-search "ny"

   $ tizonia --iheart-search "albuquerque"

   $ tizonia --iheart-search "90s"

   $ tizonia --iheart-search "rock" --iheart-keywords 'The Rocket'
