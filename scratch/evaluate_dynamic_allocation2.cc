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
 * This script is used to evaluate IEEE 802.11ad dynamic allocation of service period.
 * The scenario consists of 3 DMG STAs (West + South + East) and one DMG PCP/AP as following:
 *
 * Network Topology:
 *                         DMG AP (0,1)
 *
 *
 * West DMG STA (-1,0)                      East DMG STA (1,0)
 *
 *
 *                      South DMG STA (0,-1)
 *
 * Simulation Description:
 * Once all the stations have assoicated successfully with the PCP/AP. The PCP/AP starts polling each station as much as possible
 * during beacon interval. A user registers a callback function using 'RegisterSPRequestFunction' in the DmgStaWifiMac for
 * requesting resources per station. Upon completion of Polling Period, the user has access to all the requested resources
 * and can develop his/her own resource scheduller. In the first Polling Period, the user request Beamforming training (TxSS)
 * in each of the allocated SPs with a peer station as following:
 *
 * West DMG STA <-----> East DMG STA (2ms)
 * South DMG STA <-----> West DMG STA (2ms)
 * South DMG STA <-----> East DMG STA (2ms)
 *
 * After that phase, the 3 STAs request the following SP allocations for data communication:
 *
 * West DMG STA -----> East DMG STA (32ms)
 * South DMG STA -----> East DMG STA (5ms)
 * East DMG STA -----> DMG PCP/AP (16ms)
 *
 * The sequence of the allocations in the DTI depends on the association sequence i.e. the order of the associated station.
 *
 * Running Simulation:
 * ./waf --run "evaluate_dynamic_allocation --simulationTime=10 --pcap=true"
 *
 * Output:
 * The simulation generates PCAP traces for each station.
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateDynamicAllocation");

using namespace ns3;
using namespace std;

/** West Node Allocation Variables **/
uint64_t westEastLastTotalRx = 0;
double westEastAverageThroughput = 0;
/** South Node Allocation Variables **/
uint64_t southEastTotalRx = 0;
double southEastAverageThroughput = 0;
/** East Node Allocation Variables **/
uint64_t eastApTotalRx = 0;
double eastApAverageThroughput = 0;

Ptr<PacketSink> sink1, sink2, sink3;

NetDeviceContainer staDevices;

Ptr<WifiNetDevice> apWifiNetDevice;
Ptr<WifiNetDevice> southWifiNetDevice;
Ptr<WifiNetDevice> westWifiNetDevice;
Ptr<WifiNetDevice> eastWifiNetDevice;

Ptr<DmgApWifiMac> apWifiMac;
Ptr<DmgStaWifiMac> southWifiMac;
Ptr<DmgStaWifiMac> westWifiMac;
Ptr<DmgStaWifiMac> eastWifiMac;

/*** Access Point Variables ***/
uint8_t assoicatedStations = 0;           /* Total number of assoicated stations with the AP */

/*** Stations Variables ***/
uint8_t bfTrainedStations = 0;            /* Total number of beamformed trained stations */
bool bfTrained = false;                   /* Flag to indicate if stations did the BFT stage */

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
  double thr1, thr2, thr3;
  Time now = Simulator::Now ();
  thr1 = CalculateSingleStreamThroughput (sink1, westEastLastTotalRx, westEastAverageThroughput);
  thr2 = CalculateSingleStreamThroughput (sink2, southEastTotalRx, southEastAverageThroughput);
  thr3 = CalculateSingleStreamThroughput (sink3, eastApTotalRx, eastApAverageThroughput);
  std::cout << now.GetSeconds () << '\t' << thr1 << '\t'<< thr2 << '\t'<< thr3 << std::endl;
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

void
StationAssoicated (Ptr<DmgStaWifiMac> staWifiMac, Mac48Address address)
{
  std::cout << "DMG STA " << staWifiMac->GetAddress () << " associated with DMG AP " << address << std::endl;
  std::cout << "Association ID (AID) = " << staWifiMac->GetAssociationID () << std::endl;
  assoicatedStations++;
  /* Check if all stations have assoicated with the AP */
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

      /* Temporary Solution */
      eastWifiMac->CommunicateInServicePeriod (apWifiMac->GetAddress ());

      /* Initiate Dynamic Allocation after all stations have associated successfully with the PCP/AP */
      Simulator::ScheduleNow (&DmgApWifiMac::InitiateDynamicAllocation, apWifiMac);
    }
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
      /* Temporary Solution */
      staWifiMac->CommunicateInServicePeriod (address);
    }
}

