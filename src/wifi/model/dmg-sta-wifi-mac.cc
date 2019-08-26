/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/trace-source-accessor.h"

#include "amsdu-subframe-header.h"
#include "dcf-manager.h"
#include "dmg-capabilities.h"
#include "dmg-sta-wifi-mac.h"
#include "dmg-wifi-phy.h"
#include "ext-headers.h"
#include "mgt-headers.h"
#include "mac-low.h"
#include "msdu-aggregator.h"
#include "snr-tag.h"
#include "wifi-mac-header.h"
#include "wifi-mac-queue.h"

#include <cmath>

/*
 * The state machine for this DMG STA is:
 --------------                                                                                                 -----------
 | Associated |  <------------------------------------------------------------------------------      ------->  | Refused |
 --------------                                                                                 \    /          -----------
    \                    --------   --------------------   ------------------------              \  /
     \    ------------   | Wait |   | Wait Beamforming |   | Beamforming Training |   -----------------------------
      \-> | Scanning |-->|Beacon|-->|     Training     |-->|     Completed        |-->| Wait Association Response |
          ------------   --------   --------------------   ------------------------   -----------------------------
           ^ |  \                                                                                  ^
           | |   \                                                                                 |
            -     \                                                                      -----------------------
                   \-------------------------------------------------------------------> | Wait Probe Response |
                                                                                         -----------------------

 Unassociated (not depicted) is a transient state between Scanning and either
 WaitBeacon or WaitProbeResponse
*/

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DmgStaWifiMac");

NS_OBJECT_ENSURE_REGISTERED (DmgStaWifiMac);

TypeId
DmgStaWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgStaWifiMac")
    .SetParent<DmgWifiMac> ()
    .AddConstructor<DmgStaWifiMac> ()

    /* Add Scanning Capability to DmgStaWifiMac */
    .AddAttribute ("ScanningTimeout", "The interval to dwell on a channel while scanning",
                   TimeValue (MilliSeconds (120)),
                   MakeTimeAccessor (&DmgStaWifiMac::m_scanningTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("MaxMissedBeacons",
                   "Number of DMG Beacons which much be consecutively missed before "
                   "we attempt to restart association.",
                   UintegerValue (10),
                   MakeUintegerAccessor (&DmgStaWifiMac::m_maxMissedBeacons),
                   MakeUintegerChecker<uint32_t> ())

    .AddAttribute ("ProbeRequestTimeout", "The interval between two consecutive probe request attempts.",
                    TimeValue (Seconds (0.05)),
                    MakeTimeAccessor (&DmgStaWifiMac::m_probeRequestTimeout),
                    MakeTimeChecker ())
    .AddAttribute ("AssocRequestTimeout", "The interval between two consecutive assoc request attempts.",
                   TimeValue (Seconds (0.5)),
                   MakeTimeAccessor (&DmgStaWifiMac::m_assocRequestTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("MaxLostBeacons",
                   "Maximum Number of Lost Beacons.",
                   UintegerValue (10),
                   MakeUintegerAccessor (&DmgStaWifiMac::m_maxLostBeacons),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("ActiveProbing",
                   "If true, we send probe requests. If false, we don't."
                   "NOTE: if more than one STA in your simulation is using active probing, "
                   "you should enable it at a different simulation time for each STA, "
                   "otherwise all the STAs will start sending probes at the same time resulting in collisions."
                   "See bug 1060 for more info.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&DmgStaWifiMac::SetActiveProbing, &DmgStaWifiMac::GetActiveProbing),
                   MakeBooleanChecker ())

    /* A-BFT Attributes */
    .AddAttribute ("ImmediateAbft",
                   "If true, we start a responder TXSS in the following A-BFT when we receive a DMG Beacon frame"
                   " in the BTI with a fragmented initiator TXSS, otherwise we scan for  the number of beacon"
                   " intervals indicated in a received TXSS Span field in order to cover a complete initiator"
                   " TXSS and find a suitable TX sector from the PCP/AP.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&DmgStaWifiMac::m_immediateAbft),
                   MakeBooleanChecker ())
    .AddAttribute ("RSSRetryLimit", "Responder Sector Sweep Retry Limit.",
                   UintegerValue (dot11RSSRetryLimit),
                   MakeUintegerAccessor (&DmgStaWifiMac::m_rssAttemptsLimit),
                   MakeUintegerChecker<uint8_t> (1, 32))
    .AddAttribute ("RSSBackoff", "Maximum Responder Sector Sweep Backoff value.",
                   UintegerValue (dot11RSSBackoff),
                   MakeUintegerAccessor (&DmgStaWifiMac::m_rssBackoffLimit),
                   MakeUintegerChecker<uint8_t> (1 ,32))

    /* Link Maintenance Attributes */
    .AddAttribute ("BeamLinkMaintenanceUnit", "The unit used for dot11BeamLinkMaintenanceTime calculation.",
                   EnumValue (UNIT_32US),
                   MakeEnumAccessor (&DmgStaWifiMac::m_beamlinkMaintenanceUnit),
                   MakeEnumChecker (UNIT_32US, "32US",
                                    UNIT_2000US, "2000US"))
    .AddAttribute ("BeamLinkMaintenanceValue", "The value of the beamlink maintenance used for dot11BeamLinkMaintenanceTime calculation.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&DmgStaWifiMac::m_beamlinkMaintenanceValue),
                   MakeUintegerChecker<uint8_t> (0, 63))

    /* DMG Relay Capabilities */
    .AddAttribute ("ActivateRelayAck", "Whether to Send Relay ACK Request in HD-DF",
                    BooleanValue (false),
                    MakeBooleanAccessor (&DmgStaWifiMac::m_relayAckRequest),
                    MakeBooleanChecker ())
    .AddAttribute ("RDSDuplexMode", "0 = HD-DF, 1 = FD-AF.",
                    BooleanValue (false),
                    MakeBooleanAccessor (&DmgStaWifiMac::m_rdsDuplexMode),
                    MakeBooleanChecker ())
    .AddAttribute ("RDSLinkChangeInterval", "In MicroSeconds",
                    UintegerValue (200),
                    MakeUintegerAccessor (&DmgStaWifiMac::m_relayLinkChangeInterval),
                    MakeUintegerChecker<uint8_t> (1, std::numeric_limits<uint8_t>::max ()))
    .AddAttribute ("RDSDataSensingTime", "In MicroSeconds. By default, it is set to SIFS plus SBIFS.",
                    UintegerValue (4),
                    MakeUintegerAccessor (&DmgStaWifiMac::m_relayDataSensingTime),
                    MakeUintegerChecker<uint8_t> (1, std::numeric_limits<uint8_t>::max ()))
    .AddAttribute ("RDSFirstPeriod", "In MicroSeconds",
                    UintegerValue (4000),
                    MakeUintegerAccessor (&DmgStaWifiMac::m_relayFirstPeriod),
                    MakeUintegerChecker<uint16_t> (1, std::numeric_limits<uint16_t>::max ()))
    .AddAttribute ("RDSSecondPeriod", "In MicroSeconds",
                    UintegerValue (4000),
                    MakeUintegerAccessor (&DmgStaWifiMac::m_relaySecondPeriod),
                    MakeUintegerChecker<uint16_t> (1, std::numeric_limits<uint16_t>::max ()))

    /* DMG Capabilities */
    .AddAttribute ("SupportSPSH", "Whether the DMG STA supports Spartial Sharing and Interference Mitigation (SPSH)",
                    BooleanValue (false),
                    MakeBooleanAccessor (&DmgStaWifiMac::m_supportSpsh),
                    MakeBooleanChecker ())

    /* Dynamic Allocation of Service Period */
    .AddAttribute ("StaAvailabilityElement", "Whether STA availability element is announced in Association Request",
                   BooleanValue (false),
                   MakeBooleanAccessor (&DmgStaWifiMac::m_staAvailabilityElement),
                   MakeBooleanChecker ())
    .AddAttribute ("PollingPhase", "The PollingPhase is set to 1 to indicate that the STA is "
                   "available during PPs otherwise it is set to 0",
                   BooleanValue (false),
                   MakeBooleanAccessor (&DmgStaWifiMac::m_pollingPhase),
                   MakeBooleanChecker ())

    /* Add Scanning Capability to DmgStaWifiMac */
    .AddTraceSource ("BeaconArrival",
                     "Time that beacons arrive from my AP while associated",
                     MakeTraceSourceAccessor (&DmgStaWifiMac::m_beaconArrival),
                     "ns3::Time::TracedValueCallback")

    .AddTraceSource ("Assoc", "Associated with an access point.",
                     MakeTraceSourceAccessor (&DmgStaWifiMac::m_assocLogger),
                     "ns3::DmgWifiMac::AssociationTracedCallback")
    .AddTraceSource ("DeAssoc", "Association with an access point lost.",
                     MakeTraceSourceAccessor (&DmgStaWifiMac::m_deAssocLogger),
                     "ns3::Mac48Address::TracedCallback")

    /* DMG BSS peer and service discovery */
    .AddTraceSource ("InformationResponseReceived", "Received information response regarding specific station.",
                     MakeTraceSourceAccessor (&DmgStaWifiMac::m_informationReceived),
                     "ns3::Mac48Address::TracedCallback")

    /* Relay Procedure Related Traces */
    .AddTraceSource ("ChannelReportReceived", "The DMG STA has received a channel report.",
                     MakeTraceSourceAccessor (&DmgStaWifiMac::m_channelReportReceived),
                     "ns3::Mac48Address::TracedCallback")
    .AddTraceSource ("RdsDiscoveryCompleted",
                     "The RDS Discovery procedure is completed",
                     MakeTraceSourceAccessor (&DmgStaWifiMac::m_rdsDiscoveryCompletedCallback),
                     "ns3::Mac48Address::RdsDiscoveryCompletedTracedCallback")
    .AddTraceSource ("TransmissionLinkChanged", "The current transmission link has been changed.",
                     MakeTraceSourceAccessor (&DmgStaWifiMac::m_transmissionLinkChanged),
                     "ns3::DmgStaWifiMac::TransmissionLinkChangedTracedCallback")
    .AddTraceSource ("BeamLinkMaintenanceTimerExpired",
                     "The BeamLink maintenance timer associated to a link has expired.",
                     MakeTraceSourceAccessor (&DmgStaWifiMac::m_beamLinkMaintenanceTimerExpired),
                     "ns3::DmgStaWifiMac::BeamLinkMaintenanceTimerExpiredTracedCallback")
  ;
  return tid;
}

DmgStaWifiMac::DmgStaWifiMac ()
  : m_probeRequestEvent (),
    m_assocRequestEvent (),
    m_beaconWatchdogEnd (Seconds (0.0)),
    m_waitBeaconEvent (),
    m_abftEvent (),
//    m_dtiStartEvent (),
    m_linkChangeInterval (),
    m_firstPeriod (),
    m_secondPeriod ()
{
  NS_LOG_FUNCTION (this);
  /** Initialize Relay Variables **/
  m_relayMode = false;
  /* Set missed ACK/BlockACK callback */
  for (EdcaQueues::iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->SetMissedAckCallback (MakeCallback (&DmgStaWifiMac::MissedAck, this));
    }
  /** Association State Variables **/
  m_state = UNASSOCIATED;
  /* Let the lower layers know that we are acting as a non-AP DMG STA in an infrastructure BSS. */
  SetTypeOfStation (DMG_STA);
}

DmgStaWifiMac::~DmgStaWifiMac ()
{
  NS_LOG_FUNCTION(this);
}

void
DmgStaWifiMac::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);

  /* Initialize DMG Reception */
  m_receivedDmgBeacon = false;

  /** Initialize A_BFT Variables **/
  a_bftSlot = CreateObject<UniformRandomVariable> ();
  m_rssBackoffVariable = CreateObject<UniformRandomVariable> ();
  m_rssBackoffVariable->SetAttribute ("Min", DoubleValue (0));
  m_rssBackoffVariable->SetAttribute ("Max", DoubleValue (m_rssBackoffLimit));
  m_failedRssAttemptsCounter = 0;
  m_rssBackoffRemaining = 0;
  m_nextBeacon = 0;
  m_abftState = WAIT_BEAMFORMING_TRAINING;

  /* Initialize upper DMG Wifi MAC */
  DmgWifiMac::DoInitialize ();

  /* Channel Measurement */
  StaticCast<DmgWifiPhy> (m_phy)->RegisterMeasurementResultsReady (MakeCallback (&DmgStaWifiMac::ReportChannelQualityMeasurement, this));

  /* Link Maintenance */
  if (m_beamlinkMaintenanceUnit == UNIT_32US)
    {
      dot11BeamLinkMaintenanceTime = m_beamlinkMaintenanceValue * 32;
    }
  else
    {
      dot11BeamLinkMaintenanceTime = m_beamlinkMaintenanceValue * 2000;
    }

  /* Initialize variables since we expect to receive DMG Beacon */
  m_accessPeriod = CHANNEL_ACCESS_BTI;
  m_sectorFeedbackSchedulled = false;
  /* At the beginning of the BTI period, a DMG STA should stay in Quasi-Omni receiving mode */
  m_codebook->StartReceivingInQuasiOmniMode ();
}

void
DmgStaWifiMac::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  DmgWifiMac::DoDispose ();
}

void
DmgStaWifiMac::SetWifiRemoteStationManager (Ptr<WifiRemoteStationManager> stationManager)
{
  NS_LOG_FUNCTION (this << stationManager);
  DmgWifiMac::SetWifiRemoteStationManager (stationManager);
}

void
DmgStaWifiMac::SetMaxLostBeacons (uint32_t lost)
{
  NS_LOG_FUNCTION (this << lost);
  m_maxLostBeacons = lost;
}

void
DmgStaWifiMac::SetProbeRequestTimeout (Time timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  m_probeRequestTimeout = timeout;
}

void
DmgStaWifiMac::SetAssocRequestTimeout (Time timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  m_assocRequestTimeout = timeout;
}

void
DmgStaWifiMac::StartActiveAssociation (void)
{
  NS_LOG_FUNCTION (this);
  TryToEnsureAssociated ();
}

void
DmgStaWifiMac::SetActiveProbing (bool enable)
{
  NS_LOG_FUNCTION(this << enable);
  if (enable)
    {
      Simulator::ScheduleNow (&DmgStaWifiMac::TryToEnsureAssociated, this);
    }
  else
    {
      m_probeRequestEvent.Cancel ();
    }
  m_activeProbing = enable;
}

bool
DmgStaWifiMac::GetActiveProbing (void) const
{
  return m_activeProbing;
}

void
DmgStaWifiMac::SendProbeRequest (void)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_PROBE_REQUEST);
  hdr.SetAddr1 (Mac48Address::GetBroadcast ());
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (Mac48Address::GetBroadcast ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  Ptr<Packet> packet = Create<Packet> ();
  MgtProbeRequestHeader probe;
  probe.SetSsid (GetSsid ());

  /* DMG Capabilities Information Element */
  probe.AddWifiInformationElement (GetDmgCapabilities ());

  packet->AddHeader (probe);

  // The standard is not clear on the correct queue for management
  // frames if we are a QoS AP. The approach taken here is to always
  // use the DCF for these regardless of whether we have a QoS
  // association or not.
  m_dca->Queue(packet, hdr);

  if (m_probeRequestEvent.IsRunning())
    {
      m_probeRequestEvent.Cancel();
    }
  m_probeRequestEvent = Simulator::Schedule(m_probeRequestTimeout, &DmgStaWifiMac::ProbeRequestTimeout, this);
}

void
DmgStaWifiMac::SendAssociationRequest (void)
{
  NS_LOG_FUNCTION (this << GetBssid ());
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ASSOCIATION_REQUEST);
  hdr.SetAddr1 (GetBssid ());
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetBssid ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  Ptr<Packet> packet = Create<Packet> ();
  MgtAssocRequestHeader assoc;
  assoc.SetSsid (GetSsid ());

  /* DMG Capabilities Information Element */
  assoc.AddWifiInformationElement (GetDmgCapabilities ());
  /* Multi-band Information Element */
  if (m_supportMultiBand)
    {
      assoc.AddWifiInformationElement (GetMultiBandElement ());
    }
  /* Add Relay Capability Element */
  if (m_redsActivated || m_rdsActivated)
    {
      assoc.AddWifiInformationElement (GetRelayCapabilitiesElement ());
    }
  if (m_staAvailabilityElement)
    {
      assoc.AddWifiInformationElement (GetStaAvailabilityElement ());
    }

  packet->AddHeader (assoc);

  // The standard is not clear on the correct queue for management
  // frames if we are a QoS AP. The approach taken here is to always
  // use the DCF for these regardless of whether we have a QoS
  // association or not.
  m_dca->Queue (packet, hdr);

  if (m_assocRequestEvent.IsRunning ())
    {
      m_assocRequestEvent.Cancel ();
    }

  /* For now, we assume station talks to the DMG AP only */
  SteerAntennaToward (GetBssid ());

  m_assocRequestEvent = Simulator::Schedule (m_assocRequestTimeout, &DmgStaWifiMac::AssocRequestTimeout, this);
}

