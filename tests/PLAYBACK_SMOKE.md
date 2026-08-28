# Playback smoke checks

The CI-safe `playback-smoke` Meson test generates short MP3, FLAC,
Ogg/Vorbis, Opus, and WAV/PCM fixtures in a temporary directory. It checks
OpenMAX component and role discovery, plays every fixture through the ALSA
`null` device, and serves the MP3 fixture from a loopback HTTP server. The
server is started and stopped by the test, and no cloud credentials or
external network access are required.

Run the suite with:

```bash
meson setup build -Dtest=true -Ddocs=false
meson compile -C build -j1
meson test -C build --suite playback-smoke --print-errorlogs
```

The legacy YouTube tests remain available for explicit live-service testing,
but are excluded from the default test traversal. Enable them only when
network access and the required credentials are available:

```bash
meson setup build-live -Dtest=true -Dlive-service-tests=true -Ddocs=false
meson test -C build-live --print-errorlogs
```

## Manual audio-device checks

The following checks require a real audio session or hardware and are not
part of CI. Create a configuration with the desired renderer and run a local
fixture or another known-good audio file.

ALSA hardware:

```bash
cat >/tmp/tizonia-alsa.conf <<'EOF'
[ilcore]
component-paths = /usr/lib/x86_64-linux-gnu/tizonia0-plugins12;
[plugins]
OMX.Aratelia.audio_renderer.alsa.pcm.alsa_device = default
[tizonia]
default-audio-renderer = OMX.Aratelia.audio_renderer.alsa.pcm
EOF
TIZONIA_RC_FILE=/tmp/tizonia-alsa.conf tizonia ./sample.mp3
```

Expected result: the file plays through the configured ALSA device. If the
device is unavailable, ALSA reports the device error and the check is manual
failure, not a CI failure.

PulseAudio:

```bash
pactl info
cat >/tmp/tizonia-pulse.conf <<'EOF'
[ilcore]
component-paths = /usr/lib/x86_64-linux-gnu/tizonia0-plugins12;
[tizonia]
default-audio-renderer = OMX.Aratelia.audio_renderer.pulseaudio.pcm
EOF
TIZONIA_RC_FILE=/tmp/tizonia-pulse.conf tizonia ./sample.mp3
```

Expected result: `pactl info` reports an active server and the file plays
through PulseAudio. A missing daemon or hardware device is recorded as a
manual unavailable result.
