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
 * @file   tizrc.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief Tizonia Platform - Configuration file utility functions
 *
 * Very naive implementation of a ini file parser (to be replaced in the near
 * future with a json-based utility).
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <wordexp.h>

#include <tizplatform.h>
#include "tizplatform_internal.h"
#include "tizplatform_config.h"

#ifndef SYSCONFDIR
#define SYSCONFDIR "/etc"
#endif

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.platform.rc"
#endif

#define PAT_SIZE PATH_MAX

static char delim[2] = {';', '\0'};
static char pat[PAT_SIZE];

typedef struct file_info file_info_t;
struct file_info
{
  char name[PATH_MAX + NAME_MAX];
  time_t ctime;
  int exists;
};

static char * g_list_value_keys[] = {
  "component-paths",
};

static const int g_num_list_value_keys = 1;

static file_info_t g_rcfiles[] = {{"$SYSCONFDIR/tizonia/tizonia.conf"},
                                  {"$HOME/.config/tizonia/tizonia.conf"},
                                  {"$TIZONIA_RC_FILE/tizonia.conf"}};

static const int g_num_rcfiles = sizeof (g_rcfiles) / sizeof (g_rcfiles[0]);

static char *
trimwhitespace (char * str)
{
  char * end;

  /* Trim leading space */
  while (isspace (*str))
    str++;

  if (*str == 0) /* All spaces? */
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
trimlistseparator (char * str)
{
  char * end;

  /* Trim trailing ';' */
  end = str + strlen (str) - 1;
  while (end > str && ';' == (*end))
    end--;

  /* Write new null terminator */
  *(end + 1) = 0;

  return str;
}

static char *
trimsectioning (char * str)
{
  char * end;

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
trimcommenting (char * str)
{

  /* Trim leading '#' */
  while ('#' == (*str))
    str++;

  return str;
}

static keyval_t *
find_node (const tiz_rcfile_t * ap_rc, const char * key)
{
  keyval_t * p_kvs = NULL;

  assert (ap_rc);
  assert (key);

  p_kvs = ap_rc->p_keyvals;

  while (p_kvs && p_kvs->p_key)
    {
      if (0 == strncmp (p_kvs->p_key, key, PATH_MAX))
        {
          return p_kvs;
        }
      p_kvs = p_kvs->p_next;
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Key not found [%s] [%p]", key, p_kvs);
  return NULL;
}

static bool
is_list (const char * key)
{
  int i;
  for (i = 0; i < g_num_list_value_keys; i++)
    {
      if (0 == strncmp (g_list_value_keys[i], key, PATH_MAX))
        {
          return true;
        }
    }

  return false;
}

static int
get_node (const tiz_rcfile_t * ap_rc, char * str, keyval_t ** app_kv)
{
  int ret = 0;
  char * needle = strstr (str, "=");
  char * key = strndup (trimwhitespace (str), needle - str);
  char * value_start = str + (needle - str) + 1;
  char * value = strndup (trimlistseparator (trimwhitespace (value_start)),
                          strlen (str) - (needle - str));
  keyval_t * p_kv = NULL;
  value_t * p_v = NULL;
  value_t * p_next_v = NULL;

  assert (ap_rc);
  assert (str);
  assert (app_kv);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "key : [%s]", trimwhitespace (key));
  TIZ_LOG (TIZ_PRIORITY_TRACE, "val : [%s]",
           trimlistseparator (trimwhitespace (value)));

/*   if (strstr (value, "\"")) */
/*     { */
/*       char quoted[PATH_MAX]; */
/*       printf ("vakue : %s\n", value); */
/*       if (sscanf (str, "%*[^\"]\"%31[^\"]\"", quoted) == 1) */
/*         { */
/*           free (value); value = NULL; */
/*           value = strdup (quoted); */
/*           printf ("got '%s'\n", value); */
/*         } */
/*     } */

  /* Find if the key exists already */
  p_kv = find_node (ap_rc, key);
  if (!p_kv)
    {
      p_kv = (keyval_t *) tiz_mem_calloc (1, sizeof (keyval_t));
      p_v = (value_t *) tiz_mem_calloc (1, sizeof (value_t));

      if (!p_kv || !p_v)
        {
          tiz_mem_free (p_kv);
          p_kv = NULL;
          tiz_mem_free (p_v);
          p_v = NULL;
          tiz_mem_free (value);
          value = NULL;
          tiz_mem_free (key);
          key = NULL;
          ret = -1;
        }
      else
        {
          p_kv->p_key = key;
          p_kv->p_value_list = p_v;
          p_kv->p_value_iter = p_v;
          p_kv->valcount++;
          p_kv->p_next = NULL;
          if (is_list (key))
            {
              char * token = strtok (value, delim);
              while (token)
                {
                  p_v->p_value = strndup (token, PATH_MAX);
                  token = strtok (0, delim);
                  if (token)
                    {
                      p_v->p_next
                        = (value_t *) tiz_mem_calloc (1, sizeof (value_t));
                      p_v = p_v->p_next;
                      p_kv->valcount++;
                    }
                }
              tiz_mem_free (value);
              value = NULL;
            }
          else
            {
              p_v->p_value = value;
            }
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
          p_v = (value_t *) tiz_mem_calloc (1, sizeof (value_t));
          p_v->p_value = value;

          p_next_v = p_kv->p_value_list;

          for (;;)
            {
              if (!p_next_v->p_next)
                {
                  p_next_v->p_next = p_v;
                  p_kv->valcount++;
                  break;
                }
              p_next_v = p_next_v->p_next;
            }

          ret = 0;
        }
      else
        {
          /* Replace the existing value */
          tiz_mem_free (p_kv->p_value_list->p_value);
          p_kv->p_value_list->p_value = value;
          p_kv->valcount = 1;
          ret = 0;
        }
      tiz_mem_free (key);
      key = NULL;
    }

  if (-1 != ret)
    {
      *app_kv = p_kv;
    }

  return ret;
}

static int
extractkeyval (FILE * ap_file, char * ap_str, keyval_t ** app_last_kv,
               tiz_rcfile_t * ap_tiz_rcfile)
{
  int len;
  int ret = 0;
  int is_new = 0;
  keyval_t * p_kv = NULL;
  value_t *p_v = NULL, *p_next_v = NULL;

  is_new = get_node (ap_tiz_rcfile, ap_str, &p_kv);

  if (!p_kv)
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR,
               "Could not allocate memory "
               "for keyval_t...");
      return OMX_ErrorInsufficientResources;
    }

  /* Check if this is an existent node or not */
  if (is_new)
    {
      /* This is a new node */
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
          p_next_v = (value_t *) tiz_mem_calloc (1, sizeof (value_t));

          p_next_v->p_value
            = strndup (trimlistseparator (trimwhitespace (pat)), PATH_MAX);

          p_v->p_next = p_next_v;
          p_v = p_next_v;
          p_kv->valcount++;
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

  return ret;
}

static char *
shell_expand_value (char * p_value)
{
  char *p_expanded = p_value;
  if (p_value)
    {
      wordexp_t p;
      wordexp (p_value, &p, 0);
      if (p.we_wordc > 0)
        {
          char ** w;
          w = p.we_wordv;
          p_expanded = strndup (w[0], PATH_MAX);
        }
      else
        {
          p_expanded = strndup (p_value, PATH_MAX);
        }
      wordfree (&p);
    }
  return p_expanded;
}

static char *
shell_expand_value_in_place (char * p_value, value_t * p_value_list)
{
  char * p_expanded = p_value;
  assert (p_value_list);
  if (p_value && p_value_list)
    {
      wordexp_t p;
      if (0 == wordexp (p_value, &p, 0) && p.we_wordc > 0)
        {
          char ** w;
          w = p.we_wordv;
          p_expanded = strndup (w[0], PATH_MAX);
          /* Replace the existing value */
          tiz_mem_free (p_value_list->p_value);
          p_value_list->p_value = p_expanded;
          wordfree (&p);
       }
    }
  return p_expanded;
}

static int
analyze_pattern (FILE * ap_file, char * ap_str, keyval_t ** app_last_kv,
                 tiz_rcfile_t * ap_tiz_rcfile)
{
  if (strstr (ap_str, "#"))
    {
      char * str = trimcommenting (trimwhitespace (ap_str));
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Comment : [%s]", trimwhitespace (str));
      (void) str;
    }
  else if ('[' == ap_str[0] && ']' == ap_str[strlen (ap_str) - 1])
    {
      char * str = trimsectioning (ap_str);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Section : [%s]", str);
      (void) str;
    }
  else if (strstr (ap_str, "="))
    {
      char * str = trimwhitespace (ap_str);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "key-value pair : [%s]", str);
      return extractkeyval (ap_file, str, app_last_kv, ap_tiz_rcfile);
    }

  return 0;
}

