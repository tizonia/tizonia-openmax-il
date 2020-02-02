Plex
====

Tizonia can stream audio from a Plex server. To stream from a Plex server, it
is necessary to provide the base URL where the server can be contacted and the
authentication token. These two elements must be provided on the command-line
or via configuration file.

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

