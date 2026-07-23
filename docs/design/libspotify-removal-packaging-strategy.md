# Libspotify Removal Blast Radius and V1 Packaging Strategy

Issue #859 is a design-only prerequisite for removing Tizonia's
libspotify-based Spotify streaming support in v1. The implementation issues
should not treat `-Dlibspotify=false` as only a CI convenience; the v1 target is
that Spotify/libspotify is absent from the normal build, package set, installed
configuration, and documented product surface.

## Current Shape

Spotify support is split across three layers:

- The player owns the user surface. `player/src/tizprogramopts.cpp` registers
  `--spotify-*` CLI options and reads `spotify.*` keys from `tizonia.conf`.
  `player/src/tizplayapp.cpp` wires those options to `spotify_stream()` and
  instantiates `tiz::graph::spotifyconfig` and `tiz::graphmgr::spotifymgr` when
  `HAVE_LIBSPOTIFY` is enabled.
- The player graph owns the in-process OpenMAX flow.
  `player/src/services/spotify/` creates a graph around
  `OMX.Aratelia.audio_source.spotify.pcm`, retrieves PCM format metadata from
  the source component, and tunnels it to the configured audio renderer.
- The Spotify source plugin owns actual playback.
  `plugins/spotify_source/` depends on `libspotify >= 12.1.51`, includes
  `<libspotify/api.h>`, embeds the Spotify app key, manages a libspotify
  session/cache, and uses `clients/spotify/libtizspotify` plus
  `clients/spotify/spotifyproxy/tizspotifyproxy.py` to build the playback queue
  from Spotify Web API metadata.

That makes the removal larger than deleting one plugin directory: the public
OpenMAX extension header, resource-manager database, config template, CLI help,
shell completions, docs, Meson/autotools graphs, and Debian package metadata all
advertise the feature today.

## Removal Blast Radius

