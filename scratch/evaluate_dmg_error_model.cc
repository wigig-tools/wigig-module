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
#include <complex>
#include <iomanip>
#include <string>

/**
 * Simulation Objective:
 * This script is used to evaluate the performance of the IEEE 802.11ad protocol using custom SNR to BER lookup tables.
 * The tables are generated in MATLAB R2018b using WLAN Toolbox.
 *
 * Network Topology:
 * The scenario consists of a signle DMG STA and a single DMG PCP/AP.
 *
 *          DMG PCP/AP (0,0)                       DMG STA (+1,0)
 *
 * Simulation Description:
 * The DMG STA generates uplink UDP traffic towards the DMG PCP/AP. The user changes the distance between the DMG STA and
 * the DMG PCP/AP to decrease/increase the received SNR.
 *
 * Running Simulation:
 * ./waf --run "evaluate_dmg_error_model --simulationTime=10 --pcap=true"
 *
 * Simulation Output:
 * The simulation generates the following traces:
 * 1. PCAP traces for each station.
 * 2. IP Layer Statistics using Flow Monitor Module.
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateDmgErrorModel");

using namespace ns3;
using namespace std;

/**  Application Variables **/
uint64_t totalRx = 0;
double throughput = 0;
Ptr<PacketSink> packetSink;
Ptr<OnOffApplication> onoff;

/* Network Nodes */
Ptr<Node> apWifiNode;
Ptr<Node> staWifiNode;

Ptr<WifiNetDevice> staWifiNetDevice;
Ptr<DmgStaWifiMac> staWifiMac;
Ptr<DmgWifiPhy> staWifiPhy;

Ptr<WifiNetDevice> apWifiNetDevice;
Ptr<DmgWifiPhy> apWifiPhy;

Ptr<WifiRemoteStationManager> staRemoteStationManager;

/* Statistics */
uint64_t macTxDataFailed = 0;

uint64_t transmittedPackets = 0;
uint64_t droppedPackets = 0;
uint64_t receivedPackets = 0;

template <typename T>
std::string to_string_with_precision (const T a_value, const int n = 6)
{
    std::ostringstream out;
    out.precision (n);
    out << std::fixed << a_value;
    return out.str ();
}

double
CalculateSingleStreamThroughput (Ptr<PacketSink> sink, uint64_t &lastTotalRx, double &averageThroughput)
{
  double thr = (sink->GetTotalRx() - lastTotalRx) * (double) 8/1e5;     /* Convert Application RX Packets to MBits. */
  lastTotalRx = sink->GetTotalRx ();
  averageThroughput += thr;
  return thr;
}

