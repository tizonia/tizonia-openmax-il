/**
 * Copyright (C) 2011-2019 Aratelia Limited - Juan A. Rubio
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
 * @file   tizrmwaiter.hpp
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - RM waiter utility class
 *
 *
 */

#ifndef TIZRMWAITER_HPP
#define TIZRMWAITER_HPP

#include <string>
#include <vector>

class tizrmwaiter
{

public:
  tizrmwaiter (const uint32_t &rid, const uint32_t &quantity,
               const std::string &cname, const std::vector< uint8_t > &uuid,
               const uint32_t &grpid, const uint32_t &pri)
    : cname_ (cname),
      uuid_ (uuid),
      grpid_ (pri),
      pri_ (pri),
      rid_ (rid),
      quantity_ (quantity)
  {
  }

  std::string cname () const
  {
    return cname_;
  }

  uint32_t resid () const
  {
    return rid_;
  }

  uint32_t quantity () const
  {
    return quantity_;
  }

  const std::vector< uint8_t > &uuid () const
  {
    return uuid_;
  }

  uint32_t grpid () const
  {
    return grpid_;
  }

  uint32_t pri () const
  {
    return pri_;
  }

  bool operator==(const tizrmwaiter &rhs) const
  {
    return ((uuid_ == rhs.uuid_) && (rid_ == rhs.rid_));
  }

  bool operator<(const tizrmwaiter &rhs) const
  {
    if (rid_ < rhs.rid_)
    {
      return true;
    }
    else
    {
      if (uuid_ < rhs.uuid_)
      {
        return true;
      }
      else
      {
        return false;
      }
    }
  }

private:
  std::string cname_;
  std::vector< uint8_t > uuid_;
  uint32_t grpid_;
  uint32_t pri_;
  uint32_t rid_;
  uint32_t quantity_;
};

class remove_waiter_functor
{

public:
  explicit remove_waiter_functor (const std::vector< unsigned char > &uuid)
    : uuid_ (uuid)
  {
  }

  bool operator()(const tizrmwaiter &waiter)
  {
    return (uuid_ == waiter.uuid ());
  }

private:
  std::vector< unsigned char > uuid_;
};

#endif  // TIZRMWAITER_HPP
