#include <mp4v2/mp4v2.h>

typedef enum mp4_track_type mp4_track_type_t;
enum mp4_track_type
{
  mp4_track_audio,
  mp4_track_video,
  mp4_track_hint,
  mp4_track_cntl,
  mp4_track_od,
  mp4_track_scene,
  mp4_track_unknown
};

typedef enum mp4_audio_type mp4_audio_type_t;
enum mp4_audio_type
{
  mp4_audio_mp3,
  mp4_audio_aac,
  mp4_audio_aac_from_mov,
  mp4_audio_amr,
  mp4_audio_amrwb,
  mp4_audio_unknown
};

typedef enum mp4_video_type mp4_video_type_t;
enum mp4_video_type
{
  mp4_video_mpeg1,
  mp4_video_mpeg2,
  mp4_video_mpeg4,
  mp4_video_unknown
};

char *
mp4_get_audio_info (MP4FileHandle mp4File, MP4TrackId trackId,
                    mp4_audio_type_t * audioType, uint32_t * timeScale,
                    MP4Duration * trackDuration, double * msDuration,
                    uint32_t * avgBitRate);
