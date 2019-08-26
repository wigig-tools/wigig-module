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
 * Simulation Objective:
 * This script is used to evaluate IEEE 802.11ad relay operation for UDP connection using Link Switching Type working in
 * Half Duplex Decode and Forward relay mode. IEEE 802.11ad defines relay operation mode for SP protection against sudden
 * link interruptions.
 *
 * Network Topology:
 * The scenario consists of 3 DMG STAs (Two REDS + 1 RDS) and one PCP/AP.
 *
 *                         DMG AP (0,1)
 *
 *
 * Source REDS (-1,0)                        Destination REDS (1,0)
 *
 *
 *                          RDS (0,-1)
 *
 * Simulation Description:
 * At the beginning each station requests information regarding the capabilities of all other stations. Once this is completed
 * we initiate Relay Discovery Procedure. During the relay discovery procedure, the Source DMG performs Beamforming Training with
 * the destination REDS and all the available RDSs. After the source REDS completes BF with the destination REDS it can establish
 * service period for direct communication without gonig throught the DMG PCP/AP.
 *
 * The user is able to define his/her own algorithm for the selection of the best Relay Station (RDS) between the source REDS
 * and the destination REDS for data forwarding.
 *
 * Running Simulation:
 * To use this script simply type the following run command:
 * ./waf --run "evaluate_halfduplex_relay --simulationTime=10 --pcap=true"
 *
 * Output:
 * The simulation generates the following traces:
 * 1. PCAP traces for each station.
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateHalfDuplexRelayOperation");

using namespace ns3;
using namespace std;

Ptr<WifiNetDevice> apWifiNetDevice;
Ptr<WifiNetDevice> srcRedsNetDevice;
Ptr<WifiNetDevice> dstRedsNetDevice;
Ptr<WifiNetDevice> rdsNetDevice;

Ptr<DmgApWifiMac> apWifiMac;
Ptr<DmgStaWifiMac> srcRedsMac;
Ptr<DmgStaWifiMac> dstRedsMac;
Ptr<DmgStaWifiMac> rdsMac;

/*** Access Point Variables ***/
Ptr<PacketSink> sink;
uint64_t lastTotalRx = 0;
double averagethroughput = 0;

uint8_t assoicatedStations = 0;                   /* Total number of assoicated stations with the AP */
uint8_t stationsTrained = 0;                      /* Number of BF trained stations */
bool scheduledStaticPeriods = false;              /* Flag to indicate whether we scheduled Static Service Periods or not */

/*** Service Period Parameters ***/
uint16_t spDuration = MAX_SP_BLOCK_DURATION;      /* The duration of the allocated service period in MicroSeconds */
uint8_t spBlocks = 2;

