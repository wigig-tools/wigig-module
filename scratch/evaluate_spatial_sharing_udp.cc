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
 *
 * Simulation Objective:
 * This script is used to evaluate spatial sharing and interference assessment as defined in IEEE 802.11ad.
 *
 * Network Topology:
 * The scenario consists of 4 DMG STAs and one PCP/AP as following:
 *
 *  DMG STA 1 (-2.0, +2.0)           DMG STA 2 (+2.0, +2.0)
 *
 *
 *
 *                  DMG AP (0.0, 0.0)
 *
 *
 *
 * DMG STA 3 (-2.0, -2.0)            DMG STA 4 (+2.0, -2.0)
 *
 * Simulation Description:
 * Once all the stations have assoicated successfully with the PCP/AP. The PCP/AP allocates two SPs
 * to perform TxSS between all the stations. Once West DMG STA has completed TxSS phase with East and
 * South DMG STAs. The PCP/AP will allocate two static service periods at the time (Spatial Sharing)
 * for communication as following:
 *
 * SP1: DMG STA (1)  ----->  DMG STA (2) (SP Length = 20ms)
 * SP2: DMG STA (3)  ----->  DMG STA (4) (SP Length = 12ms)
 *
 * Running the Simulation:
 * To run the script with the default parameters:
 * ./waf --run "evaluate_service_period_udp"
 *
 * To run the script with different duration for the allocations e.g. SP1=10ms and SP2=5ms:
 * ./waf --run "evaluate_service_period_udp --sp1Duration=10000 --sp2Duration=5000"
 *
 * Simulation Output:
 * The simulation generates the following traces:
 * 1. PCAP traces for each station. From the PCAP files, we can see that data transmission takes place
 * during its SP. In addition, we can notice in the announcement of the two Static Allocation Periods
 * inside each DMG Beacon.
 *
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateSpatialSharing");

using namespace ns3;
using namespace std;

/** DMG STA (1 & 2) Allocation Variables **/
uint64_t northLastTotalRx = 0;
uint64_t northLastTotalPackets = 0;
double northAverageThroughput = 0;
/** DMG STA (3 & 4) Allocation Variables **/
uint64_t southLastTotalRx = 0;
uint64_t southLastTotalPackets = 0;
double southAverageThroughput = 0;

Ptr<PacketSink> sink1, sink2;

/* Network Nodes */
Ptr<WifiNetDevice> apWifiNetDevice;
Ptr<WifiNetDevice> wifiNetDevice1, wifiNetDevice2, wifiNetDevice3, wifiNetDevice4;
NetDeviceContainer staDevices;

Ptr<DmgApWifiMac> apWifiMac;
Ptr<DmgStaWifiMac> wifiMac1, wifiMac2, wifiMac3, wifiMac4;

/*** Access Point Variables ***/
uint8_t assoicatedStations = 0;           /* Total number of assoicated stations with the AP */
uint8_t stationsTrained = 0;              /* Number of BF trained stations */
bool scheduledStaticPeriods = false;      /* Flag to indicate whether we scheduled Static Service Periods or not */

/*** Service Period Parameters ***/
uint16_t sp1Duration = 20000;             /* The duration of the allocated service period (1) in MicroSeconds */
uint16_t sp2Duration = 12000;             /* The duration of the allocated service period (2) in MicroSeconds */
uint32_t sp1StartTime;                    /* The start time of the allocated service period (1) in MicroSeconds */
uint32_t sp2StartTime;                    /* The start time of the allocated service period (2) in MicroSeconds */
uint16_t offsetDuration = 1000;           /* The offset between the start of the two service period allocations in MicroSeconds */
typedef std::map<Mac48Address, bool> ReportMap;
typedef ReportMap::const_iterator ReportMapCI;
ReportMap m_reportsStatus;

/* Spatial Sharing Parameters */
bool reportsReceived = false;             /* Initial quality reports received */
uint8_t periodicity = 4;                  /* Periodicity of spatial sharing check-up */
uint8_t currentPeriod;                    /* The remaining Beacon Intervals for the check-up */

double
CalculateSingleStreamThroughput (Ptr<PacketSink> sink, uint64_t &lastTotalRx, uint64_t &lastTotalPacket, double &averageThroughput)
{
  double thr = (sink->GetTotalRx() - lastTotalRx) * (double) 8/1e5;     /* Convert Application RX Packets to MBits. */
  lastTotalRx = sink->GetTotalRx ();
  lastTotalPacket = sink->GetTotalReceivedPackets ();
  averageThroughput += thr;
  return thr;
}

