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
 * @file   tizport.c
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - port class implementation
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include "OMX_Types.h"
#include "OMX_TizoniaExt.h"

#include "tizport.h"
#include "tizport_decls.h"
#include "tizfsm.h"

#include "tizutils.h"
#include "tizosal.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.tizonia.port"
#endif

#define TIZ_USEBUFFER_WAIT_MICROSECONDS 500000

#define TIZ_HDR_NOT_FOUND -1

#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

static OMX_VERSIONTYPE _spec_version = {
  {
   OMX_VERSION_MAJOR,
   OMX_VERSION_MINOR,
   OMX_VERSION_REVISION,
   OMX_VERSION_STEP}
};

typedef struct tizport_buf_props tizport_buf_props_t;

struct tizport_buf_props
{
  OMX_BUFFERHEADERTYPE *p_hdr;
/*   tiz_pd_set_t flags_; */
  OMX_BOOL owned;
};

typedef struct tizport_mark_info tizport_mark_info_t;

struct tizport_mark_info
{
  OMX_HANDLETYPE p_target;
  OMX_PTR p_data;
  OMX_BOOL owned;
};

static OMX_U8 *
default_alloc_hook (OMX_U32 * ap_size, OMX_PTR * app_port_priv, void *ap_args)
{
  OMX_U8 *p = NULL;

  assert (ap_size);

  p = tiz_mem_alloc (*ap_size * sizeof (OMX_U8));

  TIZ_LOG (TIZ_LOG_TRACE, "size [%u] pBuffer [%p]", *ap_size, p);

  return p;
}

static void
default_free_hook (OMX_PTR ap_buf, OMX_PTR ap_port_priv, void *ap_args)
{
  TIZ_LOG (TIZ_LOG_TRACE, "pBuffer [%p]", ap_buf);
  assert (ap_buf);
  tiz_mem_free (ap_buf);
}

static OMX_ERRORTYPE
alloc_buf (void *ap_obj,
           OMX_U32 * ap_size, OMX_U8 ** app_buf, OMX_PTR * app_portPrivate)
{
  struct tizport *p_obj = ap_obj;
  OMX_U8 *p_buf = 0;
  OMX_U32 alloc_size = 0;

  assert (ap_size);
  assert (app_buf);

  alloc_size = *ap_size;

  if (!(p_buf = p_obj->opts_.mem_hooks.pf_alloc
        (&alloc_size, app_portPrivate, p_obj->opts_.mem_hooks.p_args)))
    {
      return OMX_ErrorInsufficientResources;
    }

  TIZ_LOG (TIZ_LOG_TRACE, "size [%u] pBuffer [%p]", *ap_size, p_buf);

  /* Verify 1.1.2 behaviour */
  if (OMX_TRUE == p_obj->announce_bufs_)
    {
      assert (alloc_size == *ap_size);
    }

  *ap_size = alloc_size;
  *app_buf = p_buf;

  return OMX_ErrorNone;
}

static void
free_buf (void *ap_obj, OMX_PTR ap_buf, OMX_PTR ap_portPrivate)
{
  struct tizport *p_obj = ap_obj;
  TIZ_LOG (TIZ_LOG_TRACE, "ap_buf[%p]", ap_buf);
  assert (ap_buf);
  p_obj->opts_.mem_hooks.pf_free (ap_buf, ap_portPrivate,
                                  p_obj->opts_.mem_hooks.p_args);
}

static OMX_ERRORTYPE
register_header (const void *ap_obj,
                 OMX_BUFFERHEADERTYPE * ap_hdr, OMX_BOOL ais_owned)
{
  struct tizport *p_obj = (struct tizport *) ap_obj;
  tizport_buf_props_t *p_bps = tiz_mem_alloc (sizeof (tizport_buf_props_t));

  if (!p_bps)
    {
      return OMX_ErrorInsufficientResources;
    }

  p_bps->p_hdr = ap_hdr;
  p_bps->owned = ais_owned;

  if (OMX_ErrorNone != tiz_vector_push_back (p_obj->p_hdrs_info_, &p_bps))
    {
      tiz_mem_free (p_bps);
      return OMX_ErrorInsufficientResources;
    }

  return OMX_ErrorNone;
}

static OMX_S32
find_buffer (const void *ap_obj, OMX_BUFFERHEADERTYPE * ap_hdr,
             OMX_BOOL * ap_is_owned)
{
  struct tizport *p_obj = (struct tizport *) ap_obj;
  OMX_S32 i, hdr_count = tiz_vector_length (p_obj->p_hdrs_info_);
  tizport_buf_props_t **pp_bps = NULL, *p_bps = NULL;
  assert (ap_hdr);
  assert (ap_is_owned);

  for (i = 0; i < hdr_count; ++i)
    {
      pp_bps = tiz_vector_at (p_obj->p_hdrs_info_, i);
      assert (pp_bps && *pp_bps);
      p_bps = *pp_bps;
      if (ap_hdr == p_bps->p_hdr)
        {
          *ap_is_owned = p_bps->owned;
          return i;
        }
    }

  return TIZ_HDR_NOT_FOUND;
}

static OMX_BUFFERHEADERTYPE *
deregister_header (const void *ap_obj, OMX_S32 hdr_pos)
{
  struct tizport *p_obj = (struct tizport *) ap_obj;
  tizport_buf_props_t **pp_bps = NULL;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;

  assert (hdr_pos >= 0);
  pp_bps = tiz_vector_at (p_obj->p_hdrs_info_, hdr_pos);
  assert (pp_bps && *pp_bps);
  p_hdr = (*pp_bps)->p_hdr;
  tiz_mem_free (*pp_bps);
  tiz_vector_erase (p_obj->p_hdrs_info_, hdr_pos, 1);
  return p_hdr;
}

/*
 * tizport class
 */

static void *
port_ctor (void *ap_obj, va_list * app)
{
  struct tizport *p_obj = super_ctor (tizport, ap_obj, app);
  tizport_options_t *p_opts = NULL;
  OMX_BOOL supplier = OMX_FALSE;
  OMX_INDEXTYPE id1 = OMX_IndexParamPortDefinition;
  OMX_INDEXTYPE id2 = OMX_IndexParamCompBufferSupplier;
  OMX_INDEXTYPE id3 = OMX_TizoniaIndexParamBufferPreAnnouncementsMode;

  /* Register the indexes managed by this base port class */
  tiz_vector_init (&(p_obj->p_indexes_), sizeof (OMX_INDEXTYPE));
  tiz_vector_push_back (p_obj->p_indexes_, &id1);
  tiz_vector_push_back (p_obj->p_indexes_, &id2);
  tiz_vector_push_back (p_obj->p_indexes_, &id3);

  /* Init buffer headers list */
  tiz_vector_init (&(p_obj->p_hdrs_info_), sizeof (tizport_buf_props_t *));
  tiz_vector_init (&(p_obj->p_hdrs_), sizeof (OMX_BUFFERHEADERTYPE *));

  /* Init buffer marks list */
  tiz_vector_init (&(p_obj->p_marks_), sizeof (tizport_mark_info_t *));

  /* Initialize the port options structure */
  if ((p_opts = va_arg (*app, tizport_options_t *)))
    {
      p_obj->opts_ = *p_opts;
    }

  if (NULL == p_obj->opts_.mem_hooks.pf_alloc
      || NULL == p_obj->opts_.mem_hooks.pf_free)
    {
      /* Use default hooks */
      p_obj->opts_.mem_hooks.pf_alloc = default_alloc_hook;
      p_obj->opts_.mem_hooks.pf_free = default_free_hook;
      p_obj->opts_.mem_hooks.p_args = NULL;
    }

  /* Init the OMX_PARAM_PORTDEFINITIONTYPE structure */
  p_obj->portdef_.nSize = sizeof (OMX_PARAM_PORTDEFINITIONTYPE);
  p_obj->portdef_.nVersion.nVersion = OMX_VERSION;
  p_obj->portdef_.eDir = p_obj->opts_.dir;
  p_obj->portdef_.nBufferCountMin = p_obj->opts_.min_buf_count;
  /* From 1.2, nBufferCountActual must default to nBufferCountMin */
  p_obj->portdef_.nBufferCountActual = p_obj->opts_.min_buf_count;
  p_obj->portdef_.nBufferSize = p_obj->opts_.min_buf_size;
  p_obj->portdef_.bEnabled = OMX_TRUE;
  p_obj->portdef_.bPopulated = OMX_FALSE;
  p_obj->portdef_.eDomain = p_obj->opts_.domain;
  /* NOTE: p_obj->portdef_.format must be finished up by concrete ports */
  p_obj->portdef_.bBuffersContiguous = p_obj->opts_.contiguous;
  p_obj->portdef_.nBufferAlignment = p_obj->opts_.alignment;

  /* Store here the port's preference that needs to be advertised when the
   * component is in OMX_StateLoaded or the port is disabled. */
  p_obj->contiguity_pref_ = p_obj->opts_.contiguous;

  /* Init the OMX_PARAM_BUFFERSUPPLIERTYPE structure */
  p_obj->bufsupplier_.nSize = sizeof (OMX_PARAM_BUFFERSUPPLIERTYPE);
  p_obj->bufsupplier_.nVersion.nVersion = OMX_VERSION;
  p_obj->bufsupplier_.eBufferSupplier = p_obj->opts_.buf_supplier;

  /* Initialize the port flags */
  TIZ_PD_ZERO (&p_obj->flags_);

/*   { */
/*     unsigned int a_i; */
/*     tiz_pd_set_t *a_arr = &(p_obj->flags_); */
/*     for (a_i=0; a_i < sizeof(tiz_pd_set_t) / sizeof(_tiz_pd_mask); ++a_i) */
/*       /\* TIZ_PDS_BITS (a_arr)[a_i] = 0; *\/ */
/*       TIZ_LOG (TIZ_LOG_TRACE, " TIZ_PDS_BITS(a_arr)[%u] = [%x] ", a_i, TIZ_PDS_BITS (a_arr)[a_i]); */

/*   } */

  TIZ_PD_SET (EFlagEnabled, &p_obj->flags_);

/*   { */
/*     unsigned int a_i; */
/*     tiz_pd_set_t *a_arr = &(p_obj->flags_); */
/*     for (a_i=0; a_i < sizeof(tiz_pd_set_t) / sizeof(_tiz_pd_mask); ++a_i) */
/*       /\* TIZ_PDS_BITS (a_arr)[a_i] = 0; *\/ */
/*       TIZ_LOG (TIZ_LOG_TRACE, " TIZ_PDS_BITS(a_arr)[%u] = [%x] ", a_i, TIZ_PDS_BITS (a_arr)[a_i]); */

/*   } */

  supplier = ((p_obj->portdef_.eDir == OMX_DirInput &&
               p_obj->bufsupplier_.eBufferSupplier == OMX_BufferSupplyInput)
              ||
              (p_obj->portdef_.eDir == OMX_DirOutput &&
               p_obj->bufsupplier_.eBufferSupplier == OMX_BufferSupplyOutput))
    ? OMX_TRUE : OMX_FALSE;

  if (supplier)
    {
      tizport_set_flags (p_obj, 1, EFlagBufferSupplier);


      {
        unsigned int a_i;
        tiz_pd_set_t *a_arr = &(p_obj->flags_);
        for (a_i = 0; a_i < sizeof (tiz_pd_set_t) / sizeof (_tiz_pd_mask);
             ++a_i)
          /* TIZ_PDS_BITS (a_arr)[a_i] = 0; */
          TIZ_LOG (TIZ_LOG_TRACE, " TIZ_PDS_BITS(a_arr)[%u] = [%x] ", a_i,
                   TIZ_PDS_BITS (a_arr)[a_i]);
      }

    }

  p_obj->thdl_ = NULL;
  p_obj->tpid_ = 0;
  p_obj->claimed_count_ = 0;

  p_obj->announce_bufs_ = OMX_TRUE;     /* Default to 1.1.2 behaviour */

  return p_obj;
}

