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
 * This script is used to evaluate IEEE 802.11ad Fast Session Transfer Mechanism with the presence of blockage.
 * This is the script used to generate the results in the paper.
 *
 * Network Topology:
 * The scenario consists of signle DMG STA and single PCP/AP.
 *
 *          DMG PCP/AP (0,0)                       DMG STA (+1,0)
 *
 * Simulation Description:
 * Network topology is simple and consists of
 * One Access Point + One Station operating initially in the 60GHz band. We introduce link interruption which causes
 * The nodes to swtich to the 2.4GHz band.
 *
 * Running Simulation:
 * To use this script simply type the following run command:
 * ./waf --run "evaluate_fst_mechanism --llt=10000 --dataRate=5Gbps"
 *
 * To generate PCAP files, type the following run command:
 * ./waf --run "evaluate_fst_mechanism --llt=10000 --dataRate=5Gbps --pcap=1"
 *
 * Simulation Output:
 * The simulation generates the following traces:
 * 1. PCAP traces for each station. The simulation generates two PCAP files for each node.
 * One PCAP file corresponds to 11ad Band and the other PCAP for 11n band.
 * In the 11ad PCAP files, you can check the setup of FSTS. In the 11n PCAP file you can
 * see the exchange of FST ACK Request/Response frames.
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateFstMechanism");

using namespace ns3;
using namespace std;

Ptr<MultiBandNetDevice> staMultibandDevice;
Ptr<MultiBandNetDevice> apMultibandDevice;

double m_blockageValue = -45;             /* in dBm */

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
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

/**
 * Insert Blockage
 * \return The actual value of the blockage we introduce in the simulator.
 */
double
DoInsertBlockage ()
{
  return m_blockageValue;
}

/**
 * Insert Blockage on a certain path.
 * \param channel The channel to insert the object with
 * \param peerWifiPhy The Peer WifiPhy to establish this blockage with.
 */
void
InsertBlockage (Ptr<YansWifiChannel> channel, Ptr<WifiPhy> srcWifiPhy, Ptr<WifiPhy> dstWifiPhy)
{
  std::cout << "Blockage Inserted at " << Simulator::Now () << std::endl;
  channel->AddBlockage (&DoInsertBlockage, srcWifiPhy, dstWifiPhy);
}

