/*
 * Copyright (C) 2008 Search Solution Corporation. All rights reserved by Search Solution. 
 *
 *   This program is free software; you can redistribute it and/or modify 
 *   it under the terms of the GNU General Public License as published by 
 *   the Free Software Foundation; either version 2 of the License, or 
 *   (at your option) any later version. 
 *
 *  This program is distributed in the hope that it will be useful, 
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
 *  GNU General Public License for more details. 
 *
 *  You should have received a copy of the GNU General Public License 
 *  along with this program; if not, write to the Free Software 
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA 
 *
 */

/*
 * oid.c - object identifier (OID) module  (at client and server)
 */

#ident "$Id$"

#include "config.h"

#include "oid.h"
#include "transform.h"

typedef struct oid_cache_entry OID_CACHE_ENTRY;
struct oid_cache_entry
{
  OID *oid;
  const char *class_name;
};

static OID oid_Root_class = { 0, 0, 0 };
static OID oid_Serial_class = { 0, 0, 0 };
static OID oid_Partition_class = { 0, 0, 0 };
static OID oid_Collation_class = { 0, 0, 0 };
static OID oid_HA_apply_info_class = { 0, 0, 0 };
static OID oid_Class_class = { 0, 0, 0 };
static OID oid_Attribute_class = { 0, 0, 0 };
static OID oid_Rep_Read_Tran = { 0, 0x8000, 0 };

const OID oid_Null_oid = { NULL_PAGEID, NULL_SLOTID, NULL_VOLID };
PAGEID oid_Next_tempid = NULL_PAGEID;

/* ROOT_CLASS OID values. Set during restart/initialization.*/
OID *oid_Root_class_oid = &oid_Root_class;


OID *oid_Serial_class_oid = &oid_Serial_class;
OID *oid_Partition_class_oid = &oid_Partition_class;


OID_CACHE_ENTRY oid_Cache[OID_CACHE_SIZE] = {
  {&oid_Root_class, NULL},	/* Root class is not identifiable by a name */
  {&oid_Serial_class, CT_SERIAL_NAME},
  {&oid_Partition_class, CT_PARTITION_NAME},
  {&oid_Collation_class, CT_COLLATION_NAME},
  {&oid_HA_apply_info_class, CT_HA_APPLY_INFO_NAME},
  {&oid_Class_class, CT_CLASS_NAME},
  {&oid_Attribute_class, CT_ATTRIBUTE_NAME}
};

/*
 * oid_set_root() -  Set the value of the root oid to the given value
 *   return: void
 *   oid(in): Rootoid value
 *   
 * Note:  This function is used during restart/initialization.
 */
void
oid_set_root (const OID * oid)
{
  oid_Root_class_oid = &oid_Root_class;
  if (oid != oid_Root_class_oid)
    {
      oid_Root_class_oid->volid = oid->volid;
      oid_Root_class_oid->pageid = oid->pageid;
      oid_Root_class_oid->slotid = oid->slotid;
    }
}

/*
 * oid_is_root() - Find out if the passed oid is the root oid
 *   return: true/false
 *   oid(in): Object identifier
 */
bool
oid_is_root (const OID * oid)
{
  return OID_EQ (oid, oid_Root_class_oid);
}

/*
 * oid_set_serial () - Store serial class OID
 *
 * return   : 
 * oid (in) :
 */
void
oid_set_serial (const OID * oid)
{
  COPY_OID (oid_Serial_class_oid, oid);
}

/*
 * ois_is_serial () - Compare OID with serial class OID.
 *
 * return : 
 * const OID * oid (in) :
 */
bool
oid_is_serial (const OID * oid)
{
  return OID_EQ (oid, oid_Serial_class_oid);
}

/*
 * oid_get_serial_oid () - Get serial class OID
 *
 * return    : Void.
 * oid (out) : Serial class OID.
 */
void
oid_get_serial_oid (OID * oid)
{
  COPY_OID (oid, oid_Serial_class_oid);
}

/*
 * oid_set_partition () - Store _db_partition class OID
 *
 * return   : 
 * oid (in) :
 */
