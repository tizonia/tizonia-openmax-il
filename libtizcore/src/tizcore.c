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

#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_Types.h"

#include "tizcore.h"
#include "tizrmproxy_c.h"
#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.ilcore"
#endif

#define TIZ_DEFAULT_COMP_ENTRY_POINT_NAME "OMX_ComponentInit"
#define TIZ_SHARED_LIB_SONAME_STRING ".so.0.0.0"
#define TIZ_SHARED_LIB_SONAMET_STRING ".so.0.0.0T"

static OMX_VERSIONTYPE tc_spec_version = {
  {
    OMX_VERSION_MAJOR,
    OMX_VERSION_MINOR,
    OMX_VERSION_REVISION,
    OMX_VERSION_STEP
  }
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
  tizrm_proxy_callbacks_t rmcbacks;
  OMX_UUIDTYPE uuid;
};

static tizcore_t *pg_core = NULL;

static inline tizcore_t *get_core ();

static tizcore_registry_item_t *find_comp_in_registry (const OMX_STRING
                                                       ap_name);

static void
wait_complete (OMX_U32 rid, OMX_PTR ap_data)
{
  TIZ_LOG (TIZ_LOG_TRACE, "wait_complete : rid [%u]", rid);
}

static void
preemption_req (OMX_U32 rid, OMX_PTR ap_data)
{
  TIZ_LOG (TIZ_LOG_TRACE, "preemption_req : rid [%u]", rid);
}

static void
preemption_complete (OMX_U32 rid, OMX_PTR ap_data)
{
  TIZ_LOG (TIZ_LOG_TRACE, "preemption_complete : rid [%u]", rid);
}

