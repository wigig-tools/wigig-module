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
 * This script is used to evaluate IEEE 802.11ad relay operation for TCP connection using Link Switching Type working in
 * Half Duplex Decode and Forward relay mode. IEEE 802.11ad defines relay operation mode for SP protection against sudden
 * link interruptions.
 *
 * Network Topology:
 * The scenario consists of 3 DMG STAs (West STA, East STA and, one RDS) and a single PCP/AP.
 *
 *                           DMG AP (0,1)
 *
 *
 * West STA (-1.73,0)                         East STA (1.73,0)
 *
 *
 *                            RDS (0,-1)
 *
 * Simulation Description:
 * In this simulation scenario we use TCP as transport protocol. TCP requires bi-directional transmission. So we need to
 * establish forward and reverse SP allocations since the standard supports only unicast transmission for single SP allocation.
 * As a result, we create the following two SP allocations:
 *
 * SP1 for TCP Segments: West DMG STA -----> East DMG STA (8ms)
 * SP2 for TCP ACKs    : East DMG STA -----> West DMG STA (8ms)
 *
 * We swap betweeen those two SPs allocations during DTI access period up-to certain number of blocks as following:
 *
 * |-----SP1-----| |-----SP2-----| |-----SP1-----| |-----SP2-----| |-----SP1-----| |-----SP2-----|
 *
 * We add guard time between these consecutive SP allocations around 5us for protection.
 *
 * At the beginning each station requests information regarding the capabilities of all other stations. Once this is completed
 * West STA initiates Relay Discovery Procedure. During the relay discovery procedure, WEST STA performs Beamforming Training
 * with EAST STA and all the available RDSs. After WEST STA completes BF with the EAST STA it can establish service period for
 * direct communication without gonig throught the DMG PCP/AP. Once the RLS is completed, we repeat the same previous steps to
 * establish relay link from East STA to West STA. At this point, we schedule the previous SP allocations during DTI period.
 *
 * During the course of the simulation, we implicitly inform all the stations about relay switching decision. The user can enable
 * or disable relay switching per SP allocation.
 *
 * Running Simulation:
 * ./waf --run "evaluate_halfduplex_relay --simulationTime=10 --pcap=true"
 *
 * Output:
 * The simulation generates the following traces:
 * 1. PCAP traces for each station.
 * 2. ASCII traces corresponding to TCP socket information (CWND, RWND, and RTT).
 * 3. ASCII traces corresponding to Wifi MAC Queue size changes for each DMG STA.
 */

NS_LOG_COMPONENT_DEFINE ("EvaluateHalfDuplexRelay");

using namespace ns3;
using namespace std;

/** West Node Allocation Variables **/
Ptr<PacketSink> sink;
uint64_t westEastLastTotalRx = 0;
double westEastAverageThroughput = 0;

/* DMG Devices */
Ptr<WifiNetDevice> apWifiNetDevice;
Ptr<WifiNetDevice> westRedsNetDevice;
Ptr<WifiNetDevice> eastRedsNetDevice;
Ptr<WifiNetDevice> rdsNetDevice;

Ptr<DmgApWifiMac> apWifiMac;
Ptr<DmgStaWifiMac> westRedsMac;
Ptr<DmgStaWifiMac> eastRedsMac;
Ptr<DmgStaWifiMac> rdsMac;

/*** Access Point Variables ***/
uint8_t assoicatedStations = 0;                   /* Total number of assoicated stations with the AP */
uint8_t stationsTrained = 0;                      /* Number of BF trained stations */
bool scheduledStaticPeriods = false;              /* Flag to indicate whether we scheduled Static Service Periods or not */
uint32_t channelMeasurementResponses = 0;

/*** Service Period Parameters ***/
uint16_t sp1Duration = 8000;                      /* The duration of the forward SP allocation in MicroSeconds (8ms) */
uint16_t sp2Duration = 8000;                      /* The duration of the reverse SP allocation in MicroSeconds (8ms) */
uint8_t spBlocks = 3;                             /* The number of SP allocations in one DTI */
uint16_t cbapDuration = 10000;                    /* The duration of the allocated CBAP period in MicroSeconds (10ms) */

bool switchForward = true;                        /* Switch the forward link. */
bool switchReverse = false;                       /* Switch the reverse link. */

typedef enum {
  FORWARD_DIRECTION = 0,
  REVERSE_DIRECTION = 1,
} RelayDiretion;

RelayDiretion m_relayDirection = FORWARD_DIRECTION; /* The current direction of the relay link (Forward or reverse)*/

/********************************************************
 *            Custom TCP Send Application
 ********************************************************/

/**
 * This code defines an application to run during the simulation that
 * setups connections and manages sending data.
 */
class TcpSendApplication : public Application
{
public:
  /**
   * Default constructors.
   */
  TcpSendApplication ();
  virtual ~TcpSendApplication ();

