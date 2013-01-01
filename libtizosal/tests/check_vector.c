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
 * @file   check_vector.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tests for the vector API implementation
 *
 *
 */

/* static OMX_S32 */
/* vector_cmp(OMX_PTR ap_left, OMX_PTR ap_right) */
/* { */
/*   TIZ_LOG(TIZ_LOG_TRACE, "left [%d] right [%d]", */
/*             *(int*)ap_left, *(int*)ap_right); */

/*   if ((*(int*)ap_left) == (*(int*)ap_right)) */
/*     { */
/*       return 0; */
/*     } */
/*   else if ((*(int*)ap_left) < (*(int*)ap_right)) */
/*     { */
/*       return -1; */
/*     } */

/*   return 1; */

/* } */

START_TEST (test_vector_init_and_destroy)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  tiz_vector_t *p_vector;

  TIZ_LOG (TIZ_LOG_TRACE, "test_vector_init_and_destroy");

  error = tiz_vector_init (&p_vector, 10);

  fail_if (error != OMX_ErrorNone);

  tiz_vector_destroy (p_vector);
}
END_TEST

START_TEST (test_vector_push_and_pop_length_front_back_ints)
{
  OMX_U32 i;
  OMX_ERRORTYPE error = OMX_ErrorNone;
  int *p_item = NULL;
  tiz_vector_t *p_vector = NULL;

  TIZ_LOG (TIZ_LOG_TRACE, "test_vector_push_and_pop_length_front_back_ints");

  error = tiz_vector_init (&p_vector, sizeof(int));
  fail_if (error != OMX_ErrorNone);

  for (i = 0; i < 10; i++)
    {
      p_item = (int *) tiz_mem_alloc (sizeof (int));
      fail_if (p_item == NULL);
      *p_item = i;

      error = tiz_vector_push_back (p_vector, p_item);
      fail_if (error != OMX_ErrorNone);

      p_item = tiz_vector_back (p_vector);
      fail_if (*p_item != i);
    }

  TIZ_LOG (TIZ_LOG_TRACE, "vector length %d",
             tiz_vector_length (p_vector));
  fail_if (10 != tiz_vector_length (p_vector));

  for (i = 0; i < 10; i++)
    {
      fail_if (10 - i != tiz_vector_length (p_vector));

      p_item = tiz_vector_back (p_vector);
      fail_if (*p_item != 10 - i - 1);

      tiz_vector_pop_back (p_vector);
    }

  fail_if (0 != tiz_vector_length (p_vector));

  tiz_vector_destroy (p_vector);
}
END_TEST

START_TEST (test_vector_push_and_pop_length_front_back_pointers)
{
  OMX_U32 i;
  OMX_ERRORTYPE error = OMX_ErrorNone;
  int *p_item = NULL;
  tiz_vector_t *p_vector = NULL;

  TIZ_LOG (TIZ_LOG_TRACE, "test_vector_push_and_pop_length_front_back_pointers");

  error = tiz_vector_init (&p_vector, sizeof(int*));
  fail_if (error != OMX_ErrorNone);

  for (i = 0; i < 10; i++)
    {
      p_item = (int *) tiz_mem_alloc (sizeof (int));
      fail_if (p_item == NULL);
      *p_item = i;

      error = tiz_vector_push_back (p_vector, &p_item);
      fail_if (error != OMX_ErrorNone);

      p_item = *(int **) tiz_vector_back (p_vector);
      fail_if (*p_item != i);
    }

  TIZ_LOG (TIZ_LOG_TRACE, "vector length %d",
             tiz_vector_length (p_vector));
  fail_if (10 != tiz_vector_length (p_vector));

  for (i = 0; i < 10; i++)
    {
      fail_if (10 - i != tiz_vector_length (p_vector));

      p_item = *(int **) tiz_vector_back (p_vector);
      fail_if (*p_item != 10 - i - 1);

      tiz_vector_pop_back (p_vector);
      tiz_mem_free(p_item);
    }

  fail_if (0 != tiz_vector_length (p_vector));
  tiz_vector_destroy (p_vector);
}
END_TEST

START_TEST (test_vector_push_back_vector)
{
  OMX_U32 i = 0;
  OMX_ERRORTYPE error = OMX_ErrorNone;
  int *p_item = NULL;
  tiz_vector_t *p_vector = NULL;
  tiz_vector_t *p_vector2 = NULL;

  TIZ_LOG (TIZ_LOG_TRACE, "test_vector_push_back_vector");

  error = tiz_vector_init (&p_vector, sizeof(int*));
  error = tiz_vector_init (&p_vector2, sizeof(tiz_vector_t*));

  fail_if (error != OMX_ErrorNone);

  for (i = 0; i < 10; i++)
    {
      p_item = (int *) tiz_mem_alloc (sizeof (int));
      fail_if (p_item == NULL);
      *p_item = i;

      error = tiz_vector_push_back (p_vector, &p_item);
      fail_if (error != OMX_ErrorNone);

      p_item = *(int **) tiz_vector_back (p_vector);
      fail_if (*p_item != i);
    }

  TIZ_LOG (TIZ_LOG_TRACE, "vector length %d",
             tiz_vector_length (p_vector));
  fail_if (10 != tiz_vector_length (p_vector));


  TIZ_LOG (TIZ_LOG_TRACE, "pushing vector [%p] [%p]", p_vector, &p_vector);

  tiz_vector_push_back (p_vector2, &p_vector);
  p_vector = *(tiz_vector_t **) tiz_vector_back (p_vector2);

  TIZ_LOG (TIZ_LOG_TRACE, "received vector [%p] [%p] [%p]",
           p_vector, &p_vector, *(tiz_vector_t **) p_vector);

  for (i = 0; i < 10; i++)
    {
      fail_if (10 - i != tiz_vector_length (p_vector));

      p_item = * (int **) tiz_vector_back (p_vector);
      fail_if (*p_item != 10 - i - 1);

      TIZ_LOG (TIZ_LOG_TRACE, "received int [%d]", *p_item);

      tiz_vector_pop_back (p_vector);
      tiz_mem_free(p_item);
    }

  fail_if (0 != tiz_vector_length (p_vector));

  tiz_vector_destroy (p_vector);
}
END_TEST

