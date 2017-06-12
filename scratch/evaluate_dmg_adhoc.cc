/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "common-functions.h"

#include <string>
/**
 * Simulation Objective:
 * This script is used to demonstrage the usage of the DMG AD-HOC Class for data communication.
 * The DMG AD-HOC is an experimental class which simplifies the implmenetation of the Beacon Interval.
 * In precise, it does not include BHI access period as a result only data communication takes place.
 *
 * Network Topology:
 * The scenario consists of two DMG AD-HOC Terminals.
 *
 *      Backbone Server <-----------> DMG AD-HOC (0,0)               DMG AD-HOC (+1,0)
 *
 * Simulation Description:
 *
 * Running Simulation:
 * To evaluate CSMA/CA channel access scheme:
 * ./waf --run "evaluate_dmg_adhoc --scheme=1 --simulationTime=10 --pcap=true"
 *
 * Simulation Output:
 * The simulation generates the following traces:
 * 1. PCAP traces for each station.
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateDmgAdhoc");

using namespace ns3;
using namespace std;

Ptr<Node> serverNode;
Ptr<Node> apWifiNode;
Ptr<Node> staWifiNode;

/*** Access Point Variables ***/
Ptr<PacketSink> sink;
uint64_t lastTotalRx = 0;
double averagethroughput = 0;

void
CalculateThroughput ()
{
  Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
  double cur = (sink->GetTotalRx() - lastTotalRx) * (double) 8/1e5;     /* Convert Application RX Packets to MBits. */

  std::cout << now.GetSeconds () << '\t' << cur << std::endl;
  lastTotalRx = sink->GetTotalRx ();
  averagethroughput += cur;

  Simulator::Schedule (Seconds (0.1), &CalculateThroughput);
}

