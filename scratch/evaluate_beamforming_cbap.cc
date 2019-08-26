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
 * This script is used to evaluate allocation of Beamforming Service Periods in IEEE 802.11ad.
 *
 * Network Topology:
 * The scenario consists of 2 DMG STAs (West + East) and one PCP/AP as following:
 *
 *                         DMG AP  (0,+1)
 *
 *
 *                         DMG STA (0,-1)
 *
 * Simulation Description:
 * Once all the stations have assoicated successfully with the PCP/AP, the PCP/AP allocates three SPs
 * to perform Beamforming Training (TxSS) as following:
 *
 * Running the Simulation:
 * To run the script with the default parameters:
 * ./waf --run "evaluate_beamforming_cbap
 *
 * Simulation Output:
 * The simulation generates the following traces:
 * 1. PCAP traces for each station. From the PCAP files, we can see the allocation of beamforming service periods.
 * 2. SNR Dump for each sector.
 */

NS_LOG_COMPONENT_DEFINE ("BeamformingCBAP");

using namespace ns3;
using namespace std;

/**  Application Variables **/
uint64_t totalRx = 0;
double throughput = 0;
Ptr<PacketSink> packetSink;
Ptr<OnOffApplication> onoff;
Ptr<BulkSendApplication> bulk;

/* Network Nodes */
Ptr<WifiNetDevice> apWifiNetDevice;
Ptr<WifiNetDevice> staWifiNetDevice;

NetDeviceContainer staDevices;

Ptr<DmgApWifiMac> apWifiMac;
Ptr<DmgStaWifiMac> staWifiMac;

/* Flow monitor */
Ptr<FlowMonitor> monitor;

/*** Access Point Variables ***/
uint8_t stationsTrained = 0;              /* Number of BF trained stations */

/*** Beamforming Service Periods ***/
uint8_t beamformedLinks = 0;              /* Number of beamformed links */

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
  std::cout << std::left << std::setw (12) << Simulator::Now ().GetSeconds ()
            << std::left << std::setw (12) << thr << std::endl;
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

void
StationAssoicated (Ptr<DmgStaWifiMac> staWifiMac, Mac48Address address, uint16_t aid)
{
  std::cout << "DMG STA " << staWifiMac->GetAddress () << " associated with DMG AP " << address << std::endl;
  std::cout << "Association ID (AID) = " << aid << std::endl;
  staWifiMac->InitiateTxssCbap (apWifiMac->GetAddress ());
}

void
SLSCompleted (Ptr<DmgWifiMac> wifiMac, Mac48Address address, ChannelAccessPeriod accessPeriod,
              BeamformingDirection beamformingDirection, bool isInitiatorTxss, bool isResponderTxss,
              SECTOR_ID sectorId, ANTENNA_ID antennaId)
{
  if (accessPeriod == CHANNEL_ACCESS_BHI)
    {
      if (wifiMac == apWifiMac)
        {
          std::cout << "DMG AP " << apWifiMac->GetAddress () << " completed SLS phase with DMG STA " << address << std::endl;
        }
      else
        {
          std::cout << "DMG STA " << wifiMac->GetAddress () << " completed SLS phase with DMG AP " << address << std::endl;
        }
      std::cout << "Best Tx Antenna Configuration: SectorID=" << uint (sectorId) << ", AntennaID=" << uint (antennaId) << std::endl;
    }
  else if (accessPeriod == CHANNEL_ACCESS_DTI)
    {
      beamformedLinks++;
      std::cout << "DMG STA " << wifiMac->GetAddress () << " completed SLS phase with DMG STA " << address << std::endl;
      std::cout << "The best antenna configuration is SectorID=" << uint32_t (sectorId)
                << ", AntennaID=" << uint32_t (antennaId) << std::endl;
      if (beamformedLinks == 2)
        {
          apWifiMac->PrintSnrTable ();
          staWifiMac->PrintSnrTable ();
        }
    }
}

