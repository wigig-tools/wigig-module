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
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

/**
 * This script is used to evaluate allocation of Static Service Periods in IEEE 802.11ad .
 * The scenario consists of 3 DMG STAs (West + South + East) and one PCP/AP as following:
 *
 *                         DMG AP (0,1)
 *
 *
 * West DMG STA (-1,0)                      East DMG STA (1,0)
 *
 *
 *                      East DMG STA (0,-1)
 *
 * Once all the stations have assoicated successfully with the PCP/AP. The PCP/AP allocates three SPs
 * to perform TxSS between all the stations. Once West DMG STA has completed TxSS phase with East and
 * South DMG STAs. The PCP/AP will allocate two static service periods for communication as following:
 *
 * SP1: DMG West STA -----> DMG East STA (SP Length = 32ms)
 * SP2: DMG West STA -----> DMG South STA (SP Length = 32ms)
 *
 * From the PCAP files, we can see that data transmission takes place during its SP. In addition, we can
 * notice in the announcement of the two Static Allocation Periods inside each DMG Beacon.
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateServicePeriod");

using namespace ns3;
using namespace std;

/* Network Nodes */
Ptr<WifiNetDevice> apWifiNetDevice;
Ptr<WifiNetDevice> westWifiNetDevice;
Ptr<WifiNetDevice> southWifiNetDevice;
Ptr<WifiNetDevice> eastWifiNetDevice;

NetDeviceContainer staDevices;

Ptr<DmgApWifiMac> apWifiMac;
Ptr<DmgStaWifiMac> westWifiMac;
Ptr<DmgStaWifiMac> southWifiMac;
Ptr<DmgStaWifiMac> eastWifiMac;

/*** Access Point Variables ***/
uint8_t assoicatedStations = 0;           /* Total number of assoicated stations with the AP */
uint8_t stationsTrained = 0;              /* Number of BF trained stations */
bool scheduledStaticPeriods = false;      /* Flag to indicate whether we scheduled Static Service Periods or not */

void
CalculateThroughput (Ptr<PacketSink> sink, uint64_t lastTotalRx, double averageThroughput)
{
  Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
  double cur = (sink->GetTotalRx() - lastTotalRx) * (double) 8/1e5;     /* Convert Application RX Packets to MBits. */
  std::cout << now.GetSeconds () << '\t' << cur << std::endl;
  lastTotalRx = sink->GetTotalRx ();
  averageThroughput += cur;
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput, sink, lastTotalRx, averageThroughput);
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

void
StationAssoicated (Ptr<DmgStaWifiMac> staWifiMac, Mac48Address address)
{
  std::cout << "DMG STA " << staWifiMac->GetAddress () << " associated with DMG AP " << address << std::endl;
  std::cout << "Association ID (AID) = " << staWifiMac->GetAssociationID () << std::endl;
  assoicatedStations++;
  /* Map AID to MAC Addresses in each node instead of requesting information */
  Ptr<DmgStaWifiMac> dmgStaMac;
  for (NetDeviceContainer::Iterator i = staDevices.Begin (); i != staDevices.End (); ++i)
    {
      dmgStaMac = StaticCast<DmgStaWifiMac> (StaticCast<WifiNetDevice> (*i)->GetMac ());
      if (dmgStaMac != staWifiMac)
        {
          dmgStaMac->MapAidToMacAddress (staWifiMac->GetAssociationID (), staWifiMac->GetAddress ());
        }
    }
  /* Check if all stations have assoicated with the AP */
  if (assoicatedStations == 3)
    {
      std::cout << "All stations got associated with " << address << std::endl;
      /* Schedule Beamforming Training SP */
      apWifiMac->AllocateBeamformingServicePeriod (westWifiMac->GetAssociationID (), eastWifiMac->GetAssociationID (), 0, true);
      apWifiMac->AllocateBeamformingServicePeriod (westWifiMac->GetAssociationID (), southWifiMac->GetAssociationID (), 500, true);
      apWifiMac->AllocateBeamformingServicePeriod (southWifiMac->GetAssociationID (), eastWifiMac->GetAssociationID (), 1000, true);
    }
}

void
SLSCompleted (Ptr<DmgStaWifiMac> staWifiMac, Mac48Address address,
              ChannelAccessPeriod accessPeriod, SECTOR_ID sectorId, ANTENNA_ID antennaId)
{
  if (accessPeriod == CHANNEL_ACCESS_DTI)
    {
      std::cout << "DMG STA " << staWifiMac->GetAddress () << " completed SLS phase with DMG STA " << address << std::endl;
      if ((westWifiMac->GetAddress () == staWifiMac->GetAddress ()) &&
          ((southWifiMac->GetAddress () == address) || (eastWifiMac->GetAddress () == address)))
        {
          stationsTrained++;
        }
      if ((stationsTrained == 2) & !scheduledStaticPeriods)
        {
          std::cout << "West DMG STA " << staWifiMac->GetAddress () << " completed SLS phase with South and East DMG STAs " << std::endl;
          std::cout << "Schedule Static Periods" << std::endl;
          scheduledStaticPeriods = true;
          /* Schedule Static Periods */
          apWifiMac->AddAllocationPeriod (SERVICE_PERIOD_ALLOCATION, true, westWifiMac->GetAssociationID (),
                                          eastWifiMac->GetAssociationID (), 0, 32000);
          apWifiMac->AddAllocationPeriod (SERVICE_PERIOD_ALLOCATION, true, westWifiMac->GetAssociationID (),
                                          southWifiMac->GetAssociationID (), 36000, 32000);
        }
    }
}

