/**
 * Copyright (C) 2011-2015 Aratelia Limited - Juan A. Rubio
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
 * @file   tizcore.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL Core
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>
#include <dlfcn.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_Types.h"

#include "tizrmproxy_c.h"
#include "tizplatform.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.ilcore"
#endif

#define TIZ_IL_CORE_THREAD_NAME "omxilcore"
#define TIZ_IL_CORE_RM_NAME "OMX.Aratelia.ilcore"
#define TIZ_DEFAULT_COMP_ENTRY_POINT_NAME "OMX_ComponentInit"
#define TIZ_SHARED_LIB_SONAME_STRING ".so.0.0.0"
#define TIZ_SHARED_LIB_SONAMET_STRING ".so.0.0.0T"

typedef struct role_list_item role_list_item_t;
typedef role_list_item_t *role_list_t;
struct role_list_item
{
  OMX_U8 role[OMX_MAX_STRINGNAME_SIZE];
  role_list_item_t *p_next;
};

typedef enum tizcore_state tizcore_state_t;
enum tizcore_state
{
  ETIZCoreStateStopped = 0,
  ETIZCoreStateStarting,
  ETIZCoreStateStarted
};

typedef enum tizcore_msg_class tizcore_msg_class_t;
enum tizcore_msg_class
{
  ETIZCoreMsgInit = 0,
  ETIZCoreMsgDeinit,
  ETIZCoreMsgGetHandle,
  ETIZCoreMsgFreeHandle,
  ETIZCoreMsgComponentNameEnum,
  ETIZCoreMsgComponentOfRoleEnum,
  ETIZCoreMsgRoleOfComponentEnum,
  ETIZCoreMsgGetCoreInterface,
  ETIZCoreMsgFreeCoreInterface,
  ETIZCoreMsgMax,
};

typedef struct tizcore_msg_str tizcore_msg_str_t;
struct tizcore_msg_str
{
  tizcore_msg_class_t msg;
  const OMX_STRING str;
};

static tizcore_msg_str_t tizcore_msg_to_str_tbl[] = {
  {ETIZCoreMsgInit, (const OMX_STRING) "ETIZCoreMsgInit"},
  {ETIZCoreMsgDeinit, (const OMX_STRING) "ETIZCoreMsgDeinit"},
  {ETIZCoreMsgGetHandle, (const OMX_STRING) "ETIZCoreMsgGetHandle"},
  {ETIZCoreMsgFreeHandle, (const OMX_STRING) "ETIZCoreMsgFreeHandle"},
  {ETIZCoreMsgComponentNameEnum, (const OMX_STRING) "ETIZCoreMsgComponentNameEnum"},
  {ETIZCoreMsgComponentOfRoleEnum, (const OMX_STRING) "ETIZCoreMsgComponentOfRoleEnum"},
  {ETIZCoreMsgRoleOfComponentEnum, (const OMX_STRING) "ETIZCoreMsgRoleOfComponentEnum"},
  {ETIZCoreMsgGetCoreInterface, (const OMX_STRING) "ETIZCoreMsgGetCoreInterface"},
  {ETIZCoreMsgFreeCoreInterface, (const OMX_STRING) "ETIZCoreMsgFreeCoreInterface"},
  {ETIZCoreMsgMax, (const OMX_STRING) "ETIZCoreMsgMax"},
};

static OMX_STRING
tizcore_msg_to_str (const tizcore_msg_class_t a_msg)
{
  const size_t count =
    sizeof (tizcore_msg_to_str_tbl) / sizeof (tizcore_msg_str_t);
  size_t i = 0;

  for (i = 0; i < count; ++i)
    {
      if (tizcore_msg_to_str_tbl[i].msg == a_msg)
        {
          return tizcore_msg_to_str_tbl[i].str;
        }
    }

  return (OMX_STRING) "Unknown core message";
}

typedef struct tizcore_msg_gethandle tizcore_msg_gethandle_t;
struct tizcore_msg_gethandle
{
  OMX_HANDLETYPE *pp_hdl;
  OMX_STRING p_comp_name;
  OMX_PTR p_app_data;
  OMX_CALLBACKTYPE *p_callbacks;
};

typedef struct tizcore_msg_freehandle tizcore_msg_freehandle_t;
struct tizcore_msg_freehandle
{
  OMX_HANDLETYPE p_hdl;
};

typedef struct tizcore_msg_compnameenum tizcore_msg_compnameenum_t;
struct tizcore_msg_compnameenum
{
  OMX_STRING p_comp_name;
  OMX_U32 namelen;
  OMX_U32 index;
};

typedef struct tizcore_msg_compofroleenum tizcore_msg_compofroleenum_t;
struct tizcore_msg_compofroleenum
{
  OMX_STRING p_comp_name;
  OMX_STRING p_role;
  OMX_U32 index;
};

/* Use here the same structure being used for comp of role enum API */
typedef struct tizcore_msg_compofroleenum tizcore_msg_roleofcompenum_t;
typedef struct tizcore_msg tizcore_msg_t;
struct tizcore_msg
{
  tizcore_msg_class_t class;
  union
  {
    tizcore_msg_gethandle_t gh;
    tizcore_msg_freehandle_t fh;
    tizcore_msg_compnameenum_t cne;
    tizcore_msg_compofroleenum_t cre;
    tizcore_msg_roleofcompenum_t rce;
  };
};

/* Forward declarations */
static OMX_ERRORTYPE do_init (tizcore_state_t *, tizcore_msg_t *);
static OMX_ERRORTYPE do_deinit (tizcore_state_t *, tizcore_msg_t *);
static OMX_ERRORTYPE do_cne (tizcore_state_t *, tizcore_msg_t *);
static OMX_ERRORTYPE do_gh (tizcore_state_t *, tizcore_msg_t *);
static OMX_ERRORTYPE do_fh (tizcore_state_t *, tizcore_msg_t *);
static OMX_ERRORTYPE do_cre (tizcore_state_t *, tizcore_msg_t *);
static OMX_ERRORTYPE do_rce (tizcore_state_t *, tizcore_msg_t *);

typedef OMX_ERRORTYPE (*tizcore_msg_dispatch_f) (tizcore_state_t * ap_state,
                                                 tizcore_msg_t * ap_msg);
static const tizcore_msg_dispatch_f tizcore_msg_to_fnt_tbl[] = {
  do_init,
  do_deinit,
  do_gh,
  do_fh,
  do_cne,
  do_cre,
  do_rce,
  NULL,                        /* ETIZCoreMsgGetCoreInterface */
  NULL,                        /* ETIZCoreMsgFreeCoreInterface */
};


typedef struct tizcore_registry_item tizcore_registry_item_t;
typedef tizcore_registry_item_t *tizcore_registry_t;
struct tizcore_registry_item
{
  OMX_STRING p_comp_name;
  OMX_STRING p_dl_name;
  OMX_STRING p_dl_path;
  OMX_PTR p_entry_point;
  OMX_PTR p_dl_hdl;
  OMX_HANDLETYPE p_hdl;
  role_list_t p_roles;
  tizcore_registry_item_t *p_next;
};