DynamicAllocationInfoField RequestAllocation (Mac48Address address, BF_Control_Field &bf)
{
  DynamicAllocationInfoField info;
  info.SetTID (AC_BE);
  info.SetAllocationType (SERVICE_PERIOD_ALLOCATION);
  if (bfTrained)
    {
      bf.SetBeamformTraining (false);
      /* Data Communication Stage */
      if (address == westWifiMac->GetAddress ())
        {
          info.SetSourceAID (westWifiMac->GetAssociationID ());
          info.SetDestinationAID (eastWifiMac->GetAssociationID ());
          info.SetAllocationDuration (32000);
        }
      else if (address == southWifiMac->GetAddress ())
        {
          info.SetSourceAID (southWifiMac->GetAssociationID ());
          info.SetDestinationAID (eastWifiMac->GetAssociationID ());
          info.SetAllocationDuration (5000);
        }
      else if (address == eastWifiMac->GetAddress ())
        {
          info.SetSourceAID (eastWifiMac->GetAssociationID ());
          info.SetDestinationAID (AID_AP);
          info.SetAllocationDuration (16000);
        }
    }
  else
    {
      /* Set beamforming control field to perform SLS */
      bf.SetBeamformTraining (true);
      bf.SetAsInitiatorTxss (true);
      bf.SetAsResponderTxss (true);
      /* Beamforming Training Stage */
      if (address == westWifiMac->GetAddress ())
        {
          info.SetSourceAID (westWifiMac->GetAssociationID ());
          info.SetDestinationAID (eastWifiMac->GetAssociationID ());
        }
      else if (address == southWifiMac->GetAddress ())
        {
          info.SetSourceAID (southWifiMac->GetAssociationID ());
          info.SetDestinationAID (westWifiMac->GetAssociationID ());
        }
      else if (address == eastWifiMac->GetAddress ())
        {
          info.SetSourceAID (southWifiMac->GetAssociationID ());
          info.SetDestinationAID (eastWifiMac->GetAssociationID ());
        }
      info.SetAllocationDuration (2000); // 2ms
      bfTrainedStations++;
      if (bfTrainedStations == 3)
        {
          bfTrained = true;
        }
    }
  return info;
}

void
PollingPeriodCompleted (Mac48Address address)
{
  /* Check the dynamic information received in all the SPRs */
  AllocationDataList list = apWifiMac->GetSprList ();

  /**
   * Here we have admission control and resource scheduller.
   */

  /* Add the allocation to the list of Dynamic Allocation */
  /* For simplicity we consider all resource requests are accepted as they are */
  for (AllocationDataListCI it = list.begin (); it != list.end (); it++)
    {
      apWifiMac->AddGrantData ((*it));
    }
}

void
GrantPeriodCompleted (Mac48Address address)
{
  Time remainingTime = apWifiMac->GetDTIRemainingTime ();
  Time ppDuration = apWifiMac->GetPollingPeriodDuration (3);
  if (ppDuration <= remainingTime)
    {
      apWifiMac->InitiatePollingPeriod (ppDuration);
    }
  else
    {
      std::cout << "No enough time to start Polling Period in this DTI" << std::endl;
    }
}

