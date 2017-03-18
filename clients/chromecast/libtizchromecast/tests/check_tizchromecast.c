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
 * @file   check_tizchromecast.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia Chromecast Unit Tests
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

#include "tizchromecast_c.h"

#define CHROMECAST_TEST_TIMEOUT 2500
#define CHROMECAST_DEVICE_NAME  "Chromecast-Ultra"

#define URL "http://localhost:8010"
#define CONTENT_TYPE "audio/mp3"
#define TITLE "Tizonia Audio Stream"

#define CMD_LEN 1000
#define PLAYER "tizonia"

static bool chromecast_credentials_present (void)
{
  if (!strcmp (CHROMECAST_DEVICE_NAME, "xxx"))
    {
      return false;
    }
  return true;
}

START_TEST (test_chromecast_play_media)
{
  tiz_chromecast_t *p_chromecast = NULL;
  int rc = tiz_chromecast_init (&p_chromecast, CHROMECAST_DEVICE_NAME);
  ck_assert (0 == rc);
  ck_assert (p_chromecast);

  rc = tiz_chromecast_load (p_chromecast, URL, CONTENT_TYPE, TITLE);
  ck_assert (0 == rc);

/*   while (1) */
  {
/*     char cmd[CMD_LEN]; */
/*     { */
/*       const int result = tiz_chromecast_play (p_chromecast); */
/*       ck_assert (0 == result); */
/*       fprintf (stderr, "result = %d\n", result); */
/*     } */

/*     { */
/*       const int result = tiz_chromecast_pause (p_chromecast); */
/*       ck_assert (0 == result); */
/*       fprintf (stderr, "result = %d\n", result); */
/*     } */

/*     { */
/*       const int result = tiz_chromecast_stop (p_chromecast); */
/*       ck_assert (0 == result); */
/*       fprintf (stderr, "result = %d\n", result); */
/*     } */

/*     snprintf (cmd, CMD_LEN, "%s \"%s\"", PLAYER, next_url); */
/*     fprintf (stderr, "cmd = %s\n", cmd); */
/*     ck_assert (-1 != system (cmd)); */
  }

  tiz_chromecast_destroy (p_chromecast);
}
END_TEST

Suite *
chromecast_suite (void)
{
  TCase *tc_chromecast;
  Suite *s = suite_create ("libtizchromecast");

  /* test case */
  tc_chromecast = tcase_create ("Chromecast client lib unit tests");
  tcase_set_timeout (tc_chromecast, CHROMECAST_TEST_TIMEOUT);
  tcase_add_test (tc_chromecast, test_chromecast_play_media);
  suite_add_tcase (s, tc_chromecast);

  return s;
}

int main (void)
{
  int number_failed = 1;
  if (chromecast_credentials_present ())
    {
      SRunner *sr = srunner_create (chromecast_suite ());
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
