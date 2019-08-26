/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
#include "common-functions.h"

/**
 * Simulation Objective:
 * This script is used to evaluate the throughput achieved using a simple allocation of a static
 * service period for a communication from DMG PCP/AP to a DMG STA.
 *
 * Network Topology:
 * The scenario consists of a single DMG STA and one DMG PCP/AP as following:
 *
 *
 *                  DMG PCP/AP (0,0)          DMG STA (+1,0)
 *
 *
 * Simulation Description:
 * Once the statio have associated successfully with the DMG PCP/AP. The DMG PCP/AP allocates a signle
 * static service period for communication as following:
 *
 * SP: DMG PCP/AP -----> DMG STA (SP Length = 3.2 ms)
 *
 * Running the Simulation:
 * To run the script with the default parameters:
 * ./waf --run "evaluate_simple_service_period"
 *
 * To run the script with different duration for the service period e.g. SP1=10ms:
 * ./waf --run "evaluate_service_period_udp --spDuration=10000"
 *
 * Simulation Output:
 * The simulation generates the following traces:
 * 1. PCAP traces for each station. From the PCAP files, we can see that data transmission takes place
 * during its SP. In addition, we can notice in the announcement of the two Static Allocation Periods
 * inside each DMG Beacon.
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateSimpleServicePeriod");

using namespace ns3;
using namespace std;

/**  Application Variables **/
Ptr<PacketSink> packetSink;

/* Network Nodes */
Ptr<WifiNetDevice> apWifiNetDevice;
Ptr<WifiNetDevice> staWifiNetDevice;
Ptr<DmgApWifiMac> apWifiMac;
Ptr<DmgStaWifiMac> staWifiMac;
NetDeviceContainer staDevices;

/*** Service Periods ***/
uint16_t spDuration = 3200;               /* The duration of the allocated service period in MicroSeconds */

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
StationAssociated (Mac48Address address, uint16_t aid)
{
  std::cout << "DMG STA: " << staWifiMac->GetAddress () << " associated with DMG PCP/AP: " << address << std::endl;
  std::cout << "Association ID (AID) = " << aid << std::endl;
  std::cout << "Schedule Static Service Period (DMG PCP/AP ----> DMG STA)" << std::endl;
  /* Schedule Static Periods */
  apWifiMac->AllocateSingleContiguousBlock (1, SERVICE_PERIOD_ALLOCATION, true,
                                            AID_AP,
                                            aid,
                                            0, spDuration);
}

/**
 * Callback method to log the number of packets in the Wifi MAC Queue.
 */
void
QueueOccupancyChange (Ptr<OutputStreamWrapper> file, uint32_t oldValue, uint32_t newValue)
{
  std::ostream *output = file->GetStream ();
  *output << Simulator::Now ().GetNanoSeconds () << "," << newValue << endl;
}