typedef struct tizcore tizcore_t;
struct tizcore
{
  void *p_core;
  tiz_thread_t thread;
  tiz_sem_t sem;
  tiz_queue_t *p_queue;
  OMX_ERRORTYPE error;
  tizcore_state_t state;
  tizcore_registry_t p_registry;
  tizrm_t rm;
  tiz_rm_proxy_callbacks_t rmcbacks;
  OMX_UUIDTYPE uuid;
};

static tizcore_t *pg_core = NULL;
static tizcore_t *get_core (void);
static tizcore_registry_item_t *find_comp_in_registry (const OMX_STRING
                                                       ap_name);

static void
wait_complete (OMX_U32 rid, OMX_PTR ap_data)
{
  (void) ap_data;
  TIZ_LOG (TIZ_PRIORITY_TRACE, "wait_complete : rid [%u]", rid);
}

static void
preemption_req (OMX_U32 rid, OMX_PTR ap_data)
{
  (void) ap_data;
  TIZ_LOG (TIZ_PRIORITY_TRACE, "preemption_req : rid [%u]", rid);
}

static void
preemption_complete (OMX_U32 rid, OMX_PTR ap_data)
{
  (void) ap_data;
  TIZ_LOG (TIZ_PRIORITY_TRACE, "preemption_complete : rid [%u]", rid);
}

static void
free_roles (role_list_item_t *ap_role_lst)
{
  role_list_item_t *p_rli = NULL;

  while (NULL != ap_role_lst)
    {
      p_rli = ap_role_lst->p_next;
      tiz_mem_free (ap_role_lst);
      ap_role_lst = p_rli;
    }
}

static OMX_ERRORTYPE
get_component_roles (OMX_COMPONENTTYPE * ap_hdl,
                     role_list_t * app_role_list)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  role_list_item_t *p_role = NULL;
  role_list_item_t *p_first = NULL;
  role_list_item_t *p_last = NULL;
  OMX_U32 role_index = 0;

  assert (NULL != ap_hdl);
  assert (NULL != app_role_list);

  /* Find component roles */
  while (OMX_ErrorNoMore != rc && role_index < 255)
  {
    p_role = (role_list_item_t *)
      tiz_mem_calloc (1, sizeof (role_list_item_t));

    if (NULL == p_role)
      {
        TIZ_LOG (TIZ_PRIORITY_ERROR, "[OMX_ErrorInsufficientResources] : "
                 "Could not allocate role list item. "
                 "Bailing registration");
        rc = OMX_ErrorInsufficientResources;
        break;
      }

    rc = ap_hdl->ComponentRoleEnum ((OMX_HANDLETYPE) ap_hdl,
                                    p_role->role, role_index);

    if (OMX_ErrorNone != rc && OMX_ErrorNoMore != rc)
      {
        TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s] : Call to ComponentRoleEnum failed",
                 tiz_err_to_str (rc));
        tiz_mem_free (p_role);
        break;
      }
    else if (OMX_ErrorNoMore == rc)
      {
        TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] : No more roles",
                 tiz_err_to_str (rc));
        tiz_mem_free (p_role);
        break;
      }

    if (NULL != p_last)
      {
        p_last->p_next = p_role;
      }
    else
      {
        p_first = p_role;
      }

    p_last = p_role;

    TIZ_LOG (TIZ_PRIORITY_TRACE, "Found role [#%d] to be [%s]",
             role_index, p_role->role);

    role_index++;
  }

  if (OMX_ErrorNoMore == rc)
    {
      if (0 == role_index)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "Non-conformant component found. "
                   "No roles retrieved. Skipping component registration...");
          rc = OMX_ErrorUndefined;
        }
      else
        {
          rc = OMX_ErrorNone;
        }
    }
  else if (OMX_ErrorNoMore != rc && OMX_ErrorNone != rc)
    {
      free_roles (p_first);
      p_first = NULL;
      role_index = 0;
    }

  * app_role_list = role_index > 0 ? p_first : NULL;

  return rc;
}

static OMX_ERRORTYPE
add_to_comp_registry (const OMX_STRING ap_dl_path,
                      const OMX_STRING ap_dl_name,
                      OMX_PTR ap_entry_point,
                      OMX_PTR ap_dl_hdl,
                      OMX_COMPONENTTYPE * ap_hdl,
                      tizcore_registry_item_t ** app_reg_item)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_VERSIONTYPE comp_ver, spec_ver;
  OMX_UUIDTYPE comp_uuid;
  tizcore_registry_item_t *p_registry_last = NULL;
  tizcore_registry_item_t *p_registry_new = NULL;
  role_list_t p_role_list = NULL;
  tizcore_t *p_core = get_core ();
  char comp_name[OMX_MAX_STRINGNAME_SIZE];

  TIZ_LOG (TIZ_PRIORITY_TRACE, "dl_name [%s]", ap_dl_name);

  assert (NULL != ap_dl_name);
  assert (NULL != ap_entry_point);
  assert (NULL != p_core);
  assert (NULL != ap_hdl);
  assert (NULL != app_reg_item);

  * app_reg_item = NULL;

  /* Allocate new registry item */
  if (NULL == (p_registry_new = (tizcore_registry_item_t *) tiz_mem_calloc
        (1, sizeof (tizcore_registry_item_t))))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "[OMX_ErrorInsufficientResources] : "
               "Could not allocate memory for registry item.");
      return OMX_ErrorInsufficientResources;
    }

  /* Load the component */
  if (OMX_ErrorNone != (rc =
                        ((OMX_COMPONENTINITTYPE) ap_entry_point)
                        ((OMX_HANDLETYPE) ap_hdl)))
    {
      rc = (rc == OMX_ErrorInsufficientResources
            ? OMX_ErrorInsufficientResources : OMX_ErrorUndefined);
      TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s] : Call to entry point failed",
               tiz_err_to_str (rc));
      tiz_mem_free (p_registry_new);
      return rc;
    }

  /* Get Component info */
  if (OMX_ErrorNone !=
      (rc =
       ap_hdl->
       GetComponentVersion ((OMX_HANDLETYPE) ap_hdl,
                            (OMX_STRING) (&comp_name), &comp_ver,
                            &spec_ver, &comp_uuid)))
    {
      rc = (rc == OMX_ErrorInsufficientResources
            ? OMX_ErrorInsufficientResources : OMX_ErrorUndefined);
      TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s] Call to GetComponentVersion failed",
               tiz_err_to_str (rc));
      tiz_mem_free (p_registry_new);
      (void) ap_hdl->ComponentDeInit ((OMX_HANDLETYPE) ap_hdl);
      return rc;
    }

  /* Check in case the component already exists in the registry... */
  if (NULL != (p_registry_last = find_comp_in_registry (comp_name)))
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "[OMX_ErrorUndefined] : "
               "Component already in registry [%s]", comp_name);
      tiz_mem_free (p_registry_new);
      (void) ap_hdl->ComponentDeInit ((OMX_HANDLETYPE) ap_hdl);
      return OMX_ErrorUndefined;
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "component not in registry [%s]", comp_name);

  /* Get the roles */
  if (OMX_ErrorNone != (rc = get_component_roles (ap_hdl, &p_role_list)))
    {
      rc = (rc == OMX_ErrorInsufficientResources
            ? OMX_ErrorInsufficientResources : OMX_ErrorUndefined);
      TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s] Failed while getting component roles",
               tiz_err_to_str (rc));
      free_roles (p_role_list);
      tiz_mem_free (p_registry_new);
      (void) ap_hdl->ComponentDeInit ((OMX_HANDLETYPE) ap_hdl);
      return rc;
    }

  if (OMX_ErrorNone == rc)
    {

      /* Add to registry */
      if (NULL == (p_core->p_registry))
        {
          /* First entry in the registry */
          TIZ_LOG (TIZ_PRIORITY_TRACE,
                   "Component added (first component) [%s]", comp_name);

          p_core->p_registry = p_registry_new;
        }
      else
        {
          /* Find the last entry in the registry */
          p_registry_last = p_core->p_registry;
          while (true)
            {
              if (p_registry_last->p_next)
                {
                  p_registry_last = p_registry_last->p_next;
                }
              else
                {
                  break;
                }
            }
          p_registry_last->p_next = p_registry_new;
        }

      /* Finish filling the registry entry... */
      p_registry_new->p_comp_name = strndup (comp_name, OMX_MAX_STRINGNAME_SIZE);
      p_registry_new->p_dl_name = strndup (ap_dl_name, NAME_MAX);
      p_registry_new->p_dl_path = strndup (ap_dl_path, PATH_MAX);
      p_registry_new->p_entry_point = ap_entry_point;
      p_registry_new->p_dl_hdl = ap_dl_hdl;
      p_registry_new->p_hdl = ap_hdl;
      p_registry_new->p_roles = p_role_list;

      /* TODO: move this to its own function */
      TIZ_LOG (TIZ_PRIORITY_TRACE,
               "Component [%s] added.", p_registry_new->p_comp_name);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "dl_name [%s].", p_registry_new->p_dl_name);
      TIZ_LOG (TIZ_PRIORITY_TRACE, "dl_path [%s].", p_registry_new->p_dl_path);
      TIZ_LOG (TIZ_PRIORITY_TRACE,
               "dl_entry_point [%p].", p_registry_new->p_entry_point);
      TIZ_LOG (TIZ_PRIORITY_TRACE,
               "p_dl_hdl [%p].", p_registry_new->p_dl_hdl);
      TIZ_LOG (TIZ_PRIORITY_TRACE,
               "p_hdl [%p].", p_registry_new->p_hdl);

      * app_reg_item = p_registry_new;

    }

  ap_hdl->ComponentDeInit ((OMX_HANDLETYPE) ap_hdl);

  return rc;
}

