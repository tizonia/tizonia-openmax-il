.. Tizonia documentation

User Guide
==========

Tizonia provides an efficient and unbstrusive music search and listening
experience:

**Simplest CLI UI**
  Because the keyboard is faster than
  the mouse you get to listen to the music you want in less time.

**Play the music, not matter what**
  Tizonia uses fuzzy string matching techniques to match your search keywords
  against the data provided by the streaming services. Music plays
  immediately. When a search returns no results Tizonia may try a 'Feeling
  Lucky' guess.

**Non-stop music**
  Tizonia only mode of operation for its play queue is **looped**
  mode. Tizonia does not currently provide non-looped playback.

**Limit the distractions**
  By limiting the interactions. No fast-forward or rewind. Only skip to
  next/previous track. No elaborate 'ncurses' CLI interface to get
  distracted with. There is some track info and a simple visual progress bar.

**Reduce 'muscle-memory'**
  Finding the music that you want and listening to it should (for the most part)
  require a single command-line option typed on the terminal. E.g.:
  ``tizonia --spotify-album "Islands"``

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
   server
   client
   chromecast
   themes
   openmax
   localmedia
   mprisv2
