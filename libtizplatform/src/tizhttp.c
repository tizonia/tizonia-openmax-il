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
 * @file   tizhttp.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief HTTP parser implementation - Based on joyent's http-parser library
 *         (see https://github.com/joyent/http-parser)
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "tizplatform.h"
#include "http-parser/http_parser.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.platform.http"
#endif

#include "avl/avl.h"

struct tiz_http_parser
{
  http_parser parser;
  http_parser_settings settings;
  avl_tree * p_dict;
  char * p_last_header;
};

typedef struct http_kv_pair http_kv_pair_t;
struct http_kv_pair
{
  char * p_key;
  char * p_value;
};

static char *
to_lower_case (char * p_str)
{
  char * p_c = p_str;
  for (; *p_c != '\0'; p_c++)
    {
      *p_c = tolower (*p_c);
    }
  return p_str;
}

static int
compare_kv_pairs (void * compare_arg, void * p_a, void * p_b)
{
  assert (p_a);
  assert (p_b);

  if (NULL == p_a || NULL == p_b)
    {
      return 0;
    }

  return strcmp (((http_kv_pair_t *) p_a)->p_key,
                 ((http_kv_pair_t *) p_b)->p_key);
}

static int
free_kv_pair (void * ap_key)
{
  http_kv_pair_t * p_kvp = ap_key;

  assert (p_kvp);

  if (p_kvp)
    {
      if (p_kvp->p_key)
        {
          tiz_mem_free (p_kvp->p_key);
        }
      if (p_kvp->p_value)
        {
          tiz_mem_free (p_kvp->p_value);
        }
      tiz_mem_free (p_kvp);
    }

  return 1;
}

static const char *
get_kv_value (tiz_http_parser_t * ap_parser, const char * ap_key)
{
  http_kv_pair_t kvp;
  http_kv_pair_t * p_kvp_found = NULL;
  void * pp_kvpf = NULL;

  assert (ap_parser);

  if (NULL == ap_key)
    {
      return NULL;
    }

  pp_kvpf = &p_kvp_found;
  kvp.p_key = (char *) ap_key;
  kvp.p_value = NULL;

  if (avl_get_item_by_key (ap_parser->p_dict, &kvp, pp_kvpf) == 0)
    {
      return p_kvp_found->p_value;
    }

  return NULL;
}

static int
insert_kv_pair (tiz_http_parser_t * ap_parser, const char * ap_key,
                size_t a_name_len, const char * ap_value, size_t a_value_len)
{
  http_kv_pair_t * p_kvp = NULL;

  assert (ap_parser);

  if (NULL == ap_key || NULL == ap_value)
    {
      return 1;
    }

  if (NULL
      == (p_kvp = (http_kv_pair_t *) tiz_mem_alloc (sizeof (http_kv_pair_t))))
    {
      return 1;
    }

  p_kvp->p_key = strndup (ap_key, a_name_len);
  p_kvp->p_value = strndup (ap_value, a_value_len);

  if (NULL == p_kvp->p_key || NULL == p_kvp->p_value)
    {
      tiz_mem_free (p_kvp->p_key);
      tiz_mem_free (p_kvp->p_value);
      tiz_mem_free (p_kvp);
      return 1;
    }

  if (get_kv_value (ap_parser, ap_key) == NULL)
    {
      unsigned long index = 0;
      avl_insert_by_key (ap_parser->p_dict, (void *) p_kvp, &index);
    }
  else
    {
      unsigned long index = 0;
      avl_remove_by_key (ap_parser->p_dict, (void *) p_kvp, free_kv_pair);
      avl_insert_by_key (ap_parser->p_dict, (void *) p_kvp, &index);
    }

  return 0;
}

static int
on_message_begin (http_parser * ap_parser)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "*** on message begin ***");
  return 0;
}

static int
on_url (http_parser * ap_parser, const char * ap_at, size_t a_length)
{
  tiz_http_parser_t * p_hp = (tiz_http_parser_t *) ap_parser;
  const char * p_url_str = "url";

  assert (p_hp);
  assert (ap_at);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "*** on url [%.*s] ***", (int) a_length, ap_at);

  return insert_kv_pair (p_hp, p_url_str, 3, ap_at, a_length);
}

static int
on_status (http_parser * ap_parser, const char * at, size_t length)
{
  (void) ap_parser;
  (void) at;
  (void) length;
  TIZ_LOG (TIZ_PRIORITY_TRACE, "*** on status ***");
  return 0;
}

static int
on_header_field (http_parser * ap_parser, const char * ap_at, size_t a_length)
{
  tiz_http_parser_t * p_hp = (tiz_http_parser_t *) ap_parser;

  assert (p_hp);
  assert (ap_at);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "*** on header field [%.*s] ***", (int) a_length,
           ap_at);

  tiz_mem_free (p_hp->p_last_header);
  p_hp->p_last_header = NULL;
  ;
  p_hp->p_last_header = strndup (ap_at, a_length);

  return 0;
}

