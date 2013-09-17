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
 * @file   tizrmdb.cc
 * @author Juan A. Rubio <juan.rubio@aratelia.com>
 *
 * @brief  Tizonia OpenMAX IL - RM SQLite3 database handling - implementation
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "./tizrmdb.h"
#include "tizosal.h"

#include <sqlite3.h>
#include <stdlib.h>

#include <boost/assert.hpp>
#include <vector>
#include <sstream>
#include <iostream>

#ifdef TIZ_LOG_CATEGORY_NAME
#undef TIZ_LOG_CATEGORY_NAME
#define TIZ_LOG_CATEGORY_NAME "tiz.rm.daemon.db"
#endif

static const char *TIZ_RM_DB_DROP_ALLOC_TABLE =
  "drop table if exists allocation";
static const char *TIZ_RM_DB_CREATE_ALLOC_TABLE =
  "create table allocation(cname varchar(255), uuid varchar(16), grpid "
  "smallint, pri smallint, resid smallint, allocation mediumint)";

static const char *TIZ_RM_DB_RESNAMES_FROM_RESOURCES =
  "select resname from resources";
static const char *TIZ_RM_DB_RESIDS_FROM_RESOURCES =
  "select resid from resources";

static const char *TIZ_RM_DB_CNAMES_FROM_COMPONENTS =
  "select cname from components";

tizrmdb::tizrmdb (char const *ap_dbname):
  pdb_ (0),
  dbname_ (ap_dbname)
{
}

tizrmdb::~tizrmdb ()
{
  close ();
}

tizrm_error_t
tizrmdb::connect ()
{
  tizrm_error_t ret_val = TIZRM_SUCCESS;

  if (!dbname_.empty ())
    {
      int
        rc = open (dbname_.c_str ());
      TIZ_LOG (TIZ_TRACE, "Connecting db [%s]", dbname_.c_str ());
      if (rc != SQLITE_OK)
        {
          TIZ_LOG (TIZ_TRACE, "Could not connect db [%s]", dbname_.c_str ());
          ret_val = TIZRM_DATABASE_OPEN_ERROR;
        }
      else
        {
          rc = reset_alloc_table ();
          if (rc != SQLITE_OK)
            {
              TIZ_LOG (TIZ_TRACE, "Could not init db [%s]", dbname_.c_str ());
              ret_val = TIZRM_DATABASE_INIT_ERROR;
            }
        }
    }
  else
    {
      TIZ_LOG (TIZ_TRACE, "Empty db name");
      ret_val = TIZRM_DATABASE_OPEN_ERROR;
    }

  return ret_val;
}

tizrm_error_t
tizrmdb::disconnect ()
{
  tizrm_error_t ret_val = TIZRM_SUCCESS;
  int rc = close ();

  if (SQLITE_OK != rc)
    {
      TIZ_LOG (TIZ_TRACE, "Could not disconnect db");
      ret_val = TIZRM_DATABASE_CLOSE_ERROR;
    }

  return ret_val;
}

int
tizrmdb::open (char const *ap_dbname)
{
  BOOST_ASSERT (ap_dbname);
  close ();
  return sqlite3_open (ap_dbname, &pdb_);
}

int
tizrmdb::close ()
{
  int rc = SQLITE_OK;
  if (pdb_)
    {
      rc = sqlite3_close (pdb_);
      pdb_ = 0;
      dbname_.clear ();
    }

  return rc;
}

int
tizrmdb::reset_alloc_table ()
{
  int rc = SQLITE_OK;
  char *p_errmsg;

  // Drop allocation table
  if (pdb_)
    {
      rc = sqlite3_exec (pdb_, TIZ_RM_DB_DROP_ALLOC_TABLE,
                         NULL, NULL, &p_errmsg);
      TIZ_LOG (TIZ_TRACE, "Dropping allocation table...");
      if (rc != SQLITE_OK)
        {
          TIZ_LOG (TIZ_TRACE, "Could not drop allocation table [%s]",
                   p_errmsg);
        }

      rc = sqlite3_exec (pdb_, TIZ_RM_DB_CREATE_ALLOC_TABLE,
                         NULL, NULL, &p_errmsg);
      if (rc != SQLITE_OK)
        {
          TIZ_LOG (TIZ_TRACE, "Could not create allocation table [%s]",
                   p_errmsg);
          return rc;
        }
      TIZ_LOG (TIZ_TRACE, "Created allocation table succesfully");
    }

  return rc;
}

