/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
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
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#ifndef COMMON_HEADER_H
#define COMMON_HEADER_H

#include "wifi-information-element.h"

namespace ns3 {

/**
 * \ingroup wifi
 * Implemention for generic Management Frame.
 */
class MgtFrame {
public:
  /**
   * Add new Wifi Information Element that is not vendor specific.
   * \param element the Wifi Information Element to bee added to the DMG Beacon Frame Body.
   */
  void AddWifiInformationElement (Ptr<WifiInformationElement> element);
  /**
   * Get a specific Wifi information element by ID.
   * \param id The ID of the Wifi Information Element.
   * \return
   */
  Ptr<WifiInformationElement> GetInformationElement (WifiInformationElementId id);
  /**
   * Get List of Information Element associated with this frame.
   * \return
   */
  WifiInformationElementMap GetListOfInformationElement (void) const;

protected:
  void PrintInformationElements (std::ostream &os) const;
  uint32_t GetInformationElementsSerializedSize (void) const;
  Buffer::Iterator SerializeInformationElements (Buffer::Iterator start) const;
  Buffer::Iterator DeserializeInformationElements (Buffer::Iterator start);

private:
  WifiInformationElementMap m_map;                 //!< Map of Wifi Information Element.

};

}

#endif /* COMMON_HEADER_H */
