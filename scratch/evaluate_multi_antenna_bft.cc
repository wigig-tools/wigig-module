/*
 * Copyright (c) 2015-2020 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/wifi-module.h"
#include "common-functions.h"

/**
 * Simulation Objective:
 * Evaluate SP-based TXSS-SLS beamforming training in the DTI channel access period for
 * a multi-antenan system.
 *
 * Network Topology:
 * The scenario consists of a single DMG STA and one DMG PCP/AP placed in a large room.
 *
 * -------------------------------------------------------------------------------------------
 * |                                   DMG PCP/AP (0,+1)                                     |
 * |   .------>Y                                                                             |
 * |   |                                                                                     |
 * |   |                                                                                     |
 * |   ↓                                                                                     |
 * |   X                                                                                     |
 * |                                                                                         |
 * |                                                                                         |
 * |                                                                                         |
 * |                                                                                         |
 * |                                                                                         |
 * |                                                                                         |
 * |                                   DMG STA (0,-1)                                        |
 * -------------------------------------------------------------------------------------------
 *
 * Simulation Description:
 * Once the station has successfully assoicated with the DMG PCP/AP, the DMG PCP/AP allocates
 * a beamforming SP to perform TXSS-SLS beamforming training with the DMG PCP/AP.
 * Both DMG PCP/AP and DMG STA are equiped with two phased antenna arrays. The two arrays are
 * placed at the Y-axis and the interdistance between the two arrays is 14 cm.
 * We utilize the Q-D realziation software to generate the Q-D channel between each pair of
 * antenna arrays.
 *
 * Running the Simulation:
 * To run the script with the default parameters:
 * ./waf --run "evaluate_multi_antenna_bft"
 *
 * Simulation Output:
 * The simulation generates the following traces:
 * 1. PCAP traces for each station. From the PCAP files, we can see the allocation of beamforming service periods.
 * 2. SNR Dump for each sector.
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateMultiAntennaBFT");

using namespace ns3;
using namespace std;

typedef std::map<Mac48Address, uint32_t> MAP_MAC2ID;
typedef MAP_MAC2ID::iterator MAP_MAC2ID_I;
MAP_MAC2ID map_Mac2ID;
Ptr<QdPropagationEngine> qdPropagationEngine; /* Q-D Propagation Engine. */

/* Network Nodes */
Ptr<WifiNetDevice> apWifiNetDevice, staWifiNetDevice;
Ptr<DmgApWifiMac> apWifiMac;
Ptr<DmgStaWifiMac> staWifiMac;

/** Simulation Arguments **/
double simulationTime = 10;         /* Simulation time in seconds. */
bool printSnrInfo = true;           /* Flag to indicate whether we print SNR for the SLS phase. */
string directory = "";              /* Path to the directory where to store the results. */

/** Simulation Variables **/
AsciiTraceHelper ascii;             /* ASCII Helper. */
string runNumber;                   /* Simulation run number. */

void
StationAssoicated (Ptr<DmgWifiMac> staWifiMac, Mac48Address address, uint16_t aid)
{
  std::cout << "DMG STA " << staWifiMac->GetAddress () << " associated with DMG PCP/AP " << address
            << ", Association ID (AID) = " << aid << std::endl;
  /*** Schedule Beamforming Training SP ***/
  uint32_t startTime =0;
  Time duration = apWifiMac->ComputeBeamformingAllocationSize (staWifiMac->GetAddress (), true, true);
  apWifiMac->AllocateBeamformingServicePeriod (AID_AP, staWifiMac->GetAssociationID (),
                                               startTime, duration.GetMicroSeconds (),
                                               true, true);
}

void
StationDeassoicated (Ptr<DmgWifiMac> staWifiMac, Mac48Address address)
{
  std::cout << "DMG STA " << staWifiMac->GetAddress () << " deassociated from DMG PCP/AP " << address << std::endl;
}

