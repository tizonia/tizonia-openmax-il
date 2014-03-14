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
 * @file   tizdecgraphmgr.h
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  A manager for decoding graphs
 *
 *
 */

#ifndef TIZDECGRAPHMGR_H
#define TIZDECGRAPHMGR_H

#include "tizgraphtypes.h"
#include "tizgraphmgr.h"

namespace tiz
{
  namespace graphmgr
  {
    /**
     *  @class decodemgr
     *  @brief A manager for decoding graphs.
     *
     */
    class decodemgr : public mgr
    {
    public:

      decodemgr ();
      virtual ~decodemgr ();

    protected:

      ops * do_init (const uri_lst_t &file_list,
                     const error_callback_t &error_cback);

    };

    typedef boost::shared_ptr<decodemgr> decodemgr_ptr_t;

    class decodemgrops : public ops
    {
    public:

      decodemgrops (mgr * p_mgr, const uri_lst_t &file_list,
                    const error_callback_t &error_cback);

    };
  } // namespace graphmgr
} // namespace tiz

#endif // TIZDECGRAPHMGR_H
