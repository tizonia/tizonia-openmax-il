Global Options
==============

There are a number of features that may be used globally with all the included
services, or at a minumum, with more than one. Command-line options that fall
under this categoray are listed below.

OPTIONS
-------

``-h [ --help ] [=arg(=help)]``
    Print a usage message for a specific help topic (e.g. global, openmax, server, spotify, googlemusic, soundcloud, etc).

``-v [ --version ]``
    Print the version information.

``-r [ --recurse ]``
    Recursively process a given path.

``-s [ --shuffle ]``
    Shuffle the playlist.

``-d [ --daemon ]``
    Run in the background.

``-c [ --cast ] arg``
    Cast to a Chromecast device (arg: device name or ip address). Available in combination with Google Play Music, SoundCloud, YouTube, Plex and HTTP radio stations.

``-b [ --buffer-seconds ] arg``
    Size of the buffer (in seconds) to be used while downloading streams. Increase in case of cuts in gmusic, scloud, youtube or plex.

``--proxy-server arg``
    Url to the proxy server that should be used. (not required if provided via config file).

``--proxy-user arg``
    User name to be used during proxy server authentication (not required if provided via config file).

``--proxy-password arg``
    Password to be used during proxy server authentication (not required if provided via config file).
