/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
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
 * This script is used to evaluate IEEE 802.11ad relay operation for UDP connection using Link Switching Type working in
 * Half Duplex Decode and Forward relay mode. IEEE 802.11ad defines relay operation mode for SP protection against sudden
 * link interruptions.
 *
 * Network Topology:
 * The scenario consists of 3 DMG STAs (Two REDS + 1 RDS) and one PCP/AP.
 *
 *                        DMG AP (0,1)
 *
 *
 * West REDS (-1,0)                        East REDS (1,0)
 *
 *
 *                         RDS (0,-1)
 *
 * Simulation Description:
 * At the beginning each station requests information regarding the capabilities of all other stations. Once this is completed
 * we initiate Relay Discovery Procedure. During the relay discovery procedure, the Source DMG performs Beamforming Training with
 * the destination REDS and all the available RDSs. After the source REDS completes BF with the destination REDS it can establish
 * service period for direct communication without gonig throught the DMG PCP/AP.
 *
 * We establish forward and reverse SP allocations since the standard supports only unicast transmission for single SP allocation.
 * As a result, we create the following two SP allocations:
 *
 * SP1 for Forward Traffic : West REDS -----> East REDS (8ms)
 * SP2 for Reverse Traffic : East REDS -----> West REDS (8ms)
 *
 * The user is able to define his/her own algorithm for the selection of the best Relay Station (RDS) between the source REDS
 * and the destination REDS for data forwarding.
 *
 * Running Simulation:
 * To use this script simply type the following run command:
 * ./waf --run "evaluate_halfduplex_relay_udp --dataRate=800Mbps --simulationTime=10 --pcap=true"
 * Note: The default script switches the link for SP1 only.
 *
 * To switch the link for SP2, run the following command:
 * ./waf --run "evaluate_halfduplex_relay_udp --switchReverse=true --simulationTime=10 --pcap=true"
 *
 * Output:
 * The simulation generates the following traces:
 * 1. PCAP traces for each station.
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateHalfDuplexRelayOperationUDP");

using namespace ns3;
using namespace std;

/** West Node Allocation Variables **/
uint64_t westEastLastTotalRx = 0;
double westEastAverageThroughput = 0;
/** South Node Allocation Variables **/
uint64_t eastWestTotalRx = 0;
double eastWestAverageThroughput = 0;

Ptr<PacketSink> sink1, sink2;

/* DMG Devices */
Ptr<WifiNetDevice> apWifiNetDevice;
Ptr<WifiNetDevice> westRedsNetDevice;
Ptr<WifiNetDevice> eastRedsNetDevice;
Ptr<WifiNetDevice> rdsNetDevice;

Ptr<DmgApWifiMac> apWifiMac;
Ptr<DmgStaWifiMac> westRedsMac;
Ptr<DmgStaWifiMac> eastRedsMac;
Ptr<DmgStaWifiMac> rdsMac;

/*** Access Point Variables ***/
uint8_t assoicatedStations = 0;                   /* Total number of assoicated stations with the AP */
uint8_t stationsTrained = 0;                      /* Number of BF trained stations */
bool scheduledStaticPeriods = false;              /* Flag to indicate whether we scheduled Static Service Periods or not */
uint32_t channelMeasurementResponses = 0;

/*** Service Period Parameters ***/
uint16_t sp1Duration = 8000;                      /* The duration of the forward SP allocation in MicroSeconds */
uint16_t sp2Duration = 8000;                      /* The duration of the reverse SP allocation in MicroSeconds */
uint8_t spBlocks = 3;                             /* The number of SP allocations in one DTI */
uint16_t cbapDuration = 10000;                    /* The duration of the allocated CBAP period in MicroSeconds (10ms) */

bool switchForward = true;                        /* Switch the forward link. */
bool switchReverse = false;                       /* Switch the reverse link. */

typedef enum {
  FORWARD_DIRECTION = 0,
  REVERSE_DIRECTION = 1,
} RelayDiretion;

RelayDiretion m_relayDirection = FORWARD_DIRECTION;

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
  double thr1, thr2;
  thr1 = CalculateSingleStreamThroughput (sink1, westEastLastTotalRx, westEastAverageThroughput);
  thr2 = CalculateSingleStreamThroughput (sink2, eastWestTotalRx, eastWestAverageThroughput);
  std::cout << Simulator::Now ().GetSeconds () << '\t' << thr1 << '\t'<< thr2 << std::endl;
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

