
#include <assert.h>
#include <strings.h>
#include <stdlib.h>

#include "mp4info.h"

#define NUM_ELEMENTS_IN_ARRAY(name) ((sizeof((name))) / (sizeof(*(name))))

static const char * mpeg4AudioNames[] = {
  "MPEG-4 AAC main",
  "MPEG-4 AAC LC",
  "MPEG-4 AAC SSR",
  "MPEG-4 AAC LTP",
  "MPEG-4 AAC HE",
  "MPEG-4 AAC Scalable",
  "MPEG-4 TwinVQ",
  "MPEG-4 CELP",
  "MPEG-4 HVXC",
  NULL,
  NULL,
  "MPEG-4 TTSI",
  "MPEG-4 Main Synthetic",
  "MPEG-4 Wavetable Syn",
  "MPEG-4 General MIDI",
  "MPEG-4 Algo Syn and Audio FX",
  "MPEG-4 ER AAC LC",
  NULL,
  "MPEG-4 ER AAC LTP",
  "MPEG-4 ER AAC Scalable",
  "MPEG-4 ER TwinVQ",
  "MPEG-4 ER BSAC",
  "MPEG-4 ER ACC LD",
  "MPEG-4 ER CELP",
  "MPEG-4 ER HVXC",
  "MPEG-4 ER HILN",
  "MPEG-4 ER Parametric",
  "MPEG-4 SSC",
  "MPEG-4 PS",
  "MPEG-4 MPEG Surround",
  NULL,
  "MPEG-4 Layer-1",
  "MPEG-4 Layer-2",
  "MPEG-4 Layer-3",
  "MPEG-4 DST",
  "MPEG-4 Audio Lossless",
  "MPEG-4 SLS",
  "MPEG-4 SLS non-core",
};

static const uint8_t mpegAudioTypes[] = {
  MP4_MPEG2_AAC_MAIN_AUDIO_TYPE,  // 0x66
  MP4_MPEG2_AAC_LC_AUDIO_TYPE,    // 0x67
  MP4_MPEG2_AAC_SSR_AUDIO_TYPE,   // 0x68
  MP4_MPEG2_AUDIO_TYPE,           // 0x69
  MP4_MPEG1_AUDIO_TYPE,           // 0x6B
  // private types
  MP4_PCM16_LITTLE_ENDIAN_AUDIO_TYPE,
  MP4_VORBIS_AUDIO_TYPE,
  MP4_ALAW_AUDIO_TYPE,
  MP4_ULAW_AUDIO_TYPE,
  MP4_G723_AUDIO_TYPE,
  MP4_PCM16_BIG_ENDIAN_AUDIO_TYPE,
};

static const char * mpegAudioNames[] = {
  "MPEG-2 AAC Main",
  "MPEG-2 AAC LC",
  "MPEG-2 AAC SSR",
  "MPEG-2 Audio (13818-3)",
  "MPEG-1 Audio (11172-3)",
  // private types
  "PCM16 (little endian)",
  "Vorbis",
  "G.711 aLaw",
  "G.711 uLaw",
  "G.723.1",
  "PCM16 (big endian)",
};