static void *
port_dtor (void *ap_obj)
{
  struct tizport *p_obj = ap_obj;

  assert (p_obj);

  tiz_vector_clear (p_obj->p_indexes_);
  tiz_vector_destroy (p_obj->p_indexes_);

  /* TODO : Delete tizport_buf_props_t items, if any */
  tiz_vector_clear (p_obj->p_hdrs_info_);
  tiz_vector_destroy (p_obj->p_hdrs_info_);

  tiz_vector_clear (p_obj->p_hdrs_);
  tiz_vector_destroy (p_obj->p_hdrs_);

  /* TODO : Delete tizport_mark_info_t items, if any */
  tiz_vector_clear (p_obj->p_marks_);
  tiz_vector_destroy (p_obj->p_marks_);

  return super_dtor (tizport, ap_obj);
}

/*
 * from tizapi
 */

static OMX_ERRORTYPE
port_GetParameter (const void *ap_obj,
                   OMX_HANDLETYPE ap_hdl,
                   OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const struct tizport *p_obj = ap_obj;

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "PORT [%d] GetParameter [%s]...",
                 p_obj->pid_, tiz_idx_to_str (a_index));
  assert (p_obj);

  switch (a_index)
    {
    case OMX_IndexParamPortDefinition:
      {
        const tizfsm_state_id_t now =
          tizfsm_get_substate (tiz_get_fsm (ap_hdl));
        OMX_PARAM_PORTDEFINITIONTYPE *p_pdef = ap_struct;
        *p_pdef = p_obj->portdef_;

        /* From IL 1.2, bBuffersContiguous should advertise the port's
         * preference in OMX_StateLoaded state and port Disabled  */
        if (now == EStateLoaded || OMX_FALSE == p_obj->portdef_.bEnabled)
          {
            p_pdef->bBuffersContiguous = p_obj->contiguity_pref_;
          }
      }
      break;

    case OMX_IndexParamCompBufferSupplier:
      {
        OMX_PARAM_BUFFERSUPPLIERTYPE *p_bs = ap_struct;
        p_bs->nVersion.nVersion = OMX_VERSION;
        p_bs->eBufferSupplier = p_obj->bufsupplier_.eBufferSupplier;
      }
      break;

    default:
      {
        if (OMX_TizoniaIndexParamBufferPreAnnouncementsMode == a_index)
          {
            OMX_TIZONIA_PARAM_BUFFER_PREANNOUNCEMENTSMODETYPE *p_pm =
              ap_struct;
            p_pm->nVersion.nVersion = OMX_VERSION;
            p_pm->bEnabled = p_obj->announce_bufs_;
          }
        else
          {
            TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl),
                           TIZ_CBUF (ap_hdl),
                           "[OMX_ErrorUnsupportedIndex] - [%s]...",
                           tiz_idx_to_str (a_index));
            return OMX_ErrorUnsupportedIndex;
          }
      }
    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
port_SetParameter (const void *ap_obj,
                   OMX_HANDLETYPE ap_hdl,
                   OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  struct tizport *p_obj = (struct tizport *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;

  assert (p_obj);

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "PORT [%d] SetParameter [%s]...", p_obj->pid_,
                 tiz_idx_to_str (a_index));

  switch (a_index)
    {
    case OMX_IndexParamPortDefinition:
      {
        const tizfsm_state_id_t now =
          tizfsm_get_substate (tiz_get_fsm (ap_hdl));
        const OMX_PARAM_PORTDEFINITIONTYPE *p_pdef
          = (OMX_PARAM_PORTDEFINITIONTYPE *) ap_struct;

        /* The derived port knows how to set the 'format' field ... */
        if (OMX_ErrorNone
            != (rc = tizport_set_portdef_format (p_obj, p_pdef)))
          {
            return rc;
          }

        /* Apply values to the read-write parameters only. From IL 1.2 those
         * are: nBufferCountActual and bBuffersContiguous */
        if (p_pdef->nBufferCountActual < p_obj->portdef_.nBufferCountMin)
          {
            return OMX_ErrorBadParameter;
          }

        p_obj->portdef_.nBufferCountActual = p_pdef->nBufferCountActual;

        /* As of IL 1.2, updates to bBuffersContiguous shall only be allowed if
         * the port is being enabled or during the transition from
         * OMX_StateLoaded to OMX_StateIdle. Otherwise, just ignore. */
        if (now == ESubStateLoadedToIdle || TIZPORT_IS_BEING_ENABLED (p_obj))
          {
            p_obj->portdef_.bBuffersContiguous = p_pdef->bBuffersContiguous;
          }

      }
      break;

    case OMX_IndexParamCompBufferSupplier:
      {
        OMX_BOOL is_supplier = OMX_FALSE;
        const OMX_PARAM_BUFFERSUPPLIERTYPE *p_bufsup
          = (OMX_PARAM_BUFFERSUPPLIERTYPE *) ap_struct;

        if (p_obj->bufsupplier_.eBufferSupplier > OMX_BufferSupplyMax)
          {
            return OMX_ErrorBadParameter;
          }

        if (p_obj->bufsupplier_.eBufferSupplier != p_bufsup->eBufferSupplier)
          {
            /* the buffer supplier override procedure */
            /* is initiated by the input port... */
            if (p_obj->thdl_ && p_obj->portdef_.eDir == OMX_DirInput)
              {
                OMX_PARAM_BUFFERSUPPLIERTYPE supplier;
                supplier.nSize = sizeof (OMX_PARAM_BUFFERSUPPLIERTYPE);
                supplier.nVersion.nVersion = OMX_VERSION;
                supplier.nPortIndex = p_obj->tpid_;
                supplier.eBufferSupplier = p_bufsup->eBufferSupplier;
                if (OMX_ErrorNone
                    != (rc
                        = OMX_SetParameter (p_obj->thdl_,
                                            OMX_IndexParamCompBufferSupplier,
                                            &supplier)))
                  {
                    return rc;
                  }
              }

            p_obj->bufsupplier_.eBufferSupplier = p_bufsup->eBufferSupplier;

            is_supplier =
              ((p_obj->portdef_.eDir == OMX_DirInput &&
                p_obj->bufsupplier_.eBufferSupplier == OMX_BufferSupplyInput)
               ||
               (p_obj->portdef_.eDir == OMX_DirOutput &&
                p_obj->bufsupplier_.eBufferSupplier ==
                OMX_BufferSupplyOutput)) ? OMX_TRUE : OMX_FALSE;

            if (is_supplier)
              {
                tizport_set_flags (p_obj, 1, EFlagBufferSupplier);
              }
            else
              {
                tizport_clear_flags (p_obj, 1, EFlagBufferSupplier);
              }
          }

      }
      break;

    default:
      {
        if (OMX_TizoniaIndexParamBufferPreAnnouncementsMode == a_index)
          {
            const OMX_TIZONIA_PARAM_BUFFER_PREANNOUNCEMENTSMODETYPE *p_pm
              = ap_struct;
            p_obj->announce_bufs_ = p_pm->bEnabled;
            TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl),
                           TIZ_CBUF (ap_hdl), "Preannouncements - [%s]...",
                           p_pm->bEnabled ==
                           OMX_TRUE ? "ENABLED" : "DISABLED");

          }
        else
          {
            TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl),
                           TIZ_CBUF (ap_hdl),
                           "[OMX_ErrorUnsupportedIndex] - [%s]...",
                           tiz_idx_to_str (a_index));
            return OMX_ErrorUnsupportedIndex;
          }
      }

    };

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
port_GetConfig (const void *ap_obj,
                OMX_HANDLETYPE ap_hdl,
                OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const struct tizport *p_obj = ap_obj;
  assert (p_obj);
  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "PORT [%d] GetConfig [%s] (unsupported index)...",
                 p_obj->pid_, tiz_idx_to_str (a_index));
  return OMX_ErrorUnsupportedIndex;
}