int
main (int argc, char *argv[])
{
  uint32_t payloadSize = 1472;                  /* Transport Layer Payload size in bytes. */
  string dataRate = "200Mbps";                  /* Application Layer Data Rate. */
  uint32_t queueSize = 1000;                    /* Wifi Mac Queue Size. */
  string phyMode = "DMG_MCS24";                 /* Type of the Physical Layer. */
  bool verbose = false;                         /* Print Logging Information. */
  double simulationTime = 10;                   /* Simulation time in seconds. */
  bool pcapTracing = false;                     /* PCAP Tracing is enabled or not. */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("dataRate", "Payload size in bytes", dataRate);
  cmd.AddValue ("queueSize", "The size of the Wifi Mac Queue", queueSize);
  cmd.AddValue ("phyMode", "802.11ad PHY Mode", phyMode);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable PCAP Tracing", pcapTracing);
  cmd.Parse (argc, argv);

  /* Global params: no fragmentation, no RTS/CTS, fixed rate for all packets */
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));
//  Config::SetDefault ("ns3::WifiMacQueue::MaxPacketNumber", UintegerValue (queueSize));

  /**** WifiHelper is a meta-helper: it helps creates helpers ****/
  WifiHelper wifi;

  /* Basic setup */
  wifi.SetStandard (WIFI_PHY_STANDARD_80211ad);

  /* Turn on logging */
  if (verbose)
    {
      wifi.EnableLogComponents ();
      LogComponentEnable ("EvaluateDynamicAllocation", LOG_LEVEL_ALL);
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
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "ControlMode", StringValue (phyMode),
                                                                "DataMode", StringValue (phyMode));
  /* Give all nodes steerable antenna */
  wifiPhy.EnableAntenna (true, true);
  wifiPhy.SetAntenna ("ns3::Directional60GhzAntenna",
                      "Sectors", UintegerValue (8),
                      "Antennas", UintegerValue (1));

  /* Make two nodes and set them up with the phy and the mac */
  NodeContainer wifiNodes;
  wifiNodes.Create (4);
  Ptr<Node> apNode = wifiNodes.Get (0);
  Ptr<Node> westNode = wifiNodes.Get (1);
  Ptr<Node> southNode = wifiNodes.Get (2);
  Ptr<Node> eastNode = wifiNodes.Get (3);

  /* Add a DMG upper mac */
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

  /* Install PCP/AP Node */
  Ssid ssid = Ssid ("DynamicAllocation");
  wifiMac.SetType("ns3::DmgApWifiMac",
                  "Ssid", SsidValue(ssid),
                  "BE_MaxAmpduSize", UintegerValue (0),
                  "BE_MaxAmsduSize", UintegerValue (0),
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
                   "BE_MaxAmsduSize", UintegerValue (0),
                   "StaAvailabilityElement", BooleanValue (true),
                   "PollingPhase", BooleanValue (true));

  staDevices = wifi.Install (wifiPhy, wifiMac, NodeContainer (westNode, southNode, eastNode));

  /* Setting mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, +1.0, 0.0));   /* PCP/AP */
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
  PacketSinkHelper sinkHelper1 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 5001));
  PacketSinkHelper sinkHelper2 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 5002));
  PacketSinkHelper sinkHelper3 ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 5003));
  sink1 = StaticCast<PacketSink> (sinkHelper1.Install (eastNode).Get (0));
  sink2 = StaticCast<PacketSink> (sinkHelper2.Install (eastNode).Get (0));
  sink3 = StaticCast<PacketSink> (sinkHelper3.Install (apNode).Get (0));

  /* Install Simple UDP Transmiter on the West Node (Transmit to the East Node) */
  ApplicationContainer srcApp;
  OnOffHelper src;
  src.SetAttribute ("Remote", AddressValue (InetSocketAddress (staInterfaces.GetAddress (2), 5001)));
  src.SetAttribute ("MaxBytes", UintegerValue (0));
  src.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  src.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
  src.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  src.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  srcApp = src.Install (westNode);
  srcApp.Start (Seconds (1.0));

  /* Install Simple UDP Transmiter on the South Node (Transmit to the East Node) */
  src.SetAttribute ("Remote", AddressValue (InetSocketAddress (staInterfaces.GetAddress (2), 5002)));
  srcApp = src.Install (southNode);
  srcApp.Start (Seconds (1.0));

  /* Install Simple UDP Transmiter on the East Node (Transmit to the PCP/AP) */
  src.SetAttribute ("Remote", AddressValue (InetSocketAddress (apInterface.GetAddress (0), 5003)));
  srcApp = src.Install (eastNode);
  srcApp.Start (Seconds (1.0));

  /* Schedule Throughput Calulcation */
  Simulator::Schedule (Seconds (1.1), &CalculateThroughput);

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

  /* Connect traces related to dynamic allocation */
  apWifiMac->TraceConnectWithoutContext ("PPCompleted", MakeCallback (&PollingPeriodCompleted));
  apWifiMac->TraceConnectWithoutContext ("GPCompleted", MakeCallback (&PollingPeriodCompleted));
  westWifiMac->RegisterSPRequestFunction (MakeCallback (&RequestAllocation));
  southWifiMac->RegisterSPRequestFunction (MakeCallback (&RequestAllocation));
  eastWifiMac->RegisterSPRequestFunction (MakeCallback (&RequestAllocation));

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
  std::cout << "Time(s)" << '\t' << "A1" << '\t'<< "A2" << '\t'<< "A3" << std::endl;

  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();
  Simulator::Destroy ();

  /* Print Results Summary */
  std::cout << "Total number of packets received during each channel time allocation:" << std::endl;
  std::cout << "A1 = " << sink1->GetTotalReceivedPackets () << std::endl;
  std::cout << "A2 = " << sink2->GetTotalReceivedPackets () << std::endl;
  std::cout << "A3 = " << sink3->GetTotalReceivedPackets () << std::endl;

  std::cout << "Total throughput during each channel time allocation:" << std::endl;
  std::cout << "A1 = " << westEastAverageThroughput/((simulationTime - 1) * 10) << std::endl;
  std::cout << "A2 = " << southEastAverageThroughput/((simulationTime - 1) * 10) << std::endl;
  std::cout << "A3 = " << eastApAverageThroughput/((simulationTime - 1) * 10) << std::endl;

  return 0;
}

