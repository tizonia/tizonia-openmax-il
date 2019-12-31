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

# Tizonia's Spotify tracks playback
spotify-tracks() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --spotify-tracks=\""$@"\"
}

# Tizonia's Spotify artist search
spotify-artist() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --spotify-artist=\""$@"\"
}

# Tizonia's Spotify album search
spotify-album() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --spotify-album=\""$@"\"
}

# Tizonia's Spotify playlist playback
spotify-playlist() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --spotify-playlist=\""$@"\"
}

# Tizonia's Spotify playback by track id
spotify-tracks-id() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --spotify-track-id=\""$@"\"
}

# Tizonia's Spotify Spotify playback by artist id
spotify-artist-id() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --spotify-artist-id=\""$@"\"
}

# Tizonia's Spotify playback by album id
spotify-album-id() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --spotify-album-id=\""$@"\"
}

# Tizonia's Spotify playback by playlist id
spotify-playlist-id() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --spotify-playlist-id=\""$@"\"
}

# Tizonia's Spotify related artist search
spotify-related-artists() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --spotify-related-artists=\""$@"\"
}

# Tizonia's Spotify featured playlist search
spotify-featured-playlist() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --spotify-featured-playlist=\""$@"\"
}

# Tizonia's Spotify new releases search
spotify-new-releases() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --spotify-new-releases=\""$@"\"
}

# Tizonia's Spotify recommendations by track id
spotify-recommendations-by-track-id() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --spotify-recommendations-by-track-id=\""$@"\"
}

# Tizonia's Spotify recommendations by artist id
spotify-recommendations-by-artist-id() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --spotify-recommendations-by-artist-id=\""$@"\"
}

# Tizonia's Spotify recommendations by genre
spotify-recommendations-by-genre() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --spotify-recommendations-by-genre=\""$@"\"
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

# Tizonia's Google Play Music entire library playback
gmusic-library() {
    eval tizonia "$(tiz-grab-env-options)" --gmusic-library
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

# Tizonia's Google Play Music free station search
gmusic-station() {
    tiz-check-empty-params "$@" || return
    eval tizonia "$(tiz-grab-env-options)" --gmusic-station=\""$@"\"
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
