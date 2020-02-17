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
 * @file   check_tizplatform.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia Platform Unit Tests
 *
 *
 */


#include <stdlib.h>
#include <check.h>
#include <signal.h>
#include <unistd.h>
#include <linux/limits.h>
#include "../src/tizplatform.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.platform.check"
#endif

#include "check_tizplatform.h"
#include "./check_mem.c"
#include "./check_sem.c"
#include "./check_mutex.c"
#include "./check_queue.c"
#include "./check_pqueue.c"
#include "./check_vector.c"
#include "./check_rc.c"
#include "./check_soa.c"
#include "./check_event.c"
#include "./check_http_parser.c"
#include "./check_map.c"

#define EVENT_API_TEST_TIMEOUT 100

Suite *
platform_mem_suite (void)
{
  TCase *tc_mem = NULL;
  Suite *s = suite_create ("Memory allocation APIs");

  /* Memory API test case */
  tc_mem = tcase_create ("memory");
  tcase_add_test (tc_mem, test_mem_alloc_and_free);
  suite_add_tcase (s, tc_mem);

  return s;
}

Suite *
platform_sync_suite (void)
{
  TCase *tc_sem = NULL;
  Suite *s = suite_create ("Synchronization");

  /* synch APIs test case */
  tc_sem = tcase_create ("synchronization");
  tcase_add_test (tc_sem, test_sem_init_and_destroy);
  tcase_add_test (tc_sem, test_sem_post_and_wait);
  tcase_add_test_raise_signal (tc_sem, test_sem_init_null, SIGABRT);
  tcase_add_test (tc_sem, test_sem_destroy_null);
  tcase_add_test_raise_signal (tc_sem, test_sem_wait_null, SIGABRT);
  tcase_add_test_raise_signal (tc_sem, test_sem_post_null, SIGABRT);
  tcase_add_test (tc_sem, test_mutex_init_and_destroy);
  tcase_add_test (tc_sem, test_mutex_lock_and_unlock);
  tcase_add_test_raise_signal (tc_sem, test_mutex_init_null, SIGABRT);
  tcase_add_test (tc_sem, test_mutex_destroy_null);
  tcase_add_test_raise_signal (tc_sem, test_mutex_lock_null, SIGABRT);
  tcase_add_test_raise_signal (tc_sem, test_mutex_unlock_null, SIGABRT);
  suite_add_tcase (s, tc_sem);

  return s;
}

Suite *
platform_queue_suite (void)
{
  TCase *tc_queue = NULL;
  Suite *s = suite_create ("Synchronized FIFO queue");

  /* queue API test case */
  tc_queue = tcase_create ("queue");
  tcase_add_test (tc_queue, test_queue_init_and_destroy);
  tcase_add_test (tc_queue, test_queue_send_and_receive);
  suite_add_tcase (s, tc_queue);

  return s;
}

Suite *
platform_pqueue_suite (void)
{
  TCase *tc_pqueue = NULL;
  Suite *s = suite_create ("Priority queue");

  /* pqueue API test case */
  tc_pqueue = tcase_create ("priority queue");
  tcase_add_test (tc_pqueue, test_pqueue_init_and_destroy);
  tcase_add_test (tc_pqueue, test_pqueue_send_and_receive_one_group);
  tcase_add_test (tc_pqueue, test_pqueue_send_and_receive_two_groups);
  tcase_add_test (tc_pqueue, test_pqueue_send_and_receive_three_groups);
  tcase_add_test (tc_pqueue, test_pqueue_first);
  tcase_add_test (tc_pqueue, test_pqueue_remove);
  tcase_add_test (tc_pqueue, test_pqueue_removep);
  suite_add_tcase (s, tc_pqueue);

  return s;
}

