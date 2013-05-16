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
 * @file   tizgraph.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  OpenMAX IL graph base class impl
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizmp3graph.h"
#include "tizprobe.h"
#include "tizosal.h"

#include "OMX_Core.h"
#include "OMX_Component.h"

#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>


#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.graph.mp3"
#endif

tizmp3graph::tizmp3graph (tizprobe_ptr_t probe_ptr)
  : tizgraph (3, probe_ptr)
{
}

OMX_ERRORTYPE
tizmp3graph::do_load ()
{
  OMX_ERRORTYPE ret = OMX_ErrorNone;

  component_names_t comp_list;
  comp_list.push_back ("OMX.Aratelia.file_reader.binary");
  comp_list.push_back ("OMX.Aratelia.audio_decoder.mp3");
  comp_list.push_back ("OMX.Aratelia.audio_renderer.pcm");

  if (OMX_ErrorNone != (ret = verify_existence (comp_list)))
    {
      return ret;
    }

  component_roles_t role_list;
  role_list.push_back ("audio_reader.binary");
  role_list.push_back ("audio_decoder.mp3");
  role_list.push_back ("audio_renderer.pcm");

  if (OMX_ErrorNone != (ret = verify_role_list (comp_list, role_list)))
    {
      return ret;
    }

  if (OMX_ErrorNone != (ret = instantiate_list (comp_list)))
    {
      return ret;
    }

  return ret;
}

