/* -*-Mode: c++; -*- */
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
 * @file   tizplaylist.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Plyalist utility class
 *
 *
 */

#ifndef TIZPLAYLIST_H
#define TIZPLAYLIST_H

#include "tizgraphtypes.h"

class tizplaylist;
typedef tizplaylist tizplaylist_t;

class tizplaylist
{

public:
  explicit tizplaylist (const uri_lst_t &uri_list = uri_lst_t ());
  tizplaylist (const tizplaylist &playlist, const int from, const int to);

  tizplaylist get_next_sub_playlist ();
  uri_lst_t get_sublist (const int from, const int to) const;
  const uri_lst_t &get_uri_list () const;

  int size () const;
  bool is_single_format_playlist () const;

  static bool assemble_play_list (const std::string &base_uri,
                                  const bool shuffle_playlist,
                                  const bool recurse,
                                  const file_extension_lst_t &extension_list,
                                  uri_lst_t &file_list, std::string &error_msg);

private:
  enum single_format
  {
    Unknown,
    Yes,
    No
  };

private:
  uri_lst_t uri_list_;
  int current_index_;
  mutable single_format single_format_list_;
};

#endif  // TIZPLAYLIST_H