  /**
   * Setup the TCP send application.
   *
   * \param socket Socket to send data to.
   * \param address Address to send data to.
   * \param packetSize Size of the packets to send.
   * \param dataRate Data rate used to determine when to send the packets.
   * \param isBulk The Application behaves as BulkSendApplication or as OnOffApplication.
   */
  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, DataRate dataRate, bool isBulk);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  /**
   * Schedule when the next packet will be sent.
   */
  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint64_t        m_packetsSent;
  uint64_t        m_totBytes;     //!< Total bytes sent so far
  bool            m_connected;    //!< True if connected
  bool            m_bulk;         //!< True if Bulk, otherwise OnOff

private:
  void ConnectionSucceeded (Ptr<Socket> socket);
  void ConnectionFailed (Ptr<Socket> socket);
  void DataSend (Ptr<Socket>, uint32_t); // for socket's SetSendCallback

};

TcpSendApplication::TcpSendApplication ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0),
    m_connected (false),
    m_bulk (true)
{
}

TcpSendApplication::~TcpSendApplication ()
{
  m_socket = 0;
}

void
TcpSendApplication::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, DataRate dataRate, bool isBulk)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_dataRate = dataRate;
  m_bulk = isBulk;
}

void
TcpSendApplication::StartApplication (void)
{
  m_running = true;
  // Make sure the socket is created
  if (m_socket)
    {
      // Fatal error if socket type is not NS3_SOCK_STREAM or NS3_SOCK_SEQPACKET
      if (m_socket->GetSocketType () != Socket::NS3_SOCK_STREAM &&
          m_socket->GetSocketType () != Socket::NS3_SOCK_SEQPACKET)
        {
          NS_FATAL_ERROR ("Using BulkSend with an incompatible socket type. "
                          "BulkSend requires SOCK_STREAM or SOCK_SEQPACKET. "
                          "In other words, use TCP instead of UDP.");
        }

      if (Inet6SocketAddress::IsMatchingType (m_peer))
        {
          m_socket->Bind6 ();
        }
      else if (InetSocketAddress::IsMatchingType (m_peer))
        {
          m_socket->Bind ();
        }

      m_socket->Connect (m_peer);
      m_socket->ShutdownRecv ();
      m_socket->SetConnectCallback (
        MakeCallback (&TcpSendApplication::ConnectionSucceeded, this),
        MakeCallback (&TcpSendApplication::ConnectionFailed, this));
      m_socket->SetSendCallback (
        MakeCallback (&TcpSendApplication::DataSend, this));
    }
  if (m_connected)
    {
      SendPacket ();
    }
}

void
TcpSendApplication::StopApplication (void)
{
  m_running = false;
  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
      m_connected = false;
    }
  else
    {
      NS_LOG_WARN ("TcpSendApplication found null socket to close in StopApplication");
    }
}

void
TcpSendApplication::SendPacket (void)
{
  NS_LOG_FUNCTION (this);
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  TimestampTag timestamp;
  timestamp.SetTimestamp (Simulator::Now ());
  packet->AddByteTag (timestamp);

  if (m_bulk)
    {
      while (true)
        {
          NS_LOG_LOGIC ("sending packet at " << Simulator::Now ());
          int actual = m_socket->Send (packet);
          if (actual > 0)
            {
              m_totBytes += actual;
            }
          // We exit this loop when actual < toSend as the send side
          // buffer is full. The "DataSent" callback will pop when
          // some buffer space has freed ip.
          if ((unsigned)actual != m_packetSize)
            {
              break;
            }
        }
    }
  else
    {
      m_socket->Send (packet);
      ScheduleTx ();
    }
}

void
TcpSendApplication::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      //cout << "packet: " << m_packetsSent << " at " << Simulator::Now () << endl;
      m_sendEvent = Simulator::Schedule (tNext, &TcpSendApplication::SendPacket, this);
    }
}

void
TcpSendApplication::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("TcpSendApplication Connection succeeded");
  m_connected = true;
  SendPacket ();
}

void
TcpSendApplication::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_LOGIC ("TcpSendApplication, Connection Failed");
}

void TcpSendApplication::DataSend (Ptr<Socket>, uint32_t)
{
  NS_LOG_FUNCTION (this);
  if (m_connected  && m_bulk)
    { // Only send new data if the connection has completed
      SendPacket ();
    }
}

/**
 * Callback method to log changes of the congestion window.
 */
void
CwndChange (Ptr<OutputStreamWrapper> file, const uint32_t oldCwnd, const uint32_t newCwnd)
{
  std::ostream *output = file->GetStream ();
  *output << Simulator::Now ().GetNanoSeconds () << "," << newCwnd << endl;
}

/**
* Callback method to log changes of the receive window.
*/
void
RwndChange (Ptr<OutputStreamWrapper> file, const uint32_t oldRwnd, const uint32_t newRwnd)
{
  std::ostream *output = file->GetStream ();
  *output << Simulator::Now ().GetNanoSeconds () << "," << newRwnd << endl;
}