int
main(int argc, char *argv[])
{
  string applicationType = "bulk";                /* Type of the Tx application */
  uint32_t payloadSize = 1448;                    /* Transport Layer Payload size in bytes. */
  string socketType = "ns3::TcpSocketFactory";    /* Socket Type (TCP/UDP) */
  uint32_t maxPackets = 0;                        /* Maximum Number of Packets */
  string tcpVariant = "NewReno";                  /* TCP Variant Type. */
  uint32_t bufferSize = 131072;                   /* TCP Send/Receive Buffer Size. */
  uint32_t mcsIndex = 24;                         /* The index of the MCS */
  uint32_t queueSize = 10000;                     /* Wifi Mac Queue Size. */
  string phyMode = "DMG_MCS";                     /* Type of the Physical Layer. */
  double distance = 1.0;                          /* The distance between transmitter and receiver in meters. */
  bool verbose = false;                           /* Print Logging Information. */
  double simulationTime = 10;                     /* Simulation time in seconds. */
  bool pcapTracing = false;                       /* PCAP Tracing is enabled or not. */
  std::vector<std::string> dataRateList;          /* List of the maximum data rate supported by the standard*/
  std::map<std::string, std::string> tcpVariants; /* List of the tcp Variants */

  /** MCS List **/
  /* CTRL PHY */
  dataRateList.push_back ("27.5Mbps");          //MCS0

  /* SC PHY */
  dataRateList.push_back ("385Mbps");           //MCS1
  dataRateList.push_back ("770Mbps");           //MCS2
  dataRateList.push_back ("962.5Mbps");         //MCS3
  dataRateList.push_back ("1155Mbps");          //MCS4
  dataRateList.push_back ("1251.25Mbps");       //MCS5
  dataRateList.push_back ("1540Mbps");          //MCS6
  dataRateList.push_back ("1925Mbps");          //MCS7
  dataRateList.push_back ("2310Mbps");          //MCS8
  dataRateList.push_back ("2502.5Mbps");        //MCS9
  dataRateList.push_back ("3080Mbps");          //MCS10
  dataRateList.push_back ("3850Mbps");          //MCS11
  dataRateList.push_back ("4620Mbps");          //MCS12
  /* OFDM PHY */
  dataRateList.push_back ("693.00Mbps");        //MCS13
  dataRateList.push_back ("866.25Mbps");        //MCS14
  dataRateList.push_back ("1386.00Mbps");       //MCS15
  dataRateList.push_back ("1732.50Mbps");       //MCS16
  dataRateList.push_back ("2079.00Mbps");       //MCS17
  dataRateList.push_back ("2772.00Mbps");       //MCS18
  dataRateList.push_back ("3465.00Mbps");       //MCS19
  dataRateList.push_back ("4158.00Mbps");       //MCS20
  dataRateList.push_back ("4504.50Mbps");       //MCS21
  dataRateList.push_back ("5197.50Mbps");       //MCS22
  dataRateList.push_back ("6237.00Mbps");       //MCS23
  dataRateList.push_back ("6756.75Mbps");       //MCS24

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
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("socketType", "Type of the Socket (ns3::TcpSocketFactory, ns3::UdpSocketFactory)", socketType);
  cmd.AddValue ("maxPackets", "Maximum number of packets to send", maxPackets);
  cmd.AddValue ("tcpVariant", "Transport protocol to use: TcpTahoe, TcpReno, TcpNewReno, TcpWestwood, TcpWestwoodPlus", tcpVariant);
  cmd.AddValue ("bufferSize", "TCP Buffer Size (Send/Receive)", bufferSize);
  cmd.AddValue ("queueSize", "The size of the Wifi Mac Queue", queueSize);
  cmd.AddValue ("dist", "distance between nodes", distance);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable PCAP Tracing", pcapTracing);
  cmd.Parse (argc, argv);

  /* Global params: no fragmentation, no RTS/CTS, fixed rate for all packets */
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));

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
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (bufferSize));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (bufferSize));

  /**** WifiHelper is a meta-helper: it helps creates helpers ****/
  WifiHelper wifi;

  /* Basic setup */
  wifi.SetStandard (WIFI_PHY_STANDARD_80211ad);

  /* Turn on logging */
  if (verbose)
    {
      wifi.EnableLogComponents ();
      LogComponentEnable ("EvaluateDmgAdhoc", LOG_LEVEL_ALL);
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
  ostringstream mcs;
  mcs << mcsIndex;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "ControlMode", StringValue ("DMG_MCS0"),
                                                                "DataMode", StringValue (phyMode + mcs.str ()));
  /* Give all nodes steerable antenna */
  wifiPhy.EnableAntenna (true, true);
  wifiPhy.SetAntenna ("ns3::Directional60GhzAntenna",
                      "Sectors", UintegerValue (8),
                      "Antennas", UintegerValue (1));

  /* Make three nodes and set them up with the phy and the mac */
  NodeContainer wifiNodes;
  wifiNodes.Create (3);
  serverNode = wifiNodes.Get (0);
  apWifiNode = wifiNodes.Get (1);
  staWifiNode = wifiNodes.Get (2);

  /* Create Backbone network */
  PointToPointHelper p2pHelper;
  p2pHelper.SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
  p2pHelper.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (20)));
  p2pHelper.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue (1000));

  NetDeviceContainer serverDevices;
  serverDevices = p2pHelper.Install (serverNode, apWifiNode);

  /* Add a DMG ADHOC MAC */
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

  wifiMac.SetType ("ns3::DmgAdhocWifiMac",
                   "BE_MaxAmpduSize", UintegerValue (0), //Enable A-MPDU with the highest maximum size allowed by the standard
                   "BE_MaxAmsduSize", UintegerValue (7935));

  NetDeviceContainer apDevice;
  apDevice = wifi.Install (wifiPhy, wifiMac, apWifiNode);

  NetDeviceContainer staDevice;
  staDevice = wifi.Install (wifiPhy, wifiMac, staWifiNode);

  /* Setting mobility model, Initial Position 1 meter apart */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (distance, 0.0, 0.0));

  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiNodes);

  /* Internet stack*/
  InternetStackHelper stack;
  stack.Install (wifiNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer serverInterface;
  serverInterface = address.Assign (serverDevices);
  address.NewNetwork ();
  Ipv4InterfaceContainer apInterface;
  apInterface = address.Assign (apDevice);
  Ipv4InterfaceContainer staInterface;
  staInterface = address.Assign (staDevice);

  /* Populate routing table */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* We do not want any ARP packets */
  PopulateArpCache ();

  /* Install Simple UDP Server on the access point */
  PacketSinkHelper sinkHelper (socketType, InetSocketAddress (Ipv4Address::GetAny (), 9999));
  ApplicationContainer sinkApp = sinkHelper.Install (serverNode);
  sink = StaticCast<PacketSink> (sinkApp.Get (0));
  sinkApp.Start (Seconds (0.0));

  /* Install TCP/UDP Transmitter on the station */
  Address dest (InetSocketAddress (serverInterface.GetAddress (0), 9999));
  ApplicationContainer srcApp;
  if (applicationType == "onoff")
    {
      OnOffHelper src (socketType, dest);
      src.SetAttribute ("MaxBytes", UintegerValue (0));
      src.SetAttribute ("PacketSize", UintegerValue (payloadSize));
      src.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
      src.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
      src.SetAttribute ("DataRate", DataRateValue (DataRate ("10Mbps")));

      srcApp = src.Install (staWifiNode);
    }
  else if (applicationType == "bulk")
    {
      BulkSendHelper src (socketType, dest);
      srcApp= src.Install (staWifiNode);
    }

  srcApp.Start (Seconds (0.0));

  /* Set Maximum number of packets in WifiMacQueue */
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/DcaTxop/Queue/MaxPackets", UintegerValue (queueSize));
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxPackets", UintegerValue (queueSize));

  if (pcapTracing)
    {
      p2pHelper.EnablePcap ("Traces/Server", serverDevices.Get (0));
      wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
      wifiPhy.EnablePcap ("Traces/AccessPoint" + mcs.str (), apDevice, false);
      wifiPhy.EnablePcap ("Traces/Station" + mcs.str (), staDevice, false);
    }

  Simulator::Schedule (Seconds (0.1), &CalculateThroughput);
  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
