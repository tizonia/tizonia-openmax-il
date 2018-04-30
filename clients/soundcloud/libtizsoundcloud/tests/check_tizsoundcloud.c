/**
 * Copyright (C) 2011-2018 Aratelia Limited - Juan A. Rubio
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
 * @file   check_tizsoundcloud.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia SoundCloud Unit Tests
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

#include "tizsoundcloud_c.h"

#define SOUNDCLOUD_TEST_TIMEOUT 2500
#define SOUNDCLOUD_USERNAME     "xxx"
#define SOUNDCLOUD_PASS         "xxx"

#define SOUNDCLOUD_USER "TWIT"
#define SOUNDCLOUD_PLAYLIST "metal"

#define CMD_LEN 1000
#define PLAYER "tizonia"

static bool soundcloud_credentials_present (void)
{
  if (!strcmp (SOUNDCLOUD_USERNAME, "xxx"))
    {
      return false;
    }
  return true;
}

START_TEST (test_scloud_play_stream)
{
  tiz_scloud_t *p_soundcloud = NULL;
  int rc = tiz_scloud_init (&p_soundcloud, SOUNDCLOUD_USERNAME, SOUNDCLOUD_PASS);
  ck_assert (0 == rc);
  ck_assert (p_soundcloud);

  rc = tiz_scloud_play_stream (p_soundcloud);
  ck_assert (0 == rc);

/*   while (1) */
  {
/*     char cmd[CMD_LEN]; */
    {
      const char *next_url = tiz_scloud_get_next_url (p_soundcloud);
      ck_assert (next_url);
      fprintf (stderr, "url = %s\n", next_url);
    }

    {
      const char *user = tiz_scloud_get_current_track_user (p_soundcloud);
      ck_assert (user);
      fprintf (stderr, "user = %s\n", user);
    }

    {
      const char *title = tiz_scloud_get_current_track_title (p_soundcloud);
      ck_assert (title);
      fprintf (stderr, "title = %s\n", title);
    }

    {
      const char *duration
          = tiz_scloud_get_current_track_duration (p_soundcloud);
      ck_assert (duration);
      fprintf (stderr, "duration = %s\n", duration);
    }

    {
      const char *year = tiz_scloud_get_current_track_year (p_soundcloud);
      ck_assert (year);
      fprintf (stderr, "year = %s\n", year);
    }

    {
      const char *permalink = tiz_scloud_get_current_track_permalink (p_soundcloud);
      ck_assert (permalink);
      fprintf (stderr, "permalink = %s\n", permalink);
    }

    {
      const char *license = tiz_scloud_get_current_track_license (p_soundcloud);
      ck_assert (license);
      fprintf (stderr, "license = %s\n", license);
    }

/*     snprintf (cmd, CMD_LEN, "%s \"%s\"", PLAYER, next_url); */
/*     fprintf (stderr, "cmd = %s\n", cmd); */
/*     ck_assert (-1 != system (cmd)); */
  }

  tiz_scloud_destroy (p_soundcloud);
}
END_TEST

