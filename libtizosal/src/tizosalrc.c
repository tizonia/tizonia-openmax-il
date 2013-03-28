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
 * @file   tizosalrc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia OpenMAX IL - Configuration file utility functions
 *
 * Very simplistic implementation of a config file parser.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>

#include "tizosalrc.h"
#include "tizosalmem.h"
#include "tizosallog.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.osal.rc"
#endif

#define TIZRC_ETCPATH "/etc/tizonia"
#define TIZRC_PATH ""

#define PAT_SIZE 255

static char pat[PAT_SIZE];

typedef struct value value_t;

struct value
{
  char *p_value;
  value_t *p_next;
};

typedef struct keyval keyval_t;

struct keyval
{
  char *p_key;
  value_t *p_value_list;
  value_t *p_value_iter;
  int valcount;
  keyval_t *p_next;
};

struct tiz_rcfile
{
  keyval_t *p_keyvals;
  int count;
};

typedef struct file_info file_info_t;

struct file_info
{
  char name[256];
  time_t ctime;
  int exists;
};

static char *list_keys[] = {
  "component-paths",
};

static const int nrlist_keys = 1;

static file_info_t rcfiles[] = {
  {"$TIZRC_ETCPATH/tizrc"},
  {"$TIZRC_PATH/tizrc"},
  {"$HOME/.tizrc"}
};

static const int g_nrcfiles = sizeof (rcfiles) / sizeof (rcfiles[0]);

/* static void */
/* lowercase (char *str) */
/* { */
/*   int i; */
/*   for (i = 0; str[i]; i++) */
/*     { */
/*       str[i] = tolower (str[i]); */
/*     } */
/* } */


static char *
trimwhitespace (char *str)
{
  char *end;

  /* Trim leading space */
  while (isspace (*str))
    str++;

  if (*str == 0)                /* All spaces? */
    return str;

  /* Trim trailing space */
  end = str + strlen (str) - 1;
  while (end > str && isspace (*end))
    {
      end--;
    }

  /* Write new null terminator */
  *(end + 1) = 0;

  return str;
}

static char *
trimlistseparator (char *str)
{
  char *end;

  /* Trim trailing ';' */
  end = str + strlen (str) - 1;
  while (end > str && ';' == (*end))
    end--;

  /* Write new null terminator */
  *(end + 1) = 0;

  return str;
}

static char *
trimsectioning (char *str)
{
  char *end;

  /* Trim leading '[' */
  while ('[' == (*str))
    str++;

  /* Trim trailing ']' */
  end = str + strlen (str) - 1;
  while (end > str && ']' == (*end))
    end--;

  /* Write new null terminator */
  *(end + 1) = 0;

  return str;
}

static char *
trimcommenting (char *str)
{

  /* Trim leading '#' */
  while ('#' == (*str))
    str++;

  return str;
}

static keyval_t *
find_node (const tiz_rcfile_t * ap_rc, const char *key)
{
  keyval_t *p_kvs = NULL;

  assert (NULL != ap_rc);
  assert (NULL != key);

  p_kvs = ap_rc->p_keyvals;

  TIZ_LOG (TIZ_TRACE, "Searching for Key [%s]", key);

  while (p_kvs && p_kvs->p_key)
    {
      TIZ_LOG (TIZ_TRACE, "Searching Key [%s]", p_kvs->p_key);
      /* TODO: strncmp here */
      if (0 == strcmp (p_kvs->p_key, key))
        {
          TIZ_LOG (TIZ_TRACE, "Found Key [%s]", p_kvs->p_key);
          return p_kvs;
        }
      p_kvs = p_kvs->p_next;
    }

  TIZ_LOG (TIZ_TRACE, "Key not found [%s] [%p]", key, p_kvs);
  return NULL;

}

static bool
is_list (const char *key)
{
  int i;
  for (i = 0; i < nrlist_keys; i++)
    {
      /* TODO: strncmp here */
      if (0 == strcmp (list_keys[i], key))
        {
          TIZ_LOG (TIZ_TRACE, "Found list Key [%s]", key);
          return true;
        }
    }

  return false;
}

