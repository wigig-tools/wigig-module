/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
#include "common-functions.h"

/**
 * Simulation Objective:
 * This script is used to evaluate allocation of Static Service Periods in IEEE 802.11ad.
 *
 * Network Topology:
 * The scenario consists of 4 DMG STAs (West + East + North + South) and one PCP/AP as following:
 *
 *                            North DMG STA2(0.0, +5.0)
 *
 *
 *
 * West DMG STA (-10.0, 0.0)        DMG AP (0.0, 0.0)          East DMG STA (+10.0, 0.0)
 *
 *
 *
 *                            South DMG STA (-0.0,-5.0)
 *
 * Simulation: Description:
 * Once all the stations have assoicated successfully with the PCP/AP. The PCP/AP allocates two SPs
 * to perform TxSS between all the stations. Once West DMG STA has completed TxSS phase with East and
 * South DMG STAs. The PCP/AP will allocate two static service periods at the time (Spatial Sharing)
 * for communication as following:
 *
 * SP1: West DMG STA -----> North DMG STA (SP Length = 3.2ms)
 * SP2: South DMG STA -----> East DMG STA (SP Length = 3.2ms)
 *
 * Output:
 * From the PCAP files, we can see that data transmission takes place during its SP. In addition, we can
 * notice in the announcement of the two Static Allocation Periods inside each DMG Beacon.
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateSpatialSharing");

using namespace ns3;
using namespace std;

/* Network Nodes */
Ptr<WifiNetDevice> apWifiNetDevice;
Ptr<WifiNetDevice> westWifiNetDevice, northWifiNetDevice, southWifiNetDevice, eastWifiNetDevice;
NetDeviceContainer staDevices;

Ptr<DmgApWifiMac> apWifiMac;
Ptr<DmgStaWifiMac> westWifiMac, northWifiMac, southWifiMac, eastWifiMac;

/*** Access Point Variables ***/
uint8_t assoicatedStations = 0;           /* Total number of assoicated stations with the AP */
uint8_t stationsTrained = 0;              /* Number of BF trained stations */
bool scheduledStaticPeriods = false;      /* Flag to indicate whether we scheduled Static Service Periods or not */

/*** Service Period ***/
uint16_t servicePeriodDuration = 3200;    /* The duration of the allocated service periods in MicroSeconds */
uint16_t offsetDuration = 3200;           /* The offset between the start of the two service periods in MicroSeconds */

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
StationAssoicated (Ptr<DmgStaWifiMac> staWifiMac, Mac48Address address)
{
  std::cout << "DMG STA " << staWifiMac->GetAddress () << " associated with DMG AP " << address << std::endl;
  std::cout << "Association ID (AID) = " << staWifiMac->GetAssociationID () << std::endl;
  assoicatedStations++;
  /* Check if all stations have associated with the AP */
  if (assoicatedStations == 4)
    {
      std::cout << "All stations got associated with " << address << std::endl;
      /* Map AID to MAC Addresses in each node instead of requesting information */
      Ptr<DmgStaWifiMac> sourceStaMac, destStaMac;
      for (NetDeviceContainer::Iterator i = staDevices.Begin (); i != staDevices.End (); ++i)
        {
          sourceStaMac = StaticCast<DmgStaWifiMac> (StaticCast<WifiNetDevice> (*i)->GetMac ());
          for (NetDeviceContainer::Iterator j = staDevices.Begin (); j != staDevices.End (); ++j)
            {
              if (i != j)
                {
                  destStaMac = StaticCast<DmgStaWifiMac> (StaticCast<WifiNetDevice> (*j)->GetMac ());
                  sourceStaMac->MapAidToMacAddress (destStaMac->GetAssociationID (), destStaMac->GetAddress ());
                }
            }
        }
      /* Schedule SP for Beamforming Training */
      apWifiMac->AllocateBeamformingServicePeriod (westWifiMac->GetAssociationID (), northWifiMac->GetAssociationID (), 0, true);
      apWifiMac->AllocateBeamformingServicePeriod (southWifiMac->GetAssociationID (), eastWifiMac->GetAssociationID (), 3000, true);
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
      if ((westWifiMac->GetAddress () == staWifiMac->GetAddress ()) || (southWifiMac->GetAddress () == staWifiMac->GetAddress ()))
        {
          stationsTrained++;
        }
      if ((stationsTrained == 2) & !scheduledStaticPeriods)
        {
          std::cout << "Schedule Static Periods" << std::endl;
          scheduledStaticPeriods = true;
          /* Schedule Static Periods */
          apWifiMac->AllocateSingleContiguousBlock (1, SERVICE_PERIOD_ALLOCATION, true, westWifiMac->GetAssociationID (),
                                                    northWifiMac->GetAssociationID (), 0, servicePeriodDuration);
          apWifiMac->AllocateSingleContiguousBlock (2, SERVICE_PERIOD_ALLOCATION, true, southWifiMac->GetAssociationID (),
                                                    eastWifiMac->GetAssociationID (), offsetDuration, servicePeriodDuration);
        }
    }
}

