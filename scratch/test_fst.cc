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
#include "ns3/applications-module.h"
#include "ns3/cone-antenna.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"

/**
 * This script is used to test 802.11ad Fast Session Transfer Mechanism with LLT=0.
 * Network topology is simple and consists of One Access Point + One Station operating initially
 * in the 60GHz band.
 *
 * To use this script simply type the following run command:
 * ./waf --run "test_fst --dataRate=5Gbps"
 *
 * To generate PCAP files, type the following run command:
 * ./waf --run "test_fst --dataRate=5Gbps --pcap=1"
 *
 * The simulation generates two PCAP files for each node. One PCAP file corresponds to 11ad Band
 * and the other PCAP for 11n band. In the 11ad PCAP files, you can check the setup of FSTS.
 * In the 11n PCAP file you can see the exchange of FST ACK Request/Response frames.
 *
 */

NS_LOG_COMPONENT_DEFINE ("test_fst");

using namespace ns3;
using namespace std;

Ptr<MultiBandNetDevice> staMultibandDevice;
Ptr<MultiBandNetDevice> apMultibandDevice;

/*** Access Point Variables ***/
Ptr<PacketSink> sink;
uint64_t lastTotalRx = 0;
double averagethroughput = 0;

void
CalculateThroughput ()
{
  Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
  double cur = (sink->GetTotalRx() - lastTotalRx) * (double) 8/1e5;     /* Convert Application RX Packets to MBits. */
  std::cout << now.GetSeconds () << '\t' << cur << std::endl;
  lastTotalRx = sink->GetTotalRx ();
  averagethroughput += cur;
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

void
PopulateArpCache (void)
{
  Ptr<ArpCache> arp = CreateObject<ArpCache> ();
  arp->SetAliveTimeout (Seconds (3600 * 24 * 365));

  for (NodeList::Iterator i = NodeList::Begin (); i != NodeList::End (); ++i)
    {
      Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
      NS_ASSERT (ip != 0);
      ObjectVectorValue interfaces;
      ip->GetAttribute ("InterfaceList", interfaces);
      for (ObjectVectorValue::Iterator j = interfaces.Begin (); j != interfaces.End (); j ++)
        {
          Ptr<Ipv4Interface> ipIface = (j->second)->GetObject<Ipv4Interface> ();
          NS_ASSERT (ipIface != 0);
          Ptr<NetDevice> device = ipIface->GetDevice ();
          NS_ASSERT (device != 0);
          Mac48Address addr = Mac48Address::ConvertFrom(device->GetAddress ());
          for (uint32_t k = 0; k < ipIface->GetNAddresses (); k++)
            {
              Ipv4Address ipAddr = ipIface->GetAddress (k).GetLocal ();
              if (ipAddr == Ipv4Address::GetLoopback ())
                continue;
              ArpCache::Entry *entry = arp->Add (ipAddr);
              entry->MarkWaitReply (0);
              entry->MarkAlive (addr);
            }
        }
    }

  for (NodeList::Iterator i = NodeList::Begin (); i != NodeList::End (); ++i)
    {
      Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
      NS_ASSERT (ip != 0);
      ObjectVectorValue interfaces;
      ip->GetAttribute("InterfaceList", interfaces);
      for(ObjectVectorValue::Iterator j = interfaces.Begin (); j != interfaces.End (); j ++)
        {
          Ptr<Ipv4Interface> ipIface = (j->second)->GetObject<Ipv4Interface> ();
          ipIface->SetAttribute ("ArpCache", PointerValue (arp));
        }
    }
}

int
main(int argc, char *argv[])
{
  string applicationType = "onoff";             /* Type of the Tx application */
  uint32_t payloadSize = 1472;                  /* Transport Layer Payload size in bytes. */
  string dataRate = "100Mbps";                  /* Application Layer Data Rate. */
  string socketType = "ns3::UdpSocketFactory";  /* Socket Type (TCP/UDP) */
  uint32_t maxPackets = 0;                      /* Maximum Number of Packets */
  string tcpVariant = "ns3::TcpNewReno";        /* TCP Variant Type. */
  uint32_t bufferSize = 131072;                 /* TCP Send/Receive Buffer Size. */
  uint32_t queueSize = 10000;                   /* Wifi Mac Queue Size. */
  double fstTime = 5;                           /* Time to transfer current session. */
  string adPhyMode = "DMG_MCS24";               /* Type of the 802.11ad Physical Layer. */
  string nPhyMode = "HtMcs6";                   /* Type of the 802.11n Physical Layer. */
  double simulationTime = 10;                   /* Simulation time in seconds. */
  bool pcapTracing = false;                     /* PCAP Tracing is enabled or not. */

  /* Command line argument parser setup. */
  CommandLine cmd;

  cmd.AddValue ("applicationType", "Type of the Tx Application: onoff, bulk, myApp", applicationType);
  cmd.AddValue ("socketType", "Type of the Socket (ns3::TcpSocketFactory, ns3::UdpSocketFactory)", socketType);
  cmd.AddValue ("maxPackets", "Maximum number of packets to send", maxPackets);
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("dataRate", "Payload size in bytes", dataRate);
  cmd.AddValue ("tcpVariant", "Transport protocol to use: TcpTahoe, TcpReno, TcpNewReno, TcpWestwood, TcpWestwoodPlus ", tcpVariant);
  cmd.AddValue ("bufferSize", "TCP Buffer Size (Send/Receive)", bufferSize);
  cmd.AddValue ("transferSession", "Time to transfer current session", fstTime);
  cmd.AddValue ("queueSize", "The size of the Wifi Mac Queue", queueSize);
  cmd.AddValue ("adPhyMode", "802.11ad PHY Mode", adPhyMode);
  cmd.AddValue ("nPhyMode", "802.11n PHY Mode", nPhyMode);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable PCAP Tracing", pcapTracing);
  cmd.Parse (argc, argv);

  /* Global params: no fragmentation, no RTS/CTS, fixed rate for all packets */
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::WifiMacQueue::MaxPacketNumber", UintegerValue (queueSize));

  /*** Configure TCP Options ***/
  /* Select TCP variant */
  TypeId tid = TypeId::LookupByName (tcpVariant);
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (tid));
  /* Configure TCP Segment Size */
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (bufferSize));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (bufferSize));

  /**** Allocate 802.11ad Wifi MAC ****/
  /* Add a DMG upper mac */
  DmgWifiMacHelper adWifiMac = DmgWifiMacHelper::Default ();

  /**** Set up 60 Ghz Channel ****/
  YansWifiChannelHelper adWifiChannel ;
  /* Simple propagation delay model */
  adWifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  /* Friis model with standard-specific wavelength */
  adWifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (56.16e9));

  /**** Setup Physical Layer ****/
  YansWifiPhyHelper adWifiPhy = YansWifiPhyHelper::Default ();
  adWifiPhy.SetChannel (adWifiChannel.Create ());
  adWifiPhy.Set ("TxPowerStart", DoubleValue (10.0));
  adWifiPhy.Set ("TxPowerEnd", DoubleValue (10.0));
  adWifiPhy.Set ("TxPowerLevels", UintegerValue (1));
  adWifiPhy.Set ("TxGain", DoubleValue (0));
  adWifiPhy.Set ("RxGain", DoubleValue (0));
  adWifiPhy.Set("RxNoiseFigure", DoubleValue (10));
  adWifiPhy.Set ("CcaMode1Threshold", DoubleValue (-79));
  adWifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-79 + 3));
  adWifiPhy.SetErrorRateModel ("ns3::SensitivityModel60GHz");
  ObjectFactory adRemoteStationManager = ObjectFactory ();
  adRemoteStationManager.SetTypeId ("ns3::ConstantRateWifiManager");
  adRemoteStationManager.Set ("ControlMode", StringValue (adPhyMode));
  adRemoteStationManager.Set ("DataMode", StringValue (adPhyMode));

  adWifiPhy.EnableAntenna (true, true);
  adWifiPhy.SetAntenna ("ns3::Directional60GhzAntenna",
                        "Sectors", UintegerValue (4),
                        "Antennas", UintegerValue (1),
                        "AngleOffset", DoubleValue (M_PI/4));

  /* 802.11ad Structure */
  WifiTechnologyHelperStruct adWifiStruct;
  adWifiStruct.MacHelper = &adWifiMac;
  adWifiStruct.PhyHelper = &adWifiPhy;
  adWifiStruct.RemoteStationManagerFactory = adRemoteStationManager;
  adWifiStruct.Standard = WIFI_PHY_STANDARD_80211ad;
  adWifiStruct.Operational = true;

  /**** Allocate 802.11ad Wifi MAC ****/
  /* Add a DMG upper mac */
  HtWifiMacHelper nWifiMac = HtWifiMacHelper::Default ();

  /**** Set up Legacy Channel ****/
  YansWifiChannelHelper nWifiChannel ;
  nWifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  nWifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (2.4e9));

  /**** Setup Physical Layer ****/
  YansWifiPhyHelper nWifiPhy = YansWifiPhyHelper::Default ();
  nWifiPhy.SetChannel (nWifiChannel.Create ());
  nWifiPhy.Set ("TxPowerStart", DoubleValue (10.0));
  nWifiPhy.Set ("TxPowerEnd", DoubleValue (10.0));
  nWifiPhy.Set ("TxPowerLevels", UintegerValue (1));
  nWifiPhy.Set ("TxGain", DoubleValue (0));
  nWifiPhy.Set ("RxGain", DoubleValue (0));
  nWifiPhy.Set ("RxNoiseFigure", DoubleValue (10));
  nWifiPhy.Set ("CcaMode1Threshold", DoubleValue (-79));
  nWifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-79 + 3));
  nWifiPhy.SetErrorRateModel ("ns3::YansErrorRateModel");
  nWifiPhy.EnableAntenna (false, false);
  // Set channel width
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/ChannelWidth", UintegerValue (20));
  ObjectFactory nRemoteStationManager = ObjectFactory ();
  nRemoteStationManager.SetTypeId ("ns3::ConstantRateWifiManager");
  nRemoteStationManager.Set ("ControlMode", StringValue (nPhyMode));
  nRemoteStationManager.Set ("DataMode", StringValue (nPhyMode));

  /* 802.11n Structure */
  WifiTechnologyHelperStruct legacyWifiStruct;
  legacyWifiStruct.MacHelper = &nWifiMac;
  legacyWifiStruct.PhyHelper = &nWifiPhy;
  legacyWifiStruct.RemoteStationManagerFactory = nRemoteStationManager;
  legacyWifiStruct.Standard = WIFI_PHY_STANDARD_80211n_5GHZ;
  legacyWifiStruct.Operational = false;

  /*** Create Network Topology ***/
  PointToPointHelper p2pHelper;
  p2pHelper.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
  p2pHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (50)));
  p2pHelper.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (5000));

  NodeContainer networkNodes;
  networkNodes.Create (3);
  Ptr<Node> serverNode = networkNodes.Get (0);
  Ptr<Node> apWifiNode = networkNodes.Get (1);
  Ptr<Node> staWifiNode = networkNodes.Get (2);

  NetDeviceContainer backboneDevices;
  backboneDevices = p2pHelper.Install (serverNode, apWifiNode);

  /* Technologies List*/
  WifiTechnologyHelperList technologyList;
  technologyList.push_back (adWifiStruct);
  technologyList.push_back (legacyWifiStruct);

  MultiBandWifiHelper multibandHelper;
  /* Configure AP with different wifi technologies */
  Ssid ssid = Ssid ("network");
  adWifiMac.SetType ("ns3::DmgApWifiMac",
                     "Ssid", SsidValue (ssid),
                     "QosSupported", BooleanValue (true), "DmgSupported", BooleanValue (true),
                     "BE_MaxAmpduSize", UintegerValue (262143), //Enable A-MPDU with the highest maximum size allowed by the standard
                     "BE_MaxAmsduSize", UintegerValue (7935),
                     "SSSlotsPerABFT", UintegerValue (8), "SSFramesPerSlot", UintegerValue (8),
                     "BeaconInterval", TimeValue (MicroSeconds (102400)),
                     "BeaconTransmissionInterval", TimeValue (MicroSeconds (400)),
                     "ATIDuration", TimeValue (MicroSeconds (300)));

  nWifiMac.SetType ("ns3::ApWifiMac",
                    "Ssid", SsidValue (ssid),
                    "QosSupported", BooleanValue (true), "HtSupported", BooleanValue (true),
                    "BE_MaxAmpduSize", UintegerValue (65535), //Enable A-MPDU with the highest maximum size allowed by the standard
                    "BE_MaxAmsduSize", UintegerValue (7935));

  NetDeviceContainer apDevice;
  apDevice = multibandHelper.Install (technologyList, apWifiNode);

  /* Configure STA with different wifi technologies */
  adWifiMac.SetType ("ns3::DmgStaWifiMac",
                     "Ssid", SsidValue (ssid),
                     "ActiveProbing", BooleanValue (false),
                     "BE_MaxAmpduSize", UintegerValue (262143), //Enable A-MPDU with the highest maximum size allowed by the standard
                     "BE_MaxAmsduSize", UintegerValue (7935),
                     "QosSupported", BooleanValue (true), "DmgSupported", BooleanValue (true),
                     "LLT", UintegerValue (0));

  nWifiMac.SetType ("ns3::StaWifiMac",
                    "Ssid", SsidValue (ssid),
                    "ActiveProbing", BooleanValue (false),
                    "BE_MaxAmpduSize", UintegerValue (65535), //Enable A-MPDU with the highest maximum size allowed by the standard
                    "BE_MaxAmsduSize", UintegerValue (7935),
                    "QosSupported", BooleanValue (true), "HtSupported", BooleanValue (true));

  NetDeviceContainer staDevices;
  staDevices = multibandHelper.Install (technologyList, staWifiNode);

  /* Setting mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (1.0, 1.0, 0.0));

  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (apWifiNode);
  mobility.Install (staWifiNode);

  /* Internet stack*/
  InternetStackHelper stack;
  stack.Install (networkNodes);

  Ipv4AddressHelper address;
  address.SetBase ("192.168.0.0", "255.255.255.0");
  Ipv4InterfaceContainer backboneInterfaces;
  backboneInterfaces = address.Assign (backboneDevices);

  address.SetBase ("192.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer apInterface;
  apInterface = address.Assign (apDevice);
  Ipv4InterfaceContainer staInterface;
  staInterface = address.Assign (staDevices);

  /* Populate routing table */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* We do not want any ARP packets */
  PopulateArpCache ();

  /* Install TCP/UDP Receiver on the access point */
  PacketSinkHelper sinkHelper (socketType, InetSocketAddress (Ipv4Address::GetAny (), 9999));
  ApplicationContainer sinkApp = sinkHelper.Install (serverNode);
  sink = StaticCast<PacketSink> (sinkApp.Get (0));
  sinkApp.Start (Seconds (0.0));

  /* Install TCP/UDP Transmitter on the station */
  Address dest (InetSocketAddress (backboneInterfaces.GetAddress (0), 9999));
  ApplicationContainer srcApp;
  if (applicationType == "onoff")
    {
      OnOffHelper src (socketType, dest);
      src.SetAttribute ("MaxBytes", UintegerValue (payloadSize * maxPackets));
      src.SetAttribute ("PacketSize", UintegerValue (payloadSize));
      src.SetAttribute ("PacketSize", UintegerValue (payloadSize));
      src.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
      src.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      src.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));

      srcApp = src.Install (staWifiNode);
    }
  else if (applicationType == "bulk")
    {
      BulkSendHelper src (socketType, dest);
      srcApp= src.Install (staWifiNode);
    }

  /* Start Application */
  srcApp.Start (Seconds (1.0));
  Simulator::Schedule (Seconds (1.1), &CalculateThroughput);

  /* Enable Traces */
  if (pcapTracing)
    {
      Ptr<MultiBandNetDevice> apMultiBandDevice = StaticCast<MultiBandNetDevice> (apDevice.Get (0));
      Ptr<MultiBandNetDevice> staMultiBandDevice = StaticCast<MultiBandNetDevice> (staDevices.Get (0));
      WifiTechnologyList apTechnologyList = apMultiBandDevice->GetWifiTechnologyList ();
      WifiTechnologyList staTechnologyList = staMultiBandDevice->GetWifiTechnologyList ();

      p2pHelper.EnablePcap ("EndServer", backboneDevices.Get (0));

      adWifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
      nWifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

      /* AP Technologies PCAP */
      for (WifiTechnologyList::iterator item = apTechnologyList.begin (); item != apTechnologyList.end (); item++)
        {
          if (item->first == WIFI_PHY_STANDARD_80211ad)
            {
              adWifiPhy.EnableMultiBandPcap ("adAccessPoint", apMultiBandDevice, item->second.Phy);
            }
          else if (item->first == WIFI_PHY_STANDARD_80211n_5GHZ)
            {
              nWifiPhy.EnableMultiBandPcap ("nAccessPoint", apMultiBandDevice, item->second.Phy);
            }
        }
      /* STA Technologies PCAP */
      for (WifiTechnologyList::iterator item = staTechnologyList.begin (); item != staTechnologyList.end (); item++)
        {
          if (item->first == WIFI_PHY_STANDARD_80211ad)
            {
              adWifiPhy.EnableMultiBandPcap ("adStation", staMultiBandDevice, item->second.Phy);
            }
          else if (item->first == WIFI_PHY_STANDARD_80211n_5GHZ)
            {
              nWifiPhy.EnableMultiBandPcap ("nStation", staMultiBandDevice, item->second.Phy);
            }
        }
    }

  /* Variables */
  staMultibandDevice = StaticCast<MultiBandNetDevice> (staDevices.Get (0));
  apMultibandDevice = StaticCast<MultiBandNetDevice> (apDevice.Get (0));

  /* Schedule for FST event, STA is the Initiator */
  Simulator::Schedule (Seconds (fstTime), &MultiBandNetDevice::EstablishFastSessionTransferSession, staMultibandDevice,
                       Mac48Address::ConvertFrom (apMultibandDevice->GetAddress ()));

  /* Start Simulation */
  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