static int
get_node (const tiz_rcfile_t * ap_rc, char *str, keyval_t ** app_kv)
{
  int ret = 0;
  char *needle = strstr (str, "=");
  char *key = strndup (trimwhitespace (str), needle - str);
  char *value_start = str + (needle - str) + 1;
  char *value = strndup (trimlistseparator (trimwhitespace (value_start)),
                         strlen (str) - (needle - str));
  keyval_t *p_kv = NULL;
  value_t *p_v = NULL, *p_next_v = NULL;

  assert (ap_rc);
  assert (str);
  assert (app_kv);

  TIZ_LOG (TIZ_TRACE, "key : [%s]", trimwhitespace (key));
  TIZ_LOG (TIZ_TRACE, "val : [%s]",
           trimlistseparator (trimwhitespace (value)));

  /* Find if the key exists already */
  if (!(p_kv = find_node (ap_rc, key)))
    {
      p_kv = (keyval_t *) tiz_mem_calloc (1, sizeof (keyval_t));
      p_v = (value_t *) tiz_mem_calloc (1, sizeof (value_t));

      if (!p_kv || !p_v)
        {
          tiz_mem_free (p_kv);
          tiz_mem_free (p_v);
          tiz_mem_free (value);
          tiz_mem_free (key);
          ret = -1;
        }
      else
        {
          p_v->p_value = value;
          p_kv->p_key = key;
          p_kv->p_value_list = p_v;
          p_kv->p_value_iter = p_v;
          p_kv->valcount++;
          p_kv->p_next = NULL;
          ret = 1;
        }
    }
  else
    {
      /* There is already a node with that key */
      /* Need to check if this node holds value lists */
      if (is_list (key))
        {
          /* Add another value to the list */
          TIZ_LOG (TIZ_TRACE, "Found list Key [%s]", key);
          p_v = (value_t *) tiz_mem_calloc (1, sizeof (value_t));
          p_v->p_value = value;

          p_next_v = p_kv->p_value_list;

          for (;;)
            {
              if (!p_next_v->p_next)
                {
                  p_next_v->p_next = p_v;
                  p_kv->valcount++;
                  TIZ_LOG (TIZ_TRACE, "Added value - "
                           "new valcount [%d]", p_kv->valcount);
                  break;
                }
              p_next_v = p_next_v->p_next;
            }

          ret = 0;

        }
      else
        {
          /* Replace the existing value */
          TIZ_LOG (TIZ_TRACE, "Replacing existing value [%s] "
                   "key [%s]", p_kv->p_value_list->p_value, p_kv->p_key);
          tiz_mem_free (p_kv->p_value_list->p_value);
          p_kv->p_value_list->p_value = value;
          p_kv->valcount = 1;
          ret = 0;
        }
      tiz_mem_free (key);
    }

  *app_kv = p_kv;

  return ret;

}

static int
extractkeyval (FILE * ap_file, char *ap_str, keyval_t ** app_last_kv,
               tiz_rcfile_t * ap_tiz_rcfile)
{
  int len;
  int ret = 0;
  int is_new = 0;
  keyval_t *p_kv = NULL;
  value_t *p_v = NULL, *p_next_v = NULL;

  is_new = get_node (ap_tiz_rcfile, ap_str, &p_kv);

  if (!p_kv)
    {
      TIZ_LOG (TIZ_TRACE, "Could not allocate memory " "for keyval_t...");
      return OMX_ErrorInsufficientResources;
    }

  /* Check if this is an existent node or not */
  if (is_new)
    {
      /* This is a new node */
      TIZ_LOG (TIZ_TRACE, "Linking new kv [%p] [%p] pair - "
               "value [%s]", app_last_kv, p_kv, p_kv->p_value_list->p_value);
      (*app_last_kv) = p_kv;
      p_v = p_kv->p_value_list;
    }
  else
    {
      p_v = p_kv->p_value_list;
      for (;;)
        {
          if (!p_v->p_next)
            {
              break;
            }
          p_v = p_v->p_next;
        }
    }

  while (fgets (pat, PAT_SIZE, ap_file) != NULL)
    {
      if (strstr (pat, ";"))
        {
          TIZ_LOG (TIZ_TRACE, "val : [%s]",
                   trimlistseparator (trimwhitespace (pat)));

          p_next_v = (value_t *) tiz_mem_calloc (1, sizeof (value_t));

          /* TODO : Use strndup */
          p_next_v->p_value =
            strdup (trimlistseparator (trimwhitespace (pat)));

          p_v->p_next = p_next_v;
          p_v = p_next_v;
          p_kv->valcount++;
          TIZ_LOG (TIZ_TRACE, "Added value - "
                   "new valcount [%d]", p_kv->valcount);
        }
      else
        {
          len = strlen (pat);
          if (pat[len - 1] == '\n')
            {
              pat[len - 1] = '\0';
            }

          if (strlen (pat) > 1)
            {
              ret = 1;
            }
          break;
        }
    }

  TIZ_LOG (TIZ_TRACE, "ret : [%d]", ret);

  return ret;
}

