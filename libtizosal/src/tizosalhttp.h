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
 * @file   tizosalhttp.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  HTTP parser API
 *
 *
 */

#ifndef TIZOSALHTTP_H
#define TIZOSALHTTP_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <OMX_Core.h>
#include <OMX_Types.h>

  typedef struct tiz_http_parser tiz_http_parser_t;
  typedef struct tiz_http_parser *tiz_http_parser_ptr_t;

  typedef enum tiz_http_parser_type
  {
    ETIZHttpParserTypeRequest,
    ETIZHttpParserTypeResponse,
    ETIZHttpParserTypeBoth,
    ETIZHttpParserTypeMax,
  } tiz_http_parser_type_t;

  OMX_ERRORTYPE tiz_http_parser_init (tiz_http_parser_ptr_t * app_parser,
                                      tiz_http_parser_type_t type);

  void tiz_http_parser_destroy (tiz_http_parser_t * ap_parser);

  int tiz_http_parser_parse (tiz_http_parser_t * ap_parser,
                             const char *ap_data, unsigned long a_len);

  const char *tiz_http_parser_get_header (tiz_http_parser_t * ap_parser,
                                          const char *ap_hdr_name);

  const char *tiz_http_parser_get_url (tiz_http_parser_t * ap_parser);

  const char *tiz_http_parser_get_method (tiz_http_parser_t * ap_parser);

  /* Return a string name of the last parser error */
  const char *tiz_http_parser_errno_name (tiz_http_parser_t * ap_parser);

  /* Return a string description of the last parser error */
  const char *tiz_http_parser_errno_description (tiz_http_parser_t *
                                                 ap_parser);

#ifdef __cplusplus
}
#endif

#endif                          /* TIZOSALHTTP_H */