static int
on_header_value (http_parser * ap_parser, const char * ap_at, size_t a_length)
{
  tiz_http_parser_t * p_hp = (tiz_http_parser_t *) ap_parser;
  char * p_hdr = p_hp->p_last_header;

  assert (p_hp);
  assert (ap_at);
  assert (p_hdr);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "*** on header value [%.*s] ***", (int) a_length,
           ap_at);

  return insert_kv_pair (p_hp, to_lower_case (p_hdr), strlen (p_hdr), ap_at,
                         a_length);
}

static int
on_headers_complete (http_parser * ap_parser)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "*** on headers complete ***");
  return 0;
}

static int
on_body (http_parser * ap_parser, const char * ap_at, size_t a_length)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "*** on body ***");
  return 0;
}

static int
on_message_complete (http_parser * ap_parser)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "*** on message complete ***");
  return 0;
}

static inline void
clean_up_parser (tiz_http_parser_ptr_t ap_parser)
{
  if (ap_parser)
    {
      if (ap_parser->p_dict)
        {
          avl_free_avl_tree (ap_parser->p_dict, free_kv_pair);
        }
      tiz_mem_free (ap_parser->p_last_header);
      tiz_mem_free (ap_parser);
    }
}

OMX_ERRORTYPE
tiz_http_parser_init (tiz_http_parser_ptr_t * app_parser,
                      tiz_http_parser_type_t type)
{
  tiz_http_parser_ptr_t p_hp = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (app_parser);
  assert (type < ETIZHttpParserTypeMax);

  if (NULL == (p_hp = (tiz_http_parser_ptr_t) tiz_mem_calloc (
                 1, sizeof (tiz_http_parser_t))))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "Error allocating http parser structure.");
      rc = OMX_ErrorInsufficientResources;
      goto end;
    }

  if (NULL == (p_hp->p_dict = avl_new_avl_tree (compare_kv_pairs, NULL)))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "Error allocating avl tree structure.");
      rc = OMX_ErrorInsufficientResources;
      goto end;
    }

  http_parser_init (&(p_hp->parser), (enum http_parser_type) type);

  p_hp->settings.on_message_begin = on_message_begin;
  p_hp->settings.on_url = on_url;
  p_hp->settings.on_status = on_status;
  p_hp->settings.on_header_field = on_header_field;
  p_hp->settings.on_header_value = on_header_value;
  p_hp->settings.on_headers_complete = on_headers_complete;
  p_hp->settings.on_body = on_body;
  p_hp->settings.on_message_complete = on_message_complete;

end:

  if (OMX_ErrorNone != rc)
    {
      clean_up_parser (p_hp);
      p_hp = NULL;
    }
  else
    {
      *app_parser = p_hp;
    }

  return rc;
}

void
tiz_http_parser_destroy (tiz_http_parser_t * ap_parser)
{
  clean_up_parser (ap_parser);
}

int
tiz_http_parser_parse (tiz_http_parser_t * ap_parser, const char * ap_data,
                       unsigned long a_len)
{
  assert (ap_parser);
  assert (ap_data);

  return http_parser_execute ((http_parser *) ap_parser, &(ap_parser->settings),
                              ap_data, a_len);
}

const char *
tiz_http_parser_errno_name (tiz_http_parser_t * ap_parser)
{
  assert (ap_parser);
  return http_errno_name (HTTP_PARSER_ERRNO ((http_parser *) ap_parser));
}

const char *
tiz_http_parser_errno_description (tiz_http_parser_t * ap_parser)
{
  assert (ap_parser);
  return http_errno_description (HTTP_PARSER_ERRNO ((http_parser *) ap_parser));
}

const char *
tiz_http_parser_get_header (tiz_http_parser_t * ap_parser,
                            const char * ap_hdr_name)
{
  assert (ap_parser);
  char * p_str = strndup (ap_hdr_name, HTTP_MAX_HEADER_SIZE);
  const char * p_res = NULL;
  if (p_str)
    {
      p_res = get_kv_value (ap_parser, to_lower_case (p_str));
      tiz_mem_free (p_str);
    }
  return p_res;
}

const char *
tiz_http_parser_get_url (tiz_http_parser_t * ap_parser)
{
  const char * p_url_str = "url";
  assert (ap_parser);
  return get_kv_value (ap_parser, p_url_str);
}

const char *
tiz_http_parser_get_method (tiz_http_parser_t * ap_parser)
{
  assert (ap_parser);
  return http_method_str (((http_parser *) ap_parser)->method);
}
