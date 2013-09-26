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
 * @file   check_soa.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Small object allocator unit tests
 *
 *
 */

#include <malloc.h>

#define MAX_CLASS0_OBJS 126
#define MAX_CLASS1_OBJS 62
#define MAX_CLASS2_OBJS 40
#define MAX_CLASS3_OBJS 30
#define MAX_CLASS4_OBJS 14

START_TEST (test_soa_basic_life_cycle)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  tiz_soa_t *p_soa = NULL;
  size_t class0 = 8;
  size_t class1 = 40;
  size_t class2 = 72;
  size_t class3 = 104;
  size_t class4 = 136;
  void *class0_objs[MAX_CLASS0_OBJS];
  void *class1_objs[MAX_CLASS1_OBJS];
  void *class2_objs[MAX_CLASS2_OBJS];
  void *class3_objs[MAX_CLASS3_OBJS];
  void *class4_objs[MAX_CLASS4_OBJS];
  int i = 0;
  tiz_soa_info_t info;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_soa_basic_life_cycle - begin");

  malloc_trim (0);
  malloc_stats();

  error = tiz_soa_init (&p_soa);
  fail_if (error != OMX_ErrorNone);

  /* Allocate MAX_CLASS0_OBJS class #0 objects */
  for (i=0; i<MAX_CLASS0_OBJS; i++)
    {
      fail_if (NULL == (class0_objs[i] = tiz_soa_calloc (p_soa, class0)));
    }

  tiz_soa_info (p_soa, &info);
  fail_if (info.chunks != 1);
  fail_if (info.objects != MAX_CLASS0_OBJS);
  fail_if (info.slices[0] != MAX_CLASS0_OBJS);

  /* Allocate MAX_CLASS1_OBJS class #1 objects */
  for (i=0; i<MAX_CLASS1_OBJS; i++)
    {
      fail_if (NULL == (class1_objs[i] = tiz_soa_calloc (p_soa, class1)));
    }

  tiz_soa_info (p_soa, &info);
  fail_if (info.chunks != 2);
  fail_if (info.objects != MAX_CLASS0_OBJS + MAX_CLASS1_OBJS);
  fail_if (info.slices[1] != MAX_CLASS1_OBJS);

  /* Allocate MAX_CLASS2_OBJS class #2 objects */
  for (i=0; i<MAX_CLASS2_OBJS; i++)
    {
      fail_if (NULL == (class2_objs[i] = tiz_soa_calloc (p_soa, class2)));
    }

  tiz_soa_info (p_soa, &info);
  fail_if (info.chunks != 3);
  fail_if (info.objects != MAX_CLASS0_OBJS + MAX_CLASS1_OBJS
           + MAX_CLASS2_OBJS);
  fail_if (info.slices[2] != MAX_CLASS2_OBJS);

  /* Allocate MAX_CLASS3_OBJS class #3 objects */
  for (i=0; i<MAX_CLASS3_OBJS; i++)
    {
      fail_if (NULL == (class3_objs[i] = tiz_soa_calloc (p_soa, class3)));
    }

  tiz_soa_info (p_soa, &info);
  fail_if (info.chunks != 4);
  fail_if (info.objects != MAX_CLASS0_OBJS + MAX_CLASS1_OBJS
           + MAX_CLASS2_OBJS + MAX_CLASS3_OBJS);
  fail_if (info.slices[3] != MAX_CLASS3_OBJS);

  /* Allocate MAX_CLASS4_OBJS class #4 objects */
  for (i=0; i<MAX_CLASS4_OBJS; i++)
    {
      fail_if (NULL == (class4_objs[i] = tiz_soa_calloc (p_soa, class4)));
    }

  tiz_soa_info (p_soa, &info);
  fail_if (info.chunks != 5);
  fail_if (info.objects != MAX_CLASS0_OBJS + MAX_CLASS1_OBJS
           + MAX_CLASS2_OBJS + MAX_CLASS3_OBJS + MAX_CLASS4_OBJS);
  fail_if (info.slices[4] != MAX_CLASS4_OBJS);


  /* Destruction */


  /* Destroy MAX_CLASS0_OBJS class #0 objects */
  for (i=0; i<MAX_CLASS0_OBJS; i++)
    {
      tiz_soa_free (p_soa, class0_objs[i]);
    }

  tiz_soa_info (p_soa, &info);
  fail_if (info.chunks != 5);
  fail_if (info.objects != MAX_CLASS1_OBJS
           + MAX_CLASS2_OBJS + MAX_CLASS3_OBJS + MAX_CLASS4_OBJS);
  fail_if (info.slices[0] != 0);

  /* Destroy MAX_CLASS1_OBJS class #1 objects */
  for (i=0; i<MAX_CLASS1_OBJS; i++)
    {
      tiz_soa_free (p_soa, class1_objs[i]);
    }

  tiz_soa_info (p_soa, &info);
  fail_if (info.chunks != 5);
  fail_if (info.objects != MAX_CLASS2_OBJS + MAX_CLASS3_OBJS
           + MAX_CLASS4_OBJS);
  fail_if (info.slices[1] != 0);

  /* Destroy MAX_CLASS2_OBJS class #2 objects */
  for (i=0; i<MAX_CLASS2_OBJS; i++)
    {
      tiz_soa_free (p_soa, class2_objs[i]);
    }

  tiz_soa_info (p_soa, &info);
  fail_if (info.chunks != 5);
  fail_if (info.objects != MAX_CLASS3_OBJS + MAX_CLASS4_OBJS);
  fail_if (info.slices[2] != 0);

  /* Destroy MAX_CLASS3_OBJS class #3 objects */
  for (i=0; i<MAX_CLASS3_OBJS; i++)
    {
      tiz_soa_free (p_soa, class3_objs[i]);
    }

  tiz_soa_info (p_soa, &info);
  fail_if (info.chunks != 5);
  fail_if (info.objects != MAX_CLASS4_OBJS);
  fail_if (info.slices[3] != 0);

  /* Destroy MAX_CLASS4_OBJS class #4 objects */
  for (i=0; i<MAX_CLASS4_OBJS; i++)
    {
      tiz_soa_free (p_soa, class4_objs[i]);
    }

  tiz_soa_info (p_soa, &info);
  fail_if (info.chunks != 5);
  fail_if (info.objects != 0);
  fail_if (info.slices[4] != 0);

  tiz_soa_destroy (p_soa);
  fail_if (error != OMX_ErrorNone);

  malloc_trim (0);
  malloc_stats ();
  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_soa_basic_life_cycle - end");
}
END_TEST