void
CalculateThroughput (void)
{
  double thr1, thr2;
  Time now = Simulator::Now ();
  thr1 = CalculateSingleStreamThroughput (sink1, northLastTotalRx, northLastTotalPackets, northAverageThroughput);
  thr2 = CalculateSingleStreamThroughput (sink2, southLastTotalRx, southLastTotalPackets, southAverageThroughput);
  std::cout << now.GetSeconds () << '\t' << thr1 << '\t'<< thr2 << std::endl;
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
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
      /* Schedule SP for Beamforming Training (SLS TxSS)*/
      uint32_t startTime = 0;
      startTime = apWifiMac->AllocateBeamformingServicePeriod (wifiMac1->GetAssociationID (),
                                                               wifiMac2->GetAssociationID (), startTime, true);
      startTime = apWifiMac->AllocateBeamformingServicePeriod (wifiMac3->GetAssociationID (),
                                                               wifiMac4->GetAssociationID (), startTime, true);
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
      if ((wifiMac1->GetAddress () == staWifiMac->GetAddress ()) || (wifiMac3->GetAddress () == staWifiMac->GetAddress ()))
        {
          stationsTrained++;
        }
      if ((stationsTrained == 2) & !scheduledStaticPeriods)
        {
          std::cout << "Schedule Allocation Periods" << std::endl;
          scheduledStaticPeriods = true;
          /* Schedule Allocation Periods */
          uint32_t startTime = 0;
          startTime = apWifiMac->AllocateCbapPeriod (true, startTime, 10000);
          sp1StartTime = apWifiMac->AllocateSingleContiguousBlock (1, SERVICE_PERIOD_ALLOCATION, true,
                                                                   wifiMac1->GetAssociationID (),
                                                                   wifiMac2->GetAssociationID (),
                                                                   startTime,
                                                                   sp1Duration);
          startTime = sp1StartTime;
          /* Candidate SP */
          sp2StartTime = apWifiMac->AllocateSingleContiguousBlock (2, SERVICE_PERIOD_ALLOCATION, true,
                                                                   wifiMac3->GetAssociationID (),
                                                                   wifiMac4->GetAssociationID (),
                                                                   startTime + offsetDuration,
                                                                   sp2Duration);
        }
    }
}

void
AssesstInterference (Mac48Address address, uint8_t peerAid, MeasurementMethod method,
                     uint32_t startTime, uint16_t spDuration, uint8_t blocks)
{
  Ptr<DirectionalChannelQualityRequestElement> element = Create<DirectionalChannelQualityRequestElement> ();
  element->SetOperatingClass (0);
  element->SetChannelNumber (0);
  element->SetAid (peerAid);
  element->SetMeasurementMethod (method);
  element->SetMeasurementStartTime (startTime);
  element->SetMeasurementDuration (spDuration);
  element->SetNumberOfTimeBlocks (blocks);
  apWifiMac->SendDirectionalChannelQualityRequest (address, 1, element);
}

void MeasureOverSP1 (MeasurementMethod method, uint32_t spStartTime, uint16_t spDuration)
{
  AssesstInterference (wifiMac3->GetAddress (), wifiMac4->GetAssociationID (),
                       method, spStartTime, spDuration, 10);
  AssesstInterference (wifiMac4->GetAddress (), wifiMac3->GetAssociationID (),
                       method, spStartTime, spDuration, 10);
}

void MeasureOverSP2 (MeasurementMethod method, uint32_t spStartTime, uint16_t spDuration)
{
  AssesstInterference (wifiMac1->GetAddress (), wifiMac2->GetAssociationID (),
                       method, spStartTime, spDuration, 10);
  AssesstInterference (wifiMac2->GetAddress (), wifiMac1->GetAssociationID (),
                       method, spStartTime, spDuration, 10);
}

void ClearReportsStatus (void)
{
  m_reportsStatus[wifiMac1->GetAddress ()] = false;
  m_reportsStatus[wifiMac2->GetAddress ()] = false;
  m_reportsStatus[wifiMac3->GetAddress ()] = false;
  m_reportsStatus[wifiMac4->GetAddress ()] = false;
}

bool CheckReportsAvailability (void)
{
  for (ReportMapCI it = m_reportsStatus.begin (); it != m_reportsStatus.end (); it++)
    {
      if (it->second == false)
        {
          return false;
        }
    }
  return true;
}

