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
 * @file   tizmacros.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia Platform - Macro utilities
 *
 *
 */

#ifndef TIZMACROS_H
#define TIZMACROS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tizomxutils.h"
#include "tizomxutils.h"

#include <stddef.h>

#undef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#undef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
#define TIZ_LIKELY(expr) (__builtin_expect (expr, 1))
#define TIZ_UNLIKELY(expr) (__builtin_expect (expr, 0))
#else
#define TIZ_LIKELY(expr) (expr)
#define TIZ_UNLIKELY(expr) (expr)
#endif

/**
 * tiz_check_omx:
 * @expr: An OMX expression to check
 *
 * Verifies that the expression evaluates to OMX_ErrorNone.  Otherwise an error
 * message is logged and the current function returns the resulting openmax il
 * error.
 */
#define tiz_check_omx(expr)                                            \
  do                                                                   \
    {                                                                  \
      OMX_ERRORTYPE _err = (expr);                                     \
      if                                                               \
        TIZ_LIKELY (OMX_ErrorNone == _err)                             \
        {                                                              \
        }                                                              \
      else                                                             \
        {                                                              \
          TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s]", tiz_err_to_str (_err)); \
          return _err;                                                 \
        }                                                              \
    }                                                                  \
  while (0)

/**
 * tiz_check_omx_ret_oom:
 * @expr: The OMX expression to check
 *
 * Verifies that the OpenMAX IL expression evaluates to OMX_ErrorNone.
 * Otherwise an error message is logged and the current function returns
 * OMX_ErrorInsufficientResources.
 */
#define tiz_check_omx_ret_oom(expr)                    \
  do                                                   \
    {                                                  \
      OMX_ERRORTYPE _err = (expr);                     \
      if                                               \
        TIZ_LIKELY (OMX_ErrorNone == _err)             \
        {                                              \
        }                                              \
      else                                             \
        {                                              \
          TIZ_LOG (TIZ_PRIORITY_ERROR,                 \
                   "[OMX_ErrorInsufficientResources] " \
                   "was [%s]",                         \
                   tiz_err_to_str (_err));             \
          return OMX_ErrorInsufficientResources;       \
        }                                              \
    }                                                  \
  while (0)

/**
 * tiz_check_omx_ret_null:
 * @expr: the OMX expression to check.
 *
 * Verifies that the expression evaluates to OMX_ErrorNone. Otherwise an error
 * message is logged and the current function returns NULL.
 */
#define tiz_check_omx_ret_null(expr)                        \
  do                                                        \
    {                                                       \
      OMX_ERRORTYPE _err = (expr);                          \
      if                                                    \
        TIZ_LIKELY (OMX_ErrorNone == _err)                  \
        {                                                   \
        }                                                   \
      else                                                  \
        {                                                   \
          TIZ_LOG (TIZ_PRIORITY_ERROR, "[NULL] : was [%s]", \
                   tiz_err_to_str (_err));                  \
          return NULL;                                      \
        }                                                   \
    }                                                       \
  while (0)

/**
 * tiz_check_omx_ret_val:
 * @expr: the OMX expression to be checked.
 * @val: the OpenMAX IL error value that is returned when the OMX expression
 * evaluates to false.
 *
 * Verifies that the expression evaluates to OMX_ErrorNone. Otherwise an error
 * message is logged and the current function returns @val (an OpenMAX IL
 * error).
 */
#define tiz_check_omx_ret_val(expr, val)                         \
  do                                                             \
    {                                                            \
      OMX_ERRORTYPE _err = (expr);                               \
      if                                                         \
        TIZ_LIKELY (OMX_ErrorNone == _err)                       \
        {                                                        \
        }                                                        \
      else                                                       \
        {                                                        \
          TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s] : was [%s]",        \
                   tiz_err_to_str (val), tiz_err_to_str (_err)); \
          return (val);                                          \
        }                                                        \
    }                                                            \
  while (0)

/**
 * tiz_check_null:
 * @expr: A expression to check against NULL
 *
 * Verifies that the expression evaluates to non-NULL. Otherwise an error
 * message is logged and the current function returns NULL.
 */
#define tiz_check_null(expr)                                   \
  do                                                           \
    {                                                          \
      if                                                       \
        TIZ_LIKELY ((expr))                                    \
        {                                                      \
        }                                                      \
      else                                                     \
        {                                                      \
          TIZ_LOG (TIZ_PRIORITY_ERROR, "[NULL] : [%s]", #expr) \
          return NULL;                                         \
        }                                                      \
    }                                                          \
  while (0)

/**
 * tiz_check_null_ret_oom:
 * @expr: the expression to check
 *
 * Verifies that the expression evaluates to non-NULL. Otherwise an error
 * message is logged and the current function returns
 * OMX_ErrorInsufficientResources.
 */
#define tiz_check_null_ret_oom(expr)                                        \
  do                                                                        \
    {                                                                       \
      if                                                                    \
        TIZ_LIKELY (expr != NULL)                                           \
        {                                                                   \
        }                                                                   \
      else                                                                  \
        {                                                                   \
          TIZ_LOG (TIZ_PRIORITY_ERROR, "[OMX_ErrorInsufficientResources]"); \
          return OMX_ErrorInsufficientResources;                            \
        }                                                                   \
    }                                                                       \
  while (0)

/**
 * tiz_check_true_ret_void:
 * @expr: the expression to check
 *
 * Verifies that the expression evaluates to true.  If the expression evaluates
 * to false, an error message is logged and the current function returns.  This
 * is to be used only in functions that do not return a value.
 */
#define tiz_check_true_ret_void(expr)                               \
  do                                                                \
    {                                                               \
      if                                                            \
        TIZ_LIKELY ((expr))                                         \
        {                                                           \
        }                                                           \
      else                                                          \
        {                                                           \
          TIZ_LOG (TIZ_PRIORITY_ERROR, "Check '%s' failed", #expr); \
          return;                                                   \
        }                                                           \
    }                                                               \
  while (0)

/**
 * tiz_check_true_ret_val:
 * @expr: the expression to check
 * @val: the value to return if the expression does not evaluate to true
 *
 * Verifies that the expression evaluates to true.  If the expression evaluates
 * to false, an error message is logged and @val is returned from the current
 * function.
  */
#define tiz_check_true_ret_val(expr, val)                               \
  do                                                                    \
    {                                                                   \
      if                                                                \
        TIZ_LIKELY ((expr))                                             \
        {                                                               \
        }                                                               \
      else                                                              \
        {                                                               \
          if (0 == (int) val)                                           \
            {                                                           \
              TIZ_LOG (TIZ_PRIORITY_DEBUG, "Check '%s' failed", #expr); \
            }                                                           \
          else                                                          \
            {                                                           \
              TIZ_LOG (TIZ_PRIORITY_ERROR, "Check '%s' failed", #expr); \
            }                                                           \
          return val;                                                   \
        }                                                               \
    }                                                                   \
  while (0)

/**
 * tiz_goto_end_on_omx_err:
 * @omx_expr: the OMX expression to check
 * @msg: A log message to output in case of error
 *
 * Verifies that the expression evaluates to OMX_ErrorNone.  Otherwise a message is logged
 * and control is transferred to the 'end' label (this must exist outside of this macro).
  */
#define tiz_goto_end_on_omx_err(omx_expr, msg)                              \
  do                                                                        \
    {                                                                       \
      if (OMX_ErrorNone != (omx_expr))                                      \
        {                                                                   \
          TIZ_LOG (TIZ_PRIORITY_ERROR, "Check '%s' failed [%s]", #omx_expr, \
                   msg);                                                    \
          goto end;                                                         \
        }                                                                   \
    }                                                                       \
  while (0)

/**
 * tiz_goto_end_on_null:
 * @expr: the pointer expression to check
 * @msg: A log message to output in case of error
 *
 * Verifies that the expression evaluates to non-NULL.  Otherwise a message is logged
 * and control is transferred to the 'end' label (this must exist outside of this macro).
  */
#define tiz_goto_end_on_null(expr, msg)                                       \
  do                                                                          \
    {                                                                         \
      if (NULL == (expr))                                                     \
        {                                                                     \
          TIZ_LOG (TIZ_PRIORITY_ERROR, "Check '%s' failed [%s]", #expr, msg); \
          goto end;                                                           \
        }                                                                     \
    }                                                                         \
  while (0)

/* DEPRECATED */
/**
 * tiz_check_omx_err:
 * @deprecated From v0.6.0. Use tiz_check_omx instead. To be removed in
 * a future release.
 */
#define tiz_check_omx_err (expr) tiz_check_omx (expr)

/* DEPRECATED */
/**
 * tiz_check_omx_err_ret_oom:
 * @deprecated From v0.6.0. Use tiz_check_omx_ret_oom instead. To be removed in
 * a future release.
 */
#define tiz_check_omx_err_ret_oom(expr) tiz_check_omx_ret_oom (expr)

/* DEPRECATED */
/**
 * tiz_check_omx_err_ret_null:
 * @deprecated From v0.6.0. Use tiz_check_omx_ret_null instead. To be removed in
 * a future release.
 */
#define tiz_check_omx_err_ret_null(expr) tiz_check_omx_ret_null (expr)

/* DEPRECATED */
/**
 * tiz_check_omx_err_ret_val:
 * @deprecated From v0.6.0. Use tiz_check_omx_ret_val instead. To be removed in
 * a future release.
 */
#define tiz_check_omx_err_ret_val(expr, val) tiz_check_omx_ret_val (expr, val)

/* DEPRECATED */
/**
 * tiz_ret_on_err:
 * @deprecated From v0.6.0. Use tiz_check_true_ret_void instead. To be removed in
 * a future release.
 */
#define tiz_ret_on_err (expr) tiz_check_true_ret_void (expr)

/* DEPRECATED */
/**
 * tiz_ret_val_on_err:
 * @deprecated From v0.6.0. Use tiz_check_true_ret_val instead. To be removed in
 * a future release
 */
#define tiz_ret_val_on_err (expr, val) tiz_check_true_ret_val (expr, val)

/* Avoid unused variable warnings */

#ifdef TIZ_UNUSED
#elif defined(__GNUC__)
#define TIZ_UNUSED(x) UNUSED_##x __attribute__ ((unused))
#elif defined(__LCLINT__)
#define TIZ_UNUSED(x) /*@unused@*/ x
#elif defined(__cplusplus)
#define TIZ_UNUSED(x)
#else
#define TIZ_UNUSED(x) x
#endif

/* Turn off ASAN (Address Sanitazer) */

#if defined(__clang__) || defined(__GNUC__)
#define ATTRIBUTE_NO_SANITIZE_ADDRESS __attribute__ ((no_sanitize_address))
#else
#define ATTRIBUTE_NO_SANITIZE_ADDRESS
#endif

#ifdef __cplusplus
}
#endif

#endif /* TIZMACROS_H */