START_TEST (test_soa_reserve_life_cycle)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  tiz_soa_t *p_soa = NULL;
  size_t class0 = 8;
  size_t class4 = 136;
  void *class0_objs[MAX_CLASS0_OBJS];
  void *class4_objs[MAX_CLASS4_OBJS];
  int i = 0;
  tiz_soa_info_t info;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_soa_reserve_life_cycle - begin");

  malloc_trim (0);
  malloc_stats();

  error = tiz_soa_init (&p_soa);
  fail_if (error != OMX_ErrorNone);

  /* Reserve class 0's chunk */
  error = tiz_soa_reserve_chunk (p_soa, 0);
  fail_if (error != OMX_ErrorNone);

  tiz_soa_info (p_soa, &info);
  fail_if (info.chunks != 1);
  fail_if (info.objects != 0);
  fail_if (info.slices[0] != 0);

  /* Allocate MAX_CLASS0_OBJS class #0 objects */
  for (i=0; i<MAX_CLASS0_OBJS; i++)
    {
      fail_if (NULL == (class0_objs[i] = tiz_soa_calloc (p_soa, class0)));
    }

  tiz_soa_info (p_soa, &info);
  fail_if (info.chunks != 1);
  fail_if (info.objects != MAX_CLASS0_OBJS);
  fail_if (info.slices[0] != MAX_CLASS0_OBJS);

  /* Reserve class 4's chunk */
  error = tiz_soa_reserve_chunk (p_soa, 4);
  fail_if (error != OMX_ErrorNone);

  tiz_soa_info (p_soa, &info);
  fail_if (info.chunks != 2);
  fail_if (info.objects != MAX_CLASS0_OBJS);
  fail_if (info.slices[4] != 0);

  /* Allocate MAX_CLASS4_OBJS class #4 objects */
  for (i=0; i<MAX_CLASS4_OBJS; i++)
    {
      fail_if (NULL == (class4_objs[i] = tiz_soa_calloc (p_soa, class4)));
    }

  tiz_soa_info (p_soa, &info);
  fail_if (info.chunks != 2);
  fail_if (info.objects != MAX_CLASS0_OBJS + MAX_CLASS4_OBJS);
  fail_if (info.slices[4] != MAX_CLASS4_OBJS);

  tiz_soa_destroy (p_soa);
  fail_if (error != OMX_ErrorNone);

  malloc_trim (0);
  malloc_stats();
  TIZ_LOG (TIZ_PRIORITY_TRACE, "test_soa_reserve_life_cycle - end");
}
END_TEST

/* Local Variables: */
/* c-default-style: gnu */
/* fill-column: 79 */
/* indent-tabs-mode: nil */
/* compile-command: "make check" */
/* End: */
