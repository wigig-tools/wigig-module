/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
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
 * This script is used to evaluate various options to control different access periods within the Beacon Interval.
 *
 * Network Topology:
 * Network topology is simple and consists of One Access Point + One Station. Each station has one antenna array with
 * eight virutal sectors to cover 360 in the 2D Domain.
 *
 *              DMG PCP/AP (0,0)                       DMG STA (-1,0)
 *
 * Running Simulation:
 * To evaluate the script, run the following command:
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
  uint32_t nextAbft = 0;                  /* The number of beacon intervals during which the A-BFT is not be present. */
  uint32_t slotsPerABFT = 8;              /* The number of Sector Sweep Slots Per A-BFT. */
  uint32_t sswPerSlot = 8;                /* The number of SSW Frames per Sector Sweep Slot. */
  bool atiPresent = false;                /* Whether the BI period contains ATI access period. */
  uint16_t atiDuration = 300;             /* The duration of the ATI access period. */
  bool verbose = false;                   /* Print Logging Information. */
  double simulationTime = 4;              /* Simulation time in seconds. */
  bool pcapTracing = true;                /* PCAP Tracing is enabled or not. */

  /* Command line argument parser setup. */
  CommandLine cmd;

  cmd.AddValue ("beaconInterval", "The interval between two Target Beacon Transmission Times (TBTTs)", beaconInterval);
  cmd.AddValue ("beaconRandomization", "Whether to change the sequence of DMG Beacons at each BI", beaconRandomization);
  cmd.AddValue ("nextBeacon", "The number of beacon intervals following the current beacon interval during"
                "which the DMG Beacon is not be present", nextBeacon);
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
  DmgWifiHelper wifi;

  /* Turn on logging */
  if (verbose)
    {
      wifi.EnableLogComponents ();
      LogComponentEnable ("EvaluateBeaconInterval", LOG_LEVEL_ALL);
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
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "ControlMode", StringValue ("DMG_MCS12"),
                                                                "DataMode", StringValue ("DMG_MCS12"));

  /* Make two nodes and set them up with the PHY and the MAC */
  NodeContainer wifiNodes;
  wifiNodes.Create (2);
  apWifiNode = wifiNodes.Get (0);
  staWifiNode = wifiNodes.Get (1);

  /* Add a DMG upper mac */
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

  Ssid ssid = Ssid ("BTI_Test");
  wifiMac.SetType ("ns3::DmgApWifiMac",
                   "Ssid", SsidValue (ssid),
                   "BeaconInterval", TimeValue (MicroSeconds (beaconInterval)),
                   "EnableBeaconRandomization", BooleanValue (beaconRandomization),
                   "NextBeacon", UintegerValue (nextBeacon),
                   "NextABFT", UintegerValue (nextAbft),
                   "SSSlotsPerABFT", UintegerValue (slotsPerABFT),
                   "SSFramesPerSlot", UintegerValue (sswPerSlot),
                   "ATIPresent", BooleanValue (atiPresent),
                   "ATIDuration", TimeValue (MicroSeconds (atiDuration)));

  /* Set Analytical Codebook for the DMG Devices */
  wifi.SetCodebook ("ns3::CodebookAnalytical",
                    "CodebookType", EnumValue (SIMPLE_CODEBOOK),
                    "Antennas", UintegerValue (1),
                    "Sectors", UintegerValue (8));

  /* Create Wifi Network Devices (WifiNetDevice) */
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
  positionAlloc->Add (Vector (-1.0, 0.0, 0.0));

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

  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