bool
tizrmdb::resource_available (const unsigned int &rid,
                             const unsigned int &quantity) const
{
  bool ret_val = false;
  int rc = SQLITE_OK;
  char query[255];

  TIZ_LOG (TIZ_TRACE, "tizrmdb::resource_available : Checking resource "
           " availability for resid [%d] - quantity [%d]", rid, quantity);

  snprintf (query, sizeof (query),
            "select * from resources where resid='%d' and current>='%d'",
            rid, quantity);

  rc = run_query (query);

  if (SQLITE_OK == rc && !vdata_.empty ())
    {
      TIZ_LOG (TIZ_TRACE, "tizrmdb::resource_available : "
               "Enough resource id [%d] available", rid);
      ret_val = true;
    }

  return ret_val;
}

bool
tizrmdb::resource_provisioned (const unsigned int &rid) const
{
  bool ret_val = false;
  int rc = SQLITE_OK;
  char query[255];

  TIZ_LOG (TIZ_TRACE, "tizrmdb::resource_provisioned");

  snprintf (query, sizeof (query),
            "select * from resources where resid='%d'", rid);

  rc = run_query (query);

  if (SQLITE_OK == rc && !vdata_.empty ())
    {
      ret_val = true;
    }

  TIZ_LOG (TIZ_TRACE, "Resource id [%d] is [%s]", rid,
           (ret_val == true ? "PROVISIONED" : "NOT PROVISIONED"));

  return ret_val;
}

bool
tizrmdb::resource_acquired (const std::vector < unsigned char >&uuid,
                            const unsigned int &rid,
                            const unsigned int &quantity) const
{
  bool ret_val = false;
  int rc = SQLITE_OK;
  char query[255];
  char uuid_str[129];

  tiz_uuid_str (&uuid[0], uuid_str);

  TIZ_LOG (TIZ_TRACE, "tizrmdb::resource_acquired : uuid [%s] - "
           "rid [%d] - quantity [%d]", uuid_str, rid, quantity);

  snprintf (query, sizeof (query),
            "select * from allocation where uuid='%s' and resid='%d' "
            "and allocation>='%d'", uuid_str, rid, quantity);

  rc = run_query (query);

  if (SQLITE_OK == rc && !vdata_.empty ())
    {
      ret_val = true;
    }

  TIZ_LOG (TIZ_TRACE, "tizrmdb::resource_acquired : "
           "'%s' : allocated [%s] units "
           "of resource id [%d] (at least [%d] units were expected)",
           uuid_str, (true == ret_val ? "ENOUGH" : "NOT ENOUGH"),
           rid, quantity);

  return ret_val;
}

bool
tizrmdb::comp_provisioned (const std::string & cname) const
{
  bool ret_val = false;
  int rc = SQLITE_OK;

  TIZ_LOG (TIZ_TRACE, "tizrmdb::comp_provisioned : Checking [%s]",
           cname.c_str ());

  rc = run_query (TIZ_RM_DB_CNAMES_FROM_COMPONENTS);

  if (SQLITE_OK == rc)
    {
      for (std::vector < std::string >::iterator it = vdata_.begin ();
           it < vdata_.end (); ++it)
        {
          if (cname.compare (*it) == 0)
            {
              ret_val = true;
            }
        }
    }

  TIZ_LOG (TIZ_TRACE, "'%s' is [%s]",
           cname.c_str (),
           (true == ret_val ? "PROVISIONED" : "NOT PROVISIONED"));

  return ret_val;
}

