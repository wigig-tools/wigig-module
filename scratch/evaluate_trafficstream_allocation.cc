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
 * This script is used to evaluate allocation of Static Service Periods using traffic stream in IEEE 802.11ad.
 *
 * Network Topology:
 * The scenario consists of 3 DMG STAs (West + South + East) and one DMG PCP/AP as following:
 *
 *                         DMG AP (0,1)
 *
 *
 * West DMG STA (-1,0)                      East DMG STA (1,0)
 *
 *
 *                      South DMG STA (0,-1)
 *
 * Simulation Description:
 * Once all the stations have assoicated successfully with the PCP/AP. The PCP/AP allocates three SPs
 * to perform SLS (TxSS) between all the stations. Once West DMG STA has completed TxSS phase with East and
 * South DMG STAs. The West DMG STA sends two ADDTS Request for SP allocations request as following:
 *
 * Traffic Format = ISOCHRONOUS Traffic Type (Periodic Traffic)
 * Allocation Period = BI/4 i.e. 4 SPs per BI.
 * Single SP Allocation Duration = 3.2ms
 *
 * SP1: West DMG STA -----> East DMG STA
 * SP2: West DMG STA -----> South DMG STA
 *
 * The PCP/AP takes care of positioning the SPs within the BI.
 *
 * Running the Simulation:
 * To run the script with the default parameters:
 * ./waf --run "evaluate_trafficstream_allocation"
 *
 * Simulation Output:
 * The simulation generates the following traces:
 * 1. PCAP traces for each station. FFrom the PCAP files, we can see that data transmission takes place during its SP.
 * In addition, we can notice in the announcement of the two Static Allocation Periods inside each DMG Beacon.
 *
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateTrafficStreamAllocation");

using namespace ns3;
using namespace std;

/** West-East Allocation Variables **/
uint64_t westEastLastTotalRx = 0;
double westEastAverageThroughput = 0;
/** West-South Node Allocation Variables **/
uint64_t westSouthTotalRx = 0;
double westSouthAverageThroughput = 0;

Ptr<PacketSink> sink1, sink2;

/* Network Nodes */
Ptr<WifiNetDevice> apWifiNetDevice;
Ptr<WifiNetDevice> southWifiNetDevice;
Ptr<WifiNetDevice> westWifiNetDevice;
Ptr<WifiNetDevice> eastWifiNetDevice;

NetDeviceContainer staDevices;

Ptr<DmgApWifiMac> apWifiMac;
Ptr<DmgStaWifiMac> southWifiMac;
Ptr<DmgStaWifiMac> westWifiMac;
Ptr<DmgStaWifiMac> eastWifiMac;

/*** Access Point Variables ***/
uint8_t assoicatedStations = 0;           /* Total number of assoicated stations with the AP */
uint8_t stationsTrained = 0;              /* Number of BF trained stations */
bool scheduledStaticPeriods = false;      /* Flag to indicate whether we scheduled Static Service Periods or not */

/*** Service Period ***/
uint16_t spDuration = 3200;               /* The duration of the allocated service period block in MicroSeconds */

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
  double thr1, thr2;
  Time now = Simulator::Now ();
  thr1 = CalculateSingleStreamThroughput (sink1, westEastLastTotalRx, westEastAverageThroughput);
  thr2 = CalculateSingleStreamThroughput (sink2, westSouthTotalRx, westSouthAverageThroughput);
  std::cout << now.GetSeconds () << '\t' << thr1 << '\t'<< thr2 << std::endl;
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

void
StationAssoicated (Ptr<DmgStaWifiMac> staWifiMac, Mac48Address address)
{
  std::cout << "DMG STA " << staWifiMac->GetAddress () << " associated with DMG AP " << address << std::endl;
  std::cout << "Association ID (AID) = " << staWifiMac->GetAssociationID () << std::endl;
  assoicatedStations++;
  /* Check if all stations have assoicated with the PCP/AP */
  if (assoicatedStations == 3)
    {
      /* Map AID to MAC Addresses in each node instead of requesting information */
      Ptr<DmgStaWifiMac> srcMac, dstMac;
      for (NetDeviceContainer::Iterator i = staDevices.Begin (); i != staDevices.End (); ++i)
        {
          srcMac = StaticCast<DmgStaWifiMac> (StaticCast<WifiNetDevice> (*i)->GetMac ());
          for (NetDeviceContainer::Iterator j = staDevices.Begin (); j != staDevices.End (); ++j)
            {
              dstMac = StaticCast<DmgStaWifiMac> (StaticCast<WifiNetDevice> (*j)->GetMac ());
              if (srcMac != dstMac)
                {
                  srcMac->MapAidToMacAddress (dstMac->GetAssociationID (), dstMac->GetAddress ());
                }
            }
        }

      std::cout << "All stations got associated with " << address << std::endl;
      /* Schedule Beamforming Training SP */
      apWifiMac->AllocateBeamformingServicePeriod (westWifiMac->GetAssociationID (), eastWifiMac->GetAssociationID (), 0, true);
      apWifiMac->AllocateBeamformingServicePeriod (westWifiMac->GetAssociationID (), southWifiMac->GetAssociationID (), 500, true);
      apWifiMac->AllocateBeamformingServicePeriod (southWifiMac->GetAssociationID (), eastWifiMac->GetAssociationID (), 1000, true);
    }
}

