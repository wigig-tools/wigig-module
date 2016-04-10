/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#ifndef DMG_WIFI_MAC_HELPER_H
#define DMG_WIFI_MAC_HELPER_H

#include "qos-wifi-mac-helper.h"
#include "wifi-mac-helper.h"
#include "ns3/qos-utils.h"

namespace ns3 {

/**
 * \brief create DMG-enabled MAC layers for a ns3::WifiNetDevice.
 *
 * This class can create MACs of type ns3::DmgApWifiMac, ns3::DmgStaWifiMac,
 * and, ns3::DmgAdhocWifiMac, with QosSupported, HTSupported and DMGSupported attributes set to True.
 */
class DmgWifiMacHelper : public QosWifiMacHelper
{
public:
  /**
   * Create a QosWifiMacHelper that is used to make life easier when working
   * with Wifi devices using a HT MAC layer.
   */
  DmgWifiMacHelper ();

  /**
   * Destroy a DmgWifiMacHelper
   */
  virtual ~DmgWifiMacHelper ();

  /**
   * Create a mac helper in a default working state.
   */
  static DmgWifiMacHelper Default (void);

 };

} // namespace ns3

#endif /* DMG_WIFI_MAC_HELPER_H */