bool
tizrmdb::comp_provisioned_with_resid (const std::string & cname,
                                      const unsigned int &rid) const
{
  bool ret_val = false;
  int rc = SQLITE_OK;
  char query[255];

  TIZ_LOG (TIZ_TRACE, "tizrmdb::comp_provisioned_with_resid : "
           "'%s' : Checking component provisioning for "
           "resource id [%d]", cname.c_str (), rid);

  snprintf (query, sizeof (query),
            "select * from components where cname='%s' and resid=%d",
            cname.c_str (), rid);

  rc = run_query (query);

  if (SQLITE_OK == rc && !vdata_.empty ())
    {
      ret_val = true;
    }

  TIZ_LOG (TIZ_TRACE, "'%s' : is [%s] with resource id [%d]",
           cname.c_str (),
           (true == ret_val ? "PROVISIONED" : "NOT PROVISIONED"), rid);

  return ret_val;
}

tizrm_error_t
tizrmdb::acquire_resource (const unsigned int &rid,
                           const unsigned int &quantity,
                           const std::string & cname,
                           const std::vector < unsigned char >&uuid,
                           const unsigned int &grpid,
                           const unsigned int &pri)
{
  int rc = SQLITE_OK;
  char query[500];
  char uuid_str[129];
  int current = 0;
  int requirement = 0;

  tiz_uuid_str (&uuid[0], uuid_str);

  TIZ_LOG (TIZ_TRACE, "tizrmdb::acquire_resource : "
           "'%s': Acquiring [%d] units of resource [%d] "
           "uuid [%s]", cname.c_str (), quantity, rid, uuid_str);

  // Check that the component is provisioned and is allowed access to the
  // resource
  if (!comp_provisioned_with_resid (cname, rid))
    {
      TIZ_LOG (TIZ_TRACE, "tizrmdb::acquire_resource : "
               "'%s' is not provisioned...", cname.c_str ());
      return TIZRM_COMPONENT_NOT_PROVISIONED;
    }

  requirement = strtol (vdata_[vdata_.size () - 1].c_str (), NULL, 0);

  TIZ_LOG (TIZ_TRACE, "tizrmdb::acquire_resource : "
           "[%s]: provisioned requirement [%d] units, "
           "actually requested [%d] ...",
           cname.c_str (), requirement, quantity);

  if (quantity > requirement)
    {
      TIZ_LOG (TIZ_TRACE, "tizrmdb::acquire_resource : "
               "[%s]: requested [%d] units, but provisioned "
               "only [%d]", cname.c_str (), quantity, requirement);
      return TIZRM_NOT_ENOUGH_RESOURCE_PROVISIONED;
    }

  // Check that the requested resource is provisioned and there is availability
  if (!resource_available (rid, quantity))
    {
      TIZ_LOG (TIZ_TRACE, "tizrmdb::acquire_resource : "
               "Resource [%d] not available...", rid);
      return TIZRM_NOT_ENOUGH_RESOURCE_AVAILABLE;
    }

  current = strtol (vdata_[vdata_.size () - 1].c_str (), NULL, 0);

  TIZ_LOG (TIZ_TRACE, "tizrmdb::acquire_resource: "
           "Resource [%d]: available [%d] units ...", rid, current);

  snprintf (query, sizeof (query),
            "insert or replace into allocation (cname, uuid, grpid, "
            "pri, resid, allocation) "
            "values('%s', '%s', %d, %d, %d, %d)",
            cname.c_str (), uuid_str, grpid, pri, rid, quantity);

  rc = run_query (query);

  if (SQLITE_OK != rc)
    {
      TIZ_LOG (TIZ_TRACE, "tizrmdb::acquire_resource : "
               "'%s' : Could not update allocation table", cname.c_str ());
      return TIZRM_DATABASE_ERROR;
    }

  snprintf (query, sizeof (query),
            "update resources set current=%d where resid=%d",
            current - quantity, rid);

  rc = run_query (query);

  if (SQLITE_OK != rc)
    {
      TIZ_LOG (TIZ_TRACE, "tizrmdb::acquire_resource : "
               "Could not update resource table " "for resource [%s]", rid);
      return TIZRM_DATABASE_ERROR;
    }

  TIZ_LOG (TIZ_TRACE, "tizrmdb::acquire_resource: "
           "Succesfully acquired resource [%d] for [%s]", rid, cname.c_str ());

  return TIZRM_SUCCESS;
}