int
main (int argc, char *argv[])
{
  uint32_t packetSize = 1448;                   /* Transport Layer Payload size in bytes. */
  string dataRate = "300Mbps";                  /* Application Layer Data Rate. */
  uint64_t maxPackets = 0;                      /* The maximum number of packets to transmit. */
  uint32_t msduAggregationSize = 7935;          /* The maximum aggregation size for A-MSDU in Bytes. */
  uint32_t queueSize = 10000;                   /* Wifi Mac Queue Size. */
  string phyMode = "DMG_MCS12";                 /* Type of the Physical Layer. */
  bool verbose = false;                         /* Print Logging Information. */
  double simulationTime = 10;                   /* Simulation time in seconds. */
  bool pcapTracing = false;                     /* PCAP Tracing is enabled or not. */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("packetSize", "Payload size in bytes", packetSize);
  cmd.AddValue ("dataRate", "Data rate for OnOff Application", dataRate);
  cmd.AddValue ("maxPackets", "The maximum number of packets to transmit", maxPackets);
  cmd.AddValue ("msduAggregation", "The maximum aggregation size for A-MSDU in Bytes", msduAggregationSize);
  cmd.AddValue ("queueSize", "The size of the Wifi Mac Queue", queueSize);
  cmd.AddValue ("spDuration", "The duration of service period in MicroSeconds", spDuration);
  cmd.AddValue ("phyMode", "802.11ad PHY Mode", phyMode);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable PCAP Tracing", pcapTracing);
  cmd.Parse (argc, argv);

  /* Global params: no fragmentation, no RTS/CTS, fixed rate for all packets */
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));

  /**** DmgWifiHelper is a meta-helper ****/
  DmgWifiHelper wifi;

  /* Basic setup */
  wifi.SetStandard (WIFI_PHY_STANDARD_80211ad);

  /* Turn on logging */
  if (verbose)
    {
      wifi.EnableLogComponents ();
      LogComponentEnable ("EvaluateSimpleServicePeriod", LOG_LEVEL_ALL);
    }

  /**** Set up Channel ****/
  DmgWifiChannelHelper wifiChannel ;
  /* Simple propagation delay model */
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  /* Friis model with standard-specific wavelength */
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (60.48e9));

  /**** Setup physical layer ****/
  DmgWifiPhyHelper wifiPhy = DmgWifiPhyHelper::Default ();
  /* Nodes will be added to the channel we set up earlier */
  wifiPhy.SetChannel (wifiChannel.Create ());
  /* All nodes transmit at 10 dBm == 10 mW, no adaptation */
  wifiPhy.Set ("TxPowerStart", DoubleValue (10.0));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (10.0));
  wifiPhy.Set ("TxPowerLevels", UintegerValue (1));
  /* Set operating channel */
  wifiPhy.Set ("ChannelNumber", UintegerValue (2));
  /* Sensitivity model includes implementation loss and noise figure */
  wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-79));
  wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-79 + 3));
  /* Set default algorithm for all nodes to be constant rate */
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "ControlMode", StringValue (phyMode),
                                                                "DataMode", StringValue (phyMode));

  /* Make four nodes and set them up with the phy and the mac */
  NodeContainer wifiNodes;
  wifiNodes.Create (2);
  Ptr<Node> apNode = wifiNodes.Get (0);
  Ptr<Node> staNode = wifiNodes.Get (1);

  /* Add a DMG upper mac */
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

  /* Install DMG PCP/AP Node */
  Ssid ssid = Ssid ("ServicePeriod");
  wifiMac.SetType ("ns3::DmgApWifiMac",
                   "Ssid", SsidValue(ssid),
                   "BE_MaxAmpduSize", UintegerValue (0),
                   "BE_MaxAmsduSize", UintegerValue (msduAggregationSize),
                   "SSSlotsPerABFT", UintegerValue (8), "SSFramesPerSlot", UintegerValue (8),
                   "BeaconInterval", TimeValue (MicroSeconds (102400)),
                   "ATIPresent", BooleanValue (false));

  /* Set Analytical Codebook for the DMG Devices */
  wifi.SetCodebook ("ns3::CodebookAnalytical",
                    "CodebookType", EnumValue (SIMPLE_CODEBOOK),
                    "Antennas", UintegerValue (1),
                    "Sectors", UintegerValue (8));

  NetDeviceContainer apDevice;
  apDevice = wifi.Install (wifiPhy, wifiMac, apNode);

  /* Install DMG STA Nodes */
  wifiMac.SetType ("ns3::DmgStaWifiMac",
                   "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false),
                   "BE_MaxAmpduSize", UintegerValue (0),
                   "BE_MaxAmsduSize", UintegerValue (msduAggregationSize));

  staDevices = wifi.Install (wifiPhy, wifiMac, staNode);

  /* Setting mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));   /* PCP/AP */
  positionAlloc->Add (Vector (1.0, 0.0, 0.0));   /* DMG STA */

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
  Ipv4InterfaceContainer staInterfaces;
  staInterfaces = address.Assign (staDevices);

  /* Populate routing table */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* We do not want any ARP packets */
  PopulateArpCache ();

  /* Install Simple UDP Server on the STA */
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9999));
  ApplicationContainer sinks = sinkHelper.Install (staNode);
  packetSink = StaticCast<PacketSink> (sinks.Get (0));

  /** STA Node Variables **/
  uint64_t staNodeLastTotalRx = 0;
  double staNodeAverageThroughput = 0;

  /* Install Simple UDP Transmiter on the DMG PCP/AP */
  Ptr<OnOffApplication> onoff;
  ApplicationContainer srcApp;
  OnOffHelper src ("ns3::UdpSocketFactory", InetSocketAddress (staInterfaces.GetAddress (0), 9999));
  src.SetAttribute ("MaxPackets", UintegerValue (maxPackets));
  src.SetAttribute ("PacketSize", UintegerValue (packetSize));
  src.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
  src.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  src.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  srcApp = src.Install (apNode);
  onoff = StaticCast<OnOffApplication> (srcApp.Get (0));

  /* Schedule Applications */
  srcApp.Start (Seconds (1.0));
  srcApp.Stop (Seconds (simulationTime));
  sinks.Start (Seconds (1.0));

  /* Schedule Throughput Calulcations */
  Simulator::Schedule (Seconds (1.1), &CalculateThroughput, packetSink, staNodeLastTotalRx, staNodeAverageThroughput);

  /* Set Maximum number of packets in WifiMacQueue */
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxPackets", UintegerValue (queueSize));

  /* Connect Wifi MAC Queue Occupancy */
  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> queueOccupanyStream;
  /* Trace DMG PCP/AP MAC Queue Changes */
  queueOccupanyStream = asciiTraceHelper.CreateFileStream ("Traces/AccessPointMacQueueOccupany.txt");
  Config::ConnectWithoutContext ("/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/OccupancyChanged",
                                 MakeBoundCallback (&QueueOccupancyChange, queueOccupanyStream));

  /* Enable Traces */
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
      wifiPhy.EnablePcap ("Traces/AccessPoint", apDevice, false);
      wifiPhy.EnablePcap ("Traces/STA", staDevices.Get (0), false);
    }

  /* Install FlowMonitor on all nodes */
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  /** Connect Traces **/
  apWifiNetDevice = StaticCast<WifiNetDevice> (apDevice.Get (0));
  staWifiNetDevice = StaticCast<WifiNetDevice> (staDevices.Get (0));
  apWifiMac = StaticCast<DmgApWifiMac> (apWifiNetDevice->GetMac ());
  staWifiMac = StaticCast<DmgStaWifiMac> (staWifiNetDevice->GetMac ());
  apWifiMac->TraceConnectWithoutContext ("StationAssociated", MakeCallback (&StationAssociated));

  Simulator::Stop (Seconds (simulationTime + 0.101));
  Simulator::Run ();
  Simulator::Destroy ();

  /* Print per flow statistics */
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")" << std::endl;;
      std::cout << "  Tx Packets: " << i->second.txPackets << std::endl;
      std::cout << "  Tx Bytes:   " << i->second.txBytes << std::endl;
      std::cout << "  TxOffered:  " << i->second.txBytes * 8.0 / ((simulationTime - 1) * 1e6)  << " Mbps" << std::endl;;
      std::cout << "  Rx Packets: " << i->second.rxPackets << std::endl;;
      std::cout << "  Rx Bytes:   " << i->second.rxBytes << std::endl;
      std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / ((simulationTime - 1) * 1e6)  << " Mbps" << std::endl;;
    }

  /* Print Application Layer Results Summary */
  std::cout << "\nApplication Layer Statistics:" << std::endl;;
  std::cout << "  Tx Packets: " << onoff->GetTotalTxPackets () << std::endl;
  std::cout << "  Tx Bytes:   " << onoff->GetTotalTxBytes () << std::endl;
  std::cout << "  Rx Packets: " << packetSink->GetTotalReceivedPackets () << std::endl;
  std::cout << "  Rx Bytes:   " << packetSink->GetTotalRx () << std::endl;
  std::cout << "  Throughput: " << packetSink->GetTotalRx () * 8.0 / ((simulationTime - 1) * 1e6) << " Mbps" << std::endl;

  return 0;
}