void
DmgStaWifiMac::TryToEnsureAssociated (void)
{
  NS_LOG_FUNCTION (this);
  switch (m_state)
  {
  case ASSOCIATED:
    return;
    break;

  case WAIT_PROBE_RESP:
    /* we have sent a probe request earlier so we
     do not need to re-send a probe request immediately.
     We just need to wait until probe-request-timeout
     or until we get a probe response
    */
    break;

  case UNASSOCIATED:
    /* we were associated but we missed a bunch of beacons
    * so we should assume we are not associated anymore.
    * We try to initiate a probe request now.
    */
    m_linkDown ();
    if (m_activeProbing)
      {
        SetState (WAIT_PROBE_RESP);
        m_bestBeaconObserved.Clear ();
        SendProbeRequest ();
      }
    else
      {
        if (m_waitBeaconEvent.IsRunning ())
          {
            m_waitBeaconEvent.Cancel ();
          }
        m_bestBeaconObserved.Clear ();
        m_waitBeaconEvent = Simulator::Schedule (m_scanningTimeout, &DmgStaWifiMac::WaitBeaconTimeout, this);
        SetState (WAIT_BEACON);
      }
    break;

  case WAIT_BEACON:
    /* Continue to wait and gather beacons */
      break;

  case WAIT_ASSOC_RESP:
    /* we have sent an assoc request so we do not need to
     re-send an assoc request right now. We just need to
     wait until either assoc-request-timeout or until
     we get an assoc response.
    */
    break;

  case REFUSED:
    /* we have sent an assoc request and received a negative
     assoc resp. We wait until someone restarts an
     association with a given ssid.
     */
      StartScanning ();
      break;
  case BEACON_MISSED:
  case SCANNING:
    break;
  }
}

void
DmgStaWifiMac::AssocRequestTimeout (void)
{
  NS_LOG_FUNCTION (this);
  SetState (WAIT_ASSOC_RESP);
  SendAssociationRequest ();
}

uint16_t
DmgStaWifiMac::GetAssociationID (void)
{
  NS_LOG_FUNCTION (this);
  if (m_state == ASSOCIATED)
    {
      return m_aid;
    }
  else
    {
      return 0;
    }
}

void
DmgStaWifiMac::CreateAllocation (DmgTspecElement elem)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (GetBssid ());
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetBssid ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  DmgAddTSRequestFrame frame;
  frame.SetDmgTspecElement (elem);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.qos = WifiActionHeader::ADDTS_REQUEST;
  actionHdr.SetAction (WifiActionHeader::QOS, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (frame);
  packet->AddHeader (actionHdr);

  m_dca->Queue (packet, hdr);
}

void
DmgStaWifiMac::DeleteAllocation (uint16_t reason, DmgAllocationInfo &allocationInfo)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (GetBssid ());
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetBssid ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  DelTsFrame frame;
  frame.SetReasonCode (reason);
  frame.SetDmgAllocationInfo (allocationInfo);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.qos = WifiActionHeader::DELTS;
  actionHdr.SetAction (WifiActionHeader::QOS, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (frame);
  packet->AddHeader (actionHdr);

  m_dca->Queue (packet, hdr);
}

void
DmgStaWifiMac::ProbeRequestTimeout (void)
{
  NS_LOG_FUNCTION (this);
  SetState (WAIT_PROBE_RESP);
  if (m_bestBeaconObserved.m_snr > 0)
    {
      NS_LOG_DEBUG ("one or more ProbeResponse received; selecting " << m_bestBeaconObserved.m_bssid);
      SupportedRates rates = m_bestBeaconObserved.m_probeResp.GetSupportedRates ();
      for (uint32_t i = 0; i < m_phy->GetNBssMembershipSelectors (); i++)
        {
         uint32_t selector = m_phy->GetBssMembershipSelector (i);
         if (!rates.IsSupportedRate (selector))
           {
             return;
           }
        }
      for (uint32_t i = 0; i < m_phy->GetNModes (); i++)
        {
          WifiMode mode = m_phy->GetMode (i);
          uint8_t nss = 1; // Assume 1 spatial stream
          if (rates.IsSupportedRate (mode.GetDataRate (m_phy->GetChannelWidth (), false, nss)))
            {
              m_stationManager->AddSupportedMode (m_bestBeaconObserved.m_bssid, mode);
              if (rates.IsBasicRate (mode.GetDataRate (m_phy->GetChannelWidth (), false, nss)))
                {
                  m_stationManager->AddBasicMode (mode);
                }
            }
        }

      SetBssid (m_bestBeaconObserved.m_bssid);
      Time delay = MicroSeconds (m_bestBeaconObserved.m_probeResp.GetBeaconIntervalUs () * m_maxMissedBeacons);
      RestartBeaconWatchdog (delay);
      if (m_probeRequestEvent.IsRunning ())
        {
          m_probeRequestEvent.Cancel ();
        }
       SetState (WAIT_ASSOC_RESP);
       SendAssociationRequest ();
    }
  else
    {
      NS_LOG_DEBUG ("no probe responses received; resend request");
      SendProbeRequest ();
    }
}

void
DmgStaWifiMac::WaitBeaconTimeout (void)
{
  NS_LOG_FUNCTION (this);
  if (m_bestBeaconObserved.m_snr > 0)
    {
      NS_LOG_DEBUG ("DMG Beacon found, selecting " << m_bestBeaconObserved.m_bssid);
      SetBssid (m_bestBeaconObserved.m_bssid);
      SetState (WAIT_ASSOC_RESP);
      SendAssociationRequest ();
    }
  else
    {
      NS_LOG_DEBUG ("No DMG Beacons were received; restart scanning");
      StartScanning ();
    }
}

void
DmgStaWifiMac::MissedBeacons (void)
{
  NS_LOG_FUNCTION (this);
  if (m_beaconWatchdogEnd > Simulator::Now ())
    {
      if (m_beaconWatchdog.IsRunning ())
        {
          m_beaconWatchdog.Cancel ();
        }
      m_beaconWatchdog = Simulator::Schedule (m_beaconWatchdogEnd - Simulator::Now (), &DmgStaWifiMac::MissedBeacons, this);
      return;
    }
  NS_LOG_DEBUG ("DMG Beacon watchdog expired; starting to scan again");
  StartScanning ();
}

void
DmgStaWifiMac::RestartBeaconWatchdog (Time delay)
{
  NS_LOG_FUNCTION (this << delay << Simulator::GetDelayLeft (m_beaconWatchdog) << m_beaconWatchdogEnd);
  m_beaconWatchdogEnd = std::max (Simulator::Now () + delay, m_beaconWatchdogEnd);
  if (Simulator::GetDelayLeft (m_beaconWatchdog) < delay && m_beaconWatchdog.IsExpired ())
    {
      NS_LOG_DEBUG ("Restart watchdog.");
      m_beaconWatchdog = Simulator::Schedule (delay, &DmgStaWifiMac::MissedBeacons, this);
    }
}

bool
DmgStaWifiMac::IsBeamformedTrained (void) const
{
  return m_abftState == BEAMFORMING_TRAINING_COMPLETED;
}

bool
DmgStaWifiMac::IsAssociated (void) const
{
  return m_state == ASSOCIATED;
}

bool
DmgStaWifiMac::IsWaitAssocResp (void) const
{
  return m_state == WAIT_ASSOC_RESP;
}

void
DmgStaWifiMac::ForwardDataFrame (WifiMacHeader hdr, Ptr<Packet> packet, Mac48Address destAddress)
{
  NS_LOG_FUNCTION (this << packet << destAddress);
  hdr.SetAddr1 (destAddress);
  hdr.SetAddr2 (GetAddress ());
  if (hdr.IsQosAmsdu ())
    {
      hdr.SetType (WIFI_MAC_QOSDATA);
      hdr.SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);
      hdr.SetQosNoAmsdu ();
      MsduAggregator::DeaggregatedMsdus packets = MsduAggregator::Deaggregate (packet);
      for (MsduAggregator::DeaggregatedMsdusCI i = packets.begin (); i != packets.end (); ++i)
        {
          m_edca[AC_BE]->Queue ((*i).first, hdr);
          NS_LOG_DEBUG ("Frame Length=" << (*i).first->GetSize ());
        }
    }
  else
    {
      m_edca[AC_BE]->Queue (packet, hdr);
    }
}

void
DmgStaWifiMac::Enqueue (Ptr<const Packet> packet, Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << to);
  if (!IsAssociated ())
    {
      NotifyTxDrop (packet);
      TryToEnsureAssociated ();
      return;
    }
  WifiMacHeader hdr;
  // If we are not a QoS AP then we definitely want to use AC_BE to
  // transmit the packet. A TID of zero will map to AC_BE (through \c
  // QosUtilsMapTidToAc()), so we use that as our default here.
  uint8_t tid = 0;
  /* The HT Control field is not present in frames transmitted by a DMG STA. */
  hdr.SetAsDmgPpdu ();
  /* The QoS Data and QoS Null subtypes are the only Data subtypes transmitted by a DMG STA. */
  hdr.SetType (WIFI_MAC_QOSDATA);
  // Fill in the QoS control field in the MAC header
  tid = QosUtilsGetTidForPacket (packet);
  // Any value greater than 7 is invalid and likely indicates that
  // the packet had no QoS tag, so we revert to zero, which'll
  // mean that AC_BE is used.
  if (tid > 7)
    {
      tid = 0;
    }
  hdr.SetQosTid (tid);
  hdr.SetQosNoEosp ();
  hdr.SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);
  hdr.SetQosNoAmsdu ();
  hdr.SetQosRdGrant (m_supportRdp);

  bool found = false;
  AccessPeriodInformation accessPeriodInfo;
  for (DataForwardingTableIterator it = m_dataForwardingTable.begin (); it != m_dataForwardingTable.end (); it++)
    {
      accessPeriodInfo = it->second;
      if (it->first == to)
        {
          found = true;
          break;
        }
    }

  if (found && accessPeriodInfo.nextHopAddress != GetBssid ())
    {
      hdr.SetAddr1 (accessPeriodInfo.nextHopAddress);
      hdr.SetAddr2 (GetAddress ());
      hdr.SetAddr3 (GetBssid ());
      hdr.SetDsNotTo ();
    }
  else
    {
      /* The DMG PCP/AP is our receiver */
      hdr.SetAddr1 (GetBssid ());
      hdr.SetAddr2 (GetAddress ());
      hdr.SetAddr3 (to);
      hdr.SetDsTo ();
    }
  hdr.SetDsNotFrom ();

  //Sanity check that the TID is valid
  NS_ASSERT (tid < 8);

  m_edca[QosUtilsMapTidToAc (tid)]->Queue (packet, hdr);
}

void
DmgStaWifiMac::StartScanning (void)
{
  NS_LOG_FUNCTION (this);
  SetState (SCANNING);
  m_candidateChannels = m_phy->GetOperationalChannelList ();
  if (m_candidateChannels.size () == 1)
    {
      NS_LOG_DEBUG ("No need to scan; only one channel possible");
      m_candidateChannels.clear ();
      SetState (UNASSOCIATED);
      TryToEnsureAssociated ();
      return;
    }
  // Keep track of whether we find any good beacons, so that if we do
  // not, we restart scanning
  m_bestBeaconObserved.Clear ();
  uint32_t nextChannel = m_candidateChannels.back ();
  m_candidateChannels.pop_back ();
  NS_LOG_DEBUG ("Scanning channel " << nextChannel);
  Simulator::Schedule (m_scanningTimeout, &DmgStaWifiMac::ScanningTimeout, this);
}

void
DmgStaWifiMac::ScanningTimeout (void)
{
  NS_LOG_FUNCTION (this);
  if (m_candidateChannels.size () == 0)
    {
      if (m_bestBeaconObserved.m_channelNumber == 0)
        {
          NS_LOG_DEBUG ("No beacons found when scanning; restart scanning");
          StartScanning ();
          return;
        }
      NS_LOG_DEBUG ("Stopping scanning; best beacon found on channel " << m_bestBeaconObserved.m_channelNumber);
      m_phy->SetChannelNumber (m_bestBeaconObserved.m_channelNumber);
      SetState (UNASSOCIATED);
      TryToEnsureAssociated ();
      return;
    }
  uint32_t nextChannel = m_candidateChannels.back ();
  m_candidateChannels.pop_back ();
  NS_LOG_DEBUG ("Scanning channel " << nextChannel);
  Simulator::Schedule (m_scanningTimeout, &DmgStaWifiMac::ScanningTimeout, this);
}

void
DmgStaWifiMac::CommunicateInServicePeriod (Mac48Address peerAddress)
{
  NS_LOG_FUNCTION (this << GetAddress () << peerAddress);
  /* The two stations can communicate in TDMA like manner */
  DataForwardingTableIterator it = m_dataForwardingTable.find (peerAddress);
  if (it != m_dataForwardingTable.end ())
    {
      it->second.isCbapPeriod = false;
    }
  else
    {
      AccessPeriodInformation info;
      info.isCbapPeriod = false;
      info.nextHopAddress = peerAddress;
      m_dataForwardingTable[peerAddress] = info;
    }
}

Ptr<StaAvailabilityElement>
DmgStaWifiMac::GetStaAvailabilityElement (void) const
{
  Ptr<StaAvailabilityElement> availabilityElement = Create<StaAvailabilityElement> ();
  StaInfoField field;
  field.SetAid (m_aid);
  field.SetCbap (true);
  field.SetPollingPhase (m_pollingPhase);
  availabilityElement->AddStaInfo (field);
  return availabilityElement;
}

void
DmgStaWifiMac::SendSprFrame (Mac48Address to, Time duration, DynamicAllocationInfoField &info, BF_Control_Field &bfField)
{
  NS_LOG_FUNCTION (this << to);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_CTL_DMG_SPR);
  hdr.SetAddr1 (to);            //RA Field (MAC Address of the STA being polled)
  hdr.SetAddr2 (GetAddress ()); //TA Field (MAC Address of the PCP/AP)
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();

  Ptr<Packet> packet = Create<Packet> ();
  CtrlDMG_SPR spr;
  spr.SetDynamicAllocationInfo (info);
  spr.SetBFControl (bfField);

  packet->AddHeader (spr);

  /* Transmit control frames directly without DCA + DCF Manager */
  SteerAntennaToward (to);
  TransmitControlFrameImmediately (packet, hdr, duration);
}

void
DmgStaWifiMac::RegisterSPRequestFunction (ServicePeriodRequestCallback callback)
{
  m_servicePeriodRequestCallback = callback;
}

void
DmgStaWifiMac::StartBeaconInterval (void)
{
  NS_LOG_FUNCTION (this << "DMG STA Starting BI at " << Simulator::Now ());
  /* Schedule the next period */
  if (m_nextBeacon == 0)
    {
      StartBeaconTransmissionInterval ();
    }
  else
    {
      /* We will not receive DMG Beacon during this BI */
      m_nextBeacon--;
      m_biStartTime = Simulator::Now ();
      if (m_atiPresent)
        {
          StartAnnouncementTransmissionInterval ();
          NS_LOG_DEBUG ("ATI for Station:" << GetAddress () << " is scheduled at " << Simulator::Now ());
        }
      else
        {
          StartDataTransmissionInterval ();
          NS_LOG_DEBUG ("DTI for Station:" << GetAddress () << " is scheduled at " << Simulator::Now ());
        }
    }
}

void
DmgStaWifiMac::EndBeaconInterval (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("DMG STA Ending BI at " << Simulator::Now ());
  /* Disable Channel Access by CBAP */
//  EndContentionPeriod ();
  /* Start New Beacon Interval */
  StartBeaconInterval ();
}

void
DmgStaWifiMac::StartBeaconTransmissionInterval (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("DMG STA Starting BTI at " << Simulator::Now ());
  m_accessPeriod = CHANNEL_ACCESS_BTI;
  /* Re-initialize variables since we expect to receive DMG Beacon */
  m_sectorFeedbackSchedulled = false;
  /* Switch to the next Quasi-omni pattern */
  m_codebook->SwitchToNextQuasiPattern ();
  /* Handle special case in which we have associated to DMG PCP/AP but we did not receive later DMG Beacon
     because of that we lost somehow sunchronization so we try to use old schedulling information. */
  /**
    * 9.33.6.3 Contention-based access period (CBAP) allocation:
    * When the entire DTI is allocated to CBAP through the CBAP Only field in the DMG Parameters field, then
    * that CBAP is pseudo-static and exists for dot11MaxLostBeacons beacon intervals following the most
    * recently transmitted DMG Beacon that contained the indication, except if the CBAP is canceled by the
    * transmission by the PCP/AP of a DMG Beacon with the CBAP Only field of the DMG Parameters field
    * equal to 0 or an Announce frame with an Extended Schedule element.
    */
//  if (m_isCbapOnly)
//    {
//      m_dtiStartEvent = Simulator::Schedule (m_bhiDuration, &DmgStaWifiMac::StartDataTransmissionInterval, this);
//    }
}

