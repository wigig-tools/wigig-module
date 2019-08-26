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
 * This script is used to evaluate allocation of Beamforming Service Periods in IEEE 802.11ad.
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
 * The script is intended to simulate the steps requried to do beamforming in DTI between an initiator
 * and a responder as defined in 802.11ad:
 *
 * 9.35.6 Beamforming in DTI
 * 9.35.6.1 General
 * An initiator and responder may perform BF training in the DTI within an SP allocation or within a TXOP allocation.
 * An initiator shall determine the capabilities of the responder prior to initiating BF training with the
 * responder if the responder is associated. A STA may obtain the capabilities of other STAs through the
 * Information Request and Information Response frames (10.29.1) or following a STA’s association with the
 * PBSS/infrastructure BSS. The initiator should use its own capabilities and the capabilities of the responder
 * to compute the required allocation size to perform BF training and BF training related timeouts.
 * An initiator may request the PCP/AP to schedule an SP to perform BF training with a responder by setting
 * the Beamforming Training subfield in the BF Control field of the DMG TSPEC element or SPR frame to 1.
 * The PCP/AP shall set the Beamforming Training subfield to 1 in the Allocation field of the Extended
 * Schedule element if the Beamforming Training subfield in the BF Control field of the DMG TSPEC element
 * or SPR frame that generated this Allocation field is equal to 1. The PCP/AP should set the Beamforming
 * Training subfield in an Allocation field of the Extended Schedule element to 0 if this subfield was equal to 1
 * when the allocation was last transmitted by the PCP/AP in an Extended Schedule element and if, since that
 * last transmission, the PCP/AP did not receive a DMG TSPEC element for this allocation with the
 * Beamforming Training subfield equal to 1.
 *
 * Once all the stations have assoicated successfully with the PCP/AP, the DMG West STA send Information Request Element
 * frame to the DMG AP to request the capabilities of the DMG East STA. One this information become available, DMG West STA
 * send a request to the PCP/AP to allocate two SPs to perform Beamforming Training (TxSS & RxSS) as following:
 *
 * SP1: West DMG STA (TxSS) -------> East DMG STA (TxSS)
 * SP2: West DMG STA (RxSS) -------> East DMG STA (RxSS)
 *
 * Running the Simulation:
 * To run the script with the default parameters:
 * ./waf --run "evaluate_beamforming_sp2"
 *
 * Simulation Output:
 * The simulation generates the following traces:
 * 1. PCAP traces for each station. From the PCAP files, we can see the allocation of beamforming service periods.
 * 2. SNR Dump for each sector.
 */

NS_LOG_COMPONENT_DEFINE ("BeamformingSP");

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
uint32_t beamformingStartTime = 0;        /* The start time of the next neafmorming SP */
uint8_t receivedInformation = 0;          /* Number of information response received */

void
StationAssoicated (Ptr<DmgStaWifiMac> staWifiMac, Mac48Address address, uint16_t aid)
{
  std::cout << "DMG STA " << staWifiMac->GetAddress () << " associated with DMG AP " << address << std::endl;
  std::cout << "Association ID (AID) = " << aid << std::endl;
  assoicatedStations++;
  /* Check if all stations have assoicated with the PCP/AP */
  if (assoicatedStations == 2)
    {
      std::cout << "All stations got associated with " << address << std::endl;
      /* Create list of Information Element ID to request */
      WifiInformationElementIdList list;
      list.push_back (IE_DMG_CAPABILITIES);
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
  bfField.SetAsInitiatorTxss (isInitiatorTxss);
  bfField.SetAsResponderTxss (isResponderTxss);
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
      /** Create Airtime Allocation Requests for Beamforming **/
      DmgTspecElement element;
      Time duration;
      /* SP1 Allocation */
      duration = westWifiMac->ComputeBeamformingAllocationSize (eastWifiMac->GetAddress (), true, true);
      element = CreateBeamformingAllocationRequest (ISOCHRONOUS, eastWifiMac->GetAssociationID (), true, true, duration.GetMicroSeconds ());
      westWifiMac->CreateAllocation (element);
      /* SP2 Allocation */
      duration = westWifiMac->ComputeBeamformingAllocationSize (eastWifiMac->GetAddress (), false, false);
      element = CreateBeamformingAllocationRequest (ISOCHRONOUS, eastWifiMac->GetAssociationID (), false, false, duration.GetMicroSeconds ());
      westWifiMac->CreateAllocation (element);
    }
}

void
SLSCompleted (Ptr<DmgWifiMac> staWifiMac, Mac48Address address, ChannelAccessPeriod accessPeriod,
              BeamformingDirection beamformingDirection, bool isInitiatorTxss, bool isResponderTxss,
              SECTOR_ID sectorId, ANTENNA_ID antennaId)
{
  if (accessPeriod == CHANNEL_ACCESS_DTI)
    {
      beamformedLinks++;
      std::cout << "DMG STA " << staWifiMac->GetAddress () << " completed SLS phase with DMG STA " << address << std::endl;
      std::cout << "The best antenna configuration is SectorID=" << uint32_t (sectorId)
                << ", AntennaID=" << uint32_t (antennaId) << std::endl;
      if (beamformedLinks == 6)
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
  std::cout << "DMG AP received ADDTS Request for allocating BF Service Period" << std::endl;
  beamformingStartTime = apWifiMac->AllocateBeamformingServicePeriod (srcAid, info.GetDestinationAid (),
                                                                      beamformingStartTime,
                                                                      element.GetMinimumDuration (),
                                                                      bfControl.IsInitiatorTxss (), bfControl.IsResponderTxss ());

  /* Set status code */
  StatusCode code;
  code.SetSuccess ();

  /* The PCP/AP shall transmit the ADDTS Response frame to the STAs identified as source and destination AID of
   * the DMG TSPEC contained in the ADDTS Request frame if the ADDTS Request it is sent by a non-PCP/ non-AP STA. */
  TsDelayElement delayElem;
  Mac48Address destAddress = apWifiMac->GetStationAddress (info.GetDestinationAid ());
  apWifiMac->SendDmgAddTsResponse (address, code, delayElem, element);
  apWifiMac->SendDmgAddTsResponse (destAddress, code, delayElem, element);
}

int
main (int argc, char *argv[])
{
  bool verbose = false;                         /* Print Logging Information. */
  double simulationTime = 10;                   /* Simulation time in seconds. */
  bool pcapTracing = false;                     /* PCAP Tracing is enabled or not. */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable PCAP Tracing", pcapTracing);
  cmd.Parse (argc, argv);

  /* Global params: no fragmentation, no RTS/CTS, fixed rate for all packets */
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));

  /**** DmgWifiHelper is a meta-helper ****/
  DmgWifiHelper wifi;

  /* Basic setup */
  wifi.SetStandard (WIFI_PHY_STANDARD_80211ad);

  /* Turn on logging */
  if (verbose)
    {
      wifi.EnableLogComponents ();
      LogComponentEnable ("BeamformingSP", LOG_LEVEL_ALL);
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

  /* Make four nodes and set them up with the phy and the mac */
  NodeContainer wifiNodes;
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
