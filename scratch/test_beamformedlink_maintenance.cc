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
 * Evaluate beamformed link maintenance procedure for allocated Service Periods.
 *
 * Network Topology:
 * The scenario consists of 2 DMG STAs (West + East) and one PCP/AP as following:
 *
 *                        DMG PCP/AP (0,1)
 *
 *
 * West DMG STA (-1,0)                      East DMG STA (1,0)
 *
 * Simulation Description:
 * Once all the stations have assoicated successfully with the DMG PCP/AP.
 * The PCP/AP allocates a service period to perform TXSS between the two
 * stations. Once West DMG STA has completed SLS TXSS with East DMG STA,
 * the DMG PCP/AP allocates one static service periods for communication
 * as following:
 *
 * SP: DMG West STA -----> DMG East STA (SP Length = 3.2ms)
 *
 * Running the Simulation:
 * ./waf --run "test_beamformedlink_maintenance"
 *
 * Output:
 * From the PCAP files, we can see that data transmission takes place during the SPs.
 * In addition, we can notice the announcement of two static service period allocations
 * inside each DMG Beacon.
 */

NS_LOG_COMPONENT_DEFINE ("TestBeamFormedLinkMaintenance");

using namespace ns3;
using namespace std;

/* Network Nodes */
NetDeviceContainer staDevices;
Ptr<WifiNetDevice> apWifiNetDevice, westWifiNetDevice,  eastWifiNetDevice;
Ptr<DmgApWifiMac> apWifiMac;
Ptr<DmgStaWifiMac> westWifiMac, eastWifiMac;
Ptr<DmgWifiPhy> westWifiPhy, eastWifiPhy;
Ptr<DmgWifiChannel> mmWaveChannel;

/*** Access Point Variables ***/
uint8_t assoicatedStations = 0;           /* Total number of assoicated stations with the DMG PCP/AP. */
uint8_t stationsTrained = 0;              /* Number of BF trained stations. */
bool scheduledStaticPeriods = false;      /* Flag to indicate whether we scheduled Static Service Periods or not. */

/*** Service Periods ***/
uint16_t spDuration = 10000;              /* The duration of the allocated service period in MicroSeconds. */

void
CalculateThroughput (Ptr<PacketSink> sink, uint64_t lastTotalRx, double averageThroughput)
{
  Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
  double cur = (sink->GetTotalRx() - lastTotalRx) * (double) 8/1e5;     /* Convert Application RX Packets to MBits. */
  std::cout << std::left << std::setw (12) << now.GetSeconds ()
            << std::left << std::setw (12) << cur << std::endl;
  lastTotalRx = sink->GetTotalRx ();
  averageThroughput += cur;
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput, sink, lastTotalRx, averageThroughput);
}

void
StationAssoicated (Ptr<DmgStaWifiMac> staWifiMac, Mac48Address address, uint16_t aid)
{
  std::cout << "DMG STA " << staWifiMac->GetAddress () << " associated with DMG PCP/AP " << address << std::endl;
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
        }
    }

  /* Check if all stations have assoicated with the DMG PCP/AP */
  if (assoicatedStations == 2)
    {
      /* For simplicity we assume that each station is aware of the capabilities of the peer station */
      /* Otherwise, we have to request the capabilities of the peer station. */
      westWifiMac->StorePeerDmgCapabilities (eastWifiMac);
      eastWifiMac->StorePeerDmgCapabilities (westWifiMac);

      std::cout << "All stations got associated with " << address << std::endl;
      /* Schedule Beamforming Training SP */
      std::cout << "Allocate Beamforming Training SP from SRC AID=" << westWifiMac->GetAssociationID ()
                << " --> DST AID=" << eastWifiMac->GetAssociationID () << std::endl;
      apWifiMac->AllocateBeamformingServicePeriod (westWifiMac->GetAssociationID (), eastWifiMac->GetAssociationID (), 0, true);
    }
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
      if (!scheduledStaticPeriods)
        {
          std::cout << "Schedule Static Periods" << std::endl;
          scheduledStaticPeriods = true;
          /* Schedule two static SPs */
          uint32_t allocationStart = 0;
          /* First Service Period */
          allocationStart = apWifiMac->AllocateSingleContiguousBlock (1, SERVICE_PERIOD_ALLOCATION, true,
                                                                      westWifiMac->GetAssociationID (),
                                                                      eastWifiMac->GetAssociationID (), allocationStart, spDuration);

          /* Second Service Period */
          allocationStart = allocationStart + MilliSeconds (2).GetMicroSeconds ();
          allocationStart = apWifiMac->AllocateSingleContiguousBlock (1, SERVICE_PERIOD_ALLOCATION, true,
                                                                      westWifiMac->GetAssociationID (),
                                                                      eastWifiMac->GetAssociationID (), allocationStart, spDuration);

        }
    }
}