void
RlsCompleted (Ptr<DmgStaWifiMac> staWifiMac, Mac48Address address)
{
  if (staWifiMac == westRedsMac)
    {
      std::cout << "West STA: RLS Procedure is completed with RDS=" << address << " at " << Simulator::Now () << std::endl;
      std::cout << "East STA: Execute RLS procedure" << std::endl;
      m_relayDirection = REVERSE_DIRECTION;
      Simulator::ScheduleNow (&DmgStaWifiMac::StartRelayDiscovery, eastRedsMac, westRedsMac->GetAddress ());
    }
  else
    {
      uint32_t startTime = 0;
      std::cout << "East REDS: RLS Procedure is completed with RDS=" << address << " at " << Simulator::Now () << std::endl;

      /* Assetion check values */
      NS_ASSERT_MSG (((sp1Duration + sp2Duration) * (spBlocks)) < apWifiMac->GetDTIDuration (),
                     "Allocations cannot exceed DTI period");

      /* Schedule a CBAP allocation for communication between DMG STAs */
      startTime = apWifiMac->AllocateCbapPeriod (true, startTime, cbapDuration);

      /* Protection Period */
      startTime += GUARD_TIME.GetMicroSeconds ()/2;

      /* Schedule SP allocations for data communication between the source REDS and the destination REDS */
      std::cout << "Allocating static service period allocation for communication between "
                << westRedsMac->GetAddress () << " and " << eastRedsMac->GetAddress () << std::endl;
      startTime = apWifiMac->AddAllocationPeriod (1, SERVICE_PERIOD_ALLOCATION, true,
                                                  westRedsMac->GetAssociationID (), eastRedsMac->GetAssociationID (),
                                                  startTime, sp1Duration, sp2Duration, spBlocks);

      /* Protection Period */
      startTime += GUARD_TIME.GetMicroSeconds ()/2;

      std::cout << "Allocating static service period allocation for communication between "
                << eastRedsMac->GetAddress ()  << " and " << westRedsMac->GetAddress () << std::endl;

      startTime = apWifiMac->AddAllocationPeriod (2, SERVICE_PERIOD_ALLOCATION, true,
                                                  eastRedsMac->GetAssociationID (), westRedsMac->GetAssociationID (),
                                                  startTime, sp2Duration, sp1Duration, spBlocks);
    }
}

void
StartChannelMeasurements (Ptr<DmgStaWifiMac> srcRedsMac, Ptr<DmgStaWifiMac> dstRedsMac,
                          Ptr<DmgStaWifiMac> staWifiMac, Mac48Address address,
                          string srcName, string dstName)
{
  if ((rdsMac->GetAddress () == staWifiMac->GetAddress ()) &&
      ((srcRedsMac->GetAddress () == address) || (dstRedsMac->GetAddress () == address)))
    {
      stationsTrained++;
      if (stationsTrained == 2)
        {
          stationsTrained = 0;
          std::cout << "RDS: Completed BF Training with both " << srcName << " and " << dstName << std::endl;
          /* Send Channel Measurement Request from the source REDS to the RDS */
          std::cout << srcName << ": Send Channel Measurement Request to the candidate RDS" << std::endl;
          srcRedsMac->SendChannelMeasurementRequest (rdsMac->GetAddress (), 10);
        }
    }
  else if ((srcRedsMac->GetAddress () == staWifiMac->GetAddress ()) && (dstRedsMac->GetAddress () == address))
    {
      std::cout << srcName << ": Completed BF Training with " << dstName << std::endl;
      /* Send Channel Measurement Request to the destination REDS */
      std::cout << srcName << ": Send Channel Measurement Request to " << dstName << std::endl;
      srcRedsMac->SendChannelMeasurementRequest ((dstRedsMac->GetAddress ()), 10);
    }
}