static void
delete_registry (void)
{
  tizcore_t *p_core = get_core ();
  tizcore_registry_item_t *p_registry_last = NULL, *p_registry_next = NULL;
  role_list_item_t *p_roles_last = NULL, *p_roles_next = NULL;

  if (NULL == p_core->p_registry)
    {
      return;
    }

  p_registry_last = p_core->p_registry;
  while (NULL != p_registry_last)
    {
      tiz_mem_free (p_registry_last->p_comp_name);
      tiz_mem_free (p_registry_last->p_dl_name);
      tiz_mem_free (p_registry_last->p_dl_path);

      /* Delete roles */
      p_roles_last = p_registry_last->p_roles;
      while (p_roles_last)
        {
          p_roles_next = p_roles_last->p_next;
          tiz_mem_free (p_roles_last);
          p_roles_last = p_roles_next;
        }

      p_registry_next = p_registry_last->p_next;
      tiz_mem_free (p_registry_last);
      p_registry_last = p_registry_next;
    }

  p_core->p_registry = NULL;
}

static OMX_ERRORTYPE
instantiate_comp_lib (const OMX_STRING ap_path,
                      const OMX_STRING ap_name,
                      const OMX_STRING ap_entry_point_name,
                      OMX_PTR * app_dl_hdl, OMX_PTR * app_entry_point)
{
  char full_name[PATH_MAX];
  OMX_S32 len;
  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", ap_name);

  assert (NULL != ap_path);
  assert (NULL != ap_name);
  assert (NULL != ap_entry_point_name);
  assert (NULL != app_dl_hdl);
  assert (NULL != app_entry_point);

  len = strlen (ap_path);
  memcpy (full_name, ap_path, len + 1);
  if (ap_path[len - 1] != '/')
    {
      full_name[len] = '/';
      len++;
    }
  full_name[len] = 0;

  if (NULL == (* app_dl_hdl = dlopen (strcat (full_name, ap_name), RTLD_LAZY)))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "[OMX_ErrorUndefined] : "
               "Error loading dynamic library [%s]", dlerror ());
      return OMX_ErrorUndefined;
    }

  /* TODO: OMX_ComponentInit is not a mandatory name for the component entry
   * point. Use an IL Core extension to configure additional entry point
   * function names. */
  if (NULL == (* app_entry_point = dlsym (* app_dl_hdl, ap_entry_point_name)))
    {
      TIZ_LOG (TIZ_PRIORITY_DEBUG, "[OMX_ErrorUndefined] : "
               "Default entry point [%s] not found in [%s]",
               ap_entry_point_name, ap_name);
      dlclose (* app_dl_hdl);
      *app_dl_hdl = NULL;
      return OMX_ErrorUndefined;
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
cache_comp_info (const OMX_STRING ap_dl_path, const OMX_STRING ap_dl_name)
{
  OMX_PTR p_dl_hdl = NULL;
  OMX_PTR p_entry_point = NULL;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_COMPONENTTYPE *p_hdl = NULL;
  tizcore_registry_item_t *p_reg_item = NULL;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "dl_name [%s]", ap_dl_name);

  rc = instantiate_comp_lib (ap_dl_path, ap_dl_name,
                             (const OMX_STRING) TIZ_DEFAULT_COMP_ENTRY_POINT_NAME,
                             &p_dl_hdl, &p_entry_point);

  if (OMX_ErrorNone == rc)
    {
      /*  Allocate the component hdl */
      if (!(p_hdl = (OMX_COMPONENTTYPE *) tiz_mem_calloc
            (1, (sizeof (OMX_COMPONENTTYPE)))))
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "[OMX_ErrorInsufficientResources] : "
                   "Could not allocate memory for component handle.");
          rc = OMX_ErrorInsufficientResources;
        }
      else
        {
          if (OMX_ErrorNone ==
              (rc = add_to_comp_registry (ap_dl_path, ap_dl_name,
                                          p_entry_point, NULL,
                                          p_hdl, &p_reg_item)))
            {
              assert (p_reg_item && p_reg_item->p_hdl);
              TIZ_LOG (TIZ_PRIORITY_TRACE, "component [%s] : info cached",
                       p_reg_item->p_comp_name);
              p_reg_item->p_hdl = NULL;
            }

          /* delete the comp hadle */
          /* we are only caching the component info */
          tiz_mem_free (p_hdl);
        }

      dlclose (p_dl_hdl);
    }

  if (OMX_ErrorNoMore == rc)
    {
      rc = OMX_ErrorNone;
    }

  return rc;
}

static char **
find_component_paths(unsigned long * ap_npaths)
{
  char **val_lst = NULL;

  assert(ap_npaths);

  val_lst = tiz_rcfile_get_value_list("il-core", "component-paths", ap_npaths);

  if (!val_lst || 0 == * ap_npaths)
    {
      val_lst = NULL;
      * ap_npaths = 0;
    }

  return val_lst;
}