void
SLSCompleted (Ptr<OutputStreamWrapper> stream, Ptr<SLS_PARAMETERS> parameters, SlsCompletionAttrbitutes attributes)
{
  uint32_t dstID = map_Mac2ID[attributes.peerStation];
  double linkSnr = parameters->wifiMac->GetWifiRemoteStationManager ()->GetLinkSnr (attributes.peerStation);
  *stream->GetStream () << parameters->srcNodeID + 1 << "," << dstID + 1 << ","
                        << qdPropagationEngine->GetCurrentTraceIndex () << ","
                        << uint16_t (attributes.sectorID) << "," << uint16_t (attributes.antennaID)  << ","
                        << parameters->wifiMac->GetTypeOfStation ()  << ","
                        << map_Mac2ID[parameters->wifiMac->GetBssid ()] + 1  << ","
                        << linkSnr << ","
                        << Simulator::Now ().GetNanoSeconds () << std::endl;
  if (printSnrInfo)
    {
      std::cout << "DMG STA " << parameters->srcNodeID + 1 << " completed SLS phase with DMG STA " << dstID + 1 << std::endl;
      if (attributes.accessPeriod == CHANNEL_ACCESS_BHI)
        {
          std::cout << "The best antenna configuration in BHI is AntennaID=";
        }
      else
        {
          std::cout << "The best antenna configuration in DTI is AntennaID=";
        }
      std::cout << uint16_t (attributes.antennaID) << ", SectorID=" << uint16_t (attributes.sectorID)
                << ", SNR Value=" << linkSnr << std::endl;
      parameters->wifiMac->PrintSnrTable ();
    }
}

