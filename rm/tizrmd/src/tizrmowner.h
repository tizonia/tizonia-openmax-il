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
 * @file   tizrmowner.hh
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - RM owner utility class
 *
 *
 */

#ifndef TIZRMOWNER_HH
#define TIZRMOWNER_HH

#include <list>
#include <string>
#include <vector>

class tizrmowner
{
public:

  tizrmowner(const std::string &cname,
             const std::vector< unsigned char > &uuid,
             const unsigned int &grpid,
             const unsigned int &pri,
             const unsigned int &rid,
             const unsigned int &quantity) :
    cname_(cname),
    uuid_(uuid),
    grpid_(grpid),
    pri_(pri),
    rid_(rid),
    quantity_(quantity)
  {
  }

  bool operator==(const tizrmowner &rhs) const
  {
    return (cname_ == rhs.cname_
            && uuid_ == rhs.uuid_
            && grpid_ == rhs.grpid_
            && pri_ == rhs.pri_
            && rid_ == rhs.rid_
            && quantity_ == rhs.quantity_);
  }

  bool operator<(const tizrmowner& rhs) const
  {
    return (pri_ < rhs.pri_);
  }

public:

  std::string cname_;
  std::vector< unsigned char > uuid_;
  unsigned int grpid_;
  unsigned int pri_;
  unsigned int rid_;
  unsigned int quantity_;

};

typedef std::list<tizrmowner> tizrm_owners_list_t;

#endif // TIZRMOWNER_HH