static void
free_paths (char **pp_paths, unsigned long npaths)
{
  int i = 0;

  assert (NULL != pp_paths);

  for (i = 0; i < (int)npaths; i++)
    {
      tiz_mem_free (pp_paths[i]);
    }

  tiz_mem_free (pp_paths);
}

static OMX_ERRORTYPE
scan_component_folders (void)
{
  DIR *p_dir;
  int i = 0;
  char **pp_paths;
  unsigned long npaths = 0;
  struct dirent *p_dir_entry = NULL;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Looking for component plugins...");

  if (NULL == (pp_paths = find_component_paths(&npaths)))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "No component paths configured");
      return OMX_ErrorInsufficientResources;
    }

  for (i = 0; i < (int)npaths; i++)
    {
      if (NULL == (p_dir = opendir (pp_paths[i])))
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "[OMX_ErrorUndefined] : "
                   "Error opening directory  [%s] - [%s]",
                   pp_paths[i], strerror(errno));
        }
      else
        {
          while (NULL != (p_dir_entry = readdir (p_dir)))
            {
              if (p_dir_entry->d_type == DT_REG
                  && strstr (p_dir_entry->d_name,
                             TIZ_SHARED_LIB_SONAME_STRING)
                  && !strstr (p_dir_entry->d_name,
                              TIZ_SHARED_LIB_SONAMET_STRING))
                {
                  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]",
                           p_dir_entry->d_name);
                  if (OMX_ErrorInsufficientResources
                      == cache_comp_info (pp_paths[i],
                                          p_dir_entry->d_name))
                    {
                      free_paths (pp_paths, npaths);
                      return OMX_ErrorInsufficientResources;
                    }
                }
            }

          (void)closedir (p_dir);
        }

    }

  free_paths (pp_paths, npaths);

  return OMX_ErrorNone;
}

static tizcore_registry_item_t *
find_role_in_registry (const OMX_STRING ap_role_str, OMX_U32 a_index)
{
  tizcore_t *p_core = get_core ();
  tizcore_registry_t p_registry = NULL;
  role_list_item_t *p_role = NULL;
  OMX_U32 num_components_found = 0;

  assert (NULL != p_core);
  assert (NULL != ap_role_str);

  p_registry = p_core->p_registry;

  while (p_registry && num_components_found < a_index + 1)
    {
      p_role = p_registry->p_roles;
      while (p_role)
        {
          if (0 == strncmp ((OMX_STRING)p_role->role, ap_role_str,
                            OMX_MAX_STRINGNAME_SIZE))
            {
              num_components_found++;
              TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] found - comp [%s] "
                       "num comps [%d].", ap_role_str, p_registry->p_comp_name,
                       num_components_found);
              break;
            }
          p_role = p_role->p_next;
        }

      if (num_components_found < a_index + 1)
        {
          p_registry = p_registry->p_next;
        }
    }

  if (num_components_found < a_index + 1)
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Could not find [%s] index [%d].",
               ap_role_str, a_index);
      /* Make sure we return null in this case */
      p_registry = NULL;
    }

  return p_registry;
}

static tizcore_registry_item_t *
find_comp_in_registry (const OMX_STRING ap_name)
{
  tizcore_t *p_core = get_core ();
  tizcore_registry_t p_registry = NULL;

  assert (NULL != p_core);
  assert (NULL != ap_name);

  p_registry = p_core->p_registry;

  while (NULL != p_registry)
    {
      if (0 == strncmp (p_registry->p_comp_name, ap_name,
                        OMX_MAX_STRINGNAME_SIZE))
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] found.", ap_name);
          return p_registry;
        }
      p_registry = p_registry->p_next;
    }


  TIZ_LOG (TIZ_PRIORITY_TRACE, "Could not find [%s].", ap_name);

  return p_registry;
}

static tizcore_registry_item_t *
find_hdl_in_registry (OMX_HANDLETYPE ap_hdl)
{

  tizcore_t *p_core = get_core ();
  tizcore_registry_t p_registry = NULL;

  assert (NULL != p_core);
  assert (NULL != ap_hdl);

  p_registry = p_core->p_registry;

  while (NULL != p_registry)
    {
      if (p_registry->p_hdl == ap_hdl)
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] found.", p_registry->p_comp_name);
          break;
        }
      p_registry = p_registry->p_next;
    }

  if (NULL == p_registry)
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "Could not find hdl [%p].", ap_hdl);
    }

  return p_registry;
}

static inline OMX_ERRORTYPE
instantiate_component (tizcore_msg_gethandle_t * ap_msg)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_PTR p_dl_hdl = NULL;
  OMX_PTR p_entry_point = NULL;
  OMX_COMPONENTTYPE *p_hdl = NULL;
  tizcore_registry_item_t *p_reg_item = NULL;

  assert (NULL != ap_msg);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Instantiate [%s]", ap_msg->p_comp_name);

  /* TODO: From this point on, Fix error handling!!!!... */

  if (NULL != (p_reg_item = find_comp_in_registry (ap_msg->p_comp_name)))
    {
      if (OMX_ErrorNone
          == (rc = instantiate_comp_lib (p_reg_item->p_dl_path,
                                         p_reg_item->p_dl_name,
                                         (const OMX_STRING) TIZ_DEFAULT_COMP_ENTRY_POINT_NAME,
                                         &p_dl_hdl, &p_entry_point)))
        {
          /* TODO: refactor these two blocks into a function. They are also used */
          /* in add_to_comp_registry */

          /*  Allocate the component hdl */
          if (NULL == (p_hdl = (OMX_COMPONENTTYPE *) tiz_mem_calloc
                       (1, (sizeof (OMX_COMPONENTTYPE)))))
            {
              TIZ_LOG (TIZ_PRIORITY_ERROR, "{OMX_ErrorInsufficientResources] : "
                       "Could not allocate memory for component handle");
              return OMX_ErrorInsufficientResources;
            }

          /* Load the component */
          if (OMX_ErrorNone != (rc =
                                ((OMX_COMPONENTINITTYPE) p_entry_point)
                                ((OMX_HANDLETYPE) p_hdl)))
            {
              TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s] : Call to component's entry point "
                       "failed", tiz_err_to_str (rc));
              return rc;
            }

          TIZ_LOG (TIZ_PRIORITY_TRACE, "Success - component hdl [%p]",
                   p_hdl);

          if (OMX_ErrorNone
              != (rc = p_hdl->SetCallbacks ((OMX_HANDLETYPE) p_hdl,
                                            ap_msg->p_callbacks,
                                            ap_msg->p_app_data)))
            {
              TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s] : Call to SetCallbacks failed",
                       tiz_err_to_str (rc));
              return rc;
            }

          *(ap_msg->pp_hdl) = p_hdl;
          p_reg_item->p_hdl = p_hdl;
          p_reg_item->p_dl_hdl = p_dl_hdl;
        }
    }
  else
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "[OMX_ErrorComponentNotFound] : "
               "Component [%s] not found.", ap_msg->p_comp_name);

      rc = OMX_ErrorComponentNotFound;
    }

  return rc;
}