static int
load_rc_file (const file_info_t * ap_finfo, tiz_rcfile_t * ap_tiz_rcfile)
{
  FILE * p_file = 0;
  int len;
  keyval_t **pp_last_kv = NULL, *p_kv = NULL;

  assert (ap_finfo);
  assert (ap_finfo->name);
  assert (ap_tiz_rcfile);

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
          if (pp_last_kv && *pp_last_kv)
            {
              pp_last_kv = &(*pp_last_kv)->p_next;
              ap_tiz_rcfile->count++;
            }
        };

      if (pp_last_kv && *pp_last_kv)
        {
          pp_last_kv = &(*pp_last_kv)->p_next;
          ap_tiz_rcfile->count++;
        }
    }

  fclose (p_file);

  return 0;
}

static int
stat_ctime (const char * path, time_t * time)
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

static int
try_open_file (char * p_file_name)
{
  int retcode = -1;
  assert (p_file_name);
  if (p_file_name)
    {
      FILE * p_file = NULL;
      if ((p_file = fopen (p_file_name, "r")) != 0)
        {
          fclose (p_file);
          retcode = 0;
        }
    }
  return retcode;
}

/* $XDG_CONFIG_DIRS
   defines the preference-ordered set of base directories
   to search for configuration files in addition to the $XDG_CONFIG_HOME
   base directory. The directories in $XDG_CONFIG_DIRS should be seperated
   with a colon ':'. */
