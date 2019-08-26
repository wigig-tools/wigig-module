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
 * This script is used to evaluate allocation of Static Service Periods in IEEE 802.11ad.
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
 * Once all the stations have assoicated successfully with the PCP/AP. The PCP/AP allocates two SPs
 * to perform TxSS between all the stations. Once West DMG STA has completed TxSS phase with East DMG.
 * The PCP/AP allocates two static service periods for communication as following:
 *
 * SP: DMG West STA -----> DMG East STA (SP Length = 3.2ms)
 *
 * Running the Simulation:
 * To run the script with the default parameters:
 * ./waf --run "evaluate_service_period_udp"
 *
 * To run the script with different duration for the service period e.g. SP1=10ms:
 * ./waf --run "evaluate_service_period_udp --spDuration=10000"
 *
 * Simulation Output:
 * The simulation generates the following traces:
 * 1. PCAP traces for each station. From the PCAP files, we can see that data transmission takes place
 * during its SP. In addition, we can notice in the announcement of the two Static Allocation Periods
 * inside each DMG Beacon.
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateServicePeriod");

using namespace ns3;
using namespace std;

/**  Application Variables **/
Ptr<PacketSink> packetSink;

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

/*** Service Periods ***/
uint16_t spDuration = 3200;               /* The duration of the allocated service period in MicroSeconds */
uint8_t receivedInformation = 0;          /* Number of information response received */
uint32_t beamformingStartTime = 0;        /* The start time of the next neafmorming SP */

void
CalculateThroughput (Ptr<PacketSink> sink, uint64_t lastTotalRx, double averageThroughput)
{
  Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
  double cur = (sink->GetTotalRx() - lastTotalRx) * (double) 8/1e5;     /* Convert Application RX Packets to MBits. */
  std::cout << now.GetSeconds () << '\t' << cur << std::endl;
  lastTotalRx = sink->GetTotalRx ();
  averageThroughput += cur;
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput, sink, lastTotalRx, averageThroughput);
}

void
StationAssoicated (Ptr<DmgStaWifiMac> staWifiMac, Mac48Address address, uint16_t staAssociationID)
{
  std::cout << "DMG STA " << staWifiMac->GetAddress () << " associated with DMG AP " << address << std::endl;
  std::cout << "Association ID (AID) = " << staAssociationID << std::endl;
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
      /** Create Airtime Allocation Request for Beamforming **/
      DmgTspecElement element;
      Time duration;
      /* Beamforming Service Period Allocation */
      duration = westWifiMac->ComputeBeamformingAllocationSize (address, true, true);
      element = CreateBeamformingAllocationRequest (ISOCHRONOUS, eastWifiMac->GetAssociationID (),
                                                    true, true, duration.GetMicroSeconds ());
      westWifiMac->CreateAllocation (element);
    }
}

//void
//SLSCompleted (Ptr<DmgWifiMac> staWifiMac, Mac48Address address, ChannelAccessPeriod accessPeriod,
//              BeamformingDirection beamformingDirection, bool isInitiatorTxss, bool isResponderTxss,
//              SECTOR_ID sectorId, ANTENNA_ID antennaId)
//{
//  if (accessPeriod == CHANNEL_ACCESS_DTI)
//    {
//      std::cout << "DMG STA " << staWifiMac->GetAddress () << " completed SLS phase with DMG STA " << address << std::endl;
//      std::cout << "The best antenna configuration is SectorID=" << uint32_t (sectorId)
//                << ", AntennaID=" << uint32_t (antennaId) << std::endl;
//      if (!scheduledStaticPeriods)
//        {
//          std::cout << "Schedule Static Periods" << std::endl;
//          scheduledStaticPeriods = true;
//          /* Schedule Static Periods */
//          apWifiMac->AllocateSingleContiguousBlock (1, SERVICE_PERIOD_ALLOCATION, true, westWifiMac->GetAssociationID (),
//                                                    eastWifiMac->GetAssociationID (), 0, spDuration);
//        }
//    }
//}

void
SLSCompleted (Ptr<DmgWifiMac> staWifiMac, Mac48Address address, ChannelAccessPeriod accessPeriod,
              BeamformingDirection beamformingDirection, bool isInitiatorTxss, bool isResponderTxss,
              SECTOR_ID sectorId, ANTENNA_ID antennaId)
{
  if (accessPeriod == CHANNEL_ACCESS_DTI)
    {
      std::cout << "DMG STA " << staWifiMac->GetAddress () << " completed SLS phase with DMG STA " << address << std::endl;
      std::cout << "The best antenna configuration is SectorID=" << uint32_t (sectorId)
                << ", AntennaID=" << uint32_t (antennaId) << std::endl;
      apWifiMac->PrintSnrTable ();
      westWifiMac->PrintSnrTable ();
      eastWifiMac->PrintSnrTable ();
    }
}

