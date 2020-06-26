.. Tizonia documentation

User Guide
==========

Tizonia provides an efficient and unbstrusive music search and listening
experience by following a few principles:

**Simple CLI UI**
  The keyboard is faster than the mouse, the goal is to listen to the music in
  less time. With Tizonia, the music plays almost immediately.

**Must play some music, not matter what**
  Tizonia uses fuzzy string matching techniques to match the search keywords
  against the data provided by the streaming services. This maximizes the
  chances of finding the music that the user wants. When a search returns no
  results Tizonia may try a 'I'm Feeling Lucky' guess.

**Music never stops**
  Tizonia's play queue always runs in **looped** mode. There is currently no
  way to configure non-looped playback (although this may be added in the
  future, see [#f1]_).

**Limited distractions**
  No fast-forward or rewind. Only skip to next/previous track (see [#f2]_).  No
  elaborate 'ncurses' CLI interface to interact with (some people might will see as
  a downside). Only the track info and a simple visual progress bar.

**Reduced 'muscle-memory'**
  Finding the music that you want and listening to it should require a single
  command-line option on the terminal. E.g.: ``tizonia --spotify-album Islands``.
  Bash and Zsh completions are available. There is also a
  `Oh-my-zsh <https://ohmyz.sh/>`_ plugin available `here
  <https://github.com/tizonia/tizonia-openmax-il/blob/master/player/tools/tizonia.plugin.zsh>`_.

.. toctree::
   :maxdepth: 1

   helptopics
   config
   keyboard
   global
   spotify
   googlemusic
   soundcloud
   youtube
   tunein
   plex
   iheart
   server
   client
   chromecast
   themes
   openmax
   localmedia
   mprisv2
   snap
   docker
   macos
   raspberrypi

.. rubric:: Footnotes

.. [#f1] There are plans to add an option to disable looped playback: `https://github.com/tizonia/tizonia-openmax-il/issues/491 <https://github.com/tizonia/tizonia-openmax-il/issues/491>`_.
.. [#f2] There are plans to add seeking, useful for long shows and podcasts: `https://github.com/tizonia/tizonia-openmax-il/issues/438 <https://github.com/tizonia/tizonia-openmax-il/issues/438>`_.
