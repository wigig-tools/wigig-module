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
 *
 * Simulation Objective:
 * This script is used to evaluate various options to control IEEE 802.11ad Beacon Interval.
 *
 * Network Topology:
 * Network topology is simple and consists of One Access Point + One Station. Each station has one antenna array with
 * eight virutal sectors to cover 360 in the 2D Domain.
 *
 *          DMG PCP/AP (0,0)                       DMG STA (+1,0)
 *
 * Running Simulation:
 * To evaluate CSMA/CA channel access scheme:
 * ./waf --run "evaluate_beacon_interval --beaconInterval=102400 --nextBeacon=1 --beaconRandomization=true
 *  --btiDuration=400 --nextAbft=0 --atiPresent=false --simulationTime=10"
 *
 * Simulation Output:
 * The simulation generates the following traces:
 * 1. PCAP traces for each station.
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateBeaconInterval");

using namespace ns3;
using namespace std;

Ptr<Node> apWifiNode;
Ptr<Node> staWifiNode;

Ptr<DmgApWifiMac> apWifiMac;
Ptr<DmgStaWifiMac> staWifiMac;

int
main(int argc, char *argv[])
{
  uint32_t beaconInterval = 102400;       /* The interval between two Target Beacon Transmission Times (TBTTs). */
  bool beaconRandomization = false;       /* Whether to change the sequence of DMG Beacons at each BI. */
  uint32_t nextBeacon = 0;                /* The number of BIs following the current BI during which the DMG Beacon is not be present. */
  uint32_t btiDuration = 400;             /* The duration of the BTI period. */
  uint32_t nextAbft = 0;                  /* The number of beacon intervals during which the A-BFT is not be present. */
  uint32_t slotsPerABFT = 8;              /* The number of Sector Sweep Slots Per A-BFT. */
  uint32_t sswPerSlot = 8;                /* The number of SSW Frames per Sector Sweep Slot. */
  bool atiPresent = true;                 /* Whether the BI period contains ATI access period. */
  uint16_t atiDuration = 300;             /* The duration of the ATI access period. */
  bool verbose = false;                   /* Print Logging Information. */
  double simulationTime = 10;             /* Simulation time in seconds. */
  bool pcapTracing = true;                /* PCAP Tracing is enabled or not. */

  /* Command line argument parser setup. */
  CommandLine cmd;

  cmd.AddValue ("beaconInterval", "The interval between two Target Beacon Transmission Times (TBTTs)", beaconInterval);
  cmd.AddValue ("beaconRandomization", "Whether to change the sequence of DMG Beacons at each BI", beaconRandomization);
  cmd.AddValue ("nextBeacon", "The number of beacon intervals following the current beacon interval during"
                "which the DMG Beacon is not be present", nextBeacon);
  cmd.AddValue ("btiDuration", "The duration of the BTI period", btiDuration);
  cmd.AddValue ("nextAbft", "The number of beacon intervals during which the A-BFT is not be present", nextAbft);
  cmd.AddValue ("slotsPerABFT", "The number of Sector Sweep Slots Per A-BFT", slotsPerABFT);
  cmd.AddValue ("sswPerSlot", "The number of SSW Frames per Sector Sweep Slot", sswPerSlot);
  cmd.AddValue ("atiPresent", "Flag to indicate if the BI period contains ATI access period", atiPresent);
  cmd.AddValue ("atiDuration", "The duration of the ATI access period", atiDuration);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable PCAP Tracing", pcapTracing);
  cmd.Parse (argc, argv);

  /**** WifiHelper is a meta-helper: it helps creates helpers ****/
  WifiHelper wifi;

  /* Basic setup */
  wifi.SetStandard (WIFI_PHY_STANDARD_80211ad);

  /* Turn on logging */
  if (verbose)
    {
      wifi.EnableLogComponents ();
      LogComponentEnable ("EvaluateBeaconInterval", LOG_LEVEL_ALL);
    }

  /**** Set up Channel ****/
  YansWifiChannelHelper wifiChannel ;
  /* Simple propagation delay model */
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  /* Friis model with standard-specific wavelength */
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (60.48e9));

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
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "ControlMode", StringValue ("DMG_MCS24"),
                                                                "DataMode", StringValue ("DMG_MCS24"));
  /* Give all nodes steerable antenna */
  wifiPhy.EnableAntenna (true, true);
  wifiPhy.SetAntenna ("ns3::Directional60GhzAntenna",
                      "Sectors", UintegerValue (8),
                      "Antennas", UintegerValue (1));

  /* Make two nodes and set them up with the phy and the mac */
  NodeContainer wifiNodes;
  wifiNodes.Create (2);
  apWifiNode = wifiNodes.Get (0);
  staWifiNode = wifiNodes.Get (1);

  /**** Allocate a default DMG Wifi MAC ****/
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

  Ssid ssid = Ssid ("BTI_Test");
  wifiMac.SetType ("ns3::DmgApWifiMac",
                   "Ssid", SsidValue (ssid),
                   "BeaconInterval", TimeValue (MicroSeconds (beaconInterval)),
                   "EnableBeaconRandomization", BooleanValue (beaconRandomization),
                   "NextBeacon", UintegerValue (nextBeacon),
                   "BeaconTransmissionInterval", TimeValue (MicroSeconds (btiDuration)),
                   "NextABFT", UintegerValue (nextAbft),
                   "SSSlotsPerABFT", UintegerValue (slotsPerABFT),
                   "SSFramesPerSlot", UintegerValue (sswPerSlot),
                   "ATIPresent", BooleanValue (atiPresent),
                   "ATIDuration", TimeValue (MicroSeconds (atiDuration)));

  NetDeviceContainer apDevice;
  apDevice = wifi.Install (wifiPhy, wifiMac, apWifiNode);

  wifiMac.SetType ("ns3::DmgStaWifiMac",
                   "Ssid", SsidValue (ssid),
                   "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevice;
  staDevice = wifi.Install (wifiPhy, wifiMac, staWifiNode);

  /* Setting mobility model, Initial Position 1 meter apart */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (1.0, 0.0, 0.0));

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

  /* Enable Traces */
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
      wifiPhy.EnablePcap ("Traces/AccessPoint", apDevice, false);
      wifiPhy.EnablePcap ("Traces/Station", staDevice, false);
    }

  /* Since we have one node, so we steer AP antenna sector towarads it */
  Ptr<WifiNetDevice> apWifiNetDevice = StaticCast<WifiNetDevice> (apDevice.Get (0));
  Ptr<WifiNetDevice> staWifiNetDevice = StaticCast<WifiNetDevice> (staDevice.Get (0));
  apWifiMac = StaticCast<DmgApWifiMac> (apWifiNetDevice->GetMac ());
  staWifiMac = StaticCast<DmgStaWifiMac> (staWifiNetDevice->GetMac ());
  Simulator::Schedule (Seconds (0.9), &DmgApWifiMac::SteerAntennaToward, apWifiMac,
                       Mac48Address::ConvertFrom (staWifiNetDevice->GetAddress ()));

  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