/**
 * Callback method to log changes of the round trip time.
 */
void
RttChange (Ptr<OutputStreamWrapper> file, const Time oldRtt, const Time newRtt)
{
  std::ostream *output = file->GetStream ();
  *output << Simulator::Now ().GetNanoSeconds () << "," << newRtt << endl;
}

/**
 * Callback method to log changes of the TcpTxBufferSize
 */
void
BufferSizeChange (Ptr<OutputStreamWrapper> file, uint32_t oldValue, uint32_t newValue)
{
  std::ostream *output = file->GetStream ();
  *output << Simulator::Now ().GetNanoSeconds ()  << "," << newValue << endl;
}

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
  double thr = CalculateSingleStreamThroughput (sink, westEastLastTotalRx, westEastAverageThroughput);
  std::cout << Simulator::Now ().GetSeconds () << '\t' << thr << std::endl;
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

void
RlsCompleted (Ptr<DmgStaWifiMac> staWifiMac, Mac48Address address)
{
  if (staWifiMac == westRedsMac)
    {
      std::cout << "West STA: RLS Procedure is completed with RDS=" << address << " at " << Simulator::Now () << std::endl;
      std::cout << "East STA: Execute RLS procedure" << std::endl;
      m_relayDirection = REVERSE_DIRECTION;
      Simulator::ScheduleNow (&DmgStaWifiMac::StartRelayDiscovery, eastRedsMac, westRedsMac->GetAddress ());
    }
  else
    {
      uint32_t startTime = 0;
      std::cout << "East REDS: RLS Procedure is completed with RDS=" << address << " at " << Simulator::Now () << std::endl;

      /* Assetion check values */
      NS_ASSERT_MSG (((sp1Duration + sp2Duration) * (spBlocks)) < apWifiMac->GetDTIDuration (),
                     "Allocations cannot exceed DTI period");

      /* Schedule a CBAP allocation for communication between DMG STAs */
      startTime = apWifiMac->AllocateCbapPeriod (true, startTime, cbapDuration);

      /* Protection Period */
      startTime += GUARD_TIME.GetMicroSeconds ()/2;

      /* Schedule SP allocations for data communication between the source REDS and the destination REDS */
      std::cout << "Allocating static service period allocation for communication between "
                << westRedsMac->GetAddress () << " and " << eastRedsMac->GetAddress () << std::endl;
      startTime = apWifiMac->AddAllocationPeriod (1, SERVICE_PERIOD_ALLOCATION, true,
                                                  westRedsMac->GetAssociationID (), eastRedsMac->GetAssociationID (),
                                                  startTime, sp1Duration, sp2Duration, spBlocks);

      /* Protection Period */
      startTime += GUARD_TIME.GetMicroSeconds ()/2;

      std::cout << "Allocating static service period allocation for communication between "
                << eastRedsMac->GetAddress ()  << " and " << westRedsMac->GetAddress () << std::endl;

      startTime = apWifiMac->AddAllocationPeriod (2, SERVICE_PERIOD_ALLOCATION, true,
                                                  eastRedsMac->GetAssociationID (), westRedsMac->GetAssociationID (),
                                                  startTime, sp2Duration, sp1Duration, spBlocks);
    }
}

void
StartChannelMeasurements (Ptr<DmgStaWifiMac> srcRedsMac, Ptr<DmgStaWifiMac> dstRedsMac,
                          Ptr<DmgStaWifiMac> staWifiMac, Mac48Address address,
                          string srcName, string dstName)
{
  if ((rdsMac->GetAddress () == staWifiMac->GetAddress ()) &&
      ((srcRedsMac->GetAddress () == address) || (dstRedsMac->GetAddress () == address)))
    {
      stationsTrained++;
      if (stationsTrained == 2)
        {
          stationsTrained = 0;
          std::cout << "RDS: Completed BF Training with both " << srcName << " and " << dstName << std::endl;
          /* Send Channel Measurement Request from the source REDS to the RDS */
          std::cout << srcName << ": Send Channel Measurement Request to the candidate RDS" << std::endl;
          srcRedsMac->SendChannelMeasurementRequest (rdsMac->GetAddress (), 10);
        }
    }
  else if ((srcRedsMac->GetAddress () == staWifiMac->GetAddress ()) && (dstRedsMac->GetAddress () == address))
    {
      std::cout << srcName << ": Completed BF Training with " << dstName << std::endl;
      /* Send Channel Measurement Request to the destination REDS */
      std::cout << srcName << ": Send Channel Measurement Request to " << dstName << std::endl;
      srcRedsMac->SendChannelMeasurementRequest ((dstRedsMac->GetAddress ()), 10);
    }
}

