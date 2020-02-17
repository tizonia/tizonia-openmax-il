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
 * @file   check_event.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Event loop API unit tests
 *
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define CHECK_IO_SERV_PORT 9877
#define CHECK_IO_MAXLINE 4096
#define CHECK_IO_MSG "Hello there!"
#define CHECK_IO_ECHO_CMD "/bin/bash -c \"echo -n \"Hello\\ there!\" > /dev/udp/127.0.0.1/9877\""

#define CHECK_TIMER_PERIOD 1.5

#define CHECK_STAT_FILE "/tmp/check_event.txt"
#define CHECK_STAT_RM_CMD "/bin/bash -c \"rm -f /tmp/check_event.txt\""
#define CHECK_STAT_TOUCH_CMD "/bin/bash -c \"touch /tmp/check_event.txt\""
#define CHECK_STAT_ECHO_CMD "/bin/bash -c \"echo \"Hello\" > /tmp/check_event.txt\""

static bool g_io_cback_received = false;
static int g_timeout_count = 5;
static int g_restart_count = 2;
static bool g_timer_restarted = false;
static bool g_file_status_changed = false;

static void
check_event_io_cback (OMX_HANDLETYPE p_hdl, tiz_event_io_t * ap_ev_io, void *ap_arg1,
                      const uint32_t a_id, int fd, int events)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  int rcvfromrc;
  char msg [CHECK_IO_MAXLINE];
  struct sockaddr_in cliaddr;
  socklen_t len = sizeof (cliaddr);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "io cback received");

  fail_if (NULL == ap_ev_io);
  fail_if ((events & TIZ_EVENT_READ) == 0);

  error = tiz_event_io_stop (ap_ev_io);
  fail_if (OMX_ErrorNone != error);

  rcvfromrc = recvfrom (fd, msg, CHECK_IO_MAXLINE, 0, (struct sockaddr *) &cliaddr,
                        &len);
  fail_if (rcvfromrc < 0);

  msg [rcvfromrc] = '\0';
  TIZ_LOG (TIZ_PRIORITY_TRACE, "received : [%s]", msg);

  fail_if (strncmp(msg, CHECK_IO_MSG, strlen (msg) != 0));

  g_io_cback_received = true;
}

static int
start_udp_server()
{
  int sockfd, bindrc, fcntlrc, flags;
  struct sockaddr_in servaddr;

  sockfd = socket (AF_INET, SOCK_DGRAM, 0);
  fail_if (sockfd < 0);

  bzero (&servaddr, sizeof(servaddr));
  servaddr.sin_family      = AF_INET;
  servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
  servaddr.sin_port        = htons (CHECK_IO_SERV_PORT);

  bindrc = bind (sockfd, (const struct sockaddr *) &servaddr,
                 sizeof(servaddr));
  fail_if (bindrc < 0);

  flags = fcntl(sockfd, F_GETFL, 0);
  fail_if (flags < 0);

  flags |= O_NONBLOCK;
  fcntlrc = fcntl(sockfd, F_SETFL, flags);
  fail_if (fcntlrc < 0);

  return sockfd;
}

static void
stop_udp_server(int sockfd)
{
  close(sockfd);
}

static void
check_event_timer_cback (OMX_HANDLETYPE p_hdl, tiz_event_timer_t * ap_ev_timer,
                         void *ap_arg, const uint32_t a_id)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "timer cback received - timeout count [%d]",
           g_timeout_count);

  fail_if (NULL == ap_ev_timer);

  if (--g_timeout_count == 0)
    {
      if (false == g_timer_restarted)
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "timeout count [%d] - restarting timer",
                   g_timeout_count);
          error = tiz_event_timer_restart (ap_ev_timer, a_id);
          fail_if (OMX_ErrorNone != error);
          g_timeout_count = g_restart_count;
          g_timer_restarted = true;
        }
      else
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "timeout count [%d] - stopping timer",
                   g_timeout_count);
          error = tiz_event_timer_stop (ap_ev_timer);
          fail_if (OMX_ErrorNone != error);
        }
    }
}

static void
check_event_stat_cback (OMX_HANDLETYPE p_hdl, tiz_event_stat_t * ap_ev_stat,
                        void *ap_arg1, const uint32_t a_id, int events)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "stat cback received ");

  fail_if (NULL == ap_ev_stat);

  g_file_status_changed = true;

  error = tiz_event_stat_stop (ap_ev_stat);
  fail_if (OMX_ErrorNone != error);
}

/* TESTS */

START_TEST (test_event_loop_init_and_destroy)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;

  error = tiz_event_loop_init ();
  fail_if (error != OMX_ErrorNone);

  error = tiz_event_loop_init ();
  fail_if (error != OMX_ErrorNone);

  tiz_event_loop_destroy ();
}
END_TEST

