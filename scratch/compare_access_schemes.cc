/*
 * Copyright (c) 2015-2020 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
#include "common-functions.h"
#include <iomanip>

/**
 * Simulation Objective:
 * Compare the performance of the channel access schemes in IEEE 802.11ad/ay standards.
 * Basically, the simulation compares the achievable throughput between CSMA/CA and SP allocations.
 * Thw two devices support DMG/EDMG SC and OFDM PHY layers.
 *
 * Network Topology:
 * The scenario consists of signle DMG STA and single PCP/AP.
 *
 *          DMG PCP/AP (0,0)                       DMG STA (+1,0)
 *
 * Simulation Description:
 * In the case of CSMA/CA access period, the whole DTI access period is allocated as CBAP. Whereas in the
 * of SP allocation, once the DMG STA has assoicated successfully with the PCP/AP. The PCP/AP allocates the
 * whole DTI as SP allocation.
 *
 * Running Simulation:
 * To evaluate CSMA/CA channel access scheme using the IEEE 802.11ad standard:
 * ./waf --run "compare_access_schemes --scheme=1 --simulationTime=10 --pcap=true"
 *
 * To evaluate Service Period (SP) channel access scheme:
 * ./waf --run "compare_access_schemes --scheme=0 --simulationTime=10 --pcap=true"
 *
 * Simulation Output:
 * The simulation generates the following traces:
 * 1. PCAP traces for each station.
 * 2. The achieved throughput during a window of 100 ms.
 */

NS_LOG_COMPONENT_DEFINE ("CompareAccessSchemes");

using namespace ns3;
using namespace std;

/**  Application Variables **/
uint64_t totalRx = 0;
double throughput = 0;
Ptr<PacketSink> packetSink;

/* Network Nodes */
Ptr<Node> apWifiNode;
Ptr<Node> staWifiNode;
Ptr<WifiNetDevice> apWifiNetDevice;
Ptr<WifiNetDevice> staWifiNetDevice;
Ptr<DmgApWifiMac> apWifiMac;
Ptr<DmgStaWifiMac> staWifiMac;

/* Access Period Parameters */
uint32_t allocationType = CBAP_ALLOCATION;      /* The type of channel access scheme during DTI (CBAP is the default). */

