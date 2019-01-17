/**
 * Copyright (C) 2011-2019 Aratelia Limited - Juan A. Rubio
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
 * @file   check_map.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Associative array API unit tests
 *
 *
 */

static OMX_S32
check_map_cmp_f (OMX_PTR ap_key1, OMX_PTR ap_key2)
{
  int *key1 = (int*) ap_key1;
  int *key2 = (int*) ap_key2;

  fail_if (NULL == ap_key1);
  fail_if (NULL == ap_key2);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "key1 [%d] key2 [%d]", *key1, *key2);

  if (*key1 == *key2)
    {
      return 0;
    }
  else if (*key1 < *key2)
    {
      return -1;
    }
  else
    {
      return 1;
    }
}

static void
check_map_free_f (OMX_PTR ap_key, OMX_PTR ap_value)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "key [%d] value [%d]",
           *((int*)ap_key), *((int*)ap_value));
  tiz_mem_free (ap_key);
}

static OMX_S32
check_map_for_each_f (OMX_PTR ap_key, OMX_PTR ap_value,
                      OMX_PTR ap_arg)
{
  static int g_each = 0;
  int *p_key = (int*) ap_key;
  int *p_value = (int*) ap_value;

  fail_if (NULL == ap_key);
  fail_if (NULL == ap_value);
  fail_if (ap_arg);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "each [%d] key [%d] value [%d]", g_each, *p_key, *p_value);

  fail_if (*p_key != *p_value);
  fail_if (g_each != *p_value);
  g_each++;

  return 0;
}

START_TEST (test_map_init_and_destroy)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  tiz_map_t *p_map = NULL;

  error = tiz_map_init (&p_map, check_map_cmp_f, check_map_free_f, NULL);

  fail_if (error != OMX_ErrorNone);

  tiz_map_destroy (p_map);
}
END_TEST

START_TEST (test_map_insert_find_erase_size_empty_at_for_each)
{
  OMX_U32 i;
  OMX_ERRORTYPE error = OMX_ErrorNone;
  int *p_item = NULL;
  tiz_map_t *p_map = NULL;;
  OMX_U32 index = 0;

  error = tiz_map_init (&p_map, check_map_cmp_f, check_map_free_f, NULL);
  fail_if (error != OMX_ErrorNone);

  fail_if (false == tiz_map_empty (p_map));

  for (i = 0; i < 10; i++)
    {
      p_item = (int *) tiz_mem_alloc (sizeof (int));
      fail_if (p_item == NULL);
      *p_item = i;
      error = tiz_map_insert (p_map, p_item, p_item, &index);
      fail_if (error != OMX_ErrorNone);
      fail_if (index != i);
      fail_if (tiz_map_size (p_map) != i+1);
    }

  fail_if (10 != tiz_map_size (p_map));
  fail_if (true == tiz_map_empty (p_map));

  i = 5;
  fail_if (5 != *(p_item = tiz_map_find (p_map, &i)));

  fail_if (5 != *(p_item = tiz_map_value_at (p_map, i)));

  fail_if (OMX_ErrorNone != tiz_map_for_each (p_map,
                                              check_map_for_each_f,
                                              NULL));


  for (i = 0; i < 10; i++)
    {
      int d = i;
      tiz_map_erase (p_map, &d);
      fail_if (tiz_map_size (p_map) != 9-i);
    }

  tiz_map_destroy (p_map);

}
END_TEST

START_TEST (test_map_clear)
{
  OMX_U32 i;
  OMX_ERRORTYPE error = OMX_ErrorNone;
  int *p_item = NULL;
  tiz_map_t *p_map = NULL;;
  OMX_U32 index = 0;

  error = tiz_map_init (&p_map, check_map_cmp_f, check_map_free_f, NULL);
  fail_if (error != OMX_ErrorNone);

  fail_if (false == tiz_map_empty (p_map));

  for (i = 0; i < 10; i++)
    {
      p_item = (int *) tiz_mem_alloc (sizeof (int));
      fail_if (p_item == NULL);
      *p_item = i;
      error = tiz_map_insert (p_map, p_item, p_item, &index);
      fail_if (error != OMX_ErrorNone);
      fail_if (index != i);
      fail_if (tiz_map_size (p_map) != i+1);
    }

  fail_if (10 != tiz_map_size (p_map));
  fail_if (true == tiz_map_empty (p_map));

  fail_if (OMX_ErrorNone != tiz_map_clear (p_map));

  fail_if (0 != tiz_map_size (p_map));
  fail_if (false == tiz_map_empty (p_map));

  tiz_map_destroy (p_map);
}
END_TEST

/* Local Variables: */
/* c-default-style: gnu */
/* fill-column: 79 */
/* indent-tabs-mode: nil */
/* compile-command: "make check" */
/* End: */