START_TEST (test_event_io)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  tiz_event_io_t * p_ev_io = NULL;
  int fd = 0;
  char cmd [128];
  OMX_HANDLETYPE p_hdl = NULL;
  uint32_t id = 1;

  error = tiz_event_loop_init ();
  fail_if (error != OMX_ErrorNone);

  fd = start_udp_server();

  error = tiz_event_io_init (&p_ev_io, p_hdl, check_event_io_cback, NULL);
  fail_if (error != OMX_ErrorNone);

  tiz_event_io_set (p_ev_io, fd, TIZ_EVENT_READ, false);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "started io watcher");

  error = tiz_event_io_start (p_ev_io, id);
  fail_if (error != OMX_ErrorNone);

  snprintf(cmd, strlen (CHECK_IO_ECHO_CMD) + 1, "%s", CHECK_IO_ECHO_CMD);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "cmd = [%s]", cmd);

  fail_if (-1 == system (cmd));

  fail_if (true != g_io_cback_received);

  error = tiz_event_io_stop (p_ev_io);
  fail_if (error != OMX_ErrorNone);

  tiz_event_io_destroy (p_ev_io);

  stop_udp_server(fd);

  tiz_event_loop_destroy ();
}
END_TEST

START_TEST (test_event_timer)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  tiz_event_timer_t * p_ev_timer = NULL;
  time_t t1, t2;
  int sleep_len = (double)(CHECK_TIMER_PERIOD * g_timeout_count) +
    (double)(CHECK_TIMER_PERIOD * g_restart_count) + 1;
  int sleep_count = sleep_len;
  OMX_HANDLETYPE p_hdl = NULL;

  error = tiz_event_loop_init ();
  fail_if (error != OMX_ErrorNone);

  error = tiz_event_timer_init (&p_ev_timer, p_hdl, check_event_timer_cback,
                                NULL);
  fail_if (error != OMX_ErrorNone);

  tiz_event_timer_set (p_ev_timer, 1., CHECK_TIMER_PERIOD);

  error = tiz_event_timer_start (p_ev_timer, 0);
  fail_if (error != OMX_ErrorNone);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "started timer watcher - sleep_len [%d]", sleep_len);

  t1 = time (NULL);
  while (--sleep_count > 0)
    {
      sleep (1);
      if (1 == sleep_count)
        {
          t2 = time (NULL);
          if ((t2 - t1) < sleep_len)
            {
              /* haven't waited enough */
              sleep_count = sleep_len - (t2 - t1) + 1;
              TIZ_LOG (TIZ_PRIORITY_TRACE, "woken up too early - new sleep_count [%d]",
                       sleep_count);
              }
        }
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "sleep_len [%d]", sleep_len);

  fail_if (0 != g_timeout_count);

  tiz_event_timer_destroy (p_ev_timer);

  tiz_event_loop_destroy ();
}
END_TEST

START_TEST (test_event_stat)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  tiz_event_stat_t * p_ev_stat = NULL;
  char cmd [128];
  int sleep_count = 5;
  OMX_HANDLETYPE p_hdl = NULL;
  int echo_cmd_pid = 0;
  uint32_t id = 1;

  snprintf (cmd, strlen (CHECK_STAT_RM_CMD) + 1, "%s", CHECK_STAT_RM_CMD);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "cmd = [%s]", cmd);
  fail_if (-1 == system (cmd));

  snprintf (cmd, strlen (CHECK_STAT_TOUCH_CMD) + 1, "%s", CHECK_STAT_TOUCH_CMD);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "cmd = [%s]", cmd);
  fail_if (-1 == system (cmd));

  error = tiz_event_loop_init ();
  fail_if (error != OMX_ErrorNone);

  error = tiz_event_stat_init (&p_ev_stat, p_hdl, check_event_stat_cback, NULL);
  fail_if (error != OMX_ErrorNone);

  tiz_event_stat_set (p_ev_stat, CHECK_STAT_FILE);

  error = tiz_event_stat_start (p_ev_stat, id);
  fail_if (error != OMX_ErrorNone);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "started stat watcher");

  snprintf (cmd, strlen (CHECK_STAT_ECHO_CMD) + 1, "%s", CHECK_STAT_ECHO_CMD);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "cmd = [%s]", cmd);


  /* Fork here to run the echo command on another process */
  echo_cmd_pid = fork ();
  fail_if (echo_cmd_pid == -1);

  if (echo_cmd_pid)
    {
      sleep (1);
      do
        {
          sleep (1);
          if (g_file_status_changed)
            {
              break;
            }
        }
      while (--sleep_count != 0);
      
      fail_if (true != g_file_status_changed);
      tiz_event_stat_destroy (p_ev_stat);
      tiz_event_loop_destroy ();

    }
  else
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Running echo command");
      fail_if (-1 == system (cmd));
    }

}
END_TEST

/* Local Variables: */
/* c-default-style: gnu */
/* fill-column: 79 */
/* indent-tabs-mode: nil */
/* compile-command: "make check" */
/* End: */
