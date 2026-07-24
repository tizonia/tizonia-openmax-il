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

   $ ./docker-tizonia --youtube-audio-mix-search "Queen Official"
   tizonia 0.22.0. Copyright (C) 2020 Juan A. Rubio and contributors
   This software is part of the Tizonia project <https://tizonia.org>

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
