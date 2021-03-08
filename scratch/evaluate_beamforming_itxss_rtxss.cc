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
 * Evaluate the allocation of Beamforming Service Periods in IEEE 802.11ad.
 *
 * Network Topology:
 * The scenario consists of 2 DMG STAs (West + East) and one DMG PCP/AP as following:
 *
 *                       DMG PCP/AP (0,+1)
 *
 *
 * West DMG STA (-1,0)                      East DMG STA (+1,0)
 *
 * Simulation Description:
 * The script simulates the steps requried to do beamforming in DTI access period between an initiator
 * and a responder as defined in 802.11ad. During the asoociation phase, each station includes its
 * DMG Capabilities IE in its Association Request frame. Once all the stations have assoicated
 * successfully with the DMG PCP/AP, the DMG West STA sends an Information Request frame to the DMG PCP/AP
 * to request the capabilities of the DMG East STA. Once this information is available, the DMG West STA
 * sends a request to the MG PCP/AP to allocate a single SP to perform Beamforming Training (TXSS) as following:
 *
 * SP: West DMG STA (TXSS) -------> East DMG STA (TXSS)
 *
 * All devices in the network have different hardware capabiltiies. The DMG PCP/AP has a single phased antenna
 * array with 16 virtual sectors. While the west DMG STA has 2 arrays each of which has 8 virtual sectors.
 * The east DMG STA has a single array with only 6 virtual sectors.
 *
 * Running the Simulation:
 * To run the script with the default parameters:
 * ./waf --run "evaluate_beamforming_itxss_rtxss --pcap=1"
 *
 * Simulation Output:
 * The simulation generates the following traces:
 * 1. PCAP traces for each station. From the PCAP files, we can see the allocation of beamforming service periods.
 * 2. SNR Dump for each sector.
 */

NS_LOG_COMPONENT_DEFINE ("BeamformingTraining");

using namespace ns3;
using namespace std;

/* Network Nodes */
Ptr<WifiNetDevice> apWifiNetDevice;
Ptr<WifiNetDevice> westWifiNetDevice;
Ptr<WifiNetDevice> eastWifiNetDevice;

Ptr<DmgApWifiMac> apWifiMac;
Ptr<DmgStaWifiMac> westWifiMac;
Ptr<DmgStaWifiMac> eastWifiMac;

/*** Access Point Variables ***/
uint8_t assoicatedStations = 0;           /* Total number of assoicated stations with the DMG PCP/AP. */
uint8_t stationsTrained = 0;              /* Number of BF trained stations. */
bool scheduledStaticPeriods = false;      /* Flag to indicate whether we scheduled Static Service Periods or not. */

/*** Beamforming Service Periods ***/
uint8_t beamformedLinks = 0;              /* Number of beamformed links. */
uint32_t beamformingStartTime = 0;        /* The start time of the next neafmorming SP. */
uint8_t receivedInformation = 0;          /* Number of information response received. */

void
StationAssoicated (Ptr<DmgStaWifiMac> staWifiMac, Mac48Address address, uint16_t aid)
{
  std::cout << "DMG STA: " << staWifiMac->GetAddress () << " associated with DMG PCP/AP: " << address << std::endl;
  std::cout << "Association ID (AID) = " << aid << std::endl;
  assoicatedStations++;
  /* Check if all stations have assoicated with the PCP/AP */
  if (assoicatedStations == 2)
    {
      std::cout << "All stations got associated with PCP/AP: " << address << std::endl;
      /* Create list of Information Element ID to request */
      WifiInformationElementIdList list;
      list.push_back (std::make_pair (IE_DMG_CAPABILITIES,0));
      /* West DMG STA Request information about East STA */
      westWifiMac->RequestInformation (eastWifiMac->GetAddress (), list);
      /* East DMG STA Request information about West STA */
      eastWifiMac->RequestInformation (westWifiMac->GetAddress (), list);
    }
}