void
oid_set_partition (const OID * oid)
{
  COPY_OID (oid_Partition_class_oid, oid);
}

/*
 * ois_is_partition () - Compare OID with partition class OID.
 *
 * return : 
 * const OID * oid (in) :
 */
bool
oid_is_partition (const OID * oid)
{
  return OID_EQ (oid, oid_Partition_class_oid);
}

/*
 * oid_get_partition_oid () - Get partition class OID
 *
 * return    : Void.
 * oid (out) : Serial class OID.
 */
void
oid_get_partition_oid (OID * oid)
{
  COPY_OID (oid, oid_Partition_class_oid);
}

/*
 * oid_is_db_class () - Is this OID of db_class?
 *
 * return   : True/false.
 * oid (in) : Check OID.
 */
bool
oid_is_db_class (const OID * oid)
{
  return OID_EQ (oid, &oid_Class_class);
}

/*
 * oid_is_db_attribute () - Is this OID of db_attribute?
 *
 * return   : True/false.
 * oid (in) : Check OID.
 */
bool
oid_is_db_attribute (const OID * oid)
{
  return OID_EQ (oid, &oid_Attribute_class);
}

/*
 * oid_compare() - Compare two oids
 *   return: 0 (oid1 == oid2), 1 (oid1 > oid2), -1 (oid1 < oid2)
 *   a(in): Object identifier of first object
 *   b(in): Object identifier of second object
 */
int
oid_compare (const void *a, const void *b)
{
  const OID *oid1_p = (const OID *) a;
  const OID *oid2_p = (const OID *) b;
  int diff;

  if (oid1_p == oid2_p)
    {
      return 0;
    }

  diff = oid1_p->volid - oid2_p->volid;
  if (diff > 0)
    {
      return 1;
    }
  else if (diff < 0)
    {
      return -1;
    }

  diff = oid1_p->pageid - oid2_p->pageid;
  if (diff > 0)
    {
      return 1;
    }
  else if (diff < 0)
    {
      return -1;
    }

  diff = oid1_p->slotid - oid2_p->slotid;
  if (diff > 0)
    {
      return 1;
    }
  else if (diff < 0)
    {
      return -1;
    }

  return 0;
}

/*
 * oid_hash() - Hash OIDs
 *   return: hash value
 *   key_oid(in): OID to hash
 *   htsize(in): Size of hash table
 */
unsigned int
oid_hash (const void *key_oid, unsigned int htsize)
{
  unsigned int hash;
  const OID *oid = (OID *) key_oid;

  hash = OID_PSEUDO_KEY (oid);
  return (hash % htsize);
}

/*
 * oid_compare_equals() - Compare oids key for hashing
 *   return: 
 *   key_oid1: First key
 *   key_oid2: Second key
 */
int
oid_compare_equals (const void *key_oid1, const void *key_oid2)
{
  const OID *oid1 = (OID *) key_oid1;
  const OID *oid2 = (OID *) key_oid2;

  return OID_EQ (oid1, oid2);
}

/*
 * oid_is_cached_class_oid () - Compare OID with the cached OID identified by
 *				cache_id
 *
 * return : 
 * cache_id (in) :
 * oid (in) :
 */
bool
oid_check_cached_class_oid (const int cache_id, const OID * oid)
{
  return OID_EQ (oid, oid_Cache[cache_id].oid);
}

/*
 * oid_set_cached_class_oid () - Sets the cached value of OID
 *
 * cache_id (in) :
 * oid (in) :
 */
void
oid_set_cached_class_oid (const int cache_id, const OID * oid)
{
  COPY_OID (oid_Cache[cache_id].oid, oid);
}

/*
 * oid_get_cached_class_name () - get the name of cached class
 *
 * return   : 
 * cache_id (in) :
 */
const char *
oid_get_cached_class_name (const int cache_id)
{
  return oid_Cache[cache_id].class_name;
}

/*
 * oid_get_rep_read_tran_oid () - Get OID that is used for RR transactions
 *				  locking. 
 *
 * return    : return the OID.
 */
OID *
oid_get_rep_read_tran_oid (void)
{
  return &oid_Rep_Read_Tran;
}
