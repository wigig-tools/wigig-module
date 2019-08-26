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

/**
 *
 * Simulation Objective:
 * This script is used to evaluate allocation of Static Service Periods in IEEE 802.11ad.
 *
 * Network Topology:
 * The scenario consists of 3 DMG STAs (West + South + East) and one DMG PCP/AP as following:
 *
 *                         DMG AP (0,1)
 *
 *
 * West DMG STA (-1,0)                      East DMG STA (1,0)
 *
 *
 *                      South DMG STA (0,-1)
 *
 * Simulation Description:
 * Once all the stations have assoicated successfully with the PCP/AP. The PCP/AP allocates three SPs
 * to perform TxSS between all the stations. Once West DMG STA has completed TxSS phase with East and
 * South DMG STAs. The PCP/AP allocates three static service periods for data communication as following:
 *
 * SP1: West DMG STA -----> East DMG STA (SP Length = 3.2ms)
 * SP2: West DMG STA -----> South DMG STA (SP Length = 3.2ms)
 * SP3: DMG AP -----> West DMG STA (SP Length = 5ms)
 *
 * Running the Simulation:
 * To run the script with the default parameters:
 * ./waf --run "evaluate_service_period"
 *
 * To run the script with different duration for the allocations e.g. SP1=10ms and SP2=5ms:
 * ./waf --run "evaluate_service_period --sp1Duration=10000 --sp2Duration=5000"
 *
 * Simulation Output:
 * The simulation generates the following traces:
 * 1. PCAP traces for each station. From the PCAP files, we can see that data transmission takes place
 * during its SP. In addition, we can notice in the announcement of the two Static Allocation Periods
 * inside each DMG Beacon.
 *
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateServicePeriod");

using namespace ns3;
using namespace std;

/** West -> East Allocation Variables **/
uint64_t westEastLastTotalRx = 0;
double westEastAverageThroughput = 0;
/** West -> South Node Allocation Variables **/
uint64_t westSouthTotalRx = 0;
double westSouthAverageThroughput = 0;
/** AP -> West Node Allocation Variables **/
uint64_t apWestTotalRx = 0;
double apWestAverageThroughput = 0;

Ptr<PacketSink> sink1, sink2, sink3;

/* Network Nodes */
Ptr<WifiNetDevice> apWifiNetDevice;
Ptr<WifiNetDevice> southWifiNetDevice;
Ptr<WifiNetDevice> westWifiNetDevice;
Ptr<WifiNetDevice> eastWifiNetDevice;

NetDeviceContainer staDevices;

Ptr<DmgApWifiMac> apWifiMac;
Ptr<DmgStaWifiMac> southWifiMac;
Ptr<DmgStaWifiMac> westWifiMac;
Ptr<DmgStaWifiMac> eastWifiMac;

/*** Access Point Variables ***/
uint8_t assoicatedStations = 0;           /* Total number of assoicated stations with the AP */
uint8_t stationsTrained = 0;              /* Number of BF trained stations */
bool scheduledStaticPeriods = false;      /* Flag to indicate whether we scheduled Static Service Periods or not */

/*** Service Period Parameters ***/
uint16_t sp1Duration = 3200;              /* The duration of the allocated service period (1) in MicroSeconds */
uint16_t sp2Duration = 3200;              /* The duration of the allocated service period (2) in MicroSeconds */
uint16_t sp3Duration = 5000;              /* The duration of the allocated service period (3) in MicroSeconds */

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
  double thr1, thr2, thr3;
  Time now = Simulator::Now ();
  thr1 = CalculateSingleStreamThroughput (sink1, westEastLastTotalRx, westEastAverageThroughput);
  thr2 = CalculateSingleStreamThroughput (sink2, westSouthTotalRx, westSouthAverageThroughput);
  thr3 = CalculateSingleStreamThroughput (sink3, apWestTotalRx, apWestAverageThroughput);
  std::cout << std::left << std::setw (12) << now.GetSeconds ()
            << std::left << std::setw (12) << thr1
            << std::left << std::setw (12) << thr2
            << std::left << std::setw (12) << thr3 << std::endl;
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