/* If $XDG_CONFIG_DIRS is either not set or empty, a value equal to
   /etc/xdg/tizonia/tizonia.conf should be used. */
/* In addition, if /etc/xdg/tizonia/tizonia.conf does not exist, a value equal
   to /etc/tizonia/tizonia.conf will be used */
static void
obtain_xdg_config_dir (void)
{
  int found = -1;
  char rcfile[PATH_MAX + NAME_MAX];
  char * p_env_str = NULL;

  if ((p_env_str = getenv ("XDG_CONFIG_DIRS")))
    {
      char * pch = NULL;
      TIZ_LOG (TIZ_PRIORITY_TRACE, "XDG_CONFIG_DIRS [%s] ...", p_env_str);
      pch = strtok (p_env_str, ":");
      while (pch != NULL && found != 0)
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "XDG_CONFIG_DIR - [%s] ...", pch);
          snprintf (rcfile, PATH_MAX + NAME_MAX - 1, "%s/tizonia/tizonia.conf",
                    pch);
          found = try_open_file (rcfile);
          pch = strtok (NULL, ":");
        }
    }

  /* Try /etc/xdg */
  if (found)
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Trying /etc/xdg");
      snprintf (rcfile, PATH_MAX + NAME_MAX - 1,
                "/etc/xdg/tizonia/tizonia.conf");
      found = try_open_file (rcfile);
    }

  /* Finally use the old /etc directory if no other config file was found */
  if (found)
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Trying /etc");
      snprintf (rcfile, PATH_MAX + NAME_MAX - 1, "/etc/tizonia/tizonia.conf");
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Using config location %s", rcfile);
  snprintf (g_rcfiles[0].name, PATH_MAX + NAME_MAX - 1, "%s", rcfile);
}

/* XDG_CONFIG_HOME
   defines the base directory relative to which user specific
   configuration files should be stored. If $XDG_CONFIG_HOME is either not
   set or empty, a default equal to $HOME/.config should be used. */
static void
obtain_xdg_config_home (void)
{
  char * p_env_str = NULL;

  if ((p_env_str = getenv ("XDG_CONFIG_HOME")))
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "XDG_CONFIG_HOME [%s]", p_env_str);
      snprintf (g_rcfiles[1].name, PATH_MAX + NAME_MAX - 1,
                "%s/tizonia/tizonia.conf", p_env_str);
    }
  else
    {
      if ((p_env_str = getenv ("HOME")))
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "HOME [%s]", p_env_str);
          snprintf (g_rcfiles[1].name, PATH_MAX + NAME_MAX - 1,
                    "%s/.config/tizonia/tizonia.conf", p_env_str);
        }
    }
}

static void
obtain_tizonia_rc_file_config (void)
{
  char * p_env_str = NULL;
  if ((p_env_str = getenv ("TIZONIA_RC_FILE")))
    {
      snprintf (g_rcfiles[2].name, PATH_MAX + NAME_MAX - 1, "%s", p_env_str);
    }
}