void
ChannelQualityReportReceived (Mac48Address address, Ptr<DirectionalChannelQualityReportElement> element)
{
  std::cout << "Received directional channel quality report from " << address << std::endl;
  m_reportsStatus[address] = true;
  if (CheckReportsAvailability ())
    {
      ClearReportsStatus ();
      if (!reportsReceived)
        {
          reportsReceived = true;
          currentPeriod = periodicity;
          std::cout << "All stations reported directional channel quality reports to the PCP/AP" << std::endl;
          /* Take decision based on the received measurements */
          std::cout << "Spatial sharing can be achieved" << std::endl;
          /* Re-schedule existing SP allocations */
          std::cout << "Reschedule existing SP allocations" << std::endl;
          apWifiMac->ModifyAllocation (1, wifiMac1->GetAssociationID (), wifiMac2->GetAssociationID (),
                                       sp1StartTime, sp1Duration + sp2Duration);
          apWifiMac->ModifyAllocation (2, wifiMac3->GetAssociationID (), wifiMac4->GetAssociationID (),
                                       sp1StartTime, sp1Duration + sp2Duration);
        }
      else
        {
          std::cout << "PCP/AP received periodic directional channel quality reports" << std::endl;
          /* Take decision based on the received measurements */
        }
    }
}

void
BeaconIntervalStarted (Mac48Address address)
{
  if (reportsReceived)
    {
      currentPeriod--;
      if (currentPeriod == 0)
        {
          /* Transmit periodically Directional Channel Quality Request to each spatial sharing capable STA*/
          std::cout << "Time for channel quality reporting" << std::endl;
          currentPeriod = periodicity;
          ClearReportsStatus ();
          MeasureOverSP1 (RSNI, sp1StartTime, sp1Duration + sp2Duration);
          MeasureOverSP2 (RSNI, sp1StartTime, sp1Duration + sp2Duration);
        }
    }
}

