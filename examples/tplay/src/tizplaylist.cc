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
 * @file   tizplaylist.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Playlist utility class
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "tizplaylist.h"

#include <tizosal.h>

#include <boost/filesystem.hpp>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.play.playlist"
#endif

namespace                       // unnamed namespace
{
  struct pathname_of
  {
    pathname_of (uri_list_t &file_list):file_list_ (file_list)
    {
    }

    void operator () (const boost::filesystem::directory_entry & p) const
    {
      file_list_.push_back (p.path ().string ());
    }
    uri_list_t &file_list_;
  };

  bool
  verify_uri (const std::string & uri)
  {
    return (boost::filesystem::portable_directory_name (uri) ||
            boost::filesystem::portable_file_name (uri));
  }

  OMX_ERRORTYPE
  process_base_uri (const std::string & uri, uri_list_t &file_list, bool recurse = false)
  {
    if (boost::filesystem::exists (uri)
        && boost::filesystem::is_regular_file (uri))
      {
        file_list.push_back (uri);
        return OMX_ErrorNone;
      }

    if (boost::filesystem::exists (uri)
        && boost::filesystem::is_directory (uri))
      {
        if (!recurse)
          {
            std::for_each (boost::filesystem::directory_iterator
                           (boost::filesystem::path (uri)),
                           boost::filesystem::directory_iterator (),
                           pathname_of (file_list));
            return file_list.empty () ? OMX_ErrorContentURIError : OMX_ErrorNone;
          }
        else
          {
            std::for_each (boost::filesystem::recursive_directory_iterator
                           (boost::filesystem::path (uri)),
                           boost::filesystem::recursive_directory_iterator (),
                           pathname_of (file_list));
            return file_list.empty () ? OMX_ErrorContentURIError : OMX_ErrorNone;
          }
      }

    return OMX_ErrorContentURIError;
  }

  OMX_ERRORTYPE
  filter_unknown_media (uri_list_t &file_list)
  {
    uri_list_t::iterator it = file_list.begin ();
    while (it != file_list.end ())
      {
        std::string extension (boost::filesystem::path (*it).extension ().
                               string ());
        if (extension.compare (".mp3") != 0
            &&
            extension.compare (".opus") != 0)
          //&&
          // extension.compare (".ivf") != 0)
          {
            file_list.erase (it);
            // Restart the loop
            it = file_list.begin ();
          }
        else
          {
            ++it;
          }
      }

    return file_list.empty () ? OMX_ErrorContentURIError : OMX_ErrorNone;
  }

} // unnamed namespace

//
// tizplaylist
// 
tizplaylist::tizplaylist(const uri_list_t &uri_list /* = uri_list_t () */ )
  : uri_list_ (uri_list), current_index_ (0), single_format_list_ (Unknown)
{
}

tizplaylist::tizplaylist(const tizplaylist &playlist, const int from, const int to)
  : uri_list_ (), current_index_ (0), single_format_list_ (Unknown)
{
  // TODO:
}

tizplaylist
tizplaylist::get_next_sub_playlist ()
{
  size_t list_size = uri_list_.size ();

  if (is_single_format_playlist ())
    {
      return tizplaylist (get_uri_list ());
    }
  else
    {
      assert (current_index_ < list_size);
      uri_list_t new_uri_list;
      std::string current_extension
        (boost::filesystem::path
         (uri_list_[current_index_]).extension (). string ());
      for ( ; current_index_ < list_size ; ++current_index_)
        {
          std::string extension (boost::filesystem::path
                                 (uri_list_[current_index_]).extension ().
                                 string ());
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

      assert (tizplaylist (new_uri_list).is_single_format_playlist ());

      return tizplaylist (new_uri_list);
    }
}

uri_list_t
tizplaylist::get_sublist (const int from, const int to) const
{
  uri_list_t new_list;
  if (from >= 0 && to < uri_list_.size () && from < to)
    {
      // .assign: Replaces the vector contents with copies of those in the
      // range [first, last)
      new_list.assign (uri_list_.begin () + from, uri_list_.begin () + to);
    }
  return new_list;
}

const uri_list_t &
tizplaylist::get_uri_list () const
{
  return uri_list_;
}

int
tizplaylist::size () const
{
  return uri_list_.size ();
}

bool
tizplaylist::is_single_format_playlist () const
{
  bool is_single_format = true;

  if (single_format_list_ == Unknown)
    {
      size_t list_size = uri_list_.size ();
      std::string current_extension
        (boost::filesystem::path (uri_list_[0]).extension (). string ());
      for (int i = 0; i < list_size ; ++i)
        {
          std::string extension (boost::filesystem::path
                                 (uri_list_[i]).extension ().
                                 string ());
          if (extension.compare (current_extension) != 0)
            {
              is_single_format = false;
              break;
            }
        }

      single_format_list_ = (is_single_format ? Yes : No);
    }
  else if (single_format_list_ == No)
    {
      is_single_format = false;
    }

  TIZ_LOG (TIZ_PRIORITY_TRACE, "Is single format? [%s]",
           is_single_format ? "YES" : "NO");

  return is_single_format;
}

OMX_ERRORTYPE
tizplaylist::assemble_play_list (const std::string &base_uri,
                                 const bool shuffle_playlist,
                                 const bool recurse,
                                 uri_list_t &file_list)
{
  if (!verify_uri (base_uri))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "Invalid base uri.");
      return OMX_ErrorContentURIError;
    }

  if (OMX_ErrorNone != process_base_uri (base_uri, file_list, recurse))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "File not found.");
      return OMX_ErrorContentURIError;
    }

  if (OMX_ErrorNone != filter_unknown_media (file_list))
    {
      TIZ_LOG (TIZ_PRIORITY_ERROR, "No supported media types found.");
      return OMX_ErrorContentURIError;
    }

  if (shuffle_playlist)
    {
      std::random_shuffle (file_list.begin (), file_list.end ());
    }
  else
    {
      std::sort (file_list.begin (), file_list.end ());
    }

  return OMX_ErrorNone;
}
