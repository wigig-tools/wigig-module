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
 * This script is used to evaluate IEEE 802.11ad relay operation using Link Switching Type + Full Duplex Amplify and Forward.
 * The scenario consists of 3 DMG STAs (Two REDS + 1 RDS) and one PCP/AP.
 * Note: The standard supports only unicast transmission for relay operation.
 *
 * To use this script simply type the following run command:
 * ./waf --run "evaluate_relay_operation --dataRate=5Gbps --performRls=1
 * --packetLossThreshold=100 --packetDropProbability=0.25"
 *
 * To compare operation without relay support type the following run command:
 * ./waf --run "evaluate_relay_operation --dataRate=5Gbps --performRls=0
 * --packetLossThreshold=100 --packetDropProbability=0.25"
 *
 * The simulation generates four PCAP files for each node. You can check the traces which matches exactly
 * the procedure for relay establishment.
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateRelayOperation");

using namespace ns3;
using namespace std;

Ptr<WifiNetDevice> apWifiNetDevice;
Ptr<WifiNetDevice> srcRedsNetDevice;
Ptr<WifiNetDevice> dstRedsNetDevice;
Ptr<WifiNetDevice> rdsNetDevice;

Ptr<DmgApWifiMac> apWifiMac;
Ptr<DmgStaWifiMac> srcRedsMac;
Ptr<DmgStaWifiMac> dstRedsMac;
Ptr<DmgStaWifiMac> rdsMac;

double m_packetDropProbability = 0.5;
uint32_t m_packetLossCounter = 0;         /* Packet Loss Counter */
uint32_t m_packetLossThreshold = 5;       /* Threshold to start RLS procedure*/

Time m_rlsStarted;                        /* The time we started an RLS procedure */
bool performRls = true;                   /* Flag to indicate whether we perfrom RLS procedure or not */

Ptr<UniformRandomVariable> randomVariable;

/*** Access Point Variables ***/
Ptr<PacketSink> sink;
uint64_t lastTotalRx = 0;
double averagethroughput = 0;

uint8_t assoicatedStations = 0;           /* Total number of assoicated stations with the AP */
uint8_t stationsTrained = 0;              /* Number of BF trained stations */
bool scheduledStaticPeriods = false;      /* Flag to indicate whether we scheduled Static Service Periods or not */

