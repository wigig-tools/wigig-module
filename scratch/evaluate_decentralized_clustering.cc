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
#include <iomanip>
#include <sstream>

/**
 * Simulation Objective:
 * This script is used to evaluate IEEE 802.11ad decentranlized clustering mechanism (formation and maintenance).
 *
 * Network Topology:
 * The scenario consists of four DMG PCP/APs in which one of them will act as S-PCP/S-AP (DMG AP_1).
 * One DMG STA connects to one DMA AP following clockwise direction i.e. DMG STA (1) connects to DMG AP (1), etc.
 *
 *
 * DMG STA_1 (-1.73, +1)       DMG AP_1 (0, +1) (S-AP)     DMG STA_2 (+1.73, +1)
 *
 *
 *
 * DMG AP_4  (-1.73, 0)                                    DMG AP_2  (+1.73, 0)
 *
 *
 *
 * DMG STA_4 (-1.73, -1)          DMG AP_3 (0, -1)         DMG STA_3 (+1.73, -1)
 *
 *
 * Simulation Description:
 *
 * Running Simulation:
 * To use this script simply type the following run command:
 * ./waf --run "evaluate_decentralized_clustering"
 *
 * Simulation Output:
 * The simulation generates four PCAP files for each DMG AP and DMG STA in the scenario.
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateDecentralizedClustering");

using namespace ns3;
using namespace std;

/* Type definitions */
struct CommunicationPair {
  Ptr<Application> srcApp;
  Ptr<PacketSink> packetSink;
  uint64_t totalRx = 0;
  double throughput = 0;
  Time startTime;
};
typedef std::vector<CommunicationPair> CommunicationPairList;
typedef CommunicationPairList::iterator CommunicationPairList_I;
typedef CommunicationPairList::const_iterator CommunicationPairList_CI;

/** Simulation Arguments **/
string applicationType = "onoff";             /* Type of the Tx application */
string socketType = "ns3::UdpSocketFactory";  /* Socket Type (TCP/UDP) */
uint32_t packetSize = 1448;                   /* Application payload size in bytes. */
string dataRate = "300Mbps";                  /* Application data rate. */
string tcpVariant = "NewReno";                /* TCP Variant Type. */
uint32_t maxPackets = 0;                      /* Maximum Number of Packets */
uint32_t msduAggregationSize = 7935;          /* The maximum aggregation size for A-MSDU in Bytes. */
uint32_t mpduAggregationSize = 262143;        /* The maximum aggregation size for A-MPDU in Bytes. */
double simulationTime = 10;                   /* Simulation time in seconds. */

/**  Applications **/
CommunicationPairList communicationPairList;    /* List of communicating devices. */

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
  double totalThr = 0;
  double thr;
  string duration = to_string_with_precision<double> (Simulator::Now ().GetSeconds () - 0.1, 1)
                  + " - " + to_string_with_precision<double> (Simulator::Now ().GetSeconds (), 1);
  std::cout << std::left << std::setw (12) << duration;
  for (CommunicationPairList_I it = communicationPairList.begin (); it != communicationPairList.end (); it++)
    {
      thr = CalculateSingleStreamThroughput (it->packetSink, it->totalRx, it->throughput);
      totalThr += thr;
      std::cout << std::left << std::setw (12) << thr;
    }
  std::cout << std::left << std::setw (12) << totalThr << std::endl;
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

void JoinedCluster (Ptr<DmgWifiMac> apWifiMac, Mac48Address address, uint8_t beaconSPIndex)
{
  std::cout << "DMG PCP/AP " << apWifiMac->GetAddress () << " joined ClusterID=" << address
            << " in BeaconSP=" << static_cast<uint16_t> (beaconSPIndex)
            << " at " << Simulator::Now () << std::endl;
}

Ptr<WifiNetDevice>
CreateAccessPoint (Ptr<Node> apNode, Ssid ssid,
                   DmgWifiHelper wifi, DmgWifiPhyHelper wifiPhy, Time channelMonitorDuration)
{
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

  wifiMac.SetType ("ns3::DmgApWifiMac",
                   "Ssid", SsidValue (ssid),
                   "BE_MaxAmpduSize", UintegerValue (mpduAggregationSize),
                   "BE_MaxAmsduSize", UintegerValue (msduAggregationSize),
                   "SSSlotsPerABFT", UintegerValue (8), "SSFramesPerSlot", UintegerValue (16),
                   "AllowBeaconing", BooleanValue (false),
                   "ATIPresent", BooleanValue (false),
                   "EnableDecentralizedClustering", BooleanValue (true),
                   "ClusterRole", EnumValue (NOT_PARTICIPATING),
                   "ChannelMonitorDuration", TimeValue (channelMonitorDuration));

  NetDeviceContainer apDevice = wifi.Install (wifiPhy, wifiMac, apNode);

  return (StaticCast<WifiNetDevice> (apDevice.Get (0)));
}

