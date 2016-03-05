/**
 * Copyright (C) 2011-2016 Aratelia Limited - Juan A. Rubio
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
 * @file   check_http_parser.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  http_parser API unit tests
 *
 *
 */


START_TEST (test_http_parser_request_test)
{
  OMX_ERRORTYPE error = OMX_ErrorNone;
  int nparsed = 0;
  tiz_http_parser_t *p_http_parser = NULL;
  const char *p_req =
    "GET /metallica HTTP/1.1\r\n"
    "User-Agent: Audacious/3.2.1 neon/0.29.6\r\n"
    "Connection: TE, close\r\n"
    "TE: trailers\r\n"
    "Host: osoton:8000\r\n"
    "Icy-MetaData: 1\r\n"
    "\r\n";
  int req_len = strlen (p_req);

  error = tiz_http_parser_init (&p_http_parser, ETIZHttpParserTypeRequest);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "tiz_http_init = %s", tiz_err_to_str (error));
  fail_if (error != OMX_ErrorNone);

  nparsed = tiz_http_parser_parse (p_http_parser, p_req, req_len);
  fail_if (nparsed != req_len);

  fail_if (0 != strcmp ("GET",
                        tiz_http_parser_get_method (p_http_parser)));

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Url = [%s]",
           tiz_http_parser_get_url (p_http_parser));

  fail_if (0 != strcmp ("/metallica",
                        tiz_http_parser_get_url (p_http_parser)));

  fail_if (0 != strcmp ("Audacious/3.2.1 neon/0.29.6",
                        tiz_http_parser_get_header (p_http_parser, "User-Agent")));

  TIZ_LOG (TIZ_PRIORITY_TRACE, "User-Agent = [%s]",
           tiz_http_parser_get_header (p_http_parser, "User-Agent"));

  fail_if (0 != strcmp ("TE, close",
                        tiz_http_parser_get_header (p_http_parser, "Connection")));

  fail_if (0 != strcmp ("trailers",
                        tiz_http_parser_get_header (p_http_parser, "TE")));

  fail_if (0 != strcmp ("osoton:8000",
                        tiz_http_parser_get_header (p_http_parser, "Host")));

  fail_if (0 != strcmp ("1",
                        tiz_http_parser_get_header (p_http_parser, "Icy-MetaData")));

  tiz_http_parser_destroy (p_http_parser);
}
END_TEST

/* Local Variables: */
/* c-default-style: gnu */
/* fill-column: 79 */
/* indent-tabs-mode: nil */
/* compile-command: "make check" */
/* End: */
