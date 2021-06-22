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
 * This script is used to evaluate the performance of the IEEE 802.11ad protocol using a custom SNR to BER lookup tables
 * The tables are generated in MATLAB R2018b using the WLAN Toolbox.
 * For the time being, we assume AWGN channel. For the future, we want to try L2SM approach.
 *
 * Network Topology:
 * The scenario consists of a signle DMG STA and a single DMG PCP/AP.
 *
 *          DMG PCP/AP (0,0)                       DMG STA (+1,0)
 *
 * Simulation Description:
 * The DMG STA generates un uplink UDP traffic towards the DMG PCP/AP. The user changes the distance between the DMG STA
 * and the DMG PCP/AP to decrease/increase the received SNR.
 *
 * Running Simulation:
 * ./waf --run "evaluate_per_vs_snr_11ad"
 *
 * Simulation Output:
 * The simulation generates the following traces:
 * 1. PCAP traces for each station.
 * 2. IP Layer Statistics using Flow Monitor Module.
 * 3. Custom traces to report PHY and MAC layer statistics.
 *
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateDmgErrorModel");

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

/* Statistics */
uint64_t macTxDataFailed = 0;
double snr = 0.0;
uint64_t macRxOk = 0;
uint64_t transmittedPackets = 0;
uint64_t droppedPackets = 0;
uint64_t receivedPackets = 0;

void
MacRxOk (WifiMacType, Mac48Address, double snrValue)
{
  macRxOk++;
  snr += snrValue;
}

void
MacTxDataFailed (Mac48Address)
{
  macTxDataFailed++;
}

void
PhyTxEnd (Ptr<const Packet>)
{
  transmittedPackets++;
}

void
PhyRxDrop (Ptr<const Packet>, WifiPhyRxfailureReason)
{
  droppedPackets++;
}

void
PhyRxEnd (Ptr<const Packet>)
{
  receivedPackets++;
}

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
  string dataRate = "150Mbps";                  /* Application data rate. */
  double simulationTime = 1;                    /* Simulation time in seconds. */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("payloadSize", "Application payload size in bytes", payloadSize);
  cmd.AddValue ("dataRate", "The data rate of the OnOff application", dataRate);
  cmd.AddValue ("simulationTime", "Simulation time in Seconds", simulationTime);
  cmd.Parse (argc, argv);

  AsciiTraceHelper ascii;       /* ASCII Helper. */
  Ptr<OutputStreamWrapper> outputFile = ascii.CreateFileStream ("PER_vs_SNR_11ad.csv");
  *outputFile->GetStream () << "MCS,DIST,APP_TX_PKTS,MAC_RX_OK,MAC_TX_FAILED,PHY_TX_PKTS,PHX_RX_PKTS,PHY_RX_DROPPED,SNR" << std::endl;

  for (uint8_t mcs = 1; mcs <= 12; mcs++)
    {
      for (double distance = 0.1; distance <= 27; distance += 0.1)
        {
          /* Reset Counters */
          macTxDataFailed = 0;
          snr = 0.0;
          macRxOk = 0;
          transmittedPackets = 0;
          droppedPackets = 0;
          receivedPackets = 0;

          /* Configure RTS/CTS and Fragmentation */
          ConfigureRtsCtsAndFragmenatation ();

          /**** DmgWifiHelper is a meta-helper: it helps creates helpers ****/
          DmgWifiHelper wifi;

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
          wifiPhy.Set ("ChannelNumber", UintegerValue (2));
          /* Set default algorithm for all nodes to be constant rate */
          wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("DMG_MCS" + std::to_string (mcs)));

          /* Make two nodes and set them up with the PHY and the MAC */
          NodeContainer wifiNodes;
          wifiNodes.Create (2);
          apWifiNode = wifiNodes.Get (0);
          staWifiNode = wifiNodes.Get (1);

          /* Add a DMG upper mac */
          DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

          /* Set Analytical Codebook for the DMG Devices */
          wifi.SetCodebook ("ns3::CodebookAnalytical",
                            "CodebookType", EnumValue (SIMPLE_CODEBOOK),
                            "Antennas", UintegerValue (1),
                            "Sectors", UintegerValue (8));

          /* Create Wifi Network Devices (WifiNetDevice) */
          wifiMac.SetType ("ns3::DmgAdhocWifiMac",
                           "BE_MaxAmpduSize", UintegerValue (0), //Enable A-MPDU with the maximum size allowed by the standard.
                           "BE_MaxAmsduSize", UintegerValue (0));

          NetDeviceContainer apDevice;
          apDevice = wifi.Install (wifiPhy, wifiMac, apWifiNode);

          NetDeviceContainer staDevice;
          staDevice = wifi.Install (wifiPhy, wifiMac, staWifiNode);

          /* Set the best antenna configurations */
          Simulator::ScheduleNow (&SetAntennaConfigurations, apDevice, staDevice);

          /* Setting mobility model */
          MobilityHelper mobility;
          Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
          positionAlloc->Add (Vector (0.0, 0.0, 0.0));        /* DMG PCP/AP */
          positionAlloc->Add (Vector (distance, 0.0, 0.0));   /* DMG STA */

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

          /* Install Simple UDP Server on the DMG AP */
          PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 9999));
          ApplicationContainer sinkApp = sinkHelper.Install (apWifiNode);
          packetSink = StaticCast<PacketSink> (sinkApp.Get (0));
          sinkApp.Start (Seconds (0.0));

          /* Install UDP Transmitter on the DMG STA */
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

          /* Stations */
          apWifiNetDevice= StaticCast<WifiNetDevice> (apDevice.Get (0));
          apWifiPhy = StaticCast<DmgWifiPhy> (apWifiNetDevice->GetPhy ());
          staWifiNetDevice = StaticCast<WifiNetDevice> (staDevice.Get (0));
          staWifiPhy = StaticCast<DmgWifiPhy> (staWifiNetDevice->GetPhy ());
          staRemoteStationManager = staWifiNetDevice->GetRemoteStationManager ();

          /* Connect MAC and PHY Traces */
          apWifiPhy->TraceConnectWithoutContext ("PhyRxEnd", MakeCallback (&PhyRxEnd));
          apWifiPhy->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&PhyRxDrop));
          staWifiPhy->TraceConnectWithoutContext ("PhyTxEnd", MakeCallback (&PhyTxEnd));
          staRemoteStationManager->TraceConnectWithoutContext ("MacTxDataFailed", MakeCallback (&MacTxDataFailed));
          staRemoteStationManager->TraceConnectWithoutContext ("MacRxOK", MakeCallback (&MacRxOk));

          /* Change the maximum number of retransmission attempts for a DATA packet */
          staRemoteStationManager->SetAttribute ("MaxSlrc", UintegerValue (0));

          Simulator::Stop (Seconds (simulationTime + 0.101));
          Simulator::Run ();
          Simulator::Destroy ();

          *outputFile->GetStream () << uint16_t (mcs) << "," << distance << "," << onoff->GetTotalTxPackets ()
                                    << "," << macRxOk << "," << macTxDataFailed << ","
                                    << transmittedPackets << "," << receivedPackets << "," << droppedPackets << ","
                                    << RatioToDb (snr / double (macRxOk)) << std::endl;
        }
    }

  return 0;
}