DmgTspecElement
CreateTimeAllocationRequest (AllocationFormat format, uint8_t destAid, bool multipleAllocation, uint16_t period, uint16_t spDuration)
{
  DmgTspecElement element;

  DmgAllocationInfo info;
  info.SetAllocationID (10);
  info.SetAllocationType (SERVICE_PERIOD_ALLOCATION);
  info.SetAllocationFormat (format);
  info.SetAsPseudoStatic (true);
  info.SetAsTruncatable (false);
  info.SetAsExtendable (false);
  info.SetLpScUsed (false);
  info.SetUp (0);
  info.SetDestinationAid (destAid);
  element.SetDmgAllocationInfo (info);

  BF_Control_Field bfField;
  bfField.SetBeamformTraining (false); // This SP for data communication
  element.SetBfControl (bfField);

  /* For more deetails on the meaning of this field refer to IEEE 802.11-2012ad 10.4.13*/
  element.SetAllocationPeriod (period, multipleAllocation);
  element.SetMinimumAllocation (spDuration * period);
  element.SetMaximumAllocation (spDuration * period * 2);
  element.SetMinimumDuration (spDuration);

  return element;
}

void
SLSCompleted (Ptr<DmgStaWifiMac> staWifiMac, Mac48Address address,
              ChannelAccessPeriod accessPeriod, SECTOR_ID sectorId, ANTENNA_ID antennaId)
{
  if (accessPeriod == CHANNEL_ACCESS_DTI)
    {
      std::cout << "DMG STA " << staWifiMac->GetAddress () << " completed SLS phase with DMG STA " << address << std::endl;
      std::cout << "The best antenna configuration is SectorID=" << uint32_t (sectorId)
                << ", AntennaID=" << uint32_t (antennaId) << std::endl;
      if ((westWifiMac->GetAddress () == staWifiMac->GetAddress ()) &&
          ((southWifiMac->GetAddress () == address) || (eastWifiMac->GetAddress () == address)))
        {
          stationsTrained++;
        }
      if ((stationsTrained == 2) & !scheduledStaticPeriods)
        {
          std::cout << "West DMG STA " << staWifiMac->GetAddress () << " completed SLS phase with South and East DMG STAs " << std::endl;
          std::cout << "Schedule Static Periods" << std::endl;
          scheduledStaticPeriods = true;
          /* Create Airtime Allocation Requests */
          DmgTspecElement element;
          element = CreateTimeAllocationRequest (ISOCHRONOUS, eastWifiMac->GetAssociationID (), false, 4, spDuration);
          westWifiMac->CreateAllocation (element);
          element = CreateTimeAllocationRequest (ISOCHRONOUS, southWifiMac->GetAssociationID (), false, 4, spDuration);
          westWifiMac->CreateAllocation (element);
        }
    }
}

