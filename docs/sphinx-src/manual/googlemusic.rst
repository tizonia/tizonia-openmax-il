Google Play Music
=================

Music streaming from the Google Play Music service is supported in both free
and paid tiers. Command-line options that contain ``unlimited`` represent those
that require a paid subscription. Credentials are required to be input via
command-line or configuration file.

OPTIONS
-------

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