void
SLSCompleted (Ptr<DmgStaWifiMac> staWifiMac, Mac48Address address,
              ChannelAccessPeriod accessPeriod, SECTOR_ID sectorId, ANTENNA_ID antennaId)
{
  if (accessPeriod == CHANNEL_ACCESS_DTI)
    {
      std::cout << "DMG STA " << staWifiMac->GetAddress () << " completed SLS phase with DMG STA " << address << std::endl;
      if (m_relayDirection == FORWARD_DIRECTION)
        {
          StartChannelMeasurements (westRedsMac, eastRedsMac, staWifiMac, address, "West STA", "East STA");
        }
      else
        {
          StartChannelMeasurements (eastRedsMac, westRedsMac, staWifiMac, address, "East STA", "West STA");
        }
    }
}

void
ProcessChannelReports (Ptr<DmgStaWifiMac> srcRedsMac, Ptr<DmgStaWifiMac> dstRedsMac, Mac48Address address,
                       string srcName, string dstName)
{
  if (address == rdsMac->GetAddress ())
    {
      std::cout << srcName << ": received Channel Measurement Response from the RDS" << std::endl;
      /* TxSS for the Link Between the Source REDS + the Destination REDS */
      apWifiMac->AllocateBeamformingServicePeriod (srcRedsMac->GetAssociationID (), dstRedsMac->GetAssociationID (), 0, true);
    }
  else if (address == dstRedsMac->GetAddress ())
    {
      std::cout << srcName << ": Received Channel Measurement Response from " << dstName << std::endl;
      std::cout << srcName << ": Execute RLS procedure" << std::endl;
      /* Initiate Relay Link Switch Procedure */
      Simulator::ScheduleNow (&DmgStaWifiMac::StartRlsProcedure, srcRedsMac);
    }
}

void
ChannelReportReceived (Mac48Address address)
{
  if (m_relayDirection == FORWARD_DIRECTION)
    {
      ProcessChannelReports (westRedsMac, eastRedsMac, address, "West STA", "East STA");
    }
  else
    {
      ProcessChannelReports (eastRedsMac, westRedsMac, address, "East STA", "West STA");
    }
}

uint8_t
SelectRelay (ChannelMeasurementInfoList rdsMeasurements, ChannelMeasurementInfoList dstRedsMeasurements, Mac48Address &rdsAddress)
{
  /* Make relay selection decision based on channel measurements */
  rdsAddress = rdsMac->GetAddress ();
  return rdsMac->GetAssociationID ();
}

void
SwitchTransmissionLink (Ptr<DmgStaWifiMac> srcRedsMac, Ptr<DmgStaWifiMac> dstRedsMac)
{
  std::cout << "Switching transmission link from the Direct Link to the Relay Link for SP Allocation:"
            << "SRC AID=" << uint (srcRedsMac->GetAssociationID ())
            << ", DST AID=" << uint (dstRedsMac->GetAssociationID ()) << std::endl;
  rdsMac->SwitchTransmissionLink (srcRedsMac->GetAssociationID (), dstRedsMac->GetAssociationID ());
  srcRedsMac->SwitchTransmissionLink (srcRedsMac->GetAssociationID (), dstRedsMac->GetAssociationID ());
  dstRedsMac->SwitchTransmissionLink (srcRedsMac->GetAssociationID (), dstRedsMac->GetAssociationID ());
}

void
TearDownRelay (Ptr<DmgStaWifiMac> srcRedsMac, Ptr<DmgStaWifiMac> dstRedsMac)
{
  std::cout << "Tearing-down Relay Link for SP Allocation:"
            << "SRC AID=" << uint (srcRedsMac->GetAssociationID ())
            << ", DST AID=" << uint (dstRedsMac->GetAssociationID ()) << std::endl;
  srcRedsMac->TeardownRelay (srcRedsMac->GetAssociationID (),
                             dstRedsMac->GetAssociationID (),
                             rdsMac->GetAssociationID ());
}

/**
 * Callback method to log changes of the bytes in WifiMacQueue
 */
void
BytesInQueueTrace (Ptr<OutputStreamWrapper> stream, uint64_t oldVal, uint64_t newVal)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << " " << newVal << std::endl;
}