tizrm_error_t
tizrmdb::release_resource (const unsigned int &rid,
                           const unsigned int &quantity,
                           const std::string & cname,
                           const std::vector < unsigned char >&uuid,
                           const unsigned int &grpid,
                           const unsigned int &pri)
{
  int rc = SQLITE_OK;
  char query[500];
  char uuid_str[129];
  int current = 0;
  int requirement = 0;

  TIZ_LOG (TIZ_TRACE, "tizrmdb::release_resource : "
           "'%s':  [%d] units of resource [%d]",
           cname.c_str (), quantity, rid);

  // Check that the component is provisioned and is allowed to access the
  // resource
  if (!comp_provisioned_with_resid (cname, rid))
    {
      TIZ_LOG (TIZ_TRACE, "'%s' is not provisioned...", cname.c_str ());
      return TIZRM_COMPONENT_NOT_PROVISIONED;
    }

  requirement = strtol (vdata_[vdata_.size () - 1].c_str (), NULL, 0);

  TIZ_LOG (TIZ_TRACE, "'%s': provisioned requirement [%d] units, "
           "actually requested [%d] ...",
           cname.c_str (), requirement, quantity);

  if (quantity > requirement)
    {
      TIZ_LOG (TIZ_TRACE, "'%s': releasing [%d] units, "
               "but provisioned only [%d]",
               cname.c_str (), quantity, requirement);
      return TIZRM_NOT_ENOUGH_RESOURCE_PROVISIONED;
    }

  // Check that the resource was effectively acquired by the component
  if (!resource_acquired (uuid, rid, quantity))
    {
      TIZ_LOG (TIZ_TRACE, "Resource [%d] cannot be released: "
               "not enough resource previously acquired", rid);
      return TIZRM_NOT_ENOUGH_RESOURCE_ACQUIRED;
    }

  current = strtol (vdata_[vdata_.size () - 1].c_str (), NULL, 0);

  TIZ_LOG (TIZ_TRACE, "Resource [%d]: current allocation [%d] units ...",
           rid, current);

  tiz_uuid_str (&uuid[0], uuid_str);

  // Update allocation table to reflect the resource release...

  // ... first delete the the row...
  snprintf (query, sizeof (query),
            "delete from allocation where "
            "cname='%s' and uuid='%s' and resid=%d",
            cname.c_str (), uuid_str, rid);

  rc = run_query (query);

  if (SQLITE_OK != rc)
    {
      TIZ_LOG (TIZ_TRACE, "'%s' : Could not update allocation "
               "table ", cname.c_str ());
      return TIZRM_DATABASE_ACCESS_ERROR;
    }

  //... now create a new one, only if there's some resource allocation
  // remaining
  if (current - quantity)
    {
      // TODO : This string is duplicated. Move to a constant
      snprintf (query, sizeof (query),
                "insert or replace into allocation (cname, uuid, grpid, pri, "
                "resid, allocation) values('%s', '%s', %d, %d, %d, %d)",
                cname.c_str (), uuid_str, grpid, pri, rid, current - quantity);

      rc = run_query (query);

      if (SQLITE_OK != rc)
        {
          TIZ_LOG (TIZ_TRACE, "'%s' : Could not update allocation table",
                   cname.c_str ());
          return TIZRM_DATABASE_ACCESS_ERROR;
        }
    }

  // Now, obtain the current resource availability
  if (!resource_available (rid, 0))
    {
      TIZ_LOG (TIZ_TRACE, "Resource [%d] not available...", rid);
      return TIZRM_NOT_ENOUGH_RESOURCE_AVAILABLE;
    }

  current = strtol (vdata_[vdata_.size () - 1].c_str (), NULL, 0);

  // Now update the resource table...
  snprintf (query, sizeof (query),
            "update resources set current=%d where resid=%d",
            current + quantity, rid);

  rc = run_query (query);

  if (SQLITE_OK != rc)
    {
      TIZ_LOG (TIZ_TRACE, "Could not update resource table "
               "for resource [%s]", rid);
      return TIZRM_DATABASE_ACCESS_ERROR;
    }

  TIZ_LOG (TIZ_TRACE, "'%s' : Succesfully released [%d] units of "
           "resource id [%d]", cname.c_str (), quantity, rid);

  return TIZRM_SUCCESS;
}