START_TEST (test_scloud_play_creator)
{
  tiz_scloud_t *p_soundcloud = NULL;
  int rc = tiz_scloud_init (&p_soundcloud, SOUNDCLOUD_USERNAME, SOUNDCLOUD_PASS);
  ck_assert (0 == rc);
  ck_assert (p_soundcloud);

  rc = tiz_scloud_play_creator (p_soundcloud, SOUNDCLOUD_USER);
  ck_assert (0 == rc);

/*   while (1) */
  {
/*     char cmd[CMD_LEN]; */

    {
      const char *next_url = tiz_scloud_get_next_url (p_soundcloud);
      ck_assert (next_url);
      fprintf (stderr, "url = %s\n", next_url);
    }

    {
      const char *user = tiz_scloud_get_current_track_user (p_soundcloud);
      ck_assert (user);
      fprintf (stderr, "user = %s\n", user);
    }

    {
      const char *title = tiz_scloud_get_current_track_title (p_soundcloud);
      ck_assert (title);
      fprintf (stderr, "title = %s\n", title);
    }

    {
      const char *duration
          = tiz_scloud_get_current_track_duration (p_soundcloud);
      ck_assert (duration);
      fprintf (stderr, "duration = %s\n", duration);
    }

    {
      const char *year = tiz_scloud_get_current_track_year (p_soundcloud);
      ck_assert (year);
      fprintf (stderr, "year = %s\n", year);
    }

    {
      const char *permalink = tiz_scloud_get_current_track_permalink (p_soundcloud);
      ck_assert (permalink);
      fprintf (stderr, "permalink = %s\n", permalink);
    }

    {
      const char *license = tiz_scloud_get_current_track_license (p_soundcloud);
      ck_assert (license);
      fprintf (stderr, "license = %s\n", license);
    }

    /*     snprintf (cmd, CMD_LEN, "%s \"%s\"", PLAYER, next_url); */
    /*     fprintf (stderr, "cmd = %s\n", cmd); */
    /*     ck_assert (-1 != system (cmd)); */
  }

  tiz_scloud_destroy (p_soundcloud);
}
END_TEST

START_TEST (test_scloud_play_playlist)
{
  tiz_scloud_t *p_soundcloud = NULL;
  int rc = tiz_scloud_init (&p_soundcloud, SOUNDCLOUD_USERNAME, SOUNDCLOUD_PASS);
  ck_assert (0 == rc);
  ck_assert (p_soundcloud);

  rc = tiz_scloud_play_playlist (p_soundcloud, SOUNDCLOUD_PLAYLIST);
  ck_assert (0 == rc);

  /* while (1) */
  {
/*     char cmd[CMD_LEN]; */

    {
      const char *next_url = tiz_scloud_get_next_url (p_soundcloud);
      ck_assert (next_url);
      fprintf (stderr, "url = %s\n", next_url);
    }

    {
      const char *user = tiz_scloud_get_current_track_user (p_soundcloud);
      ck_assert (user);
      fprintf (stderr, "user = %s\n", user);
    }

    {
      const char *title = tiz_scloud_get_current_track_title (p_soundcloud);
      ck_assert (title);
      fprintf (stderr, "title = %s\n", title);
    }

    {
      const char *duration
          = tiz_scloud_get_current_track_duration (p_soundcloud);
      ck_assert (duration);
      fprintf (stderr, "duration = %s\n", duration);
    }

    {
      const char *year = tiz_scloud_get_current_track_year (p_soundcloud);
      ck_assert (year);
      fprintf (stderr, "year = %s\n", year);
    }

    {
      const char *permalink = tiz_scloud_get_current_track_permalink (p_soundcloud);
      ck_assert (permalink);
      fprintf (stderr, "permalink = %s\n", permalink);
    }

    {
      const char *license = tiz_scloud_get_current_track_license (p_soundcloud);
      ck_assert (license);
      fprintf (stderr, "license = %s\n", license);
    }

    /*     snprintf (cmd, CMD_LEN, "%s '%s'", PLAYER, next_url); */
    /*     fprintf (stderr, "cmd = %s\n", cmd); */
    /*     ck_assert (-1 != system (cmd)); */
  }

  tiz_scloud_destroy (p_soundcloud);
}
END_TEST

Suite *
soundcloud_suite (void)
{
  TCase *tc_soundcloud;
  Suite *s = suite_create ("libtizsoundcloud");

  /* test case */
  tc_soundcloud = tcase_create ("SoundCloud client lib unit tests");
  tcase_set_timeout (tc_soundcloud, SOUNDCLOUD_TEST_TIMEOUT);
  tcase_add_test (tc_soundcloud, test_scloud_play_stream);
  tcase_add_test (tc_soundcloud, test_scloud_play_creator);
  tcase_add_test (tc_soundcloud, test_scloud_play_playlist);
  suite_add_tcase (s, tc_soundcloud);

  return s;
}

int main (void)
{
  int number_failed = 1;
  if (soundcloud_credentials_present ())
    {
      SRunner *sr = srunner_create (soundcloud_suite ());
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