void
CalculateThroughput ()
{
  Time now = Simulator::Now ();                                         /* Return the simulator's virtual time. */
  double cur = (sink->GetTotalRx() - lastTotalRx) * (double) 8/1e5;     /* Convert Application RX Packets to MBits. */
  std::cout << now.GetSeconds () << '\t' << cur << std::endl;
  lastTotalRx = sink->GetTotalRx ();
  averagethroughput += cur;
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

bool
GetPacketDropValue (void)
{
  return (randomVariable->GetValue () < m_packetDropProbability);
}

/**
 * Insert Packets Dropper
 * \return
 */
void
InsertPacketDropper (Ptr<DmgWifiChannel> channel, Ptr<WifiPhy> srcWifiPhy, Ptr<WifiPhy> dstWifiPhy)
{
  std::cout << "Packet Dropper Inserted at " << Simulator::Now () << std::endl;
  channel->AddPacketDropper (&GetPacketDropValue, srcWifiPhy, dstWifiPhy);
}

void
TxFailed (Mac48Address address)
{
  if (address == dstRedsNetDevice->GetAddress ())
    {
      m_packetLossCounter++;
      if ((m_packetLossCounter == m_packetLossThreshold) && performRls)
        {
          /* Initiate RLS */
          std::cout << "Failed to receive Data ACK from " << address << " for "
                    << m_packetLossThreshold << " times, so initiate RLS procedure." << std::endl;
          Simulator::ScheduleNow (&DmgStaWifiMac::StartRlsProcedure, srcRedsMac);
          m_rlsStarted = Simulator::Now ();
        }
    }
}

void
RlsCompleted (Mac48Address address)
{
  std::cout << "RLS Procedure is completed with " << address << std::endl;
  std::cout << "RLS Procedure lasted for " << Simulator::Now () - m_rlsStarted << std::endl;
}

void
SLSCompleted (Ptr<DmgWifiMac> staWifiMac, Mac48Address address, ChannelAccessPeriod accessPeriod,
              BeamformingDirection beamformingDirection, bool isInitiatorTxss, bool isResponderTxss,
              SECTOR_ID sectorId, ANTENNA_ID antennaId)
{
  if (accessPeriod == CHANNEL_ACCESS_DTI)
    {
      std::cout << "DMG STA " << staWifiMac->GetAddress () << " completed SLS phase with DMG STA " << address << std::endl;
      if ((rdsMac->GetAddress () == staWifiMac->GetAddress ()) &&
          ((srcRedsMac->GetAddress () == address) || (dstRedsMac->GetAddress () == address)))
        {
          stationsTrained++;
          if (stationsTrained == 2)
            {
              std::cout << "The RDS completed BF with both the source REDS and destination REDS" << std::endl;
              /* Send Channel Measurement Request to the RDS */
              srcRedsMac->SendChannelMeasurementRequest (Mac48Address::ConvertFrom (rdsNetDevice->GetAddress ()), 10);
            }
        }
      else if ((srcRedsMac->GetAddress () == staWifiMac->GetAddress ()) && (dstRedsMac->GetAddress () == address))
        {
          std::cout << "The source REDS completed BF with the destination REDS" << std::endl;
          /* Send Channel Measurement Request to the destination REDS */
          srcRedsMac->SendChannelMeasurementRequest (Mac48Address::ConvertFrom (dstRedsNetDevice->GetAddress ()), 10);
        }
    }
}

void
ChannelReportReceived (Mac48Address address)
{
  if (rdsMac->GetAddress () == address)
    {
      std::cout << "Received Channel Measurement Response from the RDS" << std::endl;
      /* TxSS for the Link Between the Source REDS + the Destination REDS */
      apWifiMac->AllocateBeamformingServicePeriod (srcRedsMac->GetAssociationID (), dstRedsMac->GetAssociationID (), 0, true);
    }
  else if (dstRedsMac->GetAddress () == address)
    {
      std::cout << "Received Channel Measurement Response from the destination REDS" << std::endl;
      std::cout << "We are ready to execute RLS procedure" << std::endl;
    }
}

int
main (int argc, char *argv[])
{
  uint32_t payloadSize = 1472;                  /* Transport Layer Payload size in bytes. */
  string dataRate = "100Mbps";                  /* Application Layer Data Rate. */
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
  cmd.AddValue ("performRls", "Flag to indicate whether to perform RLS when we exceed packetLossThreshold", performRls);
  cmd.AddValue ("packetDropProbability", "The probability to drop a packet", m_packetDropProbability);
  cmd.AddValue ("packetLossThreshold", "Number of packets allowed to loss to initaite RLS procedure", m_packetLossThreshold);
  cmd.AddValue ("phyMode", "802.11ad PHY Mode", phyMode);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable PCAP Tracing", pcapTracing);
  cmd.Parse (argc, argv);

  /* Global params: no fragmentation, no RTS/CTS, fixed rate for all packets */
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::WifiMacQueue::MaxPacketNumber", UintegerValue (queueSize));

  /**** WifiHelper is a meta-helper: it helps creates helpers ****/
  DmgWifiHelper wifi;

  /* Basic setup */
  wifi.SetStandard (WIFI_PHY_STANDARD_80211ad);

  /* Turn on logging */
  if (verbose)
    {
      wifi.EnableLogComponents ();
      LogComponentEnable ("EvaluateRelayOperation", LOG_LEVEL_ALL);
    }

  /**** Set up Channel ****/
  DmgWifiChannelHelper wifiChannel ;
  /* Simple propagation delay model */
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  /* Friis model with standard-specific wavelength */
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (56.16e9));

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
  /* Sensitivity model includes implementation loss and noise figure */
  wifiPhy.Set ("RxNoiseFigure", DoubleValue (3));
  wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-79));
  wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-79 + 3));
  /* Set default algorithm for all nodes to be constant rate */
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "ControlMode", StringValue (phyMode),
                                                                "DataMode", StringValue (phyMode));

  /* Make two nodes and set them up with the phy and the mac */
  NodeContainer wifiNodes;
  wifiNodes.Create (4);
  Ptr<Node> apNode = wifiNodes.Get (0);
  Ptr<Node> rdsNode = wifiNodes.Get (1);
  Ptr<Node> srcNode = wifiNodes.Get (2);
  Ptr<Node> dstNode = wifiNodes.Get (3);

  /**** Allocate a default Adhoc Wifi MAC ****/
  /* Add a DMG upper mac */
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

  /* Install PCP/AP Node */
  Ssid ssid = Ssid ("test802.11ad");
  wifiMac.SetType("ns3::DmgApWifiMac",
                  "Ssid", SsidValue(ssid),
                  "QosSupported", BooleanValue (true), "DmgSupported", BooleanValue (true),
                  "BE_MaxAmpduSize", UintegerValue (0),
                  "BE_MaxAmsduSize", UintegerValue (262143),
                  "SSSlotsPerABFT", UintegerValue (8), "SSFramesPerSlot", UintegerValue (8),
                  "BeaconInterval", TimeValue (MicroSeconds (102400)),
                  "ATIDuration", TimeValue (MicroSeconds (1000)));

  /* Set Analytical Codebook for the DMG Devices */
  wifi.SetCodebook ("ns3::CodebookAnalytical",
                    "CodebookType", EnumValue (SIMPLE_CODEBOOK),
                    "Antennas", UintegerValue (1),
                    "Sectors", UintegerValue (8));

  NetDeviceContainer apDevice;
  apDevice = wifi.Install (wifiPhy, wifiMac, apNode);

  /* Install RDS Node */
  wifiMac.SetType ("ns3::DmgStaWifiMac",
                   "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false),
                   "QosSupported", BooleanValue (true), "DmgSupported", BooleanValue (true),
                   "BE_MaxAmpduSize", UintegerValue (0),
                   "BE_MaxAmsduSize", UintegerValue (262143),
                   "RDSActivated", BooleanValue (true), "REDSActivated", BooleanValue (false));

  NetDeviceContainer rdsDevice;
  rdsDevice = wifi.Install (wifiPhy, wifiMac, rdsNode);

  /* Install REDS Nodes */
  wifiMac.SetType ("ns3::DmgStaWifiMac",
                   "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false),
                   "QosSupported", BooleanValue (true), "DmgSupported", BooleanValue (true),
                   "BE_MaxAmpduSize", UintegerValue (0),
                   "BE_MaxAmsduSize", UintegerValue (262143),
                   "RDSActivated", BooleanValue (false), "REDSActivated", BooleanValue (true));

  NetDeviceContainer redsDevices;
  redsDevices = wifi.Install (wifiPhy, wifiMac, NodeContainer (srcNode, dstNode));

  /* Setting mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, +1.0, 0.0));   /* PCP/AP */
  positionAlloc->Add (Vector (0.0, -1.0, 0.0));   /* RDS */
  positionAlloc->Add (Vector (-1.0, 0.0, 0.0));   /* Source REDS */
  positionAlloc->Add (Vector (+1.0, 0.0, 0.0));   /* Destination REDS */

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
  Ipv4InterfaceContainer rdsInterface;
  rdsInterface = address.Assign (rdsDevice);
  Ipv4InterfaceContainer endsInterface;
  endsInterface = address.Assign (redsDevices);

  /* Populate routing table */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* We do not want any ARP packets */
  PopulateArpCache ();

  /* Install Simple UDP Server on the access point */
  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9999));
  ApplicationContainer sinkApp = sinkHelper.Install (dstNode);
  sink = StaticCast<PacketSink> (sinkApp.Get (0));
  sinkApp.Start (Seconds (0.0));

  ApplicationContainer srcApp;
  OnOffHelper src ("ns3::UdpSocketFactory", InetSocketAddress (endsInterface.GetAddress (1), 9999));
  src.SetAttribute ("MaxBytes", UintegerValue (0));
  src.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  src.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
  src.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  src.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  srcApp = src.Install (srcNode);
  srcApp.Start (Seconds (5.0));
  Simulator::Schedule (Seconds (5.1), &CalculateThroughput);

  /* Enable Traces */
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
      wifiPhy.EnablePcap ("AccessPoint", apDevice, false);
      wifiPhy.EnablePcap ("RDS", rdsDevice, false);
      wifiPhy.EnablePcap ("REDS", redsDevices, false);
    }

  /** Connect Trace Sources **/
  apWifiNetDevice = StaticCast<WifiNetDevice> (apDevice.Get (0));
  srcRedsNetDevice = StaticCast<WifiNetDevice> (redsDevices.Get (0));
  dstRedsNetDevice = StaticCast<WifiNetDevice> (redsDevices.Get (1));
  rdsNetDevice = StaticCast<WifiNetDevice> (rdsDevice.Get (0));

  apWifiMac = StaticCast<DmgApWifiMac> (apWifiNetDevice->GetMac ());
  srcRedsMac = StaticCast<DmgStaWifiMac> (srcRedsNetDevice->GetMac ());
  dstRedsMac = StaticCast<DmgStaWifiMac> (dstRedsNetDevice->GetMac ());
  rdsMac = StaticCast<DmgStaWifiMac> (rdsNetDevice->GetMac ());

  srcRedsMac->TraceConnectWithoutContext ("ChannelReportReceived", MakeCallback (&ChannelReportReceived));
  srcRedsMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, srcRedsMac));
  dstRedsMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, dstRedsMac));
  rdsMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, rdsMac));

  Ptr<WifiRemoteStationManager> remoteStation = srcRedsNetDevice->GetRemoteStationManager ();
  remoteStation->TraceConnectWithoutContext ("MacTxDataFailed", MakeCallback (&TxFailed));
  srcRedsMac->TraceConnectWithoutContext ("RlsCompleted", MakeCallback (&RlsCompleted));

  /** Schedule Events **/
  /* Request the DMG Capabilities of other DMG STAs */
  Simulator::Schedule (Seconds (1.0), &DmgStaWifiMac::RequestRelayInformation, srcRedsMac, dstRedsMac->GetAddress ());
  Simulator::Schedule (Seconds (1.1), &DmgStaWifiMac::RequestRelayInformation, rdsMac, dstRedsMac->GetAddress ());
  Simulator::Schedule (Seconds (1.2), &DmgStaWifiMac::RequestRelayInformation, srcRedsMac, rdsMac->GetAddress ());

  /* Initiate Relay Discovery Procedure */
  Simulator::Schedule (Seconds (1.4), &DmgStaWifiMac::StartRelayDiscovery, srcRedsMac, dstRedsMac->GetAddress ());

  /* UDP Client will start transmission at this point, however we will add blockage to the link */
  Ptr<DmgWifiChannel> adChannel = StaticCast<DmgWifiChannel> (srcRedsNetDevice->GetChannel ());
  Ptr<WifiPhy> srcWifiPhy = srcRedsNetDevice->GetPhy ();
  Ptr<WifiPhy> dstWifiPhy = dstRedsNetDevice->GetPhy ();

  /* Initialize Packets Dropper */
  randomVariable = CreateObject<UniformRandomVariable> ();
  randomVariable->SetAttribute ("Min", DoubleValue (0));
  randomVariable->SetAttribute ("Max", DoubleValue (1));
  Simulator::Schedule (Seconds (6), &InsertPacketDropper, adChannel, srcWifiPhy, dstWifiPhy);

  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