static int
analyze_pattern (FILE * ap_file, char *ap_str, keyval_t ** app_last_kv,
                 tiz_rcfile_t * ap_tiz_rcfile)
{
  if (strstr (ap_str, "#"))
    {
      char *str = trimcommenting (trimwhitespace (ap_str));
      TIZ_LOG (TIZ_TRACE, "Comment : [%s]", trimwhitespace (str));
      (void) str;
    }
  else if ('[' == ap_str[0] && ']' == ap_str[strlen (ap_str) - 1])
    {
      char *str = trimsectioning (ap_str);
      TIZ_LOG (TIZ_TRACE, "Section : [%s]", str);
      (void) str;
    }
  else if (strstr (ap_str, "="))
    {
      TIZ_LOG (TIZ_TRACE, "key-value pair : [%s]", ap_str);
      return extractkeyval (ap_file, ap_str, app_last_kv, ap_tiz_rcfile);
    }

  return 0;
}

static int
load_rc_file (const file_info_t * ap_finfo, tiz_rcfile_t * ap_tiz_rcfile)
{
  FILE *p_file = 0;
  int len;
  keyval_t **pp_last_kv = NULL, *p_kv = NULL;

  assert (ap_finfo);
  assert (ap_finfo->name);
  assert (ap_tiz_rcfile);

  TIZ_LOG (TIZ_TRACE, "[%s]", ap_finfo->name);

  if ((p_file = fopen (ap_finfo->name, "r")) == 0)
    {
      return -1;
    }

  if (!ap_tiz_rcfile->p_keyvals)
    {
      pp_last_kv = &ap_tiz_rcfile->p_keyvals;
    }
  else
    {
      p_kv = ap_tiz_rcfile->p_keyvals;
      for (;;)
        {
          if (!p_kv || !p_kv->p_next)
            {
              pp_last_kv = &p_kv->p_next;
              break;
            }
          p_kv = p_kv->p_next;
        }

    }

  for (;;)
    {
      if (fgets (pat, PAT_SIZE, p_file) == NULL)
        {
          TIZ_LOG (TIZ_TRACE, "EOF?");
          break;
        }

      len = strlen (pat);
      if (len <= 1)
        {
          continue;
        }

      if (pat[len - 1] == '\n')
        {
          pat[len - 1] = '\0';
        }

      while (analyze_pattern (p_file, pat, pp_last_kv, ap_tiz_rcfile))
        {
          TIZ_LOG (TIZ_TRACE, "pp_last_kv = [%p]", pp_last_kv);
          if (pp_last_kv && *pp_last_kv)
            {
              pp_last_kv = &(*pp_last_kv)->p_next;
              ap_tiz_rcfile->count++;
              TIZ_LOG (TIZ_TRACE, "pair count = [%d]",
                       ap_tiz_rcfile->count);
            }
        };

      if (pp_last_kv && *pp_last_kv)
        {
          pp_last_kv = &(*pp_last_kv)->p_next;
          ap_tiz_rcfile->count++;
          TIZ_LOG (TIZ_TRACE, "pair count = [%d]", ap_tiz_rcfile->count);
        }

    }

  fclose (p_file);

  return 0;

}

static int
stat_ctime (const char *path, time_t * time)
{
  struct stat astat;
  int statret = stat (path, &astat);
  if (0 != statret)
    {
      return statret;
    }
  *time = astat.st_ctime;
  return statret;
}