| Surface | Current references | Removal guidance |
| --- | --- | --- |
| Meson build defaults | `meson_options.txt` defaults `libspotify=true` and includes `spotify` in the default plugin list. `meson.build` sets `-DHAVE_LIBSPOTIFY`, filters the Spotify plugin only when the option is false, and disables libspotify when `clients=false`. | For a transitional removal commit, make the default `false` and remove `spotify` from the default plugin list. For the final v1 removal, delete the option and every `enable_libspotify` branch. |
| Meson subdirs | `clients/meson.build` enters `clients/spotify` only when `enable_libspotify`; `plugins/meson.build` enters `plugins/spotify_source` only when `enable_libspotify`; `player/src/meson.build` installs and compiles Spotify service sources only when enabled. | These are cleanly severable because Meson already has the negative path. Remove the guarded subdirs/sources after the default-off change is green. |
| Autotools graph | Root, plugin, and player `configure.ac` files default `--with-libspotify` to yes. `plugins/Makefile.am` and `player/src/Makefile.am` have conditional Spotify source lists. `clients/configure.ac` and `clients/Makefile.am` still list `spotify` unconditionally. | This is more entangled than Meson. The v1 path should either delete `clients/spotify` from the legacy clients graph or add the same conditional before removing code. Root/player/plugins defaults should become no during any transitional phase. |
| Player CLI and config | `tizprogramopts.cpp` defines the Spotify help topic, options, validation, `spotify_playlist_type()`, config reads, and the `spotify-stream` dispatch. Most option registration is under `HAVE_LIBSPOTIFY`, but the class still carries Spotify members. | Remove the `--spotify-*` options, the `spotify` help topic, Spotify config reads, and the Spotify-specific validation/playlist helpers. The product should reject old Spotify options as unknown rather than silently doing nothing. |
| Player service graph | `player/src/services/spotify/` contains the manager, graph, FSM, graph operations, and config object for the PCM Spotify graph. `tizplayapp.cpp` has guarded includes and an `#else` no-op `spotify_stream()` stub. | Cleanly severable once the CLI path is removed. Delete the service directory and remove the no-op stub so there is no apparent runtime support left behind. |
| OpenMAX extension ABI | `include/omxil-1.2/OMX_TizoniaExt.h` defines the Spotify role, vendor indexes, playlist enums, bitrate enums, session params, and playlist params. | Entangled because this is an installed public header. For v1, either remove the symbols as an explicit ABI break or leave deprecated symbols for one release while ensuring no built component uses them. Do not remove these before all player/plugin references are gone. |
| Resource manager registration | `rm/tizrmd/data/tizonia-rm-db-initial.sql3` registers `OMX.Aratelia.audio_source.spotify.pcm`. | Remove with the plugin. A stale row would advertise a component that packages no longer install, producing runtime lookup failures. |
| Plugin implementation | `plugins/spotify_source/` builds `libtizspotifysrc`, depends on `libspotify`, and exports the Spotify PCM source component. | Delete from the v1 build and package set. This is the only layer with a direct `libspotify` SDK dependency. |
| Spotify client library | `clients/spotify/libtizspotify` builds the C/C++ wrapper, embeds Python via Boost.Python, imports `tizspotifyproxy`, and exposes the `tiz_spotify_*` C API consumed by the plugin. | Delete with the plugin. It does not stream by itself in the product; it exists to supply queue metadata to the libspotify source. |
| Python proxy | `clients/spotify/spotifyproxy/tizspotifyproxy.py` uses `spotipy` and `fuzzywuzzy` to resolve tracks, albums, playlists, recommendations, and user-library queues. | Delete with `libtizspotify`. Do not remove Python or Boost.Python globally; other service clients still use Python-facing code. |
| Installed config | `config/src/tizonia.conf.in` documents Spotify credentials, explicit-track policy, bitrate, and notes that proxy settings currently apply only to Spotify. | Remove the commented Spotify block and reword or remove the proxy note. Existing user config files may retain ignored `spotify.*` keys; this is not an obsolete conffile because the file is shared. |
| Product docs and completions | Sphinx manual pages, help-topic docs, `PROJECT.md`, `BUILDING_with_meson.md`, bash/zsh completions, zsh helper aliases, and the desktop comment mention Spotify. | Remove the Spotify manual page from the toctree, drop completions and helper aliases, and update overview text/release notes. |
| Debian packaging | `config/debian/control` includes `python3-tizspotifyproxy`, `libtizspotify0`, and `libtizspotifysrc0` in `tizonia-all`. `plugins/spotify_source/debian/control` Build-Depends on `libspotify-dev` and `libtizspotify-dev`. Separate Debian directories exist for `python3-tizspotifyproxy`, `libtizspotify0`, and `libtizspotifysrc0`. | Drop the Spotify binary packages and metapackage dependencies. Remove `libspotify-dev`, `libspotify12`, `libtizspotify-dev`, and Spotify package descriptions from the v1 packaging scripts. |
| Packaging helper scripts | `tools/tizonia-common.inc`, `tools/tizonia-dpkg-build`, `tools/install.sh`, `tools/tizonia-gdebi-local`, and Arch `pkgbuild/` files include Spotify package names, Mopidy repository setup, or `libspotify` installation. | Remove Spotify projects from helper arrays and stop adding the Mopidy/libspotify workaround for v1 packaging. |

## Build-Graph Impact

The Meson negative path is already the safest starting point:

- `-Dlibspotify=false` prevents `-DHAVE_LIBSPOTIFY`.
- `clients/spotify`, `plugins/spotify_source`, and Spotify player graph sources
  are skipped.
- The top-level code also forces `enable_libspotify=false` when
  `-Dclients=false`, which is why the current core CI can avoid the SDK.

To make "removed" the default instead of a flag, the implementation should do
the following in order:

1. Flip `option('libspotify', value: 'false')` and remove `spotify` from the
   default `plugins` array. This turns the existing negative path into the
   default v1 build without deleting code yet.
2. Verify the default Meson setup and the existing core-only CI invocation still
   configure without trying to discover `libspotify`.
3. Delete the guarded Spotify Meson subdirs/sources and remove `enable_libspotify`
   after the code removal is committed.
4. Keep the removal isolated from unrelated client dependencies. Python,
   Boost.Python, and service-client infrastructure are shared by non-Spotify
   services.

Autotools needs a more deliberate cleanup because `clients/configure.ac` and
`clients/Makefile.am` enter `spotify` unconditionally. For legacy build support,
the minimum v1 path is:

1. Change root, plugin, and player `--with-libspotify` defaults from yes to no.
2. Stop descending into `clients/spotify` by default.
3. Remove `plugins/spotify_source` from the conditional plugin subdir list when
   deleting the code.
4. Remove `HAVE_LIBSPOTIFY` and the conditional source variables once the code is
   gone.

If v1 chooses Meson as the only supported packaging path, the autotools cleanup
can be limited to avoiding broken default configuration rather than preserving an
opt-in Spotify path.