void
CalculateThroughput (void)
{
  double thr = CalculateSingleStreamThroughput (packetSink, totalRx, throughput);
  string duration = to_string_with_precision<double> (Simulator::Now ().GetSeconds () - 0.1, 1)
                  + " - " + to_string_with_precision<double> (Simulator::Now ().GetSeconds (), 1);
  std::cout << std::left << std::setw (12) << duration;
  std::cout << std::left << std::setw (12) << thr << std::endl;
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

void
MacTxDataFailed (Mac48Address)
{
  macTxDataFailed++;
}

void
PhyTxEnd (Ptr<const Packet>)
{
  transmittedPackets++;
}

void
PhyRxDrop (Ptr<const Packet>)
{
  droppedPackets++;
}

void
PhyRxEnd (Ptr<const Packet>)
{
  receivedPackets++;
}

void
StationAssoicated (Ptr<DmgWifiMac> staWifiMac, Mac48Address address, uint16_t aid)
{
  std::cout << "DMG STA " << staWifiMac->GetAddress () << " associated with DMG AP " << address
            << ", Association ID (AID) = " << aid << std::endl;
}

int
main(int argc, char *argv[])
{
  uint32_t payloadSize = 1472;                  /* Application payload size in bytes. */
  string dataRate = "150Mbps";                  /* Application data rate. */
  uint32_t msduAggregationSize = 0;             /* The maximum aggregation size for A-MSDU in Bytes. */
  uint32_t mpduAggregationSize = 0;             /* The maximum aggregation size for A-MSPU in Bytes. */
  uint32_t queueSize = 1000;                    /* Wifi MAC Queue Size. */
  string phyMode = "DMG_MCS12";                 /* Type of the Physical Layer. */
  uint32_t snapShotLength = std::numeric_limits<uint32_t>::max (); /* The maximum PCAP Snapshot Length */
  double distance = 1;                          /* The distance between devices. */
  bool verbose = false;                         /* Print Logging Information. */
  double simulationTime = 10;                   /* Simulation time in seconds. */
  bool pcapTracing = false;                     /* PCAP Tracing is enabled. */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("payloadSize", "Application payload size in bytes", payloadSize);
  cmd.AddValue ("dataRate", "Application data rate", dataRate);
  cmd.AddValue ("msduAggregation", "The maximum aggregation size for A-MSDU in Bytes", msduAggregationSize);
  cmd.AddValue ("mpduAggregation", "The maximum aggregation size for A-MPDU in Bytes", mpduAggregationSize);
  cmd.AddValue ("queueSize", "The maximum size of the Wifi MAC Queue", queueSize);
  cmd.AddValue ("phyMode", "802.11ad PHY Mode", phyMode);
  cmd.AddValue ("dist", "The distance between devices", distance);
  cmd.AddValue ("verbose", "Turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("snapShotLength", "The maximum PCAP Snapshot Length", snapShotLength);
  cmd.AddValue ("pcap", "Enable PCAP Tracing", pcapTracing);
  cmd.Parse (argc, argv);

  /* Global params: no fragmentation, no RTS/CTS, fixed rate for all packets */
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::QueueBase::MaxPackets", UintegerValue (queueSize));

  /**** DmgWifiHelper is a meta-helper: it helps creates helpers ****/
  DmgWifiHelper wifi;

  /* Turn on logging */
  if (verbose)
    {
      wifi.EnableLogComponents ();
      LogComponentEnable ("EvaluateDmgErrorModel", LOG_LEVEL_ALL);
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
  /* Set error model */
  wifiPhy.SetErrorRateModel ("ns3::DmgErrorModel",
                             "FileName", StringValue ("DmgFiles/ErrorModel/LookupTable_1458.txt"));
  /* Sensitivity model includes implementation loss and noise figure */
  wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-79));
  wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-79 + 3));
  /* Set default algorithm for all nodes to be constant rate */
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue (phyMode));

  /* Make two nodes and set them up with the PHY and the MAC */
  NodeContainer wifiNodes;
  wifiNodes.Create (2);
  apWifiNode = wifiNodes.Get (0);
  staWifiNode = wifiNodes.Get (1);

  /* Add a DMG upper mac */
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

  Ssid ssid = Ssid ("ErrorModel");
  wifiMac.SetType ("ns3::DmgApWifiMac",
                   "Ssid", SsidValue(ssid),
                   "BE_MaxAmpduSize", UintegerValue (mpduAggregationSize),
                   "BE_MaxAmsduSize", UintegerValue (msduAggregationSize),
                   "SSSlotsPerABFT", UintegerValue (8), "SSFramesPerSlot", UintegerValue (8),
                   "BeaconInterval", TimeValue (MicroSeconds (102400)),
                   "ATIPresent", BooleanValue (false));

  /* Set Analytical Codebook for the DMG Devices */
  wifi.SetCodebook ("ns3::CodebookAnalytical",
                    "CodebookType", EnumValue (SIMPLE_CODEBOOK),
                    "Antennas", UintegerValue (1),
                    "Sectors", UintegerValue (8));

  /* Create Wifi Network Devices (WifiNetDevice) */
  NetDeviceContainer apDevice;
  apDevice = wifi.Install (wifiPhy, wifiMac, apWifiNode);

  wifiMac.SetType ("ns3::DmgStaWifiMac",
                   "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false),
                   "BE_MaxAmpduSize", UintegerValue (mpduAggregationSize),
                   "BE_MaxAmsduSize", UintegerValue (msduAggregationSize));

  NetDeviceContainer staDevice;
  staDevice = wifi.Install (wifiPhy, wifiMac, staWifiNode);

  /* Setting mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));        /* DMG PCP/AP */
  positionAlloc->Add (Vector (distance, 0.0, 0.0));   /* DMG STA */

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

  /* Install Simple UDP Server on the DMG AP */
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9999));
  ApplicationContainer sinkApp = sinkHelper.Install (apWifiNode);
  packetSink = StaticCast<PacketSink> (sinkApp.Get (0));
  sinkApp.Start (Seconds (0.0));

  /* Install UDP Transmitter on the DMG STA */
  ApplicationContainer srcApp;
  OnOffHelper src ("ns3::UdpSocketFactory", InetSocketAddress (apInterface.GetAddress (0), 9999));
  src.SetAttribute ("MaxBytes", UintegerValue (0));
  src.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  src.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
  src.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  src.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  srcApp = src.Install (staWifiNode);
  srcApp.Start (Seconds (1.0));
  srcApp.Stop (Seconds (simulationTime));
  onoff = StaticCast<OnOffApplication> (srcApp.Get (0));

  /* Print Traces */
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
      wifiPhy.SetSnapshotLength (snapShotLength);
      wifiPhy.EnablePcap ("Traces/AccessPoint", apDevice, false);
      wifiPhy.EnablePcap ("Traces/Station", staDevice, false);
    }

  /* Stations */
  apWifiNetDevice= StaticCast<WifiNetDevice> (apDevice.Get (0));
  apWifiPhy = StaticCast<DmgWifiPhy> (apWifiNetDevice->GetPhy ());
  staWifiNetDevice = StaticCast<WifiNetDevice> (staDevice.Get (0));
  staWifiMac = StaticCast<DmgStaWifiMac> (staWifiNetDevice->GetMac ());
  staWifiPhy = StaticCast<DmgWifiPhy> (staWifiNetDevice->GetPhy ());
  staRemoteStationManager = staWifiNetDevice->GetRemoteStationManager ();

  /* Connect Traces */
  apWifiPhy->TraceConnectWithoutContext ("PhyRxEnd", MakeCallback (&PhyRxEnd));
  apWifiPhy->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&PhyRxDrop));
  staWifiMac->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, staWifiMac));
  staWifiPhy->TraceConnectWithoutContext ("PhyTxEnd", MakeCallback (&PhyTxEnd));
  staRemoteStationManager->TraceConnectWithoutContext ("MacTxDataFailed", MakeCallback (&MacTxDataFailed));

  /* Change the maximum number of retransmission attempts for a DATA packet */
  staRemoteStationManager->SetAttribute ("MaxSlrc", UintegerValue (0));

  /* Install FlowMonitor on all nodes */
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  /* Print Output*/
  std::cout << std::left << std::setw (12) << "Time [s]"
            << std::left << std::setw (12) << "Throughput [Mbps]" << std::endl;

  /* Schedule Throughput Calulcations */
  Simulator::Schedule (Seconds (1.1), &CalculateThroughput);

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

  /* Print MAC Layer Statistics */
  std::cout << "\nMAC Layer Statistics:" << std::endl;;
  std::cout << "  Number of Failed Tx Data Packets:  " << macTxDataFailed << std::endl;

  /* Print PHY Layer Statistics */
  std::cout << "\nPHY Layer Statistics:" << std::endl;;
  std::cout << "  Number of Tx Packets:         " << transmittedPackets << std::endl;
  std::cout << "  Number of Rx Packets:         " << receivedPackets << std::endl;
  std::cout << "  Number of Rx Dropped Packets: " << droppedPackets << std::endl;

  return 0;
}
