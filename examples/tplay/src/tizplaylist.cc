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
#include <boost/algorithm/string.hpp>

#include <algorithm>

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

  OMX_ERRORTYPE
  process_base_uri (const std::string & uri, uri_list_t &file_list,
                      bool recurse = false)
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
  filter_unknown_media (const extension_list_t &extension_list,
                        uri_list_t &file_list, extension_list_t &extension_list_filtered)
  {
    uri_list_t::iterator it (file_list.begin ());
    std::vector<uri_list_t::iterator> iter_list;
    extension_list_t ext_lst_cpy (extension_list);
    uri_list_t filtered_file_list;

    while (it != file_list.end ())
      {
        std::string extension (boost::filesystem::path (*it).extension ().
                               string ());
        boost::algorithm::to_lower(extension);
        extension_list_t::const_iterator low
          = std::lower_bound (ext_lst_cpy.begin(),
                              ext_lst_cpy.end(), extension);

        TIZ_LOG (TIZ_PRIORITY_TRACE, "ext [%s] it [%s]",
                 extension.c_str (), (*it).c_str ());

        if (!(low == ext_lst_cpy.end ()) && boost::iequals ((*low), extension))
          {
            TIZ_LOG (TIZ_PRIORITY_TRACE, "[%s] added to playlist low [%s]", (*it).c_str (),
                     (*low).c_str ());
            filtered_file_list.push_back (*it);
            extension_list_filtered.insert (extension);
          }
        ++it;
      }

    file_list = filtered_file_list;
    TIZ_LOG (TIZ_PRIORITY_TRACE, "%d elements in playlist", file_list.size ());
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

bool
tizplaylist::assemble_play_list (const std::string &base_uri,
                                 const bool shuffle_playlist,
                                 const bool recurse,
                                 const extension_list_t &extension_list,
                                 uri_list_t &file_list,
                                 std::string &error_msg)
{
  bool list_assembled = false;
  extension_list_t extension_list_filtered;

  try
    {
      if (base_uri.empty ())
        {
          error_msg.assign ("Empty media uri.");
          goto end;
        }

      if (OMX_ErrorNone != process_base_uri (base_uri, file_list, recurse))
        {
          error_msg.assign ("File not found.");
          goto end;
        }

      if (OMX_ErrorNone != filter_unknown_media (extension_list, file_list,
                                                 extension_list_filtered))
        {
          error_msg.assign ("No supported media types found.");
          goto end;
        }

      if (shuffle_playlist)
        {
          std::random_shuffle (file_list.begin (), file_list.end ());
        }
      else
        {
          std::sort (file_list.begin (), file_list.end ());
        }

      list_assembled = true;
    }
  catch(std::exception const& e)
    {
      error_msg.assign (e.what ());
    }
  catch(...)
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
  fprintf (stdout, "%sPlaylist length: %lu. File extensions in playlist: %s%s\n\n",
           KBLU,
           (long) file_list.size (),
           boost::algorithm::join(extension_list_filtered, ", ").c_str (),
           KNRM);
    }

  return list_assembled;
}
