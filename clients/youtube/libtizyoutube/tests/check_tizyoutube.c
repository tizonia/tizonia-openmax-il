/**
 * Copyright (C) 2011-2016 Aratelia Limited - Juan A. Rubio
 *
 * This file is part of Tizonia
 *
 * Tizonia is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Tizonia is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Tizonia.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file   check_tizyoutube.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia YouTube Unit Tests
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <check.h>
#include <assert.h>

#include "tizyoutube_c.h"

#define YOUTUBE_TEST_TIMEOUT 2500

#define CMD_LEN 1000
#define PLAYER "mplayer"
#define YOUTUBE_VIDEO_ID "y3Ca3c6J9N4"
#define YOUTUBE_SEARCH_TERM "queen"

static void dump_info(tiz_youtube_t *p_youtube)
{
  assert(p_youtube);

  {
    const char *title = tiz_youtube_get_current_audio_stream_title (p_youtube);
    ck_assert (title);
    fprintf (stderr, "current_audio_stream_title = %s\n", title);
  }

  {
    const char *author
        = tiz_youtube_get_current_audio_stream_author (p_youtube);
    ck_assert (author);
    fprintf (stderr, "current_audio_stream_author = %s\n", author);
  }

  {
    const char *size
        = tiz_youtube_get_current_audio_stream_file_size (p_youtube);
    ck_assert (size);
    fprintf (stderr, "current_audio_stream_file_size = %s\n", size);
  }

  {
    const char *duration
        = tiz_youtube_get_current_audio_stream_duration (p_youtube);
    ck_assert (duration);
    fprintf (stderr, "current_audio_stream_duration = %s\n", duration);
  }

  {
    const char *bitrate
        = tiz_youtube_get_current_audio_stream_bitrate (p_youtube);
    ck_assert (bitrate);
    fprintf (stderr, "current_audio_stream_bitrate = %s\n", bitrate);
  }

  {
    const char *count
        = tiz_youtube_get_current_audio_stream_view_count (p_youtube);
    ck_assert (count);
    fprintf (stderr, "current_audio_stream_view_count = %s\n", count);
  }

  {
    const char *description
        = tiz_youtube_get_current_audio_stream_description (p_youtube);
    ck_assert (description);
    fprintf (stderr, "current_audio_stream_description = %s\n", description);
  }

  {
    const char *file_extension
        = tiz_youtube_get_current_audio_stream_file_extension (p_youtube);
    ck_assert (file_extension);
    fprintf (stderr, "current_audio_stream_file_extension = %s\n", file_extension);
  }
}

START_TEST (test_youtube_play_audio_stream)
{
  tiz_youtube_t *p_youtube = NULL;
  int rc = tiz_youtube_init (&p_youtube);
  ck_assert (0 == rc);
  ck_assert (p_youtube != NULL);

  rc = tiz_youtube_play_audio_stream (p_youtube, YOUTUBE_VIDEO_ID);
  ck_assert (0 == rc);

  {
    char cmd[CMD_LEN];
    const char *next_url = tiz_youtube_get_next_url (p_youtube, false);
    fprintf (stderr, "url = %s\n", next_url);
    ck_assert (next_url != NULL);

    dump_info(p_youtube);

    /*     snprintf (cmd, CMD_LEN, "%s \"%s\"", PLAYER, next_url); */
    /*     fprintf (stderr, "cmd = %s\n", cmd); */
    /*     ck_assert (-1 != system (cmd)); */
  }

  tiz_youtube_destroy (p_youtube);
}
END_TEST

START_TEST (test_youtube_play_audio_search)
{
  tiz_youtube_t *p_youtube = NULL;
  int rc = tiz_youtube_init (&p_youtube);
  ck_assert (0 == rc);
  ck_assert (p_youtube != NULL);

  rc = tiz_youtube_play_audio_search (p_youtube, YOUTUBE_SEARCH_TERM);
  ck_assert (0 == rc);

  {
    char cmd[CMD_LEN];
    const char *next_url = tiz_youtube_get_next_url (p_youtube, false);
    fprintf (stderr, "url = %s\n", next_url);
    ck_assert (next_url != NULL);

    dump_info(p_youtube);

    /*     snprintf (cmd, CMD_LEN, "%s \"%s\"", PLAYER, next_url); */
    /*     fprintf (stderr, "cmd = %s\n", cmd); */
    /*     ck_assert (-1 != system (cmd)); */
  }

  tiz_youtube_destroy (p_youtube);
}
END_TEST

Suite *
youtube_suite (void)
{
  TCase *tc_youtube;
  Suite *s = suite_create ("libtizyoutube");

  /* test case */
  tc_youtube = tcase_create ("YouTube audio client lib unit tests");
  tcase_set_timeout (tc_youtube, YOUTUBE_TEST_TIMEOUT);
  tcase_add_test (tc_youtube, test_youtube_play_audio_stream);
  tcase_add_test (tc_youtube, test_youtube_play_audio_search);
  suite_add_tcase (s, tc_youtube);

  return s;
}

int main (void)
{
  int number_failed = 1;
  SRunner *sr = srunner_create (youtube_suite ());
  srunner_set_log (sr, "-");
  srunner_run_all (sr, CK_VERBOSE);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* Local Variables: */
/* c-default-style: gnu */
/* fill-column: 79 */
/* indent-tabs-mode: nil */
/* compile-command: "make check" */
/* End: */