static OMX_ERRORTYPE
remove_comp_instance (tizcore_msg_freehandle_t * ap_msg)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_COMPONENTTYPE *p_hdl = NULL;
  tizcore_registry_item_t *p_reg_item = NULL;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Removing component instance...");

  if (NULL != (p_reg_item = find_hdl_in_registry (ap_msg->p_hdl)))
    {

      p_hdl = (OMX_COMPONENTTYPE *) p_reg_item->p_hdl;
      assert (p_hdl);

      /* Unload the component */
      if (OMX_ErrorNone != (rc =
                            p_hdl->ComponentDeInit
                            ((OMX_HANDLETYPE) p_hdl)))
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR, "Call to ComponentDeinit point failed");
        }
      else
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "Success - [%s] deleted ",
                   p_reg_item->p_comp_name);
        }

      /*  Deallocate the component hdl */
      tiz_mem_free (p_hdl);
      p_reg_item->p_hdl = NULL;
      dlclose (p_reg_item->p_dl_hdl);
      p_reg_item->p_dl_hdl = NULL;

    }
  else
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "Could not find component handle in registry");
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
do_init (tizcore_state_t * ap_state, tizcore_msg_t * ap_msg)
{
  tizrm_error_t rc = TIZRM_SUCCESS;
  tizcore_t *p_core = get_core ();
  OMX_PRIORITYMGMTTYPE primgmt;
  (void) ap_msg;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "ETIZCoreMsgInit received...");
  assert (NULL != p_core);
  assert (NULL != ap_state
          && (ETIZCoreStateStarting == * ap_state
              || ETIZCoreStateStarted == * ap_state));
  assert (ETIZCoreMsgInit == ap_msg->class);

  if (ETIZCoreStateStarted == * ap_state)
    {
      return OMX_ErrorNone;
    }

  /* Init here the RM hdl */
  p_core->rmcbacks.pf_waitend = &wait_complete;
  p_core->rmcbacks.pf_preempt = &preemption_req;
  p_core->rmcbacks.pf_preempt_end = &preemption_complete;

  bzero (&p_core->uuid, 128);

  if (TIZRM_SUCCESS !=
      (rc =
       tiz_rm_proxy_init (&p_core->rm, (const OMX_STRING) TIZ_IL_CORE_RM_NAME,
                         (const OMX_UUIDTYPE *) &p_core->uuid, &primgmt,
                         &p_core->rmcbacks, NULL)))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR,
               "RM proxy initialization failed. RM error [%d]...", rc);
      return OMX_ErrorInsufficientResources;
    }

  (void) tiz_thread_setname (&(p_core->thread), (const OMX_STRING) TIZ_IL_CORE_THREAD_NAME);

  * ap_state = ETIZCoreStateStarted;
  return scan_component_folders ();
}

static OMX_ERRORTYPE
do_deinit (tizcore_state_t * ap_state, tizcore_msg_t * ap_msg)
{
  tizrm_error_t rc = TIZRM_SUCCESS;
  tizcore_t *p_core = get_core ();

  TIZ_LOG (TIZ_PRIORITY_TRACE, "ETIZCoreMsgDeinit received...");

  assert (NULL != p_core);
  assert (NULL != ap_msg);
  assert (ETIZCoreMsgDeinit == ap_msg->class);

  * ap_state = ETIZCoreStateStopped;

  /* Deinit the RM hdl */
  if (TIZRM_SUCCESS != (rc = tiz_rm_proxy_destroy (&p_core->rm)))
    {
      /* TODO: Translate into a proper error code, especially OOM error  */
      TIZ_LOG (TIZ_PRIORITY_ERROR, "[OMX_ErrorUndefined] : "
               "RM proxy deinitialization failed...");
      return OMX_ErrorUndefined;
    }

  delete_registry ();
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
do_gh (tizcore_state_t * ap_state, tizcore_msg_t * ap_msg)
{
  tizcore_msg_gethandle_t *p_msg_gh = NULL;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "ETIZCoreMsgGetHandle received...");
  assert (NULL != ap_msg);
  assert (NULL != ap_state);
  assert (ETIZCoreStateStarted == *ap_state);
  assert (ETIZCoreMsgGetHandle == ap_msg->class);

  p_msg_gh = &(ap_msg->gh);
  assert (NULL != p_msg_gh);

  return instantiate_component (p_msg_gh);
}

static OMX_ERRORTYPE
do_fh (tizcore_state_t * ap_state, tizcore_msg_t * ap_msg)
{
  tizcore_msg_freehandle_t *p_msg_fh = NULL;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "ETIZCoreMsgFreeHandle received...");
  assert (NULL != ap_msg);
  assert (NULL != ap_state);
  assert (ETIZCoreStateStarted == *ap_state);
  assert (ETIZCoreMsgFreeHandle == ap_msg->class);

  p_msg_fh = &(ap_msg->fh);
  assert (NULL != p_msg_fh);

  return remove_comp_instance (p_msg_fh);
}

static OMX_ERRORTYPE
do_cne (tizcore_state_t * ap_state, tizcore_msg_t * ap_msg)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tizcore_msg_compnameenum_t *p_msg_cne = NULL;
  tizcore_t *p_core = get_core ();
  tizcore_registry_item_t *p_reg_item = NULL;
  OMX_BOOL found = OMX_FALSE;
  OMX_U32 i = 0;

  assert (NULL != ap_msg);
  assert (NULL != ap_state);
  assert (ETIZCoreStateStarted == *ap_state);
  assert (ETIZCoreMsgComponentNameEnum == ap_msg->class);

  p_msg_cne = &(ap_msg->cne);
  assert (NULL != p_msg_cne);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "ETIZCoreMsgComponentNameEnum received : "
           "Index [%d]...", p_msg_cne->index);

  if (0 == p_msg_cne->index)
    {
      if (OMX_ErrorNone != (rc = scan_component_folders ()))
        {
          /* INFO: This IL Core function is not supposed to return */
          /* InsufficientResources, then mask it as undefined (ugly but */
          /* conformant) */
          return OMX_ErrorInsufficientResources != rc
            ? rc : OMX_ErrorUndefined;
        }
    }

  rc = OMX_ErrorNoMore;
  if (NULL != p_core->p_registry)
    {
      p_reg_item = p_core->p_registry;
      for (i = 0; i <= p_msg_cne->index && p_reg_item; ++i)
        {
          if (p_msg_cne->index == i && NULL != p_reg_item)
            {
              found = OMX_TRUE;
              break;
            }
          p_reg_item = p_reg_item->p_next;
        }
    }

  if (OMX_TRUE == found)
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] found at index [%d]",
               p_reg_item->p_comp_name, p_msg_cne->index);
      strncpy (p_msg_cne->p_comp_name, p_reg_item->p_comp_name,
               p_msg_cne->namelen);
      if (p_msg_cne->namelen > 0)
        {
          p_msg_cne->p_comp_name[p_msg_cne->namelen - 1] = '\0';
        }
      rc = OMX_ErrorNone;
    }

  return rc;
}