DmgTspecElement
CreateBeamformingAllocationRequest (AllocationFormat format, uint8_t destAid,
                                    bool isInitiatorTxss, bool isResponderTxss,
                                    uint16_t spDuration)
{
  DmgTspecElement element;

  DmgAllocationInfo info;
  info.SetAllocationID (10);
  info.SetAllocationType (SERVICE_PERIOD_ALLOCATION);
  info.SetAllocationFormat (format);
  info.SetAsPseudoStatic (false);
  info.SetAsTruncatable (false);
  info.SetAsExtendable (false);
  info.SetLpScUsed (false);
  info.SetUp (0);
  info.SetDestinationAid (destAid);
  element.SetDmgAllocationInfo (info);

  BF_Control_Field bfField;
  bfField.SetBeamformTraining (true);
  bfField.SetAsInitiatorTXSS (isInitiatorTxss);
  bfField.SetAsResponderTXSS (isResponderTxss);
  element.SetBfControl (bfField);

  /* For more deetails on the meaning of this field refer to IEEE 802.11-2012ad 10.4.13*/
  element.SetAllocationPeriod (0, false);
  element.SetMinimumDuration (spDuration);

  return element;
}

void
InformationResponseReceived (Ptr<DmgStaWifiMac> staWifiMac, Mac48Address address)
{
  std::cout << "DMG STA=" << staWifiMac->GetAddress () <<" received Information Response regarding DMG STA=" << address << std::endl;
  receivedInformation++;
  if (receivedInformation == 2)
    {
      /** Create Airtime Allocation Request for Beamforming Training **/
      DmgTspecElement element;
      Time duration;
      /* SP Allocation */
      duration = westWifiMac->ComputeBeamformingAllocationSize (address, true, true);
      element = CreateBeamformingAllocationRequest (ISOCHRONOUS, eastWifiMac->GetAssociationID (),
                                                    true, true, duration.GetMicroSeconds ());
      westWifiMac->CreateAllocation (element);
    }
}

void
SLSCompleted (Ptr<DmgWifiMac> wifiMac, SlsCompletionAttrbitutes attributes)
{
  if (attributes.accessPeriod == CHANNEL_ACCESS_DTI)
    {
      beamformedLinks++;
      std::cout << "DMG STA " << wifiMac->GetAddress ()
                << " completed SLS phase with DMG STA " << attributes.peerStation << std::endl;
      std::cout << "The best antenna configuration is AntennaID=" << uint16_t (attributes.antennaID)
                << ", SectorID=" << uint16_t (attributes.sectorID) << std::endl;
      if (beamformedLinks == 2)
        {
          apWifiMac->PrintSnrTable ();
          westWifiMac->PrintSnrTable ();
          eastWifiMac->PrintSnrTable ();
        }
    }
}

void
ADDTSReceived (Ptr<DmgApWifiMac> apWifiMac, Mac48Address address, DmgTspecElement element)
{
  DmgAllocationInfo info = element.GetDmgAllocationInfo ();
  uint8_t srcAid = apWifiMac->GetStationAid (address);
  /* Decompose Allocation */
  BF_Control_Field bfControl = element.GetBfControl ();
  std::cout << "DMG PCP/AP received ADDTS Request for allocating BF Service Period" << std::endl;
  beamformingStartTime = apWifiMac->AllocateBeamformingServicePeriod (srcAid, info.GetDestinationAid (),
                                                                      beamformingStartTime,
                                                                      element.GetMinimumDuration (),
                                                                      bfControl.IsInitiatorTXSS (), bfControl.IsResponderTXSS ());

  /* Set status code */
  StatusCode code;
  code.SetSuccess ();

  /* The DMG PCP/AP shall transmit the ADDTS Response frame to the STAs identified
   * as source and destination AID of the DMG TSPEC contained in the ADDTS Request
   * frame if the ADDTS Request is sent by a non-PCP/ non-AP STA. */
  TsDelayElement delayElem;
  Mac48Address destAddress = apWifiMac->GetStationAddress (info.GetDestinationAid ());
  apWifiMac->SendDmgAddTsResponse (address, code, delayElem, element);
  apWifiMac->SendDmgAddTsResponse (destAddress, code, delayElem, element);
}