static OMX_ERRORTYPE
port_SetConfig (const void *ap_obj,
                OMX_HANDLETYPE ap_hdl,
                OMX_INDEXTYPE a_index, OMX_PTR ap_struct)
{
  const struct tizport *p_obj = ap_obj;
  assert (p_obj);
  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "PORT [%d] SetConfig [%s] (unsupported index)...",
                 p_obj->pid_, tiz_idx_to_str (a_index));
  return OMX_ErrorUnsupportedIndex;
}

static OMX_ERRORTYPE
port_GetExtensionIndex (const void *ap_obj,
                        OMX_HANDLETYPE ap_hdl,
                        OMX_STRING ap_param_name,
                        OMX_INDEXTYPE * ap_index_type)
{
  const struct tizport *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorUnsupportedIndex;

  assert (p_obj);

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "PORT [%d] GetExtensionIndex [%s]...",
                 p_obj->pid_, ap_param_name);

  if (0 == strncmp (ap_param_name,
                    OMX_TIZONIA_INDEX_PARAM_BUFFER_PREANNOUNCEMENTSMODE,
                    strlen
                    (OMX_TIZONIA_INDEX_PARAM_BUFFER_PREANNOUNCEMENTSMODE)))
    {
      *ap_index_type = OMX_TizoniaIndexParamBufferPreAnnouncementsMode;
      rc = OMX_ErrorNone;
    }

  return rc;
}

static OMX_ERRORTYPE
port_ComponentTunnelRequest (const void *ap_obj,
                             OMX_HANDLETYPE ap_hdl,
                             OMX_U32 a_pid,
                             OMX_HANDLETYPE ap_thdl,
                             OMX_U32 a_tpid, OMX_TUNNELSETUPTYPE * ap_tsetup)
{
  struct tizport *p_obj = (struct tizport *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_PARAM_PORTDEFINITIONTYPE port_def = {
    sizeof (OMX_PARAM_PORTDEFINITIONTYPE),
    _spec_version
  };
  OMX_BUFFERSUPPLIERTYPE supplier = OMX_BufferSupplyUnspecified;
  OMX_PARAM_BUFFERSUPPLIERTYPE buf_supplier = {
    sizeof (OMX_PARAM_BUFFERSUPPLIERTYPE),
    _spec_version
  };

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl),
                 TIZ_CBUF (ap_hdl),
                 "ap_hdl [%p] a_pid [%d] ap_thdl [%p] a_tpid [%d]",
                 ap_hdl, a_pid, ap_thdl, a_tpid);

  /* See if the tunnel is being torn down */
  if (!ap_thdl)
    {
      /* Cancel existing tunnel setup, if any */
      p_obj->thdl_ = 0;
      tizport_clear_flags (p_obj, 1, EFlagTunneled);
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl),
                     TIZ_CBUF (ap_hdl), "Tunnel cancelled.");
      return OMX_ErrorNone;
    }

  /* Retrieve the tunneled component's port definition ... */
  port_def.nPortIndex = a_tpid;
  if (OMX_ErrorNone
      != (rc = OMX_GetParameter (ap_thdl, OMX_IndexParamPortDefinition,
                                 &port_def)))
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl),
                     TIZ_CBUF (ap_hdl),
                     "OMX_ErrorUndefined: Tunnelled component returned [%s]",
                     tiz_err_to_str (rc));
      return OMX_ErrorUndefined;
    }

  /* Verify tunneled port's direction */
  if (p_obj->portdef_.eDir == port_def.eDir)
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl),
                     TIZ_CBUF (ap_hdl),
                     "OMX_ErrorPortsNotCompatible: "
                     "this [%p] pid [%d] is [%s] and peer [%p] [%d] is [%s]...",
                     ap_hdl, p_obj->pid_,
                     tiz_dir_to_str (p_obj->portdef_.eDir), ap_thdl, a_tpid,
                     tiz_dir_to_str (port_def.eDir));
      return OMX_ErrorPortsNotCompatible;
    }

  p_obj->thdl_ = ap_thdl;
  p_obj->tpid_ = a_tpid;

  if (OMX_DirOutput == p_obj->portdef_.eDir)
    {
      /* Output port */

      /* Init the tunnel setup structure received... */
      ap_tsetup->nTunnelFlags = 0;
      ap_tsetup->eSupplier = p_obj->bufsupplier_.eBufferSupplier;

    }
  else
    {
      /* OMX_DirInput */

      /* Sanity-check the tunnel setup struct */
      if ((ap_tsetup->eSupplier != OMX_BufferSupplyUnspecified) &&
          (ap_tsetup->eSupplier != OMX_BufferSupplyInput) &&
          (ap_tsetup->eSupplier != OMX_BufferSupplyOutput))
        {
          return OMX_ErrorBadParameter;
        }

      /* Check domain-specific parameter compatibility */
      /* (delegated to derived port classes)... */
      if (!tizport_check_tunnel_compat (p_obj, &(p_obj->portdef_), &port_def))
        {
          p_obj->thdl_ = 0;
          TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl),
                         TIZ_CBUF (ap_hdl),
                         "OMX_ErrorPortsNotCompatible: "
                         "Derived port checks have failed");
          return OMX_ErrorPortsNotCompatible;
        }

      /* Work out the buffer supplier... */
      if (ap_tsetup->nTunnelFlags & OMX_PORTTUNNELFLAG_READONLY
          || ((ap_tsetup->eSupplier == OMX_BufferSupplyInput)
              && (p_obj->bufsupplier_.eBufferSupplier == OMX_BufferSupplyInput
                  || p_obj->bufsupplier_.eBufferSupplier
                  == OMX_BufferSupplyUnspecified))
          ||
          ((ap_tsetup->eSupplier == OMX_BufferSupplyUnspecified) &&
           (p_obj->bufsupplier_.eBufferSupplier == OMX_BufferSupplyInput)))
        {
          supplier = OMX_BufferSupplyInput;
        }
      else
        {
          supplier = OMX_BufferSupplyOutput;
        }

      /* Communicate the buffer supplier decision... */
      buf_supplier.nPortIndex = a_tpid;
      buf_supplier.eBufferSupplier = supplier;
      if (OMX_ErrorNone != OMX_SetParameter (ap_thdl,
                                             OMX_IndexParamCompBufferSupplier,
                                             &buf_supplier))
        {
          p_obj->thdl_ = 0;
          TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl),
                         TIZ_CBUF (ap_hdl),
                         "OMX_ErrorPortsNotCompatible: "
                         "SetParameter"
                         "(OMX_IndexParamCompBufferSupplier) failed");
          return OMX_ErrorPortsNotCompatible;
        }

      ap_tsetup->eSupplier = supplier;
      p_obj->bufsupplier_.eBufferSupplier = supplier;

    }

  tizport_set_flags (p_obj, 1, EFlagTunneled);

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "Tunnel request success [%p:%d] -> [%p:%d]",
                 ap_hdl, p_obj->pid_, p_obj->thdl_, p_obj->tpid_);


  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
