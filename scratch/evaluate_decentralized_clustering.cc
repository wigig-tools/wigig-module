/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

/**
 * This script is used to evaluate IEEE 802.11ad decentranlized clustering formation and maintenance
 *
 *                           DMG AP_1 (0,1)
 *
 *
 * DMG AP_3 (-1.73,0)                               DMG AP_4 (1.73,0)
 *
 *
 *                           DMG AP_2 (0,-1)
 *
 * To use this script simply type the following run command:
 * ./waf --run "evaluate_decentrazlied_clustering"
 *
 * The simulation generates four PCAP files for each node. You can check the traces which matches exactly
 * the procedure for relay establishment.
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateDecentralizedClustering");

using namespace ns3;
using namespace std;

void JoinedCluster (Ptr<DmgApWifiMac> apWifiMac, Mac48Address address, uint8_t beaconSPIndex)
{
  std::cout << "PCP/AP " << apWifiMac->GetAddress () << " joined ClusterID=" << address
            << " in BeaconSP=" << beaconSPIndex << std::endl;
}

Ptr<WifiNetDevice>
CreateAccessPoint (Ptr<Node> apNode, WifiHelper wifi, YansWifiPhyHelper wifiPhy, Time channelMonitorDuration)
{
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();
  Ssid ssid = Ssid ("Cluster");

  wifiMac.SetType ("ns3::DmgApWifiMac",
                  "Ssid", SsidValue (ssid),
                  "BE_MaxAmpduSize", UintegerValue (0),
                  "SSSlotsPerABFT", UintegerValue (8), "SSFramesPerSlot", UintegerValue (16),
                  "AllowBeaconing", BooleanValue (false),
                  "BeaconTransmissionInterval", TimeValue (MicroSeconds (800)),
                  "ATIDuration", TimeValue (MicroSeconds (1000)),
                  "EnableDecentralizedClustering", BooleanValue (true),
                  "ClusterRole", EnumValue (NOT_PARTICIPATING),
                  "ChannelMonitorDuration", TimeValue (channelMonitorDuration));

  NetDeviceContainer apDevice = wifi.Install (wifiPhy, wifiMac, apNode);

  return (StaticCast<WifiNetDevice> (apDevice.Get (0)));
}

int
main (int argc, char *argv[])
{
  string phyMode = "DMG_MCS24";                 /* Type of the Physical Layer. */
  bool verbose = false;                         /* Print Logging Information. */
  double simulationTime = 10;                   /* Simulation time in seconds. */
  bool pcapTracing = false;                     /* PCAP Tracing is enabled or not. */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("phyMode", "802.11ad PHY Mode", phyMode);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
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
      LogComponentEnable ("EvaluateDecentralizedClustering", LOG_LEVEL_ALL);
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
  wifiPhy.Set ("RxNoiseFigure", DoubleValue (10));
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
  NodeContainer syncApWwifiNode;
  syncApWwifiNode.Create (1);

  NodeContainer apWifiNodes;
  apWifiNodes.Create (3);

  /* Add a DMG upper mac */
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

  /* Install SYNC AP Node */
  Ssid ssid = Ssid ("Cluster");
  wifiMac.SetType ("ns3::DmgApWifiMac",
                   "Ssid", SsidValue (ssid),
                   "BE_MaxAmpduSize", UintegerValue (0),
                   "SSSlotsPerABFT", UintegerValue (8), "SSFramesPerSlot", UintegerValue (16),
                   "BeaconInterval", TimeValue (MicroSeconds (102400)),
                   "BeaconTransmissionInterval", TimeValue (MicroSeconds (800)),
                   "ATIDuration", TimeValue (MicroSeconds (1000)),
                   "EnableDecentralizedClustering", BooleanValue (true),
                   "ClusterMaxMem", UintegerValue (4),
                   "BeaconSPDuration", UintegerValue (100),
                   "ClusterRole", EnumValue (SYNC_PCP_AP));

  NetDeviceContainer syncApDevice;
  syncApDevice = wifi.Install (wifiPhy, wifiMac, syncApWwifiNode);

  /* Install PCP/AP Nodes */
  NetDeviceContainer apDevices;
  apDevices.Add (CreateAccessPoint (apWifiNodes.Get (0), wifi, wifiPhy, 1 * aMinChannelTime));
  apDevices.Add (CreateAccessPoint (apWifiNodes.Get (1), wifi, wifiPhy, 2 * aMinChannelTime));
  apDevices.Add (CreateAccessPoint (apWifiNodes.Get (2), wifi, wifiPhy, 3 * aMinChannelTime));

  /* Setting mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, +1.0, 0.0));     /* PCP/AP (1) (S-PCP/S-AP) */
  positionAlloc->Add (Vector (0.0, -1.0, 0.0));     /* PCP/AP (2) */
  positionAlloc->Add (Vector (-1.732, 0.0, 0.0));   /* PCP/AP (3) */
  positionAlloc->Add (Vector (+1.732, 0.0, 0.0));   /* PCP/AP (4) */

  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (syncApWwifiNode);
  mobility.Install (apWifiNodes);

  /* Internet stack*/
  InternetStackHelper stack;
  stack.Install (syncApWwifiNode);
  stack.Install (apWifiNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  address.Assign (syncApDevice);
  address.Assign (apDevices);

  /* Populate routing table */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* Connect Traces */
  Ptr<WifiNetDevice> apWifiNetDevice;
  Ptr<DmgApWifiMac> apWifiMac;
  for (uint32_t i = 0; i < apDevices.GetN (); i++)
    {
      apWifiNetDevice = StaticCast<WifiNetDevice> (apDevices.Get (i));
      apWifiMac = StaticCast<DmgApWifiMac> (apWifiNetDevice->GetMac ());
      apWifiMac->TraceConnectWithoutContext ("JoinedCluster", MakeBoundCallback (&JoinedCluster, apWifiMac));
    }

  /* Enable Traces */
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
      wifiPhy.EnablePcap ("Traces/AccessPoint", NetDeviceContainer (syncApDevice, apDevices), false);
    }

  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
