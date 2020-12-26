/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio and contributors
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
 * @file   tizplaylist.cpp
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
#include <cmath>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/system/error_code.hpp>

#include <tizplatform.h>

#include "tizplaylist.hpp"

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

    void operator() (const boost::filesystem::directory_entry &p) const
    {
      uri_list_.push_back (p.path ().string ());
    }
    uri_lst_t &uri_list_;
  };

  void add_to_extension_list (file_extension_lst_t &list,
                              const std::string &extension)
  {
    list.insert (list.end (), extension);
  }

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
tiz::playlist::playlist (const uri_lst_t &uri_list /* = uri_lst_t () */,
                         const bool shuffle /* = false */)
  : uri_list_ (uri_list),
    current_position_ (0),
    loop_playback_ (false),
    sub_list_positions_ (),
    current_sub_list_ (-1),
    shuffle_ (shuffle),
    extension_list_ (),
    single_format_ (Unknown)
{
  const int list_size = uri_list_.size ();
  if (list_size)
  {
    TIZ_LOG (TIZ_PRIORITY_TRACE, "first uri [%s]", uri_list_[0].c_str ());
    TIZ_LOG (TIZ_PRIORITY_TRACE, "last list [%s]",
             uri_list_[uri_list_.size () - 1].c_str ());
  }
  scan_list ();
}

tiz::playlist::playlist (const playlist &copy_from)
  : uri_list_ (copy_from.uri_list_),
    current_position_ (copy_from.current_position_),
    loop_playback_ (copy_from.loop_playback_),
    sub_list_positions_ (copy_from.sub_list_positions_),
    current_sub_list_ (copy_from.current_sub_list_),
    shuffle_ (copy_from.shuffle_),
    extension_list_ (copy_from.extension_list_),
    single_format_ (copy_from.single_format_)
{
  const int list_size = uri_list_.size ();
  TIZ_LOG (TIZ_PRIORITY_TRACE, "uri list size [%d]", list_size);
  if (list_size)
  {
    TIZ_LOG (TIZ_PRIORITY_TRACE, "first uri [%s]", uri_list_[0].c_str ());
    TIZ_LOG (TIZ_PRIORITY_TRACE, "last list [%s]",
             uri_list_[uri_list_.size () - 1].c_str ());
  }
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

    boost::system::error_code errcode;
    std::string canonical_base_uri
        = boost::filesystem::canonical (base_uri, errcode).string ();
    if (errcode.value () != 0)
    {
      error_msg.assign (errcode.message ());
      goto end;
    }

    if (OMX_ErrorNone
        != process_base_uri (canonical_base_uri, uri_list, recurse))
    {
      error_msg.assign ("File not found.");
      goto end;
    }

    if (OMX_ErrorNone
        != filter_unknown_media (extension_list, uri_list,
                                 extension_list_filtered))
    {
      error_msg.assign ("No supported media types found.");
      goto end;
    }

    if (shuffle_playlist)
    {
      std::srand (time (nullptr));
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

  return list_assembled;
}

void tiz::playlist::skip (const int jump)
{
  const int list_size = uri_list_.size ();
  TIZ_LOG (TIZ_PRIORITY_TRACE,
           "jump [%d] current_position_ [%d]"
           " loop_playback [%s]",
           jump, current_position_, loop_playback_ ? "YES" : "NO");
  current_position_ += jump;

  if (loop_playback ())
  {
    if (current_position_ < 0)
    {
      current_position_ = list_size - abs (current_position_);
    }
    else if (current_position_ >= list_size)
    {
      current_position_ %= list_size;
    }
  }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "jump [%d] new position [%d]... [%s]", jump,
           current_position_,
           current_position_ < list_size && current_position_ >= 0
               ? uri_list_[current_position_].c_str ()
               : "");
}

const std::string &tiz::playlist::get_current_uri () const
{
  const int list_size = uri_list_.size ();
  TIZ_LOG (TIZ_PRIORITY_TRACE, "uri list size [%d] current_position_ [%d]...",
           list_size, current_position_);
  assert (current_position_ >= 0 && current_position_ < list_size);
  return uri_list_[current_position_];
}

tiz::playlist tiz::playlist::obtain_next_sub_playlist (
    const list_direction_t up_or_down)
{
  if (uri_list_.empty () || single_format ())
  {
    return playlist (get_uri_list ());
  }
  else
  {
    assert (up_or_down < DirMax);
    if (up_or_down == DirUp)
    {
      const int sub_lists = sub_list_positions_.size () - 1;
      current_sub_list_++;
      if (current_sub_list_ >= sub_lists)
      {
        current_sub_list_ = 0;
      }
    }
    else
    {
      if (current_sub_list_ <= 0)
      {
        current_sub_list_ = sub_list_positions_.size () - 1;
      }
      --current_sub_list_;
    }

    size_t position1 = sub_list_positions_[current_sub_list_];
    size_t position2 = sub_list_positions_[current_sub_list_ + 1];
    uri_lst_t::const_iterator first = uri_list_.begin () + position1;
    uri_lst_t::const_iterator last = uri_list_.begin () + position2;

    TIZ_LOG (TIZ_PRIORITY_TRACE,
             "current_sub_list_ [%d] position1 [%d] position2_ [%d]...",
             current_sub_list_, position1, position2);

    playlist new_list (uri_lst_t (first, last));
    assert (new_list.single_format ());
    current_position_ = position1;

    return new_list;
  }
}

