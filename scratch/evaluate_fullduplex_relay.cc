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
 * This script is used to evaluate IEEE 802.11ad relay operation using Link Switching Type + Full Duplex Amplify and Forward (FD-AF).
 * The scenario consists of 3 DMG STAs (Two REDS + 1 RDS) and one PCP/AP.
 * Note: The standard supports only unicast transmission for relay operation. The relay (RDS) is responsible for protecting the
 * period allocated between the source REDS and destinations REDS. In case, the source REDS does no receive Ack/BlockAck during link
 * change interval, the source REDS will defer its transmission by data sensing time which will implicitly signal the destination REDS
 * to switch to the relay link.
 *
 *                           DMG AP (0,1)
 *
 *
 * Source REDS (-1.73,0)                        Destination REDS (1.73,0)
 *
 *
 *                            RDS (0,-1)
 *
 * To use this script simply type the following run command:
 * ./waf --run "evaluate_fullduplex"
 *
 * The simulation generates four PCAP files for each node. You can check the traces which matches exactly
 * the procedure for relay search and relay link establishment defined in the amendment.
 */






/**
 *
 * Simulation Objective:
 * This script is used to evaluate IEEE 802.11ad relay operation for TCP connection using Link Switching Type working in
 * Half Duplex Decode and Forward relay mode. IEEE 802.11ad defines relay operation mode for SP protection against sudden
 * link interruptions.
 *
 * Network Topology:
 * The scenario consists of 3 DMG STAs (West STA, East STA and, one RDS) and a single PCP/AP.
 *
 *                           DMG AP (0,1)
 *
 *
 * West STA (-1.73,0)                         East STA (1.73,0)
 *
 *
 *                            RDS (0,-1)
 *
 * Simulation Description:
 * In this simulation scenario we use TCP as transport protocol. TCP requires bi-directional transmission. So we need to
 * establish forward and reverse SP allocations since the standard supports only unicast transmission for single SP allocation.
 * As a result, we create the following two SP allocations:
 *
 * SP1 for TCP Segments: West DMG STA -----> East DMG STA (8ms)
 * SP2 for TCP ACKs    : East DMG STA -----> West DMG STA (8ms)
 *
 * We swap betweeen those two SPs allocations during DTI access period up-to certain number of blocks as following:
 *
 * |-----SP1-----| |-----SP2-----| |-----SP1-----| |-----SP2-----| |-----SP1-----| |-----SP2-----|
 *
 * We add guard time between these consecutive SP allocations around 5us for protection.
 *
 * At the beginning each station requests information regarding the capabilities of all other stations. Once this is completed
 * West STA initiates Relay Discovery Procedure. During the relay discovery procedure, WEST STA performs Beamforming Training
 * with EAST STA and all the available RDSs. After WEST STA completes BF with the EAST STA it can establish service period for
 * direct communication without gonig throught the DMG PCP/AP. Once the RLS is completed, we repeat the same previous steps to
 * establish relay link from East STA to West STA. At this point, we schedule the previous SP allocations during DTI period.
 *
 * During the course of the simulation, we implicitly inform all the stations about relay switching decision. The user can enable
 * or disable relay switching per SP allocation.
 *
 * Running Simulation:
 * ./waf --run "evaluate_halfduplex_relay --simulationTime=10 --pcap=true"
 *
 * Output:
 * The simulation generates the following traces:
 * 1. PCAP traces for each station.
 * 2. ASCII traces corresponding to TCP socket information (CWND, RWND, and RTT).
 * 3. ASCII traces corresponding to Wifi MAC Queue size changes for each DMG STA.
 */






NS_LOG_COMPONENT_DEFINE ("EvaluateFullDuplexRelayOperation");

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

Ptr<YansWifiChannel> adChannel;
Ptr<WifiPhy> srcWifiPhy;
Ptr<WifiPhy> dstWifiPhy;

/*** Access Point Variables ***/
Ptr<PacketSink> sink;
uint64_t lastTotalRx = 0;
double averagethroughput = 0;
uint8_t assoicatedStations = 0;           /* Total number of assoicated stations with the AP */
uint8_t stationsTrained = 0;              /* Number of BF trained stations */