void
ActiveTxSectorIDChanged (Ptr<DmgWifiMac> wifiMac, SectorID oldSectorID, SectorID newSectorID)
{
//  std::cout << "DMG STA: " << wifiMac->GetAddress () << " , SectorID=" << uint16_t (newSectorID) << std::endl;
}

int
main (int argc, char *argv[])
{
  bool activateApp = true;                      /* Flag to indicate whether we activate onoff or bulk App */
  string applicationType = "bulk";              /* Type of the Tx application */
  string socketType = "ns3::TcpSocketFactory";  /* Socket Type (TCP/UDP) */
  uint32_t packetSize = 1448;                   /* Application payload size in bytes. */
  string dataRate = "300Mbps";                  /* Application data rate. */
  string tcpVariant = "NewReno";                /* TCP Variant Type. */
  uint32_t bufferSize = 131072;                 /* TCP Send/Receive Buffer Size. */
  uint32_t maxPackets = 0;                      /* Maximum Number of Packets */
  uint32_t msduAggregationSize = 0;             /* The maximum aggregation size for A-MSDU in Bytes. */
  uint32_t mpduAggregationSize = 3000;         /* The maximum aggregation size for A-MSPU in Bytes. */
  uint32_t queueSize = 1000;                    /* Wifi MAC Queue Size. */
  string phyMode = "DMG_MCS12";                 /* Type of the Physical Layer. */
  bool verbose = false;                         /* Print Logging Information. */
  double simulationTime = 10;                   /* Simulation time in seconds. */
  bool pcapTracing = false;                     /* PCAP Tracing is enabled or not. */
  std::map<std::string, std::string> tcpVariants; /* List of the tcp Variants */

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
  cmd.AddValue ("activateApp", "Whether to activate data transmission or not", activateApp);
  cmd.AddValue ("applicationType", "Type of the Tx Application: onoff or bulk", applicationType);
  cmd.AddValue ("packetSize", "Application packet size in bytes", packetSize);
  cmd.AddValue ("dataRate", "Application data rate", dataRate);
  cmd.AddValue ("maxPackets", "Maximum number of packets to send", maxPackets);
  cmd.AddValue ("tcpVariant", "Transport protocol to use: TcpTahoe, TcpReno, TcpNewReno, TcpWestwood, TcpWestwoodPlus", tcpVariant);
  cmd.AddValue ("socketType", "Type of the Socket (ns3::TcpSocketFactory, ns3::UdpSocketFactory)", socketType);
  cmd.AddValue ("bufferSize", "TCP Buffer Size (Send/Receive) in Bytes", bufferSize);
  cmd.AddValue ("msduAggregation", "The maximum aggregation size for A-MSDU in Bytes", msduAggregationSize);
  cmd.AddValue ("mpduAggregation", "The maximum aggregation size for A-MPDU in Bytes", mpduAggregationSize);
  cmd.AddValue ("verbose", "Turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
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

  /**** DmgWifiHelper is a meta-helper ****/
  DmgWifiHelper wifi;

  /* Basic setup */
  wifi.SetStandard (WIFI_PHY_STANDARD_80211ad);

  /* Turn on logging */
  if (verbose)
    {
      wifi.EnableLogComponents ();
      LogComponentEnable ("BeamformingCBAP", LOG_LEVEL_ALL);
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
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue (phyMode));

  /* Make four nodes and set them up with the phy and the mac */
  NodeContainer wifiNodes;
  wifiNodes.Create (2);
  Ptr<Node> apNode = wifiNodes.Get (0);
  Ptr<Node> staNode = wifiNodes.Get (1);

  /* Add a DMG upper mac */
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

  /* Install DMG PCP/AP Node */
  Ssid ssid = Ssid ("BeamformingCBAP");
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

  NetDeviceContainer apDevice;
  apDevice = wifi.Install (wifiPhy, wifiMac, apNode);

  /* Install DMG STA Nodes */
  wifiMac.SetType ("ns3::DmgStaWifiMac",
                   "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false),
                   "BE_MaxAmpduSize", UintegerValue (mpduAggregationSize),
                   "BE_MaxAmsduSize", UintegerValue (msduAggregationSize));

  staDevices = wifi.Install (wifiPhy, wifiMac, staNode);

  /* Setting mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, +1.0, 0.0));   /* DMG PCP/AP */
  positionAlloc->Add (Vector (0.0, -1.0, 0.0));   /* DMG STA */

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
  Ipv4InterfaceContainer staInterfaces;
  staInterfaces = address.Assign (staDevices);

  /* Populate routing table */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* We do not want any ARP packets */
  PopulateArpCache ();

  if (activateApp)
    {
      /* Install Simple UDP Server on the DMG AP */
      PacketSinkHelper sinkHelper (socketType, InetSocketAddress (Ipv4Address::GetAny (), 9999));
      ApplicationContainer sinkApp = sinkHelper.Install (apNode);
      packetSink = StaticCast<PacketSink> (sinkApp.Get (0));
      sinkApp.Start (Seconds (0.0));

      /* Install TCP/UDP Transmitter on the DMG STA */
      Address dest (InetSocketAddress (apInterface.GetAddress (0), 9999));
      ApplicationContainer srcApp;
      if (applicationType == "onoff")
        {
          OnOffHelper src (socketType, dest);
          src.SetAttribute ("MaxBytes", UintegerValue (maxPackets));
          src.SetAttribute ("PacketSize", UintegerValue (packetSize));
          src.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
          src.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
          src.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
          srcApp = src.Install (staNode);
          onoff = StaticCast<OnOffApplication> (srcApp.Get (0));
        }
      else if (applicationType == "bulk")
        {
          BulkSendHelper src (socketType, dest);
          srcApp= src.Install (staNode);
          bulk = StaticCast<BulkSendApplication> (srcApp.Get (0));
        }
      srcApp.Start (Seconds (2.0));
      srcApp.Stop (Seconds (simulationTime));
    }

  /* Enable Traces */
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
      wifiPhy.EnablePcap ("Traces/AccessPoint", apDevice, false);
      wifiPhy.EnablePcap ("Traces/StaNode", staDevices.Get (0), false);
    }

  /* Stations */
  apWifiNetDevice = StaticCast<WifiNetDevice> (apDevice.Get (0));
  staWifiNetDevice = StaticCast<WifiNetDevice> (staDevices.Get (0));

  apWifiMac = StaticCast<DmgApWifiMac> (apWifiNetDevice->GetMac ());
  staWifiMac = StaticCast<DmgStaWifiMac> (staWifiNetDevice->GetMac ());

  /** Connect Traces **/
  staWifiMac->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, staWifiMac));
  apWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, apWifiMac));
  staWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, staWifiMac));

  apWifiMac->GetCodebook ()->TraceConnectWithoutContext ("ActiveTxSectorID", MakeBoundCallback (&ActiveTxSectorIDChanged, apWifiMac));
  staWifiMac->GetCodebook ()->TraceConnectWithoutContext ("ActiveTxSectorID", MakeBoundCallback (&ActiveTxSectorIDChanged, staWifiMac));

  FlowMonitorHelper flowmon;
  if (activateApp)
    {
      /* Install FlowMonitor on all nodes */
      monitor = flowmon.InstallAll ();

      /* Print Output*/
      std::cout << std::left << std::setw (12) << "Time [s]"
                << std::left << std::setw (12) << "Throughput [Mbps]" << std::endl;

      /* Schedule Throughput Calulcations */
      Simulator::Schedule (Seconds (2.1), &CalculateThroughput);
    }

  /* Schedule many TxSS CBAP during data transmission. */
  Simulator::Schedule (Seconds (2.1), &DmgWifiMac::InitiateTxssCbap, staWifiMac, apWifiMac->GetAddress ());
  Simulator::Schedule (Seconds (2.3), &DmgWifiMac::InitiateTxssCbap, staWifiMac, apWifiMac->GetAddress ());
  Simulator::Schedule (Seconds (2.5), &DmgWifiMac::InitiateTxssCbap, staWifiMac, apWifiMac->GetAddress ());
  Simulator::Schedule (Seconds (2.7), &DmgWifiMac::InitiateTxssCbap, staWifiMac, apWifiMac->GetAddress ());
  Simulator::Schedule (Seconds (2.9), &DmgWifiMac::InitiateTxssCbap, staWifiMac, apWifiMac->GetAddress ());
  Simulator::Schedule (Seconds (3.1), &DmgWifiMac::InitiateTxssCbap, staWifiMac, apWifiMac->GetAddress ());
  Simulator::Schedule (Seconds (3.6), &DmgWifiMac::InitiateTxssCbap, staWifiMac, apWifiMac->GetAddress ());
  Simulator::Schedule (Seconds (4.2), &DmgWifiMac::InitiateTxssCbap, staWifiMac, apWifiMac->GetAddress ());
  Simulator::Schedule (Seconds (4.7), &DmgWifiMac::InitiateTxssCbap, staWifiMac, apWifiMac->GetAddress ());
  Simulator::Schedule (Seconds (4.8), &DmgWifiMac::InitiateTxssCbap, staWifiMac, apWifiMac->GetAddress ());
  Simulator::Schedule (Seconds (5.0), &DmgWifiMac::InitiateTxssCbap, staWifiMac, apWifiMac->GetAddress ());
  Simulator::Schedule (Seconds (5.0), &DmgWifiMac::InitiateTxssCbap, staWifiMac, apWifiMac->GetAddress ());
  Simulator::Schedule (Seconds (5.2), &DmgWifiMac::InitiateTxssCbap, staWifiMac, apWifiMac->GetAddress ());
  Simulator::Schedule (Seconds (5.5), &DmgWifiMac::InitiateTxssCbap, staWifiMac, apWifiMac->GetAddress ());
  Simulator::Schedule (Seconds (5.7), &DmgWifiMac::InitiateTxssCbap, staWifiMac, apWifiMac->GetAddress ());
  Simulator::Schedule (Seconds (6.0), &DmgWifiMac::InitiateTxssCbap, staWifiMac, apWifiMac->GetAddress ());
  Simulator::Schedule (Seconds (6.32), &DmgWifiMac::InitiateTxssCbap, staWifiMac, apWifiMac->GetAddress ());
  Simulator::Schedule (Seconds (6.567), &DmgWifiMac::InitiateTxssCbap, staWifiMac, apWifiMac->GetAddress ());
  Simulator::Schedule (Seconds (7.123), &DmgWifiMac::InitiateTxssCbap, staWifiMac, apWifiMac->GetAddress ());
  Simulator::Schedule (Seconds (8.0), &DmgWifiMac::InitiateTxssCbap, staWifiMac, apWifiMac->GetAddress ());
  Simulator::Schedule (Seconds (8.1), &DmgWifiMac::InitiateTxssCbap, staWifiMac, apWifiMac->GetAddress ());
  Simulator::Schedule (Seconds (8.5), &DmgWifiMac::InitiateTxssCbap, staWifiMac, apWifiMac->GetAddress ());
  Simulator::Schedule (Seconds (8.5), &DmgWifiMac::InitiateTxssCbap, staWifiMac, apWifiMac->GetAddress ());

  Simulator::Stop (Seconds (simulationTime + 0.101));
  Simulator::Run ();
  Simulator::Destroy ();

  if (activateApp)
    {
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
    }

  std::cout << "  Rx Packets: " << packetSink->GetTotalReceivedPackets () << std::endl;
  std::cout << "  Rx Bytes:   " << packetSink->GetTotalRx () << std::endl;
  std::cout << "  Throughput: " << packetSink->GetTotalRx () * 8.0 / ((simulationTime - 1) * 1e6) << " Mbps" << std::endl;

  return 0;
}