void
DmgStaWifiMac::StartAssociationBeamformTraining (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("DMG STA Starting A-BFT at " << Simulator::Now ());
  m_accessPeriod = CHANNEL_ACCESS_ABFT;

  /* Schedule the end of the A-BFT period */
  Simulator::Schedule (m_abftDuration, &DmgStaWifiMac::EndAssociationBeamformTraining, this);

  /* Schedule access period after A-BFT period */
  if (m_atiPresent)
    {
      Simulator::Schedule (m_abftDuration + m_mbifs, &DmgStaWifiMac::StartAnnouncementTransmissionInterval, this);
      NS_LOG_DEBUG ("ATI for Station:" << GetAddress () << " is scheduled at " << Simulator::Now () + m_abftDuration + m_mbifs);
    }
  else
    {
      Simulator::Schedule (m_abftDuration + m_mbifs, &DmgStaWifiMac::StartDataTransmissionInterval, this);
      NS_LOG_DEBUG ("DTI for Station:" << GetAddress () << " is scheduled at " << Simulator::Now () + m_abftDuration + m_mbifs);
    }

  /* Initialize Variables */
  m_currentSlotIndex = 0;
  m_remainingSlotsPerABFT = m_ssSlotsPerABFT;

  /* Check if we should contend directly after receiving DMG Beacon */
  if (m_immediateAbft)
    {
      /* If we have already successfully beamformed in previous A-BFT then no need to contend again */
      if (!IsBeamformedTrained ())
        {
          /* Reinitialize variables */
          m_isBeamformingInitiator = false;
          m_isInitiatorTXSS = true; /* DMG-AP always performs TxSS in BTI */
          /* Do the actual association beamforming training */
          DoAssociationBeamformingTraining (m_currentSlotIndex);
        }
    }
  else
    {
      if (m_completedFragmenteBI)
        {
          m_completedFragmenteBI = false;
          /* If we have already successfully beamformed in previous A-BFT then no need to contend again */
          if (!IsBeamformedTrained ())
            {
              /* Reinitialize variables */
              m_isBeamformingInitiator = false;
              m_isInitiatorTXSS = true; /* DMG-AP always performs TxSS in BTI */
              /* Do the actual association beamforming training */
              DoAssociationBeamformingTraining (m_currentSlotIndex);
            }
        }
    }

  /* Start A-BFT access period */
  Simulator::ScheduleNow (&DmgStaWifiMac::StartSectorSweepSlot, this);
}

void
DmgStaWifiMac::EndAssociationBeamformTraining (void)
{
  NS_LOG_FUNCTION (this << m_abftState);
  if (m_abftState == START_RSS_BACKOFF_STATE)
    {
      /* Extract random value for RSS backoff */
      m_rssBackoffRemaining = m_rssBackoffVariable->GetInteger ();
      m_abftState = IN_RSS_BACKOFF_STATE;
      NS_LOG_DEBUG ("Extracted RSS Backoff Value=" << m_rssBackoffRemaining);
    }
  else if (m_abftState == IN_RSS_BACKOFF_STATE)
    {
      /* The responder shall decrement the backoff count by one at the end of each A-BFT period in the
       * following beacon intervals. */
      m_rssBackoffRemaining--;
      NS_LOG_DEBUG ("RSS Remaining Backoff=" << m_rssBackoffRemaining);
      if (m_rssBackoffRemaining == 0)
        {
          NS_LOG_DEBUG ("Completed RSS Backoff");
          m_abftState = WAIT_BEAMFORMING_TRAINING;
        }
    }
}

void
DmgStaWifiMac::DoAssociationBeamformingTraining (uint8_t currentSlotIndex)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("Selected Slot Index=" << uint16_t (m_selectedSlotIndex) <<
                ", A-BFT State=" << m_abftState);
  /* We are in RSS Backoff State and the backoff count equals zero */
  if ((m_abftState == IN_RSS_BACKOFF_STATE) && (m_rssBackoffRemaining == 0))
    {
      /* The responder may re-initiate RSS only during an A-BFT when the backoff count becomes zero. */
      m_abftState = WAIT_BEAMFORMING_TRAINING;
    }
  if ((m_abftState == WAIT_BEAMFORMING_TRAINING) || (m_abftState == FAILED_BEAMFORMING_TRAINING))
    {
      uint8_t slotIndex;  /* The index of the selected slot in the A-BFT period. */
      /* Choose a random SSW Slot to transmit SSW Frames in it */
      a_bftSlot->SetAttribute ("Min", DoubleValue (0));
      a_bftSlot->SetAttribute ("Max", DoubleValue (m_remainingSlotsPerABFT - 1));
      slotIndex = a_bftSlot->GetInteger ();
      NS_LOG_DEBUG ("Local Slot Index=" << uint16_t (slotIndex) <<
                    ", Remaining Slots in the current A-BFT=" << uint16_t (m_remainingSlotsPerABFT));
      m_selectedSlotIndex = slotIndex + currentSlotIndex;
      NS_LOG_DEBUG ("Selected Sector Slot Index=" << uint16_t (m_selectedSlotIndex));
    }
}

void
DmgStaWifiMac::StartSectorSweepSlot (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("DMG STA Starting A-BFT SSW Slot [" << uint16_t (m_currentSlotIndex) << "] at " << Simulator::Now ());
  /* If we have already successfully beamformed in previous A-BFT then no need to contend again */
  if (!IsBeamformedTrained () && (m_selectedSlotIndex == m_currentSlotIndex) &&
     ((m_abftState == WAIT_BEAMFORMING_TRAINING) || (m_abftState == FAILED_BEAMFORMING_TRAINING)))
    {
      Simulator::ScheduleNow (&DmgStaWifiMac::StartAbftResponderSectorSweep, this, GetBssid ());
    }
  m_currentSlotIndex++;
  m_remainingSlotsPerABFT--;
  if (m_remainingSlotsPerABFT > 0)
    {
      /* Schedule the beginning of the next A-BFT Slot */
      Simulator::Schedule (GetSectorSweepSlotTime (m_ssFramesPerSlot), &DmgStaWifiMac::StartSectorSweepSlot, this);
    }
}

void
DmgStaWifiMac::StartAbftResponderSectorSweep (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address << m_isResponderTXSS);
  if (m_dcfManager->CanAccess ())
    {
      m_sectorSweepStarted = Simulator::Now ();
      m_sectorSweepDuration = CalculateSectorSweepDuration (m_ssFramesPerSlot);
      /* Obtain antenna configuration for the highest received SNR to feed it back in SSW-FBCK Field */
      m_feedbackAntennaConfig = GetBestAntennaConfiguration (address, true);
      /* Schedule SSW FBCK Timeout to detect a collision i.e. missing SSW-FBCK */
      Time timeout = GetSectorSweepSlotTime (m_ssFramesPerSlot) - GetMbifs ();
      NS_LOG_DEBUG ("Scheduled SSW-FBCK Timeout Event at " << Simulator::Now () + timeout);
      m_sswFbckTimeout = Simulator::Schedule (timeout, &DmgStaWifiMac::MissedSswFeedback, this);
      /* Now start doing the specified sweeping in the Responder Phase */
      if (m_isResponderTXSS)
        {
          m_codebook->InitiateABFT (address);
          SendRespodnerTransmitSectorSweepFrame (address);
        }
      else
        {
          /* The initiator is switching receive sectors at the same time (Not supported yet) */
        }
    }
  else
    {
      /**
       * HANY: Temporary Solution in case of multiple APs and at the beginning of A-BFT Slot another STA is transmitting
       * we consider it as unsuccessful attempt and we try in another slot.
       * The IEEE 802.11-2016 standard does not specify anything about this case.
       * Check Section (10.38.5 Beamforming in A-BFT)
       */
      NS_LOG_DEBUG ("Medium is busy, contend in another A-BFT slot.");
      FailedRssAttempt ();
    }
}

void
DmgStaWifiMac::FailedRssAttempt (void)
{
  NS_LOG_FUNCTION (this);
  /* Each STA maintains a counter, FailedRSSAttempts, of the consecutive number of times the STA initiates
   * RSS during A-BFTs but does not successfully receive an SSW-Feedback frame as a response. If
   * FailedRSSAttempts exceeds dot11RSSRetryLimit, the STA shall select a backoff count as a random integer
   * drawn from a uniform distribution [0, dot11RSSBackoff), i.e., 0 inclusive through dot11RSSBackoff exclusive. */
  NS_LOG_DEBUG ("Current FailedRssAttemptsCounter=" << m_failedRssAttemptsCounter <<
                ", A-BFT State=" << m_abftState);
  m_failedRssAttemptsCounter++;
  if ((m_failedRssAttemptsCounter <= m_rssAttemptsLimit) && (m_remainingSlotsPerABFT > 0))
    {
      m_abftState = FAILED_BEAMFORMING_TRAINING;
      DoAssociationBeamformingTraining (m_currentSlotIndex - 1);
    }
  else if (m_failedRssAttemptsCounter > m_rssAttemptsLimit)
    {
      m_abftState = START_RSS_BACKOFF_STATE;
      NS_LOG_DEBUG ("Exceeded the number of RSS Attempts. Start RSS Backoff.");
    }
}

void
DmgStaWifiMac::MissedSswFeedback (void)
{
  NS_LOG_FUNCTION (this);
  FailedRssAttempt ();
}

void
DmgStaWifiMac::StartAnnouncementTransmissionInterval (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO (this << "DMG STA Starting ATI at " << Simulator::Now ());
  m_accessPeriod = CHANNEL_ACCESS_ATI;
  /* We started ATI Period we should stay in quasi-omni mode waiting for packets */
  m_codebook->SetReceivingInQuasiOmniMode ();
  Simulator::Schedule (m_atiDuration, &DmgStaWifiMac::StartDataTransmissionInterval, this);
  m_dmgAtiDca->InitiateAtiAccessPeriod (m_atiDuration);
}

void
DmgStaWifiMac::StartDataTransmissionInterval (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("DMG STA Starting DTI at " << Simulator::Now ());
  m_accessPeriod = CHANNEL_ACCESS_DTI;

  /* Initialize DMG Reception */
  m_receivedDmgBeacon = false;

  /* Schedule the beginning of the next BI interval */
  m_dtiStartTime = Simulator::Now ();
//  m_bhiDuration = m_dtiStartTime - m_biStartTime;
  m_dtiDuration = m_beaconInterval - (Simulator::Now () - m_biStartTime);
  Simulator::Schedule (m_dtiDuration, &DmgStaWifiMac::StartBeaconInterval, this);
  NS_LOG_DEBUG ("Next Beacon Interval will start at " << Simulator::Now () + m_dtiDuration);
  m_dtiStarted (GetAddress (), m_dtiDuration);

  /* Send Association Request if we are not assoicated */
  if (!IsAssociated () && IsBeamformedTrained ())
    {
      /* We allow normal DCA for access */
      SetState (WAIT_ASSOC_RESP);
      Simulator::ScheduleNow (&DmgStaWifiMac::SendAssociationRequest, this);
    }

  if (IsBeamformedTrained ())
    {
      /**
        * A STA shall not transmit within a CBAP unless at least one of the following conditions is met:
        * — The value of the CBAP Only field is equal to 1 and the value of the CBAP Source field is equal to 0
        *   within the DMG Parameters field of the DMG Beacon that allocates the CBAP
        * — The STA is a PCP/AP and the value of the CBAP Only field is equal to 1 and the value of the CBAP
        *   Source field is equal to 1 within the DMG Parameters field of the DMG Beacon that allocates the CBAP
        * — The value of the Source AID field of the CBAP is equal to the broadcast AID
        * — The STA’s AID is equal to the value of the Source AID field of the CBAP
        * — The STA’s AID is equal to the value of the Destination AID field of the CBAP
        */
      if (m_isCbapOnly && !m_isCbapSource)
        {
          NS_LOG_INFO ("CBAP allocation only in DTI");
          StartContentionPeriod (BROADCAST_CBAP, m_dtiDuration);
        }
      else
        {
          AllocationField field;
          for (AllocationFieldList::iterator iter = m_allocationList.begin (); iter != m_allocationList.end (); iter++)
            {
              field = (*iter);
              if (field.GetAllocationType () == SERVICE_PERIOD_ALLOCATION)
                {
                  Time spStart = MicroSeconds (field.GetAllocationStart ());
                  Time spLength = MicroSeconds (field.GetAllocationBlockDuration ());

                  NS_ASSERT_MSG (spStart + spLength <= m_dtiDuration, "Allocation should not exceed DTI period.");

                  if (field.GetSourceAid () == m_aid)
                    {
                      uint8_t destAid = field.GetDestinationAid ();
                      Mac48Address destAddress = m_aidMap[destAid];
                      if (field.GetBfControl ().IsBeamformTraining ())
                        {
                          Simulator::Schedule (spStart, &DmgStaWifiMac::StartBeamformingTraining, this, destAid, destAddress, true,
                                               field.GetBfControl ().IsInitiatorTxss (), field.GetBfControl ().IsResponderTxss (), spLength);
                        }
                      else
                        {
                          DataForwardingTableIterator forwardingIterator = m_dataForwardingTable.find (destAddress);
                          if (forwardingIterator == m_dataForwardingTable.end ())
                            {
                              NS_LOG_ERROR ("Did not perform Beamforming Training with " << destAddress);
                              continue;
                            }
                          else
                            {
                              forwardingIterator->second.isCbapPeriod = false;
                            }
                          ScheduleAllocationBlocks (field, SOURCE_STA);
                        }
                    }
                  else if ((field.GetSourceAid () == AID_BROADCAST) && (field.GetDestinationAid () == AID_BROADCAST))
                    {
                      /* The PCP/AP may create SPs in its beacon interval with the source and destination AID
                       * subfields within an Allocation field set to 255 to prevent transmissions during
                       * specific periods in the beacon interval. This period can used for Dynamic Allocation
                       * of service peridos (Polling) */
                      NS_LOG_INFO ("No transmission is allowed from " << spStart << " till " << spStart + spLength);
                    }
                  else if ((field.GetDestinationAid () == m_aid) || (field.GetDestinationAid () == AID_BROADCAST))
                    {
                      /* The STA identified by the Destination AID field in the Extended Schedule element
                       * should be in the receive state for the duration of the SP in order to receive
                       * transmissions from the source DMG STA. */
                      uint8_t sourceAid = field.GetSourceAid ();
                      Mac48Address sourceAddress = m_aidMap[sourceAid];
                      if (field.GetBfControl ().IsBeamformTraining ())
                        {
                          Simulator::Schedule (spStart, &DmgStaWifiMac::StartBeamformingTraining, this, sourceAid, sourceAddress, false,
                                               field.GetBfControl ().IsInitiatorTxss (), field.GetBfControl ().IsResponderTxss (), spLength);
                        }
                      else
                        {
                          ScheduleAllocationBlocks (field, DESTINATION_STA);
                        }
                    }
                  else if ((field.GetSourceAid () != m_aid) && (field.GetDestinationAid () != m_aid))
                    {
                      ScheduleAllocationBlocks (field, RELAY_STA);
                    }
                }
              else if ((field.GetAllocationType () == CBAP_ALLOCATION) &&
                      ((field.GetSourceAid () == AID_BROADCAST) ||
                       (field.GetSourceAid () == m_aid) || (field.GetDestinationAid () == m_aid)))

                {
                  Simulator::Schedule (MicroSeconds (field.GetAllocationStart ()), &DmgStaWifiMac::StartContentionPeriod, this,
                                       field.GetAllocationID (), MicroSeconds (field.GetAllocationBlockDuration ()));
                }
            }
        }
    }
}

void
DmgStaWifiMac::ScheduleAllocationBlocks (AllocationField &field, STA_ROLE role)
{
  NS_LOG_FUNCTION (this);
  Time spStart = MicroSeconds (field.GetAllocationStart ());
  Time spLength = MicroSeconds (field.GetAllocationBlockDuration ());
  Time spPeriod = MicroSeconds (field.GetAllocationBlockPeriod ());
  uint8_t blocks = field.GetNumberOfBlocks ();
  if (spPeriod > 0)
    {
      /* We allocate multiple blocks of this allocation as in (9.33.6 Channel access in scheduled DTI) */
      /* A_start + (i – 1) × A_period */
      for (uint8_t i = 0; i < blocks; i++)
        {
          NS_LOG_INFO ("Schedule SP Block [" << i << "] at " << spStart << " till " << spStart + spLength);
          Simulator::Schedule (spStart, &DmgStaWifiMac::InitiateAllocationPeriod, this,
                               field.GetAllocationID (), field.GetSourceAid (), field.GetDestinationAid (), spLength, role);
          spStart += spLength + spPeriod + GUARD_TIME;
        }
    }
  else
    {
      /* Special case when Allocation Block Period=0, i.e. consecutive blocks *
       * We try to avoid schedulling multiple blocks, so we schedule one big block */
      spLength = spLength * blocks;
      Simulator::Schedule (spStart, &DmgStaWifiMac::InitiateAllocationPeriod, this,
                           field.GetAllocationID (), field.GetSourceAid (), field.GetDestinationAid (), spLength, role);
    }
}

