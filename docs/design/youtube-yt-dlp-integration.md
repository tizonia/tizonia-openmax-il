# YouTube backend integration via yt-dlp

## Purpose

Issue [#858](https://github.com/tizonia/tizonia-openmax-il/issues/858)
is design work for
[#822](https://github.com/tizonia/tizonia-openmax-il/issues/822): replacing
the legacy YouTube backend with `yt-dlp` for the v1 Ubuntu 24.04 target. This
note identifies the smallest useful integration seam and the packaging risks to
settle before implementation.

## Current integration surface

The user-facing YouTube commands are parsed in
[`player/src/tizprogramopts.cpp`](../../player/src/tizprogramopts.cpp). Those
options map to one URI/search value and an
`OMX_TIZONIA_AUDIO_YOUTUBEPLAYLISTTYPE` value such as
`AudioStream`, `AudioPlaylist`, `AudioMix`, `AudioSearch`,
`AudioMixSearch`, `AudioChannelUploads`, or `AudioChannelPlaylist`.

For local playback,
[`tiz::playapp::youtube_stream`](../../player/src/tizplayapp.cpp) wraps those
values in a
[`tiz::graph::youtubeconfig`](../../player/src/services/youtube/tizyoutubeconfig.hpp)
and starts a
[`graphmgr::youtubemgr`](../../player/src/services/youtube/tizyoutubemgr.cpp).
The manager creates a
[`tiz::graph::youtube`](../../player/src/services/youtube/tizyoutubegraph.cpp)
graph. The graph ops instantiate `OMX.Aratelia.audio_source.http` with the
`audio_source.http.youtube` role in
[`tizyoutubegraphops.cpp`](../../player/src/services/youtube/tizyoutubegraphops.cpp).

The `audio_source.http.youtube` role is registered in
[`plugins/http_source/src/httpsrc.c`](../../plugins/http_source/src/httpsrc.c)
with a YouTube config port and
[`youtubeprc`](../../plugins/http_source/src/youtubeprc.c). During resource
allocation, `youtubeprc` reads the session and playlist OMX parameters,
initializes `tiz_youtube_t`, enqueues the requested YouTube item, obtains the
first resolved URL, and hands that URL to `tiz_urltrans` through an
`OMX_PARAM_CONTENTURITYPE`. Skip and position changes call the same
`tiz_youtube_get_next_url`, `tiz_youtube_get_prev_url`, and
`tiz_youtube_get_url` C functions before restarting the HTTP transfer.

The backend boundary today is `libtizyoutube`:

- [`tizyoutube_c.h`](../../clients/youtube/libtizyoutube/src/tizyoutube_c.h)
  exposes the C ABI used by `youtubeprc`.
- [`tizyoutube.cpp`](../../clients/youtube/libtizyoutube/src/tizyoutube.cpp)
  embeds Python through Boost.Python and calls the Python proxy methods.
- [`tizyoutubeproxy.py`](../../clients/youtube/youtubeproxy/tizyoutubeproxy.py)
  is the resolver and queue implementation. It currently imports `pafy`, uses
  `youtube_dl` indirectly, queries the YouTube Data API for some flows, and
  returns stream URLs plus metadata to the C++ wrapper.

The minimal v1 seam is therefore inside `clients/youtube`: keep the
`tizyoutube_c.h` API and `youtubeprc` contract stable, and replace the
`tizyoutubeproxy.py` resolver implementation, plus the dependency checks in
`tizyoutube.cpp`, with `yt-dlp`. The player graph, OMX parameter structs, and
HTTP transfer path should not need a first-pass rewrite if the proxy still
returns HTTP(S) stream URLs and metadata fields compatible with the existing
getters.

Chromecast has a related path:
[`youtube_stream_chromecast`](../../player/src/tizplayapp.cpp) uses the same
`youtubeconfig`, then
[`chromecastops::do_configure_youtube`](../../player/src/services/chromecast/tizchromecastgraphops.cpp)
sets the YouTube playlist on the Chromecast renderer. That path should be smoke
tested after the backend replacement, but the local HTTP source path above is
the main seam for #822.

## Invocation model

There are two practical ways to invoke `yt-dlp`.

Using `yt-dlp` as a subprocess keeps the C/C++ side decoupled from `yt_dlp`
Python internals. The implementation can parse stable machine output from
`yt-dlp -J` or focused `--print` fields rather than scraping normal progress
output. It also makes packaging easy to explain: depend on the `yt-dlp` binary
package. The trade-off is process startup for every resolution, more careful
quoting and timeout handling, awkward cookie/auth option handling if command
arguments expose paths or secrets, and a need to map process exit status,
stderr, and partial JSON into Tizonia's existing error messages.

Using `yt_dlp` as a Python module is the narrower code change because
`libtizyoutube` already embeds Python and already delegates queue behavior to
`tizyoutubeproxy.py`. The proxy can call `yt_dlp.YoutubeDL.extract_info(...,
download=False)`, select a format entry, and populate the existing queue
objects without adding another process layer. The Ubuntu `yt-dlp` package also
installs the `yt_dlp` Python package under `/usr/lib/python3/dist-packages`.
The trade-off is tighter coupling to `yt_dlp`'s Python API and info-dict shape;
the implementation should sanitize or normalize only the fields Tizonia needs
instead of passing raw `yt-dlp` structures across the C++ boundary.

For v1, prefer module invocation inside `tizyoutubeproxy.py` unless a clean
Ubuntu 24.04 smoke test shows a packaging or runtime problem. Keep the
subprocess model as a fallback design because upstream documents `yt-dlp` as
callable from any language and warns callers to use `-J`, `--print`, or similar
stable outputs rather than normal stdout.

## Stream resolution and playback constraints

`yt-dlp` should resolve each queue item to a direct HTTP(S) media URL. The
existing playback pipeline does not download files through `yt-dlp`; it expects
the YouTube source to place the resolved URL into `OMX_PARAM_CONTENTURITYPE`,
then `tiz_urltrans` streams the bytes with libcurl.

The current source processor detects the container from HTTP response headers.
In `youtubeprc.c`, `audio/webm` maps to `OMX_AUDIO_CodingWEBM` and `audio/mp4`
maps to `OMX_AUDIO_CodingMP4`. The YouTube graph currently wires WebM through
`OMX.Aratelia.container_demuxer.webm`, but MP4 is still a TODO in
`youtubeops::add_demuxer_to_component_list`. That means the v1 replacement
should be WebM-first for local playback.

Recommended initial format policy:

- Prefer audio-only WebM over HTTP(S), for example a `yt-dlp` selector shaped
  around `bestaudio[ext=webm][protocol^=http]`.
- Allow only formats whose final URL is `http://` or `https://`, matching the
  existing `youtubeprc` URI validation.
- Treat DASH/HLS manifests and non-WebM containers as deferred unless the
  graph gains the corresponding demuxer path and smoke coverage.
- Preserve existing metadata getters for title, author/uploader, duration,
  bitrate, view count, description, extension, YouTube URL/id, published date,
  queue index, and queue length. Some values may be absent in `yt-dlp` results;
  the proxy should normalize missing values to the same empty-string or zero
  behavior used today.

Playlist, search, mix, and channel flows are not equivalent across `pafy` and
`yt-dlp`. For v1, a good implementation target is:

- `--youtube-audio-stream`: resolve one video URL or id.
- `--youtube-audio-playlist`: expand a playlist URL/id into queue entries.
- `--youtube-audio-search`: use `ytsearch` and queue a bounded result set.
- Existing mix and channel commands remain accepted, but can degrade to a clear
  "not available in the yt-dlp backend yet" error if a reliable mapping is not
  obvious during #822.

## Dependency and packaging impact

Ubuntu 24.04 has `yt-dlp` in `noble/universe` as `2024.04.09-1`, and
`noble-backports/universe` has `2025.08.11-1~bpo24.04.1`. The package file list
includes both `/usr/bin/yt-dlp` and the importable `yt_dlp` Python package, and
the package dependencies include Python libraries such as `python3-requests`,
`python3-urllib3`, `python3-websockets`, `python3-mutagen`, and crypto/cert
packages. The same package page recommends `ffmpeg`, but Tizonia's local
playback path should initially avoid `yt-dlp` post-processing and let the
existing OpenMAX graph stream and decode the selected WebM audio.

Packaging work in
[#824](https://github.com/tizonia/tizonia-openmax-il/issues/824) should replace
legacy `youtube-dl`, `pafy`, and `python3-pafy` expectations with a runtime
dependency on `yt-dlp` for the YouTube-capable package set. If the implementation
uses the Python module, #824 should also add an import-level smoke test in the
package build/install verification, not just a binary presence check.

Installer work in
[#825](https://github.com/tizonia/tizonia-openmax-il/issues/825) should keep the
Ubuntu 24.04 script on distro packages rather than installing `yt-dlp` with
`pip` or `yt-dlp -U`. APT-owned packages are easier to audit and uninstall. The
trade-off is update cadence: `noble` can lag YouTube breakages, while backports
is newer but still not as fresh as upstream nightly builds. The installer should
not enable third-party PPAs for v1 by default. If the `noble` package is too old
in smoke tests, prefer documenting or depending on `noble-backports` over
introducing an unmanaged updater.

Current upstream `yt-dlp` documentation also calls out JavaScript helper support
for full YouTube extraction. Ubuntu 24.04 provides JavaScript runtimes such as
`nodejs` and `quickjs`, but `yt-dlp-ejs` is not visible as a Noble package under
that name. Treat this as a packaging open question: do not add a JavaScript
runtime dependency until the #822 smoke test demonstrates a YouTube extraction
failure that needs it.

## Risks and open questions

- YouTube extraction breaks frequently. The v1 package set needs a clear policy
  for whether YouTube support uses `noble`, `noble-backports`, or a documented
  manual override when upstream fixes are needed.
- Rate limiting and bot checks can still fail even with `yt-dlp`. The backend
  should surface extractor failures clearly and avoid infinite retry loops.
- Auth, age-restricted content, private playlists, and cookies are out of scope
  for the initial v1 replacement unless they are needed for public smoke tests.
  If added later, prefer an explicit config file path for cookies over command
  line secrets.
- Existing YouTube Data API key support does not map cleanly to `yt-dlp`.
  Keeping the option for compatibility is reasonable, but #822 should decide
  whether it becomes ignored, deprecated, or used only for any retained search
  API flow.
- Stream URLs can expire. The queue should resolve lazily enough that long
  playlists do not pre-resolve stale URLs, or it should re-resolve on playback
  failure.
- MP4 and HLS/DASH support should be deferred until the graph has matching
  demuxer and smoke coverage. WebM audio is the v1 "good enough" target.
- Tests should cover a successful single-video resolution, an empty or invalid
  result, a backend extraction error, and a format-filter miss that reports why
  playback cannot continue.

## References

- `yt-dlp` upstream README:
  <https://github.com/yt-dlp/yt-dlp>
- Ubuntu 24.04 `yt-dlp` package:
  <https://packages.ubuntu.com/noble/yt-dlp>
- Ubuntu 24.04 `yt-dlp` package file list:
  <https://packages.ubuntu.com/noble/all/yt-dlp/filelist>
- Ubuntu 24.04 backports `yt-dlp` package:
  <https://packages.ubuntu.com/noble-backports/yt-dlp>
