# Tizonia

tiz-grab-env-options () {
    local opts=''
    [[ "$SHUFFLE" == 'on' ]] && opts='--shuffle';
    [[ "$DAEMON" == 'on' ]] && opts="$opts --daemon";
    echo "$opts"
}

tiz-check-empty-params () {
    if [[ $# -eq 0 ]]; then
        echo "No arguments provided"
        return 1
    fi
    return 0
}

# Tizonia's Spotify playlist playback
spotify-playlist() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --spotify-playlist=\""$@"\"
}

# Tizonia's Google Play Music track search
gmusic-tracks() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --gmusic-tracks=\""$@"\"
}

# Tizonia's Google Play Music artist search
gmusic-artist() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --gmusic-artist=\""$@"\"
}

# Tizonia's Google Play Music album search
gmusic-album() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --gmusic-album=\""$@"\"
}

# Tizonia's Google Play Music playlist search
gmusic-playlist() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --gmusic-playlist=\""$@"\"
}

# Tizonia's Google Play Music podcast search
gmusic-podcast() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --gmusic-podcast=\""$@"\"
}

# Tizonia's Google Play Music song search
gmusic-tracks-unlimited() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --gmusic-unlimited-tracks=\""$@"\"
}

# Tizonia's Google Play Music artist unlimited search
gmusic-artist-unlimited() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --gmusic-unlimited-artist=\""$@"\"
}

# Tizonia's Google Play Music album unlimited search
gmusic-album-unlimited() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --gmusic-unlimited-album=\""$@"\"
}

# Tizonia's Google Play Music playlist unlimited search
gmusic-playlist-unlimited() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --gmusic-unlimited-playlist=\""$@"\"
}

# Tizonia's Google Play Music genre search
gmusic-genre-unlimited() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --gmusic-unlimited-genre=\""$@"\"
}

# Tizonia's Google Play Music station search
gmusic-station-unlimited() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --gmusic-unlimited-station=\""$@"\"
}

# Tizonia's Google Play Music situation search
gmusic-activity-unlimited() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --gmusic-unlimited-activity=\""$@"\"
}

# Tizonia's SoundCloud user stream playlist
soundcloud-stream() {
    eval tizonia "$(tiz-grab-env-options)" --soundcloud-user-stream
}

# Tizonia's SoundCloud user likes playlist
soundcloud-likes() {
    eval tizonia "$(tiz-grab-env-options)" --soundcloud-user-likes
}

# Tizonia's SoundCloud creator search
soundcloud-creator() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --soundcloud-creator=\""$@"\"
}

# Tizonia's SoundCloud tracks search
soundcloud-tracks() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --soundcloud-tracks=\""$@"\"
}

# Tizonia's SoundCloud playlist search
soundcloud-playlists() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --soundcloud-playlists=\""$@"\"
}

# Tizonia's SoundCloud creator search
soundcloud-genres() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --soundcloud-creator=\""$@"\"
}

# Tizonia's SoundCloud creator search
soundcloud-tags() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --soundcloud-tags=\""$@"\"
}

dirble-popular() {
    eval tizonia "$(tiz-grab-env-options)" --dirble-popular-stations
}

dirble-station() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --dirble-station=\""$@"\"
}

dirble-category() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --dirble-category=\""$@"\"
}

dirble-country() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --dirble-country=\""$@"\"
}

youtube-search() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --youtube-audio-search=\""$@"\"
}

youtube-mix() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --youtube-audio-mix=\""$@"\"
}

youtube-playlist() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --youtube-audio-playlist=\""$@"\"
}

youtube-mix-search() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --youtube-audio-mix-search=\""$@"\"
}

deezer-tracks() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --deezer-tracks=\""$@"\"
}

deezer-album() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --deezer-album=\""$@"\"
}

deezer-artist() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --deezer-artist=\""$@"\"
}

deezer-top-playlist() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --deezer-top-playlist=\""$@"\"
}

deezer-mix() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --deezer-mix=\""$@"\"
}

deezer-user-flow() {
    eval tizonia "$(tiz-grab-env-options)" --deezer-user-flow
}

deezer-user-playlist() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --deezer-user-playlist=\""$@"\"
}

alias s='spotify-playlist'
alias spotify='spotify-playlist'
alias artist='gmusic-artist-unlimited'
alias album='gmusic-album-unlimited'
alias song='gmusic-tracks-unlimited'
alias track='gmusic-tracks-unlimited'
alias genre='gmusic-genre-unlimited'
alias station='gmusic-station-unlimited'
alias activity='gmusic-activity-unlimited'
alias playlist='gmusic-playlist-unlimited'
