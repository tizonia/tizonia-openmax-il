# v1 core-only CI scope

## Purpose

PR [#832](https://github.com/tizonia/tizonia-openmax-il/pull/832) added the
first v1 CI signal: a minimal Ubuntu 24.04 Meson build in
[`.github/workflows/ci.yml`](../../.github/workflows/ci.yml). The goal is to
prove that the revived core still configures and compiles on the v1 amd64
baseline without reintroducing every historical player, client, service, codec,
or documentation dependency at once.

This is intentionally a narrow green/red signal. A green build means the core
build graph still compiles with the declared Ubuntu packages and with Meson
wrap downloads disabled. A red build should usually point to a core regression,
a missing declared core dependency, or a CI image/toolchain change. It should
not be caused by optional service SDKs, media stacks, or application features
that are outside the v1 core scope.

## CI invocation

The workflow configures one build directory with:

```sh
meson setup build \
  --buildtype=plain \
  --wrap-mode=nodownload \
  -Dplayer=false \
  -Dclients=false \
  -Ddocs=false \
  -Dtest=false \
  -Dplugins=[]
```

It then runs `meson compile -C build`. The dependency install step is derived
from that core-only configuration and keeps only the packages needed by the
remaining Meson graph: compiler/build tooling plus DBus, expat, log4c, uuid,
sqlite, Python, and curl development files.

With those options, the root Meson build still enters the core project areas:
`3rdparty`, `include`, `libtizplatform`, `rm`, `libtizcore`, `libtizonia`,
`plugins`, and `config`. Because the plugin option is an empty array, the
`plugins` directory does not enable any plugin subdirectories.

## Deliberate exclusions

The disabled options in
[`meson_options.txt`](../../meson_options.txt) are part of the design, not a
temporary CI shortcut.

- `-Dplayer=false` leaves out the command-line application layer. The v1 CI is
  checking the core libraries first, not the full user-facing player stack.
- `-Dclients=false` leaves out cloud and service client libraries and proxies
  such as Google Music, SoundCloud, YouTube, Plex, Chromecast, TuneIn,
  and iHeart. Those integrations carry service-specific APIs and heavier
  third-party requirements that are not part of the v1 amd64 baseline.
- `-Dplugins=[]` leaves out all OpenMAX IL plugins, including service,
  codec/source/sink, ALSA, PulseAudio, and renderer plugins. That avoids making
  the first signal depend on optional media SDKs, audio backends, codec stacks,
  or service credentials.
- `-Ddocs=false` avoids pulling Sphinx/Doxygen documentation tooling into the
  core compile signal.
- `-Dtest=false` avoids treating the historical test targets as part of the
  first revival gate. Tests should be brought back deliberately once their
  dependencies and expected coverage are reviewed.

Keeping these off prevents the v1 CI from becoming a broad packaging exercise.
The immediate need is to keep the core portable and buildable on the current
Ubuntu amd64 runner, with no network fetches from Meson wraps and no reliance on
obsolete or large optional SDKs.

## Signal policy

Do not widen this workflow just because a dependency can be installed. The job
exists to answer one question: does the minimal Tizonia core still compile on
the supported v1 runner? If a future change needs player, clients, plugins,
docs, or tests, it should either add a separately named job with its own
rationale or update this design note and workflow together.

This boundary also keeps failures actionable for contributors and agents. A
failure in the core job should be investigated as a core build problem until
proven otherwise. Optional integration failures belong in integration-specific
jobs where the dependency surface is visible in the job name, install step, and
Meson options.

## Post-v1 expansion

After the v1 core signal is stable, CI can expand in small, reviewed steps:

- add focused unit or smoke tests for already-built core libraries;
- restore documentation builds if the Python/Sphinx/Doxygen dependency set is
  pinned and owned;
- add selected non-service plugins where Ubuntu 24.04 provides reliable
  packages;
- add player or client coverage only after their dependency and service API
  expectations are explicit; and
- keep removed legacy service paths out of the build graph.

Each expansion should preserve the core job as the fast baseline rather than
turning it into the full historical build.