int
main (int argc, char *argv[])
{
  uint32_t payloadSize = 1472;                  /* Transport Layer Payload size in bytes. */
  string dataRate = "300Mbps";                  /* Application Layer Data Rate. */
  uint32_t msduAggregationSize = 7935;          /* The maximum aggregation size for A-MSDU in Bytes. */
  uint32_t queueSize = 10000;                   /* Wifi Mac Queue Size. */
  string phyMode = "DMG_MCS24";                 /* Type of the Physical Layer. */
  string path = "";                             /* The path of the antenna radiation pattern. */
  bool verbose = false;                         /* Print Logging Information. */
  double simulationTime = 10;                   /* Simulation time in seconds. */
  bool pcapTracing = false;                     /* PCAP Tracing is enabled or not. */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("dataRate", "Payload size in bytes", dataRate);
  cmd.AddValue ("msduAggregation", "The maximum aggregation size for A-MSDU in Bytes", msduAggregationSize);
  cmd.AddValue ("queueSize", "The size of the Wifi Mac Queue", queueSize);
  cmd.AddValue ("duration", "The duration of service period in MicroSeconds", servicePeriodDuration);
  cmd.AddValue ("offset", "The offset between the start of the two service periods", offsetDuration);
  cmd.AddValue ("phyMode", "802.11ad PHY Mode", phyMode);
  cmd.AddValue ("antennaPattern", "The path of the antenna radiation pattern generated by Matlab", path);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable PCAP Tracing", pcapTracing);
  cmd.Parse (argc, argv);

  /* Global params: no fragmentation, no RTS/CTS, fixed rate for all packets */
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::WifiMacQueue::MaxPacketNumber", UintegerValue (queueSize));

  /**** WifiHelper is a meta-helper: it helps creates helpers ****/
  DmgWifiHelper wifi;

  /* Basic setup */
  wifi.SetStandard (WIFI_PHY_STANDARD_80211ad);

  /* Turn on logging */
  if (verbose)
    {
      wifi.EnableLogComponents ();
      LogComponentEnable ("EvaluateSpatialSharing", LOG_LEVEL_ALL);
    }

  /**** Set up Channel ****/
  DmgWifiChannelHelper wifiChannel ;
  /* Simple propagation delay model */
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  /* Friis model with standard-specific wavelength */
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (56.16e9));

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
  wifiPhy.Set ("RxNoiseFigure", DoubleValue (3));
  wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-79));
  wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-79 + 3));
  /* Set default algorithm for all nodes to be constant rate */
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "ControlMode", StringValue (phyMode),
                                                                "DataMode", StringValue (phyMode));

  /* Make four nodes and set them up with the phy and the mac */
  NodeContainer wifiNodes;
  wifiNodes.Create (5);
  Ptr<Node> apNode = wifiNodes.Get (0);
  Ptr<Node> westNode = wifiNodes.Get (1);
  Ptr<Node> northNode = wifiNodes.Get (2);
  Ptr<Node> southNode = wifiNodes.Get (3);
  Ptr<Node> eastNode = wifiNodes.Get (4);

  /* Add a DMG upper mac */
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

  /* Install DMG PCP/AP Node */
  Ssid ssid = Ssid ("test802.11ad");
  wifiMac.SetType ("ns3::DmgApWifiMac",
                   "Ssid", SsidValue(ssid),
                   "BE_MaxAmpduSize", UintegerValue (0),
                   "BE_MaxAmsduSize", UintegerValue (msduAggregationSize),
                   "SSSlotsPerABFT", UintegerValue (8), "SSFramesPerSlot", UintegerValue (8),
                   "BeaconInterval", TimeValue (MilliSeconds (100)),
                   "ATIDuration", TimeValue (MicroSeconds (1000)));

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

  staDevices = wifi.Install (wifiPhy, wifiMac, NodeContainer (westNode, northNode, southNode, eastNode));

  /* Setting mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));     /* PCP/AP */
  positionAlloc->Add (Vector (-2.0, 0.0, 0.0));    /* West DMG STA */
  positionAlloc->Add (Vector (0.0, +1.0, 0.0));    /* North DMG STA 2 */
  positionAlloc->Add (Vector (0.0, -1.0, 0.0));    /* South DMG STA 3 */
  positionAlloc->Add (Vector (+2.0, 0.0, 0.0));    /* East DMG STA 4 */

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

  /* Install Simple UDP Server on North and East Nodes */
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9999));
  ApplicationContainer sinks = sinkHelper.Install (NodeContainer (northNode, eastNode));

  /** North Node Variables **/
  uint64_t northNodeLastTotalRx = 0;
  double northNodeAverageThroughput = 0;

  /* Install Simple UDP Transmiter on the West Node (Transmit to the North Node) */
  ApplicationContainer srcApp1;
  OnOffHelper src1 ("ns3::UdpSocketFactory", InetSocketAddress (staInterfaces.GetAddress (1), 9999));
  src1.SetAttribute ("MaxBytes", UintegerValue (0));
  src1.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  src1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
  src1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  src1.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  srcApp1 = src1.Install (westNode);
  srcApp1.Start (Seconds (3.0));

  /** Node 4 Variables **/
  uint64_t eastNodeLastTotalRx = 0;
  double eastNodeAverageThroughput = 0;

  /* Install Simple UDP Transmiter on the South Node (Transmit to the East Node) */
  ApplicationContainer srcApp2;
  OnOffHelper src2 ("ns3::UdpSocketFactory", InetSocketAddress (staInterfaces.GetAddress (3), 9999));
  src2.SetAttribute ("MaxBytes", UintegerValue (0));
  src2.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  src2.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
  src2.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  src2.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  srcApp2 = src2.Install (southNode);
  srcApp2.Start (Seconds (3.0));

  /* Schedule Throughput Calulcations */
  Simulator::Schedule (Seconds (3.1), &CalculateThroughput, StaticCast<PacketSink> (sinks.Get (0)),
                       northNodeLastTotalRx, northNodeAverageThroughput);

  Simulator::Schedule (Seconds (3.1), &CalculateThroughput, StaticCast<PacketSink> (sinks.Get (1)),
                       eastNodeLastTotalRx, eastNodeAverageThroughput);

  /* Enable Traces */
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
      wifiPhy.EnablePcap ("Traces/AccessPoint", apDevice, false);
      wifiPhy.EnablePcap ("Traces/West_STA", staDevices.Get (0), false);
      wifiPhy.EnablePcap ("Traces/North_STA", staDevices.Get (1), false);
      wifiPhy.EnablePcap ("Traces/South_STA", staDevices.Get (2), false);
      wifiPhy.EnablePcap ("Traces/East_STA", staDevices.Get (3), false);
    }

  /* DMG Stations */
  apWifiNetDevice = StaticCast<WifiNetDevice> (apDevice.Get (0));
  westWifiNetDevice = StaticCast<WifiNetDevice> (staDevices.Get (0));
  northWifiNetDevice = StaticCast<WifiNetDevice> (staDevices.Get (1));
  southWifiNetDevice = StaticCast<WifiNetDevice> (staDevices.Get (2));
  eastWifiNetDevice = StaticCast<WifiNetDevice> (staDevices.Get (3));

  apWifiMac = StaticCast<DmgApWifiMac> (apWifiNetDevice->GetMac ());
  westWifiMac = StaticCast<DmgStaWifiMac> (westWifiNetDevice->GetMac ());
  northWifiMac = StaticCast<DmgStaWifiMac> (northWifiNetDevice->GetMac ());
  southWifiMac = StaticCast<DmgStaWifiMac> (southWifiNetDevice->GetMac ());
  eastWifiMac = StaticCast<DmgStaWifiMac> (eastWifiNetDevice->GetMac ());

  /** Connect Traces **/
  westWifiMac->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, westWifiMac));
  northWifiMac->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, northWifiMac));
  southWifiMac->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, southWifiMac));
  eastWifiMac->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, eastWifiMac));

  westWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, westWifiMac));
  northWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, northWifiMac));
  southWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, southWifiMac));
  eastWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, eastWifiMac));

  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

