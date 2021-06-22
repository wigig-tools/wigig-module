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

/**
 * Simulation Objective:
 * This script is used to evaluate the beam refinement protocol (BRP) in IEEE 802.11ad.
 *
 * Network Topology:
 * The scenario consists of 2 DMG STAs (West + East) and one PCP/AP as following:
 *
 *                         DMG AP (0,1)
 *
 *
 * West DMG STA (-1,0)                      East DMG STA (1,0)
 *
 * Simulation Description:
 * Once all the stations have assoicated successfully with the PCP/AP, the PCP/AP allocates one SP
 * to perform Beamforming Training (TXSS) between West DMG STA and East DMG STA. Once the SLS TXSS
 * is compelted, West DMG STA initiate BRP to refine its beam pattern.
 *
 * Running the Simulation:
 * To run the script with the default parameters:
 * ./waf --run "evaluate_beam_refinement_protocol"
 *
 * Simulation Output:
 * The simulation generates the following traces:
 * 1. PCAP traces for each station. From the PCAP files, we can see the allocation of beamforming service periods.
 * 2. BRP ASCII traces.
 */

NS_LOG_COMPONENT_DEFINE ("BRP_Protocol");

using namespace ns3;
using namespace std;

/* Network Nodes */
Ptr<WifiNetDevice> apWifiNetDevice;
Ptr<WifiNetDevice> westWifiNetDevice;
Ptr<WifiNetDevice> eastWifiNetDevice;

NetDeviceContainer staDevices;

Ptr<DmgApWifiMac> apWifiMac;
Ptr<DmgStaWifiMac> westWifiMac;
Ptr<DmgStaWifiMac> eastWifiMac;

/*** Access Point Variables ***/
uint8_t assoicatedStations = 0;           /* Total number of assoicated stations with the AP */
uint8_t stationsTrained = 0;              /* Number of BF trained stations */
bool scheduledStaticPeriods = false;      /* Flag to indicate whether we scheduled Static Service Periods or not */

/*** Beamforming Service Periods ***/
uint8_t beamformedLinks = 0;              /* Number of beamformed links */

void
StationAssoicated (Ptr<DmgStaWifiMac> staWifiMac, Mac48Address address, uint16_t aid)
{
  std::cout << "DMG STA " << staWifiMac->GetAddress () << " associated with DMG AP " << address << std::endl;
  std::cout << "Association ID (AID) = " << aid << std::endl;
  assoicatedStations++;
  /* Map AID to MAC Addresses in each node instead of requesting information */
  Ptr<DmgStaWifiMac> dmgStaMac;
  for (NetDeviceContainer::Iterator i = staDevices.Begin (); i != staDevices.End (); ++i)
    {
      dmgStaMac = StaticCast<DmgStaWifiMac> (StaticCast<WifiNetDevice> (*i)->GetMac ());
      if (dmgStaMac != staWifiMac)
        {
          dmgStaMac->MapAidToMacAddress (staWifiMac->GetAssociationID (), staWifiMac->GetAddress ());
          staWifiMac->StorePeerDmgCapabilities (dmgStaMac);
        }
    }

  /* Check if all stations have assoicated with the PCP/AP */
  if (assoicatedStations == 2)
    {
      std::cout << "All stations got associated with " << address << std::endl;
      /*** Schedule Beamforming Training SPs ***/
      uint32_t startTime =0;
      startTime = apWifiMac->AllocateBeamformingServicePeriod (westWifiMac->GetAssociationID (),
                                                               eastWifiMac->GetAssociationID (), startTime, true);
    }
}

void
SLSCompleted (Ptr<DmgWifiMac> wifiMac, SlsCompletionAttrbitutes attributes)
{
  if (attributes.accessPeriod == CHANNEL_ACCESS_BHI)
    {
      if (wifiMac == apWifiMac)
        {
          std::cout << "DMG AP " << apWifiMac->GetAddress () << " completed SLS phase with DMG STA " << attributes.peerStation << std::endl;
        }
      else
        {
          std::cout << "DMG STA " << wifiMac->GetAddress () << " completed SLS phase with DMG AP " << attributes.peerStation << std::endl;
        }
      std::cout << "Best Tx Antenna Configuration: AntennaID=" << uint16_t (attributes.antennaID)
                << ", SectorID=" << uint16_t (attributes.sectorID) << std::endl;
    }
  else if (attributes.accessPeriod == CHANNEL_ACCESS_DTI)
    {
      beamformedLinks++;
      std::cout << "DMG STA " << wifiMac->GetAddress () << " completed SLS phase with DMG STA " << attributes.peerStation << std::endl;
      std::cout << "The best antenna configuration is AntennaID=" << uint16_t (attributes.antennaID)
                << ", SectorID=" << uint16_t (attributes.sectorID) << std::endl;
      if (beamformedLinks == 2)
        {
          apWifiMac->PrintSnrTable ();
          westWifiMac->PrintSnrTable ();
          eastWifiMac->PrintSnrTable ();
          std::cout << "West DMG STA " << wifiMac->GetAddress ()
                    << " initiating BRP Transaction with DMG STA " << attributes.peerStation << std::endl;
          Simulator::Schedule (MicroSeconds (3), &DmgStaWifiMac::InitiateBrpTransaction,
                               westWifiMac, eastWifiMac->GetAddress (), 0, true);
        }
    }
}

