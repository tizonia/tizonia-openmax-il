Plex
====

Tizonia can stream audio from a Plex server. To stream from a Plex server, it
is necessary to provide the base URL where the server can be contacted and the
authentication token. These two elements must be provided on the command-line
or via configuration file.

CONFIGURATION
-------------

The Tizonia configuration file contains a block under the ``[tizonia]`` section
like the one pictured below (see also :ref:`tizonia-config-label`).

.. warning:: When credentials are stored in your local
             ``tizonia.conf``, please ensure that this file has the correct
             file system permissions to prevent other users from accessing your
             credentials!.

             E.g.: ``$ chmod -og-rx $HOME/.config/tizonia/tizonia.conf``

.. code-block:: bash

   # Plex configuration
   # -------------------------------------------------------------------------
   # To avoid passing this information on the command line, uncomment and
   # configure your Plex server and account auth token here.
   #
   # To find how to obtain a Plex user authentication token, see:
   # https://support.plex.tv/articles/204059436-finding-an-authentication-token-x-plex-token/
   #
   # plex.base_url = xxxxxxxxxxxxxx (e.g. http://plexserver:32400)
   # plex.auth_token = xxxxxxxxxxxxxx (e.g. SrPEojhap3H5Qj2DmjhX)
   # plex.buffer_seconds = size of the audio buffer (in seconds) to use
   #                       while downloading streams. Default: 60.
   #                       Increase in case of cuts.
   # plex.music_section_name = name of the music section in your plex library
   #                           (default: Music)


OPTIONS
-------

``--plex-server-base-url arg``
    Plex server base URL (e.g. 'http://plexserver:32400'. Not required if provided via config file).

``--plex-auth-token arg``
    Plex account authentication token (not required if provided via config file).

``--plex-music-section arg``
    Name of the Plex music section (needed if different from 'Music'; may be provided via config file).

``--plex-audio-tracks arg``
    Search and play audio tracks from a Plex server.

``--plex-audio-artist arg``
    Search and play an artist's audio tracks from a Plex server.

``--plex-audio-album arg``
    Search and play a music album from a Plex server.

``--plex-audio-playlist arg``
    Search and play playlists from a Plex server.


EXAMPLES
--------

.. code-block:: bash

