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

#include <assert.h>
#include <check.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "tizchromecast_c.h"

#define CHROMECAST_TEST_TIMEOUT 100
#define CHROMECAST_DEVICE_NAME "Chromecast-Audio"

/* #define URL "http://192.168.1.130:8010" */
/* #define URL "http://kissfm.es.audio1.glb.ipercast.net:8000/kissfm.es/mp3" */
#define URL "http://streams.radiobob.de/bob-acdc/mp3-192/dirble/"
/* #define URL "http://livestreaming.esradio.fm/stream64.mp3" */
/* #define URL "http://192.168.1.122:8010" */
/* #define URL "http://server6.20comunicacion.com:8102/" */
#define CONTENT_TYPE "audio/mpeg"
#define TITLE "Tizonia Audio Stream"

void chromecast_new_media_status (void* ap_user_data)
{
  printf ("New media status!!!\n");
}

START_TEST (test_chromecast)
{
  tiz_chromecast_t *p_chromecast = NULL;
  pid_t pid = getpid();
  int rc = tiz_chromecast_init (&p_chromecast, CHROMECAST_DEVICE_NAME,
                                chromecast_new_media_status, NULL);
  int i = 0;
  fprintf (stderr, "test_chromecast:init [%d] = %d\n", pid, rc);
  ck_assert (0 == rc);
  ck_assert (p_chromecast);

  for (i = 0; i < 1; ++i)
    {
      fprintf (stderr, "test_chromecast: sleeping (5) = %ds\n", i);
      sleep (1);
    }

  fprintf (stderr, "\n\n\ntest_chromecast:load [%d] = before \n", pid);
  rc = tiz_chromecast_load_url (p_chromecast, URL, CONTENT_TYPE, TITLE);
  fprintf (stderr, "test_chromecast:load [%d] = %d \n", pid, rc);
  ck_assert (0 == rc);

  {
    for (i = 0; i < 1; ++i)
      {
        fprintf (stderr, "test_chromecast: sleeping (10) = %ds\n", i);
        sleep (1);
      }

    {
      fprintf (stderr, "\n\n\ntest_chromecast:pause [%d] = before \n", pid);
      rc = tiz_chromecast_pause (p_chromecast);
      fprintf (stderr, "test_chromecast:pause [%d] = %d \n", pid, rc);
      ck_assert (0 == rc);
    }

    for (i = 0; i < 1; ++i)
      {
        fprintf (stderr, "test_chromecast: sleeping (10) = %ds\n", i);
        sleep (1);
      }

    {
      rc = tiz_chromecast_play (p_chromecast);
      fprintf (stderr, "\n\n\ntest_chromecast:play [%d] = before \n", pid);
      fprintf (stderr, "test_chromecast:play [%d] = %d \n", pid, rc);
      ck_assert (0 == rc);
    }

    for (i = 0; i < 1; ++i)
      {
        fprintf (stderr, "test_chromecast: sleeping (5) = %ds\n", i);
        sleep (1);
      }

    for (i = 0; i < 2; ++i)
      {
        fprintf (stderr, "\n\n\ntest_chromecast:volume_up [%d] = before \n", pid);
        rc = tiz_chromecast_volume_up (p_chromecast);
        fprintf (stderr, "test_chromecast:volume_up [%d] = %d \n", pid, rc);
        ck_assert (0 == rc);
        sleep (1);
      }

    for (i = 0; i < 2; ++i)
      {
        fprintf (stderr, "\n\n\ntest_chromecast:mute [%d] = before \n", pid);
        rc = tiz_chromecast_mute (p_chromecast);
        fprintf (stderr, "test_chromecast:mute [%d] = %d \n", pid, rc);
        ck_assert (0 == rc);
        sleep (1);
        fprintf (stderr, "\n\n\ntest_chromecast:unmute [%d] = before \n", pid);
        rc = tiz_chromecast_unmute (p_chromecast);
        fprintf (stderr, "test_chromecast:unmute [%d] = %d \n", pid, rc);
        sleep (1);
      }

    for (i = 0; i < 2; ++i)
      {
        fprintf (stderr, "\n\n\ntest_chromecast:volume_down [%d] = before \n", pid);
        rc = tiz_chromecast_volume_down (p_chromecast);
        fprintf (stderr, "test_chromecast:volume_down [%d] = %d \n", pid, rc);
        ck_assert (0 == rc);
        sleep (1);
      }

    {
      fprintf (stderr, "\n\n\ntest_chromecast:stop [%d] = before \n", pid);
      rc = tiz_chromecast_stop (p_chromecast);
      fprintf (stderr, "test_chromecast:stop [%d] = %d \n", pid, rc);
      ck_assert (0 == rc);
    }
  }

  for (i = 0; i < 1; ++i)
    {
      fprintf (stderr, "test_chromecast: sleeping (5) = %ds\n", i);
      sleep (1);
    }

  fprintf (stderr, "\n\n\ntest_chromecast:destroy [%d] = %d \n", pid, rc);
  tiz_chromecast_destroy (p_chromecast);
}
END_TEST

Suite *chromecast_suite (void)
{
  TCase *tc_chromecast;
  Suite *s = suite_create ("libtizchromecast");

  /* test case */
  tc_chromecast = tcase_create ("Chromecast client lib unit tests");
  tcase_set_timeout (tc_chromecast, CHROMECAST_TEST_TIMEOUT);
  tcase_add_test (tc_chromecast, test_chromecast);
  suite_add_tcase (s, tc_chromecast);

  return s;
}

int main (void)
{
  int number_failed = 1;
  SRunner *sr = srunner_create (chromecast_suite ());
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