void
CalculateThroughput ()
{
  Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
  double cur = (sink->GetTotalRx() - lastTotalRx) * (double) 8/1e5;     /* Convert Application RX Packets to MBits. */
  std::cout << now.GetSeconds () << '\t' << cur << std::endl;
  lastTotalRx = sink->GetTotalRx ();
  averagethroughput += cur;
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

void
RlsCompleted (Mac48Address address)
{
  std::cout << "RLS Procedure is completed with " << address  << " at " << Simulator::Now ().As (Time::S) << std::endl;
  /* Schedule an SP for the communication between the source REDS and destination REDS */
  std::cout << "Allocating static service period for communication between " << srcRedsMac->GetAddress ()
            << " and " << dstRedsMac->GetAddress () << std::endl;
  apWifiMac->AllocateSingleContiguousBlock (1, SERVICE_PERIOD_ALLOCATION, true, srcRedsMac->GetAssociationID (),
                                            dstRedsMac->GetAssociationID (), 0, 32767);
}

void
SLSCompleted (Ptr<DmgStaWifiMac> staWifiMac, Mac48Address address,
              ChannelAccessPeriod accessPeriod, SECTOR_ID sectorId, ANTENNA_ID antennaId)
{
  if (accessPeriod == CHANNEL_ACCESS_DTI)
    {
      std::cout << "DMG STA " << staWifiMac->GetAddress () << " completed SLS phase with DMG STA " << address << std::endl;
      if ((rdsMac->GetAddress () == staWifiMac->GetAddress ()) &&
          ((srcRedsMac->GetAddress () == address) || (dstRedsMac->GetAddress () == address)))
        {
          stationsTrained++;
          if (stationsTrained == 2)
            {
              std::cout << "The RDS completed BF Training with both the source REDS and the destination REDS" << std::endl;
              /* Send Channel Measurement Request to the RDS */
              srcRedsMac->SendChannelMeasurementRequest (Mac48Address::ConvertFrom (rdsNetDevice->GetAddress ()), 10);
            }
        }
      else if ((srcRedsMac->GetAddress () == staWifiMac->GetAddress ()) && (dstRedsMac->GetAddress () == address))
        {
          std::cout << "The source REDS completed BF Training with the destination REDS" << std::endl;
          /* Send Channel Measurement Request to the destination REDS */
          srcRedsMac->SendChannelMeasurementRequest (Mac48Address::ConvertFrom (dstRedsNetDevice->GetAddress ()), 10);
        }
    }
}

void
ChannelReportReceived (Mac48Address address)
{
  if (rdsMac->GetAddress () == address)
    {
      std::cout << "Received Channel Measurement Response from the RDS" << std::endl;
      /* TxSS for the Link Between the Source REDS + the Destination REDS */
      apWifiMac->AllocateBeamformingServicePeriod (srcRedsMac->GetAssociationID (), dstRedsMac->GetAssociationID (), 0, true);
    }
  else if (dstRedsMac->GetAddress () == address)
    {
      std::cout << "Received Channel Measurement Response from the destination REDS" << std::endl;
      std::cout << "We are ready to execute RLS procedure" << std::endl;
      /* Initiate Relay Link Switch Procedure */
      Simulator::ScheduleNow (&DmgStaWifiMac::StartRlsProcedure, srcRedsMac);
    }
}

void
TransmissionLinkChanged (Mac48Address address, TransmissionLink link)
{
  if (link == RELAY_LINK)
    {
      std::cout << "DMG STA " << address << " has changed its current transmission link to the relay link" << std::endl;
    }
}

uint8_t
SelectRelay (ChannelMeasurementInfoList rdsMeasurements, ChannelMeasurementInfoList dstRedsMeasurements, Mac48Address &rdsAddress)
{
  rdsAddress = rdsMac->GetAddress ();
  return rdsMac->GetAssociationID ();
}

void
TearDownRelay (Ptr<YansWifiChannel> channel)
{
  srcRedsMac->TeardownRelay (srcRedsMac->GetAssociationID (),
                             dstRedsMac->GetAssociationID (),
                             rdsMac->GetAssociationID ());
}

/************* Functions related to implicit link switching signaling *********************/

bool droppedPacket = false;
bool insertPacketDropper = false;

bool
GetPacketDropValue (void)
{
  if (!droppedPacket)
    {
      std::cout << "Dropped packet from Destination REDS to source REDS" << std::endl;
      droppedPacket = true;
      return true;
    }
  else
    {
      return false;
    }
}

void
InsertPacketDropper ()
{
  std::cout << "Packet Dropper Inserted at " << Simulator::Now ().As (Time::S) << std::endl;
  insertPacketDropper = true;
}

void
ServicePeriodStarted (Mac48Address source, Mac48Address destination)
{
  if (insertPacketDropper)
    {
      std::cout << "Service Period for which we insert packet dropper has started at " << Simulator::Now () << std::endl;
      Simulator::Schedule (MilliSeconds (1), &YansWifiChannel::AddPacketDropper, adChannel, &GetPacketDropValue, dstWifiPhy, srcWifiPhy);
    }
}

void
ServicePeriodEnded (Mac48Address source, Mac48Address destination)
{
  if (insertPacketDropper && droppedPacket)
    {
      std::cout << "Service Period for which we insert packet dropper has ended at " << Simulator::Now () << std::endl;
      insertPacketDropper = false;
      adChannel->RemovePacketDropper ();
    }
}

int
main (int argc, char *argv[])
{
  uint32_t payloadSize = 1472;                  /* Transport Layer Payload size in bytes. */
  string dataRate = "150Mbps";                  /* Application Layer Data Rate. */
  uint32_t msduAggregationSize = 7935;          /* The maximum aggregation size for A-MSDU in Bytes. */
  uint32_t queueSize = 10000;                   /* Wifi Mac Queue Size. */
  uint32_t switchTime = 4;                      /* The time we switch to the relay link in Seconds */
  string phyMode = "DMG_MCS24";                 /* Type of the Physical Layer. */
  bool verbose = false;                         /* Print Logging Information. */
  double simulationTime = 10;                   /* Simulation time in seconds. */
  bool pcapTracing = false;                     /* PCAP Tracing is enabled or not. */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("dataRate", "Payload size in bytes", dataRate);
  cmd.AddValue ("msduAggregation", "The maximum aggregation size for A-MSDU in Bytes", msduAggregationSize);
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
      LogComponentEnable ("EvaluateFullDuplexRelayOperation", LOG_LEVEL_ALL);
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
                      "Sectors", UintegerValue (12),
                      "Antennas", UintegerValue (1));

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
  Ssid ssid = Ssid ("test802.11ad");
  wifiMac.SetType("ns3::DmgApWifiMac",
                  "Ssid", SsidValue(ssid),
                  "BE_MaxAmpduSize", UintegerValue (0),
                  "BE_MaxAmsduSize", UintegerValue (msduAggregationSize),
                  "SSSlotsPerABFT", UintegerValue (8), "SSFramesPerSlot", UintegerValue (12),
                  "BeaconInterval", TimeValue (MicroSeconds (100000)),
                  "BeaconTransmissionInterval", TimeValue (MicroSeconds (800)),
                  "ATIDuration", TimeValue (MicroSeconds (1000)));

  NetDeviceContainer apDevice;
  apDevice = wifi.Install (wifiPhy, wifiMac, apNode);

  /* Install RDS Node */
  wifiMac.SetType ("ns3::DmgStaWifiMac",
                   "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false),
                   "BE_MaxAmpduSize", UintegerValue (0),
                   "BE_MaxAmsduSize", UintegerValue (msduAggregationSize),
                   "RDSActivated", BooleanValue (true), "REDSActivated", BooleanValue (false),
                   "RDSLinkChangeInterval", UintegerValue (250));

  NetDeviceContainer rdsDevice;
  rdsDevice = wifi.Install (wifiPhy, wifiMac, rdsNode);

  /* Install REDS Nodes */
  wifiMac.SetType ("ns3::DmgStaWifiMac",
                   "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false),
                   "BE_MaxAmpduSize", UintegerValue (0),
                   "BE_MaxAmsduSize", UintegerValue (msduAggregationSize),
                   "RDSActivated", BooleanValue (false), "REDSActivated", BooleanValue (true),
                   "RDSDuplexMode", BooleanValue (true),
                   "RDSLinkChangeInterval", UintegerValue (250), "RDSDataSensingTime", UintegerValue (200));

  NetDeviceContainer redsDevices;
  redsDevices = wifi.Install (wifiPhy, wifiMac, NodeContainer (srcNode, dstNode));

  /* Setting mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, +1.0, 0.0));     /* PCP/AP */
  positionAlloc->Add (Vector (0.0, -1.0, 0.0));     /* RDS */
  positionAlloc->Add (Vector (-1.732, 0.0, 0.0));   /* Source REDS */
  positionAlloc->Add (Vector (+1.732, 0.0, 0.0));   /* Destination REDS */

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
  Ipv4InterfaceContainer endsInterface;
  endsInterface = address.Assign (redsDevices);

  /* Populate routing table */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* We do not want any ARP packets */
  PopulateArpCache ();

  /* Install Simple UDP Server on the access point */
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9999));
  ApplicationContainer sinkApp = sinkHelper.Install (dstNode);
  sink = StaticCast<PacketSink> (sinkApp.Get (0));
  sinkApp.Start (Seconds (0.0));

  ApplicationContainer srcApp;
  OnOffHelper src ("ns3::UdpSocketFactory", InetSocketAddress (endsInterface.GetAddress (1), 9999));
  src.SetAttribute ("MaxBytes", UintegerValue (0));
  src.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  src.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
  src.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  src.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  srcApp = src.Install (srcNode);
  srcApp.Start (Seconds (2.0));
  Simulator::Schedule (Seconds (2.1), &CalculateThroughput);

  /* Enable Traces */
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
      wifiPhy.EnablePcap ("Traces/AccessPoint", apDevice, false);
      wifiPhy.EnablePcap ("Traces/RDS", rdsDevice, false);
      wifiPhy.EnablePcap ("Traces/REDS", redsDevices, false);
    }

  /** Connect Trace Sources **/
  apWifiNetDevice = StaticCast<WifiNetDevice> (apDevice.Get (0));
  srcRedsNetDevice = StaticCast<WifiNetDevice> (redsDevices.Get (0));
  dstRedsNetDevice = StaticCast<WifiNetDevice> (redsDevices.Get (1));
  rdsNetDevice = StaticCast<WifiNetDevice> (rdsDevice.Get (0));

  apWifiMac = StaticCast<DmgApWifiMac> (apWifiNetDevice->GetMac ());
  srcRedsMac = StaticCast<DmgStaWifiMac> (srcRedsNetDevice->GetMac ());
  dstRedsMac = StaticCast<DmgStaWifiMac> (dstRedsNetDevice->GetMac ());
  rdsMac = StaticCast<DmgStaWifiMac> (rdsNetDevice->GetMac ());

  adChannel = StaticCast<YansWifiChannel> (srcRedsNetDevice->GetChannel ());
  srcWifiPhy = srcRedsNetDevice->GetPhy ();
  dstWifiPhy = dstRedsNetDevice->GetPhy ();

  srcRedsMac->RegisterRelaySelectorFunction (MakeCallback (&SelectRelay));

  /* For implicit signaling, we insert packet dropper at the start of the service period between the source and destination REDS */
  srcRedsMac->TraceConnectWithoutContext ("ServicePeriodStarted", MakeCallback (&ServicePeriodStarted));
  srcRedsMac->TraceConnectWithoutContext ("ServicePeriodEnded", MakeCallback (&ServicePeriodEnded));

  srcRedsMac->TraceConnectWithoutContext ("RlsCompleted", MakeCallback (&RlsCompleted));
  srcRedsMac->TraceConnectWithoutContext ("ChannelReportReceived", MakeCallback (&ChannelReportReceived));

  /* Traces related to Beamforming (TxSS)*/
  srcRedsMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, srcRedsMac));
  dstRedsMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, dstRedsMac));
  rdsMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, rdsMac));

  /* Traces related to link swtiching */
  srcRedsMac->TraceConnectWithoutContext ("TransmissionLinkChanged", MakeCallback (&TransmissionLinkChanged));
  dstRedsMac->TraceConnectWithoutContext ("TransmissionLinkChanged", MakeCallback (&TransmissionLinkChanged));

  /** Schedule Events **/
  /* Request the DMG Capabilities of other DMG STAs */
  Simulator::Schedule (Seconds (1.05), &DmgStaWifiMac::RequestInformation, srcRedsMac, dstRedsMac->GetAddress ());
  Simulator::Schedule (Seconds (1.06), &DmgStaWifiMac::RequestInformation, srcRedsMac, rdsMac->GetAddress ());
  Simulator::Schedule (Seconds (1.07), &DmgStaWifiMac::RequestInformation, rdsMac, srcRedsMac->GetAddress ());
  Simulator::Schedule (Seconds (1.08), &DmgStaWifiMac::RequestInformation, rdsMac, dstRedsMac->GetAddress ());
  Simulator::Schedule (Seconds (1.09), &DmgStaWifiMac::RequestInformation, dstRedsMac, srcRedsMac->GetAddress ());
  Simulator::Schedule (Seconds (1.10), &DmgStaWifiMac::RequestInformation, dstRedsMac, rdsMac->GetAddress ());

  /* Initiate Relay Discovery Procedure */
  Simulator::Schedule (Seconds (3.0), &DmgStaWifiMac::StartRelayDiscovery, srcRedsMac, dstRedsMac->GetAddress ());

  /* Schedule link switch event */
  Simulator::Schedule (Seconds (switchTime), &InsertPacketDropper);

  /* Schedule tear down event */
  Simulator::Schedule (Seconds (switchTime + 3), &TearDownRelay, adChannel);

  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
