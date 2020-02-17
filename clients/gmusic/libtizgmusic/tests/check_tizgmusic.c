/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @file   check_tizonia.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia IL Common Unit Tests
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <check.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tizgmusic_c.h"

#define GMUSIC_TEST_TIMEOUT 2500
#define GMUSIC_USER "xxx"
#define GMUSIC_PASS "xxx"
#define GMUSIC_DEVICE_ID "xxx"

#define GMUSIC_ARTIST "Joe Satriani"
#define GMUSIC_ALBUM "Shangri-La"

#define CMD_LEN 1000
#define PLAYER "tizonia"

static bool gmusic_credentials_present (void)
{
  if (!strcmp (GMUSIC_USER, "xxx"))
    {
      return false;
    }
  return true;
}

START_TEST (test_gmusic_play_artist)
{
  tiz_gmusic_t *p_gmusic = NULL;
  bool unlimited_search = false;
  int rc
      = tiz_gmusic_init (&p_gmusic, GMUSIC_USER, GMUSIC_PASS, GMUSIC_DEVICE_ID);
  ck_assert (0 == rc);
  ck_assert (p_gmusic);

  rc = tiz_gmusic_play_artist (p_gmusic, GMUSIC_ARTIST, unlimited_search);
  ck_assert (0 == rc);

  /* while (1) */
  {
    /*     char cmd[CMD_LEN]; */
    const char *next_url = tiz_gmusic_get_next_url (p_gmusic);
    ck_assert (next_url);
    fprintf (stderr, "url = %s\n", next_url);

    const char *artist = tiz_gmusic_get_current_song_artist (p_gmusic);
    ck_assert (artist);
    fprintf (stderr, "artist = %s\n", artist);

    const char *title = tiz_gmusic_get_current_song_title (p_gmusic);
    ck_assert (title);
    fprintf (stderr, "title = %s\n", title);

    /*     snprintf (cmd, CMD_LEN, "%s \"%s\"", PLAYER, next_url); */
    /*     fprintf (stderr, "cmd = %s\n", cmd); */
    /*     ck_assert (-1 != system (cmd)); */
  }

  tiz_gmusic_destroy (p_gmusic);
}
END_TEST

START_TEST (test_gmusic_play_album)
{
  tiz_gmusic_t *p_gmusic = NULL;
  bool unlimited_search = false;
  int rc
      = tiz_gmusic_init (&p_gmusic, GMUSIC_USER, GMUSIC_PASS, GMUSIC_DEVICE_ID);
  ck_assert (0 == rc);
  ck_assert (p_gmusic);

  rc = tiz_gmusic_play_album (p_gmusic, GMUSIC_ALBUM, unlimited_search);
  ck_assert (0 == rc);

  /* while (1) */
  {
    /*     char cmd[CMD_LEN]; */
    const char *next_url = tiz_gmusic_get_next_url (p_gmusic);
    ck_assert (next_url);
    fprintf (stderr, "url = %s\n", next_url);

    const char *artist = tiz_gmusic_get_current_song_artist (p_gmusic);
    ck_assert (artist);
    fprintf (stderr, "artist = %s\n", artist);

    const char *title = tiz_gmusic_get_current_song_title (p_gmusic);
    ck_assert (title);
    fprintf (stderr, "title = %s\n", title);
    /*     snprintf (cmd, CMD_LEN, "%s \"%s\"", PLAYER, next_url); */
    /*     fprintf (stderr, "cmd = %s\n", cmd); */
    /*     ck_assert (-1 != system (cmd)); */
  }

  tiz_gmusic_destroy (p_gmusic);
}
END_TEST

Suite *gmusic_suite (void)
{
  TCase *tc_gmusic;
  Suite *s = suite_create ("libtizgmusic");

  /* test case */
  tc_gmusic = tcase_create ("Google Music client lib unit tests");
  tcase_add_test (tc_gmusic, test_gmusic_play_artist);
  tcase_add_test (tc_gmusic, test_gmusic_play_album);
  suite_add_tcase (s, tc_gmusic);

  return s;
}

int main (void)
{
  int number_failed = 1;
  if (gmusic_credentials_present ())
    {
      SRunner *sr = srunner_create (gmusic_suite ());
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
