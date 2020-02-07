/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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

START_TEST (test_rcfile_get_single_value)
{
  const char *val =  NULL;

  val = tiz_rcfile_get_value("resource-management", "rmdb");
  fail_if (val == NULL);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_rcfile_get_single_value : "
             "value [%s]", val);
}
END_TEST

START_TEST (test_rcfile_get_unexistent_value)
{
  const char *val =  NULL;

  val = tiz_rcfile_get_value("resource-management", "unexistentvalue124");
  fail_if (val != NULL);

}
END_TEST

START_TEST (test_rcfile_get_value_list)
{
  char **pp_vlst =  NULL;
  unsigned long length = 0;
  int i=0;

  pp_vlst = tiz_rcfile_get_value_list("il-core", "component-paths", &length);
  fail_if (pp_vlst == NULL);
  fail_if (length == 0);

  for (i=0; i<length; i++)
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "test_rcfile_get_value_list : "
                 "pp_vlst[%d] = [%s]", i , pp_vlst[i]);
      tiz_mem_free(pp_vlst[i]);
    }

  tiz_mem_free(pp_vlst);
}
END_TEST

/* Local Variables: */
/* c-default-style: gnu */
/* fill-column: 79 */
/* indent-tabs-mode: nil */
/* compile-command: "make check" */
/* End: */
