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


#include "ns3/uinteger.h"
#include "bft-id-tag.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (BftIdTag);

TypeId
BftIdTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BftIdTag")
    .SetParent<Tag> ()
    .SetGroupName ("Wifi")
    .AddConstructor<BftIdTag> ()
    .AddAttribute ("BftId", "The BFT ID of the current BFT",
                   UintegerValue (0),
                   MakeUintegerAccessor (&BftIdTag::Get),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}

TypeId
BftIdTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

BftIdTag::BftIdTag ()
  : m_bftId (0)
{
}

uint32_t
BftIdTag::GetSerializedSize (void) const
{
  return sizeof (uint16_t);
}

void
BftIdTag::Serialize (TagBuffer i) const
{
  i.WriteU16 (m_bftId);
}

void
BftIdTag::Deserialize (TagBuffer i)
{
  m_bftId = i.ReadU16 ();
}

void
BftIdTag::Print (std::ostream &os) const
{
  os << "BftId=" << m_bftId;
}

void
BftIdTag::Set (uint16_t bftid)
{
  m_bftId = bftid;
}

uint16_t
BftIdTag::Get (void) const
{
  return m_bftId;
}

}