static OMX_ERRORTYPE
get_component_roles (OMX_COMPONENTTYPE * ap_hdl,
                     role_list_t * app_role_list)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  role_list_item_t *p_role = NULL, *p_first = NULL, *p_last = NULL;
  OMX_U32 role_index = 0;

  TIZ_LOG (TIZ_LOG_TRACE, "hdl [%p]", app_role_list);

  assert (ap_hdl);
  assert (app_role_list);

  /* Find component roles */
  do
    {
      p_role = (role_list_item_t *)
        tiz_mem_calloc (1, sizeof (role_list_item_t));

      if (!p_role)
        {
          TIZ_LOG (TIZ_LOG_TRACE,
                     "Error allocating role list item. "
                     "Bailing registration");
          rc = OMX_ErrorInsufficientResources;
          break;
        }

      rc =
        ap_hdl->ComponentRoleEnum ((OMX_HANDLETYPE) ap_hdl,
                                   p_role->role, role_index);

      if (OMX_ErrorNone != rc && OMX_ErrorNoMore != rc)
        {
          TIZ_LOG (TIZ_LOG_TRACE, "Call to ComponentRoleEnum failed");
          tiz_mem_free (p_role);
          p_role = NULL;
        }

      if (OMX_ErrorNone == rc && p_role)
        {
          if (p_last)
            {
              p_last->p_next = p_role;
              p_last = p_role;
            }
          else
            {
              p_last = p_role;
              p_first = p_role;
            }
          TIZ_LOG (TIZ_LOG_TRACE,
                     "Found role [#%d] to be [%s]", role_index, p_role->role);

          role_index++;
        }

    }
  while (OMX_ErrorNoMore != rc && OMX_ErrorNone == rc
         && role_index < 255);

  if (OMX_ErrorNoMore == rc && 0 == role_index)
    {
      TIZ_LOG (TIZ_LOG_TRACE,
                 "Non-conformant component found. "
                 "No roles found. Skipping component registration...");
      tiz_mem_free (p_role);
      p_role = NULL;
    }

  if (OMX_ErrorNoMore == rc && role_index > 0)
    {
      rc = OMX_ErrorNone;
    }

  * app_role_list = role_index ? p_first : NULL;

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
  tizcore_registry_item_t *p_registry_last = NULL, *p_registry_new = NULL;
  role_list_t p_role_list = NULL;
  tizcore_t *p_core = get_core ();
  char comp_name[OMX_MAX_STRINGNAME_SIZE];

  TIZ_LOG (TIZ_LOG_TRACE, "dl_name [%s]", ap_dl_name);

  assert (ap_dl_name);
  assert (ap_entry_point);
  assert (p_core);
  assert (ap_hdl);
  assert (app_reg_item);

  * app_reg_item = NULL;

  /* Allocate new registry item */
  if (!(p_registry_new = (tizcore_registry_item_t *) tiz_mem_calloc
        (1, sizeof (tizcore_registry_item_t))))
    {
      TIZ_LOG (TIZ_LOG_ERROR, "mem_alloc failed");
      return OMX_ErrorInsufficientResources;
    }

  /* Load the component */
  if (OMX_ErrorNone != (rc =
                        ((OMX_COMPONENTINITTYPE) ap_entry_point)
                        ((OMX_HANDLETYPE) ap_hdl)))
    {
      TIZ_LOG (TIZ_LOG_TRACE, "Call to entry point failed");
      tiz_mem_free (p_registry_new);
      return OMX_ErrorUndefined;
    }

  /* Get Component info */
  if (OMX_ErrorNone == rc)
    {
      if (OMX_ErrorNone !=
          (rc =
           ap_hdl->
           GetComponentVersion ((OMX_HANDLETYPE) ap_hdl,
                                (OMX_STRING) (&comp_name), &comp_ver,
                                &spec_ver, &comp_uuid)))
        {
          TIZ_LOG (TIZ_LOG_TRACE, "Call to GetComponentVersion failed");
          tiz_mem_free (p_registry_new);
          ap_hdl->ComponentDeInit ((OMX_HANDLETYPE) ap_hdl);
          return rc == OMX_ErrorInsufficientResources
            ? OMX_ErrorInsufficientResources : OMX_ErrorUndefined;
        }
    }

  /* Check in case the component already exists in the registry... */
  if ((p_registry_last = find_comp_in_registry (comp_name)))
    {
      TIZ_LOG (TIZ_LOG_TRACE, "Component already in registry [%s]",
                 comp_name);
      tiz_mem_free (p_registry_new);
      ap_hdl->ComponentDeInit ((OMX_HANDLETYPE) ap_hdl);
      return OMX_ErrorNoMore;
    }

  TIZ_LOG (TIZ_LOG_TRACE, "component not in registry [%s]", comp_name);

  /* Get the roles */
  if (OMX_ErrorNone == rc)
    {
      /* TODO: Check this error code and free role list in case of error.... */
      rc = get_component_roles (ap_hdl, &p_role_list);
    }

  if (OMX_ErrorNone == rc)
    {

      /* Add to registry */
      if (!(p_core->p_registry))
        {
          /* First entry in the registry */
          TIZ_LOG (TIZ_LOG_TRACE,
                     "Component added (first component) [%s]", comp_name);

          p_core->p_registry = p_registry_new;
        }
      else
        {
          /* Find the last entry in the registry */
          p_registry_last = p_core->p_registry;
          while (1)
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
      p_registry_new->p_comp_name = strdup (comp_name);
      p_registry_new->p_dl_name = strdup (ap_dl_name);
      p_registry_new->p_dl_path = strdup (ap_dl_path);
      p_registry_new->p_entry_point = ap_entry_point;
      p_registry_new->p_dl_hdl = ap_dl_hdl;
      p_registry_new->p_hdl = ap_hdl;
      p_registry_new->p_roles = p_role_list;

      /* TODO: move this to its own function */
      TIZ_LOG (TIZ_LOG_TRACE,
                 "Component [%s] added.", p_registry_new->p_comp_name);
      TIZ_LOG (TIZ_LOG_TRACE, "dl_name [%s].", p_registry_new->p_dl_name);
      TIZ_LOG (TIZ_LOG_TRACE, "dl_path [%s].", p_registry_new->p_dl_path);
      TIZ_LOG (TIZ_LOG_TRACE,
                 "dl_entry_point [%p].", p_registry_new->p_entry_point);
      TIZ_LOG (TIZ_LOG_TRACE,
                 "p_dl_hdl [%p].", p_registry_new->p_dl_hdl);
      TIZ_LOG (TIZ_LOG_TRACE,
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

  if (!p_core->p_registry)
    {
      return;
    }

  p_registry_last = p_core->p_registry;
  while (p_registry_last)
    {
      tiz_mem_free (p_registry_last->p_comp_name);
      tiz_mem_free (p_registry_last->p_dl_name);
      tiz_mem_free (p_registry_last->p_dl_path);

      /* Delete roles list */
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
  TIZ_LOG (TIZ_LOG_TRACE, "[%s]", ap_name);

  assert (ap_path);
  assert (ap_name);
  assert (ap_entry_point_name);
  assert (app_dl_hdl);
  assert (app_entry_point);

  len = strlen (ap_path);
  memcpy (full_name, ap_path, len + 1);
  if (ap_path[len - 1] != '/')
    {
      full_name[len] = '/';
      len++;
    }

  full_name[len] = 0;

  if (!(* app_dl_hdl = dlopen (strcat (full_name, ap_name), RTLD_LAZY)))
    {
      TIZ_LOG (TIZ_LOG_TRACE, "error [%s]", dlerror ());
      return OMX_ErrorUndefined;
    }

  /* TODO: OMX_ComponentInit is not a mandatory name for the component entry
   * point. Use an IL Core extension to configure additional entry point
   * function names. */
  if (!(* app_entry_point = dlsym (* app_dl_hdl, ap_entry_point_name)))
    {
      TIZ_LOG (TIZ_LOG_TRACE,
                 "Default entry point [%s] not found in [%s]",
                 ap_entry_point_name, ap_name);
      dlclose (* app_dl_hdl);
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

  TIZ_LOG (TIZ_LOG_TRACE, "dl_name [%s]", ap_dl_name);

  rc = instantiate_comp_lib (ap_dl_path, ap_dl_name,
                                  TIZ_DEFAULT_COMP_ENTRY_POINT_NAME,
                                  &p_dl_hdl, &p_entry_point);

  if (OMX_ErrorNone == rc)
    {

      /*  Allocate the component hdl */
      if (!(p_hdl =
            (OMX_COMPONENTTYPE *) tiz_mem_alloc
            (sizeof (OMX_COMPONENTTYPE))))
        {
          TIZ_LOG (TIZ_LOG_ERROR, "mem_alloc failed");
          dlclose (p_dl_hdl);
          return OMX_ErrorInsufficientResources;
        }

      if (OMX_ErrorNone ==
          (rc = add_to_comp_registry (ap_dl_path, ap_dl_name,
                                           p_entry_point, NULL,
                                           p_hdl, &p_reg_item)))
        {
          assert (p_reg_item && p_reg_item->p_hdl);
          TIZ_LOG (TIZ_LOG_TRACE, "component [%s] : info cached",
                     p_reg_item->p_comp_name);
          p_reg_item->p_hdl = NULL;
        }

      /* delete the IL hdl, */
      /* we are only caching the component. */
      tiz_mem_free (p_hdl);

      dlclose (p_dl_hdl);

    }

  if (OMX_ErrorNoMore == rc)
    {
      rc = OMX_ErrorNone;
    }

  return rc;

}

/* Note being used for now */
/* static int */
/* stat_ctime (const char *path, time_t * time) */
/* { */
/*   struct stat astat; */
/*   int statret = stat (path, &astat); */
/*   if (0 != statret) */
/*     { */
/*       return statret; */
/*     } */
/*   *time = astat.st_ctime; */
/*   return statret; */
/* } */

static char **
find_component_paths(unsigned long * ap_npaths)
{
  tiz_rcfile_t *p_rcfile = NULL;
  char **val_lst = NULL;

  assert(ap_npaths);

  tiz_rcfile_open(&p_rcfile);
  val_lst = tiz_rcfile_get_value_list(p_rcfile, "il-core",
                                      "component-paths", ap_npaths);
  tiz_rcfile_close(p_rcfile);

  if (!val_lst || 0 == * ap_npaths)
    {
      val_lst = NULL;
      * ap_npaths = 0;
    }

  return val_lst;
}

static OMX_ERRORTYPE
scan_component_folders (void)
{
  DIR *p_dir;
  int i = 0;
  char **pp_paths;
  unsigned long npaths = 0;
  struct dirent *p_dir_entry = NULL;

  TIZ_LOG (TIZ_LOG_TRACE, "Looking for component plugins...");

  if (NULL == (pp_paths = find_component_paths(&npaths)))
    {
      TIZ_LOG (TIZ_LOG_ERROR, "No component paths configured");
      return OMX_ErrorInsufficientResources;
    }

  for (i = 0; i < npaths; i++)
    {
      if (NULL == (p_dir = opendir (pp_paths[i])))
        {
          TIZ_LOG (TIZ_LOG_TRACE, "Can't open directory [%s]",
                     pp_paths[i]);
          return OMX_ErrorUndefined;
        }

      while ((p_dir_entry = readdir (p_dir)) != NULL)
        {
          if (p_dir_entry->d_type == DT_REG
              && strstr (p_dir_entry->d_name,
                         TIZ_SHARED_LIB_SONAME_STRING)
              && !strstr (p_dir_entry->d_name,
                          TIZ_SHARED_LIB_SONAMET_STRING))
            {
              TIZ_LOG (TIZ_LOG_TRACE, "[%s]",
                         p_dir_entry->d_name);
              cache_comp_info (pp_paths[i],
                               p_dir_entry->d_name);
            }
        }

      closedir (p_dir);

    }

  return OMX_ErrorNone;
}

static tizcore_registry_item_t *
find_comp_in_registry (const OMX_STRING ap_name)
{
  tizcore_t *p_core = get_core ();
  tizcore_registry_t p_registry = NULL;

  assert (p_core);
  assert (ap_name);

  p_registry = p_core->p_registry;

  while (p_registry)
    {
      if (0 == strcmp (p_registry->p_comp_name, ap_name))
        {
          TIZ_LOG (TIZ_LOG_TRACE, "[%s] found.", ap_name);
          break;
        }
      p_registry = p_registry->p_next;
    }

  if (!p_registry)
    {
      TIZ_LOG (TIZ_LOG_TRACE, "Could not find [%s].", ap_name);
    }

  return p_registry;
}

static tizcore_registry_item_t *
find_hdl_in_registry (OMX_HANDLETYPE ap_hdl)
{

  tizcore_t *p_core = get_core ();
  tizcore_registry_t p_registry = NULL;

  assert (p_core);
  assert (ap_hdl);

  p_registry = p_core->p_registry;

  while (p_registry)
    {
      if (p_registry->p_hdl == ap_hdl)
        {
          TIZ_LOG (TIZ_LOG_TRACE, "[%s] found.", p_registry->p_comp_name);
          break;
        }
      p_registry = p_registry->p_next;
    }

  if (!p_registry)
    {
      TIZ_LOG (TIZ_LOG_TRACE, "Could not find hdl [%p].", ap_hdl);
    }

  return p_registry;
}

static OMX_ERRORTYPE
instantiate_component (tizcore_msg_gethandle_t * ap_msg)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_PTR p_dl_hdl = NULL;
  OMX_PTR p_entry_point = NULL;
  OMX_COMPONENTTYPE *p_hdl = NULL;
  tizcore_registry_item_t *p_reg_item = NULL;

  assert (ap_msg);

  TIZ_LOG (TIZ_LOG_TRACE, "Instantiate [%s]", ap_msg->p_comp_name);

  /* TODO: From this point on, Fix error handling!!!!... */

  if (NULL != (p_reg_item = find_comp_in_registry (ap_msg->p_comp_name)))
    {
      rc = instantiate_comp_lib (p_reg_item->p_dl_path,
                                      p_reg_item->p_dl_name,
                                      TIZ_DEFAULT_COMP_ENTRY_POINT_NAME,
                                      &p_dl_hdl, &p_entry_point);

      /* TODO: refactor these two blocks into a function. They are also used */
      /* in add_to_comp_registry */

      /*  Allocate the component hdl */
      if (!(p_hdl = (OMX_COMPONENTTYPE *) tiz_mem_alloc
            (sizeof (OMX_COMPONENTTYPE))))
        {
          TIZ_LOG (TIZ_LOG_ERROR, "Couldn't allocate component hdl.");
          return OMX_ErrorInsufficientResources;
        }

      /* Load the component */
      if (OMX_ErrorNone != (rc =
                            ((OMX_COMPONENTINITTYPE) p_entry_point)
                            ((OMX_HANDLETYPE) p_hdl)))
        {
          TIZ_LOG (TIZ_LOG_TRACE, "Call to component entry point failed");
          return rc;
        }

      TIZ_LOG (TIZ_LOG_TRACE, "Success - component hdl [%p]",
                 p_hdl);

      p_hdl->SetCallbacks ((OMX_HANDLETYPE) p_hdl,
                                   ap_msg->p_callbacks, ap_msg->p_app_data);

      *(ap_msg->pp_hdl) = p_hdl;
      p_reg_item->p_hdl = p_hdl;

    }
  else
    {
      TIZ_LOG (TIZ_LOG_TRACE, "Component [%s] not found.",
                 ap_msg->p_comp_name);

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

  TIZ_LOG (TIZ_LOG_TRACE, "Removing component instance...");

  // TODO: Fix error handling...!!!

  if ((p_reg_item = find_hdl_in_registry (ap_msg->p_hdl)))
    {

      p_hdl = (OMX_COMPONENTTYPE *) p_reg_item->p_hdl;
      assert (p_hdl);

      /* Unload the component */
      if (OMX_ErrorNone != (rc =
                            p_hdl->ComponentDeInit
                            ((OMX_HANDLETYPE) p_hdl)))
        {
          TIZ_LOG (TIZ_LOG_TRACE, "Call to ComponentDeinit point failed");
          return rc;
        }


      TIZ_LOG (TIZ_LOG_TRACE, "Success - [%s] deleted ",
                 p_reg_item->p_comp_name);

      /*  Deallocate the component hdl */
      tiz_mem_free (p_hdl);
      p_reg_item->p_hdl = NULL;

    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
do_init (tizcore_state_t * ap_state)
{
  tizrm_error_t rc = TIZRM_SUCCESS;
  tizcore_t *p_core = get_core ();
  OMX_PRIORITYMGMTTYPE primgmt;

  TIZ_LOG (TIZ_LOG_TRACE, "ETIZCoreMsgInit received...");
  assert (p_core);
  assert (ap_state
          && (ETIZCoreStateStarting == * ap_state
              || ETIZCoreStateStarted == * ap_state));

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
       tizrm_proxy_init (&p_core->rm, "OMX.Aratelia.ilcore",
                         (const OMX_UUIDTYPE *) &p_core->uuid, &primgmt,
                         &p_core->rmcbacks, NULL)))
    {
      TIZ_LOG (TIZ_LOG_ERROR,
                 "RM proxy initialization failed. RM error [%d]...", rc);
      return OMX_ErrorInsufficientResources;
    }

  * ap_state = ETIZCoreStateStarted;
  return scan_component_folders ();
}

static OMX_ERRORTYPE
do_deinit (tizcore_state_t * ap_state)
{
  tizrm_error_t rc = TIZRM_SUCCESS;
  tizcore_t *p_core = get_core ();

  TIZ_LOG (TIZ_LOG_TRACE, "ETIZCoreMsgDeinit received...");

  assert (p_core);

  * ap_state = ETIZCoreStateStopped;

  /* Deinit the RM hdl */
  if (TIZRM_SUCCESS != (rc = tizrm_proxy_destroy (&p_core->rm)))
    {
      /* TODO: Translate into a proper error code, especially OOM error  */
      TIZ_LOG (TIZ_LOG_TRACE, "RM proxy deinitialization failed...");
      return OMX_ErrorUndefined;
    }

  delete_registry ();
  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
do_compenum (tizcore_msg_compnameenum_t * ap_msg)
{

  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tizcore_t *p_core = get_core ();
  tizcore_registry_item_t *p_reg_item = NULL;
  OMX_BOOL found = OMX_FALSE;
  OMX_U32 i;

  assert (ap_msg);

  TIZ_LOG (TIZ_LOG_TRACE, "ETIZCoreMsgComponentNameEnum received : "
             "Index [%d]...", ap_msg->index);

  if (0 == ap_msg->index)
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

  /*  */
  rc = OMX_ErrorNoMore;
  if (p_core->p_registry)
    {
      p_reg_item = p_core->p_registry;
      for (i = 0; i <= ap_msg->index && p_reg_item; ++i)
        {
          if (ap_msg->index == i && p_reg_item)
            {
              found = OMX_TRUE;
              break;
            }
          p_reg_item = p_reg_item->p_next;
        }
    }

  if (found)
    {
      TIZ_LOG (TIZ_LOG_TRACE, "[%s] found at index [%d]",
                 p_reg_item->p_comp_name, ap_msg->index);
      strncpy (ap_msg->p_comp_name, p_reg_item->p_comp_name,
               ap_msg->namelen);
      if (ap_msg->namelen)
        {
          ap_msg->p_comp_name[ap_msg->namelen - 1] = '\0';
        }
      rc = OMX_ErrorNone;
    }

  return rc;
}

static OMX_ERRORTYPE
do_get_hdl (tizcore_msg_gethandle_t * ap_msg)
{

  TIZ_LOG (TIZ_LOG_TRACE, "ETIZCoreMsgGetHandle received...");
  assert (ap_msg);
  return instantiate_component (ap_msg);
}

static OMX_ERRORTYPE
do_free_hdl (tizcore_msg_freehandle_t * ap_msg)
{
  TIZ_LOG (TIZ_LOG_TRACE, "ETIZCoreMsgFreeHandle received...");
  return remove_comp_instance (ap_msg);
}

static OMX_ERRORTYPE
do_compofrole (tizcore_msg_compofroleenum_t * ap_msg)
{
  /* TODO */
  return OMX_ErrorNotImplemented;
}

static OMX_ERRORTYPE
do_roleofcomp (tizcore_msg_roleofcompenum_t * ap_msg)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tizcore_registry_item_t *p_reg_item = NULL;
  role_list_item_t *p_role_item = NULL;
  OMX_BOOL found = OMX_FALSE;
  OMX_U32 i;

  assert (ap_msg);
  assert (ap_msg->p_comp_name);
  assert (ap_msg->p_role);

  TIZ_LOG (TIZ_LOG_TRACE, "ETIZCoreMsgRoleOfComponentEnum received : "
             "Index [%d]...", ap_msg->index);

  if (NULL != (p_reg_item = find_comp_in_registry (ap_msg->p_comp_name)))
    {
      p_role_item = p_reg_item->p_roles;
      for (i=0; i < ap_msg->index
             && NULL != p_role_item; ++i)
        {
          p_role_item = p_role_item->p_next;
        }

      if (p_role_item)
        {
          found = true;
          strncpy (ap_msg->p_role, (const char*) p_role_item->role, OMX_MAX_STRINGNAME_SIZE);
          /* Make sure the resulting string is always null-terminated */
          ap_msg->p_role[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
        }
      else
        {
          rc = OMX_ErrorNoMore;
        }
    }
  else
    {
      TIZ_LOG (TIZ_LOG_ERROR, "[%s]: OMX_ErrorComponentNotFound",
                 p_reg_item->p_comp_name);
      rc = OMX_ErrorComponentNotFound;
    }

  if (found)
    {
      TIZ_LOG (TIZ_LOG_TRACE, "[%s]: Found role [%s] at index [%d]",
                 p_reg_item->p_comp_name, ap_msg->p_role, ap_msg->index);
    }

  return rc;
}

static OMX_S32
dispatch_msg (tizcore_msg_t * ap_msg, tizcore_state_t * ap_state)
{
  OMX_S32 signal_client = 0;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tizcore_t *p_core = get_core ();

  assert (ap_msg);
  assert (ap_state);
  assert (p_core);

  if (ap_msg)
    {
      TIZ_LOG (TIZ_LOG_TRACE, "msg [%p] class [%d]",
                 ap_msg, ap_msg->class);
      switch (ap_msg->class)
        {
        case ETIZCoreMsgInit:
          {
            rc = do_init (ap_state);
            signal_client = 1;
            break;
          }

        case ETIZCoreMsgDeinit:
          {
            rc = do_deinit (ap_state);
            signal_client = 1;
            break;
          }

        case ETIZCoreMsgComponentNameEnum:
          {
            rc =
              do_compenum ((tizcore_msg_compnameenum_t *) ap_msg->p_data);
            signal_client = 1;
            break;
          }

        case ETIZCoreMsgGetHandle:
          {
            rc =
              do_get_hdl ((tizcore_msg_gethandle_t *) ap_msg->p_data);
            signal_client = 1;
            break;
          }

        case ETIZCoreMsgFreeHandle:
          {
            rc = do_free_hdl
              ((tizcore_msg_freehandle_t *) ap_msg->p_data);
            signal_client = 1;
            break;
          }

        case ETIZCoreMsgComponentOfRoleEnum:
          {
            rc =
              do_compofrole ((tizcore_msg_compofroleenum_t *) ap_msg->p_data);
            signal_client = 1;
            break;
          }

        case ETIZCoreMsgRoleOfComponentEnum:
          {
            rc =
              do_roleofcomp ((tizcore_msg_roleofcompenum_t *) ap_msg->p_data);
            signal_client = 1;
            break;
          }

        default:
          break;
        };
    }
  else
    {
      TIZ_LOG (TIZ_LOG_TRACE, "NULL msg");
      rc = OMX_ErrorUndefined;
      assert (0);
    }

  /* Return error to client */
  p_core->error = rc;

  tiz_mem_free (ap_msg->p_data);
  tiz_mem_free (ap_msg);

  return signal_client;
}

static void *
il_core_thread_func (void *p_arg)
{
  tizcore_t *p_core = (tizcore_t *) (p_arg);
  OMX_PTR p_data = NULL;
  OMX_S32 signal_client = 0;

  TIZ_LOG (TIZ_LOG_TRACE, "p_core [%p]", p_core);

  assert (p_core);

  tiz_sem_post (&(p_core->sem));
  TIZ_LOG (TIZ_LOG_TRACE, "signalled sem");

  for (;;)
    {
      TIZ_LOG (TIZ_LOG_TRACE, "waiting for msgs");
      /* TODO: Check ret val */
      tiz_queue_receive (p_core->p_queue, &p_data);
      signal_client = dispatch_msg
        ((tizcore_msg_t *) p_data, &(p_core->state));

      if (signal_client)
        {
          tiz_sem_post (&(p_core->sem));
        }

      if (ETIZCoreStateStopped == p_core->state)
        {
          break;
        }
    }

  TIZ_LOG (TIZ_LOG_TRACE, "exiting...");

  return NULL;
}

static inline tizcore_t *
get_core ()
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
      TIZ_LOG (TIZ_LOG_TRACE,
                 "Initializing core instance [%p]...", pg_core);

      pg_core->p_core = NULL;

      if (OMX_ErrorNone != (rc = tiz_sem_init (&(pg_core->sem), 0)))
        {
          TIZ_LOG (TIZ_LOG_TRACE, "Error Initializing core instance...");
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

      TIZ_LOG (TIZ_LOG_TRACE, "IL Core initialization success...");
    }

  return (rc == OMX_ErrorNone) ? pg_core : NULL;
}

static OMX_ERRORTYPE
start_core ()
{
  tizcore_t *p_core = get_core ();

  TIZ_LOG (TIZ_LOG_TRACE,
             "Starting IL core thread with cache in [%p]...", p_core);

  assert (p_core);

  /* Create IL Core thread */
  tiz_thread_create (&(p_core->thread), 0, 0, il_core_thread_func, p_core);

  TIZ_LOG (TIZ_LOG_TRACE, "waiting on thread creation...");
  tiz_sem_wait (&(p_core->sem));
  TIZ_LOG (TIZ_LOG_TRACE, "thread creation complete...");

  return OMX_ErrorNone;
}

static tizcore_msg_t *
init_core_message (tizcore_msg_class_t a_msg_class)
{
  tizcore_msg_t *p_msg = NULL;
  tizcore_msg_data_t p_data = NULL;

  if (!(p_msg =
       (tizcore_msg_t *) tiz_mem_calloc (1, sizeof (tizcore_msg_t))))
    {
      TIZ_LOG (TIZ_LOG_ERROR, "mem alloc failed");
      return NULL;
    }

  switch (a_msg_class)
    {
    case ETIZCoreMsgInit:
    case ETIZCoreMsgDeinit:
      {
        /* No data included in these messages */
        break;
      }

    case ETIZCoreMsgGetHandle:
      {
        p_data = (tizcore_msg_gethandle_t *)
          tiz_mem_calloc (1, sizeof (tizcore_msg_gethandle_t));
        break;
      }

    case ETIZCoreMsgFreeHandle:
      {
        p_data = (tizcore_msg_freehandle_t *)
          tiz_mem_calloc (1, sizeof (tizcore_msg_freehandle_t));
        break;
      }

    case ETIZCoreMsgComponentNameEnum:
      {
        p_data = (tizcore_msg_compnameenum_t *)
          tiz_mem_calloc (1, sizeof (tizcore_msg_compnameenum_t));
        break;
      }

    case ETIZCoreMsgComponentOfRoleEnum:
      {
        // TODO
        break;
      }

    case ETIZCoreMsgRoleOfComponentEnum:
      {
        p_data = (tizcore_msg_roleofcompenum_t *)
          tiz_mem_calloc (1, sizeof (tizcore_msg_roleofcompenum_t));
        break;
      }

    case ETIZCoreMsgGetCoreInterface:
      {
        // TODO
        break;
      }

    case ETIZCoreMsgFreeCoreInterface:
      {
        // TODO
        break;
      }

    default:
      TIZ_LOG (TIZ_LOG_TRACE, "Unknown msg class [%d]", a_msg_class);
      assert (0);
      break;
    };

  if ((ETIZCoreMsgInit != a_msg_class
       && ETIZCoreMsgDeinit != a_msg_class) && !p_data)
    {
      TIZ_LOG (TIZ_LOG_TRACE, "mem alloc failed");
      tiz_mem_free (p_msg);
      p_msg = NULL;
    }
  else
    {
      p_msg->class = a_msg_class;
      p_msg->p_data = p_data;
    }

  return p_msg;

}

static inline OMX_ERRORTYPE
send_msg_blocking (tizcore_msg_t * ap_msg)
{
  tizcore_t *p_core = get_core ();
  assert (ap_msg);
  assert (p_core);

  tiz_queue_send (p_core->p_queue, ap_msg);
  TIZ_LOG (TIZ_LOG_TRACE, "message sent [%p]", ap_msg);
  tiz_sem_wait (&(p_core->sem));
  TIZ_LOG (TIZ_LOG_TRACE, "OMX IL CORE RESULT [%s]",
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
  OMX_PARAM_PORTDEFINITIONTYPE port_def = {
    sizeof (OMX_PARAM_PORTDEFINITIONTYPE),
    tc_spec_version
  };

  TIZ_LOG (TIZ_LOG_TRACE, "ap_outhdl [%p] a_outport [%d] "
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
          TIZ_LOG (TIZ_LOG_TRACE,
                     "%s : GetParameter on output port failed",
                     tiz_err_to_str (gp_to_st_err (rc)));
          return gp_to_st_err (rc);
        }

      if (OMX_DirOutput != port_def.eDir)
        {
          TIZ_LOG (TIZ_LOG_ERROR,
                     "OMX_ErrorBadParameter : Output port not an output (%s)?",
                     tiz_dir_to_str (port_def.eDir));
          return OMX_ErrorBadParameter;
        }

      rc = p_outcmp->ComponentTunnelRequest (p_outcmp,
                                             a_outport,
                                             p_incmp, a_inport, &tsetup);
      TIZ_LOG (TIZ_LOG_TRACE,
                 "ComponentTunnelRequest (output)  returned [%s]",
                 tiz_err_to_str (rc));
    }

  if (OMX_ErrorNone == rc && p_incmp)
    {
      /* Verify the input port */
      /* Init the struct values just in case */
      /* they were overwritten in the previous call... */
      port_def.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
      port_def.nVersion.nVersion = OMX_VERSION;
      port_def.nPortIndex = a_inport;
      if (OMX_ErrorNone
          != (rc = p_incmp->GetParameter (p_incmp,
                                               OMX_IndexParamPortDefinition,
                                               &port_def)))
        {
          TIZ_LOG (TIZ_LOG_TRACE,
                     "%s : GetParameter on input port failed",
                     tiz_err_to_str (gp_to_st_err (rc)));
          return gp_to_st_err (rc);
        }

      if (OMX_DirInput != port_def.eDir)
        {
          TIZ_LOG (TIZ_LOG_ERROR,
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

      TIZ_LOG (TIZ_LOG_TRACE,
                 "ComponentTunnelRequest (input)  returned [%s]",
                 tiz_err_to_str (rc));
    }

  TIZ_LOG (TIZ_LOG_TRACE,
             "do_tunnel_requests [%s]", tiz_err_to_str (rc));

  return rc;
}


OMX_ERRORTYPE
OMX_Init (void)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  tizcore_msg_t *p_msg = NULL;

  tiz_log_init ();

  if (OMX_ErrorNone != (rc = start_core ()))
    {
      return rc;
    }

  if (!(p_msg = init_core_message (ETIZCoreMsgInit)))
    {
      TIZ_LOG (TIZ_LOG_ERROR,
                 "Error creating Init message [%p]", p_msg);
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

  TIZ_LOG (TIZ_LOG_TRACE, "OMX_Deinit");

  assert (p_core);

  if (!(p_msg = init_core_message (ETIZCoreMsgDeinit)))
    {
      /* TODO: Consider pre-allocating this message */
      TIZ_LOG (TIZ_LOG_ERROR, "Error creating DeInit message");
      return OMX_ErrorInsufficientResources;
    }

  tiz_queue_send (p_core->p_queue, p_msg);
  TIZ_LOG (TIZ_LOG_TRACE, "message sent [%p]", p_msg);

  tiz_sem_wait (&(p_core->sem));

  tiz_thread_join (&(p_core->thread), &p_result);

  tiz_queue_destroy (p_core->p_queue);
  p_core->p_queue = NULL;
  tiz_sem_destroy (&(p_core->sem));

  tiz_mem_free (pg_core);
  pg_core = NULL;

  tiz_log_deinit ();

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
OMX_ComponentNameEnum (OMX_STRING ap_cname, OMX_U32 a_namelen,
                       OMX_U32 a_index)
{
  tizcore_msg_t *p_msg = NULL;
  tizcore_msg_compnameenum_t *p_msg_cnameenum = NULL;

  /* INFO: BUG in 1.1.2 CTS: This comparison */
  /* if (OMX_MAX_STRINGNAME_SIZE > strlen(ap_cname)) { */
  /* return OMX_ErrorBadParameter} */
  /* would cause an error in the ComponentNameTest test of the 1.1.2 cts */
  /* Possibly other tests too */
  if (!a_namelen || !ap_cname)
    {
      TIZ_LOG (TIZ_LOG_ERROR, "OMX_ErrorBadParameter");
      return OMX_ErrorBadParameter;
    }

  if (!(p_msg = init_core_message (ETIZCoreMsgComponentNameEnum)))
    {
      TIZ_LOG (TIZ_LOG_ERROR, "Error creating ComponentNameEnum message");
      return OMX_ErrorInsufficientResources;
    }

  /* Finish-up this message */
  p_msg_cnameenum = (tizcore_msg_compnameenum_t *) p_msg->p_data;
  assert (p_msg_cnameenum);
  p_msg_cnameenum->p_comp_name = ap_cname;
  p_msg_cnameenum->namelen = a_namelen;
  p_msg_cnameenum->index = a_index;

  return send_msg_blocking (p_msg);
}

OMX_ERRORTYPE
OMX_GetHandle (OMX_HANDLETYPE * app_hdl, OMX_STRING ap_comp_name,
               OMX_PTR ap_app_data, OMX_CALLBACKTYPE * ap_callbacks)
{
  tizcore_msg_t *p_msg = NULL;
  tizcore_msg_gethandle_t *p_gethdl = NULL;

  if (!app_hdl || !ap_comp_name || !ap_callbacks
      || strlen (ap_comp_name) > OMX_MAX_STRINGNAME_SIZE)
    {
      TIZ_LOG (TIZ_LOG_ERROR, "OMX_ErrorBadParameter.");
      return OMX_ErrorBadParameter;
    }

  TIZ_LOG (TIZ_LOG_TRACE, "[%s]", ap_comp_name);

  if (!(p_msg = init_core_message (ETIZCoreMsgGetHandle)))
    {
      TIZ_LOG (TIZ_LOG_ERROR, "Error creating GetHandle message");
      return OMX_ErrorInsufficientResources;
    }

  /* Finish-up this message */
  p_gethdl = (tizcore_msg_gethandle_t *) p_msg->p_data;
  assert (p_gethdl);
  p_gethdl->pp_hdl = app_hdl;
  p_gethdl->p_comp_name = ap_comp_name;
  p_gethdl->p_app_data = ap_app_data;
  p_gethdl->p_callbacks = ap_callbacks;

  return send_msg_blocking (p_msg);
}

OMX_ERRORTYPE
OMX_FreeHandle (OMX_HANDLETYPE ap_hdl)
{
  tizcore_msg_t *p_msg = NULL;
  tizcore_msg_freehandle_t *p_freehdl = NULL;

  assert (ap_hdl);

  if (!(p_msg = init_core_message (ETIZCoreMsgFreeHandle)))
    {
      TIZ_LOG (TIZ_LOG_ERROR, "Error creating FreeHandle message");
      return OMX_ErrorInsufficientResources;
    }

  /* Finish-up this message */
  p_freehdl = (tizcore_msg_freehandle_t *) p_msg->p_data;
  assert (p_freehdl);
  p_freehdl->p_hdl = ap_hdl;

  return send_msg_blocking (p_msg);
}

OMX_ERRORTYPE
OMX_SetupTunnel (OMX_HANDLETYPE ap_outhdl, OMX_U32 a_outport,
                 OMX_HANDLETYPE ap_inhdl, OMX_U32 a_inport)
{
  TIZ_LOG (TIZ_LOG_TRACE, "ap_outhdl [%p] a_outport [%d] "
             "ap_inhdl [%p] a_inport [%d]", ap_outhdl, a_outport,
             ap_inhdl, a_inport);

  if (!ap_outhdl || !ap_inhdl)
    {
      TIZ_LOG (TIZ_LOG_ERROR,
                 "OMX_ErrorBadParameter: NULL hdl received");
      return OMX_ErrorBadParameter;
    }

  return do_tunnel_requests(ap_outhdl, a_outport, ap_inhdl, a_inport);
}

OMX_ERRORTYPE
OMX_TeardownTunnel(OMX_HANDLETYPE ap_outhdl, OMX_U32 a_outport,
                 OMX_HANDLETYPE ap_inhdl, OMX_U32 a_inport)
{
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  TIZ_LOG (TIZ_LOG_TRACE, "ap_outhdl [%p] a_outport [%d] "
             "ap_inhdl [%p] a_inport [%d]", ap_outhdl, a_outport,
             ap_inhdl, a_inport);

  if (!ap_outhdl || !ap_inhdl)
    {
      TIZ_LOG (TIZ_LOG_ERROR,
                 "OMX_ErrorBadParameter: NULL hdl received");
      return OMX_ErrorBadParameter;
    }

  rc = do_tunnel_requests(ap_outhdl, a_outport, NULL, a_inport);

  if (OMX_ErrorNone == rc)
    {
      rc = do_tunnel_requests(NULL, a_outport, ap_inhdl, a_inport);
    }

  return rc;
}

OMX_ERRORTYPE
OMX_ComponentOfRoleEnum(OMX_STRING ap_comp_name, OMX_STRING ap_role,
                        OMX_U32 a_index)
{
  assert(0);
  return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE
OMX_RoleOfComponentEnum(OMX_STRING ap_role, OMX_STRING ap_comp_name,
                        OMX_U32 a_index)
{
  tizcore_msg_t *p_msg = NULL;
  tizcore_msg_roleofcompenum_t *p_msg_rofcompenum = NULL;

  TIZ_LOG (TIZ_LOG_TRACE, "OMX_RoleOfComponentEnum");

  if (!ap_comp_name || !ap_role)
    {
      TIZ_LOG (TIZ_LOG_ERROR, "OMX_ErrorBadParameter");
      return OMX_ErrorBadParameter;
    }

  if (!(p_msg = init_core_message (ETIZCoreMsgRoleOfComponentEnum)))
    {
      TIZ_LOG (TIZ_LOG_ERROR, "Error creating RoleOfComponentEnum message");
      return OMX_ErrorInsufficientResources;
    }

  /* Finish-up this message */
  p_msg_rofcompenum = (tizcore_msg_roleofcompenum_t *) p_msg->p_data;
  assert (p_msg_rofcompenum);
  p_msg_rofcompenum->p_comp_name = ap_comp_name;
  p_msg_rofcompenum->p_role = ap_role;
  p_msg_rofcompenum->index = a_index;

  return send_msg_blocking (p_msg);
}

OMX_ERRORTYPE
OMX_GetCoreInterface(void ** ppItf, OMX_STRING cExtensionName)
{
  assert(0);
  return OMX_ErrorNotImplemented;
}

OMX_API void
OMX_FreeCoreInterface(void * pItf)
{
  assert(0);
}