OMX_ERRORTYPE
tiz_rcfile_open (tiz_rcfile_t ** pp_rc)
{
  int i;
  tiz_rcfile_t *p_rc = NULL;
  char *p_env_str = NULL;

  assert (pp_rc);

  /* load rc files */
  TIZ_LOG (TIZ_TRACE, "Looking for [%d] rc files...", g_nrcfiles);

  snprintf (rcfiles[0].name, sizeof (rcfiles[0].name) - 1, "%s/tizrc",
            TIZRC_ETCPATH);

  if (g_nrcfiles >= 1 && (p_env_str = getenv ("TIZRC_PATH")))
    {
      snprintf (rcfiles[1].name, sizeof (rcfiles[1].name) - 1, "%s/tizrc",
                p_env_str ? p_env_str : TIZRC_PATH);
    }

  if (g_nrcfiles >= 2 && (p_env_str = getenv ("HOME")))
    {
      snprintf (rcfiles[2].name, sizeof (rcfiles[2].name) - 1, "%s/.tizrc",
                p_env_str ? p_env_str : "");
    }

  if (!(p_rc = (tiz_rcfile_t *) tiz_mem_calloc (1, sizeof (tiz_rcfile_t))))
    {
      TIZ_LOG (TIZ_TRACE, "Could not allocate memory "
               "for tiz_rcfile_t...");
      return OMX_ErrorInsufficientResources;
    }

  for (i = 0; i < g_nrcfiles; i++)
    {
      TIZ_LOG (TIZ_TRACE, "Checking for rc file at [%s]",
               rcfiles[i].name);

      /* Check file existence and user's read access */
      if (0 != access (rcfiles[i].name, R_OK))
        {
          TIZ_LOG (TIZ_TRACE, "rc file [%s] does not exist or "
                   "user has no read access permission", rcfiles[i].name);
          continue;
        }

      /* Store stat's ctime */
      if (stat_ctime (rcfiles[i].name, &rcfiles[i].ctime) != 0)
        {
          TIZ_LOG (TIZ_TRACE, "stat_ctime for [%s] failed",
                   rcfiles[i].name);
        }

      rcfiles[i].exists = 1;

      if (0 != load_rc_file (&rcfiles[i], p_rc))
        {
          TIZ_LOG (TIZ_TRACE, "Loading [%s] rc file failed",
                   rcfiles[i].name);
        }
      else
        {
          TIZ_LOG (TIZ_TRACE, "Loading [%s] rc file succeeded",
                   rcfiles[i].name);
        }
    }

  if (p_rc->count)
    {
      *pp_rc = p_rc;
    }
  else
    {
      *pp_rc = NULL;
    }

  return OMX_ErrorNone;
}

const char *
tiz_rcfile_get_value (tiz_rcfile_t * p_rc,
                      const char *ap_section, const char *ap_key)
{
  keyval_t *p_kv = NULL;

  assert (p_rc);
  assert (ap_section);
  assert (ap_key);
  assert (is_list (ap_key) == false);

  TIZ_LOG (TIZ_TRACE, "Retrieving value for Key [%s] in section [%s]",
           ap_key, ap_section);

  if ((p_kv = find_node (p_rc, ap_key)))
    {
      return p_kv->p_value_list->p_value;
    }

  return OMX_ErrorNone;
}

char **
tiz_rcfile_get_value_list (tiz_rcfile_t * p_rc,
                           const char *ap_section,
                           const char *ap_key, unsigned long *ap_length)
{
  keyval_t *p_kv = NULL;
  char **pp_ret = NULL;
  value_t *p_next_value = NULL;

  assert (p_rc);
  assert (ap_section);
  assert (ap_key);
  assert (ap_length);
  assert (is_list (ap_key) == true);

  TIZ_LOG (TIZ_TRACE, "Retrieving value list "
           "for Key [%s] in section [%s]", ap_key, ap_section);

  if ((p_kv = find_node (p_rc, ap_key)))
    {
      int i = 0;
      TIZ_LOG (TIZ_TRACE, "Found value list - count [%d]",
               p_kv->valcount);

      *ap_length = p_kv->valcount;
      pp_ret = (char **) tiz_mem_alloc (sizeof (char *) * p_kv->valcount);

      p_next_value = p_kv->p_value_list;
      for (i = 0; i < p_kv->valcount; ++i)
        {
          if (p_next_value)
            {
              /* TODO : Use strndup */
              pp_ret[i] = strdup (p_next_value->p_value);
              TIZ_LOG (TIZ_TRACE, "item [%d] - val [%s]",
                       i, p_next_value->p_value);
              p_next_value = p_next_value->p_next;
            }
        }

    }

  return pp_ret;
}

OMX_ERRORTYPE
tiz_rcfile_close (tiz_rcfile_t * p_rc)
{
  keyval_t *p_kv_lst = NULL;
  keyval_t *p_kvt = NULL;
  value_t *p_val_lst = NULL;

  if (!p_rc)
    {
      return OMX_ErrorNone;
    }

  p_kv_lst = p_rc->p_keyvals;
  while (p_kv_lst)
    {
      value_t *p_vt = NULL;
      TIZ_LOG (TIZ_TRACE, "Deleting Key [%s]", p_kv_lst->p_key);
      tiz_mem_free (p_kv_lst->p_key);
      p_val_lst = p_kv_lst->p_value_list;
      while (p_val_lst)
        {
          TIZ_LOG (TIZ_TRACE, "Deleting Val [%s]", p_val_lst->p_value);
          p_vt = p_val_lst;
          p_val_lst = p_val_lst->p_next;
          tiz_mem_free (p_vt->p_value);
          tiz_mem_free (p_vt);
        }
      p_kvt = p_kv_lst;
      p_kv_lst = p_kv_lst->p_next;
      tiz_mem_free (p_kvt);
    }

  tiz_mem_free (p_rc);

  return OMX_ErrorNone;
}