void
SLSCompleted (Ptr<DmgWifiMac> wifiMac, Mac48Address address, ChannelAccessPeriod accessPeriod,
              BeamformingDirection beamformingDirection, bool isInitiatorTxss, bool isResponderTxss,
              SECTOR_ID sectorId, ANTENNA_ID antennaId)
{
  std::cout << "DMG STA " << wifiMac->GetAddress () << " completed SLS phase with DMG STA " << address << std::endl;
  std::cout << "Best Tx Antenna Configuration: SectorID=" << uint16_t (sectorId) << ", AntennaID=" << uint16_t (antennaId) << std::endl;
}

void
StationAssoicated (Ptr<DmgWifiMac> staWifiMac, Mac48Address address, uint16_t aid)
{
  std::cout << "DMG STA " << staWifiMac->GetAddress () << " associated with DMG AP " << address
            << ", Association ID (AID) = " << aid << std::endl;
}

CommunicationPair
InstallApplications (Ptr<Node> srcNode, Ptr<Node> dstNode, Ipv4Address address, Time startTime)
{
  CommunicationPair commPair;

  /* Install TCP/UDP Transmitter on the source node */
  Address dest (InetSocketAddress (address, 9999));
  ApplicationContainer srcApp;
  if (applicationType == "onoff")
    {
      OnOffHelper src (socketType, dest);
      src.SetAttribute ("MaxBytes", UintegerValue (maxPackets));
      src.SetAttribute ("PacketSize", UintegerValue (packetSize));
      src.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
      src.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      src.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
      srcApp = src.Install (srcNode);
    }
  else if (applicationType == "bulk")
    {
      BulkSendHelper src (socketType, dest);
      srcApp = src.Install (srcNode);
    }
  srcApp.Start (startTime);
  srcApp.Stop (Seconds (simulationTime));
  commPair.srcApp = srcApp.Get (0);

  /* Install Simple TCP/UDP Server on the destination node */
  PacketSinkHelper sinkHelper (socketType, InetSocketAddress (Ipv4Address::GetAny (), 9999));
  ApplicationContainer sinkApp = sinkHelper.Install (dstNode);
  commPair.packetSink = StaticCast<PacketSink> (sinkApp.Get (0));
  sinkApp.Start (Seconds (0.0));

  commPair.startTime = startTime;

  return commPair;
}

NetDeviceContainer
InstallMAC_Layer (Ptr<Node> node, DmgWifiHelper &wifi, DmgWifiPhyHelper &wifiPhy, std::string apName)
{
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();
  wifiMac.SetType ("ns3::DmgStaWifiMac",
                   "Ssid", SsidValue (Ssid (apName)), "ActiveProbing", BooleanValue (false),
                   "BE_MaxAmpduSize", UintegerValue (mpduAggregationSize),
                   "BE_MaxAmsduSize", UintegerValue (msduAggregationSize));
  return wifi.Install (wifiPhy, wifiMac, node);
}

