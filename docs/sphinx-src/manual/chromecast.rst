Chromecast
==========

Tizonia can cast audio to Chromecast devices from most of the supported
services. The services for which casting is not yet supported are:

* Spotify
* Plex
* Local Media

OPTIONS
-------

``-c [ --cast ] arg``
    Cast to a Chromecast device (arg: device name, 'friendly' name or ip address). Available in combination with Google Play Music, SoundCloud, YouTube, TuneIn and regular HTTP radio stations.

EXAMPLES
--------

.. code-block:: bash

   # Using Chromecast device's hostname
   $ tizonia --youtube-audio-mix-search "the greatest show" --cast "Chromecast-Ultra" --shuffle

   # Using Chromecast device's IP address
   $ tizonia --tunein-trending  --cast "192.168.1.141"

   # Using Chromecast device's 'friendly' name
   $ tizonia --gmusic-artist "dire straits" --cast "kitchen"
