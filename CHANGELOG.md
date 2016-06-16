# Change Log

## [v0.5.0](https://github.com/tizonia/tizonia-openmax-il/tree/v0.5.0) (2016-06-16)
[Full Changelog](https://github.com/tizonia/tizonia-openmax-il/compare/v0.4.0...v0.5.0)

**Improvements:**

- release 0.5.0 [\#247](https://github.com/tizonia/tizonia-openmax-il/issues/247)
- tizgmusic: play a random item from the user library or Google Music catalogue when search results are empty [\#246](https://github.com/tizonia/tizonia-openmax-il/issues/246)
- tizgmusic: add "situations" search support [\#245](https://github.com/tizonia/tizonia-openmax-il/issues/245)

**Fixed bugs:**

- tizgmusic: broken after 'gmusicapi' renamed the search method [\#244](https://github.com/tizonia/tizonia-openmax-il/issues/244)

**Closed issues:**

- release v0.4.0 [\#159](https://github.com/tizonia/tizonia-openmax-il/issues/159)

## [v0.4.0](https://github.com/tizonia/tizonia-openmax-il/tree/v0.4.0) (2016-05-04)
[Full Changelog](https://github.com/tizonia/tizonia-openmax-il/compare/v0.3.0...v0.4.0)

**Improvements:**

- release v0.4.0 [\#243](https://github.com/tizonia/tizonia-openmax-il/issues/243)
- tizgmusicproxy: station search : If no suitable station is found in the user's library, then search google play unlimited for a potential match [\#158](https://github.com/tizonia/tizonia-openmax-il/issues/158)
- tizonia-player: add Dirble support [\#123](https://github.com/tizonia/tizonia-openmax-il/issues/123)
- plugins: Dirble open radio source component [\#122](https://github.com/tizonia/tizonia-openmax-il/issues/122)

**Fixed bugs:**

- Coverity Scan \(CID 1352374\) \[/player/src/services/dirble/tizdirblegraphops.cpp:dirbleops\] : Uninitialized scalar field [\#241](https://github.com/tizonia/tizonia-openmax-il/issues/241)
- Coverity Scan \(CID 1352373\) \[/player/src/services/googlemusic/tizgmusicgraphops.cpp:gmusicops\] : Uninitialized scalar field [\#240](https://github.com/tizonia/tizonia-openmax-il/issues/240)
- Coverity Scan \(CID 1352372\) \[/player/src/services/soundcloud/tizscloudgraphops.cpp:scloudops\] : Uninitialized scalar field [\#239](https://github.com/tizonia/tizonia-openmax-il/issues/239)
- Coverity Scan \(CID 1352371\) \[/player/src/services/spotify/tizspotifygraphops.cpp:spotifyops\] : Uninitialized scalar field [\#238](https://github.com/tizonia/tizonia-openmax-il/issues/238)
- Coverity Scan \(CID 1352369\) \[/player/src/tizprobe.cpp:probe\] : Uninitialized scalar field [\#236](https://github.com/tizonia/tizonia-openmax-il/issues/236)
- Coverity Scan \(CID 1352365\) \[/player/src/tizdaemon.cpp:daemonize\] : Resource leak [\#233](https://github.com/tizonia/tizonia-openmax-il/issues/233)
- Coverity Scan \(CID 1352363\) \[/libtizplatform/src/tizev.c:init\_event\_loop\_thread\] : Resource leak [\#231](https://github.com/tizonia/tizonia-openmax-il/issues/231)
- Coverity Scan \(CID 1352362\) \[/player/src/tizgraphutil.cpp:set\_content\_uri\] : Resource leak [\#230](https://github.com/tizonia/tizonia-openmax-il/issues/230)
- Coverity Scan \(CID 1352361\) \[/plugins/http\_renderer/src/httprsrv.c:srv\_accept\_socket\] : Resource leak [\#229](https://github.com/tizonia/tizonia-openmax-il/issues/229)
- Coverity Scan \(CID 1087251\) \[/libtizplatform/src/tizsync.c:tiz\_cond\_init\] : Resource leak [\#227](https://github.com/tizonia/tizonia-openmax-il/issues/227)
- Coverity Scan \(CID 1087250\) \[/libtizplatform/src/tizsync.c:tiz\_mutex\_init\] : Resource leak [\#226](https://github.com/tizonia/tizonia-openmax-il/issues/226)
- Coverity Scan \(CID 1087249\) \[/libtizplatform/src/tizsync.c:tiz\_sem\_init\] : Resource leak [\#225](https://github.com/tizonia/tizonia-openmax-il/issues/225)
- Coverity Scan \(CID 1087248\) \[/libtizcore/src/tizcore.c:find\_component\_paths\] : Resource leak [\#224](https://github.com/tizonia/tizonia-openmax-il/issues/224)
- Coverity Scan \(CID 1087247\) \[/libtizcore/src/tizcore.c:scan\_component\_folders\] : Resource leak [\#223](https://github.com/tizonia/tizonia-openmax-il/issues/223)
- Coverity Scan \(CID 1087246\) \[/libtizcore/src/tizcore.c:instantiate\_component\] : Resource leak [\#222](https://github.com/tizonia/tizonia-openmax-il/issues/222)
- Coverity Scan \(CID 1352366\) \[/plugins/pcm\_renderer\_pa/src/pulsearprc.c:pulseaudio\_stream\_state\_cback\_handler\] : Dereference before null check [\#214](https://github.com/tizonia/tizonia-openmax-il/issues/214)
- Coverity Scan \(CID 1352348\) \[/player/src/httpserv/tizhttpservgraphfsm.hpp:operator \(\)\] : Unchecked dynamic\_cast [\#213](https://github.com/tizonia/tizonia-openmax-il/issues/213)
- Coverity Scan \(CID 1352347\) \[/player/src/httpserv/tizhttpservgraphfsm.hpp:operator \(\)\] : Unchecked dynamic\_cast [\#212](https://github.com/tizonia/tizonia-openmax-il/issues/212)
- Coverity Scan \(CID 1352346\) \[/player/src/httpserv/tizhttpservgraphfsm.hpp:operator \(\)\] : Unchecked dynamic\_cast [\#211](https://github.com/tizonia/tizonia-openmax-il/issues/211)
- Coverity Scan \(CID 1352345\) \[/player/src/httpserv/tizhttpservgraphfsm.hpp:operator \(\)\] : Unchecked dynamic\_cast [\#210](https://github.com/tizonia/tizonia-openmax-il/issues/210)
- Coverity Scan \(CID 1352344\) \[/player/src/httpserv/tizhttpservgraphfsm.hpp:operator \(\)\] : Unchecked dynamic\_cast [\#209](https://github.com/tizonia/tizonia-openmax-il/issues/209)
- Coverity Scan \(CID 1352343\) \[/player/src/httpserv/tizhttpservgraphfsm.hpp:operator \(\)\] : Unchecked dynamic\_cast [\#208](https://github.com/tizonia/tizonia-openmax-il/issues/208)
- Coverity Scan \(CID 1352342\) \[/player/src/httpserv/tizhttpservgraphfsm.hpp:operator \(\)\] : Unchecked dynamic\_cast [\#207](https://github.com/tizonia/tizonia-openmax-il/issues/207)
- Coverity Scan \(CID 1352341\) \[/player/src/httpserv/tizhttpservgraphfsm.hpp:operator \(\)\] : Unchecked dynamic\_cast [\#206](https://github.com/tizonia/tizonia-openmax-il/issues/206)
- Coverity Scan \(CID 1352378\) \[/player/src/tizgraphcmd.cpp:c\_str\] : Wrapper object use after free [\#204](https://github.com/tizonia/tizonia-openmax-il/issues/204)
- Coverity Scan \(CID 1352359\) \[/player/src/tizgraphutil.cpp:set\_role\] : Out-of-bounds write [\#203](https://github.com/tizonia/tizonia-openmax-il/issues/203)
- Coverity Scan \(CID 1352355\) \[/player/src/tizgraph.cpp:omx\_evt\] : Inferred misuse of enum [\#191](https://github.com/tizonia/tizonia-openmax-il/issues/191)
- http\_renderer: Google Music, SoundClound playback does not work in Ubuntu 16.04  [\#242](https://github.com/tizonia/tizonia-openmax-il/issues/242)
- tizmp3dec: OMX\_EventPortSettingsChanged not signalled correclty with mono streams [\#157](https://github.com/tizonia/tizonia-openmax-il/issues/157)

## [v0.3.0](https://github.com/tizonia/tizonia-openmax-il/tree/v0.3.0) (2015-12-24)
[Full Changelog](https://github.com/tizonia/tizonia-openmax-il/compare/v0.2.0...v0.3.0)

**Improvements:**

- release v0.3.0 [\#156](https://github.com/tizonia/tizonia-openmax-il/issues/156)
- libtizplatform: add 'tiz\_event\_timer\_is\_repeat' to know if a timer is repeat or not [\#153](https://github.com/tizonia/tizonia-openmax-il/issues/153)

**Fixed bugs:**

- pcm\_renderer\_alsa: EOS should be signalled after all frames in the last buffer are consumed by ALSA [\#155](https://github.com/tizonia/tizonia-openmax-il/issues/155)
- tizmp3dec: component does not always report EOS [\#154](https://github.com/tizonia/tizonia-openmax-il/issues/154)
- libtizonia: non-repeat timers are not removed from the tizservant's watcher map   [\#152](https://github.com/tizonia/tizonia-openmax-il/issues/152)
- tizhttpsrc: soundcloud gapless playback [\#151](https://github.com/tizonia/tizonia-openmax-il/issues/151)
- tizhttpsrc: google play music gapless playback [\#150](https://github.com/tizonia/tizonia-openmax-il/issues/150)
- tizspotifysrc: playback stalls when 'container\_loaded' is called by spotify [\#149](https://github.com/tizonia/tizonia-openmax-il/issues/149)
- tizspotifysrc: playlists are not found in the user library [\#148](https://github.com/tizonia/tizonia-openmax-il/issues/148)

## [v0.2.0](https://github.com/tizonia/tizonia-openmax-il/tree/v0.2.0) (2015-12-14)
[Full Changelog](https://github.com/tizonia/tizonia-openmax-il/compare/v0.1.0...v0.2.0)

**Improvements:**

- tizspotifysrc: playlist name matching improvements [\#146](https://github.com/tizonia/tizonia-openmax-il/issues/146)
- alsa\_renderer: set a custom ALSA error handler to dump error info to tizonia's log file [\#145](https://github.com/tizonia/tizonia-openmax-il/issues/145)
- release v0.2.0 [\#141](https://github.com/tizonia/tizonia-openmax-il/issues/141)
- Simplybuilt website [\#140](https://github.com/tizonia/tizonia-openmax-il/issues/140)
- CI: migrate to Travis' Ubuntu 14.04 image [\#138](https://github.com/tizonia/tizonia-openmax-il/issues/138)
- libtizplatform: update http-parser to v2.6.0 [\#137](https://github.com/tizonia/tizonia-openmax-il/issues/137)
- libtizplatform: update utarray to v1.9.9 [\#136](https://github.com/tizonia/tizonia-openmax-il/issues/136)
- tools: add a dev build script [\#133](https://github.com/tizonia/tizonia-openmax-il/issues/133)
- google music: support logging in via an auth token [\#132](https://github.com/tizonia/tizonia-openmax-il/issues/132)
- docs: update Sphinx config to integrate 'breathe' and allow Doxygen output in Read the Docs  [\#127](https://github.com/tizonia/tizonia-openmax-il/issues/127)
- tizonia-player: add SoundCloud support [\#121](https://github.com/tizonia/tizonia-openmax-il/issues/121)
- libtizonia: support for OMX\_UseEGLImage [\#118](https://github.com/tizonia/tizonia-openmax-il/issues/118)
- Improved, more straightforward command line '--help' option [\#117](https://github.com/tizonia/tizonia-openmax-il/issues/117)
- tizonia-player: improve compile times by firewalling the decoder graph fsm [\#115](https://github.com/tizonia/tizonia-openmax-il/issues/115)
- SoundCloud streaming source component [\#111](https://github.com/tizonia/tizonia-openmax-il/issues/111)

**Fixed bugs:**

- mp3\_decoder: output buffer size is too small and causes too many exchanges with the renderer [\#110](https://github.com/tizonia/tizonia-openmax-il/issues/110)
- http\_source sometimes stalls when curl instructs to unregister the socket  [\#109](https://github.com/tizonia/tizonia-openmax-il/issues/109)
- tizspotifysrc: playlist name matching improvements [\#146](https://github.com/tizonia/tizonia-openmax-il/issues/146)
- tizplatform: tiz\_queue uses internally TIZ\_QUEUE\_MAX\_ITEMS instead of the 'capacity' argument provided [\#144](https://github.com/tizonia/tizonia-openmax-il/issues/144)
- tizspotifysrc: don't allow the component's main event queue get full \(which leads to component stalling or crashing\)  [\#143](https://github.com/tizonia/tizonia-openmax-il/issues/143)
- tizspotifysrc: fix various race conditions causing cracking sound problems [\#142](https://github.com/tizonia/tizonia-openmax-il/issues/142)
- build system: remove warnings [\#139](https://github.com/tizonia/tizonia-openmax-il/issues/139)
- libtizplatform: rework tizev to dispatch event start/stop/destroy from the async watcher callback [\#135](https://github.com/tizonia/tizonia-openmax-il/issues/135)
- libtizplatform: increase line limit in config file parser utility  [\#134](https://github.com/tizonia/tizonia-openmax-il/issues/134)
- tizgmusicproxy: \(UnicodeEncodeError\) : 'ascii' codec can't encode character in 'tizonia --gmusic-unlimited-album' "Las 100 mejores canciones del Rock español" [\#130](https://github.com/tizonia/tizonia-openmax-il/issues/130)
- tizgmusicproxy: tizonia --gmusic-unlimited-album : IndexError: list index out of range [\#129](https://github.com/tizonia/tizonia-openmax-il/issues/129)
- docs: fix build system under 'docs' folder to properly build doxygen docs [\#126](https://github.com/tizonia/tizonia-openmax-il/issues/126)
- google play music: replace references to 'All-access' with 'Unlimited' [\#120](https://github.com/tizonia/tizonia-openmax-il/issues/120)
- tizgmusicproxy: unicode to ASCII without errors [\#119](https://github.com/tizonia/tizonia-openmax-il/issues/119)

## [v0.1.0](https://github.com/tizonia/tizonia-openmax-il/tree/v0.1.0) (2015-09-28)
[Full Changelog](https://github.com/tizonia/tizonia-openmax-il/compare/0.1.0...v0.1.0)

**Improvements:**

- plugins: Google Music streaming client component [\#106](https://github.com/tizonia/tizonia-openmax-il/issues/106)
- tizonia: remove dependency on libav libraries [\#103](https://github.com/tizonia/tizonia-openmax-il/issues/103)
- plugins: opus decoder based on libopusfile [\#96](https://github.com/tizonia/tizonia-openmax-il/issues/96)
- plugins: libspotify component [\#94](https://github.com/tizonia/tizonia-openmax-il/issues/94)
- tplay: implement a proper command line interface [\#87](https://github.com/tizonia/tizonia-openmax-il/issues/87)
- libtizonia: add basic support for metadata extraction [\#85](https://github.com/tizonia/tizonia-openmax-il/issues/85)
- audio\_renderer\_nb: rename component as "audio\_renderer" [\#79](https://github.com/tizonia/tizonia-openmax-il/issues/79)
- audio\_renderer: remove deprecated component from the repo [\#78](https://github.com/tizonia/tizonia-openmax-il/issues/78)
- http\_renderer: rename ice\_renderer =\> to http\_renderer [\#76](https://github.com/tizonia/tizonia-openmax-il/issues/76)
- plugins: move skeletons/work-in-progress components to their own repo [\#75](https://github.com/tizonia/tizonia-openmax-il/issues/75)
- libtizplatform: add a buffer handling utility [\#74](https://github.com/tizonia/tizonia-openmax-il/issues/74)
- plugins: add an AAC audio decoder \(based on libfaad2\) [\#73](https://github.com/tizonia/tizonia-openmax-il/issues/73)
- libtizonia: add AAC audio port [\#72](https://github.com/tizonia/tizonia-openmax-il/issues/72)
- vorbis\_decoder: migrate it to use the new filterprc class [\#71](https://github.com/tizonia/tizonia-openmax-il/issues/71)
- libtizonia: add a specialised processor class for filters [\#70](https://github.com/tizonia/tizonia-openmax-il/issues/70)
- plugins: replace mp3 decoder unit tests with Skema test suites [\#68](https://github.com/tizonia/tizonia-openmax-il/issues/68)
- il\_core: allow initialisation of the logging system via environment variable [\#67](https://github.com/tizonia/tizonia-openmax-il/issues/67)
- tizonia: MPRIS D-Bus Interface support [\#66](https://github.com/tizonia/tizonia-openmax-il/issues/66)
- build system: debian packaging [\#64](https://github.com/tizonia/tizonia-openmax-il/issues/64)
- plugins: Multi-format sampled audio file decoder \(based on libsndfile\) [\#61](https://github.com/tizonia/tizonia-openmax-il/issues/61)
- plugins: Pulseaudio pcm renderer \(based on PA async API\) [\#58](https://github.com/tizonia/tizonia-openmax-il/issues/58)
- plugins: SHOUTcast/ICEcast streaming client [\#55](https://github.com/tizonia/tizonia-openmax-il/issues/55)
- tplay: filter out non-CBR streams [\#46](https://github.com/tizonia/tizonia-openmax-il/issues/46)
- tplay: add option to filter out certain sampling rates in http streaming [\#45](https://github.com/tizonia/tizonia-openmax-il/issues/45)
- tizrmd: Rename .cc files to .cpp and .h to .hpp [\#40](https://github.com/tizonia/tizonia-openmax-il/issues/40)
- tplay: remove from 'examples'; move to the top of the repo [\#38](https://github.com/tizonia/tizonia-openmax-il/issues/38)
- Rename libtizosal to libtizplatform [\#37](https://github.com/tizonia/tizonia-openmax-il/issues/37)
- plugins: libmpg123 \(LGPL\) based mp3 decoder component [\#28](https://github.com/tizonia/tizonia-openmax-il/issues/28)
- libtizonia: Re-factor OMX\_IndexParamContentURI get/set logic [\#24](https://github.com/tizonia/tizonia-openmax-il/issues/24)
- Vorbis decoder component [\#20](https://github.com/tizonia/tizonia-openmax-il/issues/20)
- libtizonia: add a Vorbis port [\#19](https://github.com/tizonia/tizonia-openmax-il/issues/19)
- tplay: Add support for multi-format plalists \(a.k.a. the graph manager thread\) [\#18](https://github.com/tizonia/tizonia-openmax-il/issues/18)
- FLAC decoder component [\#17](https://github.com/tizonia/tizonia-openmax-il/issues/17)
- FLAC port [\#16](https://github.com/tizonia/tizonia-openmax-il/issues/16)
- Object system rework \("class" objects must be freed when the component is unloaded\) [\#15](https://github.com/tizonia/tizonia-openmax-il/issues/15)
- opus decoder component [\#11](https://github.com/tizonia/tizonia-openmax-il/issues/11)
- plugins: ogg demuxer component [\#10](https://github.com/tizonia/tizonia-openmax-il/issues/10)
- Demuxer config port [\#9](https://github.com/tizonia/tizonia-openmax-il/issues/9)
- Demuxer port [\#8](https://github.com/tizonia/tizonia-openmax-il/issues/8)
- PCM renderer component based on non-blocking ALSA apis  [\#7](https://github.com/tizonia/tizonia-openmax-il/issues/7)
- Implement pause/resume in processor class [\#6](https://github.com/tizonia/tizonia-openmax-il/issues/6)
- Add API for notification of SetConfig\(OMX\_IndexConfig..\) events to processor implementations [\#1](https://github.com/tizonia/tizonia-openmax-il/issues/1)

**Fixed bugs:**

- tizonia: remove dependency on libav libraries [\#103](https://github.com/tizonia/tizonia-openmax-il/issues/103)
- Move ~/.tizonia.conf ~/.config/tizonia/tizonia.conf, which is the XDG-compliant path [\#100](https://github.com/tizonia/tizonia-openmax-il/issues/100)
- tplay: core dump \(abort\) when connecting to a server with invalid/unhandled audio stream [\#99](https://github.com/tizonia/tizonia-openmax-il/issues/99)
- tplay: 'DBus::Error' exception causes core dump when the DBUS .service file is missing [\#98](https://github.com/tizonia/tizonia-openmax-il/issues/98)
- tplay: fix support of ogg opus decoding [\#97](https://github.com/tizonia/tizonia-openmax-il/issues/97)
- tplay: need to make file uris canonical \(i.e. absolute\) before daemonizing [\#93](https://github.com/tizonia/tizonia-openmax-il/issues/93)
- alsa audio renderer: need to reset the input header nOffset field before they are released [\#92](https://github.com/tizonia/tizonia-openmax-il/issues/92)
- mad mp3 decoder: component does not reset stream-related parameters before returning to OMX\_StateExecuting [\#91](https://github.com/tizonia/tizonia-openmax-il/issues/91)
- file reader: component does not reset stream-related parameters before returning to OMX\_StateExecuting [\#90](https://github.com/tizonia/tizonia-openmax-il/issues/90)
- libtizonia: kernel should clear output headers before they are forwarded to the processor object [\#89](https://github.com/tizonia/tizonia-openmax-il/issues/89)
- tplay: SIGSEGV in the IL RM proxy thread when closing the application [\#88](https://github.com/tizonia/tizonia-openmax-il/issues/88)
- pcm\_renderer\_alsa: no sound after port disable-\>enable sequence while in Executing state [\#86](https://github.com/tizonia/tizonia-openmax-il/issues/86)
- libtizonia: Port enable event should be received at the processor only after the port is fully populated. [\#84](https://github.com/tizonia/tizonia-openmax-il/issues/84)
- libtizonia: tizport.c:port\_depopulate asserts in Executing state if there are no buffers allocated. [\#83](https://github.com/tizonia/tizonia-openmax-il/issues/83)
- libtizonia: tiz\_port\_set\_portdef\_format is overriden in the leaf audio ports. [\#82](https://github.com/tizonia/tizonia-openmax-il/issues/82)
- http\_renderer: EPIPE should not be considered a "recoverable" error [\#81](https://github.com/tizonia/tizonia-openmax-il/issues/81)
- libtizplatform: allow null handles passed to tiz\_event\_io\_destroy and tiz\_event\_timer\_destroy [\#80](https://github.com/tizonia/tizonia-openmax-il/issues/80)
- mp3\_decoder: re-license plugin from LGPL to GPL \(libmad is GPL\) [\#77](https://github.com/tizonia/tizonia-openmax-il/issues/77)
- vorbis\\_decoder: migrate it to use the new filterprc class [\#71](https://github.com/tizonia/tizonia-openmax-il/issues/71)
- libtizonia: add a specialised processor class for filters [\#70](https://github.com/tizonia/tizonia-openmax-il/issues/70)
- libtizonia: processor does not complete transition Idle-\>Pause [\#69](https://github.com/tizonia/tizonia-openmax-il/issues/69)
- il\\_core: allow initialisation of the logging system via environment variable [\#67](https://github.com/tizonia/tizonia-openmax-il/issues/67)
- build system: debian packaging [\#64](https://github.com/tizonia/tizonia-openmax-il/issues/64)
- tplay: add option to configure streaming of VBR and or CBR mp3 streams [\#54](https://github.com/tizonia/tizonia-openmax-il/issues/54)
- tplay: allow per-process log4c config files and output log files. [\#53](https://github.com/tizonia/tizonia-openmax-il/issues/53)
- tplay: replace glibc daemon\(\) function with own implementation [\#52](https://github.com/tizonia/tizonia-openmax-il/issues/52)
- mp3\_metadata: processor does not release buffers correctly [\#51](https://github.com/tizonia/tizonia-openmax-il/issues/51)
- tplay: http server graph fails to configure the station correctly when the first song is non-CBR [\#50](https://github.com/tizonia/tizonia-openmax-il/issues/50)
- tplay: add option to configure radio station genre [\#49](https://github.com/tizonia/tizonia-openmax-il/issues/49)
- http\_renderer: it dies with signal SIGPIPE when the other end breaks the connection [\#48](https://github.com/tizonia/tizonia-openmax-il/issues/48)
- tplay: will crash if the playlist is exhausted [\#47](https://github.com/tizonia/tizonia-openmax-il/issues/47)
- http\_renderer: ICY metadata info is out of sync after the first song [\#44](https://github.com/tizonia/tizonia-openmax-il/issues/44)
- tplay: remove metadata from streamed mp3 files [\#43](https://github.com/tizonia/tizonia-openmax-il/issues/43)
- tplay: the second instance of tplay -s hangs while trying to open the default tcp port [\#42](https://github.com/tizonia/tizonia-openmax-il/issues/42)
- tplay: hangs when OMX\_ErrorInsufficientResources is received in STATE \[executing\] [\#41](https://github.com/tizonia/tizonia-openmax-il/issues/41)
- tplay: Require Boost \>= 1.54 in configure.ac [\#39](https://github.com/tizonia/tizonia-openmax-il/issues/39)
- tplay: graph has no transition from \[pause\] on \[unload\_evt\] [\#36](https://github.com/tizonia/tizonia-openmax-il/issues/36)
- libtizonia: scheduler never destroys the object system [\#35](https://github.com/tizonia/tizonia-openmax-il/issues/35)
- tplay: the program hangs during graph loading if a component is missing [\#34](https://github.com/tizonia/tizonia-openmax-il/issues/34)
- tplay: use ax\_boost m4 macros [\#33](https://github.com/tizonia/tizonia-openmax-il/issues/33)
- libtizonia: Deprecate tiz\_krn\_select and friends [\#32](https://github.com/tizonia/tizonia-openmax-il/issues/32)
- tplay: "tplay -s ." crashes at the end of the playlist [\#31](https://github.com/tizonia/tizonia-openmax-il/issues/31)
- tplay: http server graph fsm does not need transitions for seek, volume, mute and pause events. [\#30](https://github.com/tizonia/tizonia-openmax-il/issues/30)
- tplay: Rename .cc files to .cpp and .h to .hpp [\#29](https://github.com/tizonia/tizonia-openmax-il/issues/29)
- tplay: playlist should allow removal of files with unknown codec [\#27](https://github.com/tizonia/tizonia-openmax-il/issues/27)
- http renderer: processor doesn't flush its buffers [\#26](https://github.com/tizonia/tizonia-openmax-il/issues/26)
- tplay hangs after "OMX\_ErrorContentURIError : Unable to probe uri" in do\_probe [\#25](https://github.com/tizonia/tizonia-openmax-il/issues/25)
- tplay: Backguard skip functionality is broken in multi-format playlists [\#23](https://github.com/tizonia/tizonia-openmax-il/issues/23)
- travis-ci: add libfishsound-dev as project dependency [\#22](https://github.com/tizonia/tizonia-openmax-il/issues/22)
- tplay, plugins: path limits are not handled correctly [\#21](https://github.com/tizonia/tizonia-openmax-il/issues/21)
- Object system rework \\("class" objects must be freed when the component is unloaded\\) [\#15](https://github.com/tizonia/tizonia-openmax-il/issues/15)

**Closed issues:**

- libtizonia: async events need a unique id to avoid processing duplicates in the component thread [\#102](https://github.com/tizonia/tizonia-openmax-il/issues/102)
- libtizonia: automatically reset OMX\_BUFFERFLAG\_EOS on input buffers [\#101](https://github.com/tizonia/tizonia-openmax-il/issues/101)
- Implement the shoutcast metadata protocol in the http\_renderer's streaming server [\#4](https://github.com/tizonia/tizonia-openmax-il/issues/4)

**Merged pull requests:**

- Add a Bitdeli Badge to README [\#5](https://github.com/tizonia/tizonia-openmax-il/pull/5) ([bitdeli-chef](https://github.com/bitdeli-chef))



\* *This Change Log was automatically generated by [github_changelog_generator](https://github.com/skywinder/Github-Changelog-Generator)*