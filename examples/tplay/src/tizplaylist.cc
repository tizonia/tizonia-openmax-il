/**
 * Copyright (C) 2011-2014 Aratelia Limited - Juan A. Rubio
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
 * @file   tizplaylist.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Playlist utility
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <tizosal.h>

#include "tizplaylist.h"

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.playlist"
#endif

namespace  // unnamed namespace
{
  struct pathname_of
  {
    pathname_of (uri_lst_t &uri_list) : uri_list_ (uri_list)
    {
    }

    void operator()(const boost::filesystem::directory_entry &p) const
    {
      uri_list_.push_back (p.path ().string ());
    }
    uri_lst_t &uri_list_;
  };

  OMX_ERRORTYPE
  process_base_uri (const std::string &uri, uri_lst_t &uri_list,
                    bool recurse = false)
  {
    if (boost::filesystem::exists (uri)
        && boost::filesystem::is_regular_file (uri))
    {
      uri_list.push_back (uri);
      return OMX_ErrorNone;
    }

    if (boost::filesystem::exists (uri)
        && boost::filesystem::is_directory (uri))
    {
      if (!recurse)
      {
        std::for_each (boost::filesystem::directory_iterator (
                           boost::filesystem::path (uri)),
                       boost::filesystem::directory_iterator (),
                       pathname_of (uri_list));
        return uri_list.empty () ? OMX_ErrorContentURIError : OMX_ErrorNone;
      }
      else
      {
        std::for_each (boost::filesystem::recursive_directory_iterator (
                           boost::filesystem::path (uri)),
                       boost::filesystem::recursive_directory_iterator (),
                       pathname_of (uri_list));
        return uri_list.empty () ? OMX_ErrorContentURIError : OMX_ErrorNone;
      }
    }

    return OMX_ErrorContentURIError;
  }

  OMX_ERRORTYPE
  filter_unknown_media (const file_extension_lst_t &extension_list,
                        uri_lst_t &uri_list,
                        file_extension_lst_t &extension_list_filtered)
  {
    uri_lst_t::iterator it (uri_list.begin ());
    std::vector< uri_lst_t::iterator > iter_list;
    file_extension_lst_t ext_lst_cpy (extension_list);
    uri_lst_t filtered_uri_list;

    while (it != uri_list.end ())
    {
      std::string extension (
          boost::filesystem::path (*it).extension ().string ());
      boost::algorithm::to_lower (extension);
      file_extension_lst_t::const_iterator low = std::lower_bound (
          ext_lst_cpy.begin (), ext_lst_cpy.end (), extension);

      TIZ_LOG (TIZ_PRIORITY_TRACE, "ext [%s] it [%s]", extension.c_str (),
               (*it).c_str ());

      if (!(low == ext_lst_cpy.end ()) && boost::iequals ((*low), extension))
      {
        TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] added to playlist low [%s]",
                 (*it).c_str (), (*low).c_str ());
        filtered_uri_list.push_back (*it);
        extension_list_filtered.insert (extension);
      }
      ++it;
    }

    uri_list = filtered_uri_list;
    TIZ_LOG (TIZ_PRIORITY_TRACE, "%d elements in playlist", uri_list.size ());
    return uri_list.empty () ? OMX_ErrorContentURIError : OMX_ErrorNone;
  }

}  // unnamed namespace

//
// playlist
//
tiz::playlist::playlist (const uri_lst_t &uri_list /* = uri_lst_t () */)
  : uri_list_ (uri_list), current_index_ (0), is_single_format_ (Unknown)
{
  TIZ_LOG (TIZ_PRIORITY_ERROR, "uri length [%d]", uri_list_.size ());
}

tiz::playlist::playlist (const playlist &copy_from)
  : uri_list_ (copy_from.uri_list_),
    current_index_ (copy_from.current_index_),
    is_single_format_ (copy_from.is_single_format_)
{
  TIZ_LOG (TIZ_PRIORITY_ERROR, "uri length [%d]", uri_list_.size ());
}

bool tiz::playlist::assemble_play_list (
    const std::string &base_uri, const bool shuffle_playlist,
    const bool recurse, const file_extension_lst_t &extension_list,
    uri_lst_t &uri_list, std::string &error_msg)
{
  bool list_assembled = false;
  file_extension_lst_t extension_list_filtered;

  try
  {
    if (base_uri.empty ())
    {
      error_msg.assign ("Empty media uri.");
      goto end;
    }

    if (OMX_ErrorNone != process_base_uri (base_uri, uri_list, recurse))
    {
      error_msg.assign ("File not found.");
      goto end;
    }

    if (OMX_ErrorNone != filter_unknown_media (extension_list, uri_list,
                                               extension_list_filtered))
    {
      error_msg.assign ("No supported media types found.");
      goto end;
    }

    if (shuffle_playlist)
    {
      std::random_shuffle (uri_list.begin (), uri_list.end ());
    }
    else
    {
      std::sort (uri_list.begin (), uri_list.end ());
    }

    list_assembled = true;
  }
  catch (std::exception const &e)
  {
    error_msg.assign (e.what ());
  }
  catch (...)
  {
    error_msg.assign ("Undefined file system error.");
  }

end:

  if (!list_assembled)
  {
    TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s]", error_msg.c_str ());
  }
  else
  {
#define KNRM "\x1B[0m"
#define KBLU "\x1B[34m"
    fprintf (
        stdout, "%sPlaylist length: %lu. File extensions in playlist: %s%s\n\n",
        KBLU, (long)uri_list.size (),
        boost::algorithm::join (extension_list_filtered, ", ").c_str (), KNRM);
  }

  return list_assembled;
}

