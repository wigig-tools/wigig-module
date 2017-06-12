/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#include "multi-band-wifi-helper.h"
#include "ns3/multi-band-net-device.h"
#include "ns3/wifi-mac.h"
#include "ns3/regular-wifi-mac.h"
#include "ns3/dca-txop.h"
#include "ns3/edca-txop-n.h"
#include "ns3/minstrel-wifi-manager.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-remote-station-manager.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/mobility-model.h"
#include "ns3/log.h"
#include "ns3/config.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/names.h"

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
          Ptr<WifiMac> mac = j->MacHelper->Create ();
          Ptr<WifiPhy> phy = j->PhyHelper->Create (node, device);
          Ptr<WifiRemoteStationManager> stationManager = j->RemoteStationManagerFactory.Create<WifiRemoteStationManager> ();
          /* Configure MAC Address (Common for all (MACs)) */
          mac->SetAddress (address);
          mac->ConfigureStandard (j->Standard);
          phy->ConfigureStandard (j->Standard);

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

void
MultiBandWifiHelper::EnableLogComponents (void)
{
//  LogComponentEnable ("Aarfcd", LOG_LEVEL_ALL);
//  LogComponentEnable ("AdhocWifiMac", LOG_LEVEL_ALL);
//  LogComponentEnable ("AmrrWifiRemoteStation", LOG_LEVEL_ALL);
//  LogComponentEnable ("ApWifiMac", LOG_LEVEL_ALL);
//  LogComponentEnable ("ArfWifiManager", LOG_LEVEL_ALL);
//  LogComponentEnable ("Cara", LOG_LEVEL_ALL);
//LogComponentEnable ("ConstantRateWifiManager", LOG_LEVEL_ALL);
//  LogComponentEnable ("DcaTxop", LOG_LEVEL_ALL);
//  LogComponentEnable ("DcfManager", LOG_LEVEL_ALL);
//  LogComponentEnable ("DsssErrorRateModel", LOG_LEVEL_ALL);
//  LogComponentEnable ("EdcaTxopN", LOG_LEVEL_ALL);
//  LogComponentEnable ("InterferenceHelper", LOG_LEVEL_ALL);
//  LogComponentEnable ("Jakes", LOG_LEVEL_ALL);
//  LogComponentEnable ("MacLow", LOG_LEVEL_ALL);
//  LogComponentEnable ("MacRxMiddle", LOG_LEVEL_ALL);
//  LogComponentEnable ("MsduAggregator", LOG_LEVEL_ALL);
//  LogComponentEnable ("MsduStandardAggregator", LOG_LEVEL_ALL);
//  LogComponentEnable ("NistErrorRateModel", LOG_LEVEL_ALL);
//  LogComponentEnable ("OnoeWifiRemoteStation", LOG_LEVEL_ALL);
//  LogComponentEnable ("PropagationLossModel", LOG_LEVEL_ALL);
//  LogComponentEnable ("RegularWifiMac", LOG_LEVEL_ALL);
//  LogComponentEnable ("RraaWifiManager", LOG_LEVEL_ALL);
//  LogComponentEnable ("StaWifiMac", LOG_LEVEL_ALL);
//  LogComponentEnable ("SupportedRates", LOG_LEVEL_ALL);
//  LogComponentEnable ("SensitivityModel60GHz", LOG_LEVEL_ALL);
//  LogComponentEnable ("WifiChannel", LOG_LEVEL_ALL);
//  LogComponentEnable ("WifiMacQueue", LOG_LEVEL_ALL);
//  LogComponentEnable ("WifiNetDevice", LOG_LEVEL_ALL);
//  LogComponentEnable ("WifiPhyStateHelper", LOG_LEVEL_ALL);
//  LogComponentEnable ("WifiPhy", LOG_LEVEL_ALL);
//  LogComponentEnable ("WifiRemoteStationManager", LOG_LEVEL_ALL);
//  LogComponentEnable ("YansErrorRateModel", LOG_LEVEL_ALL);
//  LogComponentEnable ("YansWifiChannel", LOG_LEVEL_ALL);
//  LogComponentEnable ("YansWifiPhy", LOG_LEVEL_ALL);
}

} //namespace ns3
