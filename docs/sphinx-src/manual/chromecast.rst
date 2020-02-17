Chromecast
==========

Tizonia can cast audio to Chromecast devices from most of the supported
services. A number of services for which casting is not yet supported
are:

* Spotify (see [#f1]_)
* Plex (see [#f2]_)
* Local Media

OPTIONS
-------

``-c [ --cast ] arg``
    Cast to a Chromecast device (arg: device name, 'friendly' name or ip
    address). Available in combination with Google Play Music, SoundCloud,
    YouTube, TuneIn and regular HTTP radio stations.

EXAMPLES
--------

.. code-block:: bash

   # Using Chromecast device's hostname
   $ tizonia --youtube-audio-mix-search "the greatest show" --cast "Chromecast-Ultra" --shuffle

   # Using Chromecast device's IP address
   $ tizonia --tunein-trending  --cast "192.168.1.141"

   # Using Chromecast device's 'friendly' name
   $ tizonia --gmusic-artist "dire straits" --cast "kitchen"

.. rubric:: Footnotes

.. [#f1] There are plans to add casting of Spotify streams: `https://github.com/tizonia/tizonia-openmax-il/issues/461 <https://github.com/tizonia/tizonia-openmax-il/issues/461>`_.
.. [#f2] There are plans to add casting of Plex streams: `https://github.com/tizonia/tizonia-openmax-il/issues/462 <https://github.com/tizonia/tizonia-openmax-il/issues/462>`_.
