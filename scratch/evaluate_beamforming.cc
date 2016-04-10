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
#include "ns3/measured-2d-antenna.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

/**
 * This script is used to evaluate IEEE 802.11ad beamforming procedure in BTI + A-BFT. After each BTI and A-BFT access periods
 * we print the selected Transmit Antenna Sector ID for each DMG STA.
 * Network topology is simple and consists of One Access Point + One Station. Each station has one antenna array with
 * eight virutal sectors to cover 360.
 *
 * To run the script type one of the following commands:
 * ./waf --run "evaluate_beamforming --x_pos=1 --y_pos=0"
 * ./waf --run "evaluate_beamforming --x_pos=1 --y_pos=1"
 * ./waf --run "evaluate_beamforming --x_pos=0 --y_pos=1"
 * ./waf --run "evaluate_beamforming --x_pos=-1 --y_pos=1"
 * ./waf --run "evaluate_beamforming --x_pos=-1 --y_pos=0"
 * ./waf --run "evaluate_beamforming --x_pos=-1 --y_pos=-1"
 * ./waf --run "evaluate_beamforming --x_pos=0 --y_pos=-1"
 * ./waf --run "evaluate_beamforming --x_pos=1 --y_pos=-1"
 *
 * To Print the duration of TxSS and RxSS, type the following command:
 * NS_LOG='DmgWifiMac=*|*:DmgApWifiMac=*|*:DmgStaWifiMac=*|*' ./waf --run "evaluate_beamforming" &> debug.txt
 *
 * From the printed output, we calculate the difference in time between the beginning of BTI Period and the time we recevied
 * an SSW-FBCK from DMG AP so we obtain the amount of time to perform TxSS.
 * For RxSS, we calculate the difference in time between the tranmission of first BRP frame till the last one.
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateBeamforming");

using namespace ns3;
using namespace std;

Ptr<Node> apWifiNode;
Ptr<Node> staWifiNode;

Ptr<DmgApWifiMac> apWifiMac;
Ptr<DmgStaWifiMac> staWifiMac;

/*** Access Point Variables ***/
Ptr<PacketSink> sink;
uint64_t lastTotalRx = 0;
double averagethroughput = 0;
uint64_t apMacRx = 0;          /* Total received frames in the PHY. */
double apMacRx_size = 0;       /* Total received frames in the MAC. */
uint64_t lastMacRx = 0;

void
CalculateThroughput ()
{
  Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
  double cur = (sink->GetTotalRx() - lastTotalRx) * (double) 8/1e6;     /* Convert Application RX Packets to MBits. */
  double mac_rx = (apMacRx_size - lastMacRx) * (double) 8/1e6;         /* Convert MAC RX Frames to MBits. */

  std::cout << now.GetSeconds () << '\t' << cur << '\t' << mac_rx << std::endl;
  lastTotalRx = sink->GetTotalRx ();

  averagethroughput += cur;
  lastMacRx = apMacRx_size;

  Simulator::Schedule (Seconds (1), &CalculateThroughput);
}