tizrm_error_t
tizrmdb::release_all (const std::string & cname,
                      const std::vector < unsigned char >&uuid)
{
  int rc = SQLITE_OK;
  char query[500];
  char uuid_str[129];
  int current = 0;
  int remaining = 0;

  tiz_uuid_str (&uuid[0], uuid_str);

  TIZ_LOG (TIZ_TRACE, "tizrmdb::release_all : Releasing resources for "
           "component with uuid [%s]", uuid_str);

  for (int rid = 0; rid < TIZRM_RESOURCE_MAX; ++rid)
    {
      if (resource_acquired (uuid, rid, 0))
        {
          const std::string cname = vdata_[0];
          current = strtol (vdata_[vdata_.size () - 1].c_str (), NULL, 0);

          TIZ_LOG (TIZ_TRACE, "'%s' uuid [%s] : Resource [%d] "
                   "current allocation is "
                   "[%d] units ...", cname.c_str (), uuid_str, rid, current);

          // Update allocation table to reflect the resource release...
          snprintf (query, sizeof (query),
                    "delete from allocation where "
                    "cname='%s' and uuid='%s' and resid=%d",
                    cname.c_str (), uuid_str, rid);

          rc = run_query (query);

          if (SQLITE_OK != rc)
            {
              TIZ_LOG (TIZ_TRACE, "'%s' : Could not update allocation "
                       "table ", cname.c_str ());
              return TIZRM_DATABASE_ACCESS_ERROR;
            }

          // Now, obtain the current resource availability
          resource_available (rid, 0);

          remaining = strtol (vdata_[vdata_.size () - 1].c_str (), NULL, 0);

          // Now update the resource table...
          snprintf (query, sizeof (query),
                    "update resources set current=%d where resid=%d",
                    remaining + current, rid);

          rc = run_query (query);

          if (SQLITE_OK != rc)
            {
              TIZ_LOG (TIZ_TRACE, "Could not update resource table "
                       "for resource [%s]", rid);
              return TIZRM_DATABASE_ACCESS_ERROR;
            }

          TIZ_LOG (TIZ_TRACE, "'%s':  Released [%d] units of "
                   "resource  id [%d]", cname.c_str (), current, rid);
        }
    }

  return TIZRM_SUCCESS;
}