static OMX_ERRORTYPE
do_cre (tizcore_state_t * ap_state, tizcore_msg_t * ap_msg)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tizcore_msg_roleofcompenum_t *p_msg_cre = NULL;
  tizcore_registry_item_t *p_reg_item = NULL;
  OMX_BOOL found = OMX_FALSE;
  OMX_U32 i = 0;

  assert (NULL != ap_msg);
  assert (NULL != ap_state);
  assert (ETIZCoreStateStarted == *ap_state);
  assert (ETIZCoreMsgComponentOfRoleEnum == ap_msg->class);

  p_msg_cre = &(ap_msg->cre);
  assert (NULL != p_msg_cre);

  assert (NULL != p_msg_cre->p_comp_name);
  assert (NULL != p_msg_cre->p_role);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "ETIZCoreMsgComponentOfRoleEnum received : "
           "Role [%s] Index [%d]...", p_msg_cre->p_role,
           p_msg_cre->index);


  /* Do the obvious linear search for now */
  i = p_msg_cre->index;
  while (NULL != (p_reg_item = find_role_in_registry (p_msg_cre->p_role,
                                                      i)))
    {
      if (p_msg_cre->index == i)
        {
          assert (NULL != p_reg_item->p_comp_name);
          strncpy (p_msg_cre->p_comp_name,
                   (const char*) p_reg_item->p_comp_name,
                   OMX_MAX_STRINGNAME_SIZE);
          /* Make sure the resulting string is null-terminated */
          p_msg_cre->p_comp_name[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
          found = OMX_TRUE;
          break;
        }
      i++;
    }

  if (OMX_TRUE == found)
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]: Found role [%s] at index [%d]",
               p_reg_item->p_comp_name, p_msg_cre->p_role, p_msg_cre->index);
    }
  else
    {
      rc = OMX_ErrorNoMore;
    }

  return rc;
}

static OMX_ERRORTYPE
do_rce (tizcore_state_t * ap_state, tizcore_msg_t * ap_msg)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tizcore_msg_roleofcompenum_t *p_msg_rce = NULL;
  tizcore_registry_item_t *p_reg_item = NULL;
  role_list_item_t *p_role_item = NULL;
  OMX_BOOL found = OMX_FALSE;
  OMX_U32 i = 0;

  assert (NULL != ap_msg);
  assert (NULL != ap_state);
  assert (ETIZCoreStateStarted == *ap_state);
  assert (ETIZCoreMsgRoleOfComponentEnum == ap_msg->class);

  p_msg_rce = &(ap_msg->rce);
  assert (NULL != p_msg_rce);

  assert (NULL != p_msg_rce->p_comp_name);
  assert (NULL != p_msg_rce->p_role);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "ETIZCoreMsgRoleOfComponentEnum received : "
           "Index [%d]...", p_msg_rce->index);

  if (NULL != (p_reg_item = find_comp_in_registry (p_msg_rce->p_comp_name)))
    {
      p_role_item = p_reg_item->p_roles;
      for (i=0; i < p_msg_rce->index
             && NULL != p_role_item; ++i)
        {
          p_role_item = p_role_item->p_next;
        }

      if (NULL != p_role_item)
        {
          found = true;
          strncpy (p_msg_rce->p_role, (const char*) p_role_item->role,
                   OMX_MAX_STRINGNAME_SIZE);
          /* Make sure the resulting string is null-terminated */
          p_msg_rce->p_role[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
        }
      else
        {
          rc = OMX_ErrorNoMore;
        }
    }
  else
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "[OMX_ErrorComponentNotFound] : [%s]",
               p_msg_rce->p_comp_name);
      rc = OMX_ErrorComponentNotFound;
    }

  if (OMX_TRUE == found)
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]: Found role [%s] at index [%d]",
               p_reg_item->p_comp_name, p_msg_rce->p_role, p_msg_rce->index);
    }

  return rc;
}

static OMX_S32
dispatch_msg (tizcore_state_t * ap_state, tizcore_msg_t * ap_msg)
{
  OMX_S32 signal_client = 0;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tizcore_t *p_core = get_core ();

  assert (NULL != ap_msg);
  assert (NULL != ap_state);
  assert (NULL != p_core);

  assert (ap_msg->class < ETIZCoreMsgMax);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "msg [%p] class [%s]",
           ap_msg, tizcore_msg_to_str (ap_msg->class));

  signal_client = 1;

  rc = tizcore_msg_to_fnt_tbl[ap_msg->class] (ap_state, ap_msg);

  /* Return error to client */
  p_core->error = rc;

  tiz_mem_free (ap_msg);

  return signal_client;
}

static void *
il_core_thread_func (void *p_arg)
{
  tizcore_t *p_core = (tizcore_t *) (p_arg);
  OMX_PTR p_data = NULL;
  OMX_S32 signal_client = 0;

  assert (NULL != p_core);

  tiz_check_omx_err_ret_null (tiz_sem_post (&(p_core->sem)));

  for (;;)
    {
      tiz_check_omx_err_ret_null (tiz_queue_receive (p_core->p_queue, &p_data));
      signal_client = dispatch_msg
        (&(p_core->state), (tizcore_msg_t *) p_data);

      if (signal_client > 0)
        {
          tiz_check_omx_err_ret_null (tiz_sem_post (&(p_core->sem)));
        }

      if (ETIZCoreStateStopped == p_core->state)
        {
          break;
        }
    }

  return NULL;
}

static tizcore_t *
get_core (void)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  /* TODO: Fix error handling!! */

  if (!pg_core)
    {
      pg_core = (tizcore_t *) tiz_mem_alloc (sizeof (tizcore_t));
      if (!pg_core)
        {
          return NULL;
        }
      TIZ_LOG (TIZ_PRIORITY_TRACE,
               "Initializing core instance [%p]...", pg_core);

      pg_core->p_core = NULL;

      if (OMX_ErrorNone != (rc = tiz_sem_init (&(pg_core->sem), 0)))
        {
          TIZ_LOG (TIZ_PRIORITY_TRACE, "Error Initializing core instance...");
          return NULL;
        }

      if (OMX_ErrorNone
          != (rc = tiz_queue_init (&(pg_core->p_queue), 10)))
        {
          return NULL;
        }

      pg_core->error = OMX_ErrorNone;
      pg_core->state = ETIZCoreStateStarting;
      pg_core->p_registry = NULL;

      TIZ_LOG (TIZ_PRIORITY_TRACE, "IL Core initialization success...");
    }

  return (rc == OMX_ErrorNone) ? pg_core : NULL;
}

static OMX_ERRORTYPE
start_core (void)
{
  tizcore_t *p_core = get_core ();

  TIZ_LOG (TIZ_PRIORITY_TRACE,
           "Starting IL core thread with cache in [%p]...", p_core);

  assert (NULL != p_core);

  /* Create IL Core thread */
  tiz_thread_create (&(p_core->thread), 0, 0, il_core_thread_func, p_core);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "waiting on thread creation...");
  tiz_sem_wait (&(p_core->sem));
  TIZ_LOG (TIZ_PRIORITY_TRACE, "thread creation complete...");

  return OMX_ErrorNone;
}

static tizcore_msg_t *
init_core_message (tizcore_msg_class_t a_msg_class)
{
  tizcore_msg_t *p_msg = NULL;

  assert (a_msg_class < ETIZCoreMsgMax);

  if (NULL == (p_msg = (tizcore_msg_t *)
               tiz_mem_calloc (1, sizeof (tizcore_msg_t))))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "[OMX_ErrorInsufficientResources] : "
               "Creating message [%s]", tizcore_msg_to_str (a_msg_class));
      return NULL;
    }

  p_msg->class = a_msg_class;
  return p_msg;
}

