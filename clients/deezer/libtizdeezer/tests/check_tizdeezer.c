/**
 * Copyright (C) 2011-2017 Aratelia Limited - Juan A. Rubio
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
 * @file   check_tizdeezer.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia Deezer Unit Tests
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

#include "tizdeezer_c.h"

#define DEEZER_TEST_TIMEOUT 2500
#define DEEZER_USERNAME     "xxx"
#define DEEZER_PASS         "xxx"

#define DEEZER_PLAYLIST "metal"
#define DEEZER_ALBUM "The Miracle"

static bool deezer_credentials_present (void)
{
  if (!strcmp (DEEZER_USERNAME, "xxx"))
    {
      fprintf (stderr, "Unable to run test. No user name configured\n");
      return false;
    }
  return true;
}

START_TEST (test_deezer_play_album)
{
  tiz_deezer_t *p_deezer = NULL;
  int rc = tiz_deezer_init (&p_deezer, DEEZER_USERNAME);
  FILE *p_file = NULL;
  ck_assert (0 == rc);
  ck_assert (p_deezer);

  fprintf (stderr, "tiz_deezer_init rc = %d\n", rc);

  p_file = fopen ((const char *)"album-track.mp3", "w");
  ck_assert (NULL != p_file);

  rc = tiz_deezer_play_album (p_deezer, DEEZER_ALBUM);
  ck_assert (0 == rc);
  fprintf (stderr, "tiz_deezer_play_album rc = %d\n", rc);

  {
    rc = tiz_deezer_next_track (p_deezer);
    ck_assert (0 == rc);
    fprintf (stderr, "tiz_deezer_next_track rc = %d\n", rc);
  }

  {
    const char *title = tiz_deezer_get_current_track_title (p_deezer);
    ck_assert (title);
    fprintf (stderr, "title = %s\n", title);
  }

  {
    const char *artist = tiz_deezer_get_current_track_artist (p_deezer);
    ck_assert (artist);
    fprintf (stderr, "artist = %s\n", artist);
  }

  {
    unsigned char *p_data = NULL;
    size_t len = tiz_deezer_get_mp3_data (p_deezer, &p_data);
    while (len && p_data)
      {
        fwrite (p_data, len, 1, p_file);
        len = tiz_deezer_get_mp3_data (p_deezer, &p_data);
      }
  }

  fclose (p_file);
  tiz_deezer_destroy (p_deezer);
}
END_TEST

Suite *
deezer_suite (void)
{
  TCase *tc_deezer;
  Suite *s = suite_create ("libtizdeezer");

  /* test case */
  tc_deezer = tcase_create ("Deezer client lib unit tests");
  tcase_set_timeout (tc_deezer, DEEZER_TEST_TIMEOUT);
  tcase_add_test (tc_deezer, test_deezer_play_album);
  suite_add_tcase (s, tc_deezer);

  return s;
}

int main (void)
{
  int number_failed = 1;
  if (deezer_credentials_present ())
    {
      SRunner *sr = srunner_create (deezer_suite ());
      srunner_set_log (sr, "-");
      srunner_run_all (sr, CK_VERBOSE);
      number_failed = srunner_ntests_failed (sr);
      srunner_free (sr);
    }
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* Local Variables: */
/* c-default-style: gnu */
/* fill-column: 79 */
/* indent-tabs-mode: nil */
/* compile-command: "make check" */
/* End: */