void
ADDTSReceived (Ptr<DmgApWifiMac> apWifiMac, Mac48Address address, DmgTspecElement element)
{
  DmgAllocationInfo info = element.GetDmgAllocationInfo ();
  StatusCode code;
  uint8_t srcAid = apWifiMac->GetStationAid (address);
  /* Decompose Allocation */
  if (info.GetAllocationFormat () == ISOCHRONOUS)
    {
      if (element.GetAllocationPeriod () >= 1)
        {
          if (element.IsAllocationPeriodMultipleBI ())
            {
              /******* Allocation Period = BI * n *******/

            }
          else
            {
              /******* Allocation Period = BI / n *******/

              /* Check current allocations for empty slots */
              AllocationFieldList allocationList = apWifiMac->GetAllocationList ();
              Time allocationPeriod = apWifiMac->GetBeaconInterval () / element.GetAllocationPeriod ();
              /**
               * For the time being, we assume all the stations request the same block size
               * so the AP can allocate these blocks one behind the other.
               */
              apWifiMac->AddAllocationPeriod (info.GetAllocationID (), SERVICE_PERIOD_ALLOCATION, info.IsPseudoStatic (),
                                              srcAid, info.GetDestinationAid (),
                                              uint32_t (spDuration * allocationList.size ()), // Start Time
                                              element.GetMinimumDuration (),  // Block Duration (SP Duration that makes up the allocation)
                                              allocationPeriod.GetMicroSeconds (), element.GetAllocationPeriod ());

              /* Set status code */
              code.SetSuccess ();
            }
        }
    }
  else if (info.GetAllocationFormat () == ASYNCHRONOUS)
    {
      /******* Allocation Period = BI * n *******/

    }

  /* The PCP/AP shall transmit the ADDTS Response frame to the STAs identified as source and destination AID of
   * the DMG TSPEC contained in the ADDTS Request frame if the ADDTS Request it is sent by a non-PCP/ non-AP STA. */
  TsDelayElement delayElem;
  Mac48Address destAddress = apWifiMac->GetStationAddress (info.GetDestinationAid ());
  apWifiMac->SendDmgAddTsResponse (address, code, delayElem, element);
  if (code.GetStatusCodeValue () == STATUS_CODE_SUCCESS)
    {
      apWifiMac->SendDmgAddTsResponse (destAddress, code, delayElem, element);
    }
}

void
DeleteAllocation (Ptr<DmgStaWifiMac> wifiMac, uint8_t id, uint8_t destAid)
{
  DmgAllocationInfo info;
  info.SetAllocationID (id);
  info.SetDestinationAid (destAid);
  wifiMac->DeleteAllocation (0, info);
}