void
ADDTSReceived (Ptr<DmgApWifiMac> apWifiMac, Mac48Address address, DmgTspecElement element)
{
  DmgAllocationInfo info = element.GetDmgAllocationInfo ();
  uint8_t srcAid = apWifiMac->GetStationAid (address);
  /* Decompose Allocation */
  BF_Control_Field bfControl = element.GetBfControl ();
  if (bfControl.IsBeamformTraining ())
    {
      std::cout << "DMG AP received ADDTS Request for allocating BF Service Period" << std::endl;
      beamformingStartTime = apWifiMac->AllocateBeamformingServicePeriod (srcAid,
                                                                          info.GetDestinationAid (),
                                                                          beamformingStartTime,
                                                                          element.GetMinimumDuration (),
                                                                          bfControl.IsInitiatorTxss (),
                                                                          bfControl.IsResponderTxss ());

      /* Set status code to success */
      StatusCode code;
      code.SetSuccess ();

      /* The PCP/AP shall transmit the ADDTS Response frame to the STAs identified as source and destination AID of
       * the DMG TSPEC contained in the ADDTS Request frame if the ADDTS Request is sent by a non-PCP/ non-AP STA. */
      TsDelayElement delayElem;
      Mac48Address destAddress = apWifiMac->GetStationAddress (info.GetDestinationAid ());
      apWifiMac->SendDmgAddTsResponse (address, code, delayElem, element);
      apWifiMac->SendDmgAddTsResponse (destAddress, code, delayElem, element);
    }
}

int
main (int argc, char *argv[])
{
  uint32_t payloadSize = 1448;                  /* Transport Layer Payload size in bytes. */
  string dataRate = "300Mbps";                  /* Application Layer Data Rate. */
  uint32_t msduAggregationSize = 7935;          /* The maximum aggregation size for A-MSDU in Bytes. */
  uint32_t queueSize = 10000;                   /* Wifi Mac Queue Size. */
  string phyMode = "DMG_MCS12";                 /* Type of the Physical Layer. */
  bool verbose = false;                         /* Print Logging Information. */
  double simulationTime = 10;                   /* Simulation time in seconds. */
  bool pcapTracing = false;                     /* PCAP Tracing is enabled or not. */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("dataRate", "Data rate for OnOff Application", dataRate);
  cmd.AddValue ("msduAggregation", "The maximum aggregation size for A-MSDU in Bytes", msduAggregationSize);
  cmd.AddValue ("queueSize", "The size of the Wifi Mac Queue", queueSize);
  cmd.AddValue ("spDuration", "The duration of service period in MicroSeconds", spDuration);
  cmd.AddValue ("phyMode", "802.11ad PHY Mode", phyMode);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable PCAP Tracing", pcapTracing);
  cmd.Parse (argc, argv);

  /* Global params: no fragmentation, no RTS/CTS, fixed rate for all packets */
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::QueueBase::MaxPackets", UintegerValue (queueSize));

  /**** DmgWifiHelper is a meta-helper ****/
  DmgWifiHelper wifi;

  /* Basic setup */
  wifi.SetStandard (WIFI_PHY_STANDARD_80211ad);

  /* Turn on logging */
  if (verbose)
    {
      wifi.EnableLogComponents ();
      LogComponentEnable ("EvaluateServicePeriod", LOG_LEVEL_ALL);
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
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "ControlMode", StringValue (phyMode),
                                                                "DataMode", StringValue (phyMode));

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
                   "BE_MaxAmpduSize", UintegerValue (0),
                   "BE_MaxAmsduSize", UintegerValue (msduAggregationSize));

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

  /*** Install Applications ***/

  /* Install Simple UDP Server on the east Node */
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9999));
  ApplicationContainer sinks = sinkHelper.Install (NodeContainer (eastNode));
  packetSink = StaticCast<PacketSink> (sinks.Get (0));

  /** East Node Variables **/
  uint64_t eastNodeLastTotalRx = 0;
  double eastNodeAverageThroughput = 0;

  /* Install Simple UDP Transmiter on the West Node (Transmit to the East Node) */
  Ptr<OnOffApplication> onoff;
  ApplicationContainer srcApp;
  OnOffHelper src ("ns3::UdpSocketFactory", InetSocketAddress (staInterfaces.GetAddress (1), 9999));
  src.SetAttribute ("MaxBytes", UintegerValue (0));
  src.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  src.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
  src.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  src.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  srcApp = src.Install (westNode);
  onoff = StaticCast<OnOffApplication> (srcApp.Get (0));
  srcApp.Start (Seconds (2.0));

  /* Schedule Throughput Calulcations */
  Simulator::Schedule (Seconds (2.1), &CalculateThroughput, packetSink, eastNodeLastTotalRx, eastNodeAverageThroughput);

  /* Enable Traces */
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
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
  westWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, westWifiMac));
  eastWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, eastWifiMac));
  westWifiMac->TraceConnectWithoutContext ("InformationResponseReceived", MakeBoundCallback (&InformationResponseReceived, westWifiMac));
  eastWifiMac->TraceConnectWithoutContext ("InformationResponseReceived", MakeBoundCallback (&InformationResponseReceived, eastWifiMac));
  apWifiMac->TraceConnectWithoutContext ("ADDTSReceived", MakeBoundCallback (&ADDTSReceived, apWifiMac));

  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();

  /* Print Results Summary */
  std::cout << "Total number of transmitted packets = " << onoff->GetTotalTxPackets () << std::endl;
  std::cout << "Total number of received packets = " << packetSink->GetTotalReceivedPackets () << std::endl;
  std::cout << "Total throughput for Data SP Allocation (" << spDuration/1000.0
            << " ms) = " << (packetSink->GetTotalRx () * 8)/((simulationTime - 2) * 1e6) << " [Mbps]" << std::endl;

  Simulator::Destroy ();

  return 0;
}
