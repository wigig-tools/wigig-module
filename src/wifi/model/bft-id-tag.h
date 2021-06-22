/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 NINA GROSHEVA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Nina Grosheva <nina_grosheva@hotmail.com>
 */

#ifndef BFT_ID_TAG_H
#define BFT_ID_TAG_H

#include "ns3/tag.h"

namespace ns3 {

class Tag;

class BftIdTag : public Tag
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  TypeId GetInstanceTypeId (void) const;

  /**
   * Create a BFT ID with the default BFT ID 0
   */
  BftIdTag ();

  uint32_t GetSerializedSize (void) const;
  void Serialize (TagBuffer i) const;
  void Deserialize (TagBuffer i);
  void Print (std::ostream &os) const;

  /**
   * Set the BFT ID to the given value.
   *
   * \param bftId the value of the BFT ID to set
   */
  void Set (uint16_t bftid);
  /**
   * Return the BFT ID value.
   *
   * \return the BFT ID
   */
  uint16_t Get (void) const;


private:
  uint16_t m_bftId;  //!< BFT ID of the current BFT
};

}

#endif /* BFT_ID_TAG_H */
