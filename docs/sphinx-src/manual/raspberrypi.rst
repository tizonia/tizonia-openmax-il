Raspberry Pi
============

There are Tizonia binaries for Raspbian. All versions of Raspberry Pi are
supported. Select the ``OMX.Aratelia.audio_renderer.alsa.pcm`` renderer as the
default audio renderer in ``tizonia.conf``.


CONFIGURATION
-------------

.. code-block:: bash

   [tizonia]
   # Tizonia player section

   # The default audio renderer used by the tizonia player
   # -------------------------------------------------------------------------
   # Valid values are:
   # - OMX.Aratelia.audio_renderer.pulseaudio.pcm
   # - OMX.Aratelia.audio_renderer.alsa.pcm
   default-audio-renderer = OMX.Aratelia.audio_renderer.alsa.pcm

This can also be done with this one-liner:

.. code-block:: bash

    # Update tizonia's config
    perl -pi -e 's/^(default-audio-renderer.*)pulseaudio/\1alsa/' $HOME/.config/tizonia/tizonia.conf

Tizonia on a headless Raspberry Pi
----------------------------------

It is posible to run Tizonia on a headless Raspberry Pi (e.g. a Raspberry Pi
Zero with Raspbian Lite).

Tizonia has a dependency against D-Bus but normally, the D-Bus daemon is not
even started without X11. In this case, it is necessary to make sure that the
D-Bus daemon has been started prior to using Tizonia.

See this `recipe <https://pastebin.com/6jSTHQ2M>`_ for an idea on how to launch
the D-Bus daemon on your headless Raspberry Pi.
