.. _tizonia-docker-label:

Docker Image
============

In addition to Debian and Snap packages, there is also a docker image of
Tizonia in the `Docker Hub <https://hub.docker.com/r/tizonia/docker-tizonia>`_.


TIZONIA ON DOCKER
-----------------

To pull the image, run:

.. code-block:: bash

   $ docker pull tizonia/docker-tizonia

Launch Command
--------------

There is a convenience script to start Tizonia called `docker-tizonia
<https://github.com/tizonia/docker-tizonia/blob/master/docker-tizonia>`_. The
script bind mounts the host's ``$HOME/.config/tizonia`` to make
``tizonia.conf`` available inside the container.

.. note::

   The Tizonia process running inside the container needs 'rwx' permissions on
   ``$HOME/.config/tizonia``.

The script also bind mounts the host's ``$HOME/.cache`` to allow
debug logs to be written to disk. For example, gmusicapi logs for Google Play
Music can be found at ``$HOME/.cache/gmusicapi/log/gmusicapi.log``.

.. code-block:: bash

   #!/bin/bash

   USER_ID=$(id -u);
   GROUP_ID=$(id -g);

   if uname -s | grep -iq "Darwin" ; then
     pulse_server=docker.for.mac.localhost
     runtime_dir="$HOME"
   else
     pulse_server=unix:"${XDG_RUNTIME_DIR}/pulse/native"
     runtime_dir="${XDG_RUNTIME_DIR}/pulse"
   fi

   docker run -it --rm \
       -e PULSE_SERVER="$pulse_server" \
       --volume="$runtime_dir":"$runtime_dir" \
       --volume="${HOME}/.config/tizonia":/home/tizonia/.config/tizonia \
       --volume="${HOME}/.config/pulse/cookie":/home/tizonia/.config/pulse/cookie \
       --volume="${HOME}/.cache":/home/tizonia/.cache \
       --name tizonia \
       tizonia/docker-tizonia "$@";

.. note::

   Please make sure you download the latest version of the convenience script
   from its location on GitHub:

   - https://github.com/tizonia/docker-tizonia/blob/master/docker-tizonia


Once the script is in your path, and the permissions of
``$HOME/.config/tizonia`` have been changed, just use the usual Tizonia
commands:

.. code:: bash

   # Change Tizonia's config dir permissions
   $ chmod a+wrx $HOME/.config/tizonia

   # Install the wrapper script in a location in your PATH
   $ sudo install docker-tizonia /usr/local/bin

   # Pass the usual Tizonia commands to the wrapper
   $ docker-tizonia --youtube-audio-mix-search "Queen Official"

Known Issues
------------

``tizonia exiting (OMX_ErrorInsufficientResources)``
****************************************************

Sometimes, the Tizonia instance running in the container is unable to connect
to the PulseAudio (PA) server running on the host.

.. code:: bash

   $ ./docker-tizonia --spotify-album 'Chocolate Starfish & the Hot Dog Flavored Water'
   tizonia 0.22.0. Copyright (C) 2020 Juan A. Rubio and contributors
   This software is part of the Tizonia project <https://tizonia.org>

   [Spotify] [Connecting] : 'e@mail.com'.
   [Spotify] [Album search] 'Chocolate Starfish & the Hot Dog Flavored Water'.
   [Spotify] [Album] 'Chocolate Starfish And The Hot Dog Flavored Water'.
   [Spotify] [Track] [#01] 'Intro' [Limp Bizkit] (1m:19s).
   [Spotify] [Track] [#02] 'Hot Dog' [Limp Bizkit] (3m:50s).
   [Spotify] [Track] [#03] 'My Generation' [Limp Bizkit] (3m:41s) <Explicit>.
   [Spotify] [Track] [#04] 'Full Nelson' [Limp Bizkit] (4m:07s).
   [Spotify] [Track] [#05] 'My Way' [Limp Bizkit] (4m:33s) <Explicit>.
   [Spotify] [Track] [#06] 'Rollin' (Air Raid Vehicle)' [Limp Bizkit] (3m:34s) <Explicit>.
   [Spotify] [Track] [#07] 'Livin' It Up' [Limp Bizkit] (4m:24s) <Explicit>.
   [Spotify] [Track] [#08] 'The One' [Limp Bizkit] (5m:43s).
   [Spotify] [Track] [#09] 'Getcha Groove On - Dirt Road Mix' [Limp Bizkit] (4m:19s) <Explicit>.
   [Spotify] [Track] [#10] 'Take A Look Around' [Limp Bizkit] (5m:21s).
   [Spotify] [Track] [#11] 'It'll Be OK' [Limp Bizkit] (5m:06s).
   [Spotify] [Track] [#12] 'Boiler' [Limp Bizkit] (7m:00s).
   [Spotify] [Track] [#13] 'Hold On' [Limp Bizkit] (5m:48s).
   [Spotify] [Track] [#14] 'Rollin' (Urban Assault Vehicle)' [Limp Bizkit] (6m:23s) <Explicit>.
   [Spotify] [Track] [#15] 'Outro' [Limp Bizkit] (9m:50s).
   [Spotify] [Tracks in queue] '15'.
   [Spotify] [Cache]: '/var/tmp/tizonia-tizonia-spotify-e@mail.com'
   [Spotify] [Login] 'r8eaz0tuw621b2z1kbq41a2a5' logged in.
   [Spotify] [Streaming] : 'Chocolate Starfish & the Hot Dog Flavored Water'.

   tizonia exiting (OMX_ErrorInsufficientResources).

    [OMX.Aratelia.audio_renderer.pulseaudio.pcm:port:0]
    [OMX_ErrorInsufficientResources]


This occurs because Tizonia has not got the necessary access rights to talk to
the PA server. That may happen for a number of reasons, including but not
limited to:

- PA server restarted, and the content of the PA authentication cookie changed
  (``$HOME/.config/pulse/cookie``).
- The location of the PA cookie is not accessible to the Tizonia instance
  (i.e. check the permissions of the path: ``$HOME/.config/pulse/cookie``).


It has been reported that this problem sometimes goes away with one of these
solutions:

- restarting PulseAudio.
- restarting the desktop manager session.
- rebooting the machine.

For further discussion on this issue, see GitHub bug
`tizonia/docker-tizonia/#19 <https://github.com/tizonia/docker-tizonia/issues/19>`_.


.. _tizonia-docker-macos-label:

Mac Support
-----------

Step 1)
*******

It is required that PulseAudio to be installed via homebrew (``brew install
pulseaudio``), and the following lines in
``/usr/local/Cellar/pulseaudio/13.0/etc/pulse/default.pa`` to be uncommented:


.. code-block:: bash

   load-module module-esound-protocol-tcp
   load-module module-native-protocol-tcp


Step 2)
*******

To choose the device being used for output, bring up a list of possible output
devices and select one as the default sink:

.. code-block:: bash

   pactl list short sinks

   pacmd set-default-sink n  # where n is the chosen output number


Step 3)
*******

Start the Pulseaudio daemon:

.. code-block:: bash

   pulseaudio --load=module-native-protocol-tcp --exit-idle-time=-1 --daemon


You should now be able to utilize the ``docker-tizonia`` script to route audio
from the docker container to the host machine!


The `docker-tizonia` GitHub Repository
--------------------------------------

For up-to-date information about Tizonia packaged as a Docker container, please
visit the Dockerfile repository on GitHub:

- https://github.com/tizonia/docker-tizonia
