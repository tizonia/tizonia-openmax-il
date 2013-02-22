/**
 * Copyright (C) 2011-2013 Aratelia Limited - Juan A. Rubio
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
 * @file   check_tizosal.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OSAL Unit Tests
 *
 *
 */


#include <stdlib.h>
#include <check.h>
#include <signal.h>
#include "../src/tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.osal.check"
#endif

/* #include "./check_mem.c" */
/* #include "./check_sem.c" */
/* #include "./check_mutex.c" */
/* #include "./check_queue.c" */
/* #include "./check_pqueue.c" */
/* #include "./check_vector.c" */
/* #include "./check_rc.c" */
/* #include "./check_soa.c" */
#include "./check_event.c"

#define EVENT_API_TEST_TIMEOUT 100

Suite *
tiz_suite (void)
{

/*   TCase *tc_mem, *tc_sem, *tc_queue, *tc_pqueue, *tc_vector, *tc_rc, *tc_soa, */
/*   *tc_event; */
  TCase  *tc_event;
  Suite *s = suite_create ("tizosal");

  /* Memory API test case */
/*   tc_mem = tcase_create ("memory"); */
/*   tcase_add_test (tc_mem, test_mem_alloc_and_free); */
/*   suite_add_tcase (s, tc_mem); */

/*   /\* synch APIs test case *\/ */
/*   tc_sem = tcase_create ("synchAPIs"); */
/*   tcase_add_test (tc_sem, test_sem_init_and_destroy); */
/*   tcase_add_test (tc_sem, test_sem_post_and_wait); */
/*   tcase_add_test_raise_signal (tc_sem, test_sem_init_null, SIGABRT); */
/*   tcase_add_test_raise_signal (tc_sem, test_sem_destroy_null, SIGABRT); */
/*   tcase_add_test_raise_signal (tc_sem, test_sem_wait_null, SIGABRT); */
/*   tcase_add_test_raise_signal (tc_sem, test_sem_post_null, SIGABRT); */
/*   tcase_add_test (tc_sem, test_mutex_init_and_destroy); */
/*   tcase_add_test (tc_sem, test_mutex_lock_and_unlock); */
/*   tcase_add_test_raise_signal (tc_sem, test_mutex_init_null, SIGABRT); */
/*   tcase_add_test_raise_signal (tc_sem, test_mutex_destroy_null, SIGABRT); */
/*   tcase_add_test_raise_signal (tc_sem, test_mutex_lock_null, SIGABRT); */
/*   tcase_add_test_raise_signal (tc_sem, test_mutex_unlock_null, SIGABRT); */
/*   suite_add_tcase (s, tc_sem); */

/*   /\* queue API test case *\/ */
/*   tc_queue = tcase_create ("queueAPI"); */
/*   tcase_add_test (tc_queue, test_queue_init_and_destroy); */
/*   tcase_add_test (tc_queue, test_queue_send_and_receive); */
/*   suite_add_tcase (s, tc_queue); */

/*   /\* pqueue API test case *\/ */
/*   tc_pqueue = tcase_create ("priority queueAPI"); */
/*   tcase_add_test (tc_pqueue, test_pqueue_init_and_destroy); */
/*   tcase_add_test (tc_pqueue, test_pqueue_send_and_receive_one_group); */
/*   tcase_add_test (tc_pqueue, test_pqueue_send_and_receive_two_groups); */
/*   tcase_add_test (tc_pqueue, test_pqueue_send_and_receive_three_groups); */
/*   tcase_add_test (tc_pqueue, test_pqueue_first); */
/*   tcase_add_test (tc_pqueue, test_pqueue_remove); */
/*   tcase_add_test (tc_pqueue, test_pqueue_removep); */
/*   suite_add_tcase (s, tc_pqueue); */

/*   /\* vector API test case *\/ */
/*   tc_vector = tcase_create ("vector API"); */
/*   tcase_add_test (tc_vector, test_vector_init_and_destroy); */
/*   tcase_add_test (tc_vector, test_vector_push_and_pop_length_front_back_ints); */
/*   tcase_add_test (tc_vector, test_vector_push_and_pop_length_front_back_pointers); */
/*   tcase_add_test (tc_vector, test_vector_push_back_vector); */
/*   suite_add_tcase (s, tc_vector); */

/*   /\* config file parsing API test cases *\/ */
/*   tc_rc = tcase_create ("rc file parsing API"); */
/*   tcase_add_test (tc_rc, test_rcfile_open_and_close); */
/*   tcase_add_test (tc_rc, test_rcfile_get_single_value); */
/*   tcase_add_test (tc_rc, test_rcfile_get_unexistent_value); */
/*   tcase_add_test (tc_rc, test_rcfile_get_value_list); */
/*   suite_add_tcase (s, tc_rc); */

/*   /\* small object allocation API test cases *\/ */
/*   tc_soa = tcase_create ("small object allocation API"); */
/*   tcase_add_test (tc_soa, test_soa_basic_life_cycle); */
/*   tcase_add_test (tc_soa, test_soa_reserve_life_cycle); */
/*   suite_add_tcase (s, tc_soa); */

  /* event loop API test cases */
  tc_event = tcase_create ("event loop API");
  tcase_set_timeout (tc_event, EVENT_API_TEST_TIMEOUT);
  tcase_add_test (tc_event, test_event_loop_init_and_destroy);
  tcase_add_test (tc_event, test_event_io);
  suite_add_tcase (s, tc_event);

  return s;

}


int
main (void)
{

  tiz_log_init ();

  TIZ_LOG (TIZ_LOG_TRACE, "mAin");

  int number_failed;
  SRunner *sr = srunner_create (tiz_suite ());
  srunner_run_all (sr, CK_VERBOSE);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);

  tiz_log_deinit ();

  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;

}