port_UseBuffer (const void *ap_obj,
                OMX_HANDLETYPE ap_hdl,
                OMX_BUFFERHEADERTYPE ** app_hdr,
                OMX_U32 a_pid,
                OMX_PTR ap_app_priv, OMX_U32 a_size, OMX_U8 * ap_buf)
{
  struct tizport *p_obj = (struct tizport *) ap_obj;
  OMX_PTR p_port_priv = 0;
  OMX_PTR p_plat_priv = 0;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;

  if (NULL != ap_buf && a_size < p_obj->portdef_.nBufferSize)
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "UseBuffer : Bad parameter found...");
      return OMX_ErrorBadParameter;
    }

  assert (a_pid == p_obj->portdef_.nPortIndex);

  /* Allocate the buffer header... */
  p_hdr = tiz_mem_alloc (sizeof (OMX_BUFFERHEADERTYPE));
  if (!p_hdr)
    {
      return OMX_ErrorInsufficientResources;
    }

  /* register this buffer header... */
  if (OMX_ErrorNone != register_header (p_obj, p_hdr, OMX_FALSE))
    {
      tiz_mem_free (p_hdr);
      return OMX_ErrorInsufficientResources;
    }

  *app_hdr = p_hdr;

  /* Fill in the header fields... */
  p_hdr->nSize = sizeof (OMX_BUFFERHEADERTYPE);
  p_hdr->nVersion = p_obj->portdef_.nVersion;
  p_hdr->pBuffer = ap_buf;
  p_hdr->nAllocLen = a_size;
  p_hdr->nFilledLen = 0;
  p_hdr->nOffset = 0;
  p_hdr->pAppPrivate = ap_app_priv;
  p_hdr->pPlatformPrivate = p_plat_priv;
  p_hdr->hMarkTargetComponent = 0;
  p_hdr->pMarkData = 0;
  p_hdr->nTickCount = 0;
  p_hdr->nTimeStamp = 0;
  p_hdr->nFlags = 0;

  if (OMX_DirInput == p_obj->portdef_.eDir)
    {
      p_hdr->pInputPortPrivate = p_port_priv;
      p_hdr->pOutputPortPrivate = 0;
      p_hdr->nInputPortIndex = p_obj->portdef_.nPortIndex;
      p_hdr->nOutputPortIndex = 0;
    }
  else
    {
      p_hdr->pInputPortPrivate = 0;
      p_hdr->pOutputPortPrivate = p_port_priv;
      p_hdr->nInputPortIndex = 0;
      p_hdr->nOutputPortIndex = p_obj->portdef_.nPortIndex;
    }

  if (p_obj->portdef_.nBufferCountActual ==
      tiz_vector_length (p_obj->p_hdrs_info_))
    {
      tizport_set_flags (p_obj, 2, EFlagPopulated, EFlagEnabled);
      tizport_clear_flags (p_obj, 1, EFlagBeingEnabled);
    }

  TIZ_LOG (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
           "UseBuffer: "
           "HEADER [%p] BUFFER [%p] PORT [%d] POPULATED [%s] "
           "nBufferCountActual [%d] headers [%d]",
           p_hdr, ap_buf, p_obj->portdef_.nPortIndex,
           (TIZ_PD_ISSET (EFlagPopulated, &p_obj->flags_) ? "YES" : "NO"),
           p_obj->portdef_.nBufferCountActual,
           tiz_vector_length (p_obj->p_hdrs_info_));

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
port_AllocateBuffer (const void *ap_obj,
                     OMX_HANDLETYPE ap_hdl,
                     OMX_BUFFERHEADERTYPE ** app_hdr,
                     OMX_U32 a_pid, OMX_PTR ap_app_priv, OMX_U32 a_size)
{
  struct tizport *p_obj = (struct tizport *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_U8 *p_buf = 0;
  OMX_PTR p_port_priv = 0, p_plat_priv = 0;
  OMX_U32 buf_size = a_size;

  if (buf_size < p_obj->portdef_.nBufferSize)
    {
      TIZ_LOG (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
               "AllocateBuffer : Bad parameter found...");
      return OMX_ErrorBadParameter;
    }

  assert (a_pid == p_obj->portdef_.nPortIndex);

  /* Allocate the buffer header... */
  OMX_BUFFERHEADERTYPE *p_hdr = tiz_mem_alloc (sizeof (OMX_BUFFERHEADERTYPE));
  if (!p_hdr)
    {
      return OMX_ErrorInsufficientResources;
    }

  *app_hdr = p_hdr;

  /* Do the port-specific buffer allocation ... */
  if (OMX_ErrorNone != (rc = alloc_buf (p_obj,
                                        &buf_size, &p_buf, &p_port_priv)))
    {
      tiz_mem_free (p_hdr);
      *app_hdr = 0;
      return rc;
    }

  /* register this buffer header... */
  if (OMX_ErrorNone != register_header (p_obj, p_hdr, OMX_TRUE))
    {
      free_buf (p_obj, p_buf, p_port_priv);
      tiz_mem_free (p_hdr);
      *app_hdr = NULL;
      return OMX_ErrorInsufficientResources;
    }

  /* Fill in the header fields... */
  p_hdr->nSize = sizeof (OMX_BUFFERHEADERTYPE);
  p_hdr->nVersion = p_obj->portdef_.nVersion;
  p_hdr->pBuffer = p_buf;
  p_hdr->nAllocLen = buf_size;
  p_hdr->nFilledLen = 0;
  p_hdr->nOffset = 0;
  p_hdr->pAppPrivate = ap_app_priv;
  p_hdr->pPlatformPrivate = p_plat_priv;
  p_hdr->hMarkTargetComponent = 0;
  p_hdr->pMarkData = 0;
  p_hdr->nTickCount = 0;
  p_hdr->nTimeStamp = 0;
  p_hdr->nFlags = 0;

  if (OMX_DirInput == p_obj->portdef_.eDir)
    {
      p_hdr->pInputPortPrivate = p_port_priv;
      p_hdr->pOutputPortPrivate = 0;
      p_hdr->nInputPortIndex = p_obj->portdef_.nPortIndex;
      p_hdr->nOutputPortIndex = 0;
    }
  else
    {
      p_hdr->pInputPortPrivate = 0;
      p_hdr->pOutputPortPrivate = p_port_priv;
      p_hdr->nInputPortIndex = 0;
      p_hdr->nOutputPortIndex = p_obj->portdef_.nPortIndex;
    }

  if (tiz_vector_length (p_obj->p_hdrs_info_) ==
      p_obj->portdef_.nBufferCountActual)
    {
      tizport_set_flags (p_obj, 2, EFlagPopulated, EFlagEnabled);
      tizport_clear_flags (p_obj, 1, EFlagBeingEnabled);
    }

  TIZ_LOG (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
           "AllocateBuffer: "
           "HEADER [%p] BUFFER [%p] PORT [%d] POPULATED [%s] "
           "nBufferCountActual [%d] headers [%d]",
           p_hdr, p_buf, p_obj->portdef_.nPortIndex,
           (TIZ_PD_ISSET (EFlagPopulated, &p_obj->flags_) ? "YES" : "NO"),
           p_obj->portdef_.nBufferCountActual,
           tiz_vector_length (p_obj->p_hdrs_info_));

  tizport_set_flags (p_obj, 1, EFlagBufferSupplier);

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
port_FreeBuffer (const void *ap_obj,
                 OMX_HANDLETYPE ap_hdl,
                 OMX_U32 a_pid, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  struct tizport *p_obj = (struct tizport *) ap_obj;
  OMX_BOOL is_owned = OMX_FALSE;
  OMX_S32 hdr_pos = find_buffer (p_obj, ap_hdr, &is_owned);
  OMX_S32 hdr_count = 0;
  OMX_PTR p_port_priv = NULL;

  TIZ_LOG (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
           "FreeBuffer : HEADER [%p] BUFFER [%p]", ap_hdr, ap_hdr->pBuffer);

  if (TIZ_HDR_NOT_FOUND == hdr_pos)
    {
      return OMX_ErrorBadParameter;
    }

  p_port_priv = OMX_DirInput == p_obj->portdef_.eDir ?
    ap_hdr->pInputPortPrivate : ap_hdr->pOutputPortPrivate;

  if (is_owned)
    {
      free_buf (p_obj, ap_hdr->pBuffer, p_port_priv);
    }

  tiz_mem_free (ap_hdr);
  deregister_header (p_obj, hdr_pos);

  hdr_count = tiz_vector_length (p_obj->p_hdrs_info_);
  if (hdr_count < p_obj->portdef_.nBufferCountActual)
    {
      tizport_clear_flags (p_obj, 1, EFlagPopulated);
    }

  if (0 == hdr_count && TIZ_PD_ISSET (EFlagBeingDisabled, &p_obj->flags_))
    {
      tizport_clear_flags (p_obj, 1, EFlagBeingDisabled);
      tizport_clear_flags (p_obj, 1, EFlagEnabled);
    }

  return OMX_ErrorNone;
}

static OMX_ERRORTYPE
port_register_index (const void *ap_obj, OMX_INDEXTYPE a_index)
{
  struct tizport *p_obj = (struct tizport *) ap_obj;
  return tiz_vector_push_back (p_obj->p_indexes_, &a_index);
}

OMX_ERRORTYPE
tizport_register_index (const void *ap_obj, OMX_INDEXTYPE a_index)
{
  const struct tizport_class *class = classOf (ap_obj);
  assert (class->register_index);
  return class->register_index (ap_obj, a_index);
}

OMX_ERRORTYPE
tizport_super_register_index (const void *a_class,
                              const void *ap_obj, OMX_INDEXTYPE a_index)
{
  const struct tizport_class *superclass = super (a_class);
  assert (ap_obj && superclass->register_index);
  return superclass->register_index (ap_obj, a_index);
}


static OMX_ERRORTYPE
port_find_index (const void *ap_obj, OMX_INDEXTYPE a_index)
{
  struct tizport *p_obj = (struct tizport *) ap_obj;
  TIZ_LOG (TIZ_LOG_TRACE, "PORT [%d] - Searching for index [%s]...",
           p_obj->pid_, tiz_idx_to_str (a_index));
  assert (p_obj);
  return (tiz_vector_find (p_obj->p_indexes_, &a_index) ?
          OMX_ErrorNone : OMX_ErrorUnsupportedIndex);
}

OMX_ERRORTYPE
tizport_find_index (const void *ap_obj, OMX_INDEXTYPE a_index)
{
  const struct tizport_class *class = classOf (ap_obj);
  assert (class->find_index);
  return class->find_index (ap_obj, a_index);
}

OMX_ERRORTYPE
tizport_super_find_index (const void *a_class,
                          const void *ap_obj, OMX_INDEXTYPE a_index)
{
  const struct tizport_class *superclass = super (a_class);

  assert (ap_obj && superclass->find_index);
  return superclass->find_index (ap_obj, a_index);
}

static OMX_U32
port_index (const void *ap_obj)
{
  struct tizport *p_obj = (struct tizport *) ap_obj;
  assert (p_obj);
  return p_obj->pid_;
}

OMX_U32
tizport_index (const void *ap_obj)
{
  const struct tizport_class *class = classOf (ap_obj);
  assert (class->index);
  return class->index (ap_obj);
}

static void
port_set_index (void *ap_obj, OMX_U32 a_pid)
{
  struct tizport *p_obj = ap_obj;
  p_obj->pid_ = a_pid;
  p_obj->bufsupplier_.nPortIndex = a_pid;
  p_obj->portdef_.nPortIndex = a_pid;
}

void
tizport_set_index (void *ap_obj, OMX_U32 a_pid)
{
  const struct tizport_class *class = classOf (ap_obj);
  assert (class->set_index);
  class->set_index (ap_obj, a_pid);
}

static OMX_ERRORTYPE
port_set_portdef_format (void *ap_obj,
                         const OMX_PARAM_PORTDEFINITIONTYPE * ap_pdef)
{
  /* To be implemented by derived classes */
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tizport_set_portdef_format (void *ap_obj,
                            const OMX_PARAM_PORTDEFINITIONTYPE * ap_pdef)
{
  const struct tizport_class *class = classOf (ap_obj);
  assert (class->set_portdef_format);
  return class->set_portdef_format (ap_obj, ap_pdef);
}

static OMX_S32
port_buffer_count (const void *ap_obj)
{
  const struct tizport *p_obj = ap_obj;
  return tiz_vector_length (p_obj->p_hdrs_info_);
}

OMX_S32
tizport_buffer_count (const void *ap_obj)
{
  const struct tizport_class *class = classOf (ap_obj);
  assert (class->buffer_count);
  return class->buffer_count (ap_obj);
}

static OMX_DIRTYPE
port_dir (const void *ap_obj)
{
  const struct tizport *p_obj = ap_obj;
  return p_obj->opts_.dir;
}

OMX_DIRTYPE
tizport_dir (const void *ap_obj)
{
  const struct tizport_class *class = classOf (ap_obj);
  assert (class->dir);
  return class->dir (ap_obj);
}

static OMX_PORTDOMAINTYPE
port_domain (const void *ap_obj)
{
  const struct tizport *p_obj = ap_obj;
  return p_obj->opts_.domain;
}

OMX_PORTDOMAINTYPE
tizport_domain (const void *ap_obj)
{
  const struct tizport_class *class = classOf (ap_obj);
  assert (class->domain);
  return class->domain (ap_obj);
}

static OMX_HANDLETYPE
port_get_tunnel_comp (const void *ap_obj)
{
  const struct tizport *p_obj = ap_obj;
  return p_obj->thdl_;
}

OMX_HANDLETYPE
tizport_get_tunnel_comp (const void *ap_obj)
{
  const struct tizport_class *class = classOf (ap_obj);
  assert (class->get_tunnel_comp);
  return class->get_tunnel_comp (ap_obj);
}

static tiz_vector_t *
port_get_hdrs_list (void *ap_obj)
{
  struct tizport *p_obj = ap_obj;
  const OMX_S32 hdr_count = tiz_vector_length (p_obj->p_hdrs_info_);
  tizport_buf_props_t **pp_bps = NULL, *p_bps = NULL;
  OMX_S32 i = 0;

  tiz_vector_clear (p_obj->p_hdrs_);
  for (i = 0; i < hdr_count; ++i)
    {
      pp_bps = tiz_vector_at (p_obj->p_hdrs_info_, i);
      assert (pp_bps && *pp_bps);
      p_bps = *pp_bps;
      tiz_vector_push_back (p_obj->p_hdrs_, &(p_bps->p_hdr));
    }

  return p_obj->p_hdrs_;
}

tiz_vector_t *
tizport_get_hdrs_list (void *ap_obj)
{
  const struct tizport_class *class = classOf (ap_obj);
  assert (class->get_hdrs_list);
  return class->get_hdrs_list (ap_obj);
}

static OMX_BOOL
port_check_flags (const void *ap_obj, OMX_U32 a_nflags, va_list * app)
{
  struct tizport *p_obj = (struct tizport *) ap_obj;
  assert (p_obj);
  OMX_BOOL rv = OMX_TRUE;
  OMX_U32 i;
  tizport_flag_ids_t flag;
  va_list ap;
  va_copy (ap, *app);

  {
/*     unsigned int a_i; */
/*     tiz_pd_set_t *a_arr = &(p_obj->flags_); */
/*     for (a_i=0; a_i < sizeof(tiz_pd_set_t) / sizeof(_tiz_pd_mask); ++a_i) */
    /* TIZ_PDS_BITS (a_arr)[a_i] = 0; */
/*       TIZ_LOG (TIZ_LOG_TRACE, " TIZ_PDS_BITS(a_arr)[%u] = [%x] ", a_i, TIZ_PDS_BITS (a_arr)[a_i]); */

  }

  for (i = 0; i < a_nflags; ++i)
    {
      flag = va_arg (ap, tizport_flag_ids_t);
      if (!TIZ_PD_ISSET (flag, &p_obj->flags_))
        {
          rv = OMX_FALSE;
          break;
        }
    }

  va_end (ap);
  return rv;
}

OMX_BOOL
tizport_check_flags (const void *ap_obj, OMX_U32 a_nflags, ...)
{
  const struct tizport_class *class = classOf (ap_obj);
  va_list ap;
  OMX_BOOL rc;
  assert (class->check_flags);

  va_start (ap, a_nflags);
  rc = class->check_flags (ap_obj, a_nflags, &ap);
  va_end (ap);

  return rc;
}

static void
port_set_flags (const void *ap_obj, OMX_U32 a_nflags, va_list * app)
{
  struct tizport *p_obj = (struct tizport *) ap_obj;
  assert (p_obj);
  OMX_U32 i;
  tizport_flag_ids_t flag;
  va_list ap;
  va_copy (ap, *app);

  for (i = 0; i < a_nflags; ++i)
    {
      flag = va_arg (ap, tizport_flag_ids_t);
      TIZ_PD_SET (flag, &p_obj->flags_);
      if (EFlagEnabled == flag || EFlagBeingEnabled == flag)
        {
          p_obj->portdef_.bEnabled = OMX_TRUE;
          TIZ_PD_SET (EFlagEnabled, &p_obj->flags_);
        }
      else if (EFlagPopulated == flag)
        {
          p_obj->portdef_.bPopulated = OMX_TRUE;
        }
      else if (EFlagBufferSupplier == flag)
        {
          /* A supplier is always an allocator, but not the other way around */
          TIZ_PD_SET (EFlagBufferAllocator, &p_obj->flags_);
        }

    }

  va_end (ap);
}

void
tizport_set_flags (const void *ap_obj, OMX_U32 a_nflags, ...)
{
  const struct tizport_class *class = classOf (ap_obj);
  va_list ap;
  assert (class->set_flags);

  va_start (ap, a_nflags);
  class->set_flags (ap_obj, a_nflags, &ap);
  va_end (ap);
}

static void
port_clear_flags (const void *ap_obj, OMX_U32 a_nflags, va_list * app)
{
  struct tizport *p_obj = (struct tizport *) ap_obj;
  assert (p_obj);
  OMX_U32 i;
  tizport_flag_ids_t flag;
  va_list ap;
  va_copy (ap, *app);

  for (i = 0; i < a_nflags; ++i)
    {
      flag = va_arg (ap, tizport_flag_ids_t);
      TIZ_PD_CLR (flag, &p_obj->flags_);
      if (EFlagEnabled == flag)
        {
          p_obj->portdef_.bEnabled = OMX_FALSE;
        }
      else if (EFlagPopulated == flag)
        {
          p_obj->portdef_.bPopulated = OMX_FALSE;
        }
      else if (EFlagBufferSupplier == flag)
        {
          /* A supplier is always an allocator, but not the other way around */
          TIZ_PD_CLR (EFlagBufferAllocator, &p_obj->flags_);
        }

    }

  va_end (ap);
}

void
tizport_clear_flags (const void *ap_obj, OMX_U32 a_nflags, ...)
{
  const struct tizport_class *class = classOf (ap_obj);
  va_list ap;
  assert (class->clear_flags);

  va_start (ap, a_nflags);
  class->clear_flags (ap_obj, a_nflags, &ap);
  va_end (ap);
}

static OMX_ERRORTYPE
port_populate (const void *ap_obj, OMX_HANDLETYPE ap_hdl)
{
  struct tizport *p_obj = (struct tizport *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_U32 nbufs = 0, nbytes = 0, i = 0;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  OMX_U8 *p_buf = 0;
  OMX_PTR p_port_priv = 0;
  OMX_PARAM_PORTDEFINITIONTYPE port_def = {
    sizeof (OMX_PARAM_PORTDEFINITIONTYPE),
    _spec_version
  };

  assert (!tiz_vector_length (p_obj->p_hdrs_info_));
  assert (TIZPORT_IS_TUNNELED_AND_SUPPLIER (p_obj));
  assert (p_obj->thdl_);

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "port index [%d]...", p_obj->pid_);

  /* Retrieve the port definition from peer */
  port_def.nPortIndex = p_obj->tpid_;
  if (OMX_ErrorNone
      != (rc = OMX_GetParameter (p_obj->thdl_,
                                 OMX_IndexParamPortDefinition, &port_def)))
    {
      TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "[%s] while retrieving the "
                     "port definition from peer.. ", tiz_err_to_str (rc));
      return OMX_ErrorUndefined;
    }

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "Retrieved port definition from peer - "
                 "peer's nBufferCountActual = [%d]...",
                 port_def.nBufferCountActual);

  /* Work out the number of buffers to allocate */
  nbufs = p_obj->portdef_.nBufferCountActual;
  if (port_def.nBufferCountActual != p_obj->portdef_.nBufferCountActual)
    {

      nbufs =
        MAX ((p_obj->portdef_.nBufferCountActual),
             (port_def.nBufferCountActual));
      if (p_obj->portdef_.nBufferCountActual < nbufs)
        {
          /* Update own buffer count requirements */
          TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                         "port index [%d] "
                         "own nBufferCountActual - Old [%d] New [%d]",
                         p_obj->pid_, p_obj->portdef_.nBufferCountActual,
                         nbufs);
          p_obj->portdef_.nBufferCountActual = nbufs;
        }

      else
        {
          /* Update peer's buffer count requirements */
          TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                         "Peer port index [%d] "
                         "peer's nBufferCountActual - Old [%d] New [%d]",
                         p_obj->tpid_, p_obj->portdef_.nBufferCountActual,
                         nbufs);
          port_def.nBufferCountActual = nbufs;
          if (OMX_ErrorNone
              != (rc = OMX_SetParameter (p_obj->thdl_,
                                         OMX_IndexParamPortDefinition,
                                         &port_def)))
            {
              TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl),
                             TIZ_CBUF (ap_hdl),
                             "[%s] while updating "
                             "nBufferCountActual. Peer port index [%d] "
                             "nBufferCountActual - New [%d]",
                             tiz_err_to_str (rc), nbufs);
              return OMX_ErrorUndefined;
            }
        }
    }


  /* Tunnel population... */
  nbytes = MAX (p_obj->portdef_.nBufferSize, port_def.nBufferSize);

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "Starting tunnel population : nbufs [%d] nbytes [%d]...",
                 nbufs, nbytes);

  for (i = 0; i < nbufs; ++i)
    {
      /* Allocate the buffer, but only if the 1.1.2 behaviour is
       * enabled. Otherwise, this is deferred until the component is
       * transitioned to Exe and the buffer starts moving... */
      if (OMX_TRUE == p_obj->announce_bufs_
          && OMX_ErrorNone != (rc = alloc_buf (p_obj,
                                               &nbytes,
                                               &p_buf, &p_port_priv)))
        {
          TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                         "[%s] : while allocating a buffer.",
                         tiz_err_to_str (rc));
          return rc;
        }

      do
        {
          rc = OMX_UseBuffer (p_obj->thdl_,
                              &p_hdr,
                              p_obj->tpid_, p_port_priv, nbytes, p_buf);

          TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                         "OMX_UseBuffer returned [%s] HEADER [%p] BUFFER [%p]; "
                         "will retry [%s]", tiz_err_to_str (rc), p_hdr,
                         p_hdr->pBuffer,
                         OMX_ErrorIncorrectStateOperation ==
                         rc ? "YES" : "NO");
        }
      while (OMX_ErrorIncorrectStateOperation == rc
             && tiz_sleep (TIZ_USEBUFFER_WAIT_MICROSECONDS) == 0);

      if ((OMX_ErrorNone != rc) || !p_hdr)
        {
          free_buf (p_obj, p_buf, p_port_priv);

          if (p_hdr)
            {
              switch (rc)
                {
                case OMX_ErrorIncorrectStateOperation:
                  {
                    /* Here, the tunnelled component is not ready. Probably, */
                    /* the IL Client has not commanded yet the component to go */
                    /* to OMX_StateIdle.  */

                    /* TODO : retry procedure */
                    TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl),
                                   TIZ_CBUF (ap_hdl),
                                   "OMX_ErrorIncorrectStateOperation received");
                  }
                  break;
                default:
                  {
                    TIZ_LOG_CNAME (TIZ_LOG_ERROR, TIZ_CNAME (ap_hdl),
                                   TIZ_CBUF (ap_hdl),
                                   "OMX_UseBuffer returned [%s]",
                                   tiz_err_to_str (rc));

                  }
                  break;
                };
            }

          if (OMX_ErrorInsufficientResources == rc)
            {
              return OMX_ErrorInsufficientResources;
            }
          /* This is a gotcha. Here there is some problem with the tunnelled */
          /* component. If we return the received error, this component may */
          /* be sending back some error code that is not allowed in */
          /* OMX_SendCommand. Example: The component conformance suite */
          /* expects here OMX_ErrorNone if the tunnelled component does not */
          /* support OMX_UseBuffer or some other problem. Also, we don't send */
          /* and error event here as there's no appropriate error for this */
          /* situation (OMX_ErrorPortUnresponsiveDuringAllocation is for */
          /* non-supplier ports). Therefore, the IL Client should recover */
          /* from this situation after some time by detecting that this */
          /* component didn't transition to OMX_StateIdle. */
          return OMX_ErrorNone;
        }

      /* Fill the data in the received header so we can correctly use it when */
      /* the header is at this side of the tunnel... */
      if (OMX_DirInput == p_obj->portdef_.eDir)
        {
          p_hdr->pInputPortPrivate = p_port_priv;
          p_hdr->nInputPortIndex = p_obj->portdef_.nPortIndex;
        }
      else                      /* OMX_DirOutput == p_obj->portdef_.eDir */
        {
          p_hdr->pOutputPortPrivate = p_port_priv;
          p_hdr->nOutputPortIndex = p_obj->portdef_.nPortIndex;
        }

      /* register this buffer header... */
      if (OMX_ErrorNone != register_header (p_obj, p_hdr, OMX_FALSE))
        {
          free_buf (p_obj, p_buf, p_port_priv);

          return OMX_ErrorInsufficientResources;
        }
    }

  tizport_set_flags (p_obj, 2, EFlagPopulated, EFlagEnabled);
  tizport_clear_flags (p_obj, 1, EFlagBeingEnabled);

  assert (tiz_vector_length (p_obj->p_hdrs_info_) ==
          p_obj->portdef_.nBufferCountActual);

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "port [%d] populated", p_obj->pid_);

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tizport_populate (const void *ap_obj, OMX_HANDLETYPE ap_hdl)
{
  const struct tizport_class *class = classOf (ap_obj);
  assert (class->populate);
  return class->populate (ap_obj, ap_hdl);
}