uri_lst_t tiz::playlist::get_sublist (const int from, const int to) const
{
  const int list_size = uri_list_.size ();
  uri_lst_t new_list;
  if (from >= 0 && to < list_size && from < to)
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

int tiz::playlist::current_position () const
{
  return current_position_;
}

int tiz::playlist::size () const
{
  return uri_list_.size ();
}

bool tiz::playlist::empty () const
{
  return uri_list_.empty ();
}

bool tiz::playlist::single_format () const
{
  if (!uri_list_.empty ())
  {
    if (Unknown == single_format_)
    {
      single_format_ = Yes;
      try
      {
        const int list_size = uri_list_.size ();
        std::string current_extension (
            boost::filesystem::path (uri_list_[0]).extension ().string ());
        boost::algorithm::to_lower (current_extension);

        for (int i = 0; i < list_size; ++i)
        {
          std::string extension (
              boost::filesystem::path (uri_list_[i]).extension ().string ());
          boost::algorithm::to_lower (extension);
          if (extension.compare (current_extension) != 0)
          {
            single_format_ = No;
            break;
          }
        }
        assert (Yes == single_format_ || No == single_format_);

        TIZ_LOG (TIZ_PRIORITY_TRACE, "Is single format? [%s]",
                 single_format_ == Yes ? "YES" : "NO");
      }
      catch (std::exception const &e)
      {
        TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s]", e.what ());
      }
      catch (...)
      {
        TIZ_LOG (TIZ_PRIORITY_ERROR, "[Unknown exception]");
      }
    }
  }
  return (single_format_ == Yes);
}

bool tiz::playlist::before_begin () const
{
  bool before_begin = (current_position_ < 0);
  TIZ_LOG (TIZ_PRIORITY_TRACE, "current_position_ [%d] before begin? [%s]",
           current_position_, before_begin ? "YES" : "NO");
  return before_begin;
}

bool tiz::playlist::past_end () const
{
  const int list_size = uri_list_.size ();
  bool past_end = false;
  if (current_position_ >= list_size)
  {
    past_end = true;
  }
  TIZ_LOG (TIZ_PRIORITY_TRACE,
           "current_position_ [%d] uri_list_.size () [%d] past end? [%s]",
           current_position_, list_size, past_end ? "YES" : "NO");
  return past_end;
}

bool tiz::playlist::loop_playback () const
{
  return loop_playback_;
}

void tiz::playlist::set_loop_playback (const bool loop_playback)
{
  loop_playback_ = loop_playback;
}

bool tiz::playlist::shuffle () const
{
  return shuffle_;
}

int tiz::playlist::position () const
{
  return current_position_;
}

void tiz::playlist::set_position (const int position)
{
  if (!uri_list_.empty ())
  {
    const int list_size = size ();
    int capped_position = position;
    if (capped_position >= list_size)
    {
      capped_position %= list_size;
    }
    else if (capped_position < 0)
    {
      capped_position = list_size - abs (capped_position);
    }

    assert (capped_position >= 0 && capped_position < list_size);
    TIZ_LOG (TIZ_PRIORITY_TRACE,
             "current_position_ [%d] position [%d] new position [%d]",
             current_position_, position, capped_position);
    current_position_ = capped_position;
  }
}

void tiz::playlist::erase_uri (const int position)
{
  const int list_size = size ();
  assert (position < list_size);
  if (position < list_size)
  {
    uri_list_.erase (uri_list_.begin () + position);
  }
}

void tiz::playlist::scan_list ()
{
  if (!uri_list_.empty ())
  {
    int position = 0;
    while (position < size ())
    {
      TIZ_LOG (TIZ_PRIORITY_TRACE, "new sub list at position [%d]", position);
      sub_list_positions_.push_back (position);
      position = find_next_sub_list (position);
    }
    // For convenience, push one more position, a "last" position...
    sub_list_positions_.push_back (uri_list_.size ());

    // Find out whether this is a single-format playlist.
    (void)single_format ();
  }
}

int tiz::playlist::find_next_sub_list (const int position) const
{
  const int list_size = uri_list_.size ();
  int cur_pos = position;
  assert (cur_pos < list_size);

  try
  {
    std::string current_extension (
        boost::filesystem::path (uri_list_[cur_pos]).extension ().string ());
    boost::algorithm::to_lower (current_extension);

    add_to_extension_list (extension_list_, current_extension);

    for (; cur_pos < list_size; ++cur_pos)
    {
      std::string extension (
          boost::filesystem::path (uri_list_[cur_pos]).extension ().string ());
      boost::algorithm::to_lower (extension);
      if (!extension.compare (current_extension) == 0)
      {
        break;
      }
    }
  }
  catch (std::exception const &e)
  {
    cur_pos = list_size;
    TIZ_LOG (TIZ_PRIORITY_ERROR, "[%s]", e.what ());
  }
  catch (...)
  {
    cur_pos = list_size;
    TIZ_LOG (TIZ_PRIORITY_ERROR, "[Unknown exception]");
  }

  return cur_pos;
}

void tiz::playlist::print_info ()
{
  TIZ_PRINTF_C11 (
      "[Playlist] [Tracks in queue] '%lu'. (File extensions in queue: %s)",
      (long)uri_list_.size (),
      boost::algorithm::join (extension_list_, ", ").c_str ());
}

void tiz::playlist::print_contents () const
{
  unsigned int i = 1;
  BOOST_FOREACH (std::string uri, uri_list_)
  {
    boost::filesystem::path p (uri);
    TIZ_PRINTF_C11 ("[Playlist] [Track] [#%d] '%s'", i++,
                    p.filename ().c_str ());
  }
}