int
main (int argc, char *argv[])
{
  uint32_t payloadSize = 1472;                  /* Transport Layer Payload size in bytes. */
  string dataRate1 = "800Mbps";                 /* Application Layer Data Rate for node1->node2. */
  string dataRate2 = "200Mbps";                 /* Application Layer Data Rate for node3->node4. */
  uint32_t msduAggregationSize = 7935;          /* The maximum aggregation size for A-MSDU in Bytes. */
  uint32_t queueSize = 10000;                   /* Wifi Mac Queue Size. */
  string phyMode = "DMG_MCS12";                 /* Type of the Physical Layer. */
  bool verbose = false;                         /* Print Logging Information. */
  double simulationTime = 10;                   /* Simulation time in seconds. */
  bool pcapTracing = false;                     /* PCAP Tracing is enabled or not. */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("dataRate1", "Data rate for OnOff Application node1->node4", dataRate1);
  cmd.AddValue ("dataRate2", "Data rate for OnOff Application node1->node3", dataRate2);
  cmd.AddValue ("msduAggregation", "The maximum aggregation size for A-MSDU in Bytes", msduAggregationSize);
  cmd.AddValue ("queueSize", "The size of the Wifi Mac Queue", queueSize);
  cmd.AddValue ("sp1Duration", "The duration of service period (1) in MicroSeconds", sp1Duration);
  cmd.AddValue ("sp2Duration", "The duration of service period (2) in MicroSeconds", sp2Duration);
  cmd.AddValue ("offset", "The offset between the start of the two service periods", offsetDuration);
  cmd.AddValue ("periodicity", "Periodicity of spatial sharing check-up", periodicity);
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
      LogComponentEnable ("EvaluateSpatialSharing", LOG_LEVEL_ALL);
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
  wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-70));
  wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-70 + 3));
  /* Set default algorithm for all nodes to be constant rate */
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "ControlMode", StringValue (phyMode),
                                                                "DataMode", StringValue (phyMode));

  /* Make four nodes and set them up with the phy and the mac */
  NodeContainer wifiNodes;
  wifiNodes.Create (5);
  Ptr<Node> apNode = wifiNodes.Get (0);
  Ptr<Node> node1 = wifiNodes.Get (1);
  Ptr<Node> node2 = wifiNodes.Get (2);
  Ptr<Node> node3 = wifiNodes.Get (3);
  Ptr<Node> node4 = wifiNodes.Get (4);

  /* Add a DMG upper mac */
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

  /* Install DMG PCP/AP Node */
  Ssid ssid = Ssid ("SPSH");
  wifiMac.SetType ("ns3::DmgApWifiMac",
                   "Ssid", SsidValue(ssid),
                   "BE_MaxAmpduSize", UintegerValue (0),
                   "BE_MaxAmsduSize", UintegerValue (msduAggregationSize),
                   "SSSlotsPerABFT", UintegerValue (8), "SSFramesPerSlot", UintegerValue (8),
                   "BeaconInterval", TimeValue (MicroSeconds (102400)),
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

  staDevices = wifi.Install (wifiPhy, wifiMac, NodeContainer (node1, node2, node3, node4));

  /* Setting mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));        /* DMG PCP/AP */
  positionAlloc->Add (Vector (-2.0, +2.0, 0.0));      /* West DMG STA */
  positionAlloc->Add (Vector (+2.0, +2.0, 0.0));      /* North DMG STA */
  positionAlloc->Add (Vector (-2.0, -2.0, 0.0));      /* South DMG STA */
  positionAlloc->Add (Vector (+2.0, -2.0, 0.0));      /* East DMG STA */

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
  ApplicationContainer sinks = sinkHelper.Install (NodeContainer (node2, node4));
  sink1 = StaticCast<PacketSink> (sinks.Get (0));
  sink2 = StaticCast<PacketSink> (sinks.Get (1));

  /* Install Simple UDP Transmiter on the West Node (Transmit to the North Node) */
  ApplicationContainer srcApp;
  OnOffHelper src;
  src.SetAttribute ("Remote", AddressValue (InetSocketAddress (staInterfaces.GetAddress (1), 9999)));
  src.SetAttribute ("MaxBytes", UintegerValue (0));
  src.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  src.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
  src.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  src.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate1)));
  srcApp = src.Install (node1);
  srcApp.Start (Seconds (3.0));

  /* Install Simple UDP Transmiter on the South Node (Transmit to the East Node) */
  src.SetAttribute ("Remote", AddressValue (InetSocketAddress (staInterfaces.GetAddress (3), 9999)));
  src.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate2)));
  srcApp = src.Install (node3);
  srcApp.Start (Seconds (3.0));

  /* Set Maximum number of packets in WifiMacQueue */
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/DcaTxop/Queue/MaxPackets", UintegerValue (queueSize));
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxPackets", UintegerValue (queueSize));
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::DmgWifiMac/SPQueue/MaxPackets", UintegerValue (queueSize));

  /* Schedule Throughput Calulcations */
  Simulator::Schedule (Seconds (3.1), &CalculateThroughput);

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
  wifiNetDevice1 = StaticCast<WifiNetDevice> (staDevices.Get (0));
  wifiNetDevice2 = StaticCast<WifiNetDevice> (staDevices.Get (1));
  wifiNetDevice3 = StaticCast<WifiNetDevice> (staDevices.Get (2));
  wifiNetDevice4 = StaticCast<WifiNetDevice> (staDevices.Get (3));

  apWifiMac = StaticCast<DmgApWifiMac> (apWifiNetDevice->GetMac ());
  wifiMac1 = StaticCast<DmgStaWifiMac> (wifiNetDevice1->GetMac ());
  wifiMac2 = StaticCast<DmgStaWifiMac> (wifiNetDevice2->GetMac ());
  wifiMac3 = StaticCast<DmgStaWifiMac> (wifiNetDevice3->GetMac ());
  wifiMac4 = StaticCast<DmgStaWifiMac> (wifiNetDevice4->GetMac ());

  /** Connect Traces **/
  wifiMac1->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, wifiMac1));
  wifiMac2->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, wifiMac2));
  wifiMac3->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, wifiMac3));
  wifiMac4->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, wifiMac4));

  wifiMac1->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, wifiMac1));
  wifiMac2->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, wifiMac2));
  wifiMac3->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, wifiMac3));
  wifiMac4->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, wifiMac4));

  apWifiMac->TraceConnectWithoutContext ("BIStarted", MakeCallback (&BeaconIntervalStarted));
  apWifiMac->TraceConnectWithoutContext ("ChannelQualityReportReceived", MakeCallback (&ChannelQualityReportReceived));

  /*** Interference Assessment ***/
  /* Measure over existing SP1 */
  Simulator::Schedule (Seconds (5.0), &MeasureOverSP1, ANIPI, sp1StartTime, sp1Duration);

  /* Measure over candidate SP2 */
  Simulator::Schedule (Seconds (5.0), &MeasureOverSP2, ANIPI, sp2StartTime, sp2Duration);

  /* Print Output */
  std::cout << "Time(s)" << '\t' << "SP1" << '\t'<< "SP2" << std::endl;

  /* Initialize reports status list  */
  ClearReportsStatus ();

  Simulator::Stop (Seconds (simulationTime + 0.02));
  Simulator::Run ();

  /* Print Results Summary */
  std::cout << "Simulation ended at " << simulationTime << std::endl;
  std::cout << "Total number of packets received during each service period:" << std::endl;
  std::cout << "SP1 = " << northLastTotalPackets << std::endl;
  std::cout << "SP2 = " << southLastTotalPackets << std::endl;

  std::cout << "Total throughput during each service period:" << std::endl;
  std::cout << "SP1 = " << double (northLastTotalRx * 8) /((simulationTime - 3) * 1e6) << " Mbps" << std::endl;
  std::cout << "SP2 = " << double (southLastTotalRx * 8) /((simulationTime - 3) * 1e6) << " Mbps" << std::endl;

  Simulator::Destroy ();

  return 0;
}