int
main (int argc, char *argv[])
{
  uint32_t bufferSize = 131072;                   /* TCP Send/Receive Buffer Size. */
  uint32_t queueSize = 1000;                      /* Wifi MAC Queue Size. */
  string phyMode = "DMG_MCS12";                   /* Type of the Physical Layer. */
  uint32_t snapShotLength = std::numeric_limits<uint32_t>::max (); /* The maximum PCAP Snapshot Length */
  bool verbose = false;                           /* Print Logging Information. */
  bool pcapTracing = false;                       /* PCAP Tracing is enabled or not. */
  std::map<std::string, std::string> tcpVariants; /* List of the TCP Variants */

  /** TCP Variants **/
  tcpVariants.insert (std::make_pair ("NewReno",       "ns3::TcpNewReno"));
  tcpVariants.insert (std::make_pair ("Hybla",         "ns3::TcpHybla"));
  tcpVariants.insert (std::make_pair ("HighSpeed",     "ns3::TcpHighSpeed"));
  tcpVariants.insert (std::make_pair ("Vegas",         "ns3::TcpVegas"));
  tcpVariants.insert (std::make_pair ("Scalable",      "ns3::TcpScalable"));
  tcpVariants.insert (std::make_pair ("Veno",          "ns3::TcpVeno"));
  tcpVariants.insert (std::make_pair ("Bic",           "ns3::TcpBic"));
  tcpVariants.insert (std::make_pair ("Westwood",      "ns3::TcpWestwood"));
  tcpVariants.insert (std::make_pair ("WestwoodPlus",  "ns3::TcpWestwoodPlus"));

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("applicationType", "Type of the Tx Application: onoff or bulk", applicationType);
  cmd.AddValue ("packetSize", "Application packet size in bytes", packetSize);
  cmd.AddValue ("dataRate", "Application data rate", dataRate);
  cmd.AddValue ("maxPackets", "Maximum number of packets to send", maxPackets);
  cmd.AddValue ("tcpVariant", "Transport protocol to use: TcpTahoe, TcpReno, TcpNewReno, TcpWestwood, TcpWestwoodPlus", tcpVariant);
  cmd.AddValue ("socketType", "Type of the Socket (ns3::TcpSocketFactory, ns3::UdpSocketFactory)", socketType);
  cmd.AddValue ("bufferSize", "TCP Buffer Size (Send/Receive) in Bytes", bufferSize);
  cmd.AddValue ("msduAggregation", "The maximum aggregation size for A-MSDU in Bytes", msduAggregationSize);
  cmd.AddValue ("mpduAggregation", "The maximum aggregation size for A-MPDU in Bytes", mpduAggregationSize);
  cmd.AddValue ("queueSize", "The maximum size of the Wifi MAC Queue", queueSize);
  cmd.AddValue ("phyMode", "802.11ad PHY Mode", phyMode);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("snapShotLength", "The maximum PCAP Snapshot Length", snapShotLength);
  cmd.AddValue ("pcap", "Enable PCAP Tracing", pcapTracing);
  cmd.Parse (argc, argv);

  /* Global params: no fragmentation, no RTS/CTS, fixed rate for all packets */
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::QueueBase::MaxPackets", UintegerValue (queueSize));

  /*** Configure TCP Options ***/
  /* Select TCP variant */
  std::map<std::string, std::string>::const_iterator iter = tcpVariants.find (tcpVariant);
  NS_ASSERT_MSG (iter != tcpVariants.end (), "Cannot find Tcp Variant");
  TypeId tid = TypeId::LookupByName (iter->second);
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (tid));
  if (tcpVariant.compare ("Westwood") == 0)
    {
      Config::SetDefault ("ns3::TcpWestwood::ProtocolType", EnumValue (TcpWestwood::WESTWOOD));
      Config::SetDefault ("ns3::TcpWestwood::FilterType", EnumValue (TcpWestwood::TUSTIN));
    }
  else if (tcpVariant.compare ("WestwoodPlus") == 0)
    {
      Config::SetDefault ("ns3::TcpWestwood::ProtocolType", EnumValue (TcpWestwood::WESTWOODPLUS));
      Config::SetDefault ("ns3::TcpWestwood::FilterType", EnumValue (TcpWestwood::TUSTIN));
    }

  /* Configure TCP Segment Size */
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (packetSize));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (bufferSize));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (bufferSize));

  /**** WifiHelper is a meta-helper: it helps creates helpers ****/
  DmgWifiHelper wifi;

  /* Basic setup */
  wifi.SetStandard (WIFI_PHY_STANDARD_80211ad);

  /* Turn on logging */
  if (verbose)
    {
      wifi.EnableLogComponents ();
      LogComponentEnable ("EvaluateDecentralizedClustering", LOG_LEVEL_ALL);
    }

  /**** Set up Channel ****/
  DmgWifiChannelHelper wifiChannel ;
  /* Simple propagation delay model */
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  /* Friis model with standard-specific wavelength */
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (60.48e9));

  /**** SETUP ALL NODES ****/
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
  NodeContainer syncApWwifiNode;
  syncApWwifiNode.Create (1);

  NodeContainer apWifiNodes;
  apWifiNodes.Create (3);

  NodeContainer staWifiNodes;
  staWifiNodes.Create (4);

  /* Add a DMG upper mac */
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

  /* Install SYNC AP Node */
  Ssid ssid = Ssid ("AP1");
  wifiMac.SetType ("ns3::DmgApWifiMac",
                   "Ssid", SsidValue (ssid),
                   "BE_MaxAmpduSize", UintegerValue (mpduAggregationSize),
                   "BE_MaxAmsduSize", UintegerValue (msduAggregationSize),
                   "SSSlotsPerABFT", UintegerValue (8), "SSFramesPerSlot", UintegerValue (16),
                   "BeaconInterval", TimeValue (MicroSeconds (102400)),
                   "EnableDecentralizedClustering", BooleanValue (true),
                   "ClusterMaxMem", UintegerValue (4),
                   "BeaconSPDuration", UintegerValue (100),
                   "ClusterRole", EnumValue (SYNC_PCP_AP));

  /* Set Analytical Codebook for the DMG Devices */
  wifi.SetCodebook ("ns3::CodebookAnalytical",
                    "CodebookType", EnumValue (SIMPLE_CODEBOOK),
                    "Antennas", UintegerValue (1),
                    "Sectors", UintegerValue (8));

  NetDeviceContainer syncApDevice;
  syncApDevice = wifi.Install (wifiPhy, wifiMac, syncApWwifiNode);

  /* Install DMG PCP/AP Nodes */
  NetDeviceContainer apDevices;
  apDevices.Add (CreateAccessPoint (apWifiNodes.Get (0), Ssid ("AP2"), wifi, wifiPhy, 1 * aMinChannelTime));
  apDevices.Add (CreateAccessPoint (apWifiNodes.Get (1), Ssid ("AP3"), wifi, wifiPhy, 2 * aMinChannelTime));
  apDevices.Add (CreateAccessPoint (apWifiNodes.Get (2), Ssid ("AP4"), wifi, wifiPhy, 3 * aMinChannelTime));

  /** Install DMG STA Nodes **/
  NetDeviceContainer staDevices;
  staDevices.Add (InstallMAC_Layer (staWifiNodes.Get (0), wifi, wifiPhy, "AP1"));
  staDevices.Add (InstallMAC_Layer (staWifiNodes.Get (1), wifi, wifiPhy, "AP2"));
  staDevices.Add (InstallMAC_Layer (staWifiNodes.Get (2), wifi, wifiPhy, "AP3"));
  staDevices.Add (InstallMAC_Layer (staWifiNodes.Get (3), wifi, wifiPhy, "AP4"));

  /* Setting mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, +1.0, 0.0));    /* DMG PCP/AP (1) (S-PCP/S-AP) */
  positionAlloc->Add (Vector (+1.73, 0.0, 0.0));   /* DMG PCP/AP (2) */
  positionAlloc->Add (Vector (0.0, -1.0, 0.0));    /* DMG PCP/AP (3) */
  positionAlloc->Add (Vector (-1.73, 0.0, 0.0));   /* DMG PCP/AP (4) */
  positionAlloc->Add (Vector (-1.73, +1.0, 0.0));  /* DMG STA 1 */
  positionAlloc->Add (Vector (+1.73, +1.0, 0.0));  /* DMG STA 2 */
  positionAlloc->Add (Vector (+1.73, -1.0, 0.0));  /* DMG STA 3 */
  positionAlloc->Add (Vector (-1.73, -1.0, 0.0));  /* DMG STA 4 */

  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (syncApWwifiNode);
  mobility.Install (apWifiNodes);
  mobility.Install (staWifiNodes);

  /* Internet stack*/
  InternetStackHelper stack;
  stack.Install (syncApWwifiNode);
  stack.Install (apWifiNodes);
  stack.Install (staWifiNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer syncApInterface;
  syncApInterface = address.Assign (syncApDevice);
  Ipv4InterfaceContainer apInterfaces;
  apInterfaces = address.Assign (apDevices);
  address.Assign (staDevices);

  /* Populate routing table */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* We do not want any ARP packets */
  PopulateArpCache ();

  /** Install Applications **/
  /* DMG STA_1 -->  DMG AP_1 */
  communicationPairList.push_back (InstallApplications (staWifiNodes.Get (0), syncApWwifiNode.Get (0),
                                                        syncApInterface.GetAddress (0), Seconds (0.1)));
  /* DMG STA_2 -->  DMG AP_2 */
  communicationPairList.push_back (InstallApplications (staWifiNodes.Get (1), apWifiNodes.Get (0),
                                                        apInterfaces.GetAddress (0), Seconds (1.2)));
  /* DMG STA_3 -->  DMG AP_3 */
  communicationPairList.push_back (InstallApplications (staWifiNodes.Get (2), apWifiNodes.Get (1),
                                                        apInterfaces.GetAddress (1), Seconds (2.3)));
  /* DMG STA_4 -->  DMG AP_4 */
  communicationPairList.push_back (InstallApplications (staWifiNodes.Get (3), apWifiNodes.Get (2),
                                                        apInterfaces.GetAddress (2), Seconds (3.4)));

  /* Connect DMG PCP/AP Traces */
  Ptr<WifiNetDevice> wifiNetDevice;
  Ptr<DmgWifiMac> dmgWifiMac;
  for (uint32_t i = 0; i < apDevices.GetN (); i++)
    {
      wifiNetDevice = StaticCast<WifiNetDevice> (apDevices.Get (i));
      dmgWifiMac = StaticCast<DmgWifiMac> (wifiNetDevice->GetMac ());
      dmgWifiMac->TraceConnectWithoutContext ("JoinedCluster", MakeBoundCallback (&JoinedCluster, dmgWifiMac));
      dmgWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, dmgWifiMac));
    }
  wifiNetDevice = StaticCast<WifiNetDevice> (syncApDevice.Get (0));
  dmgWifiMac = StaticCast<DmgWifiMac> (wifiNetDevice->GetMac ());
  dmgWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, dmgWifiMac));

  /* Connect DMG STA Traces */
  for (uint32_t i = 0; i < staDevices.GetN (); i++)
    {
      wifiNetDevice = StaticCast<WifiNetDevice> (staDevices.Get (i));
      dmgWifiMac = StaticCast<DmgWifiMac> (wifiNetDevice->GetMac ());
      dmgWifiMac->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, dmgWifiMac));
      dmgWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, dmgWifiMac));
    }

  /* Enable Traces */
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
      wifiPhy.SetSnapshotLength (snapShotLength);
      wifiPhy.EnablePcap ("Traces/AccessPoint", NetDeviceContainer (syncApDevice, apDevices), false);
      wifiPhy.EnablePcap ("Traces/STA", staDevices, false);
    }

  /* Install FlowMonitor on all nodes */
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  /* Print Output */
  std::cout << "Application Layer Throughput per Communicating Pair [Mbps]" << std::endl;
  std::cout << std::left << std::setw (12) << "Time [s]";
  string columnName;
  for (uint8_t i = 0; i < communicationPairList.size (); i++)
    {
      columnName = "Pair (" + std::to_string (i + 1) + ")";
      std::cout << std::left << std::setw (12) << columnName;
    }
   std::cout << std::left << std::setw (12) << "Total" << std::endl;

  /* Schedule Throughput Calulcations */
  Simulator::Schedule (Seconds (0.1), &CalculateThroughput);

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
      std::cout << "  TxOffered:  " << i->second.txBytes * 8.0 / ((simulationTime - 0.1) * 1e6)  << " Mbps" << std::endl;;
      std::cout << "  Rx Packets: " << i->second.rxPackets << std::endl;;
      std::cout << "  Rx Bytes:   " << i->second.rxBytes << std::endl;
      std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / ((simulationTime - 0.1) * 1e6)  << " Mbps" << std::endl;;
    }

  /* Print Application Layer Results Summary */
  std::cout << "\nApplication Layer Statistics:" << std::endl;
  Ptr<OnOffApplication> onoff;
  Ptr<BulkSendApplication> bulk;
  uint16_t communicationLinks = 1;
  for (CommunicationPairList_I it = communicationPairList.begin (); it != communicationPairList.end (); it++)
    {
      std::cout << "Communication Link (" << communicationLinks << ") Statistics:" << std::endl;
      if (applicationType == "onoff")
        {
          onoff = StaticCast<OnOffApplication> (it->srcApp);
          std::cout << "  Tx Packets: " << onoff->GetTotalTxPackets () << std::endl;
          std::cout << "  Tx Bytes:   " << onoff->GetTotalTxBytes () << std::endl;
        }
      else
        {
          bulk = StaticCast<BulkSendApplication> (it->srcApp);
          std::cout << "  Tx Packets: " << bulk->GetTotalTxPackets () << std::endl;
          std::cout << "  Tx Bytes:   " << bulk->GetTotalTxBytes () << std::endl;
        }
      Ptr<PacketSink> packetSink = StaticCast<PacketSink> (it->packetSink);
      std::cout << "  Rx Packets: " << packetSink->GetTotalReceivedPackets () << std::endl;
      std::cout << "  Rx Bytes:   " << packetSink->GetTotalRx () << std::endl;
      std::cout << "  Throughput: " << packetSink->GetTotalRx () * 8.0 / ((simulationTime - it->startTime.GetSeconds ()) * 1e6)
                << " Mbps" << std::endl;
      communicationLinks++;
    }

  return 0;
}
