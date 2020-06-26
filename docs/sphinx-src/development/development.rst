Developer Guide
===============

.. toctree::
   :maxdepth: 2

Components
----------

Familiarize yourself with the various building blocks of the project (**click on the image to enlarge**):

.. uml::
   :caption: The Tizonia Project components
   :scale: 100 %
   :width: 20cm

   @startuml
   package "Tizonia (command-line app)" {
     node "Local Playback" {
       [Local playback FSMs and IL graphs]
     }
     node "Cloud Playback" {
       [Cloud service FSMs and IL graphs]
     }
     node "Chromecast" {
       [Chromecast service FSMs and IL graphs]
     }
     node "Shoutcast/Icecast Client" {
       [HTTP client FSMs and IL graphs]
     }
     node "Shoutcast/Icecast Server" {
       [HTTP server FSMs and IL graphs]
     }
   }

   node "OpenMAX IL 1.2 Subsystem" {
     IL - [IL Core (libtizilcore)]
     [IL Base plugin (libtizonia)] - IL
     IL -left- [IL Resource Manager ]
     IL -right- [OS APIS / Utilities (libtizplatform)]
     [Local playback FSMs and IL graphs] --> IL
     [Cloud service FSMs and IL graphs] --> IL
     [Chromecast service FSMs and IL graphs] -- IL
     [HTTP client FSMs and IL graphs] -- IL
     [HTTP server FSMs and IL graphs] -- IL
   }


   database "OpenMAX IL 1.2 Plugins" {
      [OMX.Aratelia.audio_decoder.opus] - IL_API
      IL_API - [OMX.Aratelia.audio_renderer.pulseaudio.pcm]
      [OMX.Aratelia.audio_decoder.aac] -up- IL_API
      [OMX.Aratelia.audio_source.http] -up- IL_API
      [OMX plugin X ...] -up- IL_API
      [OMX plugin Y ...] -up- IL_API
      IL --> IL_API
   }


   node "Python Proxies (aka clients)" {
     [tizgmusicproxy] - clientAPI_gmusic
     [tizyoutubeproxy] - clientAPI_youtube
     [tizspotifyproxy] - clientAPI_spotify
     [tizplexproxy] - clientAPI_plex
     [tizsoundcloudproxy] - clientAPI_soundcloud
     [tiztuneinproxy] - clientAPI_tunein
     [tizxxxproxy] - clientAPI_xxx
     [OMX.Aratelia.audio_source.http] --> clientAPI_gmusic
     [OMX.Aratelia.audio_source.http] --> clientAPI_youtube
     [OMX.Aratelia.audio_source.http] --> clientAPI_spotify
     [OMX.Aratelia.audio_source.http] --> clientAPI_plex
     [OMX.Aratelia.audio_source.http] --> clientAPI_soundcloud
     [OMX.Aratelia.audio_source.http] --> clientAPI_tunein
     [OMX.Aratelia.audio_source.http] --> clientAPI_xxx
   }

   cloud {
     [Google Play Music] -up-> clientAPI_gmusic
     [Spotify] -up-> clientAPI_spotify
     [YouTube] -up-> clientAPI_youtube
     [Plex] -up-> clientAPI_plex
     [SoundCloud] -up-> clientAPI_soundcloud
     [TuneIn] -up-> clientAPI_tunein
     [ServiceX] -up-> clientAPI_xxx
   }
   @enduml

.. toctree::
   :maxdepth: 2

   PROJECT

Building from Source
--------------------

Follow one of these guides to build and run the codebase locally.

.. toctree::
   :maxdepth: 2

   BUILDING_with_meson
   BUILDING

Coding Guidelines
-----------------

.. toctree::
   :maxdepth: 2

   style

Writting Documentation
----------------------

.. toctree::
   :maxdepth: 2

   documenting

API Documentation
-----------------

.. toctree::
   :maxdepth: 1

   openmaxil/openmaxil
   plugins/plugins
   clients/clients

Debugging Tools
---------------

.. toctree::
   :maxdepth: 2

   logging
   envvars
