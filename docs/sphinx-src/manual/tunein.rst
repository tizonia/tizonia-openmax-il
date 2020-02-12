TuneIn
======

User credentials are not need to stream radio stations from TuneIn. Simply use
one of the command-line options provided to searchand liste to radio stations
from the TuneIn Internet radio directory.

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

``tunein.buffer_seconds``
  This is the minimum size of the audio buffer (in seconds) that Tizonia will
  use while downloading the audio streams. It may be increased in case of
  cuts, but usually not required. Default: 120.

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

   $ tizonia --tunein-trending 'heart UK'

   $ tizonia --tunein-search 'radio marca' --tunein-type "stations" --tunein-keywords "tenerife"

   $ tizonia --tunein-location "Europe" --tunein-keywords "Spain" --tunein-keywords "Madrid"

   $ tizonia --tunein-local "Cambridge"  --tunein-type "stations" --tunein-keywords "BBC"

   $ tizonia --tunein-music 'rock' --tunein-keywords "181.FM"  --tunein-keywords "US" --tunein-type "stations"

   $ tizonia --tunein-sports "news" --tunein-type "shows"

   $ tizonia --tunein-talk 'de noticias' --tunein-keywords "esradio" --tunein-keywords "spain"

   $ tizonia --tunein-podcasts "Classical Music" --tunein-keywords "global"

   $ tizonia --tunein-search "TWIT" --tunein-type "shows"