void
CalculateThroughput ()
{
  Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
  double cur = (sink->GetTotalRx() - lastTotalRx) * (double) 8/1e5;     /* Convert Application RX Packets to MBits. */
  std::cout << std::left << std::setw (12) << now.GetSeconds ()
            << std::left << std::setw (12) << cur << std::endl;
  lastTotalRx = sink->GetTotalRx ();
  averagethroughput += cur;
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

void
StartRelayDiscoveryProcedure (Ptr<DmgStaWifiMac> srcRedsMac, Mac48Address dstRedsMac)
{
  std::cout << "Starting Relay Discovery Procedure:" << std::endl;
  srcRedsMac->StartRelayDiscovery (dstRedsMac);
}

void
RelayDiscoveryProcedureCompleted (RelayCapableStaList rdsList)
{
  std::cout << "Completed Relay Discovery Procedure and found (" << rdsList.size () << ") RDS:" << std::endl;
  for (RelayCapableStaListCI iter = rdsList.begin (); iter != rdsList.end (); iter++)
    {
      std::cout << "  Relay AID [" << iter->first << "]" << std::endl;
    }
  std::cout << "\nStarting RDS Selection Procedure" << std::endl;
}

void
ChannelReportReceived (Mac48Address address)
{
  if (rdsMac->GetAddress () == address)
    {
      std::cout << "  SRC REDS: Received Channel Measurement Response from the candidate RDS" << std::endl;
      /* TxSS for the Link Between the Source REDS + the Destination REDS */
      apWifiMac->AllocateBeamformingServicePeriod (srcRedsMac->GetAssociationID (), dstRedsMac->GetAssociationID (), 0, true);
    }
  else if (dstRedsMac->GetAddress () == address)
    {
      std::cout << "  SRC REDS: Received Channel Measurement Response from the destination REDS" << std::endl;
      std::cout << "  SRC REDS: We are ready to execute RLS procedure" << std::endl;
      std::cout << "Completed RDS Selection Procedure" << std::endl;
      /* Initiate Relay Link Switch Procedure */
      std::cout << "\nStarting RLS Procedure:" << std::endl;
      Simulator::ScheduleNow (&DmgStaWifiMac::StartRlsProcedure, srcRedsMac);
    }
}

uint8_t
SelectRelay (ChannelMeasurementInfoList rdsMeasurements, ChannelMeasurementInfoList dstRedsMeasurements, Mac48Address &rdsAddress)
{
  rdsAddress = rdsMac->GetAddress ();
  return rdsMac->GetAssociationID ();
}

void
RlsCompleted (Mac48Address address)
{
  std::cout << "RLS Procedure is completed with RDS: " << address  << " at " << Simulator::Now () << std::endl;
  std::cout << "We can switch to the relay link anytime" << std::endl;

  /* Schedule an SP for the communication between the source REDS and the destination REDS */
  std::cout << "Allocating static service period for communication in the next BI between " << srcRedsMac->GetAddress ()
            << " and " << dstRedsMac->GetAddress () << std::endl;
  NS_ASSERT_MSG (spDuration * spBlocks < apWifiMac->GetDTIDuration (), "Allocations cannot exceed DTI period");
  apWifiMac->AllocateMultipleContiguousBlocks (1, SERVICE_PERIOD_ALLOCATION, true, srcRedsMac->GetAssociationID (),
                                               dstRedsMac->GetAssociationID (), 0, spDuration, spBlocks);
}

void
SLSCompleted (Ptr<DmgWifiMac> staWifiMac, Mac48Address address, ChannelAccessPeriod accessPeriod,
              BeamformingDirection beamformingDirection, bool isInitiatorTxss, bool isResponderTxss,
              SECTOR_ID sectorId, ANTENNA_ID antennaId)
{
  if (accessPeriod == CHANNEL_ACCESS_DTI)
    {
      std::cout << "  DMG STA: " << staWifiMac->GetAddress () << " completed SLS phase with DMG STA: " << address << std::endl;
      if ((rdsMac->GetAddress () == staWifiMac->GetAddress ()) &&
          ((srcRedsMac->GetAddress () == address) || (dstRedsMac->GetAddress () == address)))
        {
          stationsTrained++;
          if (stationsTrained == 2)
            {
              std::cout << "  The RDS completed BF Training with both the source REDS and the destination REDS" << std::endl;
              /* Send Channel Measurement Request to the RDS */
              std::cout << "  SRC REDS: Send Channel Measurement Request to the candidate RDS" << std::endl;
              srcRedsMac->SendChannelMeasurementRequest (Mac48Address::ConvertFrom (rdsNetDevice->GetAddress ()), 10);
            }
        }
      else if ((srcRedsMac->GetAddress () == staWifiMac->GetAddress ()) && (dstRedsMac->GetAddress () == address))
        {
          std::cout << "  SRC REDS: Completed BF Training with the destination REDS" << std::endl;
          std::cout << "  SRC REDS: Send Channel Measurement Request to the destination REDS" << std::endl;

          /* Send Channel Measurement Request to the destination REDS */
          srcRedsMac->SendChannelMeasurementRequest (Mac48Address::ConvertFrom (dstRedsNetDevice->GetAddress ()), 10);
        }
    }
}

void
SwitchTransmissionLink (void)
{
  std::cout << "Switching transmission link from the Direct Link to the Relay Link" << std::endl;
  srcRedsMac->SwitchTransmissionLink (srcRedsMac->GetAssociationID (), dstRedsMac->GetAssociationID ());
  rdsMac->SwitchTransmissionLink (srcRedsMac->GetAssociationID (), dstRedsMac->GetAssociationID ());
  dstRedsMac->SwitchTransmissionLink (srcRedsMac->GetAssociationID (), dstRedsMac->GetAssociationID ());
}

void
TearDownRelay (void)
{
  std::cout << "Tearing-down Relay Link" << std::endl;
  srcRedsMac->TeardownRelayImmediately (srcRedsMac->GetAssociationID (),
                                        dstRedsMac->GetAssociationID (),
                                        rdsMac->GetAssociationID ());
  dstRedsMac->TeardownRelayImmediately (srcRedsMac->GetAssociationID (),
                                        dstRedsMac->GetAssociationID (),
                                        rdsMac->GetAssociationID ());
  rdsMac->TeardownRelayImmediately (srcRedsMac->GetAssociationID (),
                                    dstRedsMac->GetAssociationID (),
                                    rdsMac->GetAssociationID ());
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
  uint32_t packetSize = 1472;                   /* Transport Layer Payload size in bytes. */
  string dataRate = "100Mbps";                  /* Application Layer Data Rate. */
  uint32_t msduAggregationSize = 7935;          /* The maximum aggregation size for A-MSDU in Bytes. */
  uint32_t queueSize = 1000;                    /* Wifi Mac Queue Size. */
  uint16_t firstPeriod = 4000;                  /* The duration of the RDS first period in MicroSeconds. */
  uint16_t secondPeriod = 4000;                 /* The duration of the RDS second period in MicroSeconds. */
  double switchTime = 4;                        /* The time we switch to the relay link in Seconds. */
  double relayDuration = 3;                     /* The length of the period during which we use relay link. */
  string phyMode = "DMG_MCS12";                 /* Type of the Physical Layer. */
  bool verbose = false;                         /* Print Logging Information. */
  double simulationTime = 10;                   /* Simulation time in seconds. */
  bool pcapTracing = false;                     /* PCAP Tracing is enabled or not. */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("packetSize", "Payload size in bytes", packetSize);
  cmd.AddValue ("dataRate", "Payload size in bytes", dataRate);
  cmd.AddValue ("msduAggregation", "The maximum aggregation size for A-MSDU in Bytes", msduAggregationSize);
  cmd.AddValue ("queueSize", "The size of the Wifi Mac Queue", queueSize);
  cmd.AddValue ("spDuration", "The duration of the service period in MicroSeconds", spDuration);
  cmd.AddValue ("spBlocks", "The number of blocks making up SP allocation", spBlocks);
  cmd.AddValue ("firstPeriod", "The duration of the RDS first period in MicroSeconds", firstPeriod);
  cmd.AddValue ("secondPeriod", "The duration of the RDS second period in MicroSeconds", secondPeriod);
  cmd.AddValue ("switchTime", "The time a which we switch from the direct link to the relay link", switchTime);
  cmd.AddValue ("relayDuration", "The length of the duration during which we use relay link", relayDuration);
  cmd.AddValue ("phyMode", "The 802.11ad PHY Mode", phyMode);
  cmd.AddValue ("verbose", "Turn on all WifiNetDevice log components", verbose);
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
      LogComponentEnable ("EvaluateHalfDuplexRelayOperation", LOG_LEVEL_ALL);
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
  wifiNodes.Create (4);
  Ptr<Node> apNode = wifiNodes.Get (0);
  Ptr<Node> rdsNode = wifiNodes.Get (1);
  Ptr<Node> srcNode = wifiNodes.Get (2);
  Ptr<Node> dstNode = wifiNodes.Get (3);

  /* Add a DMG upper mac */
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

  /* Install PCP/AP Node */
  Ssid ssid = Ssid ("HD-DF");
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

  /* Install RDS Node */
  wifiMac.SetType ("ns3::DmgStaWifiMac",
                   "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false),
                   "BE_MaxAmpduSize", UintegerValue (0),
                   "BE_MaxAmsduSize", UintegerValue (msduAggregationSize),
                   "RDSActivated", BooleanValue (true),   //Activate RDS
                   "REDSActivated", BooleanValue (false));

  NetDeviceContainer rdsDevice;
  rdsDevice = wifi.Install (wifiPhy, wifiMac, rdsNode);

  /* Install REDS Nodes */
  wifiMac.SetType ("ns3::DmgStaWifiMac",
                   "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false),
                   "BE_MaxAmpduSize", UintegerValue (0),
                   "BE_MaxAmsduSize", UintegerValue (msduAggregationSize),
                   "RDSActivated", BooleanValue (false),
                   "REDSActivated", BooleanValue (true),  //Activate REDS
                   "RDSDuplexMode", BooleanValue (false),
                   "RDSDataSensingTime", UintegerValue (200),
                   "RDSFirstPeriod", UintegerValue (firstPeriod),
                   "RDSSecondPeriod", UintegerValue (secondPeriod));

  NetDeviceContainer redsDevices;
  redsDevices = wifi.Install (wifiPhy, wifiMac, NodeContainer (srcNode, dstNode));

  /* Setting mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, +1.0, 0.0));   /* PCP/AP */
  positionAlloc->Add (Vector (0.0, -1.0, 0.0));   /* RDS */
  positionAlloc->Add (Vector (-1.0, 0.0, 0.0));   /* Source REDS */
  positionAlloc->Add (Vector (+1.0, 0.0, 0.0));   /* Destination REDS */

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
  Ipv4InterfaceContainer rdsInterface;
  rdsInterface = address.Assign (rdsDevice);
  Ipv4InterfaceContainer redsInterfaces;
  redsInterfaces = address.Assign (redsDevices);

  /* Populate routing table */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* We do not want any ARP packets */
  PopulateArpCache ();

  /* Install Simple UDP Server on the access point */
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9999));
  ApplicationContainer sinkApp = sinkHelper.Install (dstNode);
  sink = StaticCast<PacketSink> (sinkApp.Get (0));

  ApplicationContainer srcApp;
  Ptr<OnOffApplication> onoff;
  OnOffHelper src ("ns3::UdpSocketFactory", InetSocketAddress (redsInterfaces.GetAddress (1), 9999));
  src.SetAttribute ("MaxBytes", UintegerValue (0));
  src.SetAttribute ("PacketSize", UintegerValue (packetSize));
  src.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
  src.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  src.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  srcApp = src.Install (srcNode);
  onoff = StaticCast<OnOffApplication> (srcApp.Get (0));
  Simulator::Schedule (Seconds (2.1), &CalculateThroughput);

  /* Schedule Applications */
  srcApp.Start (Seconds (2.0));
  srcApp.Stop (Seconds (simulationTime));
  sinkApp.Start (Seconds (0.0));

  /* Set Maximum number of packets in WifiMacQueue */
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxPackets", UintegerValue (queueSize));

  /* Enable Traces */
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
//      wifiPhy.SetSnapshotLength (120);
      wifiPhy.EnablePcap ("Traces/AccessPoint", apDevice, false);
      wifiPhy.EnablePcap ("Traces/RDS", rdsDevice, false);
      wifiPhy.EnablePcap ("Traces/REDS", redsDevices, false);
    }

  /* Install FlowMonitor on all nodes */
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  /** Connect Trace Sources **/
  apWifiNetDevice = StaticCast<WifiNetDevice> (apDevice.Get (0));
  srcRedsNetDevice = StaticCast<WifiNetDevice> (redsDevices.Get (0));
  dstRedsNetDevice = StaticCast<WifiNetDevice> (redsDevices.Get (1));
  rdsNetDevice = StaticCast<WifiNetDevice> (rdsDevice.Get (0));

  apWifiMac = StaticCast<DmgApWifiMac> (apWifiNetDevice->GetMac ());
  srcRedsMac = StaticCast<DmgStaWifiMac> (srcRedsNetDevice->GetMac ());
  dstRedsMac = StaticCast<DmgStaWifiMac> (dstRedsNetDevice->GetMac ());
  rdsMac = StaticCast<DmgStaWifiMac> (rdsNetDevice->GetMac ());

  srcRedsMac->RegisterRelaySelectorFunction (MakeCallback (&SelectRelay));
  srcRedsMac->TraceConnectWithoutContext ("RdsDiscoveryCompleted", MakeCallback (&RelayDiscoveryProcedureCompleted));
  srcRedsMac->TraceConnectWithoutContext ("RlsCompleted", MakeCallback (&RlsCompleted));
  srcRedsMac->TraceConnectWithoutContext ("ChannelReportReceived", MakeCallback (&ChannelReportReceived));
  srcRedsMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, srcRedsMac));
  dstRedsMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, dstRedsMac));
  rdsMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, rdsMac));

  /* Connect Wifi MAC Queue Occupancy */
  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> queueOccupanyStream;
  /* Trace DMG RDS MAC Queue Changes */
  queueOccupanyStream = asciiTraceHelper.CreateFileStream ("Traces/RDS_Queue.txt");
  Config::ConnectWithoutContext ("/NodeList/1/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/OccupancyChanged",
                                 MakeBoundCallback (&QueueOccupancyChange, queueOccupanyStream));
  /* Trace DMG Source REDS MAC Queue Changes */
  queueOccupanyStream = asciiTraceHelper.CreateFileStream ("Traces/Src_REDS_Queue.txt");
  Config::ConnectWithoutContext ("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/OccupancyChanged",
                                 MakeBoundCallback (&QueueOccupancyChange, queueOccupanyStream));

  /** Schedule Events **/
  /* Request the DMG Capabilities of other DMG STAs */
  Simulator::Schedule (Seconds (1.05), &DmgStaWifiMac::RequestRelayInformation, srcRedsMac, dstRedsMac->GetAddress ());
  Simulator::Schedule (Seconds (1.06), &DmgStaWifiMac::RequestRelayInformation, srcRedsMac, rdsMac->GetAddress ());
  Simulator::Schedule (Seconds (1.07), &DmgStaWifiMac::RequestRelayInformation, rdsMac, srcRedsMac->GetAddress ());
  Simulator::Schedule (Seconds (1.08), &DmgStaWifiMac::RequestRelayInformation, rdsMac, dstRedsMac->GetAddress ());
  Simulator::Schedule (Seconds (1.09), &DmgStaWifiMac::RequestRelayInformation, dstRedsMac, srcRedsMac->GetAddress ());
  Simulator::Schedule (Seconds (1.10), &DmgStaWifiMac::RequestRelayInformation, dstRedsMac, rdsMac->GetAddress ());

  /* Initiate Relay Discovery Procedure */
  Simulator::Schedule (Seconds (1.3), &StartRelayDiscoveryProcedure, srcRedsMac, dstRedsMac->GetAddress ());

  /* Schedule link switch event */
  Simulator::Schedule (Seconds (switchTime), &SwitchTransmissionLink);

  /* Schedule tear down event */
  Simulator::Schedule (Seconds (switchTime + relayDuration), &TearDownRelay);

  Simulator::Stop (Seconds (simulationTime + 0.201));
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
      std::cout << "  TxOffered:  " << i->second.txBytes * 8.0 / ((simulationTime - 2) * 1e6)  << " Mbps" << std::endl;;
      std::cout << "  Rx Packets: " << i->second.rxPackets << std::endl;;
      std::cout << "  Rx Bytes:   " << i->second.rxBytes << std::endl;
      std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / ((simulationTime - 2) * 1e6)  << " Mbps" << std::endl;;
    }

  /* Print Application Layer Results Summary */
  std::cout << "\nApplication Layer Statistics:" << std::endl;;
  std::cout << "  Tx Packets:   " << onoff->GetTotalTxPackets () << std::endl;
  std::cout << "  Tx Bytes:     " << onoff->GetTotalTxBytes () << std::endl;
  std::cout << "  Rx Packets:   " << sink->GetTotalReceivedPackets () << std::endl;
  std::cout << "  Rx Bytes:     " << sink->GetTotalRx () << std::endl;
  std::cout << "  Lost Packets: " << onoff->GetTotalTxPackets () - sink->GetTotalReceivedPackets () << std::endl;
  std::cout << "  Throughput:   " << sink->GetTotalRx () * 8.0 / ((simulationTime - 2) * 1e6) << " Mbps" << std::endl;

  return 0;
}