void
StartBeamformingServicePeriod (Time btDuration)
{
  westWifiMac->StartBeamformingTraining (eastWifiMac->GetAssociationID (), eastWifiMac->GetAddress (), true, true, true, btDuration);
}

void
BeamLinkMaintenanceTimerStateChanged (Ptr<DmgStaWifiMac> wifiMac, Ptr<OutputStreamWrapper> stream,
                                      BEAM_LINK_MAINTENANCE_TIMER_STATE state,
                                      uint8_t aid, Mac48Address address, Time timeLeft)
{
  *stream->GetStream () << Simulator::Now ().GetNanoSeconds ()
                        << "," << state << "," << uint16_t (aid)
                        << "," << timeLeft.GetMicroSeconds () << std::endl;
  if (state == BEAM_LINK_MAINTENANCE_TIMER_EXPIRES)
    {
      std::cout << "BeamLink Maintenance Timer Expired for " << wifiMac->GetAddress () << " with " << address << std::endl;
//      std::cout << "Time left in the allocated service period = " << wifiMac->GetRemainingAllocationTime ().GetMicroSeconds ()<< " MicroSeconds" << std::endl;
      /* Take decision whether to use the rest of the service period for Beamforming training */
    //  Time btDuration = wifiMac->CalculateBeamformingTrainingDuration (wifiMac->GetNumberOfSectors (), eastWifiMac->GetNumberOfSectors ());
    //  Time txEndTime = westWifiPhy->GetLastTxDuration () + MicroSeconds (10); /* 10 US as a protection period */
    //  timeLeft -= txEndTime;
    //  if (timeLeft < btDuration)
    //    {
    //      uint32_t startTime = 0;
    //      std::cout << "We do not have enough time in the current SP, so schedule new SP for beamforming training" << std::endl;
    //      startTime = apWifiMac->AllocateBeamformingServicePeriod (wifiMac->GetAssociationID (), aid, startTime, true);
    //      apWifiMac->ModifyAllocation (1, wifiMac->GetAssociationID (), aid, startTime, spDuration);
    //    }
    //  else
    //    {
    //      std::cout << "We have enough time in the remaining period of the current SP allocation" << std::endl;
    //      /* Terminate current Service Period */
    //      wifiMac->EndServicePeriod ();
    //      eastWifiMac->EndServicePeriod ();
    //      Simulator::Schedule (txEndTime, &StartBeamformingServicePeriod, btDuration);
    //    }
    }
}

/************* Functions to intrduce blockage in a communication link *********************/

bool induceBlockage = false;              /* Flag to indicate if we want to block a link or not */
double m_blockageValue = -100;             /* Link loss due to blockage in dBm */

/**
 * Insert Blockage
 * \return The actual value of the blockage we introduce in the simulator.
 */
double
DoInsertBlockage (void)
{
  return m_blockageValue;
}

/**
 * Insert Blockage on a certain path from Src -> Destination.
 * \param channel The channel to insert the blockage in.
 * \param srcWifiPhy The source WifiPhy.
 * \param dstWifiPhy The destination WifiPhy.
 */
void
BlockLink (Ptr<DmgWifiChannel> channel, Ptr<DmgWifiPhy> srcWifiPhy, Ptr<DmgWifiPhy> dstWifiPhy)
{
  std::cout << "Blockage Inserted at " << Simulator::Now () << std::endl;
  induceBlockage = true;
  channel->AddBlockage (&DoInsertBlockage, srcWifiPhy, dstWifiPhy);
}

void
InduceBlockage (void)
{
  induceBlockage = true;
}

void
ServicePeriodStarted (Mac48Address source, Mac48Address destination)
{
  if (induceBlockage)
    {
      std::cout << "Service Period for which we induce link blockage has started at " << Simulator::Now () << std::endl;
      Simulator::Schedule (MilliSeconds (1), &BlockLink, mmWaveChannel, westWifiPhy, eastWifiPhy);
    }
}

void
ServicePeriodEnded (Mac48Address source, Mac48Address destination)
{
  if (induceBlockage)
    {
      std::cout << "Service Period for which we induced link blockage has ended at " << Simulator::Now () << std::endl;
      induceBlockage = false;
      mmWaveChannel->RemoveBlockage ();
    }
}