Suite *
platform_vector_suite (void)
{
  TCase *tc_vector = NULL;
  Suite *s = suite_create ("Dynamic array implementation");

  /* vector API test case */
  tc_vector = tcase_create ("vector");
  tcase_add_test (tc_vector, test_vector_init_and_destroy);
  tcase_add_test (tc_vector, test_vector_push_and_pop_length_front_back_ints);
  tcase_add_test (tc_vector, test_vector_push_and_pop_length_front_back_pointers);
  tcase_add_test (tc_vector, test_vector_push_back_vector);
  suite_add_tcase (s, tc_vector);

  return s;
}

Suite *
platform_rcfile_suite (void)
{
  TCase *tc_rc = NULL;
  Suite *s = suite_create ("Runcon file parsing APIs");

  putenv(TIZ_PLATFORM_RC_FILE_ENV);

  /* config file parsing API test cases */
  tc_rc = tcase_create ("rcfile");
  tcase_add_test (tc_rc, test_rcfile_get_single_value);
  tcase_add_test (tc_rc, test_rcfile_get_unexistent_value);
  tcase_add_test (tc_rc, test_rcfile_get_value_list);
  suite_add_tcase (s, tc_rc);

  return s;
}

Suite *
platform_soa_suite (void)
{
  TCase *tc_soa = NULL;
  Suite *s = suite_create ("Small object allocation APIs");

  /* small object allocation API test cases */
  tc_soa = tcase_create ("soa");
  tcase_add_test (tc_soa, test_soa_basic_life_cycle);
  tcase_add_test (tc_soa, test_soa_reserve_life_cycle);
  suite_add_tcase (s, tc_soa);

  return s;
}

Suite *
platform_event_suite (void)
{
  TCase  *tc_event;
  Suite *s = suite_create ("events");

  /* event loop API test cases */
  tc_event = tcase_create ("event loop API");
  tcase_set_timeout (tc_event, EVENT_API_TEST_TIMEOUT);
  tcase_add_test (tc_event, test_event_loop_init_and_destroy);
  tcase_add_test (tc_event, test_event_io);
  tcase_add_test (tc_event, test_event_timer);
  tcase_add_test (tc_event, test_event_stat);
  suite_add_tcase (s, tc_event);

  return s;
}

Suite *
platform_http_parser_suite (void)
{
  TCase  *tc_http;
  Suite *s = suite_create ("http parser");

  /* http parser API test cases */
  tc_http = tcase_create ("http parser API");
  tcase_add_test (tc_http, test_http_parser_request_test);
  suite_add_tcase (s, tc_http);

  return s;
}

Suite *
platform_map_suite (void)
{
  TCase  *tc_map;
  Suite *s = suite_create ("map");

  /* map API test cases */
  tc_map = tcase_create ("map API");
  tcase_add_test (tc_map, test_map_init_and_destroy);
  tcase_add_test (tc_map, test_map_insert_find_erase_size_empty_at_for_each);
  tcase_add_test (tc_map, test_map_clear);
  suite_add_tcase (s, tc_map);

  return s;

}

int
main (void)
{
  int number_failed = 0;
  SRunner *sr = NULL;

  tiz_log_init();

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Tizonia Platform unit tests");

  sr = srunner_create (platform_mem_suite ());
  srunner_add_suite (sr, platform_sync_suite ());
  srunner_add_suite (sr, platform_queue_suite ());
  srunner_add_suite (sr, platform_pqueue_suite ());
  srunner_add_suite (sr, platform_vector_suite ());
  srunner_add_suite (sr, platform_rcfile_suite ());
  srunner_add_suite (sr, platform_soa_suite ());
  srunner_add_suite (sr, platform_http_parser_suite ());
  srunner_add_suite (sr, platform_map_suite ());
/*   srunner_add_suite (sr, platform_event_suite ()); */
  srunner_run_all (sr, CK_VERBOSE);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);

  tiz_log_deinit ();

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* Local Variables: */
/* c-default-style: gnu */
/* fill-column: 79 */
/* indent-tabs-mode: nil */
/* compile-command: "make check" */
/* End: */
