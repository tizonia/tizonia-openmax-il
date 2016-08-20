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
#define YOUTUBE_API_KEY     "xxx"

#define CMD_LEN 1000
#define PLAYER "tizonia"

static bool youtube_credentials_present (void)
{
  if (!strcmp (YOUTUBE_API_KEY, "xxx"))
    {
      return false;
    }
  return true;
}

START_TEST (test_youtube_play_popular_stations)
{
  tiz_youtube_t *p_youtube = NULL;
  int rc = tiz_youtube_init (&p_youtube, YOUTUBE_API_KEY);
  ck_assert (0 == rc);
  ck_assert (p_youtube != NULL);

  rc = tiz_youtube_play_popular_stations (p_youtube);
  ck_assert (0 == rc);

/*   while (1) */
  {
    char cmd[CMD_LEN];
    const char *next_url = tiz_youtube_get_next_url (p_youtube);
    fprintf (stderr, "url = %s\n", next_url);
    ck_assert (next_url != NULL);

    next_url = tiz_youtube_get_next_url (p_youtube);
    fprintf (stderr, "url = %s\n", next_url);
    ck_assert (next_url != NULL);

/*     { */
/*       const char *user = tiz_youtube_get_current_track_user (p_youtube); */
/*       ck_assert (user); */
/*       fprintf (stderr, "user = %s\n", user); */
/*     } */

/*     { */
/*       const char *title = tiz_youtube_get_current_track_title (p_youtube); */
/*       ck_assert (title); */
/*       fprintf (stderr, "title = %s\n", title); */
/*     } */

/*     { */
/*       const char *duration */
/*           = tiz_youtube_get_current_track_duration (p_youtube); */
/*       ck_assert (duration); */
/*       fprintf (stderr, "duration = %s\n", duration); */
/*     } */

/*     { */
/*       const char *year = tiz_youtube_get_current_track_year (p_youtube); */
/*       ck_assert (year); */
/*       fprintf (stderr, "year = %s\n", year); */
/*     } */

/*     { */
/*       const char *permalink = tiz_youtube_get_current_track_permalink (p_youtube); */
/*       ck_assert (permalink); */
/*       fprintf (stderr, "permalink = %s\n", permalink); */
/*     } */

/*     { */
/*       const char *license = tiz_youtube_get_current_track_license (p_youtube); */
/*       ck_assert (license); */
/*       fprintf (stderr, "license = %s\n", license); */
/*     } */

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
  tcase_add_test (tc_youtube, test_youtube_play_popular_stations);
  suite_add_tcase (s, tc_youtube);

  return s;
}

int main (void)
{
  int number_failed = 1;
  if (youtube_credentials_present ())
    {
      SRunner *sr = srunner_create (youtube_suite ());
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
