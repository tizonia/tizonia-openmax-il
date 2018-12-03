# Change Log

## [v0.16.0](https://github.com/tizonia/tizonia-openmax-il/tree/v0.16.0) (2018-12-03)
[Full Changelog](https://github.com/tizonia/tizonia-openmax-il/compare/v0.15.0...v0.16.0)

**Improvements:**

- Spotify: add options to play Spotify recommendations using a track id, artist id, or genre name as seeds [\#536](https://github.com/tizonia/tizonia-openmax-il/issues/536)
- Spotify: URIs for track, artist, and album displayed on terminal [\#535](https://github.com/tizonia/tizonia-openmax-il/issues/535)
- Spotify: add option to force playing the next song after token is lost [\#534](https://github.com/tizonia/tizonia-openmax-il/issues/534)
- Spotify: add option to play an album from Spotify's new releases list [\#532](https://github.com/tizonia/tizonia-openmax-il/issues/532)
- Spotify: add option to search and play a featured playlist [\#529](https://github.com/tizonia/tizonia-openmax-il/issues/529)
- Spotify: add option to play top tracks from a number of similar artists [\#528](https://github.com/tizonia/tizonia-openmax-il/issues/528)
- Spotify: add options for playback by Spotify ID, URI, or URL [\#527](https://github.com/tizonia/tizonia-openmax-il/issues/527)
- Spotify: add fuzzy search capability to --spotify-artist [\#526](https://github.com/tizonia/tizonia-openmax-il/issues/526)
- Spotify: hide spotify debug messages [\#525](https://github.com/tizonia/tizonia-openmax-il/issues/525)
- Kali Linux support [\#510](https://github.com/tizonia/tizonia-openmax-il/issues/510)

**Fixed bugs:**

- Spotify: tizonia does not exit when the Spotify login attempt fails [\#533](https://github.com/tizonia/tizonia-openmax-il/issues/533)
- Spotify: --spotify-artist 'Mötley Crüe' returns Artist not found [\#524](https://github.com/tizonia/tizonia-openmax-il/issues/524)
- '--help config': information provided is outdated \(and confuses people\) [\#523](https://github.com/tizonia/tizonia-openmax-il/issues/523)
- Google Music: '--gmusic-unlimited-station': random station playing [\#522](https://github.com/tizonia/tizonia-openmax-il/issues/522)
- Using --gmusic-playlist exits with error 'maximum recursion depth exceeded in cmp' [\#519](https://github.com/tizonia/tizonia-openmax-il/issues/519)
- --gmusic-unlimited-playlist doesn't work \(key not found\) whereas --gmusic-playlist does \(and finds the list\) [\#515](https://github.com/tizonia/tizonia-openmax-il/issues/515)
- Linux Mint 18 Sarah: installation issue via install.sh script [\#508](https://github.com/tizonia/tizonia-openmax-il/issues/508)
- --gmusic-unlimited-activity plays same playlist for activity every time [\#507](https://github.com/tizonia/tizonia-openmax-il/issues/507)
- Google Music: playback stops at the end of a song with 'gnutls\_handshake\(\) failed: An unexpected TLS packet was received.' [\#500](https://github.com/tizonia/tizonia-openmax-il/issues/500)

**Closed issues:**

- Why so many dependencies?! [\#530](https://github.com/tizonia/tizonia-openmax-il/issues/530)
- Updating Instructions threw an unhandled exception, seems like a syntax error [\#520](https://github.com/tizonia/tizonia-openmax-il/issues/520)
- Tizonia Snap exits with OMX\_ErrorInsufficientResources error [\#513](https://github.com/tizonia/tizonia-openmax-il/issues/513)
- Tizonia can't find my Spotify playlists [\#506](https://github.com/tizonia/tizonia-openmax-il/issues/506)
- googlemusic settings help [\#505](https://github.com/tizonia/tizonia-openmax-il/issues/505)
- tizonia-all-git not found in AUR [\#504](https://github.com/tizonia/tizonia-openmax-il/issues/504)
- Gmusic-Unlimited Aratelia.audio\_source.http:port0 OMX\_ErrorInsufficientResources [\#503](https://github.com/tizonia/tizonia-openmax-il/issues/503)
- OMX\_ErrorInsufficientResources [\#501](https://github.com/tizonia/tizonia-openmax-il/issues/501)
- release v0.16.0 [\#537](https://github.com/tizonia/tizonia-openmax-il/issues/537)

## [v0.15.0](https://github.com/tizonia/tizonia-openmax-il/tree/v0.15.0) (2018-06-15)
[Full Changelog](https://github.com/tizonia/tizonia-openmax-il/compare/v0.14.0...v0.15.0)

**Improvements:**

- player: improved Spotify search capabilities \(tracks, artist, album, and playlists\) [\#495](https://github.com/tizonia/tizonia-openmax-il/issues/495)
- spotify\_source: use Spotify Web APIs for search functions [\#494](https://github.com/tizonia/tizonia-openmax-il/issues/494)
- clients: Spotify Web API client library [\#493](https://github.com/tizonia/tizonia-openmax-il/issues/493)
- clients: Spotify Web API proxy/wrapper Python module [\#492](https://github.com/tizonia/tizonia-openmax-il/issues/492)
- Spotify Playlists with Single Quotes in Name Can't Be Found [\#486](https://github.com/tizonia/tizonia-openmax-il/issues/486)
- spotify: unable to play some playlists [\#471](https://github.com/tizonia/tizonia-openmax-il/issues/471)

**Fixed bugs:**

- Spotify: tizonia stalls when trying to open a playlist [\#490](https://github.com/tizonia/tizonia-openmax-il/issues/490)
- Google Music: playback stops at the end of a song and playlist stalls [\#489](https://github.com/tizonia/tizonia-openmax-il/issues/489)
- player: Spotify playlist search stalls sometimes [\#487](https://github.com/tizonia/tizonia-openmax-il/issues/487)
- Spotify Playlists with Single Quotes in Name Can't Be Found [\#486](https://github.com/tizonia/tizonia-openmax-il/issues/486)
- AUR install depends on python2-eventlet-git which fails to build, instead of python2-eventlet which is in the default repository [\#484](https://github.com/tizonia/tizonia-openmax-il/issues/484)
- spotify: unable to play some playlists [\#471](https://github.com/tizonia/tizonia-openmax-il/issues/471)
- player: 'segmentation fault' while trying to get metadata of a track [\#433](https://github.com/tizonia/tizonia-openmax-il/issues/433)

**Closed issues:**

- release v0.15.0 [\#497](https://github.com/tizonia/tizonia-openmax-il/issues/497)

## [v0.14.0](https://github.com/tizonia/tizonia-openmax-il/tree/v0.14.0) (2018-04-20)
[Full Changelog](https://github.com/tizonia/tizonia-openmax-il/compare/v0.13.0...v0.14.0)

**Improvements:**

- YouTube Channels support - channel playlist search [\#476](https://github.com/tizonia/tizonia-openmax-il/issues/476)
- YouTube Channels support - channel uploads [\#475](https://github.com/tizonia/tizonia-openmax-il/issues/475)
- Debian Testing/Buster \(10\) not available [\#474](https://github.com/tizonia/tizonia-openmax-il/issues/474)
- google music: support for free radio stations [\#473](https://github.com/tizonia/tizonia-openmax-il/issues/473)
- google music: add option to play the entire user library [\#470](https://github.com/tizonia/tizonia-openmax-il/issues/470)

**Fixed bugs:**

- RSS Feed in official Website is broken [\#480](https://github.com/tizonia/tizonia-openmax-il/issues/480)
- segmentation fault with snap and spotify [\#479](https://github.com/tizonia/tizonia-openmax-il/issues/479)
- google music: Playlist shows 0 tracks in Tizonia, has tracks on Google Play web interface [\#478](https://github.com/tizonia/tizonia-openmax-il/issues/478)
- httprsrv.c:857:3: note: ‘snprintf’ output between 11 and 138 bytes into a destination of size 128 [\#477](https://github.com/tizonia/tizonia-openmax-il/issues/477)
- google music: tiz\_urltrans\_on\_buffers\_ready: assertion [\#472](https://github.com/tizonia/tizonia-openmax-il/issues/472)
- player: symbol lookup error: lib/tizonia0-plugins12/libtizhttpsrc.so.0.0.13: undefined symbol: tiz\_str\_util\_to\_lower [\#464](https://github.com/tizonia/tizonia-openmax-il/issues/464)
- Google Music: track downloading is interrupted seemingly randomly [\#349](https://github.com/tizonia/tizonia-openmax-il/issues/349)

**Closed issues:**

- python modules \_soundcloud\_ and \_fuzzywuzzy\_ were missing [\#469](https://github.com/tizonia/tizonia-openmax-il/issues/469)
- how to fix "FATAL. Could not init OpenMAX IL : OMX\_ErrorInsufficientResources" [\#466](https://github.com/tizonia/tizonia-openmax-il/issues/466)
- Bionic install from repository: libspotify12 missing [\#465](https://github.com/tizonia/tizonia-openmax-il/issues/465)
- release v0.14.0 [\#482](https://github.com/tizonia/tizonia-openmax-il/issues/482)

## [v0.13.0](https://github.com/tizonia/tizonia-openmax-il/tree/v0.13.0) (2018-03-11)
[Full Changelog](https://github.com/tizonia/tizonia-openmax-il/compare/v0.12.0...v0.13.0)

**Improvements:**

- libtizplatform: need API to configure connection timeout in tizurltransfer.h [\#453](https://github.com/tizonia/tizonia-openmax-il/issues/453)
- dirble: m3u files are not parsed [\#451](https://github.com/tizonia/tizonia-openmax-il/issues/451)
- soundcloud: improve the matching of search results [\#446](https://github.com/tizonia/tizonia-openmax-il/issues/446)
- google music: improve the matching of search results [\#445](https://github.com/tizonia/tizonia-openmax-il/issues/445)
- player: add progress display [\#444](https://github.com/tizonia/tizonia-openmax-il/issues/444)
- Client request: Plex Media Server [\#387](https://github.com/tizonia/tizonia-openmax-il/issues/387)

**Fixed bugs:**

- Lots of ChannelError messages during Spotify playback [\#443](https://github.com/tizonia/tizonia-openmax-il/issues/443)
- aac\_decoder: 'OMX\_ErrorInsuficientResources' is sometimes returned instead of 'OMX\_ErrorStreamCorruptFatal' [\#459](https://github.com/tizonia/tizonia-openmax-il/issues/459)
- dirble: can't play radio stations - 'Content-Length' header comparison should be case-insensitive [\#458](https://github.com/tizonia/tizonia-openmax-il/issues/458)
- dirble: error 'Unable to override decoder/renderer sampling rates' [\#457](https://github.com/tizonia/tizonia-openmax-il/issues/457)
- dirble: buffer underruns in some radio stations \(specially with FlumotionHTTPServer\) [\#456](https://github.com/tizonia/tizonia-openmax-il/issues/456)
- dirble: can't play radio stations if url contains mixed-case 'http' prefix [\#455](https://github.com/tizonia/tizonia-openmax-il/issues/455)
- player: can't play radio stations if url contains mixed-case 'http' prefix [\#454](https://github.com/tizonia/tizonia-openmax-il/issues/454)
- dirble: connection timeout is currently too long \(20 s\) [\#452](https://github.com/tizonia/tizonia-openmax-il/issues/452)
- player: no local files played if the playlist contains unicode file names [\#449](https://github.com/tizonia/tizonia-openmax-il/issues/449)
- soundcloud: UnicodeDecodeError 'ascii' codec can't decode byte 0xf0 in position 13 [\#432](https://github.com/tizonia/tizonia-openmax-il/issues/432)
- Building tizonia on Ubuntu 17.10 Artful [\#421](https://github.com/tizonia/tizonia-openmax-il/issues/421)

**Closed issues:**

- There is no tizonia.conf file [\#447](https://github.com/tizonia/tizonia-openmax-il/issues/447)
- Most CD ripper programs rip within a newly created folder representing the CD's name and artist, yet Tizonia appears to be oblivious to all nested folders in ~/Music. [\#442](https://github.com/tizonia/tizonia-openmax-il/issues/442)
- Installer script doesn't copy tizonia.conf file [\#441](https://github.com/tizonia/tizonia-openmax-il/issues/441)
- Unable to run on Solus OS [\#434](https://github.com/tizonia/tizonia-openmax-il/issues/434)
- release v0.13.0 [\#460](https://github.com/tizonia/tizonia-openmax-il/issues/460)

**Merged pull requests:**

- Fixed path to default config file for standard install [\#448](https://github.com/tizonia/tizonia-openmax-il/pull/448) ([nlwstein](https://github.com/nlwstein))

## [v0.12.0](https://github.com/tizonia/tizonia-openmax-il/tree/v0.12.0) (2018-02-11)
[Full Changelog](https://github.com/tizonia/tizonia-openmax-il/compare/v0.11.0...v0.12.0)

**Improvements:**

- snap: move snapcraft.yaml to its own repo [\#431](https://github.com/tizonia/tizonia-openmax-il/issues/431)
- libtizplatform: allow expansion of environment variables in tizonia configuration file [\#429](https://github.com/tizonia/tizonia-openmax-il/issues/429)
- tizonia-config: Default configuration files should be installed to $sysconfdir/xdg/subdir/filename with $sysconfdir defaulting to /etc. [\#427](https://github.com/tizonia/tizonia-openmax-il/issues/427)
- build system: update ax\_boost\*.m4 macros with latest version from autoconf-archive [\#426](https://github.com/tizonia/tizonia-openmax-il/issues/426)
- build system: Tizonia snap package [\#309](https://github.com/tizonia/tizonia-openmax-il/issues/309)
- clients: add a 'tizchromecastproxy' Python module [\#298](https://github.com/tizonia/tizonia-openmax-il/issues/298)
- Chromecast support [\#296](https://github.com/tizonia/tizonia-openmax-il/issues/296)

**Fixed bugs:**

- Use of deprecated gethostbyname\(\) [\#425](https://github.com/tizonia/tizonia-openmax-il/issues/425)

**Closed issues:**

- Spotify playlist fails [\#435](https://github.com/tizonia/tizonia-openmax-il/issues/435)
- release v0.12.0  [\#436](https://github.com/tizonia/tizonia-openmax-il/issues/436)

**Merged pull requests:**

- Make aac\_decoder optional [\#422](https://github.com/tizonia/tizonia-openmax-il/pull/422) ([luigino](https://github.com/luigino))

## [v0.11.0](https://github.com/tizonia/tizonia-openmax-il/tree/v0.11.0) (2017-12-17)
[Full Changelog](https://github.com/tizonia/tizonia-openmax-il/compare/v0.10.0...v0.11.0)

**Improvements:**

- libtizplatform: use $XDG\_CONFIG\_HOME and $XDG\_CONFIG\_DIRS to locate tizonia.conf [\#428](https://github.com/tizonia/tizonia-openmax-il/issues/428)
- 3rdparty: embed pkgw/dbus-cplusplus [\#413](https://github.com/tizonia/tizonia-openmax-il/issues/413)
- http\_source: youtubeprc - increase the amount of data emitted by the output port [\#411](https://github.com/tizonia/tizonia-openmax-il/issues/411)
- webm\_demuxer: catch up internal nestegg with kinetiknz/nestegg master [\#405](https://github.com/tizonia/tizonia-openmax-il/issues/405)
- build system: top-level 'configure' option to enable/disable the 'alsa' omx component [\#402](https://github.com/tizonia/tizonia-openmax-il/issues/402)
- build system: top-level 'configure' option to enable/disable 'spotify\_source' omx component build [\#396](https://github.com/tizonia/tizonia-openmax-il/issues/396)
- tizonia-player: bash completion file distributed in debian package [\#385](https://github.com/tizonia/tizonia-openmax-il/issues/385)
- tizonia-player: zsh completion file distributed in debian package [\#384](https://github.com/tizonia/tizonia-openmax-il/issues/384)
- tizonia-player: bash completion file [\#307](https://github.com/tizonia/tizonia-openmax-il/issues/307)
- clients: add a 'libtizchromecast' binding library [\#297](https://github.com/tizonia/tizonia-openmax-il/issues/297)

**Fixed bugs:**

- player: youtube crash with webm/vorbis stream [\#420](https://github.com/tizonia/tizonia-openmax-il/issues/420)
- release v0.11.0 [\#419](https://github.com/tizonia/tizonia-openmax-il/issues/419)
- player: garbage on the screen after 'tizonia' exits on error [\#418](https://github.com/tizonia/tizonia-openmax-il/issues/418)
- player: audio playback from local files is broken for most formats [\#417](https://github.com/tizonia/tizonia-openmax-il/issues/417)
- webm\_demuxer: dmuxfltprc - allow nestegg initialisation to fail while buffering input data [\#412](https://github.com/tizonia/tizonia-openmax-il/issues/412)
- libtizplatform: tizurltransfer needs to work correctly when copying to an omx buffer that already contains data [\#410](https://github.com/tizonia/tizonia-openmax-il/issues/410)
- webm\_demuxer: not enough input data results in the nestegg object not initializing correctly  [\#408](https://github.com/tizonia/tizonia-openmax-il/issues/408)
- webm\_demuxer: allocation of the nestegg object fails silently with some streams [\#407](https://github.com/tizonia/tizonia-openmax-il/issues/407)
- youtube: console informational element 'stream \#' appears garbled [\#406](https://github.com/tizonia/tizonia-openmax-il/issues/406)
- install.sh: make sure pip2 is used on systems that have pip3 as its default pip installation [\#404](https://github.com/tizonia/tizonia-openmax-il/issues/404)
- Crashes on yt livestreams [\#403](https://github.com/tizonia/tizonia-openmax-il/issues/403)
- Ogg Vorbis decoding is slow and static-y [\#386](https://github.com/tizonia/tizonia-openmax-il/issues/386)

**Closed issues:**

- Building on 17.10 hangs [\#414](https://github.com/tizonia/tizonia-openmax-il/issues/414)
- aac decoder: pcmmode.eEndian should be set to OMX\_EndianLittle instead of OMX\_EndianBig . [\#409](https://github.com/tizonia/tizonia-openmax-il/issues/409)
- No such file or directory \(youtube\). [\#401](https://github.com/tizonia/tizonia-openmax-il/issues/401)
- Can't install on Ubuntu 14.04 trusty [\#398](https://github.com/tizonia/tizonia-openmax-il/issues/398)
- .config reload [\#397](https://github.com/tizonia/tizonia-openmax-il/issues/397)
- Ubuntu 18.04 support [\#393](https://github.com/tizonia/tizonia-openmax-il/issues/393)
- Binary for Ubuntu 17.10 [\#389](https://github.com/tizonia/tizonia-openmax-il/issues/389)
- Can´t autoreconf -ifs [\#388](https://github.com/tizonia/tizonia-openmax-il/issues/388)

**Merged pull requests:**

- Updating link to soundcloud docs page [\#415](https://github.com/tizonia/tizonia-openmax-il/pull/415) ([djsiddz2](https://github.com/djsiddz2))

## [v0.10.0](https://github.com/tizonia/tizonia-openmax-il/tree/v0.10.0) (2017-10-22)
[Full Changelog](https://github.com/tizonia/tizonia-openmax-il/compare/v0.9.0...v0.10.0)

**Improvements:**

- distribution: raspbian stretch [\#370](https://github.com/tizonia/tizonia-openmax-il/issues/370)
- build system : top-level 'configure' option to enable/disable 'player' compilation [\#249](https://github.com/tizonia/tizonia-openmax-il/issues/249)
- CI: Migrate to Travis' new infrastructure [\#124](https://github.com/tizonia/tizonia-openmax-il/issues/124)
- player: dirble graph's default sampling rate should be 44100 instead of 48000 [\#382](https://github.com/tizonia/tizonia-openmax-il/issues/382)
- dirble: update country search to accept country names in addition to country ISO 3166 codes [\#381](https://github.com/tizonia/tizonia-openmax-il/issues/381)
- youtube: add APIs to query the playback queue progress \(e.g. currently playing stream 5 or 17\) [\#380](https://github.com/tizonia/tizonia-openmax-il/issues/380)
- arch pl0x [\#335](https://github.com/tizonia/tizonia-openmax-il/issues/335)

**Fixed bugs:**

- Coverity Scan \(CID 1087260\) \[/libtizplatform/src/avl/avl.c:avl\_iterate\_index\_range\] : Dereference null return value [\#205](https://github.com/tizonia/tizonia-openmax-il/issues/205)
- Coverity Scan \(CID 1352377\) \[/plugins/http\_renderer/src/httprsrv.c:httpr\_srv\_set\_mp3\_settings\] : Result is not floating-point [\#196](https://github.com/tizonia/tizonia-openmax-il/issues/196)
- Coverity Scan \(CID 1352358\) \[/libtizonia/src/tizuricfgport.c:uri\_cfgport\_GetParameter\] : Array compared against 0 [\#192](https://github.com/tizonia/tizonia-openmax-il/issues/192)
- Coverity Scan \(CID 1352332\) \[/plugins/http\_renderer/src/httprsrv.c:srv\_get\_socket\_buffer\_size\] : Unchecked return value from library [\#174](https://github.com/tizonia/tizonia-openmax-il/issues/174)
- Coverity Scan \(CID 1352331\) \[/plugins/http\_renderer/src/httprsrv.c:srv\_send\_http\_error\] : Unchecked return value from library [\#173](https://github.com/tizonia/tizonia-openmax-il/issues/173)
- Coverity Scan \(CID 1352330\) \[/plugins/http\_renderer/src/httprsrv.c:srv\_create\_server\_socket\] : Unchecked return value from library [\#172](https://github.com/tizonia/tizonia-openmax-il/issues/172)
- Coverity Scan \(CID 1352329\) \[/plugins/http\_source/src/httpsrctrans.c:start\_curl\] : Unchecked return value [\#171](https://github.com/tizonia/tizonia-openmax-il/issues/171)
- Coverity Scan \(CID 1352328\) \[/plugins/http\_source/src/scloudprc.c:update\_metadata\] : Unchecked return value [\#170](https://github.com/tizonia/tizonia-openmax-il/issues/170)
- Coverity Scan \(CID 1352340\) \[/plugins/http\_renderer/src/httprsrv.c:srv\_accept\_connection\] : Logically dead code [\#167](https://github.com/tizonia/tizonia-openmax-il/issues/167)
- Coverity Scan \(CID 1352339\) \[/plugins/http\_renderer/src/httprsrv.c:srv\_accept\_connection\] : Logically dead code [\#166](https://github.com/tizonia/tizonia-openmax-il/issues/166)
- youtube: URLs are permanently deleted from the playback queue in the presence of temporary communication errors [\#379](https://github.com/tizonia/tizonia-openmax-il/issues/379)
- pcm\_renderer\_alsa: OMX\_ErrorInsufficientResources on Raspberry Pi \(one-channel playback through FiiO USB DAC\) [\#378](https://github.com/tizonia/tizonia-openmax-il/issues/378)
- 'tizonia --youtube-audio-mix-search' fails with 'TypeError: sequence item 9: expected string or Unicode, dict found' [\#377](https://github.com/tizonia/tizonia-openmax-il/issues/377)
- Build failure on Arch Linux [\#376](https://github.com/tizonia/tizonia-openmax-il/issues/376)
- Current AUR build is broken on at least two levels [\#375](https://github.com/tizonia/tizonia-openmax-il/issues/375)
- player: OMX\_ErrorInsufficientResources while pausing on Raspberry PI \(using alsa renderer\) [\#374](https://github.com/tizonia/tizonia-openmax-il/issues/374)
- "Illegal instrunction" on "2017-07-05-raspbian-jessie-lite.img" [\#372](https://github.com/tizonia/tizonia-openmax-il/issues/372)
- http\_renderer: 'tizonia --server' hangs after the first song [\#371](https://github.com/tizonia/tizonia-openmax-il/issues/371)
- player: the terminal is occasionally left in an inconsistent state when the application fails [\#369](https://github.com/tizonia/tizonia-openmax-il/issues/369)
- Various warnings while building with gcc 7.1.1 \(Manjaro Linux\) [\#367](https://github.com/tizonia/tizonia-openmax-il/issues/367)
- docs: doc builds fail due to Breathe defect [\#343](https://github.com/tizonia/tizonia-openmax-il/issues/343)
- Issues with Google Music : --gmusic-playlist [\#328](https://github.com/tizonia/tizonia-openmax-il/issues/328)
- tizonia: pulsearprc.c:592: init\_pulseaudio\_stream: Assertion `ap\_prc-\>p\_pa\_context\_' failed. Aborted \(core dumped\) [\#301](https://github.com/tizonia/tizonia-openmax-il/issues/301)

**Closed issues:**

- tools/install.sh still does not work for ubuntu-16.04 [\#373](https://github.com/tizonia/tizonia-openmax-il/issues/373)
- The PKGBUILD installs config files to /usr/etc instead of /etc [\#366](https://github.com/tizonia/tizonia-openmax-il/issues/366)
- release v0.10.0 [\#383](https://github.com/tizonia/tizonia-openmax-il/issues/383)

**Merged pull requests:**

- PKGBUILD: Fix style inconsistencies [\#365](https://github.com/tizonia/tizonia-openmax-il/pull/365) ([sylveon](https://github.com/sylveon))
- vp8\_decoder: add inport\_SetParameter to set nBufferSize [\#363](https://github.com/tizonia/tizonia-openmax-il/pull/363) ([CapOM](https://github.com/CapOM))

## [v0.9.0](https://github.com/tizonia/tizonia-openmax-il/tree/v0.9.0) (2017-08-04)
[Full Changelog](https://github.com/tizonia/tizonia-openmax-il/compare/v0.8.0...v0.9.0)

**Improvements:**

- libtizonia: OMX\_IndexParamVideoQuantization not supported [\#351](https://github.com/tizonia/tizonia-openmax-il/issues/351)

**Fixed bugs:**

- release v0.9.0 [\#361](https://github.com/tizonia/tizonia-openmax-il/issues/361)
- tizdeezerproxy: removal due to copyright issues [\#360](https://github.com/tizonia/tizonia-openmax-il/issues/360)
- libtizonia: nBufferSize is not updated on video output port [\#358](https://github.com/tizonia/tizonia-openmax-il/issues/358)
- vp8\_decoder: the flush handler should not discard the codec config, only the stored stream data [\#354](https://github.com/tizonia/tizonia-openmax-il/issues/354)
- libtizonia: tizscheduler.c::set\_thread\_name crashes if there is no "third" dot in the component name [\#353](https://github.com/tizonia/tizonia-openmax-il/issues/353)
- libtizonia: port hooks \(both alloc and eglimage validation\) are not restored and get lost after the component's role is changed or reset [\#348](https://github.com/tizonia/tizonia-openmax-il/issues/348)
- aac\_decoder: aacdecprc.c:transform\_buffer:436 --- \[OMX\_ErrorInsufficientResources\] : Unable to store all the data. [\#346](https://github.com/tizonia/tizonia-openmax-il/issues/346)
- libtizonia: a port that does not support OMX\_UseEGLImage should return OMX\_ErrorNotImplemented [\#345](https://github.com/tizonia/tizonia-openmax-il/issues/345)

**Closed issues:**

- libtizonia: allow registration of different egl validation hooks for different component roles [\#350](https://github.com/tizonia/tizonia-openmax-il/issues/350)
- support mesa/gallium [\#116](https://github.com/tizonia/tizonia-openmax-il/issues/116)

**Merged pull requests:**

- Framerate [\#359](https://github.com/tizonia/tizonia-openmax-il/pull/359) ([CapOM](https://github.com/CapOM))
- Display proper file path for systemwide configuration in help and README [\#355](https://github.com/tizonia/tizonia-openmax-il/pull/355) ([5nafu](https://github.com/5nafu))
- tizavcport: Add support for OMX\_IndexParamVideoQuantization [\#352](https://github.com/tizonia/tizonia-openmax-il/pull/352) ([gpalsingh](https://github.com/gpalsingh))
- Dev [\#344](https://github.com/tizonia/tizonia-openmax-il/pull/344) ([CapOM](https://github.com/CapOM))
- Fix path to tizonia.conf file [\#342](https://github.com/tizonia/tizonia-openmax-il/pull/342) ([dbart](https://github.com/dbart))

## [v0.8.0](https://github.com/tizonia/tizonia-openmax-il/tree/v0.8.0) (2017-06-25)
[Full Changelog](https://github.com/tizonia/tizonia-openmax-il/compare/v0.7.0...v0.8.0)

**Improvements:**

- tizonia-player: oh-my-zsh plugin [\#336](https://github.com/tizonia/tizonia-openmax-il/issues/336)
- Google Music : add a '--gmusic-podcast' option [\#334](https://github.com/tizonia/tizonia-openmax-il/issues/334)
- Google Music : add a '--gmusic-tracks' option [\#333](https://github.com/tizonia/tizonia-openmax-il/issues/333)
- Google Music : add a '--gmusic-unlimited-playlist' [\#332](https://github.com/tizonia/tizonia-openmax-il/issues/332)
- webm\_demuxer: update 'nestegg' library to latest [\#323](https://github.com/tizonia/tizonia-openmax-il/issues/323)
- webm\_demuxer: implement VP8/VP9 video demuxing in 'filter' role [\#322](https://github.com/tizonia/tizonia-openmax-il/issues/322)
- vp8\_decoder: add support for raw streams [\#314](https://github.com/tizonia/tizonia-openmax-il/issues/314)
- build system: debian packaging of YUV renderer component [\#313](https://github.com/tizonia/tizonia-openmax-il/issues/313)
- build system: debian packaging of VP8 decoder component [\#312](https://github.com/tizonia/tizonia-openmax-il/issues/312)
- tizonia-player: zsh completion file [\#306](https://github.com/tizonia/tizonia-openmax-il/issues/306)
- clients: add a 'tizdeezerproxy' Python module  [\#300](https://github.com/tizonia/tizonia-openmax-il/issues/300)
- clients: add a 'libtizdeezer' binding library [\#299](https://github.com/tizonia/tizonia-openmax-il/issues/299)
- player: add '--youtube-audio-mix-search' option [\#295](https://github.com/tizonia/tizonia-openmax-il/issues/295)
- Add support for Deezer [\#294](https://github.com/tizonia/tizonia-openmax-il/issues/294)

**Fixed bugs:**

- release v0.8.0  [\#339](https://github.com/tizonia/tizonia-openmax-il/issues/339)
- libtizonia: IL 1.2 slaving behavior needs to be applied to both ports \(input and output\) if the port settings changed is triggered internally [\#338](https://github.com/tizonia/tizonia-openmax-il/issues/338)
- Issues with Google Music : player stalls if connection is lost \(libcurl  SSL error with errno 104\) [\#337](https://github.com/tizonia/tizonia-openmax-il/issues/337)
- Issues with Google Music : --gmusic-unlimited-station incorrect "not found" message \(minor issue\) [\#331](https://github.com/tizonia/tizonia-openmax-il/issues/331)
- Issues with Google Music : --gmusic-unlimited-activity sometimes does not return any results [\#330](https://github.com/tizonia/tizonia-openmax-il/issues/330)
- yuv\_renderer: 'sdlivr\_prc\_buffers\_ready' last few buffers in the stream sometime are not processed [\#329](https://github.com/tizonia/tizonia-openmax-il/issues/329)
- libtizonia: tizvideoport needs to override 'SetParameter\_internal' [\#327](https://github.com/tizonia/tizonia-openmax-il/issues/327)
- yuv\_renderer: add port enable/disable functionality [\#326](https://github.com/tizonia/tizonia-openmax-il/issues/326)
- Issues with Google Music : --gmusic-unlimited-tracks [\#325](https://github.com/tizonia/tizonia-openmax-il/issues/325)
- libtizonia: 'videoport' base class has no 'tiz\_port\_set\_portdef\_format' override [\#321](https://github.com/tizonia/tizonia-openmax-il/issues/321)
- tizyoutubeproxy: allow for retries as a workaround for SSLEOFErrors while retrieving youtube stream URLs [\#315](https://github.com/tizonia/tizonia-openmax-il/issues/315)
- libtizplatform: can't locate the global tizonia.conf configuration file \(@sysconfdir@\) when the user's config file is not present [\#311](https://github.com/tizonia/tizonia-openmax-il/issues/311)
- tizonia.conf: Debian installation puts it under /etc/tizonia/tizonia.conf instead of /etc/tizonia [\#310](https://github.com/tizonia/tizonia-openmax-il/issues/310)
- player: command line argument processing is broken on systems with boost 1.59 or greater [\#308](https://github.com/tizonia/tizonia-openmax-il/issues/308)
- tools/install.sh: script does not support installation on Ubuntu-based LinuxMint OSes  [\#303](https://github.com/tizonia/tizonia-openmax-il/issues/303)
- tools/install.sh: script does not support installation on 16.04-based Elementary OS 0.4 Loki [\#302](https://github.com/tizonia/tizonia-openmax-il/issues/302)

**Closed issues:**

- Can't find a supported Debian or Ubuntu-based distribution. [\#304](https://github.com/tizonia/tizonia-openmax-il/issues/304)

**Merged pull requests:**

- libtizonia: add AVC port [\#320](https://github.com/tizonia/tizonia-openmax-il/pull/320) ([gpalsingh](https://github.com/gpalsingh))

## [v0.7.0](https://github.com/tizonia/tizonia-openmax-il/tree/v0.7.0) (2017-03-02)
[Full Changelog](https://github.com/tizonia/tizonia-openmax-il/compare/v0.6.0...v0.7.0)

**Improvements:**

- clients-youtube: skipping to a new song takes too long the first time, faster after that [\#291](https://github.com/tizonia/tizonia-openmax-il/issues/291)

**Fixed bugs:**

- Coverity Scan \(CID 1087243\) \[/libtizcore/src/tizcore.c:instantiate\_comp\_lib\] : Copy into fixed size buffer [\#234](https://github.com/tizonia/tizonia-openmax-il/issues/234)
- Coverity Scan \(CID 1087305\) \[/libtizcore/src/tizcore.c:get\_core\] : Logically dead code [\#164](https://github.com/tizonia/tizonia-openmax-il/issues/164)
- Coverity Scan \(CID 1087304\) \[/libtizplatform/src/tizrc.c:tiz\_rcfile\_init\] : Logically dead code [\#163](https://github.com/tizonia/tizonia-openmax-il/issues/163)
- release v0.7.0 [\#293](https://github.com/tizonia/tizonia-openmax-il/issues/293)
- player: tizonia '--youtube-audio-playlist' always returns "Playlist not found" [\#292](https://github.com/tizonia/tizonia-openmax-il/issues/292)
- tizonia: application crashes if option values are not provided [\#290](https://github.com/tizonia/tizonia-openmax-il/issues/290)
- googlemusic: --gmusic-artist outputs dictionary contents instead of artist name [\#289](https://github.com/tizonia/tizonia-openmax-il/issues/289)
- Coverity Scan \(CID 1352370\) \[/usr/include/boost/msm/back/dispatch\_table.hpp:dispatch\_table\] : Uninitialized pointer field [\#237](https://github.com/tizonia/tizonia-openmax-il/issues/237)
- Coverity Scan \(CID 1352368\) \[/usr/include/boost/msm/back/state\_machine.hpp:state\_machine\] : Uninitialized scalar field [\#235](https://github.com/tizonia/tizonia-openmax-il/issues/235)
- Coverity Scan \(CID 1352350\) \[/player/src/tizgraphmgrops.cpp:deinit\] : Double lock [\#216](https://github.com/tizonia/tizonia-openmax-il/issues/216)
- Coverity Scan \(CID 993663\) \[/libtizplatform/src/ev/ev.c:infy\_cb\] : Untrusted array index read [\#197](https://github.com/tizonia/tizonia-openmax-il/issues/197)
- Coverity Scan \(CID 1087266\) \[/libtizonia/src/tizutils.c:tiz\_fsm\_state\_to\_str\] : Mixing enum types [\#175](https://github.com/tizonia/tizonia-openmax-il/issues/175)

## [v0.6.0](https://github.com/tizonia/tizonia-openmax-il/tree/v0.6.0) (2017-01-16)
[Full Changelog](https://github.com/tizonia/tizonia-openmax-il/compare/v0.5.0...v0.6.0)

**Improvements:**

- libtizplatform: update utarray to v2.0.1 [\#288](https://github.com/tizonia/tizonia-openmax-il/issues/288)
- libtizplatform: update http-parser to v2.7.1 [\#287](https://github.com/tizonia/tizonia-openmax-il/issues/287)
- vorbis\_decoder: stream metadata retrieval [\#281](https://github.com/tizonia/tizonia-openmax-il/issues/281)
- opus\_decoder: stream metadata retrieval  [\#280](https://github.com/tizonia/tizonia-openmax-il/issues/280)
- vorbis\_decoder: handling of port flush, enable and disable events [\#278](https://github.com/tizonia/tizonia-openmax-il/issues/278)
- libtizplatform: add 'tiz\_audio\_coding\_to\_str' utility function [\#277](https://github.com/tizonia/tizonia-openmax-il/issues/277)
- spotify\_source: play a random playlist when one cannot be found in the user library [\#274](https://github.com/tizonia/tizonia-openmax-il/issues/274)
- libtizplatform: fix and clean up 'tiz\_check\_xxx' utilities [\#269](https://github.com/tizonia/tizonia-openmax-il/issues/269)
- libtizonia: add a muxer port [\#268](https://github.com/tizonia/tizonia-openmax-il/issues/268)
- libtizonia: add an ogg port [\#267](https://github.com/tizonia/tizonia-openmax-il/issues/267)
- libtizonia: add a webm port [\#266](https://github.com/tizonia/tizonia-openmax-il/issues/266)
- libtizonia: rename 'tiz\_buffer\_overwrite\_mode' as 'tiz\_buffer\_seek\_mode'  [\#265](https://github.com/tizonia/tizonia-openmax-il/issues/265)
- libtizplatform: add 'tiz\_buffer\_seek', 'tiz\_buffer\_offset' and 'tiz\_buffer\_overwrite\_mode' apis [\#264](https://github.com/tizonia/tizonia-openmax-il/issues/264)
- libtizplatform: remove 'tiz\_buffer\_t' apis that were deprecated in version 0.2.0   [\#263](https://github.com/tizonia/tizonia-openmax-il/issues/263)
- libtizcore: avoid reading non-shared object entries during directory scanning [\#262](https://github.com/tizonia/tizonia-openmax-il/issues/262)
- libtizonia: add port disabled/enabled accessors in 'tiz\_filter\_prc\_t' [\#261](https://github.com/tizonia/tizonia-openmax-il/issues/261)
- libtizonia: 'tiz\_filter\_prc\_t' needs to support components with multiple input and output ports [\#260](https://github.com/tizonia/tizonia-openmax-il/issues/260)
- tizonia/gmusic: graph's default sampling rate should be 44100 instead of 48000  [\#259](https://github.com/tizonia/tizonia-openmax-il/issues/259)
- libtizplatform: fix clang-tidy-3.8 warnings [\#258](https://github.com/tizonia/tizonia-openmax-il/issues/258)
- libtizonia: fix clang-tidy-3.8 warnings [\#257](https://github.com/tizonia/tizonia-openmax-il/issues/257)
- tizplatform: libcurl-based data transfer utility [\#254](https://github.com/tizonia/tizonia-openmax-il/issues/254)
- clients: youtube audio streaming proxy Python module [\#253](https://github.com/tizonia/tizonia-openmax-il/issues/253)
- clients: youtube audio streaming client library [\#252](https://github.com/tizonia/tizonia-openmax-il/issues/252)
- player: youtube audio streaming graph [\#251](https://github.com/tizonia/tizonia-openmax-il/issues/251)
- plugins: webm demuxer component  [\#250](https://github.com/tizonia/tizonia-openmax-il/issues/250)

**Fixed bugs:**

- Coverity Scan \(CID 1352364\) \[/libtizplatform/src/tizrc.c:get\_node\] : Resource leak [\#232](https://github.com/tizonia/tizonia-openmax-il/issues/232)
- Coverity Scan \(CID 1087255\) \[/libtizplatform/src/tizhttp.c:insert\_kv\_pair\] : Resource leak [\#228](https://github.com/tizonia/tizonia-openmax-il/issues/228)
- Coverity Scan \(CID 1087245\) \[/libtizplatform/src/tizrc.c:tiz\_rcfile\_init\] : Resource leak [\#221](https://github.com/tizonia/tizonia-openmax-il/issues/221)
- Coverity Scan \(CID 1087244\) \[/libtizplatform/src/tizvector.c:tiz\_vector\_init\] : Resource leak [\#220](https://github.com/tizonia/tizonia-openmax-il/issues/220)
- Coverity Scan \(CID 1352353\) \[/libtizplatform/src/tizev.c:enqueue\_timer\_msg\] : Missing unlock [\#219](https://github.com/tizonia/tizonia-openmax-il/issues/219)
- Coverity Scan \(CID 1352352\) \[/libtizplatform/src/tizev.c:enqueue\_io\_msg\] : Missing unlock [\#218](https://github.com/tizonia/tizonia-openmax-il/issues/218)
- Coverity Scan \(CID 1352351\) \[/libtizplatform/src/tizev.c:enqueue\_stat\_msg\] : Missing unlock [\#217](https://github.com/tizonia/tizonia-openmax-il/issues/217)
- Coverity Scan \(CID 1352357\) \[/plugins/http\_renderer/src/httprsrv.c:srv\_write\_omx\_buffer\] : Macro compares unsigned to 0 [\#202](https://github.com/tizonia/tizonia-openmax-il/issues/202)
- Coverity Scan \(CID 1352356\) \[/plugins/vorbis\_decoder/src/vorbisdprc.c:transform\_buffer\] : Macro compares unsigned to 0 [\#201](https://github.com/tizonia/tizonia-openmax-il/issues/201)
- Coverity Scan \(CID 1352334\) \[/plugins/spotify\_source/src/spfysrcprc.c:send\_port\_auto\_detect\_events\] : Constant expression result [\#200](https://github.com/tizonia/tizonia-openmax-il/issues/200)
- Coverity Scan \(CID 1352333\) \[/plugins/pcm\_renderer\_pa/src/pulsearprc.c:init\_pulseaudio\_stream\] : Operands don't affect result [\#199](https://github.com/tizonia/tizonia-openmax-il/issues/199)
- Coverity Scan \(CID 1352349\) \[/player/src/tizplayapp.cpp:getch\_\] : Overflowed return value [\#198](https://github.com/tizonia/tizonia-openmax-il/issues/198)
- Coverity Scan \(CID 1352354\) \[/libtizonia/src/tizservant.c:srv\_issue\_trans\_event\] : Mixing enum types [\#190](https://github.com/tizonia/tizonia-openmax-il/issues/190)
- Coverity Scan \(CID 1352337\) \[/libtizplatform/src/tizev.c:ev\_io\_msg\_dequeue\] : Same on both sides [\#189](https://github.com/tizonia/tizonia-openmax-il/issues/189)
- Coverity Scan \(CID 1352336\) \[/libtizplatform/src/tizev.c:ev\_timer\_msg\_dequeue\] : Same on both sides [\#188](https://github.com/tizonia/tizonia-openmax-il/issues/188)
- Coverity Scan \(CID 1352335\) \[/libtizplatform/src/tizev.c:ev\_stat\_msg\_dequeue\] : Same on both sides [\#187](https://github.com/tizonia/tizonia-openmax-il/issues/187)
- Coverity Scan \(CID 1087279\) \[/libtizonia/src/tizfsm.c:fsm\_complete\_transition\] : Mixing enum types [\#186](https://github.com/tizonia/tizonia-openmax-il/issues/186)
- Coverity Scan \(CID 1087277\) \[/libtizonia/src/tizidletoexecuting.c:idletoexecuting\_trans\_complete\] : Mixing enum types [\#185](https://github.com/tizonia/tizonia-openmax-il/issues/185)
- Coverity Scan \(CID 1087276\) \[/libtizonia/src/tizidletoloaded.c:idletoloaded\_trans\_complete\] : Mixing enum types [\#184](https://github.com/tizonia/tizonia-openmax-il/issues/184)
- Coverity Scan \(CID 1087275\) \[/libtizonia/src/tizloadedtoidle.c:loadedtoidle\_trans\_complete\] : Mixing enum types [\#183](https://github.com/tizonia/tizonia-openmax-il/issues/183)
- Coverity Scan \(CID 1087274\) \[/libtizonia/src/tizwaitforresources.c:waitforresources\_trans\_complete\] : Mixing enum types [\#182](https://github.com/tizonia/tizonia-openmax-il/issues/182)
- Coverity Scan \(CID 1087272\) \[/libtizonia/src/tizstate.c:state\_trans\_complete\] : Mixing enum types [\#181](https://github.com/tizonia/tizonia-openmax-il/issues/181)
- Coverity Scan \(CID 1087271\) \[/libtizonia/src/tizpause.c:pause\_trans\_complete\] : Mixing enum types [\#180](https://github.com/tizonia/tizonia-openmax-il/issues/180)
- Coverity Scan \(CID 1087270\) \[/libtizonia/src/tizloaded.c:loaded\_trans\_complete\] : Mixing enum types [\#179](https://github.com/tizonia/tizonia-openmax-il/issues/179)
- Coverity Scan \(CID 1087269\) \[/libtizonia/src/tizexecuting.c:executing\_trans\_complete\] : Mixing enum types [\#178](https://github.com/tizonia/tizonia-openmax-il/issues/178)
- Coverity Scan \(CID 1087268\) \[/libtizonia/src/tizexecutingtoidle.c:executingtoidle\_trans\_complete\] : Mixing enum types [\#177](https://github.com/tizonia/tizonia-openmax-il/issues/177)
- Coverity Scan \(CID 1087267\) \[/libtizonia/src/tizpausetoidle.c:pausetoidle\_trans\_complete\] : Mixing enum types [\#176](https://github.com/tizonia/tizonia-openmax-il/issues/176)
- Coverity Scan \(CID 1352327\) \[/player/src/tizplayapp.cpp:getch\_\] : Truncated stdio return value [\#162](https://github.com/tizonia/tizonia-openmax-il/issues/162)
- release v0.6.0 [\#286](https://github.com/tizonia/tizonia-openmax-il/issues/286)
- vorbis\_decoder: handling port settings changes is missing [\#285](https://github.com/tizonia/tizonia-openmax-il/issues/285)
- libtizplatform: tiz\_vector\_clear should not crash if passed a null pointer [\#284](https://github.com/tizonia/tizonia-openmax-il/issues/284)
- pcm\_renderer\_pa: audio is muted after the component is destroyed and then recreated [\#283](https://github.com/tizonia/tizonia-openmax-il/issues/283)
- opus\_decoder: handling port settings changes is missing [\#282](https://github.com/tizonia/tizonia-openmax-il/issues/282)
- opus\_decoder: disable -\> enable sequence is broken [\#279](https://github.com/tizonia/tizonia-openmax-il/issues/279)
- vorbis\\_decoder: handling of port flush, enable and disable events [\#278](https://github.com/tizonia/tizonia-openmax-il/issues/278)
- player: spotify playback won't start after the spotify storage area has been cleaned \(or on a first install\) [\#276](https://github.com/tizonia/tizonia-openmax-il/issues/276)
- spotify\_source: per-user and per-account spotify cache locations [\#275](https://github.com/tizonia/tizonia-openmax-il/issues/275)
- player: spotify playback won't start if playlist name is not a perfect match AND "invalid" playlists exist in the container     [\#273](https://github.com/tizonia/tizonia-openmax-il/issues/273)
- player: oggopusgraph renders PCM at the original sampling rate instead of at the decoders rate [\#271](https://github.com/tizonia/tizonia-openmax-il/issues/271)
- opus\_decoder: opus decoder expects a comment header at beginning of stream \(which may or may not be there\) [\#270](https://github.com/tizonia/tizonia-openmax-il/issues/270)
- dirble: 'AttributeError' while processing station stream dictionaries that have no 'content\_type' key [\#256](https://github.com/tizonia/tizonia-openmax-il/issues/256)
- spotify: playlist processing stalls when a track is not available in Spotify [\#255](https://github.com/tizonia/tizonia-openmax-il/issues/255)

**Closed issues:**

- Spotify connection problem. Error 410 [\#272](https://github.com/tizonia/tizonia-openmax-il/issues/272)
- No pluginsdir variable in pkg-config file. [\#248](https://github.com/tizonia/tizonia-openmax-il/issues/248)

## [v0.5.0](https://github.com/tizonia/tizonia-openmax-il/tree/v0.5.0) (2016-06-17)
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