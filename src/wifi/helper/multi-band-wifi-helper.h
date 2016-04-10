/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#ifndef MULTI_BAND_WIFI_HELPER_H
#define MULTI_BAND_WIFI_HELPER_H

#include <string>
#include "wifi-helper.h"
#include "wifi-mac-helper.h"
#include "ns3/attribute.h"
#include "ns3/object-factory.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/wifi-phy-standard.h"
#include "ns3/trace-helper.h"

namespace ns3 {

class WifiPhy;
class WifiMac;
class WifiRemoteStationManager;
class MultiBandNetDevice;
class Node;

/**
 * \brief helps to create Wifi Devices with different technologies
 * Note only one technology should be operational (Tx/Rx Data) at anytime
 */
typedef struct {
  WifiPhyHelper *PhyHelper;
  WifiMacHelper *MacHelper;
  ObjectFactory RemoteStationManagerFactory;
  enum WifiPhyStandard Standard;
  bool Operational;                                 /* Flag to indicate whether this technology is operational or not */
} WifiTechnologyHelperStruct;

typedef std::vector<WifiTechnologyHelperStruct> WifiTechnologyHelperList;

/**
 * \brief helps to create WifiNetDevice objects
 *
 * This class can help to create a large set of similar
 * WifiNetDevice objects and to configure a large set of
 * their attributes during creation.
 */
class MultiBandWifiHelper
{
public:
  virtual ~MultiBandWifiHelper ();

  /**
   * Create a Wifi helper in an empty state: all its parameters
   * must be set before calling ns3::WifiHelper::Install
   */
  MultiBandWifiHelper ();

  /**
   * \param technology the WifiTechnology to create.
   * \param c the set of nodes on which a wifi multi-band capable device must be created
   * \returns a device container which contains all the devices created by this method.
   */
  virtual NetDeviceContainer Install (const WifiTechnologyHelperList list, NodeContainer c) const;

  /**
   * Helper to enable all WifiNetDevice log components with one statement
   */
  static void EnableLogComponents (void);

};

} //namespace ns3

#endif /* WIFI_HELPER_H */