int
main (int argc, char *argv[])
{
  uint32_t payloadSize = 1472;                  /* Transport Layer Payload size in bytes. */
  string dataRate = "300Mbps";                  /* Application Layer Data Rate. */
  uint32_t queueSize = 10000;                   /* Wifi Mac Queue Size. */
  string phyMode = "DMG_MCS24";                 /* Type of the Physical Layer. */
  bool verbose = false;                         /* Print Logging Information. */
  double simulationTime = 10;                   /* Simulation time in seconds. */
  bool pcapTracing = false;                     /* PCAP Tracing is enabled or not. */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("dataRate", "Payload size in bytes", dataRate);
  cmd.AddValue ("queueSize", "The size of the Wifi Mac Queue", queueSize);
  cmd.AddValue ("phyMode", "802.11ad PHY Mode", phyMode);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable PCAP Tracing", pcapTracing);
  cmd.Parse (argc, argv);

  /* Global params: no fragmentation, no RTS/CTS, fixed rate for all packets */
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::WifiMacQueue::MaxPacketNumber", UintegerValue (queueSize));

  /**** WifiHelper is a meta-helper: it helps creates helpers ****/
  WifiHelper wifi;

  /* Basic setup */
  wifi.SetStandard (WIFI_PHY_STANDARD_80211ad);

  /* Turn on logging */
  if (verbose)
    {
      wifi.EnableLogComponents ();
      LogComponentEnable ("EvaluateServicePeriod", LOG_LEVEL_ALL);
    }

  /**** Set up Channel ****/
  YansWifiChannelHelper wifiChannel ;
  /* Simple propagation delay model */
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  /* Friis model with standard-specific wavelength */
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (56.16e9));

  /**** SETUP ALL NODES ****/
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  /* Nodes will be added to the channel we set up earlier */
  wifiPhy.SetChannel (wifiChannel.Create ());
  /* All nodes transmit at 10 dBm == 10 mW, no adaptation */
  wifiPhy.Set ("TxPowerStart", DoubleValue (10.0));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (10.0));
  wifiPhy.Set ("TxPowerLevels", UintegerValue (1));
  wifiPhy.Set ("TxGain", DoubleValue (0));
  wifiPhy.Set ("RxGain", DoubleValue (0));
  /* Sensitivity model includes implementation loss and noise figure */
  wifiPhy.Set ("RxNoiseFigure", DoubleValue (3));
  wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-60));
  wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-60 + 3));
  /* Set the phy layer error model */
  wifiPhy.SetErrorRateModel ("ns3::SensitivityModel60GHz");
  /* Set default algorithm for all nodes to be constant rate */
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "ControlMode", StringValue (phyMode),
                                                                "DataMode", StringValue (phyMode));
  /* Give all nodes steerable antenna */
  wifiPhy.EnableAntenna (true, true);
  wifiPhy.SetAntenna ("ns3::Directional60GhzAntenna",
                      "Sectors", UintegerValue (8),
                      "Antennas", UintegerValue (1),
                      "AngleOffset", DoubleValue (0));

  /* Make two nodes and set them up with the phy and the mac */
  NodeContainer wifiNodes;
  wifiNodes.Create (4);
  Ptr<Node> apNode = wifiNodes.Get (0);
  Ptr<Node> westNode = wifiNodes.Get (1);
  Ptr<Node> southNode = wifiNodes.Get (2);
  Ptr<Node> eastNode = wifiNodes.Get (3);

  /**** Allocate a default Adhoc Wifi MAC ****/
  /* Add a DMG upper mac */
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

  /* Install PCP/AP Node */
  Ssid ssid = Ssid ("test802.11ad");
  wifiMac.SetType ("ns3::DmgApWifiMac",
                   "Ssid", SsidValue(ssid),
                   "QosSupported", BooleanValue (true), "DmgSupported", BooleanValue (true),
                   "BE_MaxAmpduSize", UintegerValue (0), //Enable A-MPDU with the highest maximum size allowed by the standard
                   "BE_MaxAmsduSize", UintegerValue (7935),
                   "SSSlotsPerABFT", UintegerValue (8), "SSFramesPerSlot", UintegerValue (8),
                   "BeaconInterval", TimeValue (MicroSeconds (102400)),
                   "BeaconTransmissionInterval", TimeValue (MicroSeconds (600)),
                   "ATIDuration", TimeValue (MicroSeconds (1000)));

  NetDeviceContainer apDevice;
  apDevice = wifi.Install (wifiPhy, wifiMac, apNode);

  /* Install DMG STA Nodes */
  wifiMac.SetType ("ns3::DmgStaWifiMac",
                   "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false),
                   "BE_MaxAmpduSize", UintegerValue (0), //Enable A-MPDU with the highest maximum size allowed by the standard
                   "BE_MaxAmsduSize", UintegerValue (7935),
                   "QosSupported", BooleanValue (true), "DmgSupported", BooleanValue (true));

  staDevices = wifi.Install (wifiPhy, wifiMac, NodeContainer (westNode, southNode, eastNode));

  /* Setting mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, +1.0, 0.0));   /* PCP/AP */
  positionAlloc->Add (Vector (-1.0, 0.0, 0.0));   /* West STA */
  positionAlloc->Add (Vector (0.0, -1.0, 0.0));   /* South STA */
  positionAlloc->Add (Vector (+1.0, 0.0, 0.0));   /* East STA */

  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiNodes);

  /* Internet stack*/
  InternetStackHelper stack;
  stack.Install (wifiNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer ApInterface;
  ApInterface = address.Assign (apDevice);
  Ipv4InterfaceContainer staInterfaces;
  staInterfaces = address.Assign (staDevices);

  /* Populate routing table */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* We do not want any ARP packets */
  PopulateArpCache ();

  /*** Install Applications ***/

  /* Install Simple UDP Server on east Node */
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9999));
  ApplicationContainer sinks = sinkHelper.Install (NodeContainer (eastNode, southNode));

  /** East Node Variables **/
  uint64_t eastNodeLastTotalRx = 0;
  double eastNodeAverageThroughput = 0;

  /* Install Simple UDP Transmitter on west node (Transmit to East Node) */
  ApplicationContainer srcApp1;
  OnOffHelper src1 ("ns3::UdpSocketFactory", InetSocketAddress (staInterfaces.GetAddress (2), 9999));
  src1.SetAttribute ("MaxBytes", UintegerValue (0));
  src1.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  src1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
  src1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  src1.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  srcApp1 = src1.Install (westNode);
  srcApp1.Start (Seconds (3.0));

  /** South Node Variables **/
  uint64_t southNodeLastTotalRx = 0;
  double southNodeAverageThroughput = 0;

  /* Install Simple UDP Transmitter on west node (Transmit to South Node) */
  ApplicationContainer srcApp2;
  OnOffHelper src2 ("ns3::UdpSocketFactory", InetSocketAddress (staInterfaces.GetAddress (1), 9999));
  src2.SetAttribute ("MaxBytes", UintegerValue (0));
  src2.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  src2.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
  src2.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  src2.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  srcApp2 = src2.Install (westNode);
  srcApp2.Start (Seconds (3.0));

  /* Schedule Throughput Calulcations */
  Simulator::Schedule (Seconds (3.1), &CalculateThroughput, StaticCast<PacketSink> (sinks.Get (0)),
                       eastNodeLastTotalRx, eastNodeAverageThroughput);

  Simulator::Schedule (Seconds (3.1), &CalculateThroughput, StaticCast<PacketSink> (sinks.Get (1)),
                       southNodeLastTotalRx, southNodeAverageThroughput);

  /* Enable Traces */
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
      wifiPhy.EnablePcap ("AccessPoint", apDevice, false);
      wifiPhy.EnablePcap ("WestNode", staDevices.Get (0), false);
      wifiPhy.EnablePcap ("SouthNode", staDevices.Get (0), false);
      wifiPhy.EnablePcap ("EastNode", staDevices.Get (0), false);
    }

  /* Stations */
  apWifiNetDevice = StaticCast<WifiNetDevice> (apDevice.Get (0));
  westWifiNetDevice = StaticCast<WifiNetDevice> (staDevices.Get (0));
  southWifiNetDevice = StaticCast<WifiNetDevice> (staDevices.Get (1));
  eastWifiNetDevice = StaticCast<WifiNetDevice> (staDevices.Get (2));

  apWifiMac = StaticCast<DmgApWifiMac> (apWifiNetDevice->GetMac ());
  westWifiMac = StaticCast<DmgStaWifiMac> (westWifiNetDevice->GetMac ());
  southWifiMac = StaticCast<DmgStaWifiMac> (southWifiNetDevice->GetMac ());
  eastWifiMac = StaticCast<DmgStaWifiMac> (eastWifiNetDevice->GetMac ());

  /** Connect Traces **/
  westWifiMac->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, westWifiMac));
  southWifiMac->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, southWifiMac));
  eastWifiMac->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, eastWifiMac));

  westWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, westWifiMac));
  southWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, southWifiMac));
  eastWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, eastWifiMac));

  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