tizrm_error_t
tizrmdb::find_owners (const unsigned int &rid,
                      const unsigned int &pri,
                      tizrm_owners_list_t & owners) const
{
  int rc = SQLITE_OK;
  char query[500];
  char uuid_str[129];

  TIZ_LOG (TIZ_TRACE, "tizrmdb::find_owners : resource id [%d] "
           "pri > [%d]", rid, pri);

  owners.clear ();

  snprintf (query, sizeof (query),
            "select * from allocation where resid='%d' and pri>'%d'",
            rid, pri);

  rc = run_query (query);

  if (SQLITE_OK != rc)
    {
      return TIZRM_DATABASE_ERROR;
    }

  int headingsize = vcol_head_.size ();
  int datasize = vdata_.size ();
  int num_cols = headingsize ? datasize / headingsize : 0;
  for (int i = 0; i < num_cols; ++i)
    {
      int idx = i * headingsize;
      std::vector < unsigned char >uuid_vec;
      OMX_UUIDTYPE uuid_array;
      tiz_str_uuid (vdata_[idx + 1].data (), &uuid_array);

      uuid_vec.assign (&uuid_array[0], &uuid_array[0] + 128);

      int owner_grpid = strtol (vdata_[idx + 2].c_str (), NULL, 0);
      int owner_pri = strtol (vdata_[idx + 3].c_str (), NULL, 0);
      int owner_rid = strtol (vdata_[idx + 4].c_str (), NULL, 0);
      int owner_quantity = strtol (vdata_[idx + 5].c_str (), NULL, 0);

      TIZ_LOG (TIZ_TRACE, "tizrmdb::find_owners : owner [%s] "
               "uuid [%s] grpid [%d] pri [%d] rid [%d] quantity [%d]",
               vdata_[idx].c_str (), vdata_[idx + 1].c_str (),
               owner_grpid, owner_pri, owner_rid, owner_quantity);

      owners.push_back (tizrmowner (vdata_[idx], uuid_vec, owner_grpid,
                                    owner_pri, owner_rid, owner_quantity));
    }

  // Sort the owners list in ascending priority order, using tizrmowner's
  // operator<
  owners.sort ();

  TIZ_LOG (TIZ_TRACE, "tizrmdb::find_owners : "
           "Found [%d] owners with priority > [%d] that have allocated "
           "resource id [%d]", owners.size (), pri, rid);

  return TIZRM_SUCCESS;
}

int
tizrmdb::run_query (char const *ap_sql)
{
  int rc = SQLITE_OK;
  char *p_errmsg;
  char **pp_result;
  int nrow, ncol;

  BOOST_ASSERT (ap_sql);

  TIZ_LOG (TIZ_TRACE, "Running query [%s]", ap_sql);

  vcol_head_.clear ();
  vdata_.clear ();

  rc = sqlite3_get_table (pdb_, ap_sql, &pp_result, &nrow, &ncol, &p_errmsg);

  TIZ_LOG (TIZ_TRACE, "Running query [%s] rc [%d] nrow [%d] ncol [%d]",
           ap_sql, rc, nrow, ncol);


  if (SQLITE_OK == rc)
    {
      for (int i = 0; i < ncol; ++i)
        {
          vcol_head_.push_back (pp_result[i]);
        }

      for (int i = 0; i < ncol * nrow; ++i)
        {
          vdata_.push_back (pp_result[ncol + i]);
        }
    }
  else
    {
      TIZ_LOG (TIZ_TRACE, "Query execution failure: [%s] - [%s]",
               sqlite_error_str (rc).c_str (), p_errmsg);
    }

  TIZ_LOG (TIZ_TRACE, "Run query [%s] rc [%d]", ap_sql, rc);

  sqlite3_free_table (pp_result);

  print_query_result ();

  return rc;
}

int
tizrmdb::run_query (char const *ap_sql) const
{
  return const_cast < tizrmdb * >(this)->run_query (ap_sql);
}

void
tizrmdb::print_query_result () const
{
  TIZ_LOG (TIZ_TRACE, "tizrmdb::print_query_result : vdata_.size() [%d]"
           " vcol_head_.size() [%d]", vdata_.size (), vcol_head_.size ());

  if (!vcol_head_.empty ())
    {
      int headingsize = vcol_head_.size ();
      std::stringstream headings;
      for (int i = 0; i < headingsize; ++i)
        {
          headings << "[" << vcol_head_[i] << "] \t\t\t ";
        }
      TIZ_LOG (TIZ_TRACE, "%s", headings.str ().c_str ());

      int datasize = vdata_.size ();
      std::stringstream data;
      for (int i = 0; i < datasize; ++i)
        {
          data << "[" << vdata_[i] << "] \t\t ";
        }
      TIZ_LOG (TIZ_TRACE, "%s", data.str ().c_str ());
    }
}

