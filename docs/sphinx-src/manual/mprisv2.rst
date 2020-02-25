Remote Control
==============

Remote control of Tizonia is possible on environments that support `D-Bus
<https://en.wikipedia.org/wiki/D-Bus>`_. Tizonia implements a subset of the
`Media Player Remote Interfacing Specification
<https://www.freedesktop.org/wiki/Specifications/mpris-spec/>`_. More
specifically a subset of the `MPRIS D-Bus Interface Specification (version 2)
<https://specifications.freedesktop.org/mpris-spec/latest/>`_

Tizonia ships with a tool called ``tizonia-remote``. This tool provides an easy
way access to the subset of the MPRISv2 commands that are implemented.

CONFIGURATION
-------------

The Tizonia configuration file contains a block like this one (see
:ref:`tizonia-config-label`) under the ``[tizonia]`` section.

.. code-block:: bash

   # MPRIS v2 interface enable/disable switch
   # -------------------------------------------------------------------------
   # Valid values are: true | false
   #
   mpris-enabled = false

``mpris-enabled``
  Set to ``true`` or ``false`` to enable or disable the MPRISv2 remote control
  interface. When this element is set to ``true``, ``tizonia-remote`` will be
  able to control a running instance of Tizonia.

OPTIONS
-------

This is the help message provided by ``tizonia-remote``.

.. code-block:: bash

   $ tizonia-remote help
   This software is part of the Tizonia project <https://tizonia.org>

   GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl.html>
   This is free software: you are free to change and redistribute it.
   There is NO WARRANTY, to the extent permitted by law.

   MPRIS2 remote control for tizonia.
   http://specifications.freedesktop.org/mpris-spec/latest/

   Usage: /usr/bin/tizonia-remote [command]

       Misc. commands
       -----------------------------------------

       help                 this help text.
       kill                 sends the TERM signal.

       org.mpris.MediaPlayer2 interface
       --------------------------------
       ::Methods::

       quit                 causes the application to stop running.

       ::Properties::

       canquit              whether the media player quit.
       canraise             whether the media player can raise its UI.
       hastracklist         whether the org.mpris.MediaPlayer2.TrackList interface
                            is implemented.
       identity             returns the name that identifies the media player.
       urischemes           returns the URI schemes supported.
       mimetypes            returns the mime types supported.

       org.mpris.MediaPlayer2.Player interface
       ---------------------------------------
       ::Methods::

       play                 starts or resumes playback.
       stop                 stops playback.
       pause                pauses playback.
       next                 skips to the next track in the tracklist.
       prev                 skips to the previous track in the tracklist.
       seek                 (NOT IMPLEMENTED) seeks forward in the current
                            track by the specified number of microseconds.

       ::Properties::

       playstatus           the current playback status ("Playing", "Paused" or "Stopped").
       loopstatus           the current loop / repeat status ("None", "Track", "Playlist")
                            (Read/Write).
       rate                 the current playback rate (Read/Write).
       shuffle              the current shuffled playback status (true or false).
       metadata             the metadata of the current track (Read/Write) (NOT IMPLEMENTED).
       volume               the volume level (Read/Write).

       org.mpris.MediaPlayer2.TrackList interface
       ------------------------------------------
       NOT IMPLEMENTED

       org.mpris.MediaPlayer2.Playlists interface
       ------------------------------------------
       NOT IMPLEMENTED


EXAMPLES
--------

.. code-block:: bash

   $ tizonia-remote next

   $ tizonia-remote prev

   $ tizonia-remote pause

   $ tizonia-remote play

   $ tizonia-remote quit

   $ tizonia-remote volume .9

   $ tizonia-remote volume
   0.9
