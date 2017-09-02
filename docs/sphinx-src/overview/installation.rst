.. Tizonia documentation


Installation
============

Debian/Ubuntu packages
----------------------

.. note:: For the latest installation information, please visit the `README.md
   <https://github.com/tizonia/tizonia-openmax-il/blob/master/tools/install.sh>`_
   file on GitHub.

Debian/Ubuntu packages are available from `Bintray
<https://bintray.com/tizonia>`_ for the following distro/arch combinations:

* `Ubuntu Trusty <https://bintray.com/tizonia/ubuntu/tizonia-trusty/view>`_ (14.04)

  * amd64, i386, armhf

* `Ubuntu Vivid <https://bintray.com/tizonia/ubuntu/tizonia-vivid/view>`_ (15.04)

  * amd64, i386, armhf

* `Debian Jessie <https://bintray.com/tizonia/debian/tizonia-jessie/view>`_ (8)

  * amd64, i386, armhf, armel

* `Raspbian Jessie <https://bintray.com/tizonia/raspbian/tizonia-jessie/view>`_ (8)

  * armhf

* `Debian Stretch <https://bintray.com/tizonia/debian/tizonia-stretch/view>`_ (9)

  * amd64, i386, armhf, armel

* `Raspbian Stretch <https://bintray.com/tizonia/raspbian/tizonia-stretch/view>`_ (9)

  * armhf


.. note:: Elementary OS and Linux Mint are supported on releases based on
   Ubuntu 'Trusty' or Ubuntu 'Xenial'.


Installing the latest release
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To conveniently install the latest release of the Debian packages, and all
their dependencies, use the ``install.sh`` script:

.. code-block:: bash

   $ curl -kL https://github.com/tizonia/tizonia-openmax-il/raw/master/tools/install.sh | bash
   # or it's shortened version
   $ curl -L -O https://goo.gl/Vu8qGR && chmod +x Vu8qGR && ./Vu8qGR

.. note:: The `install.sh
          <https://github.com/tizonia/tizonia-openmax-il/blob/master/tools/install.sh>`_
          script is hosted on GitHub, and displayed here for clarity:

.. literalinclude:: ../../../tools/install.sh
    :linenos:
    :language: bash

Arch User Repository (AUR) packages
-----------------------------------

- `tizonia-all (0.9.0) <https://aur.archlinux.org/packages/tizonia-all/>`_
- `tizonia-all-git <https://aur.archlinux.org/packages/tizonia-all-git/>`_

.. code-block:: bash

   $ yaourt -S tizonia-all # for the latest released version
   # or
   $ yaourt -S tizonia-all-git # for the bleeding edge

