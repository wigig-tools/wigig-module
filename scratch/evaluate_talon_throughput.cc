/*
 * Copyright (c) 2015-2020 IMDEA Networks Institute
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
#include <string>
#include <iomanip>

/**
 * Simulation Objective:
 * Evaluate the performance of the CSMA/CA using TP-LIMK AD7200 TALON Router confiugrations.
 * The TALON Router uses the following configuration:
 * 1. A-MPDU aggregation is enabled with aggregation up-to 32 packets or 65,535 Bytes.
 * 2. A-MSDU aggregation is disabled.
 * 3. SC PHY is used and MCS is fixed to MCS-12 (Can be changed).
 * 4. The BTI compormises 32 sectors.
 * 5. The SLS comromises 35 sectors.
 * 6. ATI Present is false.
 * 7. TALON Router antenna array beam patterns are utilized.
 *
 * Network Topology:
 * The scenario consists of a signle DMG STA and a single DMG PCP/AP.
 *
 *
 *          DMG PCP/AP (0,0)                       DMG STA (+1,0)
 *
 *
 * Simulation Description:
 * In the case of CSMA/CA access period, the whole DTI access period is allocated as a CBAP
 * channel access period. The DMG STA generates uplink traffic towards the DMG PCP/AP using
 * either UDP or TCP.
 *
 * Running Simulation:
 * To evaluate CSMA/CA channel access scheme:
 * ./waf --run "evaluate_talon_throughput --simulationTime=10 --pcap=true"
 *
 * Simulation Output:
 * The simulation generates the following traces:
 * 1. PCAP traces for each station.
 * 2. Throughput using window size of 100 ms.
 * 3. Flows statistics.
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateTalonThroughput");

using namespace ns3;
using namespace std;

/**  Application Variables **/
uint64_t totalRx = 0;
double throughput = 0;
Ptr<PacketSink> packetSink;

/* Network Nodes */
Ptr<Node> apWifiNode;
Ptr<Node> staWifiNode;