int
main(int argc, char *argv[])
{
  uint32_t payloadSize = 1472;                  /* Transport Layer Payload size in bytes. */
  string dataRate = "100Mbps";                  /* Application Layer Data Rate. */
  uint32_t queueSize = 1000;                    /* Wifi Mac Queue Size. */
  string adPhyMode = "DMG_MCS24";               /* Type of the 802.11ad Physical Layer. */
  string nPhyMode = "HtMcs7";                   /* Type of the 802.11n Physical Layer. */
  uint32_t llt = 100;                           /* Link Loss Timeout. */
  double simulationTime = 10;                   /* Simulation time in seconds. */
  bool pcapTracing = false;                     /* PCAP Tracing is enabled or not. */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("dataRate", "Payload size in bytes", dataRate);
  cmd.AddValue ("queueSize", "The size of the Wifi Mac Queue", queueSize);
  cmd.AddValue ("blockageValue", "The amount of attenuation in [dBm] the blockage adds", m_blockageValue);
  cmd.AddValue ("llt", "LLT", llt);
  cmd.AddValue ("adPhyMode", "802.11ad PHY Mode", adPhyMode);
  cmd.AddValue ("nPhyMode", "802.11n PHY Mode", nPhyMode);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable PCAP Tracing", pcapTracing);
  cmd.Parse (argc, argv);

  /* Global params: no fragmentation, no RTS/CTS, fixed rate for all packets */
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));
//  Config::SetDefault ("ns3::WifiMacQueue::MaxPacketNumber", UintegerValue (queueSize));

  /**** Allocate 802.11ad Wifi MAC ****/
  /* Add a DMG upper mac */
  DmgWifiMacHelper adWifiMac = DmgWifiMacHelper::Default ();

  /**** Set up 60 Ghz Channel ****/
  YansWifiChannelHelper adWifiChannel ;
  /* Simple propagation delay model */
  adWifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  /* Friis model with standard-specific wavelength */
  adWifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (60.48e9));

  /**** Setup Physical Layer ****/
  YansWifiPhyHelper adWifiPhy = YansWifiPhyHelper::Default ();
  adWifiPhy.SetChannel (adWifiChannel.Create ());
  adWifiPhy.Set ("TxPowerStart", DoubleValue (10.0));
  adWifiPhy.Set ("TxPowerEnd", DoubleValue (10.0));
  adWifiPhy.Set ("TxPowerLevels", UintegerValue (1));
  adWifiPhy.Set ("TxGain", DoubleValue (0));
  adWifiPhy.Set ("RxGain", DoubleValue (0));
  adWifiPhy.Set("RxNoiseFigure", DoubleValue (10));
  adWifiPhy.Set ("CcaMode1Threshold", DoubleValue (-79));
  adWifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-79 + 3));
  adWifiPhy.SetErrorRateModel ("ns3::SensitivityModel60GHz");
  ObjectFactory adremoteStationManager = ObjectFactory ();
  adremoteStationManager.SetTypeId ("ns3::ConstantRateWifiManager");
  adremoteStationManager.Set ("ControlMode", StringValue (adPhyMode));
  adremoteStationManager.Set ("DataMode", StringValue (adPhyMode));

  adWifiPhy.EnableAntenna (true, true);
  adWifiPhy.SetAntenna ("ns3::Directional60GhzAntenna",
                        "Sectors", UintegerValue (4),
                        "Antennas", UintegerValue (1));

  /* 802.11ad Structure */
  WifiTechnologyHelperStruct adWifiStruct;
  adWifiStruct.MacHelper = &adWifiMac;
  adWifiStruct.PhyHelper = &adWifiPhy;
  adWifiStruct.RemoteStationManagerFactory = adremoteStationManager;
  adWifiStruct.Standard = WIFI_PHY_STANDARD_80211ad;
  adWifiStruct.Operational = true;

  /**** Allocate 802.11ad Wifi MAC ****/
  /* Add a DMG upper mac */
  HtWifiMacHelper nWifiMac = HtWifiMacHelper::Default ();

  /**** Set up Legacy Channel ****/
  YansWifiChannelHelper nWifiChannel ;
  nWifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  nWifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (2.4e9));

  /**** Setup Physical Layer ****/
  YansWifiPhyHelper nWifiPhy = YansWifiPhyHelper::Default ();
  nWifiPhy.SetChannel (nWifiChannel.Create ());
  nWifiPhy.Set ("TxPowerStart", DoubleValue (10.0));
  nWifiPhy.Set ("TxPowerEnd", DoubleValue (10.0));
  nWifiPhy.Set ("TxPowerLevels", UintegerValue (1));
  nWifiPhy.Set ("TxGain", DoubleValue (0));
  nWifiPhy.Set ("RxGain", DoubleValue (0));
  nWifiPhy.Set ("RxNoiseFigure", DoubleValue (10));
  nWifiPhy.Set ("CcaMode1Threshold", DoubleValue (-79));
  nWifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-79 + 3));
  nWifiPhy.SetErrorRateModel ("ns3::YansErrorRateModel");
  nWifiPhy.EnableAntenna (false, false);
  ObjectFactory nRemoteStationManager = ObjectFactory ();
  nRemoteStationManager.SetTypeId ("ns3::ConstantRateWifiManager");
  nRemoteStationManager.Set ("ControlMode", StringValue (nPhyMode));
  nRemoteStationManager.Set ("DataMode", StringValue (nPhyMode));

  /* 802.11n Structure */
  WifiTechnologyHelperStruct legacyWifiStruct;
  legacyWifiStruct.MacHelper = &nWifiMac;
  legacyWifiStruct.PhyHelper = &nWifiPhy;
  legacyWifiStruct.RemoteStationManagerFactory = nRemoteStationManager;
  legacyWifiStruct.Standard = WIFI_PHY_STANDARD_80211n_5GHZ;
  legacyWifiStruct.Operational = false;

  /* Make two nodes and set them up with the phy and the mac */
  NodeContainer wifiNodes;
  wifiNodes.Create (2);
  Ptr<Node> apWifiNode = wifiNodes.Get (0);
  Ptr<Node> staWifiNode = wifiNodes.Get (1);

  /* Technologies List*/
  WifiTechnologyHelperList technologyList;
  technologyList.push_back (adWifiStruct);
  technologyList.push_back (legacyWifiStruct);

  MultiBandWifiHelper multibandHelper;
  /* Configure AP with different wifi technologies */
  Ssid ssid = Ssid ("FST");
  adWifiMac.SetType ("ns3::DmgApWifiMac",
                     "Ssid", SsidValue (ssid),
                     "BE_MaxAmpduSize", UintegerValue (262143), //Enable A-MPDU with the highest maximum size allowed by the standard
                     "BE_MaxAmsduSize", UintegerValue (7935),
                     "SSSlotsPerABFT", UintegerValue (8), "SSFramesPerSlot", UintegerValue (8),
                     "BeaconInterval", TimeValue (MicroSeconds (102400)),
                     "BeaconTransmissionInterval", TimeValue (MicroSeconds (400)),
                     "ATIPresent", BooleanValue (false),
                     "SupportMultiBand", BooleanValue (true));

  nWifiMac.SetType ("ns3::ApWifiMac",
                    "Ssid", SsidValue (ssid),
                    "BE_MaxAmpduSize", UintegerValue (65535), //Enable A-MPDU with the highest maximum size allowed by the standard
                    "BE_MaxAmsduSize", UintegerValue (7935),
                    "QosSupported", BooleanValue (true), "HtSupported", BooleanValue (true),
                    "SupportMultiBand", BooleanValue (true));

  NetDeviceContainer apDevice;
  apDevice = multibandHelper.Install (technologyList, apWifiNode);

  /* Configure STA with different wifi technologies */
  adWifiMac.SetType ("ns3::DmgStaWifiMac",
                     "Ssid", SsidValue (ssid),
                     "ActiveProbing", BooleanValue (false),
                     "BE_MaxAmpduSize", UintegerValue (262143), //Enable A-MPDU with the highest maximum size allowed by the standard
                     "BE_MaxAmsduSize", UintegerValue (7935),
                     "DmgSupported", BooleanValue (true),
                     "LLT", UintegerValue (llt),
                     "SupportMultiBand", BooleanValue (true));

  nWifiMac.SetType ("ns3::StaWifiMac",
                    "Ssid", SsidValue (ssid),
                    "ActiveProbing", BooleanValue (false),
                    "BE_MaxAmpduSize", UintegerValue (65535), //Enable A-MPDU with the highest maximum size allowed by the standard
                    "BE_MaxAmsduSize", UintegerValue (7935),
                    "QosSupported", BooleanValue (true), "HtSupported", BooleanValue (true),
                    "SupportMultiBand", BooleanValue (true));

  NetDeviceContainer staDevices;
  staDevices = multibandHelper.Install (technologyList, staWifiNode);

  /* Setting mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (1.0, 1.0, 0.0));

  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (apWifiNode);
  mobility.Install (staWifiNode);

  /* Internet stack*/
  InternetStackHelper stack;
  stack.Install (apWifiNode);
  stack.Install (staWifiNode);

  Ipv4AddressHelper address;
  address.SetBase ("192.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer apInterface;
  apInterface = address.Assign (apDevice);
  Ipv4InterfaceContainer staInterface;
  staInterface = address.Assign (staDevices);

  /* Populate routing table */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* We do not want any ARP packets */
  PopulateArpCache ();

  /* Install Simple UDP Application on the access point */
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9999));
  ApplicationContainer sinkApp = sinkHelper.Install (apWifiNode);
  sink = StaticCast<PacketSink> (sinkApp.Get (0));
  sinkApp.Start (Seconds (0.0));

  ApplicationContainer srcApp;
  OnOffHelper src ("ns3::UdpSocketFactory", InetSocketAddress (apInterface.GetAddress (0), 9999));
  src.SetAttribute ("MaxBytes", UintegerValue (0));
  src.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  src.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
  src.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  src.SetAttribute ("DataRate", DataRateValue (dataRate));
  srcApp = src.Install (staWifiNode);
  srcApp.Start (Seconds (1.0));
  Simulator::Schedule (Seconds (1.1), &CalculateThroughput);

  /* Enable Traces */
  if (pcapTracing)
    {
      Ptr<MultiBandNetDevice> apMultiBandDevice = StaticCast<MultiBandNetDevice> (apDevice.Get (0));
      Ptr<MultiBandNetDevice> staMultiBandDevice = StaticCast<MultiBandNetDevice> (staDevices.Get (0));
      WifiTechnologyList apTechnologyList = apMultiBandDevice->GetWifiTechnologyList ();
      WifiTechnologyList staTechnologyList = staMultiBandDevice->GetWifiTechnologyList ();

      adWifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
      nWifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

      /* AP Technologies PCAP */
      for (WifiTechnologyList::iterator item = apTechnologyList.begin (); item != apTechnologyList.end (); item++)
        {
          if (item->first == WIFI_PHY_STANDARD_80211ad)
            {
              adWifiPhy.EnableMultiBandPcap ("Traces/adAccessPoint", apMultiBandDevice, item->second.Phy);
            }
          else if (item->first == WIFI_PHY_STANDARD_80211n_5GHZ)
            {
              nWifiPhy.EnableMultiBandPcap ("Traces/nAccessPoint", apMultiBandDevice, item->second.Phy);
            }
        }
      /* STA Technologies PCAP */
      for (WifiTechnologyList::iterator item = staTechnologyList.begin (); item != staTechnologyList.end (); item++)
        {
          if (item->first == WIFI_PHY_STANDARD_80211ad)
            {
              adWifiPhy.EnableMultiBandPcap ("Traces/adStation", staMultiBandDevice, item->second.Phy);
            }
          else if (item->first == WIFI_PHY_STANDARD_80211n_5GHZ)
            {
              nWifiPhy.EnableMultiBandPcap ("Traces/nStation", staMultiBandDevice, item->second.Phy);
            }
        }
    }

  /* Variables */
  staMultibandDevice = StaticCast<MultiBandNetDevice> (staDevices.Get (0));
  apMultibandDevice = StaticCast<MultiBandNetDevice> (apDevice.Get (0));
  Ptr<YansWifiChannel> adChannel = StaticCast<YansWifiChannel> (staMultibandDevice->GetChannel ());
  Ptr<WifiPhy> srcWifiPhy = apMultibandDevice->GetPhy ();
  Ptr<WifiPhy> dstWifiPhy = staMultibandDevice->GetPhy ();

  /* Schedule for FST Session Creation, STA is the Initiator */
  Simulator::Schedule (Seconds (2), &MultiBandNetDevice::EstablishFastSessionTransferSession, staMultibandDevice,
                       Mac48Address::ConvertFrom (apMultibandDevice->GetAddress ()));

  /* Schedule for link Interruption */
  Simulator::Schedule (Seconds (3), &InsertBlockage, adChannel, srcWifiPhy, dstWifiPhy);

  /* Start Simulation */
  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
