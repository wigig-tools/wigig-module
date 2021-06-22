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
#include <complex>
#include <iomanip>
#include <string>

/**
 * Simulation Objective:
 * Evaluate the maximum achievable throughput for each MCS defined in IEEE 802.11ad and 11ay standards.
 *
 * Network Topology:
 * The scenario consists of two DMG AdHoc STAs.
 *
 *          DMG STA [1] (0,0)                       DMG STA [2] (+1,0)
 *
 * Simulation Description:
 * The DMG STA[2] generates a UDP traffic towards the DMG STA [1].
 *
 * Running Simulation:
 * ./waf --run "evaluate_achievable_throughput"
 *
 * To evaluate 11ay throughput, type the following command line:
 * ./waf --run "evaluate_achievable_throughput --standard=ay"
 *
 * IEEE 802.11ay supports channel bonding and to check the achievable throughput for different settings
 * it is important to set the correct channel index.
 * To check the achievable throughput with 4.32 GHz channel wdith, type the following command:
 * ./waf --run "evaluate_achievable_throughput --standard=ay --channel=9"
 *
 * Channel 9, is the first channel that supports 4.32 GHz. You need to do manual modifications to
 * the data rate of the onoffapplication to push more data.
 *
 * Simulation Output:
 * The simulation generates the following traces:
 * 1. Custom traces to report PHY and MAC layer statistics.
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateAchievableThroughput");

using namespace ns3;
using namespace std;

/**  Application Variables **/
Ptr<PacketSink> packetSink;
Ptr<OnOffApplication> onoff;

/* Network Nodes */
Ptr<Node> apWifiNode, staWifiNode;
Ptr<WifiNetDevice> staWifiNetDevice, apWifiNetDevice;
Ptr<DmgWifiPhy> staWifiPhy, apWifiPhy;
Ptr<WifiRemoteStationManager> staRemoteStationManager;

void
SetAntennaConfigurations (NetDeviceContainer &apDevice, NetDeviceContainer &staDevice)
{
  Ptr<WifiNetDevice> apWifiNetDevice = DynamicCast<WifiNetDevice> (apDevice.Get (0));
  Ptr<WifiNetDevice> staWifiNetDevice = DynamicCast<WifiNetDevice> (staDevice.Get (0));
  Ptr<DmgAdhocWifiMac> apWifiMac = DynamicCast<DmgAdhocWifiMac> (apWifiNetDevice->GetMac ());
  Ptr<DmgAdhocWifiMac> staWifiMac = DynamicCast<DmgAdhocWifiMac> (staWifiNetDevice->GetMac ());
  apWifiMac->AddAntennaConfig (1, 1, staWifiMac->GetAddress ());
  staWifiMac->AddAntennaConfig (5, 1, apWifiMac->GetAddress ());
  apWifiMac->SteerAntennaToward (staWifiMac->GetAddress ());
  staWifiMac->SteerAntennaToward (apWifiMac->GetAddress ());
}

