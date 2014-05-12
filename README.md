Tizonia OpenMAX IL
==================

An experimental implementation for Linux of the OpenMAX IL 1.2 provisional
specification.

[![Build Status](https://travis-ci.org/tizonia/tizonia-openmax-il.png)](https://travis-ci.org/tizonia/tizonia-openmax-il)

_This is a highly experimental, rapidly-changing project. APIs might change overnight._

Tizonia OpenMAX IL consists of a number of resources:

* An almost complete implementation of a base OpenMAX IL 1.2 component library
  (with support for both Base and Interop profiles)
* An IL Core implementation.
* An OS abstraction/utility library, with wrappers and utilities for
    * memory allocation,
    * threading and synchronization primitives,
    * FIFO and priority queues,
    * dynamic arrays,
    * associative arrays,
    * small object allocation,
    * HTTP parsing,
    * uuids,
    * config file parsing,
    * Evented I/O (via libev)
    * etc
* A Resource Management (RM) framework, including
  * a C client library,
  * and a D-Bus-based RM server written in C++.
* A growing number of OpenMAX IL plugin implementations, including:
  * a mp3 decoder (based on libmad),
  * a mp3 encoder (based on LAME),
  * a PCM renderer (based on ALSA lib)
  * a http renderer (ala icecast)
  * an ogg demuxer (based on liboggz)
  * an opus decoder (based on libopus)
  * a flac decoder (based on libflac)
  * a vorbis decoder (based on libfishsound)
  * a Vp8 decoder (based on libvpx),
  * a SDL-based YUV renderer
  * binary file readers and writers
  * etc...
* `tplay`
  * A command line application that reproduces or streams a number of audio
    formats (mp3, vorbis, opus, flac) using graphs of tunneled OpenMAX IL
    components.

Tizonia OpenMAX IL is released under the GNU Lesser General Public License
version 3.

For more information, please visit the project web site at http://tizonia.org