OMX_ERRORTYPE
tizmp3graph::configure_mp3_graph (const int file_index)
{
  OMX_ERRORTYPE ret = OMX_ErrorNone;

  TIZ_LOG (TIZ_TRACE, "Configure current_file_index_ [%d]...",
           current_file_index_);

  assert (file_index < file_list_.size ());
  assert (OMX_StateLoaded == current_graph_state_);
 
  const std::string &uri = file_list_[file_index];
  
  if (!uri.empty ())
    {
      // Probe the new uri
      probe_ptr_.reset ();
      probe_ptr_ = boost::make_shared < tizprobe > (uri);
      if (probe_ptr_->get_omx_domain () != OMX_PortDomainAudio
          || probe_ptr_->get_audio_coding_type () != OMX_AUDIO_CodingMP3)
        {
          return OMX_ErrorContentURIError;
        }
    }

  // Set the new URI
  OMX_PARAM_CONTENTURITYPE *p_uritype = NULL;

  if (NULL == (p_uritype = (OMX_PARAM_CONTENTURITYPE *) tiz_mem_calloc
               (1, sizeof (OMX_PARAM_CONTENTURITYPE)
                + OMX_MAX_STRINGNAME_SIZE)))
    {
      return OMX_ErrorInsufficientResources;
    }

  p_uritype->nSize = sizeof (OMX_PARAM_CONTENTURITYPE)
    + OMX_MAX_STRINGNAME_SIZE;
  p_uritype->nVersion.nVersion = OMX_VERSION;

  strncpy ((char *) p_uritype->contentURI, probe_ptr_->get_uri ().c_str (),
           OMX_MAX_STRINGNAME_SIZE);
  p_uritype->contentURI[strlen (probe_ptr_->get_uri ().c_str ())] = '\0';

  ret = OMX_SetParameter (handles_[0], OMX_IndexParamContentURI, p_uritype);

  tiz_mem_free (p_uritype);
  p_uritype = NULL;

  if (OMX_ErrorNone != ret)
    {
      return ret;
    }

  // Obtain the mp3 settings from the decoder's port #0
  OMX_AUDIO_PARAM_MP3TYPE mp3type, mp3type_orig;;

  mp3type.nSize             = sizeof (OMX_AUDIO_PARAM_MP3TYPE);
  mp3type.nVersion.nVersion = OMX_VERSION;
  mp3type.nPortIndex        = 0;

  tiz_check_omx_err (OMX_GetParameter (handles_[1], OMX_IndexParamAudioMp3,
                                       &mp3type));
  mp3type_orig = mp3type;
  
  // Set the mp3 settings on decoder's port #0

  probe_ptr_->get_mp3_codec_info (mp3type);
  mp3type.nPortIndex = 0;
  tiz_check_omx_err (OMX_SetParameter (handles_[1], OMX_IndexParamAudioMp3,
                                       &mp3type));

  if (mp3type_orig.nSampleRate != mp3type.nSampleRate
      || mp3type_orig.nChannels != mp3type.nChannels)
    {
      // Await port settings change event on decoders's port #1
      waitevent_list_t event_list
        (1,
         waitevent_info
         (handles_[1],
          OMX_EventPortSettingsChanged,
          1,     //nData1
          (OMX_U32) OMX_IndexParamAudioPcm,       //nData2
          NULL));
      cback_handler_.wait_for_event_list (event_list);
    }

  // Set the pcm settings on renderer's port #0
  OMX_AUDIO_PARAM_PCMMODETYPE pcmtype;

  probe_ptr_->get_pcm_codec_info (pcmtype);
  pcmtype.nSize = sizeof (OMX_AUDIO_PARAM_PCMMODETYPE);
  pcmtype.nVersion.nVersion = OMX_VERSION;
  pcmtype.nPortIndex = 0;

  tiz_check_omx_err (OMX_SetParameter (handles_[2], OMX_IndexParamAudioPcm,
                                       &pcmtype));

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tizmp3graph::do_configure (const tizgraphconfig_ptr_t config)
{
  OMX_ERRORTYPE ret = OMX_ErrorNone;

  file_list_ = config->get_uris ();
  current_file_index_ = 0;

  tiz_check_omx_err (setup_suppliers ());
  tiz_check_omx_err (setup_tunnels ());

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tizmp3graph::do_execute ()
{
  TIZ_LOG (TIZ_TRACE, "Configure current_file_index_ [%d] list size [%d]...",
           current_file_index_, file_list_.size ());

  assert (OMX_StateLoaded == current_graph_state_);

  if (current_file_index_ < file_list_.size ())
    {
      tiz_check_omx_err (configure_mp3_graph (current_file_index_++));
      tiz_check_omx_err (transition_all (OMX_StateIdle, OMX_StateLoaded));
      tiz_check_omx_err (transition_all (OMX_StateExecuting, OMX_StateIdle));
    }

  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tizmp3graph::do_pause ()
{
  if (OMX_StateExecuting == current_graph_state_)
    {
      return transition_all (OMX_StatePause, OMX_StateExecuting);
    }
  else if (OMX_StatePause == current_graph_state_)
    {
      return transition_all (OMX_StateExecuting, OMX_StatePause);
    }
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tizmp3graph::do_seek ()
{
  return OMX_ErrorNone;
}

OMX_ERRORTYPE
tizmp3graph::do_skip (const int jump)
{
  tiz_check_omx_err (transition_all (OMX_StateIdle, OMX_StateExecuting));
  tiz_check_omx_err (transition_all (OMX_StateLoaded, OMX_StateIdle));

  current_file_index_ += jump;

  if (jump > 0)
    {
      current_file_index_--;
    }

  if (current_file_index_ < 0)
    {
      current_file_index_ = 0;
    }

  if (current_file_index_ > file_list_.size ())
    {
      current_file_index_ = file_list_.size ();
    }

  TIZ_LOG (TIZ_TRACE, "Configure current_file_index_ [%d]...",
           current_file_index_);

  return do_execute ();
}

OMX_ERRORTYPE
tizmp3graph::do_volume ()
{
  return OMX_ErrorNone;
}

void
tizmp3graph::do_unload ()
{
  (void) transition_all (OMX_StateIdle, OMX_StateExecuting);
  (void) transition_all (OMX_StateLoaded, OMX_StateIdle);
  tear_down_tunnels ();
  destroy_list ();
}

void
tizmp3graph::do_eos (const OMX_HANDLETYPE handle)
{
  if (handle == handles_[2])
    {
      (void) transition_all (OMX_StateIdle, OMX_StateExecuting);
      (void) transition_all (OMX_StateLoaded, OMX_StateIdle);
      (void) do_execute ();
    }
}