void
StationAssoicated (Ptr<DmgStaWifiMac> staWifiMac, Mac48Address address, uint16_t aid)
{
  std::cout << "DMG STA: " << staWifiMac->GetAddress () << " associated with DMG PCP/AP: " << address << std::endl;
  std::cout << "Association ID (AID) = " << staWifiMac->GetAssociationID () << std::endl;
  assoicatedStations++;
  /* Check if all stations have assoicated with the PCP/AP */
  if (assoicatedStations == 3)
    {
      /* Map AID to MAC Addresses in each node instead of requesting information */
      Ptr<DmgStaWifiMac> srcMac, dstMac;
      for (NetDeviceContainer::Iterator i = staDevices.Begin (); i != staDevices.End (); ++i)
        {
          srcMac = StaticCast<DmgStaWifiMac> (StaticCast<WifiNetDevice> (*i)->GetMac ());
          for (NetDeviceContainer::Iterator j = staDevices.Begin (); j != staDevices.End (); ++j)
            {
              dstMac = StaticCast<DmgStaWifiMac> (StaticCast<WifiNetDevice> (*j)->GetMac ());
              if (srcMac != dstMac)
                {
                  srcMac->MapAidToMacAddress (dstMac->GetAssociationID (), dstMac->GetAddress ());
                }
            }
        }

      std::cout << "All stations got associated with DMG PCP/AP: " << address << std::endl;

      /* For simplicity we assume that each station is aware of the capabilities of the peer station */
      /* Otherwise, we have to request the capabilities of the peer station. */
      westWifiMac->StorePeerDmgCapabilities (eastWifiMac);
      westWifiMac->StorePeerDmgCapabilities (southWifiMac);
      eastWifiMac->StorePeerDmgCapabilities (westWifiMac);
      eastWifiMac->StorePeerDmgCapabilities (southWifiMac);
      southWifiMac->StorePeerDmgCapabilities (westWifiMac);
      southWifiMac->StorePeerDmgCapabilities (eastWifiMac);

      /* Schedule Beamforming Training SPs */
      uint32_t allocationStart = 0;
      allocationStart = apWifiMac->AllocateBeamformingServicePeriod (westWifiMac->GetAssociationID (),
                                                                     eastWifiMac->GetAssociationID (), allocationStart, true);
      allocationStart = apWifiMac->AllocateBeamformingServicePeriod (westWifiMac->GetAssociationID (),
                                                                     southWifiMac->GetAssociationID (), allocationStart, true);
      allocationStart = apWifiMac->AllocateBeamformingServicePeriod (southWifiMac->GetAssociationID (),
                                                                     eastWifiMac->GetAssociationID (), allocationStart, true);
    }
}

void
SLSCompleted (Ptr<DmgWifiMac> staWifiMac, Mac48Address address, ChannelAccessPeriod accessPeriod,
              BeamformingDirection beamformingDirection, bool isInitiatorTxss, bool isResponderTxss,
              SECTOR_ID sectorId, ANTENNA_ID antennaId)
{
  if (accessPeriod == CHANNEL_ACCESS_DTI)
    {
      std::cout << "DMG STA " << staWifiMac->GetAddress () << " completed SLS phase with DMG STA " << address << std::endl;
      std::cout << "The best antenna configuration is SectorID=" << uint32_t (sectorId)
                << ", AntennaID=" << uint32_t (antennaId) << std::endl;
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
          uint32_t startAllocation = 0;
          Time guardTime = MicroSeconds (10);
          /* SP1 */
          startAllocation = apWifiMac->AllocateSingleContiguousBlock (1, SERVICE_PERIOD_ALLOCATION, true,
                                                                      westWifiMac->GetAssociationID (),
                                                                      eastWifiMac->GetAssociationID (),
                                                                      startAllocation, sp1Duration);
          /* SP2 */
          startAllocation = apWifiMac->AllocateSingleContiguousBlock (2, SERVICE_PERIOD_ALLOCATION, true,
                                                                      westWifiMac->GetAssociationID (),
                                                                      southWifiMac->GetAssociationID (),
                                                                      startAllocation + guardTime.GetMicroSeconds (), sp2Duration);
          /* SP3 */
          apWifiMac->AllocateSingleContiguousBlock (3, SERVICE_PERIOD_ALLOCATION, true,
                                                    AID_AP,
                                                    westWifiMac->GetAssociationID (),
                                                    startAllocation + guardTime.GetMicroSeconds (), sp3Duration);
        }
    }
}