void
DmgStaWifiMac::InitiateAllocationPeriod (AllocationID id, uint8_t srcAid, uint8_t dstAid, Time spLength, STA_ROLE role)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (id) << static_cast<uint16_t> (srcAid)
                   << static_cast<uint16_t> (dstAid) << spLength << role);

  /* Relay Pair */
  REDS_PAIR redsPair = std::make_pair (srcAid, dstAid);
  RELAY_LINK_MAP_ITERATOR it = m_relayLinkMap.find (redsPair);
  bool protectedAllocation = (it != m_relayLinkMap.end ());

  if (role == SOURCE_STA)
    {
      Mac48Address dstAddress = m_aidMap[dstAid];
      if (protectedAllocation)
        {
          RELAY_LINK_INFO info = it->second;
          NS_LOG_INFO ("Initiating relay periods for the source REDS");
          /* Schedule events related to the beginning and end of relay period */
          Simulator::ScheduleNow (&DmgStaWifiMac::InitiateRelayPeriods, this, info);
          Simulator::Schedule (spLength, &DmgStaWifiMac::EndRelayPeriods, this, redsPair);

          /* Schedule events related to the intervals within the relay period */
          if ((info.transmissionLink == RELAY_LINK) && (info.rdsDuplexMode == 0))
            {
              Simulator::ScheduleNow (&DmgStaWifiMac::StartHalfDuplexRelay,
                                      this, id, spLength, true);
            }
          else if ((info.transmissionLink == DIRECT_LINK) && (info.rdsDuplexMode == 0))
            {
              /* Schedule the beginning of this service period */
              Simulator::ScheduleNow (&DmgStaWifiMac::StartServicePeriod,
                                      this, id, spLength, dstAid, dstAddress, true);
            }
          else if (info.rdsDuplexMode == 1)
            {
              Simulator::ScheduleNow (&DmgStaWifiMac::StartFullDuplexRelay,
                                   this, id, spLength, dstAid, dstAddress, true);
            }
          Simulator::Schedule (spLength, &DmgStaWifiMac::EndServicePeriod, this);
        }
      else
        {
          /* No relay link has been established so schedule normal service period */
          Simulator::ScheduleNow (&DmgStaWifiMac::StartServicePeriod, this,
                                  id, spLength, dstAid, dstAddress, true);
          Simulator::Schedule (spLength, &DmgStaWifiMac::EndServicePeriod, this);
        }
    }
  else if (role == DESTINATION_STA)
    {
      Mac48Address srcAddress = m_aidMap[srcAid];
      if (protectedAllocation)
        {
          RELAY_LINK_INFO info = it->second;
          NS_LOG_INFO ("Initiating relay periods for the destination REDS");
          /* Schedule events related to the beginning and end of relay period */
          Simulator::ScheduleNow (&DmgStaWifiMac::InitiateRelayPeriods, this, info);
          Simulator::Schedule (spLength, &DmgStaWifiMac::EndRelayPeriods, this, redsPair);

          /* Schedule events related to the intervals within the relay period */
          if ((info.transmissionLink == RELAY_LINK) && (info.rdsDuplexMode == 0))
            {
              Simulator::ScheduleNow (&DmgStaWifiMac::StartHalfDuplexRelay, this,
                                      id, spLength, false);
            }
          else if ((info.transmissionLink == DIRECT_LINK) && (info.rdsDuplexMode == 0))
            {
              Simulator::ScheduleNow (&DmgStaWifiMac::StartServicePeriod, this,
                                      id, spLength, srcAid, srcAddress, false);
            }
          else if (info.rdsDuplexMode == 1)
            {
              /* Schedule Data Sensing Timeout to detect missing frame transmission */
              Simulator::ScheduleNow (&DmgStaWifiMac::StartFullDuplexRelay, this,
                                      id, spLength, srcAid, srcAddress, false);
              Simulator::Schedule (MicroSeconds (m_relayDataSensingTime), &DmgStaWifiMac::RelayDataSensingTimeout, this);

            }
        }
      else
        {
          Simulator::ScheduleNow (&DmgStaWifiMac::StartServicePeriod, this,
                                  id, spLength, srcAid, srcAddress, false);
          Simulator::Schedule (spLength, &DmgStaWifiMac::EndServicePeriod, this);
        }
    }
  else if ((role == RELAY_STA) && (protectedAllocation))
    {
      /* We protect this SP allocation by this RDS */
      RELAY_LINK_INFO info = it->second;
      NS_LOG_INFO ("Initiating relay periods for the RDS");

      /* We are the RDS */
      Simulator::ScheduleNow (&DmgStaWifiMac::SwitchToRelayOpertionalMode, this);
      Simulator::Schedule (spLength, &DmgStaWifiMac::RelayOperationTimeout, this);

      if (info.rdsDuplexMode == 1) // FD-AF
        {
          NS_LOG_INFO ("Protecting the SP between by an RDS in FD-AF Mode: Source AID=" << info.srcRedsAid <<
                       " and Destination AID=" << info.dstRedsAid);
          ANTENNA_CONFIGURATION_TX antennaConfigTxSrc = m_bestAntennaConfig[info.srcRedsAddress].first;
          ANTENNA_CONFIGURATION_TX antennaConfigTxDst = m_bestAntennaConfig[info.dstRedsAddress].first;
          Simulator::ScheduleNow (&DmgWifiPhy::ActivateRdsOpereation, StaticCast<DmgWifiPhy> (m_phy),
                                  antennaConfigTxSrc.first, antennaConfigTxSrc.second,
                                  antennaConfigTxDst.first, antennaConfigTxDst.second);
          Simulator::Schedule (spLength, &DmgWifiPhy::SuspendRdsOperation, StaticCast<DmgWifiPhy> (m_phy));
        }
      else // HD-DF
        {
          NS_LOG_INFO ("Protecting the SP by an RDS in HD-DF Mode: Source AID=" << info.srcRedsAid <<
                       " and Destination AID=" << info.dstRedsAid);

          /* Schedule events related to the beginning and end of relay period */
          Simulator::ScheduleNow (&DmgStaWifiMac::InitiateRelayPeriods, this, info);
          Simulator::Schedule (spLength, &DmgStaWifiMac::EndRelayPeriods, this, redsPair);

          /* Schedule an event to direct the antennas toward the source REDS */
          Simulator::ScheduleNow (&DmgStaWifiMac::SteerAntennaToward, this, info.srcRedsAddress);

          /* Schedule half duplex relay periods */
          Simulator::ScheduleNow (&DmgStaWifiMac::StartHalfDuplexRelay, this, id, spLength, false);
        }
    }
}

void
DmgStaWifiMac::InitiateRelayPeriods (RELAY_LINK_INFO &info)
{
  NS_LOG_FUNCTION (this);
  m_periodProtected = true;
  m_relayLinkInfo = info;
  /* Schedule peridos assoicated to the transmission link */
  if ((m_relayLinkInfo.transmissionLink == RELAY_LINK) && (m_relayLinkInfo.rdsDuplexMode == 0))
    {
      m_firstPeriod = Simulator::Schedule (MicroSeconds (m_relayLinkInfo.relayFirstPeriod),
                                           &DmgStaWifiMac::RelayFirstPeriodTimeout, this);
    }
  else if ((m_relayLinkInfo.transmissionLink == DIRECT_LINK) || (m_relayLinkInfo.rdsDuplexMode == 1))
    {
      m_linkChangeInterval = Simulator::Schedule (MicroSeconds (m_relayLinkInfo.relayLinkChangeInterval),
                                                  &DmgStaWifiMac::RelayLinkChangeIntervalTimeout, this);
    }
}

void
DmgStaWifiMac::EndRelayPeriods (REDS_PAIR &pair)
{
  NS_LOG_FUNCTION (this);
  if (m_periodProtected)
    {
      m_periodProtected = false;
      /* Check if we need to remove relay link at the end of the SP allocation protected by a relay */
      if (m_relayLinkInfo.tearDownRelayLink)
        {
          RemoveRelayEntry (pair.first, pair.second);
          if (m_aid == m_relayLinkInfo.srcRedsAid)
            {
              /* Switching back to the direct link so change addresses of all the packets stored in MacLow and EdcaTxopN */
              m_low->ChangeAllocationPacketsAddress (m_currentAllocationID, m_relayLinkInfo.dstRedsAddress);
              m_edca[AC_BE]->GetQueue ()->ChangePacketsReceiverAddress (m_relayLinkInfo.selectedRelayAddress,
                                                                        m_relayLinkInfo.dstRedsAddress);
            }
        }
      else
        {
          /* Store information related to the relay operation mode */
          m_relayLinkMap[pair] = m_relayLinkInfo;
        }
    }
}

void
DmgStaWifiMac::RegisterRelaySelectorFunction (ChannelMeasurementCallback callback)
{
  m_channelMeasurementCallback = callback;
}

void
DmgStaWifiMac::RelayLinkChangeIntervalTimeout (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("DMG STA Starting Link Change Interval at " << Simulator::Now ());
  if (m_relayLinkInfo.rdsDuplexMode == 1) // FD-AF
    {
      if (CheckTimeAvailabilityForPeriod (GetRemainingAllocationTime (), MicroSeconds (m_relayLinkInfo.relayLinkChangeInterval)))
        {
          /* Schedule the next Link Change Interval */
          m_linkChangeInterval = Simulator::Schedule (MicroSeconds (m_relayLinkInfo.relayLinkChangeInterval),
                                                      &DmgStaWifiMac::RelayLinkChangeIntervalTimeout, this);

          /* Schedule Data Sensing Timeout Event at the destination REDS */
          if (m_relayLinkInfo.dstRedsAid == m_aid)
            {
              m_relayDataExchanged = false;

              /* Schedule Data Sensing Timeout to detect missing frame transmission */
              Simulator::Schedule (MicroSeconds (m_relayLinkInfo.relayDataSensingTime),
                                   &DmgStaWifiMac::RelayDataSensingTimeout, this);
            }
          else if ((m_relayLinkInfo.switchTransmissionLink) && (m_relayLinkInfo.srcRedsAid == m_aid))
            {
              /* If the source REDS decides to change the link at the start of the following Link Change Interval period and
               * the Normal mode is used, the source REDS shall start its frame transmission after Data Sensing Time from
               * the start of the following Link Change Interval period. */
              m_relayLinkInfo.switchTransmissionLink = false;
              if (m_relayLinkInfo.transmissionLink == DIRECT_LINK)
                {
                  m_relayLinkInfo.transmissionLink = RELAY_LINK;
                }
              else
                {
                  m_relayLinkInfo.transmissionLink = RELAY_LINK;
                }
              m_transmissionLinkChanged (GetAddress (), m_relayLinkInfo.transmissionLink);
              SteerAntennaToward (m_relayLinkInfo.selectedRelayAddress);
              Simulator::Schedule (MicroSeconds (m_relayLinkInfo.relayDataSensingTime),
                                   &DmgStaWifiMac::ResumeServicePeriodTransmission, this);
            }
        }
    }
  else // HD-DF
    {
      if (m_relayLinkInfo.switchTransmissionLink)
        {
          /* We are using the direct link and we decided to switch to the relay link */
          m_relayLinkInfo.switchTransmissionLink = false;
          m_relayLinkInfo.transmissionLink = RELAY_LINK;
          SuspendServicePeriodTransmission ();

          if (CheckTimeAvailabilityForPeriod (GetRemainingAllocationTime (), MicroSeconds (m_relayLinkInfo.relayFirstPeriod)))
            {
              if (m_relayLinkInfo.srcRedsAid == m_aid)
                {
                  NS_LOG_DEBUG ("We are the source REDS and we want to switch to the relay link");
                  /* If the source REDS decides to change to the relay link at the start of the following Link Change
                   * Interval period, the source REDS shall start its frame transmission at the start of the following
                   * Link Change Interval period. */
                  m_relayLinkInfo.relayForwardingActivated = true;
//                  m_edca[AC_BE]->ChangePacketsAddress (m_relayLinkInfo.dstRedsAddress, m_relayLinkInfo.selectedRelayAddress);
                  m_dataForwardingTable[m_relayLinkInfo.dstRedsAddress].nextHopAddress = m_relayLinkInfo.selectedRelayAddress;
                }
              /* Special case for First Period after link switching */
              StartRelayFirstPeriodAfterSwitching ();
              m_firstPeriod = Simulator::Schedule (MicroSeconds (m_relayLinkInfo.relayFirstPeriod),
                                                   &DmgStaWifiMac::RelayFirstPeriodTimeout, this);
            }
        }
      else
        {
          /* Check how much time left in the current service period protected by the relay */
          if (CheckTimeAvailabilityForPeriod (GetRemainingAllocationTime (), MicroSeconds (m_relayLinkInfo.relayLinkChangeInterval)))
            {
              /* Schedule the next Link Change Interval */
              m_linkChangeInterval = Simulator::Schedule (MicroSeconds (m_relayLinkInfo.relayLinkChangeInterval),
                                                          &DmgStaWifiMac::RelayLinkChangeIntervalTimeout, this);
            }
        }
    }
}

bool
DmgStaWifiMac::CheckTimeAvailabilityForPeriod (Time servicePeriodDuration, Time partialDuration)
{
  return ((servicePeriodDuration - partialDuration) >= Seconds (0));
}

void
DmgStaWifiMac::StartFullDuplexRelay (AllocationID allocationID, Time length,
                                     uint8_t peerAid, Mac48Address peerAddress, bool isSource)
{
  NS_LOG_FUNCTION (this << length << static_cast<uint16_t> (peerAid) << peerAddress << isSource);
  m_currentAllocationID = allocationID;
  m_currentAllocation = SERVICE_PERIOD_ALLOCATION;
  m_currentAllocationLength = length;
  m_allocationStarted = Simulator::Now ();
  m_peerStationAid = peerAid;
  m_peerStationAddress = peerAddress;
  m_moreData = true;
  m_servicePeriodStartedCallback (GetAddress (), peerAddress);
  /* Check current transmission link */
  if (m_relayLinkInfo.transmissionLink == DIRECT_LINK)
    {
      SteerAntennaToward (peerAddress);
    }
  else if (m_relayLinkInfo.transmissionLink == RELAY_LINK)
    {
      SteerAntennaToward (m_relayLinkInfo.selectedRelayAddress);
    }
  m_edca[AC_BE]->StartAllocationPeriod (SERVICE_PERIOD_ALLOCATION, allocationID, peerAddress, length);
  if (isSource)
    {
      m_edca[AC_BE]->InitiateTransmission ();
    }
}

void
DmgStaWifiMac::StartHalfDuplexRelay (AllocationID allocationID, Time servicePeriodLength, bool firstPertiodInitiator)
{
  NS_LOG_FUNCTION (this << servicePeriodLength << firstPertiodInitiator);
  m_currentAllocationID = allocationID;
  m_currentAllocation = SERVICE_PERIOD_ALLOCATION;
  m_currentAllocationLength = servicePeriodLength;
  m_allocationStarted = Simulator::Now ();
  if ((!m_relayLinkInfo.relayForwardingActivated) && (m_relayLinkInfo.srcRedsAid == m_aid))
    {
      m_relayLinkInfo.relayForwardingActivated = true;
      m_dataForwardingTable[m_relayLinkInfo.dstRedsAddress].nextHopAddress = m_relayLinkInfo.selectedRelayAddress;
    }
  if (m_relayLinkInfo.transmissionLink == RELAY_LINK)
    {
      StartRelayFirstPeriod ();
    }
}

void
DmgStaWifiMac::StartRelayFirstPeriodAfterSwitching (void)
{
  NS_LOG_FUNCTION (this);
  if (m_relayLinkInfo.srcRedsAid == m_aid)
    {
      SteerAntennaToward (m_relayLinkInfo.selectedRelayAddress);
      m_edca[AC_BE]->StartAllocationPeriod (SERVICE_PERIOD_ALLOCATION, m_currentAllocationID, m_relayLinkInfo.selectedRelayAddress,
                                            MicroSeconds (m_relayLinkInfo.relayFirstPeriod));
      m_edca[AC_BE]->AllowChannelAccess ();
    }
  else if (m_relayLinkInfo.selectedRelayAid == m_aid)
    {
      SteerAntennaToward (m_relayLinkInfo.srcRedsAddress);
    }
  else if (m_relayLinkInfo.dstRedsAid == m_aid)
    {
      SteerAntennaToward (m_relayLinkInfo.srcRedsAddress);
    }
}