int
main (int argc, char *argv[])
{
  string apCodebook = "CODEBOOK_URA_AP_28x_AzEl_Multi";   /* The name of the codebook file used by the DMG APs. */
  string staCodebook = "CODEBOOK_URA_STA_28x_AzEl_Multi"; /* The name of the codebook file used by the DMG STAs. */
  bool normalizeWeights = false;                    /* Whether we normalize the antenna weights vector or not. */
  uint16_t SSFramesPerSlot = 16;                    /* The number of SSW Slots within A-BFT Slot. */
  double rotationAngle = 90;                        /* Rotation angle of the antenna arrays around the z-axis in degrees. */
  bool frameCapture = false;                        /* Use a frame capture model. */
  double frameCaptureMargin = 10;                   /* Frame capture margin in dB. */
  string phyMode = "DMG_MCS12";                     /* Type of the Physical Layer. */
  double txPower = 10.0;                            /* The transmit power in dBm. */
  uint32_t snapShotLength = std::numeric_limits<uint32_t>::max (); /* The maximum PCAP Snapshot Length */
  bool verbose = false;                             /* Print Logging Information. */
  bool pcapTracing = false;                         /* PCAP Tracing is enabled or not. */
  string qdChannelFolder = "MultiAntennaBeamforming"; /* The name of the folder containing the Q-D Channel files. */

  /* Command line argument parser setup. */
  CommandLine cmd;
  cmd.AddValue ("printSnrInfo", "Whether we print SNR dumo for the SLS operation", printSnrInfo);
  cmd.AddValue ("apCodebook", "The name of the codebook file used by all the DMG APs", apCodebook);
  cmd.AddValue ("staCodebook", "The name of the codebook file used by all the DMG STAs", staCodebook);
  cmd.AddValue ("normalizeWeights", "Whether we normalize the antenna weights vector or not", normalizeWeights);
  cmd.AddValue ("SSFramesPerSlot", "The number of SSW Slots within A-BFT Slot", SSFramesPerSlot);
  cmd.AddValue ("rotationAngle", "Rotation angle of the antenna arrays around the z-axis in degrees", rotationAngle);
  cmd.AddValue ("frameCapture", "Whether to use a frame capture model", frameCapture);
  cmd.AddValue ("frameCaptureMargin", "Frame capture model margin in dB", frameCaptureMargin);
  cmd.AddValue ("phyMode", "The 802.11ad PHY Mode", phyMode);
  cmd.AddValue ("txPower", "The transmit power in dBm", txPower);
  cmd.AddValue ("qdChannelFolder", "The name of the folder containing the QD-Channel files", qdChannelFolder);
  cmd.AddValue ("simulationTime", "Simulation time in Seconds", simulationTime);
  cmd.AddValue ("directory", "Path to the directory where we store the results", directory);
  cmd.AddValue ("pcap", "Enable PCAP tracing", pcapTracing);
  cmd.AddValue ("snapShotLength", "The maximum PCAP snapshot length", snapShotLength);
  cmd.AddValue ("verbose", "Turn on all WifiNetDevice log components", verbose);
  cmd.Parse (argc, argv);

  /* Configure RTS/CTS and Fragmentation */
  ConfigureRtsCtsAndFragmenatation ();

  /**** DmgWifiHelper is a meta-helper: it helps creates helpers ****/
  DmgWifiHelper wifi;

  /**** Setup mmWave Q-D Wireless Channel ****/
  Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel> ();
  qdPropagationEngine = CreateObject<QdPropagationEngine> ();
  qdPropagationEngine->SetAttribute ("QDModelFolder", StringValue ("DmgFiles/QdChannel/" + qdChannelFolder + "/"));
  Ptr<QdPropagationLossModel> lossModelRaytracing = CreateObject<QdPropagationLossModel> (qdPropagationEngine);
  Ptr<QdPropagationDelayModel> propagationDelayRayTracing = CreateObject<QdPropagationDelayModel> (qdPropagationEngine);
  spectrumChannel->AddSpectrumPropagationLossModel (lossModelRaytracing);
  spectrumChannel->SetPropagationDelayModel (propagationDelayRayTracing);

  /**** Setup the physical layer ****/
  SpectrumDmgWifiPhyHelper spectrumWifiPhy = SpectrumDmgWifiPhyHelper::Default ();
  spectrumWifiPhy.SetChannel (spectrumChannel);
  /* All nodes transmit at 10 dBm == 10 mW, no adaptation */
  spectrumWifiPhy.Set ("TxPowerStart", DoubleValue (txPower));
  spectrumWifiPhy.Set ("TxPowerEnd", DoubleValue (txPower));
  spectrumWifiPhy.Set ("TxPowerLevels", UintegerValue (1));
  if (frameCapture)
    {
      /* Set frame capture model */
      spectrumWifiPhy.Set ("FrameCaptureModel", StringValue ("ns3::SimpleFrameCaptureModel"));
      Config::SetDefault ("ns3::SimpleFrameCaptureModel::Margin", DoubleValue (frameCaptureMargin));
    }
  /* Set the operational channel */
  spectrumWifiPhy.Set ("ChannelNumber", UintegerValue (2));
  /* Set default algorithm for all nodes to be constant rate */
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue (phyMode));

  /** Nodes Creation **/
  NodeContainer wifiNodes;
  wifiNodes.Create (2);
  Ptr<Node> apWifiNode, staWifiNode;
  apWifiNode = wifiNodes.Get (0);
  staWifiNode = wifiNodes.Get (1);

  /* Add High DMG MAC */
  DmgWifiMacHelper wifiMacHelper = DmgWifiMacHelper::Default ();
  NetDeviceContainer apDevices, staDevices, wigigDevices;
  std::vector<NetDeviceContainer> staDevicesGroups;

  Ssid ssid = Ssid ("BFT");
  wifiMacHelper.SetType ("ns3::DmgApWifiMac",
                         "Ssid", SsidValue (ssid),
                         "SSSlotsPerABFT", UintegerValue (8), "SSFramesPerSlot", UintegerValue (SSFramesPerSlot),
                         "BeaconInterval", TimeValue (MicroSeconds (102400)));

  /* Set Parametric Codebook for the DMG AP */
  wifi.SetCodebook ("ns3::CodebookParametric",
                    "NormalizeWeights", BooleanValue (normalizeWeights),
                    "FileName", StringValue ("DmgFiles/Codebook/" + apCodebook + ".txt"));

  /* Create WiFi Network Devices (WifiNetDevice) */
  NetDeviceContainer apDevice = wifi.Install (spectrumWifiPhy, wifiMacHelper, apWifiNode);
  apDevices.Add (apDevice);

  /* Change DMG AP's PAA Orientation */
  ChangeNodeAntennaOrientation (apDevice.Get (0), rotationAngle, 0, 0);

  wifiMacHelper.SetType ("ns3::DmgStaWifiMac",
                         "Ssid", SsidValue (ssid),
                         "ActiveProbing", BooleanValue (false));

  /* Set Parametric Codebook for all the DMG STAs */
  wifi.SetCodebook ("ns3::CodebookParametric",
                    "NormalizeWeights", BooleanValue (normalizeWeights),
                    "FileName", StringValue ("DmgFiles/Codebook/" + staCodebook + ".txt"));

  NetDeviceContainer staDevs = wifi.Install (spectrumWifiPhy, wifiMacHelper, staWifiNode);
  staDevicesGroups.push_back (staDevs);
  staDevices.Add (staDevs);

  /* Change Nodes PAAs Orientation */
  ChangeNodesAntennaOrientation (staDevs, rotationAngle, 0, 0);

  /* Map NetDevices MAC Addresses to ns-3 Nodes IDs */
  Ptr<WifiNetDevice> netDevice;
  wigigDevices.Add (apDevices);
  wigigDevices.Add (staDevices);
  for (uint32_t i = 0; i < wigigDevices.GetN (); i++)
    {
      netDevice = StaticCast<WifiNetDevice> (wigigDevices.Get (i));
      map_Mac2ID[netDevice->GetMac ()->GetAddress ()] = netDevice->GetNode ()->GetId ();
    }

  /* Setting mobility model for all the Nodes */
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiNodes);

  /* Install Internet stack */
  InternetStackHelper stack;
  stack.Install (wifiNodes);

  /** Generate unique traces per simulation run **/
  runNumber = std::to_string (RngSeedManager::GetRun ());

  /** Assign IP addresses  **/
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  address.Assign (apDevices);
  address.Assign (staDevices);

  /* We do not want any ARP packets */
  PopulateArpCache ();

  /* SLS Traces */
  Ptr<OutputStreamWrapper> outputSlsPhase = CreateSlsTraceStream (directory + "slsResults_" + runNumber);

  /* Connect DMG STA traces */
  staWifiNetDevice = StaticCast<WifiNetDevice> (staDevices.Get (0));
  staWifiMac = StaticCast<DmgStaWifiMac> (staWifiNetDevice->GetMac ());
  staWifiMac->TraceConnectWithoutContext ("Assoc", MakeBoundCallback (&StationAssoicated, staWifiMac));
  staWifiMac->TraceConnectWithoutContext ("DeAssoc", MakeBoundCallback (&StationDeassoicated, staWifiMac));

  Ptr<SLS_PARAMETERS> staParameters = Create<SLS_PARAMETERS> ();
  staParameters->srcNodeID = staWifiNetDevice->GetNode ()->GetId ();
  staParameters->wifiMac = staWifiMac;
  staWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, outputSlsPhase, staParameters));

  /* Connect DMG PCP/AP trace */
  apWifiNetDevice = StaticCast<WifiNetDevice> (apDevices.Get (0));
  apWifiMac = StaticCast<DmgApWifiMac> (apWifiNetDevice->GetMac ());
  Ptr<SLS_PARAMETERS> apParameters = Create<SLS_PARAMETERS> ();
  apParameters->srcNodeID = apWifiNetDevice->GetNode ()->GetId ();
  apParameters->wifiMac = apWifiMac;
  apWifiMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, outputSlsPhase, apParameters));

  /* Enable Traces */
  if (pcapTracing)
    {
      spectrumWifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
      spectrumWifiPhy.SetSnapshotLength (snapShotLength);
      spectrumWifiPhy.EnablePcap ("Traces/AccessPoint", apDevices, false);
      spectrumWifiPhy.EnablePcap ("Traces/STA", staDevices, false);
    }

  Simulator::Stop (Seconds (simulationTime + 0.101));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
