Spotify
=======

A Spotify Premium account is required to stream audio from Spotify. Tizonia
will not work with a Spotify Free account. Please introduce your Premium
account's user name and password into ``tizonia.conf``, or alternatively, use
the corresponding command-line options available.

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

# Spotify configuration
   # -------------------------------------------------------------------------
   # To avoid passing this information on the command line, uncomment
   # and configure accordingly
   #
   # spotify.user     = user
   # spotify.password = pass
   # spotify.recover_lost_token = false (set to true to continue playback after
   #                                     the token has been [spuriously?] lost)
   # spotify.allow_explicit_tracks = false (set to true to allow explicit tracks)
   # spotify.preferred_bitrate = Three possible values: 96, 160, or 320 ; default: 320

``spotify.user``
   The username of the Spotify Premium account. For older accounts, the
   username may be related to account email address (i.e. the email address
   without the @domain part). For newer accounts, the username is a random
   string of around 25 characters allocated by Spotify. This information is
   found in the ``Account overview`` section on the Spotify website.
   https://www.spotify.com/us/account/overview .

.. note:: If you created your account through Facebook you will need to create
          a 'device password' to stream from Spotify using Tizonia. Note that
          the device password option is only available for Facebook-created
          accounts. Please read the Spotify documentation at
          http://www.spotify.com/account/set-device-password/.

.. note:: User names with a '+' character will not work. There is a bug (`#599
          <https://github.com/tizonia/tizonia-openmax-il/issues/599>_`) in Tizonia
          that prevents the use of '+' characters in the user name field.

``spotify.password``
   Password of the Premium account. If you created your account through
   Facebook you will need to create a 'device password' (see note above).

.. note:: Please do not allow '#' or '$' in the password. There is a bug (`#571
          <https://github.com/tizonia/tizonia-openmax-il/issues/599>_`) in Tizonia
          that prevents this character in the password field.

``spotify.recover_lost_token``
  Set to ``true`` to allow Tizonia to resume playback and recover the token
  automatically when the playback token is lost. This can occur if for example,
  another device starts streaming using your account (e.g. listening to Spotify
  on an Echo device while Tizonia is already streaming on your
  computer). Optional (Default: false).

``spotify.allow_explicit_tracks``
  Set to ``true`` to allow explicit tracks in Tizonia's play queue. Otherwise,
  explicit tracks are removed from the queue before playback. Optional
  (Default: false).

``spotify.preferred_bitrate``
  Select the preferred bitrate while streaming from Spotify. Three possible
  values: 96, 160, or 320. Optional (Default: 320).

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

``--spotify-recommendations-by-track arg``
    Play Spotify recommendations by track name.

``--spotify-recommendations-by-artist arg``
    Play Spotify recommendations by artist name.

``--spotify-recommendations-by-genre arg``
    Play Spotify recommendations by genre name.

``--spotify-user-liked-tracks``
    Play the user's liked tracks.

``--spotify-user-recent-tracks``
    Play the user's most recent tracks.

``--spotify-user-top-tracks``
    Play the user's top tracks.

``--spotify-user-top-artists``
    Play tracks from the user's top artists.

``--spotify-user-playlist arg``
    Play tracks from the user's playlist (including private playlists,
    Daily Mixes and Discover Weekly).

    Note that Daily Mixes and Discover Weekly playlists need to be 'liked'
    beforehand in order to be found by a search performed on the user
    library.


EXAMPLES
--------

.. code-block:: bash

   $ tizonia --spotify-artist 'enya'

   $ tizonia --spotify-album 'the greatest showman'

   # Search and play a *public* playlist owned by the current user
   $ tizonia --spotify-playlist 'Summer 2019'

   # Search and play a *public* playlist owned by the specified user
   $ tizonia --spotify-playlist 'Summer 2019' --spotify-owner 'bqmtzm68dmdyk2uyvrwma69y2'

   # Globally search and play a *public* playlist on Spotify (Tizonia will play
   # the best match in the list returned by the Spotify service)
   $ tizonia --spotify-playlist 'best metal 2000s' --spotify-owner 'anyuser'

   # Play recommended tracks by seeding the search with a specific track and artist
   $ tizonia --spotify-recommendations-by-track 'Word up by cameo'
   $ tizonia --spotify-recommendations-by-track 'Word up by Gun'
   $ tizonia --spotify-recommendations-by-track 'Word up by Korn'

   # Play recommended tracks by seeding the search with a specific artist
   $ tizonia --spotify-recommendations-by-artist 'queen'

   # Play recommended tracks by seeding the search with a specific Spotify track id/uri/url
   $ tizonia --spotify-recommendations-by-track-id 3MrRksHupTVEQ7YbA0FsZK
   $ tizonia --spotify-recommendations-by-track-id 'spotify:track:3MrRksHupTVEQ7YbA0FsZK'
   $ tizonia --spotify-recommendations-by-track-id 'https://open.spotify.com/track/3MrRksHupTVEQ7YbA0FsZK'

   # Play the user's Discover Weekly and Daily Mixes in the user's
   # library. Note that these playlists must have been 'liked' beforehand
   # on Spotify
   $ tizonia --spotify-user-playlist "Discover Weekly"
   $ tizonia --spotify-user-playlist "Daily Mix 1"
   $ tizonia --spotify-user-playlist "Daily Mix 4"