void
DmgStaWifiMac::StartRelayFirstPeriod (void)
{
  NS_LOG_FUNCTION (this);
  if (m_relayLinkInfo.srcRedsAid == m_aid)
    {
      SteerAntennaToward (m_relayLinkInfo.selectedRelayAddress);
      /* Restore previously suspended transmission in LowMac */
      m_low->RestoreAllocationParameters (m_currentAllocationID);
      m_edca[AC_BE]->StartAllocationPeriod (SERVICE_PERIOD_ALLOCATION, m_currentAllocationID, m_relayLinkInfo.selectedRelayAddress,
                                            MicroSeconds (m_relayLinkInfo.relayFirstPeriod));
      m_edca[AC_BE]->InitiateTransmission ();
    }
  else if (m_relayLinkInfo.selectedRelayAid == m_aid)
    {
      SteerAntennaToward (m_relayLinkInfo.srcRedsAddress);
    }
  else if (m_relayLinkInfo.dstRedsAid == m_aid)
    {
      /* The destination REDS shall switch to the direct link at each First Period and listen to the medium toward the source REDS.
       * If the destination REDS receives a valid frame from the source REDS, the destination REDS shall remain on the direct link
       * and consider the Link Change Interval to begin at the start of the First Period. Otherwise, the destination  REDS shall
       * change the link at the start of the next Second Period and attempt to receive frames from the source REDS through the RDS.
       * If the active link is the relay link and the More Data field in the last frame  received from the RDS is equal to 0, then
       * the destination REDS shall not switch to the direct link even if it does not receive any frame during the Second Period. */
      SteerAntennaToward (m_relayLinkInfo.srcRedsAddress);
    }
}

void
DmgStaWifiMac::StartRelaySecondPeriod (void)
{
  NS_LOG_FUNCTION (this);
  if (m_relayLinkInfo.srcRedsAid == m_aid)
    {
      SteerAntennaToward (m_relayLinkInfo.dstRedsAddress);
    }
  else if (m_relayLinkInfo.selectedRelayAid == m_aid)
    {
      SteerAntennaToward (m_relayLinkInfo.dstRedsAddress);
      m_edca[AC_BE]->StartAllocationPeriod (SERVICE_PERIOD_ALLOCATION, m_currentAllocationID, m_relayLinkInfo.dstRedsAddress,
                                            MicroSeconds (m_relayLinkInfo.relaySecondPeriod));
      m_edca[AC_BE]->InitiateTransmission ();
    }
  else if (m_relayLinkInfo.dstRedsAid == m_aid)
    {
      /* The destination REDS shall change the link at the start of the next Second Period and attempt to receive frames from the
       * source REDS through the RDS. */
      SteerAntennaToward (m_relayLinkInfo.selectedRelayAddress);
    }
}

void
DmgStaWifiMac::SuspendRelayPeriod (void)
{
  NS_LOG_FUNCTION (this);
  m_edca[AC_BE]->DisableChannelAccess ();
  m_edca[AC_BE]->EndAllocationPeriod ();
}

void
DmgStaWifiMac::RelayFirstPeriodTimeout (void)
{
  NS_LOG_FUNCTION (this);
  if (CheckTimeAvailabilityForPeriod (GetRemainingAllocationTime (), MicroSeconds (m_relayLinkInfo.relaySecondPeriod)))
    {
      /* Data has been exchanged during the first period, so schedule Second Period Timer */
      m_secondPeriod = Simulator::Schedule (MicroSeconds (m_relayLinkInfo.relaySecondPeriod),
                                            &DmgStaWifiMac::RelaySecondPeriodTimeout, this);
      if (m_relayLinkInfo.srcRedsAid == m_aid)
        {
          /* We are the source REDS and the first period has expired so suspend its transmission */
          SuspendRelayPeriod ();
          StartRelaySecondPeriod ();
        }
      else if (m_relayLinkInfo.selectedRelayAid == m_aid)
        {
          /* We are the RDS and the first period has expired so initiate transmission in the second period */
          StartRelaySecondPeriod ();
        }
      else if (m_relayLinkInfo.dstRedsAid == m_aid)
        {
          /* We are the destination REDS and the first period has expired so prepare for recepition in the second period  */
          StartRelaySecondPeriod ();
        }
    }
}

void
DmgStaWifiMac::RelaySecondPeriodTimeout (void)
{
  NS_LOG_FUNCTION (this);
  if (!m_relayLinkInfo.switchTransmissionLink)
    {
      if (CheckTimeAvailabilityForPeriod (GetRemainingAllocationTime (), MicroSeconds (m_relayLinkInfo.relayFirstPeriod)))
        {
          /* Data has been exchanged during the first period, so schedule Second Period Timer */
          m_firstPeriod = Simulator::Schedule (MicroSeconds (m_relayLinkInfo.relayFirstPeriod),
                                               &DmgStaWifiMac::RelayFirstPeriodTimeout, this);
          if (m_relayLinkInfo.srcRedsAid == m_aid)
            {
              /* We are the source REDS and the second period has expired so start transmission in the first period */
              StartRelayFirstPeriod ();
            }
          else if (m_relayLinkInfo.selectedRelayAid == m_aid)
            {
              /* We are the RDS and the second period has expired so prepare for recepition in the first period */
              SuspendRelayPeriod ();
              StartRelayFirstPeriod ();
            }
          else if (m_relayLinkInfo.dstRedsAid == m_aid)
            {
              /* We are the destination REDS and the second period has expired so suspend operations */
              StartRelayFirstPeriod ();
            }
        }
    }
  else
    {
      /* If a link change to the direct link occurs, the source REDS shall start to transmit a frame using the direct link
       * at the end of the Second Period when the Link Change Interval begins. */
      m_relayLinkInfo.switchTransmissionLink = false;
      m_relayLinkInfo.transmissionLink = DIRECT_LINK;
      SuspendServicePeriodTransmission ();

      /* Check how much time left in the current service period protected by the relay */
      if (CheckTimeAvailabilityForPeriod (GetRemainingAllocationTime (), MicroSeconds (m_relayLinkInfo.relayLinkChangeInterval)))
        {
          /* Schedule the next Link Change Interval */
          m_linkChangeInterval = Simulator::Schedule (MicroSeconds (m_relayLinkInfo.relayLinkChangeInterval),
                                                      &DmgStaWifiMac::RelayLinkChangeIntervalTimeout, this);
        }
    }
}

void
DmgStaWifiMac::MissedAck (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << hdr);
  if (m_periodProtected &&
      (hdr.GetAddr1 () == m_relayLinkInfo.dstRedsAddress) &&
      (m_relayLinkInfo.rdsDuplexMode == true))
    {
      /* If a source REDS transmits a frame to the destination REDS via the direct link but does not receive an
       * expected ACK frame or BA frame from the destination REDS during a Link Change Interval period, the
       * source REDS should change the link used for frame transmission at the start of the following Link Change
       * Interval period and use the RDS to forward frames to the destination REDS. */
      m_relayLinkInfo.switchTransmissionLink = true;
      SuspendServicePeriodTransmission ();
    }
}

void
DmgStaWifiMac::RelayDataSensingTimeout (void)
{
  NS_LOG_FUNCTION (this << m_relayDataExchanged << m_dcfManager->IsReceiving () << m_moreData);
  if (m_relayLinkInfo.rdsDuplexMode == 1) // FD-AF
    {
      if ((!m_relayDataExchanged) && (!m_dcfManager->IsReceiving ()) && m_moreData)
        {
          m_relayLinkInfo.switchTransmissionLink = true;
          if (m_relayLinkInfo.transmissionLink == DIRECT_LINK)
            {
              /* In the Normal mode, if the destination REDS does not receive a valid frame from the source REDS within
               * Data Sensing Time after the start of a Link Change Interval, the destination REDS shall immediately change
               * the link to attempt to receive frames from the source REDS through the RDS. If the More Data field in the
               * last frame received from the source REDS is equal to 0, then the destination REDS shall not switch to the
               * link in the next Link Change Interval period even if it does not receive a frame during the Data Sensing
               * Time. An example of frame transfer under Normal mode with FD-AF RDS is illustrated in Figure 9-77. */
              NS_LOG_DEBUG ("Destinations REDS did not receive frames during data sensing interval so switch to the relay link");
              m_relayLinkInfo.transmissionLink = RELAY_LINK;
              SteerAntennaToward (m_relayLinkInfo.selectedRelayAddress);
            }
          else
            {
              NS_LOG_DEBUG ("Destinations REDS did not receive frames during data sensing interval so switch to the direct link");
              m_relayLinkInfo.transmissionLink = DIRECT_LINK;
              SteerAntennaToward (m_relayLinkInfo.srcRedsAddress);
            }
          m_transmissionLinkChanged (GetAddress (), m_relayLinkInfo.transmissionLink);
        }
    }
}

void
DmgStaWifiMac::SwitchTransmissionLink (uint8_t srcAid, uint8_t dstAid)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (srcAid) << static_cast<uint16_t> (dstAid));
  REDS_PAIR redsPair = std::make_pair (srcAid, dstAid);
  RELAY_LINK_MAP_ITERATOR it = m_relayLinkMap.find (redsPair);
  if (it != m_relayLinkMap.end ())
    {
      /* Check if we are currently in a service period being protected by an RDS */
      if (m_periodProtected && (srcAid == m_relayLinkInfo.srcRedsAid) && (dstAid == m_relayLinkInfo.dstRedsAid))
        {
          m_relayLinkInfo.switchTransmissionLink = true;
        }
      else
        {
          RELAY_LINK_INFO info = it->second;
          if (info.transmissionLink == DIRECT_LINK)
            {
              info.transmissionLink = RELAY_LINK;
            }
          else
            {
              info.transmissionLink = DIRECT_LINK;
            }
          it->second = info;
        }
    }
}

void
DmgStaWifiMac::SwitchToRelayOpertionalMode (void)
{
  NS_LOG_FUNCTION (this);
  m_relayMode = true;
}

void
DmgStaWifiMac::RelayOperationTimeout (void)
{
  NS_LOG_FUNCTION (this);
  m_relayMode = false;
}

void
DmgStaWifiMac::BeamLinkMaintenanceTimeout (void)
{
  NS_LOG_FUNCTION (this);
  if (!m_spSource)
    {
      /* Following the expiration of the beamlink maintenance time (specified by the current value of the
       * dot11BeamLinkMaintenanceTime variable), the destination DMG STA of the SP shall configure its receive
       * antenna to a quasi-omni antenna pattern for the remainder of the SP and during any SP following the
       * expiration of the beamlink maintenance time. */
      m_codebook->SetReceivingInQuasiOmniMode ();
    }
  DmgWifiMac::BeamLinkMaintenanceTimeout ();
}

void
DmgStaWifiMac::BrpSetupCompleted (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
}

void
DmgStaWifiMac::NotifyBrpPhaseCompleted (void)
{
  NS_LOG_FUNCTION (this);
}

void
DmgStaWifiMac::RequestInformation (Mac48Address stationAddress, WifiInformationElementIdList &list)
{
  NS_LOG_FUNCTION (this << stationAddress);
  /* Obtain Information about the node like DMG Capabilities and AID */
  ExtInformationRequest requestHdr;
  Ptr<RequestElement> requestElement = Create<RequestElement> ();
  requestElement->AddListOfRequestedElemetID (list);

  requestHdr.SetSubjectAddress (stationAddress);
  requestHdr.SetRequestInformationElement (requestElement);
  SendInformationRequest (GetBssid (), requestHdr);
}

void
DmgStaWifiMac::RequestRelayInformation (Mac48Address stationAddress)
{
  NS_LOG_FUNCTION (this << stationAddress);
  /* Obtain Information about the node like DMG Capabilities and AID */
  ExtInformationRequest requestHdr;
  Ptr<RequestElement> requestElement = Create<RequestElement> ();
  requestElement->AddRequestElementID (IE_DMG_CAPABILITIES);
  requestElement->AddRequestElementID (IE_RELAY_CAPABILITIES);

  requestHdr.SetSubjectAddress (stationAddress);
  requestHdr.SetRequestInformationElement (requestElement);
  SendInformationRequest (GetBssid (), requestHdr);
}

/* Directional Channel Measurement */

void
DmgStaWifiMac::StartChannelQualityMeasurement (Ptr<DirectionalChannelQualityRequestElement> element)
{
  NS_LOG_FUNCTION (this << element);
  if (element->GetMeasurementMethod () == ANIPI)
    {
      /* We steer the antenna towards the peer station as in 10.31.2 IEEE 802.11ad */
      Mac48Address peerStation = m_aidMap[element->GetAid ()];
      SteerAntennaToward (peerStation);
      /* Disable channel access in case (Extra protection) */
      m_edca[AC_BE]->DisableChannelAccess ();
      m_dcfManager->DisableChannelAccess ();
    }
  m_reqElem = element;
  StaticCast<DmgWifiPhy> (m_phy)->StartMeasurement (element->GetMeasurementDuration (), element->GetNumberOfTimeBlocks ());
}

void
DmgStaWifiMac::ReportChannelQualityMeasurement (TimeBlockMeasurementList list)
{
  NS_LOG_FUNCTION (this);
  Ptr<DirectionalChannelQualityReportElement> reportElem = Create<DirectionalChannelQualityReportElement> ();
  reportElem->SetAid (m_reqElem->GetAid ());
  reportElem->SetChannelNumber (m_reqElem->GetChannelNumber ());
  reportElem->SetMeasurementDuration (m_reqElem->GetMeasurementDuration ());
  reportElem->SetMeasurementMethod (m_reqElem->GetMeasurementMethod ());
  reportElem->SetNumberOfTimeBlocks(m_reqElem->GetNumberOfTimeBlocks ());
  /* Add obtained measurement results to the report */
  for (TimeBlockMeasurementListCI it = list.begin (); it != list.end (); it++)
    {
      reportElem->AddTimeBlockMeasurement ((*it));
    }
  /* Send Directional Channel Quality Report to the PCP/AP */
  SendDirectionalChannelQualityReport (reportElem);
}

void
DmgStaWifiMac::SendDirectionalChannelQualityReport (Ptr<DirectionalChannelQualityReportElement> element)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (GetBssid ());
  hdr.SetAddr2 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  RadioMeasurementReport reportHdr;
  reportHdr.SetDialogToken (0);
  reportHdr.AddMeasurementReportElement (element);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.radioMeasurementAction = WifiActionHeader::RADIO_MEASUREMENT_REPORT;
  actionHdr.SetAction (WifiActionHeader::RADIO_MEASUREMENT, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (reportHdr);
  packet->AddHeader (actionHdr);

  m_dca->Queue (packet, hdr);
}

void
DmgStaWifiMac::ForwardActionFrame (Mac48Address to, WifiActionHeader &actionHdr, Header &actionBody)
{
  NS_LOG_FUNCTION (this << to);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetBssid ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (actionBody);
  packet->AddHeader (actionHdr);
  m_dca->Queue (packet, hdr);
}

Ptr<RelayTransferParameterSetElement>
DmgStaWifiMac::GetRelayTransferParameterSet (void) const
{
  Ptr<RelayTransferParameterSetElement> element = Create<RelayTransferParameterSetElement> ();
  element->SetDuplexMode (m_rdsDuplexMode);
  element->SetCooperationMode (false);              /* Link Switching Type only */
  element->SetTxMode (false);                       /* Normal mode */
  element->SetLinkChangeInterval (m_relayLinkChangeInterval);
  element->SetDataSensingTime (m_relayDataSensingTime);
  element->SetFirstPeriod (m_relayFirstPeriod);     /* Set the duration of the first period for HD-DF mode */
  element->SetSecondPeriod (m_relaySecondPeriod);   /* Set the duration of the second period for HD-DF mode */
  return element;
}

/**
 * Functions for Relay Discovery/Selection/RLS/Tear Down
 */
