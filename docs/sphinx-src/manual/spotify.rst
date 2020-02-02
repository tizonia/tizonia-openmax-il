Spotify
=======

TODO

OPTIONS
-------

``--spotify-user arg``
    Spotify user name  (not required if provided via config file).

``--spotify-password arg``
    Spotify user password  (not required if provided via config file).

``--spotify-owner arg``
    The owner of the playlist  (this is optional: use in conjunction with --spotify-playlist or --spotify-playlist-id).

``--spotify-recover-lost-token``
    Allow Tizonia to recover the play token and keep playing after a spurious token loss (default: false).

``--spotify-allow-explicit-tracks``
    Allow Tizonia to play explicit tracks from Spotify (default: false).

``--spotify-preferred-bitrate arg``
    Preferred Spotify bitrate (kbps) (320, 160 or 96; default: 320).

``--spotify-tracks arg``
    Search and play from Spotify by track name.

``--spotify-artist arg``
    Search and play from Spotify by artist name.

``--spotify-album arg``
    Search and play from Spotify by album name.

``--spotify-playlist arg``
    Search and play public playlists (owner is assumed the current user, unless --spotify-owner is provided).

``--spotify-track-id arg``
    Play from Spotify by track ID, URI or URL.

``--spotify-artist-id arg``
    Play from Spotify by artist ID, URI or URL.

``--spotify-album-id arg``
    Play from Spotify by album ID, URI or URL.

``--spotify-playlist-id arg``
    Play public playlists from Spotify by ID, URI or URL (owner is assumed the current user, unless --spotify-owner is provided).

``--spotify-related-artists arg``
    Search and play from Spotify the top songs from a selection of related artists.

``--spotify-featured-playlist arg``
    Search and play a featured playlist from Spotify.

``--spotify-new-releases arg``
    Search and play a newly released album from Spotify.

``--spotify-recommendations-by-track-id arg``
    Play Spotify recommendations by track ID, URI or URL

``--spotify-recommendations-by-artist-id arg``
    Play Spotify recommendations by artist ID, URI or URL.

``--spotify-recommendations-by-genre arg``
    Play Spotify recommendations by genre name.