void
SLSCompleted (Ptr<DmgStaWifiMac> staWifiMac, Mac48Address address,
              ChannelAccessPeriod accessPeriod, SECTOR_ID sectorId, ANTENNA_ID antennaId)
{
  if (accessPeriod == CHANNEL_ACCESS_DTI)
    {
      std::cout << "DMG STA " << staWifiMac->GetAddress () << " completed SLS phase with DMG STA " << address << std::endl;
      if (m_relayDirection == FORWARD_DIRECTION)
        {
          StartChannelMeasurements (westRedsMac, eastRedsMac, staWifiMac, address, "West STA", "East STA");
        }
      else
        {
          StartChannelMeasurements (eastRedsMac, westRedsMac, staWifiMac, address, "East STA", "West STA");
        }
    }
}

void
ProcessChannelReports (Ptr<DmgStaWifiMac> srcRedsMac, Ptr<DmgStaWifiMac> dstRedsMac, Mac48Address address,
                       string srcName, string dstName)
{
  if (address == rdsMac->GetAddress ())
    {
      std::cout << srcName << ": received Channel Measurement Response from the RDS" << std::endl;
      /* TxSS for the Link Between the Source REDS + the Destination REDS */
      apWifiMac->AllocateBeamformingServicePeriod (srcRedsMac->GetAssociationID (), dstRedsMac->GetAssociationID (), 0, true);
    }
  else if (address == dstRedsMac->GetAddress ())
    {
      std::cout << srcName << ": Received Channel Measurement Response from " << dstName << std::endl;
      std::cout << srcName << ": Execute RLS procedure" << std::endl;
      /* Initiate Relay Link Switch Procedure */
      Simulator::ScheduleNow (&DmgStaWifiMac::StartRlsProcedure, srcRedsMac);
    }
}

void
ChannelReportReceived (Mac48Address address)
{
  if (m_relayDirection == FORWARD_DIRECTION)
    {
      ProcessChannelReports (westRedsMac, eastRedsMac, address, "West STA", "East STA");
    }
  else
    {
      ProcessChannelReports (eastRedsMac, westRedsMac, address, "East STA", "West STA");
    }
}

uint8_t
SelectRelay (ChannelMeasurementInfoList rdsMeasurements, ChannelMeasurementInfoList dstRedsMeasurements, Mac48Address &rdsAddress)
{
  /* Make relay selection decision based on channel measurements */
  rdsAddress = rdsMac->GetAddress ();
  return rdsMac->GetAssociationID ();
}

void
SwitchTransmissionLink (Ptr<DmgStaWifiMac> srcRedsMac, Ptr<DmgStaWifiMac> dstRedsMac)
{
  std::cout << "Switching transmission link from the Direct Link to the Relay Link for SP Allocation:"
            << "SRC AID=" << uint (srcRedsMac->GetAssociationID ())
            << ", DST AID=" << uint (dstRedsMac->GetAssociationID ()) << std::endl;
  rdsMac->SwitchTransmissionLink (srcRedsMac->GetAssociationID (), dstRedsMac->GetAssociationID ());
  srcRedsMac->SwitchTransmissionLink (srcRedsMac->GetAssociationID (), dstRedsMac->GetAssociationID ());
  dstRedsMac->SwitchTransmissionLink (srcRedsMac->GetAssociationID (), dstRedsMac->GetAssociationID ());
}

void
TearDownRelay (Ptr<DmgStaWifiMac> srcRedsMac, Ptr<DmgStaWifiMac> dstRedsMac)
{
  std::cout << "Tearing-down Relay Link for SP Allocation:"
            << "SRC AID=" << uint (srcRedsMac->GetAssociationID ())
            << ", DST AID=" << uint (dstRedsMac->GetAssociationID ()) << std::endl;
  srcRedsMac->TeardownRelay (srcRedsMac->GetAssociationID (),
                             dstRedsMac->GetAssociationID (),
                             rdsMac->GetAssociationID ());
}

/**
 * Callback method to log changes of the bytes in WifiMacQueue
 */
void
BytesInQueueTrace (Ptr<OutputStreamWrapper> stream, uint64_t oldVal, uint64_t newVal)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << " " << newVal << std::endl;
}