void
DmgStaWifiMac::SendChannelMeasurementRequest (Mac48Address to, uint8_t token)
{
  NS_LOG_FUNCTION (this << to << token);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetBssid ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  ExtMultiRelayChannelMeasurementRequest requestHdr;
  requestHdr.SetDialogToken (token);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.dmgAction = WifiActionHeader::DMG_MULTI_RELAY_CHANNEL_MEASUREMENT_REQUEST;
  actionHdr.SetAction (WifiActionHeader::DMG, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (requestHdr);
  packet->AddHeader (actionHdr);

  m_dca->Queue (packet, hdr);
}

void
DmgStaWifiMac::SendChannelMeasurementReport (Mac48Address to, uint8_t token, ChannelMeasurementInfoList &measurementList)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetBssid ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  ExtMultiRelayChannelMeasurementReport responseHdr;
  responseHdr.SetDialogToken (token);
  responseHdr.SetChannelMeasurementList (measurementList);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.dmgAction = WifiActionHeader::DMG_MULTI_RELAY_CHANNEL_MEASUREMENT_REPORT;
  actionHdr.SetAction (WifiActionHeader::DMG, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (responseHdr);
  packet->AddHeader (actionHdr);

  m_dca->Queue (packet, hdr);
}

void
DmgStaWifiMac::StartRelayDiscovery (Mac48Address stationAddress)
{
  NS_LOG_FUNCTION (this << stationAddress);
  /* Establish Relay with specific DMG STA */
  InformationMapIterator it = m_informationMap.find (stationAddress);
  if (it != m_informationMap.end ())
    {
      /* We already have information about the DMG STA */
      StationInformation info = static_cast<StationInformation> (it->second);
      /* Check if the remote DMG STA is Relay Capable */
      Ptr<RelayCapabilitiesElement> capabilitiesElement = StaticCast<RelayCapabilitiesElement> (info.second[IE_RELAY_CAPABILITIES]);
      if (capabilitiesElement != 0)
        {
          RelayCapabilitiesInfo capabilitiesInfo = capabilitiesElement->GetRelayCapabilitiesInfo ();
          if (capabilitiesInfo.GetRelayUsability ())
            {
              /* Initialize Relay variables */
              m_relayLinkInfo.srcRedsAid = m_aid;
              m_relayLinkInfo.srcRedsAddress = GetAddress ();
              m_relayLinkInfo.dstRedsAid = info.first->GetAID ();
              m_relayLinkInfo.dstRedsAddress = stationAddress;
              m_relayLinkInfo.dstRedsCapabilitiesInfo = capabilitiesInfo;
              m_relayLinkInfo.waitingDestinationRedsReports = false;
              m_relayLinkInfo.relayLinkEstablished = false;
              m_relayLinkInfo.transmissionLink = DIRECT_LINK;
              m_relayLinkInfo.switchTransmissionLink = false;
              m_relayLinkInfo.relayForwardingActivated = false;
              /* Send Relay Search Request Frame to the DMG PCP/AP */
              SendRelaySearchRequest (0, m_relayLinkInfo.dstRedsAid);
            }
          else
            {
              NS_LOG_INFO ("Cannot establish relay link with DMG STA=" << stationAddress);
            }
        }
      else
        {
          NS_LOG_INFO ("We don't have relay capabilities of DMG STA=" << stationAddress);
        }
    }
  else
    {
      /* Obtain Information about the node like DMG Capabilities, AID, and Relay Capabilities */
      RequestRelayInformation (GetBssid ());
    }
}

void
DmgStaWifiMac::SendRelaySearchRequest (uint8_t token, uint16_t destinationAid)
{
  NS_LOG_FUNCTION (this << token << destinationAid);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (GetBssid ());
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetBssid ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  ExtRelaySearchRequestHeader requestHdr;
  requestHdr.SetDialogToken (token);
  requestHdr.SetDestinationRedsAid (destinationAid);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.dmgAction = WifiActionHeader::DMG_RELAY_SEARCH_REQUEST;
  actionHdr.SetAction (WifiActionHeader::DMG, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (requestHdr);
  packet->AddHeader (actionHdr);

  m_dca->Queue (packet, hdr);
}

void
DmgStaWifiMac::SendRlsRequest (Mac48Address to, uint8_t token, uint16_t sourceAid, uint16_t relayAid, uint16_t destinationAid)
{
  NS_LOG_FUNCTION (this << to << token << sourceAid << relayAid << destinationAid);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetBssid ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  ExtRlsRequest requestHdr;
  requestHdr.SetDialogToken (token);
  requestHdr.SetSourceAid (sourceAid);
  requestHdr.SetRelayAid (relayAid);
  requestHdr.SetDestinationAid (destinationAid);

  RelayCapabilitiesInfo srcInfo = GetRelayCapabilitiesInfo ();
  requestHdr.SetSourceCapabilityInformation (srcInfo);
  requestHdr.SetRelayCapabilityInformation (m_relayLinkInfo.rdsCapabilitiesInfo);
  requestHdr.SetDestinationCapabilityInformation (m_relayLinkInfo.dstRedsCapabilitiesInfo);
  requestHdr.SetRelayTransferParameterSet (GetRelayTransferParameterSet ());

  /* Store current relay information */
  m_relayLinkInfo.rdsDuplexMode = m_rdsDuplexMode;
  m_relayLinkInfo.relayLinkChangeInterval = m_relayLinkChangeInterval;
  m_relayLinkInfo.relayDataSensingTime = m_relayDataSensingTime;
  m_relayLinkInfo.relayFirstPeriod = m_relayFirstPeriod;
  m_relayLinkInfo.relaySecondPeriod = m_relaySecondPeriod;

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.dmgAction = WifiActionHeader::DMG_RLS_REQUEST;
  actionHdr.SetAction (WifiActionHeader::DMG, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (requestHdr);
  packet->AddHeader (actionHdr);

  m_dca->Queue (packet, hdr);
}

void
DmgStaWifiMac::SendRlsResponse (Mac48Address to, uint8_t token, uint16_t destinationStatus, uint16_t relayStatus)
{
  NS_LOG_FUNCTION (this << token);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetBssid ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  ExtRlsResponse reponseHdr;
  reponseHdr.SetDialogToken (token);
  reponseHdr.SetDestinationStatusCode (destinationStatus);
  reponseHdr.SetRelayStatusCode (relayStatus);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.dmgAction = WifiActionHeader::DMG_RLS_RESPONSE;
  actionHdr.SetAction (WifiActionHeader::DMG, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (reponseHdr);
  packet->AddHeader (actionHdr);

  m_dca->Queue (packet, hdr);
}

void
DmgStaWifiMac::SendRlsAnnouncment (Mac48Address to, uint16_t destination_aid, uint16_t relay_aid, uint16_t source_aid)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetBssid ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  ExtRlsAnnouncment announcmentHdr;
  announcmentHdr.SetStatusCode (0);
  announcmentHdr.SetDestinationAid (destination_aid);
  announcmentHdr.SetRelayAid (relay_aid);
  announcmentHdr.SetSourceAid (source_aid);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.dmgAction = WifiActionHeader::DMG_RLS_ANNOUNCEMENT;
  actionHdr.SetAction (WifiActionHeader::DMG, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (announcmentHdr);
  packet->AddHeader (actionHdr);

  m_dca->Queue (packet, hdr);
}

void
DmgStaWifiMac::SendRelayTeardown (Mac48Address to, uint16_t sourceAid, uint16_t destinationAid, uint16_t relayAid)
{
  NS_LOG_FUNCTION (this << to << sourceAid << destinationAid << relayAid);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetBssid ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  ExtRlsTearDown frame;
  frame.SetSourceAid (sourceAid);
  frame.SetDestinationAid (destinationAid);
  frame.SetRelayAid (relayAid);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.dmgAction = WifiActionHeader::DMG_RLS_TEARDOWN;
  actionHdr.SetAction (WifiActionHeader::DMG, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (frame);
  packet->AddHeader (actionHdr);

  m_dca->Queue (packet, hdr);
}

void
DmgStaWifiMac::RemoveRelayEntry (uint16_t sourceAid, uint16_t destinationAid)
{
  NS_LOG_FUNCTION (this << sourceAid << destinationAid);
  REDS_PAIR redsPair = std::make_pair (sourceAid, destinationAid);
  RELAY_LINK_MAP_ITERATOR it = m_relayLinkMap.find (redsPair);
  if (it != m_relayLinkMap.end ())
    {
      RELAY_LINK_INFO info = it->second;
      if (info.srcRedsAid == m_aid)
        {
          /* Change next hop address for packets */
          m_dataForwardingTable[info.dstRedsAddress].nextHopAddress = info.dstRedsAddress;
        }
      m_relayLinkMap.erase (it);
    }
}

void
DmgStaWifiMac::TeardownRelayImmediately (uint16_t sourceAid, uint16_t destinationAid, uint16_t relayAid)
{
  NS_LOG_FUNCTION (this << sourceAid << destinationAid << relayAid);
  REDS_PAIR redsPair = std::make_pair (sourceAid, destinationAid);
  RELAY_LINK_MAP_ITERATOR it = m_relayLinkMap.find (redsPair);
  if (it != m_relayLinkMap.end ())
    {
      /* Check if the relay is protecting the current SP allocation */
      if (m_periodProtected &&
          (m_relayLinkInfo.srcRedsAid == sourceAid) &&
          (m_relayLinkInfo.dstRedsAid == destinationAid))
        {
          m_relayLinkInfo.tearDownRelayLink = true;
        }
      else
        {
          RemoveRelayEntry (sourceAid, destinationAid);
        }
    }
}

void
DmgStaWifiMac::TeardownRelay (uint16_t sourceAid, uint16_t destinationAid, uint16_t relayAid)
{
  NS_LOG_FUNCTION (this << sourceAid << destinationAid << relayAid);
  REDS_PAIR redsPair = std::make_pair (sourceAid, destinationAid);
  RELAY_LINK_MAP_ITERATOR it = m_relayLinkMap.find (redsPair);
  if (it != m_relayLinkMap.end ())
    {
      RELAY_LINK_INFO info = it->second;

      /* Check if the relay is protecting the current SP allocation */
      if (m_periodProtected &&
          (m_relayLinkInfo.srcRedsAid == sourceAid) &&
          (m_relayLinkInfo.dstRedsAid == destinationAid))
        {
          m_relayLinkInfo.tearDownRelayLink = true;
        }
      else
        {
          RemoveRelayEntry (sourceAid, destinationAid);
        }

      /* Inform other nodes about removal of tearing down of the relay link */
      if (m_aid == info.srcRedsAid)
        {
          /* We are the source REDS */
          SendRelayTeardown (info.selectedRelayAddress, sourceAid, destinationAid, relayAid);
        }
      else
        {
          /* We are the RDS */
          SendRelayTeardown (info.srcRedsAddress, sourceAid, destinationAid, relayAid);
        }
      SendRelayTeardown (info.dstRedsAddress, sourceAid, destinationAid, relayAid);
      SendRelayTeardown (GetBssid (), sourceAid, destinationAid, relayAid);
    }
}

void
DmgStaWifiMac::StartRlsProcedure (void)
{
  NS_LOG_FUNCTION (this);
  SendRlsRequest (m_relayLinkInfo.selectedRelayAddress, 10, m_aid,
                  m_relayLinkInfo.selectedRelayAid, m_relayLinkInfo.dstRedsAid);
}

Ptr<MultiBandElement>
DmgStaWifiMac::GetMultiBandElement (void) const
{
  Ptr<MultiBandElement> multiband = Create<MultiBandElement> ();
  multiband->SetStaRole (ROLE_NON_PCP_NON_AP);
  multiband->SetStaMacAddressPresent (false); /* The same MAC address is used across all the bands */
  multiband->SetBandID (Band_4_9GHz);
  multiband->SetOperatingClass (18);          /* Europe */
  multiband->SetChannelNumber (m_phy->GetChannelNumber ());
  multiband->SetBssID (GetBssid ());
  multiband->SetConnectionCapability (1);     /* AP */
  multiband->SetFstSessionTimeout (m_fstTimeout);
  return multiband;
}

void
DmgStaWifiMac::TxOk (Ptr<const Packet> packet, const WifiMacHeader &hdr)
{
  if ((m_currentLinkMaintained) &&
      (m_currentAllocation == SERVICE_PERIOD_ALLOCATION) &&
      (hdr.IsData ()))
    {
      /* Reset BeamLink Maintenance Timer */
      m_beamLinkMaintenanceTimeout = Simulator::Schedule (MicroSeconds (m_currentBeamLinkMaintenanceInfo.beamLinkMaintenanceTime),
                                                          &DmgStaWifiMac::BeamLinkMaintenanceTimeout, this);
    }
//  else if (hdr.IsAction ())
//    {
//      WifiActionHeader actionHdr;
//      Ptr<Packet> packet = packet->Copy ();
//      packet->RemoveHeader (actionHdr);

//      if (actionHdr.GetCategory () == WifiActionHeader::DMG)
//        {
//          switch (actionHdr.GetAction ().dmgAction)
//            {
//            case WifiActionHeader::DMG_RLS_TEARDOWN:
//              {
//                NS_LOG_LOGIC ("Transmitted RLS Tear Down Frame to " << hdr);

//              }
//            default:
//              break;
//            }
//        }
//    }
  DmgWifiMac::TxOk (packet, hdr);
}

void
DmgStaWifiMac::FrameTxOk (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this);
  if (hdr.IsSSW ())
    {
      if (m_accessPeriod == CHANNEL_ACCESS_ABFT)
        {
          if (m_codebook->GetNextSectorInABFT ())
            {
              /* We still have more sectors to sweep */
              if (m_isResponderTXSS)
                {
                  Simulator::Schedule (m_sbifs, &DmgStaWifiMac::SendRespodnerTransmitSectorSweepFrame, this, hdr.GetAddr1 ());
                }
              else
                {
                  Simulator::Schedule (m_sbifs, &DmgStaWifiMac::SendReceiveSectorSweepFrame, this, hdr.GetAddr1 (),
                                       m_totalSectors, BeamformingResponder);
                }
            }
        }
      else if (m_accessPeriod == CHANNEL_ACCESS_DTI)
        {
          /* We are performing SLS during DTI access period */
          bool changeAntenna;
          if (m_codebook->GetNextSector (changeAntenna))
            {
              /* Check if we change antenna so we use different spacing value */
              Time spacing;
              if (changeAntenna)
                {
                  spacing = m_lbifs;
                }
              else
                {
                  spacing = m_sbifs;
                }

              if (m_isBeamformingInitiator)
                {
                  if (m_isInitiatorTXSS)
                    {
                      /* We are I-TxSS */
                      Simulator::Schedule (spacing, &DmgStaWifiMac::SendInitiatorTransmitSectorSweepFrame, this, hdr.GetAddr1 ());
                    }
                  else
                    {
                      /* We are I-RxSS */
                      Simulator::Schedule (spacing, &DmgStaWifiMac::SendReceiveSectorSweepFrame, this, hdr.GetAddr1 (),
                                           m_totalSectors, BeamformingInitiator);
                    }
                }
              else
                {
                  if (m_isResponderTXSS)
                    {
                      /* We are R-TxSS */
                      Simulator::Schedule (spacing, &DmgStaWifiMac::SendRespodnerTransmitSectorSweepFrame, this, hdr.GetAddr1 ());
                    }
                  else
                    {
                      /* We are R-RxSS */
                      Simulator::Schedule (spacing, &DmgStaWifiMac::SendReceiveSectorSweepFrame, this, hdr.GetAddr1 (),
                                           m_totalSectors, BeamformingResponder);
                    }
                }
            }
          else
            {
              /* We have finished sector sweeping */
              if (m_isBeamformingInitiator)
                {
                  if (m_isResponderTXSS)
                    {
                      /* We are the initiator and the responder is performing TxSS */
                      m_codebook->SetReceivingInQuasiOmniMode ();
                    }
                  else
                    {
                      /* I-RxSS so initiator switches between different receiving sectors */
                      m_codebook->StartSectorSweeping (hdr.GetAddr1 (), ReceiveSectorSweep, 1);
                    }
                  /* Start Timeout value: The initiator may restart the ISS up to dot11BFRetryLimit times if it does not
                   * receive an SSW frame from the responder in dot11BFTXSSTime time following the end of the ISS. */
                  m_restartISSEvent = Simulator::Schedule (MilliSeconds (dot11BFTXSSTime),
                                                           &DmgStaWifiMac::RestartInitiatorSectorSweep, this, hdr.GetAddr1 ());
                }
              else
                {
                  /* Shall we use previous Tx or Rx sector if we are doing Rx or Tx sector sweep */
    //              SteerAntennaToward (hdr.GetAddr1 ());
                  m_codebook->SetReceivingInQuasiOmniMode ();
                }
            }
        }
    }
  else if (hdr.IsSSW_ACK ())
    {
      /* We are SLS Responder, raise callback for SLS Phase completion. */
      ANTENNA_CONFIGURATION antennaConfig;
      if (m_isResponderTXSS)
        {
          antennaConfig = m_bestAntennaConfig[hdr.GetAddr1 ()].first;
        }
      else if (!m_isInitiatorTXSS)
        {
          antennaConfig = m_bestAntennaConfig[hdr.GetAddr1 ()].second;
        }
      m_slsCompleted (hdr.GetAddr1 (), CHANNEL_ACCESS_DTI, BeamformingResponder, m_isInitiatorTXSS, m_isResponderTXSS,
                      antennaConfig.first, antennaConfig.second);
    }
}

void
DmgStaWifiMac::Receive (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << packet << hdr);
  Mac48Address from = hdr->GetAddr2 ();
  if (hdr->GetAddr3 () == GetAddress ())
    {
      NS_LOG_LOGIC ("packet sent by us.");
      return;
    }
  else if (hdr->GetAddr1 () != GetAddress () && !hdr->GetAddr1 ().IsGroup () && !hdr->IsDMGBeacon ())
    {
      NS_LOG_LOGIC ("packet is not for us");
      NotifyRxDrop (packet);
      return;
    }
  else if (m_relayMode && (m_rdsDuplexMode == 0) && hdr->IsData ())
    {
      NS_LOG_LOGIC ("Work as relay, forward packet to " << m_relayLinkInfo.dstRedsAddress);
      /* We are the RDS in HD-DF so forward the packet to the destination REDS */
      m_relayReceivedData = true;
      ForwardDataFrame (*hdr, packet, m_relayLinkInfo.dstRedsAddress);
      return;
    }
  else if (hdr->IsData ())
    {
      if (!IsAssociated () && hdr->GetAddr2 () != GetBssid ())
        {
          NS_LOG_LOGIC ("Received data frame while not associated: ignore");
          NotifyRxDrop (packet);
          return;
        }

      if (hdr->IsQosData ())
        {
          /* Relay Related Variables */
          m_relayDataExchanged = true;
          m_moreData = hdr->IsMoreData ();
          if (hdr->IsQosAmsdu ())
            {
              NS_ASSERT (hdr->GetAddr3 () == GetBssid ());
              DeaggregateAmsduAndForward (packet, hdr);
              packet = 0;
            }
          else
            {
              ForwardUp (packet, hdr->GetAddr3 (), hdr->GetAddr1 ());
            }
        }
      else
        {
          ForwardUp (packet, hdr->GetAddr3 (), hdr->GetAddr1 ());
        }
      return;
    }
  else if (hdr->IsProbeReq ()|| hdr->IsAssocReq ())
    {
      // This is a frame aimed at an AP, so we can safely ignore it.
      NotifyRxDrop (packet);
      return;
    }
  else if (hdr->IsAction () || hdr->IsActionNoAck ())
    {
      WifiActionHeader actionHdr;
      packet->RemoveHeader (actionHdr);
      switch (actionHdr.GetCategory ())
        {
        case WifiActionHeader::RADIO_MEASUREMENT:
          switch (actionHdr.GetAction ().radioMeasurementAction)
            {
            case WifiActionHeader::RADIO_MEASUREMENT_REQUEST:
              {
                RadioMeasurementRequest requestHdr;
                packet->RemoveHeader (requestHdr);
                Ptr<DirectionalChannelQualityRequestElement> elem =
                    DynamicCast<DirectionalChannelQualityRequestElement> (requestHdr.GetListOfMeasurementRequestElement ().at (0));
                /* Schedule the start of the requested measurement */
                Simulator::Schedule (MicroSeconds (elem->GetMeasurementStartTime ()),
                                     &DmgStaWifiMac::StartChannelQualityMeasurement, this, elem);
                return;
              }
            default:
              NS_FATAL_ERROR ("Unsupported Action frame received");
              return;
            }


        case WifiActionHeader::QOS:
          switch (actionHdr.GetAction ().qos)
            {
            case WifiActionHeader::ADDTS_RESPONSE:
              {
                DmgAddTSResponseFrame frame;
                packet->RemoveHeader (frame);
                /* Contain modified airtime allocation */
                if (frame.GetStatusCode ().IsSuccess ())
                  {
                    NS_LOG_LOGIC ("DMG Allocation Request accepted by the PCP/AP");
                  }
                else if (frame.GetStatusCode ().GetStatusCodeValue () == STATUS_CODE_REJECTED_WITH_SUGGESTED_CHANGES)
                  {
                    NS_LOG_LOGIC ("DMG Allocation Request reject by the PCP/AP");
                  }
                return;
              }
            default:
              packet->AddHeader (actionHdr);
              DmgWifiMac::Receive (packet, hdr);
              return;
            }

        case WifiActionHeader::DMG:
          switch (actionHdr.GetAction ().dmgAction)
            {
            case WifiActionHeader::DMG_RELAY_SEARCH_RESPONSE:
              {
                ExtRelaySearchResponseHeader responseHdr;
                packet->RemoveHeader (responseHdr);
                /* The response contains list of RDSs in the current DMG BSS */
                m_rdsList = responseHdr.GetRelayCapableList ();
                m_rdsDiscoveryCompletedCallback (m_rdsList);
                return;
              }
            case WifiActionHeader::DMG_MULTI_RELAY_CHANNEL_MEASUREMENT_REQUEST:
              {
                NS_LOG_LOGIC ("Received Multi-Relay Channel Measurement Request from " << hdr->GetAddr2 ());
                ExtMultiRelayChannelMeasurementRequest requestHdr;
                packet->RemoveHeader (requestHdr);
                /* Prepare the Channel Report */
                ChannelMeasurementInfoList list;
                Ptr<ExtChannelMeasurementInfo> elem;
                double measuredsnr;
                uint8_t snr;
                if (m_rdsActivated)
                  {
                    /** We are the RDS and we received the request from the source REDS **/
                    /* Obtain Channel Measurement between the source REDS and RDS */
                    GetBestAntennaConfiguration (hdr->GetAddr2 (), true, measuredsnr);
                    snr = -(unsigned int) (4 * (measuredsnr - 19));
                    elem = Create<ExtChannelMeasurementInfo> ();
                    elem->SetPeerStaAid (m_macMap[hdr->GetAddr2 ()]);
                    elem->SetSnr (snr);
                    list.push_back (elem);
                  }
                else
                  {
                    /**
                     * We are the destination REDS and we've received the request from the source REDS.
                     * Report back the measurement information between destination REDS and all the available RDS.
                     */
                    for (RelayCapableStaList::iterator iter = m_rdsList.begin (); iter != m_rdsList.end (); iter++)
                      {
                        elem = Create<ExtChannelMeasurementInfo> ();
                        GetBestAntennaConfiguration (hdr->GetAddr2 (), true, measuredsnr);
                        snr = -(unsigned int) (4 * (measuredsnr - 19));
                        elem->SetPeerStaAid (iter->first);
                        elem->SetSnr (snr);
                        list.push_back (elem);
                      }
                  }
                SendChannelMeasurementReport (hdr->GetAddr2 (), requestHdr.GetDialogToken (), list);
                return;
              }
            case WifiActionHeader::DMG_MULTI_RELAY_CHANNEL_MEASUREMENT_REPORT:
              {
                if (m_relayLinkInfo.srcRedsAid == m_aid)
                  {
                    ExtMultiRelayChannelMeasurementReport responseHdr;
                    packet->RemoveHeader (responseHdr);
                    if (!m_relayLinkInfo.waitingDestinationRedsReports)
                      {
                        /* Perform BF with the destination REDS, currently this is done by invoking a callback function
                         * to the main program. The main program schedules manually a service period between the source REDS
                         * and destination REDS */

                        /* Send Multi-Relay Channel Measurement Request to the Destination REDS */
                        m_relayLinkInfo.waitingDestinationRedsReports = true;
                        /* Store the measurement values between the source REDS and the RDS */
                        m_channelMeasurementList = responseHdr.GetChannelMeasurementInfoList ();
                      }
                    else
                      {
                        /**
                         * The source REDS is aware of the following channel measurements with:
                         * 1. Zero or more RDS.
                         * 2. Between Destination REDS and zero or more RDS.
                         * The Source REDS shall select on of the previous RDS.
                         */

                        /* Report the measurements to the user to decide relay selection */
                        m_relayLinkInfo.selectedRelayAid = m_channelMeasurementCallback (m_channelMeasurementList,
                                                                                         responseHdr.GetChannelMeasurementInfoList (),
                                                                                         m_relayLinkInfo.selectedRelayAddress);

                        m_relayLinkInfo.rdsCapabilitiesInfo = m_rdsList[m_relayLinkInfo.selectedRelayAid];

                      }
                    m_channelReportReceived (hdr->GetAddr2 ());
                  }
                return;
              }
            case WifiActionHeader::DMG_RLS_REQUEST:
              {
                ExtRlsRequest requestHdr;
                packet->RemoveHeader (requestHdr);

                /* Store the AID and address of the source and destination REDS */
                m_relayLinkInfo.srcRedsAid = requestHdr.GetSourceAid ();
                m_relayLinkInfo.srcRedsAddress = m_aidMap[m_relayLinkInfo.srcRedsAid];
                m_relayLinkInfo.dstRedsAid = requestHdr.GetDestinationAid ();
                m_relayLinkInfo.tearDownRelayLink = false;

                /* Store Parameters related to the relay link */
                Ptr<RelayTransferParameterSetElement> elem = requestHdr.GetRelayTransferParameterSet ();
                m_relayLinkInfo.rdsDuplexMode = elem->GetDuplexMode ();
                m_relayLinkInfo.relayLinkChangeInterval = elem->GetLinkChangeInterval ();
                m_relayLinkInfo.relayDataSensingTime = elem->GetDataSensingTime ();
                m_relayLinkInfo.relayFirstPeriod = elem->GetFirstPeriod ();
                m_relayLinkInfo.relaySecondPeriod = elem->GetSecondPeriod ();

                if (m_aid == requestHdr.GetRelayAid ())
                  {
                    /* We are the selected RDS so resend RLS Request to the Destination REDS */
                    NS_LOG_LOGIC ("Received RLS Request from Source REDS="
                                  << hdr->GetAddr2 () << ", resend RLS Request to Destination REDS");
                    m_relayLinkInfo.dstRedsAddress = m_aidMap[m_relayLinkInfo.dstRedsAid];
                    /* Upon receiving the RLS Request frame, the RDS shall transmit an RLS Request frame to
                     * the destination REDS containing the same information as received within the frame body
                     * of the source REDS’s RLS Request frame. */
                    WifiActionHeader actionHdr;
                    WifiActionHeader::ActionValue action;
                    action.dmgAction = WifiActionHeader::DMG_RLS_REQUEST;
                    actionHdr.SetAction (WifiActionHeader::DMG, action);
                    ForwardActionFrame (m_relayLinkInfo.dstRedsAddress, actionHdr, requestHdr);
                  }
                else if (m_aid == requestHdr.GetDestinationAid ())
                  {
                    /* We are the destination REDS, so we send RLS Response to the selected RDS */
                    NS_LOG_LOGIC ("Received RLS Request from the selected RDS "
                                  << hdr->GetAddr2 () << ", send an RLS Response to RDS");
                    m_relayLinkInfo.dstRedsAddress = GetAddress ();
                    m_relayLinkInfo.selectedRelayAddress = hdr->GetAddr2 ();
                    m_relayLinkInfo.relayLinkEstablished = true;
                    /* Create data structure of the established relay link at the destination REDS */
                    REDS_PAIR redsPair = std::make_pair (m_relayLinkInfo.srcRedsAid, m_relayLinkInfo.dstRedsAid);
                    m_relayLinkMap[redsPair] = m_relayLinkInfo;
                    /* Send RLS Resposne to the selected RDS*/
                    SendRlsResponse (m_relayLinkInfo.selectedRelayAddress, requestHdr.GetDialogToken (), 0, 0);
                  }

                return;
              }
            case WifiActionHeader::DMG_RLS_RESPONSE:
              {
                ExtRlsResponse responseHdr;
                packet->RemoveHeader (responseHdr);
                if (m_rdsActivated)
                  {
                    /* We are the RDS, resend RLS Response to Source REDS */
                    NS_LOG_LOGIC ("Receveid RLS Response from the destination REDS="
                                  << hdr->GetAddr2 () << ", send RLS Response to the Source REDS");
                    m_relayLinkInfo.selectedRelayAid = m_aid;
                    SendRlsResponse (m_relayLinkInfo.srcRedsAddress, responseHdr.GetDialogToken (),
                                     responseHdr.GetDestinationStatusCode (), 0);
                    if (responseHdr.GetDestinationStatusCode () == 0)
                      {
                        /* Create data structure of the established relay link at the RDS */
                        REDS_PAIR redsPair = std::make_pair (m_relayLinkInfo.srcRedsAid, m_relayLinkInfo.dstRedsAid);
                        m_relayLinkInfo.relayLinkEstablished = true;
                        m_relayLinkMap[redsPair] = m_relayLinkInfo;
                      }
                  }
                else
                  {
                    /* This node is the Source REDS */
                    if ((responseHdr.GetRelayStatusCode () == 0) && (responseHdr.GetDestinationStatusCode () == 0))
                      {
                        /* Create data structure of the established relay link */
                        REDS_PAIR redsPair = std::make_pair (m_aid, m_relayLinkInfo.dstRedsAid);
                        m_relayLinkInfo.relayLinkEstablished = true;
                        m_relayLinkInfo.tearDownRelayLink = false;
                        m_relayLinkMap[redsPair] = m_relayLinkInfo;
                        /* Invoke callback for the completion of the RLS procedure */
                        m_rlsCompleted (m_relayLinkInfo.selectedRelayAddress);
                        /* Send RLS Announcement frame to PCP/AP */
                        SendRlsAnnouncment (GetBssid (),
                                            m_relayLinkInfo.dstRedsAid,
                                            m_relayLinkInfo.selectedRelayAid,
                                            m_aid);
                        /* We can redo BF (Optional) */
                        NS_LOG_LOGIC ("Relay Link Switch procedure is Success, RDS operates in " << m_rdsDuplexMode
                                      << " Mode, Send RLS Announcement to the DMG PCP/AP=" << GetBssid ());
                      }
                  }
                return;
              }
            case WifiActionHeader::DMG_RLS_TEARDOWN:
              {
                NS_LOG_LOGIC ("Received RLS Tear Down Frame from=" << hdr->GetAddr2 ());
                ExtRlsTearDown header;
                packet->RemoveHeader (header);
                RemoveRelayEntry (header.GetSourceAid (), header.GetDestinationAid ());
                return;
              }
            case WifiActionHeader::DMG_INFORMATION_RESPONSE:
              {
                ExtInformationResponse responseHdr;
                packet->RemoveHeader (responseHdr);

                /* Record the Information Obtained */
                Mac48Address stationAddress = responseHdr.GetSubjectAddress ();

                /* If this field is set to the broadcast address, then the STA is providing
                 * information regarding all associated STAs.*/
                if (stationAddress.IsBroadcast ())
                  {
                    NS_LOG_LOGIC ("Received DMG Information Response frame regarding all DMG STAs in the DMG BSS.");
                  }
                else
                  {
                    NS_LOG_LOGIC ("Received DMG Information Response frame regarding DMG STA: " << stationAddress);
                  }

                /* Store all the DMG Capabilities */
                DmgCapabilitiesList dmgCapabilitiesList = responseHdr.GetDmgCapabilitiesList ();
                for (DmgCapabilitiesListI iter = dmgCapabilitiesList.begin (); iter != dmgCapabilitiesList.end (); iter++)
                  {
                    InformationMapIterator infoIter = m_informationMap.find ((*iter)->GetStaAddress ());
                    if (infoIter != m_informationMap.end ())
                      {
                        StationInformation *information = &(infoIter->second);
                        information->first = *iter;
                      }
                    else
                      {
                        StationInformation information;
                        information.first = *iter;
                        m_informationMap[(*iter)->GetStaAddress ()] = information;
                        MapAidToMacAddress ((*iter)->GetAID (), (*iter)->GetStaAddress ());
                      }
                  }

                /* Store Information related to the requested IEs */
                WifiInformationElementMap informationMap = responseHdr.GetListOfInformationElement ();
                InformationMapIterator infoIter = m_informationMap.find (stationAddress);
                Ptr<DmgCapabilities> dmgCapabilities = StaticCast<DmgCapabilities> (informationMap[IE_DMG_CAPABILITIES]);
                if (infoIter != m_informationMap.end ())
                  {
                    StationInformation *information = &(infoIter->second);
                    if (dmgCapabilities != 0)
                      {
                        information->first = dmgCapabilities;
                      }
                    information->second = informationMap;
                  }
                else
                  {
                    StationInformation information;
                    if (dmgCapabilities != 0)
                      {
                        information.first = dmgCapabilities;
                        MapAidToMacAddress (dmgCapabilities->GetAID (), dmgCapabilities->GetStaAddress ());
                      }
                    information.second = informationMap;
                    m_informationMap[stationAddress] = information;
                  }

                m_informationReceived (stationAddress);
                return;
              }
            default:
              NS_FATAL_ERROR ("Unsupported Action frame received");
              return;
            }
        default:
          packet->AddHeader (actionHdr);
          DmgWifiMac::Receive (packet, hdr);
          return;
        }
    }
  else if (hdr->IsPollFrame ())
    {
      NS_LOG_LOGIC ("Received Poll frame from=" << hdr->GetAddr2 ());

      /* Obtain resposne offset of the poll frame */
      CtrlDmgPoll poll;
      packet->RemoveHeader (poll);

      /* Obtain allocation info */
      DynamicAllocationInfoField info;
      BF_Control_Field btField;
      info = m_servicePeriodRequestCallback (GetAddress (), btField);

      /* Schedule transmission of the SPR Frame */
      Time sprDuration = hdr->GetDuration () - MicroSeconds (poll.GetResponseOffset ()) - m_phy->GetLastRxDuration ();
      Simulator::Schedule (MicroSeconds (poll.GetResponseOffset ()), &DmgStaWifiMac::SendSprFrame,
                           this, hdr->GetAddr2 (), sprDuration, info, btField);

      return;
    }
  else if (hdr->IsGrantFrame ())
    {
      NS_LOG_LOGIC ("Received Grant frame from=" << hdr->GetAddr2 ());

      CtrlDMG_Grant grant;
      packet->RemoveHeader (grant);

      /* Initiate Service Period */
      DynamicAllocationInfoField field = grant.GetDynamicAllocationInfo ();
      BF_Control_Field bf = grant.GetBFControl ();
      Mac48Address peerAddress;
      uint8_t peerAid;
      bool isSource = false;
      Time startTime =  hdr->GetDuration () - MicroSeconds (field.GetAllocationDuration ());
      if (field.GetSourceAID () == m_aid)
        {
          /* We are the initiator in the allocated SP */
          isSource = true;
          peerAid = field.GetDestinationAID ();
        }
      else
        {
          /* We are the responder in the allocated SP */
          peerAid = field.GetSourceAID ();
        }
      peerAddress = m_aidMap[peerAid];

      /** The allocation begins upon successful reception of the Grant frame plus the value from the Duration field
        * of the Grant frame minus the value of the Allocation Duration field of the Grant frame. */

      /* Check whether the service period for BFT or Data Communication */
      if (bf.IsBeamformTraining ())
        {
          Simulator::Schedule (startTime, &DmgStaWifiMac::StartBeamformingTraining, this,
                               peerAid, peerAddress, isSource, bf.IsInitiatorTxss (), bf.IsResponderTxss (),
                               MicroSeconds (field.GetAllocationDuration ()));
        }
      else
        {
          Simulator::Schedule (startTime, &DmgStaWifiMac::StartServicePeriod, this,
                               0, MicroSeconds (field.GetAllocationDuration ()),
                               peerAid, peerAddress, isSource);
        }

      return;
    }
  else if (hdr->IsDMGBeacon ())
    {
      NS_LOG_LOGIC ("Received DMG Beacon frame with BSS_ID=" << hdr->GetAddr1 ());

      ExtDMGBeacon beacon;
      packet->RemoveHeader (beacon);

      SnrTag tag;
      bool removed = packet->RemovePacketTag (tag);
      NS_ASSERT (removed);
      NS_LOG_DEBUG ("SnrTag value: " << tag.Get());

      if (beacon.GetSsid ().IsEqual (GetSsid ()))
        {
          /* Check if we have already received a DMG Beacon during the BTI period. */
          if (!m_receivedDmgBeacon)
            {
              m_receivedDmgBeacon = true;
              m_stationSnrMap.erase (hdr->GetAddr1 ());
              m_beaconArrival = Simulator::Now ();

              Time delay = MicroSeconds (beacon.GetBeaconIntervalUs () * m_maxLostBeacons);
              RestartBeaconWatchdog (delay);

              /* Beacon Interval Field */
              ExtDMGBeaconIntervalCtrlField beaconInterval = beacon.GetBeaconIntervalControlField ();
              m_nextBeacon = beaconInterval.GetNextBeacon ();
              m_atiPresent = beaconInterval.IsATIPresent ();
              m_nextAbft = beaconInterval.GetNextABFT ();
              m_nBI = beaconInterval.GetN_BI ();
              m_ssSlotsPerABFT = beaconInterval.GetABFT_Length ();
              m_ssFramesPerSlot = beaconInterval.GetFSS ();
              m_isResponderTXSS = beaconInterval.IsResponderTXSS ();

              /* DMG Parameters */
              ExtDMGParameters parameters = beacon.GetDMGParameters ();
              m_isCbapOnly = parameters.Get_CBAP_Only ();
              m_isCbapSource = parameters.Get_CBAP_Source ();

              if (m_state == UNASSOCIATED)
                {
                  m_txssSpan = beaconInterval.GetTXSS_Span ();
                  m_remainingBIs = m_txssSpan;
                  m_completedFragmenteBI = false;

                  /* Next DMG ATI Element */
                  if (m_atiPresent)
                    {
                      Ptr<NextDmgAti> atiElement = StaticCast<NextDmgAti> (beacon.GetInformationElement (IE_NEXT_DMG_ATI));
                      m_atiDuration = MicroSeconds (atiElement->GetAtiDuration ());
                    }
                  else
                    {
                      m_atiDuration = MicroSeconds (0);
                    }
                }

              /* Update the remaining number of BIs to cover the whole TxSS */
              m_remainingBIs--;
              if (m_remainingBIs == 0)
                {
                  m_completedFragmenteBI = true;
                  m_remainingBIs = m_txssSpan;
                }

              /* Record DMG Capabilities */
              Ptr<DmgCapabilities> capabilities = StaticCast<DmgCapabilities> (beacon.GetInformationElement (IE_DMG_CAPABILITIES));
              if (capabilities != 0)
                {
                  StationInformation information;
                  information.first = capabilities;
                  m_informationMap[hdr->GetAddr1 ()] = information;
                  m_stationManager->AddStationDmgCapabilities (hdr->GetAddr2 (), capabilities);
                }

              /* DMG Operation Element */
              Ptr<DmgOperationElement> operationElement
                  = StaticCast<DmgOperationElement> (beacon.GetInformationElement (IE_DMG_OPERATION));

              /* Organizing medium access periods (Synchronization with TSF) */
              m_abftDuration = m_ssSlotsPerABFT * GetSectorSweepSlotTime (m_ssFramesPerSlot);
//              m_biStartTime = MicroSeconds (beacon.GetTimestamp ()) + hdr->GetDuration () - m_btiDuration;
              m_biStartTime = MicroSeconds (beacon.GetTimestamp ());
              m_beaconInterval = MicroSeconds (beacon.GetBeaconIntervalUs ());
              NS_LOG_DEBUG ("BI Started=" << m_biStartTime.As (Time::US)
                            << ", A-BFT Duration=" << m_abftDuration.As (Time::US)
                            << ", ATI Duration=" << m_atiDuration.As (Time::US)
                            << ", BeaconInterval=" << m_beaconInterval.As (Time::US)
                            << ", BHIDuration=" << MicroSeconds (operationElement->GetMinBHIDuration ()).As (Time::US)
                            << ", TSF=" << MicroSeconds (beacon.GetTimestamp ()).As (Time::US)
                            << ", HDR-Duration=" << hdr->GetDuration ().As (Time::US)
                            << ", FrameDuration=" << m_phy->GetLastRxDuration ());

              /* Check if we have schedulled DTI before */
//              if (m_dtiStartEvent.IsRunning ())
//                {
//                  std::cout << "Cancel" << std::endl;
//                  m_dtiStartEvent.Cancel ();
//                }

              if (!beaconInterval.IsDiscoveryMode ())
                {
                  /* This function is triggered on NanoSeconds basis and Thr duration field is in MicroSeconds basis */
                  Time dmgBeaconDurationUs = MicroSeconds (ceil (static_cast<double> (m_phy->GetLastRxDuration ().GetNanoSeconds ()) / 1000));
                  Time startTime = hdr->GetDuration () + (dmgBeaconDurationUs - m_phy->GetLastRxDuration ()) + GetMbifs ();
                  if (m_nextAbft == 0)
                    {
                      /* Schedule A-BFT following the end of the BTI Period */
                      NS_LOG_DEBUG ("A-BFT Period for Station=" << GetAddress () << " is scheduled at " << Simulator::Now () + startTime);
                      SetBssid (hdr->GetAddr1 ());
                      m_abftEvent = Simulator::Schedule (startTime, &DmgStaWifiMac::StartAssociationBeamformTraining, this);
                    }
                  else
                    {
                      /* Schedule ATI period following the end of BTI Period */
                      if (m_atiPresent)
                        {
                          Simulator::Schedule (startTime, &DmgStaWifiMac::StartAnnouncementTransmissionInterval, this);
                          NS_LOG_DEBUG ("ATI for Station:" << GetAddress () << " is scheduled at " << Simulator::Now () + startTime);
                        }
                      else
                        {
                          Simulator::Schedule (startTime, &DmgStaWifiMac::StartDataTransmissionInterval, this);
                          NS_LOG_DEBUG ("DTI for Station:" << GetAddress () << " is scheduled at " << Simulator::Now () + startTime);
                        }
                    }
                }

              /* A STA shall not transmit in the A-BFT of a beacon interval if it does not receive at least one DMG Beacon
               * frame during the BTI of that beacon interval.*/

              /** Check the existance of other Information Element Fields **/

              /* Extended Scheudle Element */
              Ptr<ExtendedScheduleElement> scheduleElement =
                  StaticCast<ExtendedScheduleElement> (beacon.GetInformationElement (IE_EXTENDED_SCHEDULE));
              if (scheduleElement != 0)
                {
                  m_allocationList = scheduleElement->GetAllocationFieldList ();
                }
            }

          /* Sector Sweep Field */
          DMG_SSW_Field ssw = beacon.GetSSWField ();
          NS_LOG_DEBUG ("DMG Beacon CDOWN=" << uint16_t (ssw.GetCountDown ()));
          /* Map the antenna configuration, Addr1=BSSID */
          MapTxSnr (hdr->GetAddr1 (), ssw.GetSectorID (), ssw.GetDMGAntennaID (), m_stationManager->GetRxSnr ());
        }

      if (m_state == SCANNING || m_state == WAIT_BEACON)
        {
          NS_LOG_DEBUG ("Beacon received while scanning");
          if (m_bestBeaconObserved.m_snr < tag.Get ())
            {
              NS_LOG_DEBUG ("DMG Beacon has highest SNR so far: " << tag.Get ());
              m_bestBeaconObserved.m_channelNumber = m_phy->GetChannelNumber ();
              m_bestBeaconObserved.m_snr = tag.Get ();
              m_bestBeaconObserved.m_bssid = hdr->GetAddr3 ();
            }
        }

      return;
    }
  else if (hdr->IsSSW_FBCK ())
    {
      NS_LOG_LOGIC ("Responder: Received SSW-FBCK frame from=" << hdr->GetAddr2 ());

      CtrlDMG_SSW_FBCK fbck;
      packet->RemoveHeader (fbck);

      /* Check Beamformed link maintenance */
      RecordBeamformedLinkMaintenanceValue (fbck.GetBfLinkMaintenanceField ());

      if (m_isResponderTXSS)
        {
          /* The SSW-FBCK contains the best TX antenna by this station */
          DMG_SSW_FBCK_Field sswFeedback = fbck.GetSswFeedbackField ();
          sswFeedback.IsPartOfISS (false);

          /* Record best antenna configuration */
          ANTENNA_CONFIGURATION antennaConfig = std::make_pair (sswFeedback.GetSector (), sswFeedback.GetDMGAntenna ());
          UpdateBestTxAntennaConfiguration (hdr->GetAddr2 (), antennaConfig);

          /* We add the station to the list of the stations we can directly communicate with */
          AddForwardingEntry (hdr->GetAddr2 ());

          if (m_accessPeriod == CHANNEL_ACCESS_ABFT)
            {
              NS_LOG_LOGIC ("Best Tx Antenna Config by this DMG STA to DMG STA=" << hdr->GetAddr2 ()
                            << ": SectorID=" << static_cast<uint16_t> (antennaConfig.first)
                            << ", AntennaID=" << static_cast<uint16_t> (antennaConfig.second));

              /* Raise an event that we selected the best sector to the DMG AP */
              m_slsCompleted (hdr->GetAddr2 (), CHANNEL_ACCESS_BHI, BeamformingResponder, m_isInitiatorTXSS, m_isResponderTXSS,
                              antennaConfig.first, antennaConfig.second);

              /* We received SSW-FBCK so we cancel the timeout event and update counters */
              /* The STA shall set FailedRSSAttempts to 0 upon successfully receiving an SSW-Feedback frame during the A-BFT. */
              m_failedRssAttemptsCounter = 0;
              m_sswFbckTimeout.Cancel ();
              m_abftState = BEAMFORMING_TRAINING_COMPLETED;
            }
          else if (m_accessPeriod == CHANNEL_ACCESS_DTI)
            {
              NS_LOG_LOGIC ("Scheduled SSW-ACK Frame to " << hdr->GetAddr2 () << " at " << Simulator::Now () + m_mbifs);
              Simulator::Schedule (GetMbifs (), &DmgStaWifiMac::SendSswAckFrame, this, hdr->GetAddr2 (), hdr->GetDuration ());
            }
        }

      return;
    }
  else if (hdr->IsProbeResp ())
    {
      if (m_state == WAIT_PROBE_RESP)
        {
          MgtProbeResponseHeader probeResp;
          packet->RemoveHeader (probeResp);
          if (!probeResp.GetSsid ().IsEqual (GetSsid ()))
            {
              //not a probe resp for our ssid.
              return;
            }
          SnrTag tag;
          bool removed = packet->RemovePacketTag (tag);
          NS_ASSERT (removed);
          NS_LOG_DEBUG ("SnrTag value: " << tag.Get());
          if (tag.Get () > m_bestBeaconObserved.m_snr)
            {
              NS_LOG_DEBUG ("Save the Probe Response as a candidate");
              m_bestBeaconObserved.m_channelNumber = m_phy->GetChannelNumber ();
              m_bestBeaconObserved.m_snr = tag.Get ();
              m_bestBeaconObserved.m_bssid = hdr->GetAddr3 ();
              m_bestBeaconObserved.m_capabilities =
                  StaticCast<DmgCapabilities> (probeResp.GetInformationElement(IE_DMG_CAPABILITIES));
              m_bestBeaconObserved.m_probeResp = probeResp;
            }
        }
      return;
    }
  else if (hdr->IsAssocResp ())
    {
      if (m_state == WAIT_ASSOC_RESP)
        {
          MgtAssocResponseHeader assocResp;
          packet->RemoveHeader (assocResp);
          if (m_assocRequestEvent.IsRunning ())
            {
              m_assocRequestEvent.Cancel ();
            }
          if (assocResp.GetStatusCode ().IsSuccess ())
            {
              m_aid = assocResp.GetAid ();
              SetState (ASSOCIATED);
              MapAidToMacAddress (AID_AP, hdr->GetAddr3 ());
              NS_LOG_DEBUG ("Association completed with " << hdr->GetAddr3 ());
              if (!m_linkUp.IsNull ())
                {
                  m_linkUp ();
                }

              if (m_waitBeaconEvent.IsRunning ())
                {
                  m_waitBeaconEvent.Cancel ();
                }

              /** Record DMG AP Capabilities **/
              Ptr<DmgCapabilities> dmgCapabilities = StaticCast<DmgCapabilities> (assocResp.GetInformationElement (IE_DMG_CAPABILITIES));

              /* Record MCS1-4 as mandatory modes for data communication */
              AddMcsSupport (from, 1, 4);
              if (dmgCapabilities != 0)
                {
                  /* Record SC MCS range */
                  AddMcsSupport (from, 5, dmgCapabilities->GetMaximumScTxMcs ());
                  /* Record OFDM MCS range */
                  if (dmgCapabilities->GetMaximumOfdmTxMcs () != 0)
                    {
                      AddMcsSupport (from, 13, dmgCapabilities->GetMaximumOfdmTxMcs ());
                    }
                }

              /* Record DMG Capabilities */
              m_stationManager->AddStationDmgCapabilities (hdr->GetAddr2 (), dmgCapabilities);
            }
          else
            {
              NS_LOG_DEBUG ("Association Refused");
              SetState (REFUSED);
            }
        }
      return;
  }

  DmgWifiMac::Receive (packet, hdr);
}

Ptr<DmgCapabilities>
DmgStaWifiMac::GetDmgCapabilities (void) const
{
  Ptr<DmgCapabilities> capabilities = Create<DmgCapabilities> ();
  capabilities->SetStaAddress (GetAddress ()); /* STA MAC Adress*/
  capabilities->SetAID (static_cast<uint8_t> (m_aid));

  /* DMG STA Capability Information Field */
  capabilities->SetSPSH (m_supportSpsh);
  capabilities->SetReverseDirection (m_supportRdp);
  capabilities->SetNumberOfRxDmgAntennas (m_codebook->GetTotalNumberOfAntennas ());
  capabilities->SetNumberOfSectors (m_codebook->GetTotalNumberOfTransmitSectors ());
  capabilities->SetRxssLength (m_codebook->GetTotalNumberOfReceiveSectors ());
  capabilities->SetAmpduParameters (5, 0);      /* Hardcoded Now (Maximum A-MPDU + No restriction) */
  capabilities->SetSupportedMCS (m_maxScRxMcs, m_maxOfdmRxMcs, m_maxScTxMcs, m_maxOfdmTxMcs, m_supportLpSc, false); /* LP SC is not supported yet */
  capabilities->SetAppduSupported (false);      /* Currently A-PPDU Agregation is not supported */

  return capabilities;
}

void
DmgStaWifiMac::SetState (MacState value)
{
  enum MacState previousState = m_state;
  m_state = value;
  if (value == ASSOCIATED && previousState != ASSOCIATED)
    {
      NS_LOG_DEBUG ("Associating to " << GetBssid ());
      m_assocLogger (GetBssid (), GetAssociationID ());
    }
  else if (value != ASSOCIATED && previousState == ASSOCIATED)
    {
      NS_LOG_DEBUG ("Disassociating from " << GetBssid ());
      m_deAssocLogger (GetBssid ());
    }
}

} // namespace ns3