int
main (int argc, char *argv[])
{
  uint32_t payloadSize = 1472;                  /* Transport Layer Payload size in bytes. */
  string dataRate = "100Mbps";                  /* Application Layer Data Rate. */
  uint32_t msduAggregationSize = 7935;          /* The maximum aggregation size for A-MSDU in Bytes. */
  uint32_t queueSize = 1000;                    /* Wifi Mac Queue Size. */
  uint16_t firstPeriod = 4000;                  /* The duration of the RDS first period in MicroSeconds. */
  uint16_t secondPeriod = 4000;                 /* The duration of the RDS second period in MicroSeconds. */
  uint32_t switchTime = 4;                      /* The time we switch to the relay link in Seconds */
  string phyMode = "DMG_MCS12";                 /* Type of the Physical Layer. */
  bool verbose = false;                         /* Print Logging Information. */
  double simulationTime = 10;                   /* Simulation time in seconds. */
  bool pcapTracing = false;                     /* PCAP Tracing is enabled or not. */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("dataRate", "Payload size in bytes", dataRate);
  cmd.AddValue ("msduAggregation", "The maximum aggregation size for A-MSDU in Bytes", msduAggregationSize);
  cmd.AddValue ("queueSize", "The size of the Wifi Mac Queue", queueSize);
  cmd.AddValue ("sp1Duration", "The duration of the forward SP allocation in MicroSeconds", sp1Duration);
  cmd.AddValue ("sp2Duration", "The duration of the reverse SP allocation in MicroSeconds", sp2Duration);
  cmd.AddValue ("spBlocks", "The number of blocks making up SP allocation", spBlocks);
  cmd.AddValue ("cbapDuration", "The duration of the allocated CBAP period in MicroSeconds (10ms)", cbapDuration);
  cmd.AddValue ("firstPeriod", "The duration of the RDS first period in MicroSeconds", firstPeriod);
  cmd.AddValue ("secondPeriod", "The duration of the RDS second period in MicroSeconds", secondPeriod);
  cmd.AddValue ("switchTime", "The time a which we switch from the direct link to the relay link", switchTime);
  cmd.AddValue ("switchForward", "Switch the forward link", switchForward);
  cmd.AddValue ("switchReverse", "Switch the reverse link", switchReverse);
  cmd.AddValue ("phyMode", "The 802.11ad PHY Mode", phyMode);
  cmd.AddValue ("verbose", "Turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable PCAP Tracing", pcapTracing);
  cmd.Parse (argc, argv);

  /* Global params: no fragmentation, no RTS/CTS, fixed rate for all packets */
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));

  /**** WifiHelper is a meta-helper: it helps creates helpers ****/
  WifiHelper wifi;

  /* Basic setup */
  wifi.SetStandard (WIFI_PHY_STANDARD_80211ad);

  /* Turn on logging */
  if (verbose)
    {
      wifi.EnableLogComponents ();
      LogComponentEnable ("EvaluateHalfDuplexRelayOperation", LOG_LEVEL_ALL);
    }

  /**** Set up Channel ****/
  YansWifiChannelHelper wifiChannel ;
  /* Simple propagation delay model */
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  /* Friis model with standard-specific wavelength */
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (60.48e9));

  /**** Setup physical layer ****/
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
  wifiPhy.Set ("RxNoiseFigure", DoubleValue (10));
  wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-79));
  wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-79 + 3));
  /* Set the phy layer error model */
  wifiPhy.SetErrorRateModel ("ns3::SensitivityModel60GHz");
  /* Set default algorithm for all nodes to be constant rate */
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "ControlMode", StringValue (phyMode),
                                                                "DataMode", StringValue (phyMode));
  /* Give all nodes directional antenna */
  wifiPhy.EnableAntenna (true, true);
  wifiPhy.SetAntenna ("ns3::Directional60GhzAntenna",
                      "Sectors", UintegerValue (8),
                      "Antennas", UintegerValue (1));

  /* Make four nodes and set them up with the phy and the mac */
  NodeContainer wifiNodes;
  wifiNodes.Create (4);
  Ptr<Node> apNode = wifiNodes.Get (0);
  Ptr<Node> rdsNode = wifiNodes.Get (1);
  Ptr<Node> westNode = wifiNodes.Get (2);
  Ptr<Node> eastNode = wifiNodes.Get (3);

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
                   "BeaconTransmissionInterval", TimeValue (MicroSeconds (600)),
                   "ATIPresent", BooleanValue (false));

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
  redsDevices = wifi.Install (wifiPhy, wifiMac, NodeContainer (westNode, eastNode));

  /* Setting mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, +1.0, 0.0));   /* PCP/AP */
  positionAlloc->Add (Vector (0.0, -1.0, 0.0));   /* RDS */
  positionAlloc->Add (Vector (-1.0, 0.0, 0.0));   /* West REDS */
  positionAlloc->Add (Vector (+1.0, 0.0, 0.0));   /* East REDS */

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

  /* Install Simple UDP Server on the East STA */
  PacketSinkHelper sinkHelper1 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 5001));
  PacketSinkHelper sinkHelper2 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 5002));
  sink1 = StaticCast<PacketSink> (sinkHelper1.Install (eastNode).Get (0));
  sink2 = StaticCast<PacketSink> (sinkHelper2.Install (westNode).Get (0));

  /* Install Simple UDP Transmiter on the West Node (Transmit to the East Node) */
  ApplicationContainer srcApp;
  OnOffHelper src;
  src.SetAttribute ("Remote", AddressValue (InetSocketAddress (redsInterfaces.GetAddress (1), 5001)));
  src.SetAttribute ("MaxBytes", UintegerValue (0));
  src.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  src.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
  src.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  src.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  srcApp = src.Install (westNode);
  srcApp.Start (Seconds (3.0));

  /* Install Simple UDP Transmiter on the South Node (Transmit to the East Node) */
  src.SetAttribute ("Remote", AddressValue (InetSocketAddress (redsInterfaces.GetAddress (0), 5002)));
  srcApp = src.Install (eastNode);
  srcApp.Start (Seconds (3.0));

  /* Schedule Throughput Calulcation */
  Simulator::Schedule (Seconds (3.1), &CalculateThroughput);

  /** Connect Trace Sources **/
  apWifiNetDevice = StaticCast<WifiNetDevice> (apDevice.Get (0));
  westRedsNetDevice = StaticCast<WifiNetDevice> (redsDevices.Get (0));
  eastRedsNetDevice = StaticCast<WifiNetDevice> (redsDevices.Get (1));
  rdsNetDevice = StaticCast<WifiNetDevice> (rdsDevice.Get (0));

  /* Set Maximum number of packets in WifiMacQueue */
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/DcaTxop/Queue/MaxPackets", UintegerValue (queueSize));
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxPackets", UintegerValue (queueSize));
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::DmgWifiMac/SPQueue/MaxPackets", UintegerValue (queueSize));

  /* Enable Traces */
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
      wifiPhy.EnablePcap ("Traces/AccessPoint", apWifiNetDevice, false);
      wifiPhy.EnablePcap ("Traces/RDS", rdsNetDevice, false);
      wifiPhy.EnablePcap ("Traces/WEST", westRedsNetDevice, false);
      wifiPhy.EnablePcap ("Traces/EAST", eastRedsNetDevice, false);
    }

  apWifiMac = StaticCast<DmgApWifiMac> (apWifiNetDevice->GetMac ());
  westRedsMac = StaticCast<DmgStaWifiMac> (westRedsNetDevice->GetMac ());
  eastRedsMac = StaticCast<DmgStaWifiMac> (eastRedsNetDevice->GetMac ());
  rdsMac = StaticCast<DmgStaWifiMac> (rdsNetDevice->GetMac ());

  westRedsMac->TraceConnectWithoutContext ("RlsCompleted", MakeBoundCallback (&RlsCompleted, westRedsMac));
  eastRedsMac->TraceConnectWithoutContext ("RlsCompleted", MakeBoundCallback (&RlsCompleted, eastRedsMac));

  westRedsMac->TraceConnectWithoutContext ("ChannelReportReceived", MakeCallback (&ChannelReportReceived));
  eastRedsMac->TraceConnectWithoutContext ("ChannelReportReceived", MakeCallback (&ChannelReportReceived));

  westRedsMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, westRedsMac));
  eastRedsMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, eastRedsMac));
  rdsMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, rdsMac));

  /* Relay Selector Function */
  westRedsMac->RegisterRelaySelectorFunction (MakeCallback (&SelectRelay));
  eastRedsMac->RegisterRelaySelectorFunction (MakeCallback (&SelectRelay));

  /* Print changes in number of bytes */