OMX_ERRORTYPE
tizport_super_populate (const void *a_class, const void *ap_obj,
                        OMX_HANDLETYPE ap_hdl)
{
  const struct tizport_class *superclass = super (a_class);
  assert (ap_obj && superclass->populate);
  return superclass->populate (ap_obj, ap_hdl);
}

static OMX_ERRORTYPE
port_depopulate (const void *ap_obj)
{
  struct tizport *p_obj = (struct tizport *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_U32 nbufs = 0, i = 0;
  OMX_BUFFERHEADERTYPE *p_hdr = NULL;
  OMX_U8 *p_buf = NULL;
  OMX_PTR p_port_priv = 0;

  nbufs = tiz_vector_length (p_obj->p_hdrs_info_);
  assert (nbufs == p_obj->portdef_.nBufferCountActual);
  assert (TIZPORT_IS_TUNNELED_AND_SUPPLIER (p_obj));

  for (i = 0; i < nbufs; ++i)
    {
      p_hdr = deregister_header (p_obj, 0);
      p_buf = p_hdr->pBuffer;
      p_port_priv = OMX_DirInput == p_obj->portdef_.eDir ?
        p_hdr->pInputPortPrivate : p_hdr->pOutputPortPrivate;
/*       p_app_priv = p_hdr->pAppPrivate; */
/*       p_plat_priv = p_hdr->pPlatformPrivate; */

      TIZ_LOG (TIZ_LOG_TRACE, "HEADER [%p]", p_hdr);

      rc = OMX_FreeBuffer (p_obj->thdl_, p_obj->tpid_, p_hdr);

      /* NOTE that we don't check OMX_FreeBuffer returned error here. If */
      /* something wrong happens at the tunnelled component side we'll */
      /* continue here and try to free as many buffers as possible.... at */
      /* least the state of this component will remain valid.... (we don't */
      /* report errors coming from the tunnelled component as that is its */
      /* responsibility....) */
      (void) rc;

      /* At this point, the actual buffer header should no longer exist... */
      p_hdr = 0;

      free_buf (p_obj, p_buf, p_port_priv);
    }

  assert (tiz_vector_length (p_obj->p_hdrs_info_) == 0);

  tizport_clear_flags (p_obj, 1, EFlagPopulated);
  tizport_clear_flags (p_obj, 1, EFlagBeingDisabled);
  tizport_clear_flags (p_obj, 1, EFlagEnabled);

  TIZ_LOG (TIZ_LOG_TRACE, "port [%d] depopulated", p_obj->pid_);

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tizport_depopulate (const void *ap_obj)
{
  const struct tizport_class *class = classOf (ap_obj);
  assert (class->depopulate);
  return class->depopulate (ap_obj);
}

OMX_ERRORTYPE
tizport_super_depopulate (const void *a_class, const void *ap_obj)
{
  const struct tizport_class *superclass = super (a_class);
  assert (ap_obj && superclass->depopulate);
  return superclass->depopulate (ap_obj);
}

static OMX_BOOL
port_check_tunnel_compat (const void *ap_obj,
                          OMX_PARAM_PORTDEFINITIONTYPE * ap_this_def,
                          OMX_PARAM_PORTDEFINITIONTYPE * ap_other_def)
{
  struct tizport *p_obj = (struct tizport *) ap_obj;
  TIZ_LOG (TIZ_LOG_TRACE, "port [%d] check_tunnel_compat", p_obj->pid_);
  /* To be implemented by derived classes */
  return OMX_TRUE;
}

OMX_BOOL
tizport_check_tunnel_compat (const void *ap_obj,
                             OMX_PARAM_PORTDEFINITIONTYPE * ap_this_def,
                             OMX_PARAM_PORTDEFINITIONTYPE * ap_other_def)
{
  const struct tizport_class *class = classOf (ap_obj);
  assert (class->check_tunnel_compat);
  return class->check_tunnel_compat (ap_obj, ap_this_def, ap_other_def);
}

OMX_BOOL
tizport_super_check_tunnel_compat (const void *a_class, const void *ap_obj,
                                   OMX_PARAM_PORTDEFINITIONTYPE * ap_this_def,
                                   OMX_PARAM_PORTDEFINITIONTYPE *
                                   ap_other_def)
{
  const struct tizport_class *superclass = super (a_class);
  assert (ap_obj && superclass->check_tunnel_compat);
  return superclass->check_tunnel_compat (ap_obj, ap_this_def, ap_other_def);
}

static OMX_S32
port_update_claimed_count (void *ap_obj, OMX_S32 a_offset)
{
  struct tizport *p_obj = ap_obj;
  p_obj->claimed_count_ += a_offset;
  TIZ_LOG (TIZ_LOG_TRACE, "port [%d] offset [%d] count [%d]",
           p_obj->pid_, a_offset, p_obj->claimed_count_);
  assert (p_obj->claimed_count_ >= 0);
  assert (p_obj->claimed_count_ <= p_obj->portdef_.nBufferCountActual);
  return p_obj->claimed_count_;
}

OMX_S32
tizport_update_claimed_count (void *ap_obj, OMX_S32 a_offset)
{
  struct tizport_class *class = (struct tizport_class *) classOf (ap_obj);
  assert (class->update_claimed_count);
  return class->update_claimed_count (ap_obj, a_offset);
}

static OMX_ERRORTYPE
port_store_mark (void *ap_obj, const OMX_MARKTYPE * ap_mark_info,
                 OMX_BOOL a_owned)
{
  struct tizport *p_obj = ap_obj;
  tizport_mark_info_t *p_mi = tiz_mem_alloc (sizeof (tizport_mark_info_t));

  assert (ap_mark_info);

  if (!p_mi)
    {
      return OMX_ErrorInsufficientResources;
    }

  p_mi->p_target = ap_mark_info->hMarkTargetComponent;
  p_mi->p_data = ap_mark_info->pMarkData;
  p_mi->owned = a_owned;;

  if (OMX_ErrorNone != tiz_vector_push_back (p_obj->p_marks_, &p_mi))
    {
      tiz_mem_free (p_mi);
      return OMX_ErrorInsufficientResources;
    }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tizport_store_mark (void *ap_obj, const OMX_MARKTYPE * ap_mark_info,
                    OMX_BOOL a_owned)
{
  struct tizport_class *class = (struct tizport_class *) classOf (ap_obj);
  assert (class->store_mark);
  return class->store_mark (ap_obj, ap_mark_info, a_owned);
}

static OMX_ERRORTYPE
port_mark_buffer (void *ap_obj, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  struct tizport *p_obj = ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  const OMX_S32 mark_count = tiz_vector_length (p_obj->p_marks_);

  assert (ap_hdr);

  if (mark_count > 0)
    {
      /* Check for existing marks in the buffer header... */
      if (ap_hdr->hMarkTargetComponent)
        {
          /* Store the current mark in the buffer to give */
          /* preference to older marks */
          OMX_MARKTYPE mark_info = {
            ap_hdr->hMarkTargetComponent,
            ap_hdr->pMarkData
          };
          port_store_mark (p_obj, &mark_info, OMX_FALSE);
        }

      {
        /* Mark the buffer using the first mark in the list... */
        tizport_mark_info_t **pp_mi = NULL, *p_mi = NULL;
        pp_mi = tiz_vector_at (p_obj->p_marks_, 0);
        assert (pp_mi && *pp_mi);
        p_mi = *pp_mi;

        ap_hdr->hMarkTargetComponent = p_mi->p_target;
        ap_hdr->pMarkData = p_mi->p_data;
        rc = p_mi->owned ? OMX_ErrorNone : OMX_ErrorNotReady;

        tiz_mem_free (p_mi);
        tiz_vector_erase (p_obj->p_marks_, 0, 1);
      }

    }
  else
    {
      rc = OMX_ErrorNoMore;
    }

  return rc;
}

OMX_ERRORTYPE
tizport_mark_buffer (void *ap_obj, OMX_BUFFERHEADERTYPE * ap_hdr)
{
  struct tizport_class *class = (struct tizport_class *) classOf (ap_obj);
  assert (class->mark_buffer);
  return class->mark_buffer (ap_obj, ap_hdr);
}

static void
port_set_alloc_hooks (void *ap_obj,
                      const tiz_port_alloc_hooks_t * ap_new_hooks,
                      tiz_port_alloc_hooks_t * ap_old_hooks)
{
  struct tizport *p_obj = ap_obj;

  assert (ap_obj);
  assert (ap_new_hooks);

  if (NULL != ap_old_hooks)
    {
      *ap_old_hooks = p_obj->opts_.mem_hooks;
    }

  p_obj->opts_.mem_hooks = *ap_new_hooks;

}

void
tizport_set_alloc_hooks (void *ap_obj,
                         const tiz_port_alloc_hooks_t * ap_new_hooks,
                         tiz_port_alloc_hooks_t * ap_old_hooks)
{
  struct tizport_class *class = (struct tizport_class *) classOf (ap_obj);
  assert (class->set_alloc_hooks);
  class->set_alloc_hooks (ap_obj, ap_new_hooks, ap_old_hooks);
}

static OMX_ERRORTYPE
port_populate_header (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                      OMX_BUFFERHEADERTYPE * ap_hdr)
{
  struct tizport *p_obj = (struct tizport *) ap_obj;
  OMX_ERRORTYPE rc = OMX_ErrorNone;
  OMX_U32 size = p_obj->portdef_.nBufferSize;
  OMX_U8 *p_buf = 0;
  OMX_PTR p_port_priv = 0;

  assert (ap_obj);
  assert (ap_hdl);
  assert (ap_hdr);

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "port index [%d] announce_bufs [%s]...", p_obj->pid_,
                 p_obj->announce_bufs_ == OMX_TRUE ? "TRUE" : "FALSE");

  if (OMX_TRUE == p_obj->announce_bufs_)
    {
      /* pre-announcements are enabled on this port, therefore nothing to do
       * here. */
      return OMX_ErrorNone;
    }

  /* Call the deallocation hook */
  if (ap_hdr->pBuffer)
    {
      TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                     "Freeing buffer : port index [%d] - "
                     "HEADER [%p] BUFFER [%p]...",
                     p_obj->pid_, ap_hdr, ap_hdr->pBuffer);
      p_port_priv = OMX_DirInput == p_obj->portdef_.eDir ?
        ap_hdr->pInputPortPrivate : ap_hdr->pOutputPortPrivate;
      free_buf (p_obj, ap_hdr->pBuffer, p_port_priv);
      ap_hdr->pBuffer = NULL;
      ap_hdr->nAllocLen = 0;
    }

  /* Call the allocation hook to obtain a new buffer for this header */
  rc = alloc_buf (p_obj, &size, &p_buf, &p_port_priv);

  ap_hdr->nAllocLen = size;
  ap_hdr->pBuffer = p_buf;

  TIZ_LOG_CNAME (TIZ_LOG_TRACE, TIZ_CNAME (ap_hdl), TIZ_CBUF (ap_hdl),
                 "Allocated buffer : port index [%d] - "
                 "HEADER [%p] BUFFER [%p]...",
                 p_obj->pid_, ap_hdr, ap_hdr->pBuffer);

  if (OMX_DirInput == p_obj->portdef_.eDir)
    {
      ap_hdr->pInputPortPrivate = p_port_priv;
    }
  else
    {
      ap_hdr->pOutputPortPrivate = p_port_priv;
    }

  return rc;
}

OMX_ERRORTYPE
tizport_populate_header (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                         OMX_BUFFERHEADERTYPE * ap_hdr)
{
  struct tizport_class *class = (struct tizport_class *) classOf (ap_obj);
  assert (class->populate_header);
  return class->populate_header (ap_obj, ap_hdl, ap_hdr);
}

static void
port_depopulate_header (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                        OMX_BUFFERHEADERTYPE * ap_hdr)
{
  struct tizport *p_obj = (struct tizport *) ap_obj;
  OMX_PTR p_port_priv = 0;

  assert (ap_obj);
  assert (ap_hdr);

  if (OMX_TRUE == p_obj->announce_bufs_)
    {
      /* pre-announcements are enabled on this port, therefore nothing to do
       * here. */
      return;
    }

  /* Call the deallocation hook to release the buffer by this header */
  p_port_priv = OMX_DirInput == p_obj->portdef_.eDir ?
    ap_hdr->pInputPortPrivate : ap_hdr->pOutputPortPrivate;

  free_buf (p_obj, ap_hdr->pBuffer, p_port_priv);
  ap_hdr->pBuffer = NULL;
  ap_hdr->nAllocLen = 0;
  ap_hdr->nFilledLen = 0;
  ap_hdr->nOffset = 0;

}

void
tizport_depopulate_header (const void *ap_obj, OMX_HANDLETYPE ap_hdl,
                           OMX_BUFFERHEADERTYPE * ap_hdr)
{
  struct tizport_class *class = (struct tizport_class *) classOf (ap_obj);
  assert (class->depopulate_header);
  class->depopulate_header (ap_obj, ap_hdl, ap_hdr);
}

static OMX_BOOL
port_is_master_or_slave (const void *ap_obj, OMX_U32 * ap_mos_pid)
{
  const struct tizport *p_obj = ap_obj;

  assert (ap_obj);
  assert (ap_mos_pid);

  TIZ_LOG (TIZ_LOG_TRACE, "PORT [%d] mos [%d]...",
           p_obj->portdef_.nPortIndex, p_obj->opts_.mos_port);

  *ap_mos_pid = p_obj->opts_.mos_port;
  return (*ap_mos_pid != -1 ? OMX_TRUE : OMX_FALSE);
}

OMX_BOOL
tizport_is_master_or_slave (const void *ap_obj, OMX_U32 * ap_mos_pid)
{
  const struct tizport_class *class = classOf (ap_obj);
  assert (class->is_master_or_slave);
  return class->is_master_or_slave (ap_obj, ap_mos_pid);
}

static OMX_ERRORTYPE
port_apply_slaving_behaviour (void *ap_obj, void *ap_mos_port,
                              const OMX_INDEXTYPE a_index,
                              const OMX_PTR ap_struct,
                              tiz_vector_t * ap_changed_idxs)
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tizport_apply_slaving_behaviour (void *ap_obj, void *ap_mos_port,
                                 const OMX_INDEXTYPE a_index,
                                 const OMX_PTR ap_struct,
                                 tiz_vector_t * ap_changed_idxs)
{
  const struct tizport_class *class = classOf (ap_obj);
  assert (class->apply_slaving_behaviour);
  return class->apply_slaving_behaviour (ap_obj, ap_mos_port,
                                         a_index, ap_struct, ap_changed_idxs);
}

OMX_ERRORTYPE
tizport_super_apply_slaving_behaviour (void *a_class, void *ap_obj,
                                       void *ap_mos_port,
                                       const OMX_INDEXTYPE a_index,
                                       const OMX_PTR ap_struct,
                                       tiz_vector_t * ap_changed_idxs)
{
  const struct tizport_class *superclass = super (a_class);
  assert (ap_obj && superclass->apply_slaving_behaviour);
  return superclass->apply_slaving_behaviour (ap_obj, ap_mos_port, a_index,
                                              ap_struct, ap_changed_idxs);
}

/*
 * tizport_class
 */

static void *
port_class_ctor (void *ap_obj, va_list * app)
{
  struct tizport_class *p_obj = super_ctor (tizport_class, ap_obj, app);
  typedef void (*voidf) ();
  voidf selector;
  va_list ap;
  va_copy (ap, *app);

  while ((selector = va_arg (ap, voidf)))
    {
      voidf method = va_arg (ap, voidf);
      if (selector == (voidf) tizport_register_index)
        {
          *(voidf *) & p_obj->register_index = method;
        }
      else if (selector == (voidf) tizport_find_index)
        {
          *(voidf *) & p_obj->find_index = method;
        }
      else if (selector == (voidf) tizport_index)
        {
          *(voidf *) & p_obj->index = method;
        }
      else if (selector == (voidf) tizport_set_index)
        {
          *(voidf *) & p_obj->set_index = method;
        }
      else if (selector == (voidf) tizport_set_portdef_format)
        {
          *(voidf *) & p_obj->set_portdef_format = method;
        }
      else if (selector == (voidf) tizport_buffer_count)
        {
          *(voidf *) & p_obj->buffer_count = method;
        }
      else if (selector == (voidf) tizport_dir)
        {
          *(voidf *) & p_obj->dir = method;
        }
      else if (selector == (voidf) tizport_domain)
        {
          *(voidf *) & p_obj->domain = method;
        }
      else if (selector == (voidf) tizport_get_tunnel_comp)
        {
          *(voidf *) & p_obj->get_tunnel_comp = method;
        }
      else if (selector == (voidf) tizport_get_hdrs_list)
        {
          *(voidf *) & p_obj->get_hdrs_list = method;
        }
      else if (selector == (voidf) tizport_check_flags)
        {
          *(voidf *) & p_obj->check_flags = method;
        }
      else if (selector == (voidf) tizport_set_flags)
        {
          *(voidf *) & p_obj->set_flags = method;
        }
      else if (selector == (voidf) tizport_clear_flags)
        {
          *(voidf *) & p_obj->clear_flags = method;
        }
      else if (selector == (voidf) tizport_populate)
        {
          *(voidf *) & p_obj->populate = method;
        }
      else if (selector == (voidf) tizport_depopulate)
        {
          *(voidf *) & p_obj->depopulate = method;
        }
      else if (selector == (voidf) tizport_check_tunnel_compat)
        {
          *(voidf *) & p_obj->check_tunnel_compat = method;
        }
      else if (selector == (voidf) tizport_update_claimed_count)
        {
          *(voidf *) & p_obj->update_claimed_count = method;
        }
      else if (selector == (voidf) tizport_store_mark)
        {
          *(voidf *) & p_obj->store_mark = method;
        }
      else if (selector == (voidf) tizport_mark_buffer)
        {
          *(voidf *) & p_obj->mark_buffer = method;
        }
      else if (selector == (voidf) tizport_set_alloc_hooks)
        {
          *(voidf *) & p_obj->set_alloc_hooks = method;
        }
      else if (selector == (voidf) tizport_populate_header)
        {
          *(voidf *) & p_obj->populate_header = method;
        }
      else if (selector == (voidf) tizport_depopulate_header)
        {
          *(voidf *) & p_obj->depopulate_header = method;
        }
      else if (selector == (voidf) tizport_is_master_or_slave)
        {
          *(voidf *) & p_obj->is_master_or_slave = method;
        }
      else if (selector == (voidf) tizport_apply_slaving_behaviour)
        {
          *(voidf *) & p_obj->apply_slaving_behaviour = method;
        }

    }

  va_end (ap);
  return p_obj;
}

/*
 * initialization
 */

const void *tizport, *tizport_class;

void
init_tizport (void)
{

  if (!tizport_class)
    {
      init_tizapi ();
      tizport_class = factory_new (tizapi_class,
                                   "tizport_class",
                                   tizapi_class,
                                   sizeof (struct tizport_class),
                                   ctor, port_class_ctor, 0);

    }

  if (!tizport)
    {
      init_tizapi ();
      tizport =
        factory_new
        (tizport_class,
         "tizport",
         tizapi,
         sizeof (struct tizport),
         ctor, port_ctor,
         dtor, port_dtor,
         tizapi_GetParameter, port_GetParameter,
         tizapi_SetParameter, port_SetParameter,
         tizapi_GetConfig, port_GetConfig,
         tizapi_SetConfig, port_SetConfig,
         tizapi_GetExtensionIndex, port_GetExtensionIndex,
         tizapi_ComponentTunnelRequest, port_ComponentTunnelRequest,
         tizapi_UseBuffer, port_UseBuffer,
         tizapi_AllocateBuffer, port_AllocateBuffer,
         tizapi_FreeBuffer, port_FreeBuffer,
         tizport_register_index, port_register_index,
         tizport_find_index, port_find_index,
         tizport_index, port_index,
         tizport_set_index, port_set_index,
         tizport_set_portdef_format, port_set_portdef_format,
         tizport_buffer_count, port_buffer_count,
         tizport_dir, port_dir,
         tizport_domain, port_domain,
         tizport_get_tunnel_comp, port_get_tunnel_comp,
         tizport_get_hdrs_list, port_get_hdrs_list,
         tizport_check_flags, port_check_flags,
         tizport_set_flags, port_set_flags,
         tizport_clear_flags, port_clear_flags,
         tizport_populate, port_populate,
         tizport_depopulate, port_depopulate,
         tizport_check_tunnel_compat, port_check_tunnel_compat,
         tizport_update_claimed_count, port_update_claimed_count,
         tizport_store_mark, port_store_mark,
         tizport_mark_buffer, port_mark_buffer,
         tizport_set_alloc_hooks, port_set_alloc_hooks,
         tizport_populate_header, port_populate_header,
         tizport_depopulate_header, port_depopulate_header,
         tizport_is_master_or_slave, port_is_master_or_slave,
         tizport_apply_slaving_behaviour, port_apply_slaving_behaviour, 0);
    }
}