std::string
tizrmdb::sqlite_error_str (int error) const
{
  switch (error)
    {
    case SQLITE_OK:            /* Successful result */
      {
        return "SQLITE_OK";
      }
      break;

      /* beginning-of-error-codes */
    case SQLITE_ERROR:         /* SQL error or missing database */
      {
        return "SQLITE_ERROR";
      }
      break;
    case SQLITE_INTERNAL:      /* Internal logic error in SQLite */
      {
        return "SQLITE_INTERNAL";
      }
      break;
    case SQLITE_PERM:          /* Access permission denied */
      {
        return "SQLITE_PERM";
      }
      break;
    case SQLITE_ABORT:         /* Callback routine requested an abort */
      {
        return "SQLITE_ABORT";
      }
      break;
    case SQLITE_BUSY:          /* The database file is locked */
      {
        return "SQLITE_BUSY";
      }
      break;
    case SQLITE_LOCKED:        /* A table in the database is locked */
      {
        return "SQLITE_LOCKED";
      }
      break;
    case SQLITE_NOMEM:         /* A malloc() failed */
      {
        return "SQLITE_NOMEM";
      }
      break;
    case SQLITE_READONLY:      /* Attempt to write a readonly database */
      {
        return "SQLITE_READONLY";
      }
      break;
    case SQLITE_INTERRUPT:     /* Operation terminated by sqlite3_interrupt() */
      {
        return "SQLITE3_INTERRUPT";
      }
      break;
    case SQLITE_IOERR:         /* Some kind of disk I/O error occurred */
      {
        return "SQLITE_IOERR";
      }
      break;
    case SQLITE_CORRUPT:       /* The database disk image is malformed */
      {
        return "SQLITE_CORRUPT";
      }
      break;
    case SQLITE_NOTFOUND:      /* Unknown opcode in sqlite3_file_control() */
      {
        return "SQLITE_NOTFOUND";
      }
      break;
    case SQLITE_FULL:          /* Insertion failed because database is full */
      {
        return "SQLITE_FULL";
      }
      break;
    case SQLITE_CANTOPEN:      /* Unable to open the database file */
      {
        return "SQLITE_CANTOPEN";
      }
      break;
    case SQLITE_PROTOCOL:      /* Database lock protocol error */
      {
        return "SQLITE_PROTOCOL";
      }
      break;
    case SQLITE_EMPTY:         /* Database is empty */
      {
        return "SQLITE_EMPTY";
      }
      break;
    case SQLITE_SCHEMA:        /* The database schema changed */
      {
        return "SQLITE_SCHEMA";
      }
      break;
    case SQLITE_TOOBIG:        /* String or BLOB exceeds size limit */
      {
        return "SQLITE_TOOBIG";
      }
      break;
    case SQLITE_CONSTRAINT:    /* Abort due to constraint violation */
      {
        return "SQLITE_CONSTRAINT";
      }
      break;
    case SQLITE_MISMATCH:      /* Data type mismatch */
      {
        return "SQLITE_MISMATCH";
      }
      break;
    case SQLITE_MISUSE:        /* Library used incorrectly */
      {
        return "SQLITE_MISUSE";
      }
      break;
    case SQLITE_NOLFS:         /* Uses OS features not supported on host */
      {
        return "SQLITE_NOLFS";
      }
      break;
    case SQLITE_AUTH:          /* Authorization denied */
      {
        return "SQLITE_AUTH";
      }
      break;
    case SQLITE_FORMAT:        /* Autiziary database format error */
      {
        return "SQLITE_FORMAT";
      }
      break;
    case SQLITE_RANGE:         /* 2nd parameter to sqlite3_bind out of range */
      {
        return "SQLITE_RANGE";
      }
      break;
    case SQLITE_NOTADB:        /* File opened that is not a database file */
      {
        return "SQLITE_NOTADB";
      }
      break;
    case SQLITE_ROW:           /* sqlite3_step() has another row ready */
      {
        return "SQLITE_ROW";
      }
      break;
    case SQLITE_DONE:          /* sqlite3_step() has finished executing */
      {
        return "SQLITE_DONE";
      }
      break;
      /* end-of-error-codes */
    default:
      {
        return "UNKNOWN";
      }
    };
}
