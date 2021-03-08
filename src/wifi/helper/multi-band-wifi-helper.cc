/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#include "ns3/codebook.h"
#include "ns3/dmg-wifi-mac.h"
#include "ns3/dmg-wifi-phy.h"
#include "ns3/log.h"
#include "ns3/multi-band-net-device.h"
#include "ns3/wifi-remote-station-manager.h"
#include "ns3/mobility-model.h"
#include "multi-band-wifi-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MultiBandWifiHelper");

MultiBandWifiHelper::MultiBandWifiHelper ()
{
}

MultiBandWifiHelper::~MultiBandWifiHelper ()
{
}

NetDeviceContainer
MultiBandWifiHelper::Install (const WifiTechnologyHelperList list, NodeContainer c) const
{
  NetDeviceContainer devices;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<MultiBandNetDevice> device = CreateObject<MultiBandNetDevice> ();
      Mac48Address address = Mac48Address::Allocate ();

      for (WifiTechnologyHelperList::const_iterator j = list.begin (); j != list.end (); ++j)
        {
          /* Create MAC and PHY from helper classes */
          Ptr<WifiMac> mac = j->MacHelper->Create (device);
          Ptr<WifiPhy> phy = j->PhyHelper->Create (node, device);
          Ptr<WifiRemoteStationManager> stationManager = j->RemoteStationManagerFactory.Create<WifiRemoteStationManager> ();
          /* Configure MAC Address (Common for all (MACs)) */
          mac->SetAddress (address);
          mac->ConfigureStandard (j->Standard);
          phy->ConfigureStandard (j->Standard);
          if (j->Standard == WIFI_PHY_STANDARD_80211ad)
            {
              Ptr<Codebook> codebook = j->CodeBookFactory.Create<Codebook> ();
              StaticCast<DmgWifiMac> (mac)->SetCodebook (codebook);
              StaticCast<DmgWifiPhy> (phy)->SetCodebook (codebook);
            }

          device->AddNewWifiTechnology (phy, mac, stationManager, j->Standard, j->Operational);
          /* Register a callback for bandchange */
          mac->RegisterBandChangedCallback (MakeCallback (&MultiBandNetDevice::BandChanged, device));
          /* If this technology is operational, set it to be the default data path */
          if (j->Operational)
            {
              device->SwitchTechnology (j->Standard);
            }
        }

      device->SetAddress (address);
      node->AddDevice (device);
      devices.Add (device);
      NS_LOG_DEBUG ("node=" << node << ", mobility=" << node->GetObject<MobilityModel> ());
    }
  return devices;
}

} //namespace ns3