int
main (int argc, char *argv[])
{
  /* Application Variables */
  string applicationType = "bulk";                  /* Type of the Tx application */
  uint32_t payloadSize = 1440;                      /* Transport Layer Payload size in bytes. */
  string dataRate = "100Mbps";                      /* Application Layer Data Rate. */
  string tcpVariant = "NewReno";                    /* TCP Variant Type. */
  uint32_t flows = 1;                               /* The number of TCP/UDP flows. */
  uint32_t tcpBufferSize = 131072;                  /* TCP Send/Receive Buffer Size. */
  /* Wifi MAC/PHY Variables */
  uint32_t msduAggregationSize = 7935;              /* The maximum aggregation size for A-MSDU in Bytes. */
  uint32_t queueSize = 1000;                        /* Wifi Mac Queue Size. */
  uint32_t queueMaxDelay = 500;                     /* The maximum allowable delay for a packet to reside in the Queue. */
  uint16_t firstPeriod = 4000;                      /* The duration of the RDS first period in MicroSeconds. */
  uint16_t secondPeriod = 4000;                     /* The duration of the RDS second period in MicroSeconds. */
  uint32_t switchTime = 4;                          /* The time we switch to the relay link in Seconds. */
  string phyMode = "DMG_MCS12";                     /* Type of the Physical Layer. */
  /* Simulation Variables */
  bool verbose = false;                             /* Print Logging Information. */
  double simulationTime = 10;                       /* Simulation time in seconds. */
  bool pcapTracing = false;                         /* PCAP Tracing is enabled or not. */
  bool asciiTraciing = false;                       /* ASCII Tracing is enabled or not. */
  std::string tracesPath = "/";                     /* The location where to save traces. */

  /** TCP Variants **/
  std::map<std::string, std::string> tcpVariants;   /* List of the tcp Variants */
  tcpVariants.insert (std::make_pair ("NewReno",       "ns3::TcpNewReno"));
  tcpVariants.insert (std::make_pair ("Hybla",         "ns3::TcpHybla"));
  tcpVariants.insert (std::make_pair ("HighSpeed",     "ns3::TcpHighSpeed"));
  tcpVariants.insert (std::make_pair ("Vegas",         "ns3::TcpVegas"));
  tcpVariants.insert (std::make_pair ("Scalable",      "ns3::TcpScalable"));
  tcpVariants.insert (std::make_pair ("Veno",          "ns3::TcpVeno"));
  tcpVariants.insert (std::make_pair ("Bic",           "ns3::TcpBic"));
  tcpVariants.insert (std::make_pair ("Westwood",      "ns3::TcpWestwood"));
  tcpVariants.insert (std::make_pair ("WestwoodPlus",  "ns3::TcpWestwoodPlus"));

  /* Command line argument parser setup. */
  CommandLine cmd;
  /* Application Variables */
  cmd.AddValue ("applicationType", "Type of the Tx Application: onoff or bulk", applicationType);
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("flows", "The number of TCP flows.", flows);
  cmd.AddValue ("dataRate", "Payload size in bytes", dataRate);
  cmd.AddValue ("tcpVariant", "Transport protocol to use: TcpTahoe, TcpReno, TcpNewReno, TcpWestwood, TcpWestwoodPlus", tcpVariant);
  cmd.AddValue ("tcpBufferSize", "TCP Buffer Size (Send/Receive)", tcpBufferSize);
  /* Wifi MAC/PHY Variables */
  cmd.AddValue ("msduAggregation", "The maximum aggregation size for A-MSDU in Bytes", msduAggregationSize);
  cmd.AddValue ("queueSize", "The size of the Wifi Mac Queue", queueSize);
  cmd.AddValue ("sp1Duration", "The duration of the forward SP allocation in MicroSeconds", sp1Duration);
  cmd.AddValue ("sp2Duration", "The duration of the reverse SP allocation in MicroSeconds", sp2Duration);
  cmd.AddValue ("spBlocks", "The number of blocks making up SP allocation", spBlocks);
  cmd.AddValue ("cbapDuration", "The duration of the allocated CBAP period in MicroSeconds (10ms)", cbapDuration);
  cmd.AddValue ("firstPeriod", "The duration of the RDS first period in MicroSeconds", firstPeriod);
  cmd.AddValue ("secondPeriod", "The duration of the RDS second period in MicroSeconds", secondPeriod);
  cmd.AddValue ("switchTime", "The time a which we switch from the direct link to the relay link", switchTime);
  cmd.AddValue ("switchForward", "Switch the forward link", switchForward);
  cmd.AddValue ("switchReverse", "Switch the reverse link", switchReverse);
  cmd.AddValue ("phyMode", "The 802.11ad PHY Mode", phyMode);
  /* Simulation Variables */
  cmd.AddValue ("verbose", "Turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable PCAP Tracing for WifiNetDevices", pcapTracing);
  cmd.AddValue ("ascii", "Enable ASCII Tracing for TCP Socket", asciiTraciing);
  cmd.Parse (argc, argv);

  /* Global params: no fragmentation, no RTS/CTS, fixed rate for all packets */
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("999999"));
  Config::SetDefault ("ns3::WifiMacQueue::MaxDelay", TimeValue (MilliSeconds (queueMaxDelay)));

  /*** Configure TCP Options ***/
  /* Select TCP variant */
  std::map<std::string, std::string>::const_iterator iter = tcpVariants.find (tcpVariant);
  NS_ASSERT_MSG (iter != tcpVariants.end (), "Cannot find Tcp Variant");
  TypeId tid = TypeId::LookupByName (iter->second);
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (tid));
  if (tcpVariant.compare ("Westwood") == 0)
    {
      Config::SetDefault ("ns3::TcpWestwood::ProtocolType", EnumValue (TcpWestwood::WESTWOOD));
      Config::SetDefault ("ns3::TcpWestwood::FilterType", EnumValue (TcpWestwood::TUSTIN));
    }
  else if (tcpVariant.compare ("WestwoodPlus") == 0)
    {
      Config::SetDefault ("ns3::TcpWestwood::ProtocolType", EnumValue (TcpWestwood::WESTWOODPLUS));
      Config::SetDefault ("ns3::TcpWestwood::FilterType", EnumValue (TcpWestwood::TUSTIN));
    }

  /* Configure TCP Segment Size */
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (tcpBufferSize));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (tcpBufferSize));

  /**** WifiHelper is a meta-helper: it helps creates helpers ****/
  WifiHelper wifi;

  /* Basic setup */
  wifi.SetStandard (WIFI_PHY_STANDARD_80211ad);

  /* Turn on logging */
  if (verbose)
    {
      wifi.EnableLogComponents ();
      LogComponentEnable ("EvaluateHalfDuplexRelay", LOG_LEVEL_ALL);
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
  Ptr<Node> rdsNode = wifiNodes.Get (1);
  Ptr<Node> westNode = wifiNodes.Get (2);
  Ptr<Node> eastNode = wifiNodes.Get (3);

  /* Add a DMG upper mac */
  DmgWifiMacHelper wifiMac = DmgWifiMacHelper::Default ();

  /* Install PCP/AP Node */
  Ssid ssid = Ssid ("HD-DF");
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

  /* Install RDS Node */
  wifiMac.SetType ("ns3::DmgStaWifiMac",
                   "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false),
                   "BE_MaxAmpduSize", UintegerValue (0),
                   "BE_MaxAmsduSize", UintegerValue (msduAggregationSize),
                   "RDSActivated", BooleanValue (true),   //Activate RDS
                   "REDSActivated", BooleanValue (false));

  NetDeviceContainer rdsDevice;
  rdsDevice = wifi.Install (wifiPhy, wifiMac, rdsNode);

  /* Install REDS Nodes */
  wifiMac.SetType ("ns3::DmgStaWifiMac",
                   "Ssid", SsidValue (ssid), "ActiveProbing", BooleanValue (false),
                   "BE_MaxAmpduSize", UintegerValue (0),
                   "BE_MaxAmsduSize", UintegerValue (msduAggregationSize),
                   "RDSActivated", BooleanValue (false),
                   "REDSActivated", BooleanValue (true),  //Activate REDS
                   "RDSDuplexMode", BooleanValue (false),
                   "RDSDataSensingTime", UintegerValue (200),
                   "RDSFirstPeriod", UintegerValue (firstPeriod),
                   "RDSSecondPeriod", UintegerValue (secondPeriod));

  NetDeviceContainer redsDevices;
  redsDevices = wifi.Install (wifiPhy, wifiMac, NodeContainer (westNode, eastNode));

  /* Setting mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, +1.0, 0.0));   /* PCP/AP */
  positionAlloc->Add (Vector (0.0, -1.0, 0.0));   /* RDS */
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
  Ipv4InterfaceContainer rdsInterface;
  rdsInterface = address.Assign (rdsDevice);
  Ipv4InterfaceContainer redsInterfaces;
  redsInterfaces = address.Assign (redsDevices);

  /* Populate routing table */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* We do not want any ARP packets */
  PopulateArpCache ();

  /* Install TCP sink on the access point */
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 5001));
  sink = StaticCast<PacketSink> (sinkHelper.Install (eastNode).Get (0));

  /* Install TCP transmitter on the station */
  Address dest (InetSocketAddress (redsInterfaces.GetAddress (1), 5001));
  TypeId socketTid = TypeId::LookupByName ("ns3::TcpSocketFactory");
  AsciiTraceHelper ascii;
  if (applicationType == "onoff")
    {
      Ptr<TcpSocketBase> tcpSocket = StaticCast<TcpSocketBase> (Socket::CreateSocket (westNode, socketTid));
      Ptr<TcpTxBuffer> txBuffer = tcpSocket->GetTxBuffer ();
      Ptr<TcpSendApplication> app = CreateObject<TcpSendApplication> ();

      app->Setup (tcpSocket, dest, payloadSize, DataRate (dataRate), false);
      westNode->AddApplication (app);
      app->SetStartTime (Seconds (3.0));

      if (asciiTraciing)
        {
          /* Connect TCP Socket Traces */
          Ptr<OutputStreamWrapper> cwndStream = ascii.CreateFileStream ("Traces" + tracesPath + "cwnd.txt");
          Ptr<OutputStreamWrapper> rwndStream = ascii.CreateFileStream ("Traces" + tracesPath + "rwnd.txt");
          Ptr<OutputStreamWrapper> rttStream = ascii.CreateFileStream ("Traces" + tracesPath + "rtt.txt");
          Ptr<OutputStreamWrapper> bufferSizeStream = ascii.CreateFileStream ("Traces" + tracesPath + "bufferSize.txt");

          tcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, cwndStream));
          tcpSocket->TraceConnectWithoutContext ("RWND", MakeBoundCallback (&RwndChange, rwndStream));
          tcpSocket->TraceConnectWithoutContext ("RTT", MakeBoundCallback (&RttChange, rttStream));
          txBuffer->TraceConnectWithoutContext ("SizeChanged", MakeBoundCallback (&BufferSizeChange, bufferSizeStream));
        }
    }
  else if (applicationType == "bulk")
    {
      /* Random variable for the initialization of the TCP connections */
      Ptr<UniformRandomVariable> variable = CreateObject<UniformRandomVariable> ();
      variable->SetAttribute ("Min", DoubleValue (0));
      variable->SetAttribute ("Max", DoubleValue (100));

      /* Generate #tcpFlows */
      Ptr<TcpSocketBase> tcpSocket[flows];
      Ptr<TcpTxBuffer> txBuffer[flows];
      Ptr<TcpSendApplication> app[flows];

      for (uint32_t i = 0; i <= flows - 1; i++)
        {
          ostringstream flowId;
          flowId << i + 1;
          tcpSocket[i] = StaticCast<TcpSocketBase> (Socket::CreateSocket (westNode, socketTid));
          txBuffer[i] = tcpSocket[i]->GetTxBuffer ();
          app[i] = CreateObject<TcpSendApplication> ();

          app[i]->Setup (tcpSocket[i], dest, payloadSize, DataRate (dataRate), true);
          westNode->AddApplication (app[i]);
          app[i]->SetStartTime (Seconds (3.0) + MilliSeconds (variable->GetInteger ()));

          if (asciiTraciing)
            {
              /* Connect TCP Socket Traces */
              AsciiTraceHelper asciiTraceHelper;
              Ptr<OutputStreamWrapper> cwndStream
                  = ascii.CreateFileStream ("Traces" + tracesPath + "cwnd_" + flowId.str () + ".txt");
              Ptr<OutputStreamWrapper> rwndStream
                  = ascii.CreateFileStream ("Traces" + tracesPath + "rwnd_" + flowId.str () + ".txt");
              Ptr<OutputStreamWrapper> rttStream
                  = ascii.CreateFileStream ("Traces" + tracesPath + "rtt_" + flowId.str () + ".txt");
              Ptr<OutputStreamWrapper> bufferSizeStream
                  = ascii.CreateFileStream ("Traces" + tracesPath + "tcpBufferSize_" + flowId.str () + ".txt");

              tcpSocket[i]->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, cwndStream));
              tcpSocket[i]->TraceConnectWithoutContext ("RWND", MakeBoundCallback (&RwndChange, rwndStream));
              tcpSocket[i]->TraceConnectWithoutContext ("RTT", MakeBoundCallback (&RttChange, rttStream));
              txBuffer[i]->TraceConnectWithoutContext ("SizeChanged", MakeBoundCallback (&BufferSizeChange, bufferSizeStream));
            }
        }
    }

  /* Schedule Throughput Calulcation */
  Simulator::Schedule (Seconds (3.1), &CalculateThroughput);

  /** Connect Trace Sources **/
  apWifiNetDevice = StaticCast<WifiNetDevice> (apDevice.Get (0));
  westRedsNetDevice = StaticCast<WifiNetDevice> (redsDevices.Get (0));
  eastRedsNetDevice = StaticCast<WifiNetDevice> (redsDevices.Get (1));
  rdsNetDevice = StaticCast<WifiNetDevice> (rdsDevice.Get (0));

  /* Set Maximum number of packets in WifiMacQueue */
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/DcaTxop/Queue/MaxPackets", UintegerValue (queueSize));
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxPackets", UintegerValue (queueSize));
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::DmgWifiMac/SPQueue/MaxPackets", UintegerValue (queueSize));
  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/$ns3::DmgWifiMac/SPQueue/MaxPackets", UintegerValue (queueSize));

  /* Enable Traces */
  if (pcapTracing)
    {
      wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
      wifiPhy.EnablePcap ("Traces/AccessPoint", apWifiNetDevice, false);
      wifiPhy.EnablePcap ("Traces/RDS", rdsNetDevice, false);
      wifiPhy.EnablePcap ("Traces/WEST", westRedsNetDevice, false);
      wifiPhy.EnablePcap ("Traces/EAST", eastRedsNetDevice, false);
    }

  apWifiMac = StaticCast<DmgApWifiMac> (apWifiNetDevice->GetMac ());
  westRedsMac = StaticCast<DmgStaWifiMac> (westRedsNetDevice->GetMac ());
  eastRedsMac = StaticCast<DmgStaWifiMac> (eastRedsNetDevice->GetMac ());
  rdsMac = StaticCast<DmgStaWifiMac> (rdsNetDevice->GetMac ());

  westRedsMac->TraceConnectWithoutContext ("RlsCompleted", MakeBoundCallback (&RlsCompleted, westRedsMac));
  eastRedsMac->TraceConnectWithoutContext ("RlsCompleted", MakeBoundCallback (&RlsCompleted, eastRedsMac));

  westRedsMac->TraceConnectWithoutContext ("ChannelReportReceived", MakeCallback (&ChannelReportReceived));
  eastRedsMac->TraceConnectWithoutContext ("ChannelReportReceived", MakeCallback (&ChannelReportReceived));

  westRedsMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, westRedsMac));
  eastRedsMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, eastRedsMac));
  rdsMac->TraceConnectWithoutContext ("SLSCompleted", MakeBoundCallback (&SLSCompleted, rdsMac));

  /* Relay Selector Function */
  westRedsMac->RegisterRelaySelectorFunction (MakeCallback (&SelectRelay));
  eastRedsMac->RegisterRelaySelectorFunction (MakeCallback (&SelectRelay));

  /* Print changes in number of bytes */
  if (asciiTraciing)
    {
      Ptr<WifiMacQueue> westRedsMacQueue = westRedsMac->GetSPQueue ();
      Ptr<WifiMacQueue> rdsMacQueue = rdsMac->GetSPQueue ();
      Ptr<WifiMacQueue> eastRedsMacQueue = eastRedsMac->GetSPQueue ();
      Ptr<OutputStreamWrapper> streamBytesInQueue1 = ascii.CreateFileStream ("Traces/WEST-STA-MAC-BytesInQueue.txt");
      Ptr<OutputStreamWrapper> streamBytesInQueue2 = ascii.CreateFileStream ("Traces/RDS-MAC-BytesInQueue.txt");
      Ptr<OutputStreamWrapper> streamBytesInQueue3 = ascii.CreateFileStream ("Traces/EAST-STA-MAC-BytesInQueue.txt");
      westRedsMacQueue->TraceConnectWithoutContext ("BytesInQueue", MakeBoundCallback (&BytesInQueueTrace, streamBytesInQueue1));
      rdsMacQueue->TraceConnectWithoutContext ("BytesInQueue", MakeBoundCallback (&BytesInQueueTrace, streamBytesInQueue2));
      eastRedsMacQueue->TraceConnectWithoutContext ("BytesInQueue", MakeBoundCallback (&BytesInQueueTrace, streamBytesInQueue3));
    }

  /** Schedule Events **/
  /* Request the DMG Capabilities of other DMG STAs */
  Simulator::Schedule (Seconds (1.05), &DmgStaWifiMac::RequestInformation, westRedsMac, eastRedsMac->GetAddress ());
  Simulator::Schedule (Seconds (1.06), &DmgStaWifiMac::RequestInformation, westRedsMac, rdsMac->GetAddress ());
  Simulator::Schedule (Seconds (1.07), &DmgStaWifiMac::RequestInformation, rdsMac, westRedsMac->GetAddress ());
  Simulator::Schedule (Seconds (1.08), &DmgStaWifiMac::RequestInformation, rdsMac, eastRedsMac->GetAddress ());
  Simulator::Schedule (Seconds (1.09), &DmgStaWifiMac::RequestInformation, eastRedsMac, westRedsMac->GetAddress ());
  Simulator::Schedule (Seconds (1.10), &DmgStaWifiMac::RequestInformation, eastRedsMac, rdsMac->GetAddress ());

  /* Initiate Relay Discovery Procedure */
  Simulator::Schedule (Seconds (1.3), &DmgStaWifiMac::StartRelayDiscovery, westRedsMac, eastRedsMac->GetAddress ());

  /* Schedule link switch event */
  if (switchForward)
    {
      Simulator::Schedule (Seconds (switchTime), &SwitchTransmissionLink, westRedsMac, eastRedsMac);
    }
  if (switchReverse)
    {
      Simulator::Schedule (Seconds (switchTime), &SwitchTransmissionLink, eastRedsMac, westRedsMac);
    }

  /* Schedule tear down event */
  if (switchForward)
    {
      Simulator::Schedule (Seconds (switchTime + 3), &TearDownRelay, westRedsMac, eastRedsMac);
    }
  if (switchReverse)
    {
      Simulator::Schedule (Seconds (switchTime + 3), &TearDownRelay, eastRedsMac, westRedsMac);
    }

  /* Print Output*/
  std::cout << "Time[s]" << '\t' << "Thoughput[Mbps]" << std::endl;

  Simulator::Stop (Seconds (simulationTime));
  Simulator::Run ();
  Simulator::Destroy ();

  /* Print Results Summary */
  std::cout << "Total number of Rx packets = " << sink->GetTotalReceivedPackets () << std::endl;
  std::cout << "Average throughput [Mbps] = " << westEastAverageThroughput/((simulationTime - 3) * 10) << std::endl;

  return 0;
}