int
main (int argc, char *argv[])
{
  uint32_t payloadSize = 1472;                  /* Application payload size in bytes. */
  string msduAggSize = "max";                   /* The maximum aggregation size for A-MSDU in Bytes. */
  string mpduAggSize = "max";                   /* The maximum aggregation size for A-MPDU in Bytes. */
  string queueSize = "4000p";                   /* Wifi MAC Queue Size. */
  string standard = "ad";                       /* The WiGig standard being utilized (ad/ay). */
  uint32_t channel = 2;                         /* WiGig channel number. */
  bool enableRts = false;                       /* Flag to indicate if RTS/CTS handskahre is enabled or disabled. */
  uint32_t rtsThreshold = 0;                    /* RTS/CTS handshare threshold. */
  double simulationTime = 1;                    /* Simulation time in seconds per MCS. */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("payloadSize", "Application payload size in bytes", payloadSize);
  cmd.AddValue ("msduAggSize", "The maximum aggregation size for A-MSDU in Bytes", msduAggSize);
  cmd.AddValue ("mpduAggSize", "The maximum aggregation size for A-MPDU in Bytes", mpduAggSize);
  cmd.AddValue ("enableRts", "Enable or disable RTS/CTS handshake", enableRts);
  cmd.AddValue ("queueSize", "The maximum size of the Wifi MAC Queue", queueSize);
  cmd.AddValue ("standard", "The WiGig standard being utilized (ad/ay)", standard);
  cmd.AddValue ("channel", "WiGig channel number", channel);
  cmd.AddValue ("simulationTime", "Simulation time in Seconds per MCS", simulationTime);
  cmd.Parse (argc, argv);

  AsciiTraceHelper ascii;       /* ASCII Helper. */
  Ptr<OutputStreamWrapper> outputFile = ascii.CreateFileStream ("AchievableThroughputTable.csv");
  *outputFile->GetStream () << "MCS,THROUGHPUT" << std::endl;

  /* Validate WiGig standard value */
  WifiPhyStandard wifiStandard = WIFI_PHY_STANDARD_80211ad;
  string wifiModePrefix;
  uint modes = 1;          /* The number of PHY modes we have. */
  uint maxMcs = 24;        /* The maximum MCS index. */
  if (standard == "ad")
    {
      wifiStandard = WIFI_PHY_STANDARD_80211ad;
      wifiModePrefix = "DMG_MCS";
    }
  else if (standard == "ay")
    {
      wifiStandard = WIFI_PHY_STANDARD_80211ay;
      modes = 2;
      wifiModePrefix = "EDMG_SC_MCS";
    }
  else
    {
      NS_FATAL_ERROR ("Wrong WiGig standard");
    }

  /* Validate A-MSDU and A-MPDU values */
  ValidateFrameAggregationAttributes (msduAggSize, mpduAggSize, wifiStandard);
  /* Configure RTS/CTS and Fragmentation */
  ConfigureRtsCtsAndFragmenatation (enableRts, rtsThreshold);
  /* Wifi MAC Queue Parameters */
  ChangeQueueSize (queueSize);

  for (uint mode = 1; mode <= modes; mode++)
    {
      if (standard == "ay" && mode == 1)
        {
          wifiModePrefix = "EDMG_SC_MCS";
          maxMcs = 21;
        }
      else if (standard == "ay" && mode == 2)
        {
          wifiModePrefix = "EDMG_OFDM_MCS";
          maxMcs = 20;
        }
      for (uint mcs = 1; mcs <= maxMcs; mcs++)
        {
          WifiMode mode = WifiMode (wifiModePrefix +  std::to_string (mcs));

          /* Get the nominal PHY rate and use it as the data of the OnOff application */
          uint64_t dataRate = mode.GetPhyRate ();

          /**** DmgWifiHelper is a meta-helper: it helps creates helpers ****/
          DmgWifiHelper wifi;
          wifi.SetStandard (wifiStandard);

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
          /* All nodes transmit at 0 dBm == 1 mW, no adaptation */
          wifiPhy.Set ("TxPowerStart", DoubleValue (0.0));
          wifiPhy.Set ("TxPowerEnd", DoubleValue (0.0));
          wifiPhy.Set ("TxPowerLevels", UintegerValue (1));
          /* Set operating channel */
          wifiPhy.Set ("ChannelNumber", UintegerValue (channel));
          /* Add support for the OFDM PHY */
          wifiPhy.Set ("SupportOfdmPhy", BooleanValue (true));
          /* Set default algorithm for all nodes to be constant rate */
          wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue (wifiModePrefix + std::to_string (mcs)));
          if (standard == "ay")
            {
              /* Set the correct error model */
              wifiPhy.SetErrorRateModel ("ns3::DmgErrorModel",
                                         "FileName", StringValue ("WigigFiles/ErrorModel/LookupTable_1458_ay.txt"));
            }

          /* Make two nodes and set them up with the PHY and the MAC */
          NodeContainer wifiNodes;
          wifiNodes.Create (2);
          apWifiNode = wifiNodes.Get (0);
          staWifiNode = wifiNodes.Get (1);

          /* Add a DMG upper mac */
          DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

          /* Set Analytical Codebook for the WiGig Devices */
          wifi.SetCodebook ("ns3::CodebookAnalytical",
                            "CodebookType", EnumValue (SIMPLE_CODEBOOK),
                            "Antennas", UintegerValue (1),
                            "Sectors", UintegerValue (8));

          /* Create Wifi Network Devices (WifiNetDevice) */
          wifiMac.SetType ("ns3::DmgAdhocWifiMac",
                           "BE_MaxAmpduSize", StringValue (mpduAggSize),
                           "BE_MaxAmsduSize", StringValue (msduAggSize),
                           "EDMGSupported", BooleanValue ((standard == "ay")));

          NetDeviceContainer apDevice;
          apDevice = wifi.Install (wifiPhy, wifiMac, apWifiNode);

          NetDeviceContainer staDevice;
          staDevice = wifi.Install (wifiPhy, wifiMac, staWifiNode);

          /* Set the best antenna configurations */
          Simulator::ScheduleNow (&SetAntennaConfigurations, apDevice, staDevice);

          /* Setting mobility model */
          MobilityHelper mobility;
          Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
          positionAlloc->Add (Vector (0.0, 0.0, 0.0));        /* WiGig PCP/AP */
          positionAlloc->Add (Vector (1.0, 0.0, 0.0));        /* WiGig STA */

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

          /* Install Simple UDP Server on the WiGig PCP/AP */
          PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9999));
          ApplicationContainer sinkApp = sinkHelper.Install (apWifiNode);
          packetSink = StaticCast<PacketSink> (sinkApp.Get (0));
          sinkApp.Start (Seconds (0.0));

          /* Install UDP Transmitter on the WiGig STA */
          ApplicationContainer srcApp;
          OnOffHelper src ("ns3::UdpSocketFactory", InetSocketAddress (apInterface.GetAddress (0), 9999));
          src.SetAttribute ("MaxPackets", UintegerValue (0));
          src.SetAttribute ("PacketSize", UintegerValue (payloadSize));
          src.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
          src.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
          src.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
          srcApp = src.Install (staWifiNode);
          srcApp.Start (Seconds (0.0));
          srcApp.Stop (Seconds (simulationTime));
          onoff = StaticCast<OnOffApplication> (srcApp.Get (0));

          Simulator::Stop (Seconds (simulationTime));
          Simulator::Run ();
          Simulator::Destroy ();

          *outputFile->GetStream () << mcs << "," << packetSink->GetTotalRx () * (double) 8/1e6 << std::endl;
        }
    }

  return 0;
}