//  AsciiTraceHelper ascii;
//  Ptr<WifiMacQueue> westRedsMacQueue = westRedsMac->GetSPQueue ();
//  Ptr<WifiMacQueue> rdsMacQueue = rdsMac->GetSPQueue ();
//  Ptr<WifiMacQueue> eastRedsMacQueue = eastRedsMac->GetSPQueue ();
//  Ptr<OutputStreamWrapper> streamBytesInQueue1 = ascii.CreateFileStream ("Traces/WEST-STA-MAC-BytesInQueue.txt");
//  Ptr<OutputStreamWrapper> streamBytesInQueue2 = ascii.CreateFileStream ("Traces/RDS-MAC-BytesInQueue.txt");
//  Ptr<OutputStreamWrapper> streamBytesInQueue3 = ascii.CreateFileStream ("Traces/EAST-STA-MAC-BytesInQueue.txt");
//  westRedsMacQueue->TraceConnectWithoutContext ("BytesInQueue", MakeBoundCallback (&BytesInQueueTrace, streamBytesInQueue1));
//  rdsMacQueue->TraceConnectWithoutContext ("BytesInQueue", MakeBoundCallback (&BytesInQueueTrace, streamBytesInQueue2));
//  eastRedsMacQueue->TraceConnectWithoutContext ("BytesInQueue", MakeBoundCallback (&BytesInQueueTrace, streamBytesInQueue3));

  /** Schedule Events **/
  /* Request the DMG Capabilities of other DMG STAs */
  Simulator::Schedule (Seconds (1.05), &DmgStaWifiMac::RequestInformation, westRedsMac, eastRedsMac->GetAddress ());
  Simulator::Schedule (Seconds (1.06), &DmgStaWifiMac::RequestInformation, westRedsMac, rdsMac->GetAddress ());
  Simulator::Schedule (Seconds (1.07), &DmgStaWifiMac::RequestInformation, rdsMac, westRedsMac->GetAddress ());
  Simulator::Schedule (Seconds (1.08), &DmgStaWifiMac::RequestInformation, rdsMac, eastRedsMac->GetAddress ());
  Simulator::Schedule (Seconds (1.09), &DmgStaWifiMac::RequestInformation, eastRedsMac, westRedsMac->GetAddress ());
  Simulator::Schedule (Seconds (1.10), &DmgStaWifiMac::RequestInformation, eastRedsMac, rdsMac->GetAddress ());

  /* Initiate Relay Discovery Procedure */
  Simulator::Schedule (Seconds (1.3), &DmgStaWifiMac::StartRelayDiscovery, westRedsMac, eastRedsMac->GetAddress ());

  /* Schedule link switch event */
  if (switchForward)
    {
      Simulator::Schedule (Seconds (switchTime), &SwitchTransmissionLink, westRedsMac, eastRedsMac);
    }
  if (switchReverse)
    {
      Simulator::Schedule (Seconds (switchTime), &SwitchTransmissionLink, eastRedsMac, westRedsMac);
    }

  /* Schedule tear down event */
  if (switchForward)
    {
      Simulator::Schedule (Seconds (switchTime + 3), &TearDownRelay, westRedsMac, eastRedsMac);
    }
  if (switchReverse)
    {
      Simulator::Schedule (Seconds (switchTime + 3), &TearDownRelay, eastRedsMac, westRedsMac);
    }

  /* Print Output*/
  std::cout << "Time(s)" << '\t' << "A1" << '\t'<< "A2" << std::endl;

  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();
  Simulator::Destroy ();

  /* Print Results Summary */
  std::cout << "Total number of packets received during each channel time allocation:" << std::endl;
  std::cout << "SP1 = " << sink1->GetTotalReceivedPackets () << std::endl;
  std::cout << "SP2 = " << sink2->GetTotalReceivedPackets () << std::endl;

  std::cout << "Total throughput during each channel time allocation:" << std::endl;
  std::cout << "SP1 = " << westEastAverageThroughput/((simulationTime - 1) * 10) << std::endl;
  std::cout << "SP2 = " << eastWestAverageThroughput/((simulationTime - 1) * 10) << std::endl;

  return 0;
}

