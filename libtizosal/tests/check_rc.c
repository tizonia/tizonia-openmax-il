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
 * @file   check_rc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 * 
 * @brief  Configuration file parsing utility unit tests
 * 
 * 
 */

START_TEST (test_rcfile_open_and_close)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  tiz_rcfile_t *p_rc = NULL;

  TIZ_LOG (TIZ_LOG_TRACE, "test_rcfile_open_and_close");

  error = tiz_rcfile_open (&p_rc);
  fail_if (error != OMX_ErrorNone);
  tiz_rcfile_close (p_rc);
  fail_if (error != OMX_ErrorNone);
}
END_TEST

START_TEST (test_rcfile_get_single_value)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  tiz_rcfile_t *p_rc = NULL;
  const char *val =  NULL;
  TIZ_LOG (TIZ_LOG_TRACE, "test_rcfile_get_single_value");

  error = tiz_rcfile_open (&p_rc);
  fail_if (error != OMX_ErrorNone);

  val = tiz_rcfile_get_value(p_rc, "resource-management", "rmdb");
  fail_if (val == NULL);

  TIZ_LOG (TIZ_LOG_TRACE, "test_rcfile_get_single_value : "
             "value [%s]", val);

  tiz_rcfile_close (p_rc);
  fail_if (error != OMX_ErrorNone);
}
END_TEST

START_TEST (test_rcfile_get_unexistent_value)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  tiz_rcfile_t *p_rc = NULL;
  const char *val =  NULL;

  error = tiz_rcfile_open (&p_rc);
  fail_if (error != OMX_ErrorNone);

  val = tiz_rcfile_get_value(p_rc, "resource-management", "unexistentvalue124");
  fail_if (val != NULL);

  tiz_rcfile_close (p_rc);
  fail_if (error != OMX_ErrorNone);
}
END_TEST

START_TEST (test_rcfile_get_value_list)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  tiz_rcfile_t *p_rc = NULL;
  char **pp_vlst =  NULL;
  unsigned long length = 0;
  int i=0;

  TIZ_LOG (TIZ_LOG_TRACE, "test_rcfile_get_value_list");

  error = tiz_rcfile_open (&p_rc);
  fail_if (error != OMX_ErrorNone);

  pp_vlst = tiz_rcfile_get_value_list(p_rc, "il-core", "component-paths", &length);
  fail_if (pp_vlst == NULL);
  fail_if (length == 0);

  tiz_rcfile_close (p_rc);
  fail_if (error != OMX_ErrorNone);

  for (i=0; i<length; i++)
    {
      TIZ_LOG (TIZ_LOG_TRACE, "test_rcfile_get_value_list : "
                 "pp_vlst[%d] = [%s]", i , pp_vlst[i]);
      tiz_mem_free(pp_vlst[i]);
    }

  tiz_mem_free(pp_vlst);
}
END_TEST