OMX_ERRORTYPE
tiz_rcfile_init (tiz_rcfile_t ** pp_rc)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  int i;
  tiz_rcfile_t * p_rc = NULL;

  assert (pp_rc);

  /* Retrieve the config file from $XDG_CONFIG_DIRS (g_rcfiles[0]) */
  obtain_xdg_config_dir ();

  /* Retrieve the config file from $XDG_CONFIG_HOME or $HOME (g_rcfiles[1]) */
  obtain_xdg_config_home ();

  /* Retrieve the config file from $TIZONIA_RC_FILE (g_rcfiles[2]) */
  obtain_tizonia_rc_file_config ();

  /* Load rc files */
  TIZ_LOG (TIZ_PRIORITY_TRACE, "Looking for [%d] rc files...", g_num_rcfiles);

  if (!(p_rc = (tiz_rcfile_t *) tiz_mem_calloc (1, sizeof (tiz_rcfile_t))))
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE,
               "Could not allocate memory "
               "for tiz_rcfile_t...");
      return OMX_ErrorInsufficientResources;
    }

  for (i = (g_num_rcfiles - 1); i >= 0; --i)
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Checking for rc file [%d] at [%s]", i,
               g_rcfiles[i].name);

      /* Check file existence and user's read access */
      if (0 != access (g_rcfiles[i].name, R_OK))
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE,
                   "rc file [%s] does not exist or "
                   "user has no read access permission",
                   g_rcfiles[i].name);
          continue;
        }

      /* Store stat's ctime */
      if (stat_ctime (g_rcfiles[i].name, &g_rcfiles[i].ctime) != 0)
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "stat_ctime for [%s] failed",
                   g_rcfiles[i].name);
          continue;
        }

      if (0 != load_rc_file (&g_rcfiles[i], p_rc))
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "Loading [%s] rc file failed",
                   g_rcfiles[i].name);
          continue;
        }

      TIZ_LOG (TIZ_PRIORITY_DEBUG, "Loading [%s] rc file succeeded",
               g_rcfiles[i].name);
      g_rcfiles[i].exists = 1;

      /* We only need to load one file */
      break;
    }

  if (p_rc->count)
    {
      *pp_rc = p_rc;
    }
  else
    {
      *pp_rc = NULL;
      tiz_mem_free (p_rc);
      rc = OMX_ErrorInsufficientResources;
    }

  return rc;
}

const char *
tiz_rcfile_get_value (const char * ap_section, const char * ap_key)
{
  keyval_t * p_kv = NULL;
  tiz_rcfile_t * p_rc = tiz_rcfile_get_handle ();

  if (!p_rc)
    {
      return NULL;
    }

  assert (ap_section);
  assert (ap_key);
  assert (is_list (ap_key) == false);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Retrieving value for Key [%s] in section [%s]",
           ap_key, ap_section);

  p_kv = find_node (p_rc, ap_key);
  if (p_kv && p_kv->p_value_list)
    {
      return shell_expand_value_in_place (p_kv->p_value_list->p_value,
                                          p_kv->p_value_list);
    }

  return NULL;
}

char **
tiz_rcfile_get_value_list (const char * ap_section, const char * ap_key,
                           unsigned long * ap_length)
{
  keyval_t * p_kv = NULL;
  char ** pp_ret = NULL;
  value_t * p_next_value = NULL;
  tiz_rcfile_t * p_rc = tiz_rcfile_get_handle ();

  if (!p_rc)
    {
      return NULL;
    }

  assert (ap_section);
  assert (ap_key);
  assert (ap_length);
  assert (is_list (ap_key) == true);

  TIZ_LOG (TIZ_PRIORITY_TRACE,
           "Retrieving value list "
           "for Key [%s] in section [%s]",
           ap_key, ap_section);

  p_kv = find_node (p_rc, ap_key);
  if (p_kv)
    {
      int i = 0;
      *ap_length = p_kv->valcount;
      pp_ret = (char **) tiz_mem_alloc (sizeof (char *) * p_kv->valcount);

      p_next_value = p_kv->p_value_list;
      for (i = 0; i < p_kv->valcount; ++i)
        {
          if (p_next_value)
            {
              pp_ret[i] = shell_expand_value(p_next_value->p_value);
              p_next_value = p_next_value->p_next;
            }
        }
    }

  return pp_ret;
}

void
tiz_rcfile_destroy (tiz_rcfile_t * p_rc)
{
  keyval_t * p_kv_lst = NULL;
  keyval_t * p_kvt = NULL;
  value_t * p_val_lst = NULL;

  if (!p_rc)
    {
      return;
    }

  p_kv_lst = p_rc->p_keyvals;
  while (p_kv_lst)
    {
      value_t * p_vt = NULL;
      tiz_mem_free (p_kv_lst->p_key);
      p_val_lst = p_kv_lst->p_value_list;
      while (p_val_lst)
        {
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
}

int
tiz_rcfile_compare_value (const char * section, const char * key,
                          const char * value)
{
  int rc = -1;
  const char * p_value = tiz_rcfile_get_value (section, key);
  if (p_value)
    {
      rc = (0 == strncmp (p_value, value, PATH_MAX) ? 0 : 1);
    }
  return rc;
}

int
tiz_rcfile_status (void)
{
  int status = -1;
  tiz_rcfile_t * p_rc = NULL;
  if (OMX_ErrorNone == tiz_rcfile_init (&p_rc))
    {
      status = 0;
      tiz_rcfile_destroy (p_rc);
    }

  TIZ_LOG (TIZ_PRIORITY_DEBUG, "status [%d]", status);
  return status;
}