int
main (int argc, char *argv[])
{
  uint32_t payloadSize = 1472;                  /* Transport Layer Payload size in bytes. */
  string dataRate = "100Mbps";                  /* Application Layer Data Rate. */
  string msduAggSize = "max";                   /* The maximum aggregation size for A-MSDU in Bytes. */
  string mpduAggSize = "0";                     /* The maximum aggregation size for A-MPDU in Bytes. */
  string queueSize = "4000p";                   /* Wifi MAC Queue Size. */
  uint32_t maintenanceUnit = 0;                 /* Maintenance Time Unit for the beamformed link. */
  uint32_t maintenanceValue = 10;               /* Maintenance Time Value for the beamformed link. */
  string phyMode = "DMG_MCS12";                 /* Type of the Physical Layer. */
  bool verbose = false;                         /* Print Logging Information. */
  double simulationTime = 1;                    /* Simulation time in seconds. */
  bool pcapTracing = false;                     /* PCAP Tracing is enabled or not. */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("dataRate", "Payload size in bytes", dataRate);
  cmd.AddValue ("msduAggregation", "The maximum aggregation size for A-MSDU in Bytes", msduAggSize);
  cmd.AddValue ("queueSize", "The size of the Wifi Mac Queue", queueSize);
  cmd.AddValue ("spDuration", "The duration of service period in MicroSeconds", spDuration);
  cmd.AddValue ("maintenanceTimeUnit", "The unit of beamform meaintenance time: 0 = 32 US, 1 = 2000 US", maintenanceUnit);
  cmd.AddValue ("maintenanceTimeValue", "The value of beamform meaintenance time", maintenanceValue);
  cmd.AddValue ("phyMode", "802.11ad PHY Mode", phyMode);
  cmd.AddValue ("verbose", "Turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable PCAP Tracing", pcapTracing);
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
      LogComponentEnable ("TestBeamFormedLinkMaintenance", LOG_LEVEL_ALL);
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
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue (phyMode));

  /* Make four nodes and set them up with the phy and the mac */
  NodeContainer wifiNodes;
  wifiNodes.Create (3);
  Ptr<Node> apNode = wifiNodes.Get (0);
  Ptr<Node> westNode = wifiNodes.Get (1);
  Ptr<Node> eastNode = wifiNodes.Get (2);

  /* Add a DMG upper mac */
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

  /* Install DMG PCP/AP Node */
  Ssid ssid = Ssid ("Maintenance");
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
                   "BE_MaxAmsduSize", StringValue (msduAggSize),
                   "BeamLinkMaintenanceUnit", EnumValue (maintenanceUnit),
                   "BeamLinkMaintenanceValue", UintegerValue (maintenanceValue));

  staDevices = wifi.Install (wifiPhy, wifiMac, NodeContainer (westNode, eastNode));

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

  /*** Install Applications ***/

  /* Install a simple UDP server on the east Node */
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9999));
  ApplicationContainer sinks = sinkHelper.Install (NodeContainer (westNode, eastNode));

  /** East Node Variables **/
  uint64_t eastNodeLastTotalRx = 0;
  double eastNodeAverageThroughput = 0;

  /* Install a simple UDP Transmiter on the West Node (Transmit to the East Node) */
  ApplicationContainer container;
  OnOffHelper onoff ("ns3::UdpSocketFactory", InetSocketAddress (staInterfaces.GetAddress (1), 9999));
  onoff.SetAttribute ("MaxPackets", UintegerValue (0));
  onoff.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
  onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  onoff.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  container = onoff.Install (westNode);
  container.Start (Seconds (0.0));
  container.Stop (Seconds (simulationTime));

  /* Schedule Throughput Calulcations */
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput, StaticCast<PacketSink> (sinks.Get (1)),
                       eastNodeLastTotalRx, eastNodeAverageThroughput);

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

  mmWaveChannel = StaticCast<DmgWifiChannel> (westWifiNetDevice->GetChannel ());
  westWifiPhy = StaticCast<DmgWifiPhy> (westWifiNetDevice->GetPhy ());
  eastWifiPhy = StaticCast<DmgWifiPhy> (eastWifiNetDevice->GetPhy ());

  /** Connect Traces **/
  westWifiMac->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, westWifiMac));
  eastWifiMac->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, eastWifiMac));
  westWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, westWifiMac));
  eastWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, eastWifiMac));
  westWifiMac->TraceConnectWithoutContext ("ServicePeriodStarted", MakeCallback (&ServicePeriodStarted));
  westWifiMac->TraceConnectWithoutContext ("ServicePeriodEnded", MakeCallback (&ServicePeriodEnded));

  /* Create Stream for Timer States Traces */
  AsciiTraceHelper ascii;       /* ASCII Helper. */
  Ptr<OutputStreamWrapper> outputTimerTraces = ascii.CreateFileStream ("BeamLinkMaintenanceTimerTraces.csv");
  *outputTimerTraces->GetStream () << "TIME,STATE,AID,TIMELEFT," << std::endl;
  westWifiMac->TraceConnectWithoutContext ("BeamLinkMaintenanceTimerStateChanged",
                                           MakeBoundCallback (&BeamLinkMaintenanceTimerStateChanged,
                                                              westWifiMac, outputTimerTraces));

  /* Schedule event to introduce link blockage */
  Simulator::Schedule (Seconds (0.5), &InduceBlockage);

  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
