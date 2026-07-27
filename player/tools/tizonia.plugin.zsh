# Tizonia

tiz-grab-env-options () {
    local opts=''
    [[ "$SHUFFLE" == 'on' ]] && opts='--shuffle';
    [[ "$DAEMON" == 'on' ]] && opts="$opts --daemon";
    [[ "$CAST" == 'on' ]] && opts="$opts --cast $CAST_DEVICE";
    [[ "$ALTLOGDIR" == 'on' ]] && opts="$opts --log-directory=/tmp";
    echo "$opts"
}

tiz-check-empty-params () {
    if [[ $# -eq 0 ]]; then
        echo "No arguments provided"
        return 1
    fi
    return 0
}

# Tizonia's Tunein popular radio station search
tunein-popular() {
    eval tizonia "$(tiz-grab-env-options)" --tunein-popular-stations
}

# Tizonia's Tunein station search
tunein-station() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --tunein-station=\""$@"\"
}

# Tizonia's Tunein category search
tunein-category() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --tunein-category=\""$@"\"
}

# Tizonia's Tunein country search
tunein-country() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --tunein-country=\""$@"\"
}

# Tizonia's YouTube audio playback (using a YouTube video id)
youtube-search() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --youtube-audio-search=\""$@"\"
}

# Tizonia's YouTube mix playback (using a YouTube mix id)
youtube-mix() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --youtube-audio-mix=\""$@"\"
}

# Tizonia's YouTube audio playlist playback (using a YouTube playlist id)
youtube-playlist() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --youtube-audio-playlist=\""$@"\"
}

# Tizonia's YouTube audio mix search
youtube-mix-search() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --youtube-audio-mix-search=\""$@"\"
}

# Tizonia's YouTube audio channel playlist playback (arg = '<channel-name[space]playlist-name>')
youtube-channel-playlist() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --youtube-audio-channel-playlist=\""$@"\"
}

# Tizonia's YouTube audio channel uploads playback (arg = 'channel url or name')
youtube-channel-uploads() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --youtube-audio-channel-uploads=\""$@"\"
}

# Tizonia's Plex server music tracks search
plex-tracks() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --plex-audio-tracks=\""$@"\"
}

# Tizonia's Plex server music artist search
plex-artist() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --plex-audio-artist=\""$@"\"
}

# Tizonia's Plex server music album search
plex-album() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --plex-audio-album=\""$@"\"
}

# Tizonia's Plex server music playlist search
plex-playlist() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --plex-audio-playlist=\""$@"\"
}