void
CalculateThroughput (void)
{
  double thr = CalculateSingleStreamThroughput (packetSink, totalRx, throughput);
  std::cout << std::left << std::setw (12) << Simulator::Now ().GetSeconds ()
            << std::left << std::setw (12) << thr << std::endl;
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

int
main (int argc, char *argv[])
{
  string applicationType = "bulk";              /* Type of the Tx application */
  string socketType = "ns3::TcpSocketFactory";  /* Socket Type (TCP/UDP) */
  uint32_t packetSize = 1448;                   /* Application payload size in bytes. */
  string dataRate = "300Mbps";                  /* Application data rate. */
  string tcpVariant = "NewReno";                /* TCP Variant Type. */
  uint32_t bufferSize = 131072;                 /* TCP Send/Receive Buffer Size. */
  uint32_t maxPackets = 0;                      /* Maximum Number of Packets */
  string msduAggSize = "0";                     /* The maximum aggregation size for A-MSDU in Bytes. */
  string mpduAggSize = "65535";                 /* The maximum aggregation size for A-MPDU in Bytes. */
  string queueSize = "4000p";                   /* Wifi MAC Queue Size. */
  string phyMode = "DMG_MCS12";                 /* Type of the Physical Layer. */
  bool verbose = false;                         /* Print Logging Information. */
  double simulationTime = 10;                   /* Simulation time in seconds. */
  bool pcapTracing = false;                     /* PCAP Tracing is enabled or not. */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("applicationType", "Type of the Tx Application: onoff or bulk", applicationType);
  cmd.AddValue ("packetSize", "Application packet size in bytes", packetSize);
  cmd.AddValue ("dataRate", "Application data rate", dataRate);
  cmd.AddValue ("maxPackets", "Maximum number of packets to send", maxPackets);
  cmd.AddValue ("tcpVariant", TCP_VARIANTS_NAMES, tcpVariant);
  cmd.AddValue ("socketType", "Type of the Socket (ns3::TcpSocketFactory, ns3::UdpSocketFactory)", socketType);
  cmd.AddValue ("bufferSize", "TCP Buffer Size (Send/Receive) in Bytes", bufferSize);
  cmd.AddValue ("msduAggSize", "The maximum aggregation size for A-MSDU in Bytes", msduAggSize);
  cmd.AddValue ("mpduAggSize", "The maximum aggregation size for A-MPDU in Bytes", mpduAggSize);
  cmd.AddValue ("queueSize", "The maximum size of the Wifi MAC Queue", queueSize);
  cmd.AddValue ("phyMode", "802.11ad PHY Mode", phyMode);
  cmd.AddValue ("verbose", "Turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable PCAP Tracing", pcapTracing);
  cmd.Parse (argc, argv);

  /* Validate A-MSDU and A-MPDU values */
  ValidateFrameAggregationAttributes (msduAggSize, mpduAggSize);
  /* Configure RTS/CTS and Fragmentation */
  ConfigureRtsCtsAndFragmenatation ();
  /* Wifi MAC Queue Parameters */
  ChangeQueueSize (queueSize);

  /*** Configure TCP Options ***/
  ConfigureTcpOptions (tcpVariant, packetSize, bufferSize);

  /**** DmgWifiHelper is a meta-helper ****/
  DmgWifiHelper wifi;

  /* Basic setup */
  wifi.SetStandard (WIFI_PHY_STANDARD_80211ad);

  /* Turn on logging */
  if (verbose)
    {
      wifi.EnableLogComponents ();
      LogComponentEnable ("EvaluateTalonThroughput", LOG_LEVEL_ALL);
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
  /* The TALON Router does not support OFDM */
  wifiPhy.Set ("SupportOfdmPhy", BooleanValue (false));
  /* Set operating channel */
  wifiPhy.Set ("ChannelNumber", UintegerValue (2));
  /* Set default algorithm for all nodes to be constant rate */
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue (phyMode));

  /* Make two nodes and set them up with the PHY and the MAC */
  NodeContainer wifiNodes;
  wifiNodes.Create (2);
  apWifiNode = wifiNodes.Get (0);
  staWifiNode = wifiNodes.Get (1);

  /* Add a DMG upper mac */
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

  Ssid ssid = Ssid ("TALON");
  wifiMac.SetType ("ns3::DmgApWifiMac",
                   "Ssid", SsidValue(ssid),
                   "BE_MaxAmpduSize", StringValue (mpduAggSize),
                   "BE_MaxAmsduSize", StringValue (msduAggSize),
                   "SSSlotsPerABFT", UintegerValue (1), "SSFramesPerSlot", UintegerValue (16),
                   "BeaconInterval", TimeValue (MicroSeconds (102400)));

  /* Set Numerical Codebook for the DMG PCP/AP */
  wifi.SetCodebook ("ns3::CodebookNumerical",
                    "FileName", StringValue ("WigigFiles/Codebook/NUMERICAL_TALONAD7200_AP.txt"));

  NetDeviceContainer apDevice;
  apDevice = wifi.Install (wifiPhy, wifiMac, apWifiNode);

  /* Set Numerical Codebook for the DMG STA */
  wifi.SetCodebook ("ns3::CodebookNumerical",
                    "FileName", StringValue ("WigigFiles/Codebook/NUMERICAL_TALONAD7200_STA.txt"));

  wifiMac.SetType ("ns3::DmgStaWifiMac",
                   "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false),
                   "BE_MaxAmpduSize", StringValue (mpduAggSize),
                   "BE_MaxAmsduSize", StringValue (msduAggSize));

  NetDeviceContainer staDevice;
  staDevice = wifi.Install (wifiPhy, wifiMac, staWifiNode);

  /* Setting mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));	/* DMG PCP/AP */
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
  PacketSinkHelper sinkHelper (socketType, InetSocketAddress (Ipv4Address::GetAny (), 9999));
  ApplicationContainer sinkApp = sinkHelper.Install (apWifiNode);
  packetSink = StaticCast<PacketSink> (sinkApp.Get (0));
  sinkApp.Start (Seconds (0.0));

  /* Install TCP/UDP Transmitter on the DMG STA */
  Address dest (InetSocketAddress (apInterface.GetAddress (0), 9999));
  ApplicationContainer srcApp;
  Ptr<OnOffApplication> onoff;
  Ptr<BulkSendApplication> bulk;
  if (applicationType == "onoff")
    {
      OnOffHelper src (socketType, dest);
      src.SetAttribute ("MaxPackets", UintegerValue (maxPackets));
      src.SetAttribute ("PacketSize", UintegerValue (packetSize));
      src.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
      src.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      src.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
      srcApp = src.Install (staWifiNode);
      onoff = StaticCast<OnOffApplication> (srcApp.Get (0));
    }
  else if (applicationType == "bulk")
    {
      BulkSendHelper src (socketType, dest);
      srcApp= src.Install (staWifiNode);
      bulk = StaticCast<BulkSendApplication> (srcApp.Get (0));
    }
  srcApp.Start (Seconds (1.0));
  srcApp.Stop (Seconds (simulationTime));

  /* Print Traces */
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
      wifiPhy.SetSnapshotLength (120);
      wifiPhy.EnablePcap ("Traces/AccessPoint", apDevice, false);
      wifiPhy.EnablePcap ("Traces/Station", staDevice, false);
    }

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
  PrintFlowMonitorStatistics (flowmon, monitor, simulationTime - 1);

  /* Print Application Layer Results Summary */
  std::cout << "\nApplication Layer Statistics:" << std::endl;;
  if (applicationType == "onoff")
    {
      std::cout << "  Tx Packets: " << onoff->GetTotalTxPackets () << std::endl;
      std::cout << "  Tx Bytes:   " << onoff->GetTotalTxBytes () << std::endl;
    }
  else
    {
      std::cout << "  Tx Packets: " << bulk->GetTotalTxPackets () << std::endl;
      std::cout << "  Tx Bytes:   " << bulk->GetTotalTxBytes () << std::endl;
    }

  std::cout << "  Rx Packets: " << packetSink->GetTotalReceivedPackets () << std::endl;
  std::cout << "  Rx Bytes:   " << packetSink->GetTotalRx () << std::endl;
  std::cout << "  Throughput: " << packetSink->GetTotalRx () * 8.0 / ((simulationTime - 1) * 1e6) << " Mbps" << std::endl;

  return 0;
}