void
BRPCompleted (Ptr<DmgWifiMac> wifiMac, Mac48Address address, BeamRefinementType type,
              AntennaID antennaID, SectorID sectorID, AWV_ID awvID)
{
  std::cout << "DMG STA " << wifiMac->GetAddress ();
  if (type == RefineTransmitSector)
    {
      std::cout << " completed BRP-TX ";
    }
  else
    {
      std::cout << " completed BRP-RX ";
    }
  std::cout << "with DMG STA " << address << std::endl;
  std::cout << "Best beamforming configuration: Custom AWV ID=" << uint16_t (awvID) << ", AntennaID=" << uint16_t (antennaID)
            << ", AntennaID=" << uint16_t (sectorID) << std::endl;
  eastWifiMac->PrintBeamRefinementMeasurements ();
}

int
main (int argc, char *argv[])
{
  uint16_t sectors = 8;                         /* The number of sectors in the antenna array. */
  uint16_t awvs = 8;                            /* The number of custom AWVs per sector. */
  bool verbose = false;                         /* Print Logging Information. */
  double simulationTime = 10;                   /* Simulation time in seconds. */
  bool pcapTracing = false;                     /* PCAP Tracing is enabled or not. */
  bool asciiTracing = false;                    /* ASCII Tracing is enabled or not. */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("sectors", "The number of sectors in the antenna array", sectors);
  cmd.AddValue ("awvs", "The number of custom AWVs per sector", awvs);
  cmd.AddValue ("verbose", "Turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable PCAP Tracing", pcapTracing);
  cmd.AddValue ("ascii", "Enable ASCII Tracing", asciiTracing);
  cmd.Parse (argc, argv);

  /* Configure RTS/CTS and Fragmentation */
  ConfigureRtsCtsAndFragmenatation ();

  /**** DmgWifiHelper is a meta-helper ****/
  DmgWifiHelper wifi;

  /* Basic setup */
  wifi.SetStandard (WIFI_PHY_STANDARD_80211ad);

  /* Turn on logging */
  if (verbose)
    {
      wifi.EnableLogComponents ();
      LogComponentEnable ("BRP_Protocol", LOG_LEVEL_ALL);
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
  /* Set default algorithm for all nodes to be constant rate */
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("DMG_MCS12"));

  /* Make four nodes and set them up with the phy and the mac */
  NodeContainer wifiNodes;
  wifiNodes.Create (3);
  Ptr<Node> apNode = wifiNodes.Get (0);
  Ptr<Node> westNode = wifiNodes.Get (1);
  Ptr<Node> eastNode = wifiNodes.Get (2);

  /* Add a DMG upper mac */
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

  /* Install DMG PCP/AP Node */
  Ssid ssid = Ssid ("BRP");
  wifiMac.SetType ("ns3::DmgApWifiMac",
                   "Ssid", SsidValue(ssid),
                   "BE_MaxAmpduSize", UintegerValue (0),
                   "SSSlotsPerABFT", UintegerValue (8), "SSFramesPerSlot", UintegerValue (8),
                   "BeaconInterval", TimeValue (MicroSeconds (102400)));

  /* Set Analytical Codebook for the DMG Devices */
  wifi.SetCodebook ("ns3::CodebookAnalytical",
                    "CodebookType", EnumValue (SIMPLE_CODEBOOK),
                    "Antennas", UintegerValue (1),
                    "Sectors", UintegerValue (sectors),
                    "AWVs", UintegerValue (awvs));

  NetDeviceContainer apDevice;
  apDevice = wifi.Install (wifiPhy, wifiMac, apNode);

  /* Install DMG STA Nodes */
  wifiMac.SetType ("ns3::DmgStaWifiMac",
                   "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false),
                   "BE_MaxAmpduSize", UintegerValue (0));

  staDevices = wifi.Install (wifiPhy, wifiMac, NodeContainer (westNode, eastNode));

  /* Setting mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, +1.0, 0.0));   /* PCP/AP */
  positionAlloc->Add (Vector (-1.0, 0.0, 0.0));   /* West STA */
  positionAlloc->Add (Vector (+1.0, 0.0, 0.0));   /* East STA */

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
  Ipv4InterfaceContainer staInterfaces;
  staInterfaces = address.Assign (staDevices);

  /* Populate routing table */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* We do not want any ARP packets */
  PopulateArpCache ();

  /* Enable PCAP Traces */
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
      wifiPhy.EnablePcap ("Traces/AccessPoint", apDevice, false);
      wifiPhy.EnablePcap ("Traces/WestNode", staDevices.Get (0), false);
      wifiPhy.EnablePcap ("Traces/EastNode", staDevices.Get (1), false);
    }

  /* Enable ASCII Traces */
  if (asciiTracing)
    {
      wifiPhy.SetAsciiTraceType (ASCII_TRACE_PHY_ACTIVITY);
      wifiPhy.EnableAscii ("Traces/AccessPoint", apDevice);
//      wifiPhy.EnableAscii ("Traces/WestNode", staDevices.Get (0));
//      wifiPhy.EnableAscii ("Traces/EastNode", staDevices.Get (1));
    }

  /* Stations */
  apWifiNetDevice = StaticCast<WifiNetDevice> (apDevice.Get (0));
  westWifiNetDevice = StaticCast<WifiNetDevice> (staDevices.Get (0));
  eastWifiNetDevice = StaticCast<WifiNetDevice> (staDevices.Get (1));

  apWifiMac = StaticCast<DmgApWifiMac> (apWifiNetDevice->GetMac ());
  westWifiMac = StaticCast<DmgStaWifiMac> (westWifiNetDevice->GetMac ());
  eastWifiMac = StaticCast<DmgStaWifiMac> (eastWifiNetDevice->GetMac ());

  /** Connect Traces **/
  westWifiMac->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, westWifiMac));
  eastWifiMac->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, eastWifiMac));
  apWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, apWifiMac));
  westWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, westWifiMac));
  westWifiMac->TraceConnectWithoutContext ("BRPCompleted", MakeBoundCallback (&BRPCompleted, westWifiMac));
  eastWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, eastWifiMac));

  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
