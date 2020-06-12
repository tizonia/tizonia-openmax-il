Snap Package
============

In addition to Debian packages, Tizonia is also available as a Snap package.


SNAP STORE
----------

The official Snap package of Tizonia is available from the Snap Store, for the
following architectures:

- amd64
- armhf
- i386

.. raw:: html

   <iframe
   src="https://snapcraft.io/tizonia/embedded?button=black&channels=true&summary=true&screenshot=true"
   frameborder="0" width="100%" height="1100px" style="background: #FFFFFF;
   border: 1px solid #CCC; border-radius: 2px;"></iframe>


Configuration
-------------

Snap packages are a bit different, the ``$HOME`` directory of a snap package is
under ``$HOME/snap/<package-name>/current/``.  Therefore, when the Snap of
Tizonia is installed, Tizonia's configuration file needs to be found under:

- ``$HOME/snap/tizonia/current/.config/tizonia/tizonia.conf``

.. note:: Tizonia writes its configuration file on first run. That means, you need to run
          Tizonia at least once after installation, in order for ``tizonia.conf`` to be
          populated in its directory. After that, you can include there your login
          information for the various services supported by Tizonia. See
          :ref:`tizonia-config-label` under the ``[tizonia]`` section.

.. note:: Note that if you simply copy your existing ``tizonia.conf`` from a
          previous installation of Tizonia, i.e. from
          ``$HOME/.config/tizonia/tizonia.conf``, Tizonia installed as a snap
          will not work. This is because the ``component-paths`` directory is
          different in Tizonia Snap installations and Tizonia Debian
          installations. For the Snap package to work, your ``tizonia.conf``
          needs to have the following content in that section of the config
          file:

.. code-block:: bash

   # Component plugins discovery
   # -------------------------------------------------------------------------
   # A comma-separated list of paths to be scanned by the Tizonia IL Core when
   # searching for component plugins
   component-paths = $TIZONIA_PLUGINS_DIR


The `tizonia-snap` GitHub Repository
------------------------------------

For more information about Tizonia packaged as a Snap, please visit the package
repository on GitHub:

- https://github.com/tizonia/tizonia-snap
