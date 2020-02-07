/**
 * Copyright (C) 2011-2020 Aratelia Limited - Juan A. Rubio
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
 * @file   tizplaylist.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Plyalist utility class
 *
 *
 */

#ifndef TIZPLAYLIST_HPP
#define TIZPLAYLIST_HPP

#include "tizgraphtypes.hpp"

namespace tiz
{
  class playlist
  {

  public:
    enum list_direction_t
      {
        DirUp,
        DirDown,
        DirMax
      };

  public:
    explicit playlist (const uri_lst_t &uri_list = uri_lst_t (), const bool shuffle = false);
    playlist (const playlist &playlist);

    static bool assemble_play_list (const std::string &base_uri,
                                    const bool shuffle_playlist,
                                    const bool recurse,
                                    const file_extension_lst_t &extension_list,
                                    uri_lst_t &file_list, std::string &error_msg);

    void skip (const int jump);
    playlist obtain_next_sub_playlist (const list_direction_t up_or_down);
    const std::string & get_current_uri () const;
    uri_lst_t get_sublist (const int from, const int to) const;
    const uri_lst_t &get_uri_list () const;
    int current_index () const;
    int size () const;
    bool empty () const;
    bool single_format () const;
    bool before_begin () const;
    bool past_end () const;
    bool loop_playback () const;
    void set_loop_playback (const bool loop_playback);
    bool shuffle () const;
    void set_index (const int index);
    void erase_uri (const int index);
    void print_info ();

  private:
    enum single_format_t
      {
        Unknown,
        Yes,
        No
      };

  private:

  private:

    void scan_list ();
    int find_next_sub_list (const int index) const;

    // TODO: Possibly use a shared pointer here to make copy a less expensive
    // operation
    uri_lst_t uri_list_;
    int current_index_;
    bool loop_playback_;
    std::vector<size_t> sub_list_indexes_;
    int current_sub_list_;
    bool shuffle_;
    mutable file_extension_lst_t extension_list_;
    mutable single_format_t single_format_;
  };
}  // namespace tiz

#endif  // TIZPLAYLIST_HPP
