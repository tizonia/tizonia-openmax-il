Google Play Music
=================

Music streaming from the Google Play Music service is supported on both free
and paid tiers. In both cases, credentials are required to be input via
command-line or configuration file.

CONFIGURATION
-------------

The Tizonia configuration file contains a block under the ``[tizonia]`` section
like the one pictured below (see also :ref:`tizonia-config-label`).

.. warning:: When user names and passwords are stored in your local
             ``tizonia.conf``, please ensure that this file has the correct
             file system permissions to prevent other users from accessing your
             credentials.

             E.g.: ``$ chmod og-rwx $HOME/.config/tizonia/tizonia.conf``


.. code-block:: bash

   # Google Play Music configuration
   # -------------------------------------------------------------------------
   # To avoid passing this information on the command line, uncomment and
   # configure here.
   #
   # gmusic.user       = user@gmail.com
   # gmusic.password   = pass (account password or app-specific password for
   #                          2-factor users)
   # gmusic.device_id  = deviceid (16 hex digits, e.g. '1234567890abcdef')
   # gmusic.buffer_seconds = size of the audio buffer (in seconds) to use while
   #                         downloading streams. Default: 720. Increase in
   #                         case of cuts.

``gmusic.user``
  The Google Play Music account username, e.g. 'user@gmail.com' or just 'user'.

``gmusic.password``
  Account password or app-specific password for 2-factor users

``gmusic.device_id``
  An Android device id associated to your account. This is usually a 16-digit
  hexadecimal number, e.g. '1234567890abcdef'. There are step-by-step tutorials
  on the Internet that may help with finding the device id on Android or iOS
  devices. There are also some apps in the Google Play store that may help with
  that, e.g.  `Device ID
  <https://play.google.com/store/apps/details?id=com.evozi.deviceid&hl=en_GB>`_.
  If you are a Google Play Music subscriber, you may also try to input a random
  device id. When using a random device id with a subscriber account, the
  software will return a list of real device ids associated to the account.


``gmusic.buffer_seconds``
  This is the minimum size of the audio buffer (in seconds) that Tizonia will
  use while downloading the audio streams. It may be increased in case of
  cuts, but usually not required. Default: 720.

OPTIONS
-------

.. note:: Command-line options that contain ``unlimited`` represent those
          that require a 'Premium' subscription to operate.

``--gmusic-user arg``
    Google Play Music user name (not required if provided via config file).

``--gmusic-password arg``
    Google Play Music user's password (not required if provided via config file).

``--gmusic-device-id arg``
    Google Play Music device id (not required if provided via config file).

``--gmusic-additional-keywords arg``
    Additional search keywords (this is optional: use in conjunction
    with--gmusic-unlimited-activity).

``--gmusic-library``
    Play all tracks from the user's library.

``--gmusic-tracks arg``
    Play tracks from the user's library by track name.

``--gmusic-artist arg``
    Play tracks from the user's library by artist.

``--gmusic-album arg``
    Play an album from the user's library.

``--gmusic-playlist arg``
    A playlist from the user's library.

``--gmusic-podcast arg``
    Search and play Google Play Music podcasts (only available in the US and
    Canada).

``--gmusic-station arg``
    Search and play Google Play Music free stations.

``--gmusic-unlimited-station arg``
    Search and play Google Play Music Unlimited stations found in the user's library.

``--gmusic-unlimited-album arg``
    Search and play Google Play Music Unlimited tracks by album (best match only).

``--gmusic-unlimited-artist arg``
    Search and play Google Play Music Unlimited tracks by artist (best match only).

``--gmusic-unlimited-tracks arg``
    Search and play Google Play Music Unlimited tracks by name (50 first matches only).

``--gmusic-unlimited-playlist arg``
    Search and play Google Play Music Unlimited playlists by name.

``--gmusic-unlimited-genre arg``
    Search and play Google Play Music Unlimited tracks by genre.

``--gmusic-unlimited-activity arg``
    Search and play Google Play Music Unlimited tracks by activity.

``--gmusic-unlimited-feeling-lucky-station``
    Play the user's Google Play Music Unlimited 'I'm Feeling Lucky' station.

``--gmusic-unlimited-promoted-tracks``
    Play Google Play Music Unlimited promoted tracks.

EXAMPLES
--------

.. code-block:: bash

   $ tizonia --gmusic-unlimited-album 'the division bell'

   $ tizonia --gmusic-unlimited-feeling-lucky-station

   $ tizonia --gmusic-unlimited-genre "alt metal"

   $ tizonia --gmusic-unlimited-playlist 'new metal'

   $ tizonia --gmusic-unlimited-promoted-tracks

   $ tizonia --gmusic-unlimited-station "top christmas songs"

   $ tizonia --gmusic-unlimited-tracks 'gun word up'

   $ tizonia --gmusic-unlimited-activity "working out" --gmusic-additional-keywords 'chart'

   $ tizonia --gmusic-unlimited-activity "working out" --gmusic-additional-keywords 'chart'
