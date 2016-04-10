/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, IMDEA Networks Institute
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