char *
mp4_get_audio_info (MP4FileHandle mp4File, MP4TrackId trackId,
                    mp4_audio_type_t * audioType, uint32_t * timeScale,
                    MP4Duration * trackDuration, double * msDuration,
                    uint32_t * avgBitRate)
{

  uint8_t numMpegAudioTypes = sizeof (mpegAudioTypes) / sizeof (uint8_t);
  const char * typeName = "Unknown";
  bool foundType = false;
  uint8_t type = 0;
  const char * media_data_name;

  assert(audioType);
  assert(timeScale);
  assert(trackDuration);
  assert(msDuration);
  assert(avgBitRate);

  media_data_name = MP4GetTrackMediaDataName (mp4File, trackId);

  if (media_data_name == NULL)
    {
      typeName = "Unknown - no media data name";
      *audioType = mp4_audio_unknown;
    } else if (strcasecmp(media_data_name, "samr") == 0) {
        typeName = "AMR";
        foundType = true;
        *audioType = mp4_audio_amr;
    } else if (strcasecmp(media_data_name, "sawb") == 0) {
        typeName = "AMR-WB";
        foundType = true;
        *audioType = mp4_audio_amrwb;
    } else if (strcasecmp(media_data_name, "mp4a") == 0) {
        type = MP4GetTrackEsdsObjectTypeId(mp4File, trackId);
        switch (type) {
        case MP4_INVALID_AUDIO_TYPE:
            typeName = "AAC from .mov";
            foundType = true;
            *audioType = mp4_audio_aac_from_mov;
            break;
        case MP4_MPEG4_AUDIO_TYPE:  {
            type = MP4GetTrackAudioMpeg4Type(mp4File, trackId);
            if (type == MP4_MPEG4_INVALID_AUDIO_TYPE ||
                    type > NUM_ELEMENTS_IN_ARRAY(mpeg4AudioNames) ||
                    mpeg4AudioNames[type - 1] == NULL) {
                typeName = "MPEG-4 Unknown Profile";
                *audioType = mp4_audio_unknown;
            } else {
                typeName = mpeg4AudioNames[type - 1];
                foundType = true;
                if (type >= 0 && type <= 5)
                  {
                    *audioType = mp4_audio_aac;
                  }
                else if (type >= 31 && type <= 33)
                  {
                    *audioType = mp4_audio_mp3;
                  }
                else
                  {
                    *audioType = mp4_audio_unknown;
                  }
            }
            break;
        }
        // fall through
        default:
            {
            uint8_t i;
            for (i = 0; i < numMpegAudioTypes; i++) {
                if (type == mpegAudioTypes[i]) {
                    typeName = mpegAudioNames[i];
                    foundType = true;
                    if (type == MP4_MPEG2_AAC_MAIN_AUDIO_TYPE
                        || type == MP4_MPEG2_AAC_LC_AUDIO_TYPE
                        || type == MP4_MPEG2_AAC_SSR_AUDIO_TYPE)
                      {
                        *audioType = mp4_audio_aac;
                      }
                    else if (type == MP4_MPEG2_AUDIO_TYPE
                        || type == MP4_MPEG1_AUDIO_TYPE)
                      {
                        *audioType = mp4_audio_mp3;
                      }
                    else
                      {
                        *audioType = mp4_audio_unknown;
                      }
                    break;
                }
            }
            }
        }
    } else {
        typeName = media_data_name;
        foundType = true;
    }

    *timeScale =
        MP4GetTrackTimeScale(mp4File, trackId);

    *trackDuration =
        MP4GetTrackDuration(mp4File, trackId);

    *msDuration =
      (double)MP4ConvertFromTrackDuration(mp4File, trackId,
                                           *trackDuration, MP4_MSECS_TIME_SCALE);

    *avgBitRate =
        MP4GetTrackBitRate(mp4File, trackId);

    char *sInfo = (char*)malloc(256);

    // type duration avgBitrate samplingFrequency
    if (foundType)
        snprintf(sInfo, 256,
                 "%u\taudio\t%s%s, %.3f secs, %u kbps, %u Hz\n",
                 trackId,
                 MP4IsIsmaCrypMediaTrack(mp4File, trackId) ? "enca - " : "",
                 typeName,
                 *msDuration / 1000.0,
                 (*avgBitRate + 500) / 1000,
                 *timeScale);
    else
        snprintf(sInfo, 256,
                 "%u\taudio\t%s%s(%u), %.3f secs, %u kbps, %u Hz\n",
                 trackId,
                 MP4IsIsmaCrypMediaTrack(mp4File, trackId) ? "enca - " : "",
                 typeName,
                 type,
                 *msDuration / 1000.0,
                 (*avgBitRate + 500) / 1000,
                 *timeScale);

    return sInfo;
}