void tiz::playlist::skip (const int jump)
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "jump [%d] current_index_ [%d]...", jump,
           current_index_);
  current_index_ += jump;

  if (current_index_ < 0)
  {
    current_index_ = uri_list_.size () - abs (current_index_);
  }

  if (current_index_ >= uri_list_.size ())
  {
    current_index_ %= uri_list_.size ();
  }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "jump [%d] new current_index_ [%d]...", jump,
           current_index_);
}

const std::string &tiz::playlist::get_current_uri () const
{
  TIZ_LOG (TIZ_PRIORITY_TRACE, "uri list size [%d] current_index_ [%d]...",
           uri_list_.size (), current_index_);
  assert (current_index_ >= 0 && current_index_ < uri_list_.size ());
  return uri_list_[current_index_];
}

tiz::playlist tiz::playlist::find_next_sub_playlist ()
{
  if (is_single_format ())
  {
    return playlist (get_uri_list ());
  }
  else
  {
    const size_t list_size = uri_list_.size ();
    assert (current_index_ < list_size);
    uri_lst_t new_uri_list;
    std::string current_extension (
        boost::filesystem::path (uri_list_[current_index_])
            .extension ()
            .string ());
    for (; current_index_ < list_size; ++current_index_)
    {
      std::string extension (boost::filesystem::path (uri_list_[current_index_])
                                 .extension ()
                                 .string ());
      if (extension.compare (current_extension) == 0)
      {
        TIZ_LOG (TIZ_PRIORITY_TRACE, "Adding %s",
                 uri_list_[current_index_].c_str ());
        new_uri_list.push_back (uri_list_[current_index_]);
      }
      else
      {
        break;
      }
    }

    if (current_index_ >= list_size)
    {
      current_index_ = 0;
    }

    playlist new_list (new_uri_list);
    assert (new_list.is_single_format ());
    return new_list;
  }
}

tiz::playlist tiz::playlist::find_previous_sub_playlist ()
{
  if (is_single_format ())
  {
    return playlist (get_uri_list ());
  }
  else
  {
    const size_t list_size = uri_list_.size ();
    assert (current_index_ < list_size);
    uri_lst_t new_uri_list;
    std::string current_extension (
        boost::filesystem::path (uri_list_[current_index_])
            .extension ()
            .string ());
    for (; current_index_ >= 0; --current_index_)
    {
      std::string extension (boost::filesystem::path (uri_list_[current_index_])
                                 .extension ()
                                 .string ());
      if (extension.compare (current_extension) == 0)
      {
        TIZ_LOG (TIZ_PRIORITY_TRACE, "Adding %s",
                 uri_list_[current_index_].c_str ());
        // Insert at the front
        new_uri_list.push_front (uri_list_[current_index_]);
      }
      else
      {
        break;
      }
    }

    if (current_index_ < 0)
    {
      current_index_ = 0;
    }

    playlist new_list (new_uri_list);
    assert (new_list.is_single_format ());
    return new_list;
  }
}

uri_lst_t tiz::playlist::get_sublist (const int from, const int to) const
{
  uri_lst_t new_list;
  if (from >= 0 && to < uri_list_.size () && from < to)
  {
    // .assign: Replaces the vector contents with copies of those in the
    // range [first, last)
    new_list.assign (uri_list_.begin () + from, uri_list_.begin () + to);
  }
  return new_list;
}

const uri_lst_t &tiz::playlist::get_uri_list () const
{
  return uri_list_;
}

int tiz::playlist::size () const
{
  return uri_list_.size ();
}

bool tiz::playlist::is_single_format () const
{
  if (Unknown == is_single_format_)
  {
    is_single_format_ = Yes;

    const size_t list_size = uri_list_.size ();
    std::string current_extension (
        boost::filesystem::path (uri_list_[0]).extension ().string ());
    for (int i = 0; i < list_size; ++i)
    {
      std::string extension (
          boost::filesystem::path (uri_list_[i]).extension ().string ());
      if (extension.compare (current_extension) != 0)
      {
        is_single_format_ = No;
        break;
      }
    }
  }

  assert (Yes == is_single_format_ || No == is_single_format_);

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Is single format? [%s]",
           is_single_format_ == Yes ? "YES" : "NO");

  return (is_single_format_ == Yes);
}

bool tiz::playlist::is_last_uri () const
{
  assert (uri_list_.size () > 0);
  return current_index_ == (uri_list_.size () - 1);
}