void
CalculateThroughput (void)
{
  double thr = CalculateSingleStreamThroughput (packetSink, totalRx, throughput);
  std::cout << std::left << std::setw (12) << Simulator::Now ().GetSeconds ()
            << std::left << std::setw (12) << thr << std::endl;
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

void
StationAssoicated (Ptr<DmgStaWifiMac> staWifiMac, Mac48Address address, uint16_t aid)
{
  std::cout << "DMG STA " << staWifiMac->GetAddress () << " associated with DMG AP " << address << std::endl;
  std::cout << "Association ID (AID) = " << aid << std::endl;
  if (allocationType == SERVICE_PERIOD_ALLOCATION)
    {
      std::cout << "Allocate DTI as Service Period" << std::endl;
      apWifiMac->AllocateDTIAsServicePeriod (1, staWifiMac->GetAssociationID (), AID_AP);
    }
}

int
main (int argc, char *argv[])
{
  uint32_t payloadSize = 1472;                  /* Application payload size in bytes. */
  string dataRate = "300Mbps";                  /* Application data rate. */
  string msduAggSize = "max";                   /* The maximum aggregation size for A-MSDU in Bytes. */
  string mpduAggSize = "max";                   /* The maximum aggregation size for A-MPDU in Bytes. */
  bool enableRts = false;                       /* Flag to indicate if RTS/CTS handskahre is enabled or disabled. */
  uint32_t rtsThreshold = 0;                    /* RTS/CTS handshare threshold. */
  string queueSize = "4000p";                   /* Wifi MAC Queue Size. */
  string phyMode = "DMG_MCS12";                 /* Type of the Physical Layer. */
  string standard = "ad";                       /* The WiGig standard being utilized (ad/ay). */
  bool verbose = false;                         /* Print Logging Information. */
  double simulationTime = 10;                   /* Simulation time in seconds. */
  bool pcapTracing = false;                     /* PCAP Tracing is enabled. */
  uint32_t snapshotLength = std::numeric_limits<uint32_t>::max (); /* The maximum PCAP Snapshot Length. */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("payloadSize", "Application payload size in bytes", payloadSize);
  cmd.AddValue ("dataRate", "Application data rate", dataRate);
  cmd.AddValue ("msduAggSize", "The maximum aggregation size for A-MSDU in Bytes", msduAggSize);
  cmd.AddValue ("mpduAggSize", "The maximum aggregation size for A-MPDU in Bytes", mpduAggSize);
  cmd.AddValue ("scheme", "The access scheme used for channel access (0: SP allocation, 1: CBAP allocation)", allocationType);
  cmd.AddValue ("enableRts", "Enable or disable RTS/CTS handshake", enableRts);
  cmd.AddValue ("rtsThreshold", "The RTS/CTS threshold value", rtsThreshold);
  cmd.AddValue ("queueSize", "The maximum size of the Wifi MAC Queue", queueSize);
  cmd.AddValue ("phyMode", "The WiGig PHY Mode", phyMode);
  cmd.AddValue ("standard", "The WiGig standard being utilized (ad/ay)", standard);
  cmd.AddValue ("verbose", "Turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable PCAP Tracing", pcapTracing);
  cmd.AddValue ("snapshotLength", "The maximum PCAP snapshot length", snapshotLength);
  cmd.Parse (argc, argv);

  /* Validate WiGig standard value */
  WifiPhyStandard wifiStandard = WIFI_PHY_STANDARD_80211ad;
  if (standard == "ad")
    {
      wifiStandard = WIFI_PHY_STANDARD_80211ad;
    }
  else if (standard == "ay")
    {
      wifiStandard = WIFI_PHY_STANDARD_80211ay;
    }
  else
    {
      NS_FATAL_ERROR ("Wrong WiGig standard");
    }
  /* Validate A-MSDU and A-MPDU values */
  ValidateFrameAggregationAttributes (msduAggSize, mpduAggSize, wifiStandard);
  /* Configure RTS/CTS and Fragmentation */
  ConfigureRtsCtsAndFragmenatation (enableRts, rtsThreshold);
  /* Wifi MAC Queue Parameters */
  ChangeQueueSize (queueSize);

  /**** WifiHelper is a meta-helper: it helps creates helpers ****/
  DmgWifiHelper wifi;
  wifi.SetStandard (wifiStandard);

  /* Turn on logging */
  if (verbose)
    {
      wifi.EnableLogComponents ();
      LogComponentEnable ("CompareAccessSchemes", LOG_LEVEL_ALL);
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
  /* Add support for the OFDM PHY */
  wifiPhy.Set ("SupportOfdmPhy", BooleanValue (true));
  if (standard == "ay")
    {
      /* Set the correct error model */
      wifiPhy.SetErrorRateModel ("ns3::DmgErrorModel",
                                 "FileName", StringValue ("DmgFiles/ErrorModel/LookupTable_1458_ay.txt"));
    }
  /* Set default algorithm for all nodes to be constant rate */
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue (phyMode));

  /* Make two nodes and set them up with the PHY and the MAC */
  NodeContainer wifiNodes;
  wifiNodes.Create (2);
  apWifiNode = wifiNodes.Get (0);
  staWifiNode = wifiNodes.Get (1);

  /* Add a DMG upper mac */
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

  Ssid ssid = Ssid ("Compare");
  wifiMac.SetType ("ns3::DmgApWifiMac",
                   "Ssid", SsidValue(ssid),
                   "BE_MaxAmpduSize", StringValue (mpduAggSize),
                   "BE_MaxAmsduSize", StringValue (msduAggSize),
                   "SSSlotsPerABFT", UintegerValue (8), "SSFramesPerSlot", UintegerValue (8),
                   "BeaconInterval", TimeValue (MicroSeconds (102400)),
                   "EDMGSupported", BooleanValue ((standard == "ay")));

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
                   "BE_MaxAmpduSize", StringValue (mpduAggSize),
                   "BE_MaxAmsduSize", StringValue (msduAggSize),
                   "EDMGSupported", BooleanValue ((standard == "ay")));

  NetDeviceContainer staDevice;
  staDevice = wifi.Install (wifiPhy, wifiMac, staWifiNode);

  /* Setting mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));	/* PCP/AP */
  positionAlloc->Add (Vector (1.0, 0.0, 0.0));  /* DMG STA */

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
  src.SetAttribute ("MaxPackets", UintegerValue (0));
  src.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  src.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
  src.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  src.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  srcApp = src.Install (staWifiNode);
  srcApp.Start (Seconds (1.0));
  srcApp.Stop (Seconds (simulationTime));

  /* Print Traces */
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

      wifiPhy.EnablePcap ("Traces/AccessPoint", apDevice, false);
      wifiPhy.EnablePcap ("Traces/Station", staDevice, false);
    }

  /* Stations */
  apWifiNetDevice = StaticCast<WifiNetDevice> (apDevice.Get (0));
  staWifiNetDevice = StaticCast<WifiNetDevice> (staDevice.Get (0));
  apWifiMac = StaticCast<DmgApWifiMac> (apWifiNetDevice->GetMac ());
  staWifiMac = StaticCast<DmgStaWifiMac> (staWifiNetDevice->GetMac ());

  /* Connect Traces */
  staWifiMac->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, staWifiMac));

  /* Print Output*/
  std::cout << std::left << std::setw (12) << "Time [s]"
            << std::left << std::setw (12) << "Throughput [Mbps]" << std::endl;

  /* Schedule Throughput Calulcations */
  Simulator::Schedule (Seconds (1.1), &CalculateThroughput);

  /* Install FlowMonitor on all nodes */
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor;
  monitor = flowmon.InstallAll ();

  Simulator::Stop (Seconds (simulationTime + 0.101));
  Simulator::Run ();
  Simulator::Destroy ();

  /* Print Flow-Monitor Statistics */
  PrintFlowMonitorStatistics (flowmon, monitor, simulationTime - 1);

  /* Print Results Summary */
  std::cout << "Total #Received Packets = " << packetSink->GetTotalReceivedPackets () << std::endl;
  std::cout << "Total Throughput [Mbps] = " << throughput/((simulationTime - 1) * 10) << std::endl;

  return 0;
}