int
main (int argc, char *argv[])
{
  bool verbose = false;                         /* Print Logging Information. */
  double simulationTime = 1;                    /* Simulation time in seconds. */
  bool pcapTracing = false;                     /* PCAP Tracing is enabled or not. */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable PCAP Tracing", pcapTracing);
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
      LogComponentEnable ("BeamformingTraining", LOG_LEVEL_ALL);
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

  /* Make four nodes and set them up with the PHY and the MAC */
  NodeContainer wifiNodes;
  NetDeviceContainer staDevices;
  wifiNodes.Create (3);
  Ptr<Node> apNode = wifiNodes.Get (0);
  Ptr<Node> westNode = wifiNodes.Get (1);
  Ptr<Node> eastNode = wifiNodes.Get (2);

  /* Add a DMG upper mac */
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

  /* Install DMG PCP/AP Node */
  Ssid ssid = Ssid ("ServicePeriod");
  wifiMac.SetType ("ns3::DmgApWifiMac",
                   "Ssid", SsidValue(ssid),
                   "BE_MaxAmpduSize", UintegerValue (0),
                   "SSSlotsPerABFT", UintegerValue (8), "SSFramesPerSlot", UintegerValue (8),
                   "BeaconInterval", TimeValue (MicroSeconds (102400)));

  /* Set Analytical Codebook for the DMG PCP/AP */
  wifi.SetCodebook ("ns3::CodebookAnalytical",
                    "CodebookType", EnumValue (SIMPLE_CODEBOOK),
                    "Antennas", UintegerValue (1),
                    "Sectors", UintegerValue (16));

  NetDeviceContainer apDevice;
  apDevice = wifi.Install (wifiPhy, wifiMac, apNode);

  /* Install DMG STA Nodes */
  wifiMac.SetType ("ns3::DmgStaWifiMac",
                   "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false),
                   "BE_MaxAmpduSize", UintegerValue (0));

  /* Set Analytical Codebook for the the West Node */
  wifi.SetCodebook ("ns3::CodebookAnalytical",
                    "CodebookType", EnumValue (SIMPLE_CODEBOOK),
                    "Antennas", UintegerValue (2),
                    "Sectors", UintegerValue (8));

  NetDeviceContainer staDevice;
  staDevice = wifi.Install (wifiPhy, wifiMac, westNode);
  staDevices.Add (staDevice);

  /* Set Analytical Codebook for the the East Node */
  wifi.SetCodebook ("ns3::CodebookAnalytical",
                    "CodebookType", EnumValue (SIMPLE_CODEBOOK),
                    "Antennas", UintegerValue (1),
                    "Sectors", UintegerValue (6));

  staDevice = wifi.Install (wifiPhy, wifiMac, eastNode);
  staDevices.Add (staDevice);

  /* Setting mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, +1.0, 0.0));   /* DMG PCP/AP */
  positionAlloc->Add (Vector (-1.0, 0.0, 0.0));   /* West DMG STA */
  positionAlloc->Add (Vector (+1.0, 0.0, 0.0));   /* East DMG STA */

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

  /* Enable Traces */
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
      wifiPhy.EnablePcap ("Traces/AccessPoint", apDevice, false);
      wifiPhy.EnablePcap ("Traces/WestNode", staDevices.Get (0), false);
      wifiPhy.EnablePcap ("Traces/EastNode", staDevices.Get (1), false);
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
  westWifiMac->TraceConnectWithoutContext ("InformationResponseReceived", MakeBoundCallback (&InformationResponseReceived, westWifiMac));
  eastWifiMac->TraceConnectWithoutContext ("InformationResponseReceived", MakeBoundCallback (&InformationResponseReceived, eastWifiMac));
  westWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, westWifiMac));
  eastWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, eastWifiMac));
  apWifiMac->TraceConnectWithoutContext ("ADDTSReceived", MakeBoundCallback (&ADDTSReceived, apWifiMac));

  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