static OMX_ERRORTYPE
send_msg_blocking (tizcore_msg_t * ap_msg)
{
  tizcore_t *p_core = get_core ();
  assert (NULL != ap_msg);
  assert (NULL != p_core);

  tiz_check_omx_err (tiz_queue_send (p_core->p_queue, ap_msg));
  tiz_check_omx_err (tiz_sem_wait (&(p_core->sem)));
  TIZ_LOG (TIZ_PRIORITY_TRACE, "OMX IL CORE RESULT [%s]",
           tiz_err_to_str (p_core->error));

  return p_core->error;
}

/* TODO: Review this function in the context of 1.2 */
static OMX_ERRORTYPE
gp_to_st_err (OMX_ERRORTYPE a_getparam_err)
{
  switch (a_getparam_err)
    {
      /* Errors allowed in both OMX_GetParameter and OMX_SetupTunnel */
    case OMX_ErrorBadParameter:
    case OMX_ErrorVersionMismatch:
    case OMX_ErrorTimeout:
    case OMX_ErrorBadPortIndex:
      return a_getparam_err;

      /* Errors allowed in OMX_GetParameter but not in OMX_SetupTunnel */
    case OMX_ErrorUndefined:
    case OMX_ErrorNoMore:
    case OMX_ErrorNotReady:
    case OMX_ErrorUnsupportedIndex:
    case OMX_ErrorSeperateTablesUsed:
    default:
      /* Return something allowed... */
      return OMX_ErrorBadParameter;
    };
}

static  OMX_ERRORTYPE
do_tunnel_requests(OMX_HANDLETYPE ap_outhdl, OMX_U32 a_outport,
                   OMX_HANDLETYPE ap_inhdl, OMX_U32 a_inport)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_COMPONENTTYPE *p_incmp = (OMX_COMPONENTTYPE *) ap_inhdl,
    *p_outcmp = (OMX_COMPONENTTYPE *) ap_outhdl;
  OMX_TUNNELSETUPTYPE tsetup = { 0, OMX_BufferSupplyUnspecified };
  OMX_PARAM_PORTDEFINITIONTYPE port_def;

  TIZ_INIT_OMX_PORT_STRUCT (port_def, 0);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "ap_outhdl [%p] a_outport [%d] "
           "ap_inhdl [%p] a_inport [%d]", ap_outhdl, a_outport,
           ap_inhdl, a_inport);

  if (p_outcmp)
    {
      /* TODO: Do this in debug mode only */
      /* Verify the output port */
      port_def.nPortIndex = a_outport;
      if (OMX_ErrorNone
          != (rc = p_outcmp->GetParameter (p_outcmp,
                                           OMX_IndexParamPortDefinition,
                                           &port_def)))
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR,
                   "%s : GetParameter on output port failed",
                   tiz_err_to_str (gp_to_st_err (rc)));
          return gp_to_st_err (rc);
        }

      if (OMX_DirOutput != port_def.eDir)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR,
                   "OMX_ErrorBadParameter : Output port not an output (%s)?",
                   tiz_dir_to_str (port_def.eDir));
          return OMX_ErrorBadParameter;
        }

      rc = p_outcmp->ComponentTunnelRequest (p_outcmp,
                                             a_outport,
                                             p_incmp, a_inport, &tsetup);
      TIZ_LOG (TIZ_PRIORITY_TRACE,
               "ComponentTunnelRequest (output)  returned [%s]",
               tiz_err_to_str (rc));
    }

  if (OMX_ErrorNone == rc && NULL != p_incmp)
    {
      /* Verify the input port */
      /* Init the struct values just in case */
      /* they were overwritten in the previous call... */
      port_def.nSize = (OMX_U32) sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
      port_def.nVersion.nVersion = (OMX_U32) OMX_VERSION;
      port_def.nPortIndex = a_inport;
      if (OMX_ErrorNone
          != (rc = p_incmp->GetParameter (p_incmp,
                                          OMX_IndexParamPortDefinition,
                                          &port_def)))
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR,
                   "%s : GetParameter on input port failed",
                   tiz_err_to_str (gp_to_st_err (rc)));
          return gp_to_st_err (rc);
        }

      if (OMX_DirInput != port_def.eDir)
        {
          TIZ_LOG (TIZ_PRIORITY_ERROR,
                   "OMX_ErrorBadParameter : Input port not an input (%s)?",
                   tiz_dir_to_str (port_def.eDir));
          return OMX_ErrorBadParameter;
        }

      if (OMX_ErrorNone
          != (rc = p_incmp->ComponentTunnelRequest (p_incmp,
                                                    a_inport,
                                                    p_outcmp,
                                                    a_outport, &tsetup)))
        {
          /* Undo the tunnel request on the component with the output port */
          if (p_outcmp)
            {
              /* Ignore additional errors at this point... */
              (void) p_outcmp->ComponentTunnelRequest (p_outcmp,
                                                       a_outport,
                                                       NULL, 0, &tsetup);
            }
        }

      TIZ_LOG (TIZ_PRIORITY_TRACE,
               "ComponentTunnelRequest (input)  returned [%s]",
               tiz_err_to_str (rc));
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE,
           "do_tunnel_requests [%s]", tiz_err_to_str (rc));

  return rc;
}


OMX_ERRORTYPE
OMX_Init (void)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tizcore_msg_t *p_msg = NULL;

  if (NULL != getenv("TIZONIA_CORE_STARTS_LOG"))
    {
      tiz_log_init ();
    }

  if (OMX_ErrorNone != (rc = start_core ()))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR,
               "[%s] : Error starting core", tiz_err_to_str (rc));
      return rc;
    }

  if (NULL == (p_msg = init_core_message (ETIZCoreMsgInit)))
    {
      return OMX_ErrorInsufficientResources;
    }

  return send_msg_blocking (p_msg);
}

OMX_ERRORTYPE
OMX_Deinit (void)
{
  tizcore_msg_t *p_msg = NULL;
  OMX_PTR p_result = NULL;
  tizcore_t *p_core = get_core ();

  assert (NULL != p_core);

  if (NULL == (p_msg = init_core_message (ETIZCoreMsgDeinit)))
    {
      /* TODO: Consider pre-allocating this message */
      return OMX_ErrorInsufficientResources;
    }

  tiz_check_omx_err (tiz_queue_send (p_core->p_queue, p_msg));
  tiz_check_omx_err (tiz_sem_wait (&(p_core->sem)));
  tiz_thread_join (&(p_core->thread), &p_result);

  tiz_queue_destroy (p_core->p_queue);
  p_core->p_queue = NULL;
  (void)tiz_sem_destroy (&(p_core->sem));
  tiz_mem_free (pg_core);
  pg_core = NULL;

  (void) tiz_log_deinit ();

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
OMX_ComponentNameEnum (OMX_STRING ap_cname, OMX_U32 a_namelen,
                       OMX_U32 a_index)
{
  tizcore_msg_t *p_msg = NULL;
  tizcore_msg_compnameenum_t *p_msg_cne = NULL;

  /* INFO: BUG in 1.1.2 CTS: This comparison */
  /* if (OMX_MAX_STRINGNAME_SIZE > strlen(ap_cname)) { */
  /* return OMX_ErrorBadParameter} */
  /* would cause an error in the ComponentNameTest test of the 1.1.2 cts */
  /* Possibly other tests too */
  if (0 == a_namelen || NULL == ap_cname)
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "[OMX_ErrorBadParameter] : "
               "(name len %d - comp name %p)", a_namelen, ap_cname);
      return OMX_ErrorBadParameter;
    }

  if (NULL == (p_msg = init_core_message (ETIZCoreMsgComponentNameEnum)))
    {
      return OMX_ErrorInsufficientResources;
    }

  /* Finish-up this message */
  p_msg_cne = &(p_msg->cne);
  assert (NULL != p_msg_cne);

  p_msg_cne->p_comp_name = ap_cname;
  p_msg_cne->namelen     = a_namelen;
  p_msg_cne->index       = a_index;

  return send_msg_blocking (p_msg);
}

