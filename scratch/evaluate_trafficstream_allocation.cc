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
#include <iomanip>

/**
 * Simulation Objective:
 * Evaluate allocation of static service periods using traffic stream in the IEEE 802.11ad standard.
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
 * to perform SLS (TXSS) between all the stations. Once West DMG STA has completed TXSS phase with East and
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
 * 1. PCAP traces for each station. From the PCAP files, we can see that data transmission takes place during its SP.
 * In addition, we can notice the announcement of two static allocation periods inside each DMG Beacon.
 * 2. Summary for the total number of received packets and the total throughput during each service period.
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
uint8_t assoicatedStations = 0;           /* Total number of assoicated stations with the AP. */
uint8_t stationsTrained = 0;              /* Number of BF trained stations. */
bool scheduledStaticPeriods = false;      /* Flag to indicate whether we scheduled Static Service Periods or not. */

/*** Service Period ***/
uint16_t spDuration = 3200;               /* The duration of the allocated service period block in MicroSeconds. */

void
CalculateThroughput (void)
{
  double thr1, thr2;
  string duration = to_string_with_precision<double> (Simulator::Now ().GetSeconds () - 0.1, 1)
                  + " - " + to_string_with_precision<double> (Simulator::Now ().GetSeconds (), 1);
  thr1 = CalculateSingleStreamThroughput (sink1, westEastLastTotalRx, westEastAverageThroughput);
  thr2 = CalculateSingleStreamThroughput (sink2, westSouthTotalRx, westSouthAverageThroughput);
  std::cout << std::left << std::setw (12) << duration
            << std::left << std::setw (12) << thr1
            << std::left << std::setw (12) << thr2
            << std::endl;
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

void
StationAssoicated (Ptr<DmgStaWifiMac> staWifiMac, Mac48Address address, uint16_t aid)
{
  std::cout << "DMG STA " << staWifiMac->GetAddress () << " associated with DMG PCP/AP " << address << std::endl;
  std::cout << "Association ID (AID) = " << aid << std::endl;
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

      /* For simplicity we assume that each station is aware of the capabilities of the peer station */
      /* Otherwise, we have to request the capabilities of the peer station. */
      westWifiMac->StorePeerDmgCapabilities (eastWifiMac);
      westWifiMac->StorePeerDmgCapabilities (southWifiMac);
      eastWifiMac->StorePeerDmgCapabilities (westWifiMac);
      eastWifiMac->StorePeerDmgCapabilities (southWifiMac);
      southWifiMac->StorePeerDmgCapabilities (westWifiMac);
      southWifiMac->StorePeerDmgCapabilities (eastWifiMac);

      /* Schedule Beamforming Training SP */
      uint32_t allocationStart = 0;
      allocationStart = apWifiMac->AllocateBeamformingServicePeriod (westWifiMac->GetAssociationID (),
                                                                     eastWifiMac->GetAssociationID (), allocationStart, true);
      allocationStart = apWifiMac->AllocateBeamformingServicePeriod (westWifiMac->GetAssociationID (),
                                                                     southWifiMac->GetAssociationID (), allocationStart, true);
      allocationStart = apWifiMac->AllocateBeamformingServicePeriod (southWifiMac->GetAssociationID (),
                                                                     eastWifiMac->GetAssociationID (), allocationStart, true);
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
SLSCompleted (Ptr<DmgWifiMac> staWifiMac, SlsCompletionAttrbitutes attributes)
{
  if (attributes.accessPeriod == CHANNEL_ACCESS_DTI)
    {
      std::cout << "DMG STA " << staWifiMac->GetAddress ()
                << " completed SLS phase with DMG STA " << attributes.peerStation << std::endl;
      std::cout << "The best antenna configuration is AntennaID=" << uint16_t (attributes.antennaID)
                << ", SectorID=" << uint16_t (attributes.sectorID) << std::endl;
      if ((westWifiMac->GetAddress () == staWifiMac->GetAddress ()) &&
          ((southWifiMac->GetAddress () == attributes.peerStation) || (eastWifiMac->GetAddress () == attributes.peerStation)))
        {
          stationsTrained++;
        }
      if ((stationsTrained == 2) & !scheduledStaticPeriods)
        {
          std::cout << "West DMG STA " << staWifiMac->GetAddress ()
                    << " completed SLS phase with South and East DMG STAs " << std::endl;
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
  uint32_t payloadSize = 1472;                    /* Transport Layer Payload size in bytes. */
  string dataRate = "300Mbps";                    /* Application Layer Data Rate. */
  string msduAggSize = "max";                     /* The maximum aggregation size for A-MSDU in Bytes. */
  string mpduAggSize = "0";                       /* The maximum aggregation size for A-MPDU in Bytes. */
  string queueSize = "4000p";                     /* Wifi MAC Queue Size. */
  string phyMode = "DMG_MCS12";                   /* Type of the Physical Layer. */
  bool verbose = false;                           /* Print Logging Information. */
  double simulationTime = 10;                     /* Simulation time in seconds. */
  bool pcapTracing = false;                       /* PCAP Tracing is enabled or not. */
  uint32_t snapshotLength = std::numeric_limits<uint32_t>::max ();       /* The maximum PCAP Snapshot Length */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("dataRate", "Payload size in bytes", dataRate);
  cmd.AddValue ("msduAggSize", "The maximum aggregation size for A-MSDU in Bytes", msduAggSize);
  cmd.AddValue ("queueSize", "The size of the Wifi Mac Queue", queueSize);
  cmd.AddValue ("duration", "The duration of service period in MicroSeconds", spDuration);
  cmd.AddValue ("phyMode", "802.11ad PHY Mode", phyMode);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable PCAP Tracing", pcapTracing);
  cmd.AddValue ("snapshotLength", "The maximum PCAP Snapshot Length", snapshotLength);
  cmd.Parse (argc, argv);

  /* Validate A-MSDU and A-MPDU values */
  ValidateFrameAggregationAttributes (msduAggSize, mpduAggSize);
  /* Configure RTS/CTS and Fragmentation */
  ConfigureRtsCtsAndFragmenatation ();
  /* Wifi MAC Queue Parameters */
  ChangeQueueSize (queueSize);

  /**** WifiHelper is a meta-helper: it helps creates helpers ****/
  DmgWifiHelper wifi;

  /* Basic setup */
  wifi.SetStandard (WIFI_PHY_STANDARD_80211ad);

  /* Turn on logging */
  if (verbose)
    {
      wifi.EnableLogComponents ();
      LogComponentEnable ("EvaluateTrafficStreamAllocation", LOG_LEVEL_ALL);
    }

  /**** Set up Channel ****/
  DmgWifiChannelHelper wifiChannel ;
  /* Simple propagation delay model */
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  /* Friis model with standard-specific wavelength */
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (60.48e9));

  /**** SETUP ALL NODES ****/
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
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue (phyMode));

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
                   "BE_MaxAmpduSize", StringValue (mpduAggSize),
                   "BE_MaxAmsduSize", StringValue (msduAggSize),
                   "SSSlotsPerABFT", UintegerValue (8), "SSFramesPerSlot", UintegerValue (8),
                   "BeaconInterval", TimeValue (MicroSeconds (102400)));

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
                   "BE_MaxAmpduSize", StringValue (mpduAggSize),
                   "BE_MaxAmsduSize", StringValue (msduAggSize));

  staDevices = wifi.Install (wifiPhy, wifiMac, NodeContainer (westNode, southNode, eastNode));

  /* Setting mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, +1.0, 0.0));   /* DMG PCP/AP */
  positionAlloc->Add (Vector (-1.0, 0.0, 0.0));   /* DMG STA West */
  positionAlloc->Add (Vector (0.0, -1.0, 0.0));   /* DMG STA South */
  positionAlloc->Add (Vector (+1.0, 0.0, 0.0));   /* DMG STA East */

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
  src.SetAttribute ("MaxPackets", UintegerValue (0));
  src.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  src.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
  src.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  src.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  srcApp = src.Install (westNode);
  srcApp.Start (Seconds (3.0));
  srcApp.Stop (Seconds (simulationTime));

  /* Install Simple UDP Transmiter on the West Node (Transmit to the South Node) */
  src.SetAttribute ("Remote", AddressValue (InetSocketAddress (staInterfaces.GetAddress (1), 9999)));
  srcApp = src.Install (westNode);
  srcApp.Start (Seconds (3.0));
  srcApp.Stop (Seconds (simulationTime));

  /* Schedule Throughput Calulcations */
  Simulator::Schedule (Seconds (3.1), &CalculateThroughput);

  /* Connect Traces */
  apWifiNetDevice = StaticCast<WifiNetDevice> (apDevice.Get (0));
  westWifiNetDevice = StaticCast<WifiNetDevice> (staDevices.Get (0));
  southWifiNetDevice = StaticCast<WifiNetDevice> (staDevices.Get (1));
  eastWifiNetDevice = StaticCast<WifiNetDevice> (staDevices.Get (2));

  apWifiMac = StaticCast<DmgApWifiMac> (apWifiNetDevice->GetMac ());
  westWifiMac = StaticCast<DmgStaWifiMac> (westWifiNetDevice->GetMac ());
  southWifiMac = StaticCast<DmgStaWifiMac> (southWifiNetDevice->GetMac ());
  eastWifiMac = StaticCast<DmgStaWifiMac> (eastWifiNetDevice->GetMac ());

  /* Association Traces */
  westWifiMac->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, westWifiMac));
  southWifiMac->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, southWifiMac));
  eastWifiMac->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, eastWifiMac));

  /* Beamforming Training Traces */
  westWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, westWifiMac));
  southWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, southWifiMac));
  eastWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, eastWifiMac));

  apWifiMac->TraceConnectWithoutContext ("ADDTSReceived", MakeBoundCallback (&ADDTSReceived, apWifiMac));

  /* Enable Traces */
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
      wifiPhy.SetSnapshotLength (snapshotLength);
      wifiPhy.EnablePcap ("Traces/AccessPoint", apDevice, false);
      wifiPhy.EnablePcap ("Traces/WestNode", staDevices.Get (0), false);
      wifiPhy.EnablePcap ("Traces/SouthNode", staDevices.Get (1), false);
      wifiPhy.EnablePcap ("Traces/EastNode", staDevices.Get (2), false);
    }

  /* Print Output*/
  std::cout << std::left << std::setw (12) << "Time [s]"
            << std::left << std::setw (12) << "SP1"
            << std::left << std::setw (12) << "SP2"
            << std::endl;

  /* Install FlowMonitor on all nodes */
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  Simulator::Stop (Seconds (simulationTime + 0.101));
  Simulator::Run ();
  Simulator::Destroy ();

  /* Print per flow statistics */
  PrintFlowMonitorStatistics (flowmon, monitor, simulationTime - 1);

  /* Print Results Summary */
  std::cout << "Total number of packets received during each service period:" << std::endl;
  std::cout << "SP1 = " << sink1->GetTotalReceivedPackets () << std::endl;
  std::cout << "SP2 = " << sink2->GetTotalReceivedPackets () << std::endl;

  std::cout << "Total throughput [Mbps] during each service period allocation:" << std::endl;
  std::cout << "SP1 = " << westEastAverageThroughput/((simulationTime - 3) * 10) << std::endl;
  std::cout << "SP2 = " << westSouthAverageThroughput/((simulationTime - 3) * 10) << std::endl;

  return 0;
}