int
main (int argc, char *argv[])
{
  uint32_t payloadSize = 1472;                  /* Transport Layer Payload size in bytes. */
  string dataRate = "300Mbps";                  /* Application Layer Data Rate. */
  uint32_t msduAggregationSize = 7935;          /* The maximum aggregation size for A-MSDU in Bytes. */
  uint32_t queueSize = 10000;                   /* Wifi Mac Queue Size. */
  string phyMode = "DMG_MCS24";                 /* Type of the Physical Layer. */
  bool verbose = false;                         /* Print Logging Information. */
  double simulationTime = 10;                   /* Simulation time in seconds. */
  bool pcapTracing = false;                     /* PCAP Tracing is enabled or not. */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("dataRate", "Payload size in bytes", dataRate);
  cmd.AddValue ("msduAggregation", "The maximum aggregation size for A-MSDU in Bytes", msduAggregationSize);
  cmd.AddValue ("queueSize", "The size of the Wifi Mac Queue", queueSize);
  cmd.AddValue ("duration", "The duration of service period in MicroSeconds", spDuration);
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
      LogComponentEnable ("EvaluateTrafficStreamAllocation", LOG_LEVEL_ALL);
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
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "ControlMode", StringValue (phyMode),
                                                                "DataMode", StringValue (phyMode));
  /* Give all nodes directional antenna */
  wifiPhy.EnableAntenna (true, true);
  wifiPhy.SetAntenna ("ns3::Directional60GhzAntenna",
                      "Sectors", UintegerValue (8),
                      "Antennas", UintegerValue (1));

  /* Make four nodes and set them up with the phy and the mac */
  NodeContainer wifiNodes;
  wifiNodes.Create (4);
  Ptr<Node> apNode = wifiNodes.Get (0);
  Ptr<Node> westNode = wifiNodes.Get (1);
  Ptr<Node> southNode = wifiNodes.Get (2);
  Ptr<Node> eastNode = wifiNodes.Get (3);

  /* Add a DMG upper mac */
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

  /* Install DMG PCP/AP Node */
  Ssid ssid = Ssid ("TrafficStream");
  wifiMac.SetType ("ns3::DmgApWifiMac",
                   "Ssid", SsidValue(ssid),
                   "BE_MaxAmpduSize", UintegerValue (0),
                   "BE_MaxAmsduSize", UintegerValue (msduAggregationSize),
                   "SSSlotsPerABFT", UintegerValue (8), "SSFramesPerSlot", UintegerValue (8),
                   "BeaconInterval", TimeValue (MicroSeconds (102400)),
                   "BeaconTransmissionInterval", TimeValue (MicroSeconds (600)),
                   "ATIPresent", BooleanValue (false));

  NetDeviceContainer apDevice;
  apDevice = wifi.Install (wifiPhy, wifiMac, apNode);

  /* Install DMG STA Nodes */
  wifiMac.SetType ("ns3::DmgStaWifiMac",
                   "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false),
                   "BE_MaxAmpduSize", UintegerValue (0),
                   "BE_MaxAmsduSize", UintegerValue (msduAggregationSize));

  staDevices = wifi.Install (wifiPhy, wifiMac, NodeContainer (westNode, southNode, eastNode));

  /* Setting mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, +1.0, 0.0));   /* PCP/AP */
  positionAlloc->Add (Vector (-1.0, 0.0, 0.0));   /* West STA */
  positionAlloc->Add (Vector (0.0, -1.0, 0.0));   /* South STA */
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

  /* Install Simple UDP Server on both south and east Node */
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9999));
  ApplicationContainer sinks = sinkHelper.Install (NodeContainer (eastNode, southNode));
  sink1 = StaticCast<PacketSink> (sinks.Get (0));
  sink2 = StaticCast<PacketSink> (sinks.Get (1));

  /* Install Simple UDP Transmiter on the West Node (Transmit to the East Node) */
  ApplicationContainer srcApp;
  OnOffHelper src ("ns3::UdpSocketFactory", InetSocketAddress (staInterfaces.GetAddress (2), 9999));
  src.SetAttribute ("MaxBytes", UintegerValue (0));
  src.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  src.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
  src.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  src.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  srcApp = src.Install (westNode);
  srcApp.Start (Seconds (3.0));

  /* Install Simple UDP Transmiter on the West Node (Transmit to the South Node) */
  src.SetAttribute ("Remote", AddressValue (InetSocketAddress (staInterfaces.GetAddress (1), 9999)));
  srcApp = src.Install (westNode);
  srcApp.Start (Seconds (3.0));

  /* Set Maximum number of packets in WifiMacQueue */
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/DcaTxop/Queue/MaxPackets", UintegerValue (queueSize));
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxPackets", UintegerValue (queueSize));
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::DmgWifiMac/SPQueue/MaxPackets", UintegerValue (queueSize));

  /* Schedule Throughput Calulcations */
  Simulator::Schedule (Seconds (3.1), &CalculateThroughput);

  /* Stations */
  apWifiNetDevice = StaticCast<WifiNetDevice> (apDevice.Get (0));
  westWifiNetDevice = StaticCast<WifiNetDevice> (staDevices.Get (0));
  southWifiNetDevice = StaticCast<WifiNetDevice> (staDevices.Get (1));
  eastWifiNetDevice = StaticCast<WifiNetDevice> (staDevices.Get (2));

  apWifiMac = StaticCast<DmgApWifiMac> (apWifiNetDevice->GetMac ());
  westWifiMac = StaticCast<DmgStaWifiMac> (westWifiNetDevice->GetMac ());
  southWifiMac = StaticCast<DmgStaWifiMac> (southWifiNetDevice->GetMac ());
  eastWifiMac = StaticCast<DmgStaWifiMac> (eastWifiNetDevice->GetMac ());

  /** Connect Traces **/
  westWifiMac->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, westWifiMac));
  southWifiMac->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, southWifiMac));
  eastWifiMac->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, eastWifiMac));

  westWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, westWifiMac));
  southWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, southWifiMac));
  eastWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, eastWifiMac));

  apWifiMac->TraceConnectWithoutContext ("ADDTSReceived", MakeBoundCallback (&ADDTSReceived, apWifiMac));

  /* Enable Traces */
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
      wifiPhy.EnablePcap ("Traces/AccessPoint", apDevice, false);
      wifiPhy.EnablePcap ("Traces/WestNode", staDevices.Get (0), false);
      wifiPhy.EnablePcap ("Traces/SouthNode", staDevices.Get (1), false);
      wifiPhy.EnablePcap ("Traces/EastNode", staDevices.Get (2), false);
    }

  /* Print Output*/
  std::cout << "Time(s)" << '\t' << "SP1" << '\t'<< "SP2" << std::endl;

  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();
  Simulator::Destroy ();

  /* Print Results Summary */
  std::cout << "Total number of packets received during each service period:" << std::endl;
  std::cout << "A1 = " << sink1->GetTotalReceivedPackets () << std::endl;
  std::cout << "A2 = " << sink2->GetTotalReceivedPackets () << std::endl;

  std::cout << "Total throughput during each service period:" << std::endl;
  std::cout << "A1 = " << westEastAverageThroughput/((simulationTime - 3) * 10) << std::endl;
  std::cout << "A2 = " << westSouthAverageThroughput/((simulationTime - 3) * 10)<< std::endl;

  return 0;
}