int
main (int argc, char *argv[])
{
  uint32_t packetSize = 1472;                   /* Transport Layer Payload size in bytes. */
  string dataRate1 = "50Mbps";                  /* Application Layer Data Rate for WestNode->EastNode. */
  string dataRate2 = "40Mbps";                  /* Application Layer Data Rate for WestNode->SouthNode. */
  string dataRate3 = "100Mbps";                 /* Application Layer Data Rate for ApNode->WestNode. */
  uint32_t msduAggregationSize = 7935;          /* The maximum aggregation size for A-MSDU in Bytes. */
  uint32_t queueSize = 1000;                    /* Wifi Mac Queue Size. */
  string phyMode = "DMG_MCS12";                 /* Type of the Physical Layer. */
  bool verbose = false;                         /* Print Logging Information. */
  double simulationTime = 10;                   /* Simulation time in seconds. */
  bool pcapTracing = false;                     /* PCAP Tracing is enabled or not. */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("packetSize", "Payload size in bytes", packetSize);
  cmd.AddValue ("dataRate1", "Data rate for OnOff Application WestNode->EastNode", dataRate1);
  cmd.AddValue ("dataRate2", "Data rate for OnOff Application WestNode->SouthNode", dataRate2);
  cmd.AddValue ("dataRate3", "Data rate for OnOff Application ApNode->WestNode", dataRate3);
  cmd.AddValue ("msduAggregation", "The maximum aggregation size for A-MSDU in Bytes", msduAggregationSize);
  cmd.AddValue ("queueSize", "The size of the Wifi Mac Queue", queueSize);
  cmd.AddValue ("sp1Duration", "The duration of service period (1) in MicroSeconds", sp1Duration);
  cmd.AddValue ("sp2Duration", "The duration of service period (2) in MicroSeconds", sp2Duration);
  cmd.AddValue ("sp3Duration", "The duration of service period (3) in MicroSeconds", sp3Duration);
  cmd.AddValue ("phyMode", "802.11ad PHY Mode", phyMode);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable PCAP Tracing", pcapTracing);
  cmd.Parse (argc, argv);

  /* Global params: no fragmentation, no RTS/CTS, fixed rate for all packets */
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));

  /**** WifiHelper is a meta-helper: it helps creates helpers ****/
  DmgWifiHelper wifi;

  /* Basic setup */
  wifi.SetStandard (WIFI_PHY_STANDARD_80211ad);

  /* Turn on logging */
  if (verbose)
    {
      wifi.EnableLogComponents ();
      LogComponentEnable ("EvaluateServicePeriod", LOG_LEVEL_ALL);
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

  /* Make four nodes and set them up with the PHY and the MAC */
  NodeContainer wifiNodes;
  wifiNodes.Create (4);
  Ptr<Node> apNode = wifiNodes.Get (0);
  Ptr<Node> westNode = wifiNodes.Get (1);
  Ptr<Node> southNode = wifiNodes.Get (2);
  Ptr<Node> eastNode = wifiNodes.Get (3);

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

  /* Set Simple Analytical Codebook for the DMG Devices */
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

  /* Install Simple UDP Server on both south and east Node */
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9999));
  ApplicationContainer sinks = sinkHelper.Install (NodeContainer (eastNode, southNode, westNode));
  sink1 = StaticCast<PacketSink> (sinks.Get (0));
  sink2 = StaticCast<PacketSink> (sinks.Get (1));
  sink3 = StaticCast<PacketSink> (sinks.Get (2));

  /* Install Simple UDP Transmiter on the West Node (Transmit to the East Node) */
  ApplicationContainer srcApp;
  OnOffHelper src;
  src.SetAttribute ("Remote", AddressValue (InetSocketAddress (staInterfaces.GetAddress (2), 9999)));
  src.SetAttribute ("MaxBytes", UintegerValue (0));
  src.SetAttribute ("PacketSize", UintegerValue (packetSize));
  src.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
  src.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  src.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate1)));
  srcApp.Add (src.Install (westNode));

  /* Install Simple UDP Transmiter on the West Node (Transmit to the South Node) */
  src.SetAttribute ("Remote", AddressValue (InetSocketAddress (staInterfaces.GetAddress (1), 9999)));
  src.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate2)));
  srcApp.Add (src.Install (westNode));

  /* Install Simple UDP Transmiter on the AP Node (Transmit to the West Node) */
  src.SetAttribute ("Remote", AddressValue (InetSocketAddress (staInterfaces.GetAddress (0), 9999)));
  src.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate3)));
  srcApp.Add (src.Install (apNode));

  /* Start and stop applications */
  srcApp.Start (Seconds (3.0));
  srcApp.Stop (Seconds (simulationTime));
  sinks.Start (Seconds (3.0));

  /* Set the maximum number of packets in WifiMacQueue */
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxPackets", UintegerValue (queueSize));

  /* Schedule Throughput Calulcations */
  Simulator::Schedule (Seconds (3.1), &CalculateThroughput);

  /* Enable Traces */
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
      wifiPhy.SetSnapshotLength (120);
      wifiPhy.EnablePcap ("Traces/AccessPoint", apDevice, false);
      wifiPhy.EnablePcap ("Traces/WestNode", staDevices.Get (0), false);
      wifiPhy.EnablePcap ("Traces/SouthNode", staDevices.Get (1), false);
      wifiPhy.EnablePcap ("Traces/EastNode", staDevices.Get (2), false);
    }

  /* Install FlowMonitor on all nodes */
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

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

  /* Print Output */
  std::cout << std::left << std::setw (12) << "Time  [s]"
            << std::left << std::setw (12) << "SP1 [Mbps]"
            << std::left << std::setw (12) << "SP2 [Mbps]"
            << std::left << std::setw (12) << "SP3 [Mbps]" << std::endl;

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
  Ptr<OnOffApplication> onoff;
  Ptr<PacketSink> sink;
  std::cout << "\nApplication Layer Statistics:" << std::endl;;
  for (uint8_t i = 0; i < srcApp.GetN (); i++)
    {
      onoff = StaticCast<OnOffApplication> (srcApp.Get (i));
      sink = StaticCast<PacketSink> (sinks.Get (i));
      std::cout << "Stats (" << i + 1 << ")" << std::endl;
      std::cout << "  Tx Packets: " << onoff->GetTotalTxPackets () << std::endl;
      std::cout << "  Tx Bytes:   " << onoff->GetTotalTxBytes () << std::endl;
      std::cout << "  Rx Packets: " << sink->GetTotalReceivedPackets () << std::endl;
      std::cout << "  Rx Bytes:   " << sink->GetTotalRx () << std::endl;
      std::cout << "  Throughput: " << sink->GetTotalRx () * 8.0 / ((simulationTime - 3) * 1e6) << " Mbps" << std::endl;
    }

  return 0;
}