OMX_ERRORTYPE
OMX_GetHandle (OMX_HANDLETYPE * app_hdl, OMX_STRING ap_comp_name,
               OMX_PTR ap_app_data, OMX_CALLBACKTYPE * ap_callbacks)
{
  tizcore_msg_t *p_msg = NULL;
  tizcore_msg_gethandle_t *p_msg_gh = NULL;

  if (NULL == app_hdl || NULL == ap_comp_name || NULL == ap_callbacks
      || (strlen (ap_comp_name) > OMX_MAX_STRINGNAME_SIZE))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "[OMX_ErrorBadParameter]: NULL argument found.");
      return OMX_ErrorBadParameter;
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s]", ap_comp_name);

  if (NULL == (p_msg = init_core_message (ETIZCoreMsgGetHandle)))
    {
      return OMX_ErrorInsufficientResources;
    }

  /* Finish-up this message */
  p_msg_gh = &(p_msg->gh);
  assert (NULL != p_msg_gh);

  p_msg_gh->pp_hdl      = app_hdl;
  p_msg_gh->p_comp_name = ap_comp_name;
  p_msg_gh->p_app_data  = ap_app_data;
  p_msg_gh->p_callbacks = ap_callbacks;

  return send_msg_blocking (p_msg);
}

OMX_ERRORTYPE
OMX_FreeHandle (OMX_HANDLETYPE ap_hdl)
{
  tizcore_msg_t *p_msg = NULL;
  tizcore_msg_freehandle_t *p_msg_fh = NULL;

  assert (NULL != ap_hdl);

  if (NULL == (p_msg = init_core_message (ETIZCoreMsgFreeHandle)))
    {
      return OMX_ErrorInsufficientResources;
    }

  /* Finish-up this message */
  p_msg_fh = &(p_msg->fh);
  assert (NULL != p_msg_fh);

  p_msg_fh->p_hdl = ap_hdl;

  return send_msg_blocking (p_msg);
}

OMX_ERRORTYPE
OMX_SetupTunnel (OMX_HANDLETYPE ap_outhdl, OMX_U32 a_outport,
                 OMX_HANDLETYPE ap_inhdl, OMX_U32 a_inport)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "ap_outhdl [%p] a_outport [%d] "
           "ap_inhdl [%p] a_inport [%d]", ap_outhdl, a_outport,
           ap_inhdl, a_inport);

  if (NULL == ap_outhdl || NULL == ap_inhdl)
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR,
               "[OMX_ErrorBadParameter] : NULL hdl received (out %p - in %p)",
               ap_outhdl, ap_inhdl);
      return OMX_ErrorBadParameter;
    }

  return do_tunnel_requests(ap_outhdl, a_outport, ap_inhdl, a_inport);
}

OMX_ERRORTYPE
OMX_TeardownTunnel(OMX_HANDLETYPE ap_outhdl, OMX_U32 a_outport,
                   OMX_HANDLETYPE ap_inhdl, OMX_U32 a_inport)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_HANDLETYPE p_null_inhdl = NULL;
  OMX_HANDLETYPE p_null_outhdl = NULL;

  TIZ_LOG (TIZ_PRIORITY_TRACE, "ap_outhdl [%p] a_outport [%d] "
           "ap_inhdl [%p] a_inport [%d]", ap_outhdl, a_outport,
           ap_inhdl, a_inport);

  if (NULL == ap_outhdl || NULL == ap_inhdl)
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR,
               "[OMX_ErrorBadParameter] : NULL handle received (out %p - in %p)",
               ap_outhdl, ap_inhdl);
      return OMX_ErrorBadParameter;
    }

  rc = do_tunnel_requests(ap_outhdl, a_outport, p_null_inhdl, a_inport);

  if (OMX_ErrorNone == rc)
    {
      rc = do_tunnel_requests(p_null_outhdl, a_outport, ap_inhdl, a_inport);
    }

  return rc;
}

OMX_ERRORTYPE
OMX_ComponentOfRoleEnum(OMX_STRING ap_comp_name, OMX_STRING ap_role,
                        OMX_U32 a_index)
{
  tizcore_msg_t *p_msg = NULL;
  tizcore_msg_compofroleenum_t *p_msg_cre = NULL;

  if (NULL == ap_comp_name || NULL == ap_role)
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "[OMX_ErrorBadParameter] : NULL argument"
               "(comp name %p - role %p)", ap_comp_name, ap_role);
      return OMX_ErrorBadParameter;
    }

  if (NULL == (p_msg = init_core_message (ETIZCoreMsgComponentOfRoleEnum)))
    {
      return OMX_ErrorInsufficientResources;
    }

  /* Finish-up this message */
  p_msg_cre = &(p_msg->cre);
  assert (NULL != p_msg_cre);

  p_msg_cre->p_comp_name = ap_comp_name;
  p_msg_cre->p_role      = ap_role;
  p_msg_cre->index       = a_index;

  return send_msg_blocking (p_msg);
}

OMX_ERRORTYPE
OMX_RoleOfComponentEnum(OMX_STRING ap_role, OMX_STRING ap_comp_name,
                        OMX_U32 a_index)
{
  tizcore_msg_t *p_msg = NULL;
  tizcore_msg_roleofcompenum_t *p_msg_rce = NULL;

  if (NULL == ap_comp_name || NULL == ap_role)
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "[OMX_ErrorBadParameter] : NULL argument"
               "(comp name %p - role %p)", ap_comp_name, ap_role);
      return OMX_ErrorBadParameter;
    }

  if (NULL == (p_msg = init_core_message (ETIZCoreMsgRoleOfComponentEnum)))
    {
      return OMX_ErrorInsufficientResources;
    }

  /* Finish-up this message */
  p_msg_rce = &(p_msg->rce);
  assert (NULL != p_msg_rce);

  p_msg_rce->p_comp_name = ap_comp_name;
  p_msg_rce->p_role      = ap_role;
  p_msg_rce->index       = a_index;

  return send_msg_blocking (p_msg);
}

OMX_ERRORTYPE
OMX_GetCoreInterface(void ** ppItf, OMX_STRING cExtensionName)
{
  (void) ppItf;
  (void) cExtensionName;
  return OMX_ErrorNotImplemented;
}

void
OMX_FreeCoreInterface(void * pItf)
{
  (void) pItf;
}