## Ubuntu 24.04 Packaging Strategy

Ubuntu 24.04 Noble does not provide `libspotify` packages in the Ubuntu package
archive. The package search for `libspotify` in Noble returns no results, and a
direct lookup for `libspotify-dev` reports no such package:

- https://packages.ubuntu.com/search?keywords=libspotify&searchon=names&suite=noble&section=all
- https://packages.ubuntu.com/noble/libspotify-dev

Removing Spotify simplifies the v1 Debian packaging in four ways:

- `tizonia-all` can drop `python3-tizspotifyproxy`, `libtizspotify0`, and
  `libtizspotifysrc0`.
- `plugins/spotify_source/debian/control` and
  `clients/spotify/libtizspotify/debian/control` no longer need
  `libspotify-dev`, `libtizspotify-dev`, or the Spotify runtime/debug packages.
- Build helpers can stop adding the Mopidy APT archive or fallback
  `libspotify-12.1.51` tarball path.
- Package descriptions can stop advertising Spotify support, which avoids a
  runtime promise the Noble package set cannot satisfy.

There is no standalone Spotify conffile to remove. The installed conffile is the
shared `/etc/xdg/tizonia/tizonia.conf`; the v1 package should remove the
commented Spotify block from the template but leave existing user-owned
`spotify.*` keys alone. Release notes should say those keys are ignored in v1 and
can be deleted manually.

## Product-Surface Changes

Users should see a clear removal rather than a broken hidden feature:

- Remove the `spotify` help topic and all `--spotify-*` options from generated
  help, bash completion, zsh completion, and `player/tools/tizonia`.
- Remove zsh helper functions and aliases such as `spotify-playlist` and
  `spotify`.
- Remove Spotify examples from the manual, Docker docs, `PROJECT.md`, desktop
  metadata, and development diagrams.
- Update `tizonia.conf` comments so credentials and Spotify-only proxy behavior
  are no longer presented as configurable.
- Remove the resource-manager component row so `tizonia --comp-list` no longer
  advertises `OMX.Aratelia.audio_source.spotify.pcm`.

Suggested release-note wording:

> Tizonia v1 removes the legacy libspotify-based Spotify streaming backend. The
> upstream SDK is obsolete and is not available from the Ubuntu 24.04 package
> archive, so v1 packages no longer ship Spotify CLI options, configuration, or
> OpenMAX components. Local playback, streaming server/client playback, and the
> remaining supported service backends are unchanged.

## Sequencing

The safest implementation order is:

1. Make the existing no-Spotify build path the default in Meson and, if retained,
   autotools. Keep code in place for this step.
2. Remove product entry points: CLI options, help topic, config reads, shell
   completions, and docs that cause users to invoke Spotify.
3. Remove player Spotify service code and the `spotify_stream()` handler.
4. Remove `plugins/spotify_source`, `clients/spotify`, and their build subdirs.
5. Remove the resource-manager component registration in the same change set as
   the plugin removal.
6. Remove or deprecate public OpenMAX Spotify extension symbols after all
   internal references are gone.
7. Remove Debian package stanzas, metapackage dependencies, helper-script project
   entries, and `libspotify` installation workarounds.
8. Finish with release notes and manual cleanup.

## Risks and Guardrails

- A stale resource-manager row would make the runtime advertise a component that
  cannot be loaded.
- A stale metapackage dependency would make Ubuntu 24.04 installation fail even
  if the source tree builds.
- Removing public OpenMAX symbols too early could break compilation in files that
  still include Spotify playlist/session types.
- Removing shared Python or Boost.Python dependencies because Spotify is gone
  would break other service clients.
- Leaving the CLI handler stub in place would make `--spotify-*` behavior
  confusing; old options should be absent and fail as unknown options.
- Existing user config files may contain stale `spotify.*` keys. The application
  should ignore them after the CLI surface is removed, and release notes should
  call out the cleanup.

## Open Questions for Implementation

- Should v1 remove the OpenMAX Spotify extension symbols immediately as an ABI
  break, or leave deprecated definitions until a later cleanup?
- Is the legacy autotools build still supported for v1 packages, or can the
  removal focus on Meson while keeping autotools only buildable by default?
- Should the first implementation PR flip the default and remove the user
  surface, with source deletion in a follow-up, or should the feature be deleted
  in one larger PR after review?