void
CountFrames (uint64_t *counter, double *sizeAccumulator, const Ptr<const Packet> packet)
{
  (*counter)++;
  *sizeAccumulator += packet->GetSize ();
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
SLSCompleted (Ptr<DmgWifiMac> wifiMac, Mac48Address address,
              ChannelAccessPeriod accessPeriod, SECTOR_ID sectorId, ANTENNA_ID antennaId)
{
  if (wifiMac == apWifiMac)
    {
      std::cout << "DMG AP " << apWifiMac->GetAddress () << " completed SLS phase with DMG STA " << address << std::endl;
    }
  else
    {
      std::cout << "DMG STA " << staWifiMac->GetAddress () << " completed SLS phase with DMG AP " << address << std::endl;
    }
  std::cout << "Best Tx Antenna Configuration: SectorID=" << uint (sectorId) << ", AntennaID=" << uint (antennaId) << std::endl;
}

int
main(int argc, char *argv[])
{
  string applicationType = "onoff";             /* Type of the Tx application */
  string dataRate = "1Gbps";                    /* Application Layer Data Rate. */
  uint32_t payloadSize = 1472;                  /* Transport Layer Payload size in bytes. */
  string socketType = "ns3::UdpSocketFactory";  /* Socket Type (TCP/UDP) */
  uint32_t maxPackets = 0;                      /* Maximum Number of Packets */
  string tcpVariant = "ns3::TcpNewReno";        /* TCP Variant Type. */
  uint32_t bufferSize = 131072;                 /* TCP Send/Receive Buffer Size. */
  string phyMode = "DMG_MCS24";                 /* Type of the Physical Layer. */
  double x_pos = 1.0;                           /* The X position of the DMG STA. */
  double y_pos = 0.0;                           /* The Y position of the DMG STA. */
  bool verbose = false;                         /* Print Logging Information. */
  double simulationTime = 10;                   /* Simulation time in seconds. */
  bool pcapTracing = true;                      /* PCAP Tracing is enabled or not. */

  /* Command line argument parser setup. */
  CommandLine cmd;

  cmd.AddValue ("applicationType", "Type of the Tx Application: onoff or bulk", applicationType);
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("socketType", "Type of the Socket (ns3::TcpSocketFactory, ns3::UdpSocketFactory)", socketType);
  cmd.AddValue ("maxPackets", "Maximum number of packets to send", maxPackets);
  cmd.AddValue ("dataRate", "Payload size in bytes", dataRate);
  cmd.AddValue ("tcpVariant", "Transport protocol to use: TcpTahoe, TcpReno, TcpNewReno, TcpWestwood, TcpWestwoodPlus ", tcpVariant);
  cmd.AddValue ("bufferSize", "TCP Buffer Size (Send/Receive)", bufferSize);
  cmd.AddValue ("phyMode", "802.11ad PHY Mode", phyMode);
  cmd.AddValue ("x_pos", "The X position of the DMG STA", x_pos);
  cmd.AddValue ("y_pos", "The Y position of the DMG STA", y_pos);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable PCAP Tracing", pcapTracing);
  cmd.Parse (argc, argv);

  /* Global params: no fragmentation, no RTS/CTS, fixed rate for all packets */
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));

  /*** Configure TCP Options ***/
  /* Select TCP variant */
  TypeId tid = TypeId::LookupByName (tcpVariant);
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (tid));
  /* Configure TCP Segment Size */
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (bufferSize));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (bufferSize));

  /**** WifiHelper is a meta-helper: it helps creates helpers ****/
  WifiHelper wifi;

  /* Basic setup */
  wifi.SetStandard (WIFI_PHY_STANDARD_80211ad);

  /* Turn on logging */
  if (verbose)
    {
      wifi.EnableLogComponents ();
      LogComponentEnable ("80211ad_test", LOG_LEVEL_ALL);
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
  wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-79));
  wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-79 + 3));
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
  wifiNodes.Create (2);
  apWifiNode = wifiNodes.Get (0);
  staWifiNode = wifiNodes.Get (1);

  /**** Allocate a default Adhoc Wifi MAC ****/
  /* Add a DMG upper mac */
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

  Ssid ssid = Ssid ("test802.11ad");
  wifiMac.SetType ("ns3::DmgApWifiMac",
                   "Ssid", SsidValue(ssid),
                   "QosSupported", BooleanValue (true), "DmgSupported", BooleanValue (true),
                   "BE_MaxAmpduSize", UintegerValue (262143), //Enable A-MPDU with the highest maximum size allowed by the standard
                   "BE_MaxAmsduSize", UintegerValue (0),
                   "SSSlotsPerABFT", UintegerValue (8), "SSFramesPerSlot", UintegerValue (8),
                   "BeaconInterval", TimeValue (MicroSeconds (102400)),
                   "BeaconTransmissionInterval", TimeValue (MicroSeconds (400)),
                   "ATIDuration", TimeValue (MicroSeconds (300)));

  NetDeviceContainer apDevice;
  apDevice = wifi.Install (wifiPhy, wifiMac, apWifiNode);

  wifiMac.SetType ("ns3::DmgStaWifiMac",
                   "Ssid", SsidValue (ssid),
                   "ActiveProbing", BooleanValue (false),
                   "BE_MaxAmpduSize", UintegerValue (262143), //Enable A-MPDU with the highest maximum size allowed by the standard
                   "BE_MaxAmsduSize", UintegerValue (0),
                   "QosSupported", BooleanValue (true), "DmgSupported", BooleanValue (true));

  NetDeviceContainer staDevice;
  staDevice = wifi.Install (wifiPhy, wifiMac, staWifiNode);

  /* Setting mobility model, Initial Position 1 meter apart */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (x_pos, y_pos, 0.0));

  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiNodes);

  /* Internet stack*/
  InternetStackHelper stack;
  stack.Install (wifiNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer apInterface;
  apInterface = address.Assign (apDevice);
  Ipv4InterfaceContainer staInterface;
  staInterface = address.Assign (staDevice);

  /* Populate routing table */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* We do not want any ARP packets */
  PopulateArpCache ();

  /* Install Simple UDP Server on the access point */
  PacketSinkHelper sinkHelper (socketType, InetSocketAddress (Ipv4Address::GetAny (), 9999));
  ApplicationContainer sinkApp = sinkHelper.Install (apWifiNode);
  sink = StaticCast<PacketSink> (sinkApp.Get (0));
  sinkApp.Start (Seconds (0.0));

  /* Install TCP/UDP Transmitter on the station */
  Address dest (InetSocketAddress (apInterface.GetAddress (0), 9999));
  ApplicationContainer srcApp;
  if (applicationType == "onoff")
    {
      OnOffHelper src (socketType, dest);
      src.SetAttribute ("MaxBytes", UintegerValue (0));
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

  srcApp.Start (Seconds (1.0));

  /* Enable Traces */
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
      wifiPhy.EnablePcap ("AccessPoint", apDevice, false);
      wifiPhy.EnablePcap ("Station", staDevice, false);
    }

  /* Since we have one node, so we steer AP antenna sector towarads it */
  Ptr<WifiNetDevice> apWifiNetDevice = StaticCast<WifiNetDevice> (apDevice.Get (0));
  Ptr<WifiNetDevice> staWifiNetDevice = StaticCast<WifiNetDevice> (staDevice.Get (0));
  apWifiMac = StaticCast<DmgApWifiMac> (apWifiNetDevice->GetMac ());
  staWifiMac = StaticCast<DmgStaWifiMac> (staWifiNetDevice->GetMac ());
  Simulator::Schedule (Seconds (0.9), &DmgApWifiMac::ChangeActiveRxSector, apWifiMac,
                       Mac48Address::ConvertFrom (staWifiNetDevice->GetAddress ()));

  /* Accummulate Rx MAC Frames */
  apWifiMac->TraceConnectWithoutContext ("MacRx", MakeBoundCallback(&CountFrames, &apMacRx, &apMacRx_size));

  /* Connect SLS traces */
  apWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, apWifiMac));
  staWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, staWifiMac));

  Simulator::Schedule (Seconds (2.0), &CalculateThroughput);
  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();

  cout << "Average received throughput [Mbps] = " << averagethroughput/(simulationTime - 2) << endl;
  cout << "End Simulation at " << Simulator::Now ().GetSeconds () << endl;

  Simulator::Destroy ();

  return 0;
}
