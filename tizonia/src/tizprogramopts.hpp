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
 * @file   tizprogramopts.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Program options parsing utility.
 *
 *
 */

#ifndef TIZPROGRAMOPTS_HPP
#define TIZPROGRAMOPTS_HPP

#include <map>
#include <string>

#include <boost/program_options.hpp>
#include <boost/function.hpp>

#include <OMX_Core.h>

namespace tiz
{
  class programopts
  {
    typedef boost::function< OMX_ERRORTYPE () > option_handler_t;
    typedef std::map< std::string, option_handler_t > option_handlers_map_t;

  public:
    programopts (int argc, char *argv[]);

    int consume ();

    void print_version () const;
    void print_usage () const;
    void print_usage_extended () const;

    void set_option_handler (const std::string &option,
                             const option_handler_t handler);

    bool shuffle () const;
    bool recurse () const;
    bool daemon () const;
    const std::string &log_dir () const;
    bool debug_info () const;
    const std::string &component_name () const;
    const std::string &component_role () const;
    int port () const;
    const std::string &station_name () const;
    const std::string &station_genre () const;
    const std::string &bitrates () const;
    const std::vector< std::string > &bitrate_list () const;
    const std::string &sampling_rates () const;
    const std::vector< int > &sampling_rate_list () const;
    const std::vector< std::string > &uri_list () const;
    const std::string &spotify_user () const;
    const std::string &spotify_password () const;
    const std::vector< std::string > &spotify_playlist_container ();

  private:
    void print_license () const;
    void print_examples () const;

    void init_general_options ();
    void init_debug_options ();
    void init_omx_options ();
    void init_streaming_server_options ();
    void init_streaming_client_options ();
    void init_spotify_options ();
    void init_input_uri_option ();

    void parse_command_line (int argc, char *argv[]);

    typedef int (tiz::programopts::*consume_mem_fn_t)(bool &, std::string &);
    typedef boost::function< int(bool &, std::string &) > consume_function_t;

    int consume_debug_options (bool &done, std::string &msg);
    int consume_general_options (bool &done, std::string &msg);
    int consume_omx_options (bool &done, std::string &msg);
    int consume_streaming_server_options (bool &done, std::string &msg);
    int consume_streaming_client_options (bool &done, std::string &msg);
    int consume_spotify_client_options (bool &done, std::string &msg);
    int consume_local_decode_options (bool &done, std::string &msg);
    int consume_input_file_uris_option ();
    int consume_input_http_uris_option ();

    bool verify_omx_options () const;
    bool verify_streaming_server_options () const;
    bool verify_spotify_client_options () const;
    bool validate_port_argument (std::string &msg) const;
    bool validate_bitrates_argument (std::string &msg);
    bool validate_sampling_rates_argument (std::string &msg);
    bool is_options_count_valid (const unsigned int mode_opts_count) const;

    int call_handler (const option_handlers_map_t::const_iterator &handler_it);

    void register_consume_function (const consume_mem_fn_t cf);

  private:
    int argc_;
    char **argv_;
    option_handlers_map_t option_handlers_map_;
    boost::program_options::variables_map vm_;
    boost::program_options::options_description general_;
    boost::program_options::options_description debug_;
    boost::program_options::options_description omx_;
    boost::program_options::options_description server_;
    boost::program_options::options_description client_;
    boost::program_options::options_description spotify_;
    boost::program_options::options_description input_;
    boost::program_options::positional_options_description positional_;

  private:
    bool recurse_;
    bool shuffle_;
    bool daemon_;
    std::string log_dir_;
    bool debug_info_;
    std::string comp_name_;
    std::string role_name_;
    int port_;
    std::string station_name_;
    std::string station_genre_;
    std::string bitrates_;
    std::vector< std::string > bitrate_list_;
    std::string sampling_rates_;
    std::vector< int > sampling_rate_list_;
    std::vector< std::string > uri_list_;
    std::string spotify_user_;
    std::string spotify_pass_;
    std::string spotify_playlist_;
    std::vector< std::string > spotify_playlist_container_;
    unsigned int default_options_count_;
    std::vector<consume_function_t> consume_functions_;
  };
}
#endif  // TIZPROGRAMOPTS_HPP
