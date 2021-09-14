/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2020 IMDEA Networks Institute
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
#include "channel-access-manager.h"
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

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DmgStaWifiMac");

NS_OBJECT_ENSURE_REGISTERED (DmgStaWifiMac);

TypeId
DmgStaWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgStaWifiMac")
    .SetParent<DmgWifiMac> ()
    .AddConstructor<DmgStaWifiMac> ()

    /* Association State Machine Attributes  */
    .AddAttribute ("ActiveScanning", "Flag to indicate whether we scan the network to find the best DMG PCP/AP to associate"
                                     "with or we use a static SSID.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&DmgStaWifiMac::m_activeScanning),
                   MakeBooleanChecker ())
    .AddAttribute ("ProbeRequestTimeout", "The duration to actively probe the channel.",
                   TimeValue (Seconds (0.05)),
                   MakeTimeAccessor (&DmgStaWifiMac::m_probeRequestTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("WaitBeaconTimeout", "The duration to dwell on a channel while passively scanning for beacon",
                   TimeValue (MilliSeconds (120)),
                   MakeTimeAccessor (&DmgStaWifiMac::m_waitBeaconTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("AssocRequestTimeout", "The interval between two consecutive assoc request attempts.",
                   TimeValue (Seconds (0.5)),
                   MakeTimeAccessor (&DmgStaWifiMac::m_assocRequestTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("MaxMissedBeacons",
                   "Number of beacons which much be consecutively missed before "
                   "we attempt to restart association.",
                   UintegerValue (10),
                   MakeUintegerAccessor (&DmgStaWifiMac::m_maxMissedBeacons),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("ActiveProbing",
                   "If true, we send probe requests. If false, we don't."
                   "NOTE: if more than one STA in your simulation is using active probing, "
                   "you should enable it at a different simulation time for each STA, "
                   "otherwise all the STAs will start sending probes at the same time resulting in collisions."
                   "See bug 1060 for more info.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&DmgStaWifiMac::SetActiveProbing,
                                        &DmgStaWifiMac::GetActiveProbing),
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

    /* Association Information */
    .AddTraceSource ("Assoc", "Associated with an access point.",
                     MakeTraceSourceAccessor (&DmgStaWifiMac::m_assocLogger),
                     "ns3::DmgWifiMac::AssociationTracedCallback")
    .AddTraceSource ("DeAssoc", "Associated with the access point is lost.",
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
    .AddTraceSource ("GroupUpdateFailed", "The transmission od an update for the best sector for the AP failed.",
                     MakeTraceSourceAccessor (&DmgStaWifiMac::m_updateFailed),
                     "ns3::DmgStaWifiMac::UpdateFailedTracedCallback")
  ;
  return tid;
}

DmgStaWifiMac::DmgStaWifiMac ()
  : m_aid (0),
    m_waitBeaconEvent (),
    m_probeRequestEvent (),
    m_assocRequestEvent (),
    m_beaconWatchdog (),
    m_beaconWatchdogEnd (Seconds (0.0)),
    m_abftEvent (),
    m_dtiStartEvent (),
    m_linkChangeInterval (),
    m_firstPeriod (),
    m_secondPeriod ()
{
  NS_LOG_FUNCTION (this);
  /** Initialize Relay Variables **/
  m_relayMode = false;
  m_periodProtected = false;
  /* Set missed ACK/BlockACK callback */
  for (EdcaQueues::iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->SetMissedAckCallback (MakeCallback (&DmgStaWifiMac::MissedAck, this));
    }
  /** Association State Variables **/
  m_state = UNASSOCIATED;
  /* Let the lower layers know that we are acting as a non-AP DMG STA in an infrastructure BSS. */
  SetTypeOfStation (DMG_STA);
  m_nextBtiWithTrn = 0;
  m_trnScheduleInterval = 0;
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
  m_abftSlot = CreateObject<UniformRandomVariable> ();
  m_rssBackoffVariable = CreateObject<UniformRandomVariable> ();
  m_rssBackoffVariable->SetAttribute ("Min", DoubleValue (0));
  m_rssBackoffVariable->SetAttribute ("Max", DoubleValue (m_rssBackoffLimit));
  m_failedRssAttemptsCounter = 0;
  m_rssBackoffRemaining = 0;
  m_nextBeacon = 0;
  m_abftState = WAIT_BEAMFORMING_TRAINING;

  /* EDMG variables */
  m_groupTraining = false;
  m_groupBeamformingFailed = false;

  /* Initialize upper DMG Wifi MAC */
  DmgWifiMac::DoInitialize ();

  /* Channel Measurement */
  GetDmgWifiPhy ()->RegisterMeasurementResultsReady (MakeCallback (&DmgStaWifiMac::ReportChannelQualityMeasurement, this));
  if (m_isEdmgSupported)
    {
      StaticCast<
          DmgWifiPhy> (m_phy)->RegisterBeaconTrainingCallback (MakeCallback (&DmgStaWifiMac::RegisterBeaconSnr, this));
    }
  /* Initialize variables since we expect to receive DMG Beacon */
  m_accessPeriod = CHANNEL_ACCESS_BTI;

  /* At the beginning of the BTI period, a DMG STA should stay in Quasi-Omni receiving mode */
  m_codebook->StartReceivingInQuasiOmniMode ();
  StaticCast<DmgWifiPhy> (m_phy)->SetIsAP (false);

  /* Check if we need to scan the network */
  if (m_activeScanning)
    {
      StartScanning ();
    }
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
DmgStaWifiMac::ResumePendingTXSS (void)
{
  NS_LOG_FUNCTION (this);
  if (m_receivedDmgBeacon)
    {
      m_dmgSlsTxop->ResumeTXSS ();
    }
 else
    {
      NS_LOG_INFO ("Cannot resume TXSS because we did not receive DMG Beacon in this BI");
    }
}

void
DmgStaWifiMac::Perform_TXSS_TXOP (Mac48Address peerAddress)
{
  NS_LOG_FUNCTION (this << peerAddress);
  if (m_receivedDmgBeacon)
    {
      m_dmgSlsTxop->Initiate_TXOP_Sector_Sweep (peerAddress);
    }
  else
    {
      m_dmgSlsTxop->Append_SLS_Reqest (peerAddress);
    }
}

void
DmgStaWifiMac::SetActiveProbing (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_activeProbing = enable;
  if (m_state == WAIT_PROBE_RESP || m_state == WAIT_BEACON)
    {
      NS_LOG_DEBUG ("DMG STA is still scanning, reset scanning process");
      StartScanning ();
    }
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
  /* EDMG Capabilities Information Element */
  if (m_isEdmgSupported)
    {
      probe.AddWifiInformationElement (GetEdmgCapabilities ());
    }

  packet->AddHeader (probe);

  //The standard is not clear on the correct queue for management
  //frames if we are a QoS AP. The approach taken here is to always
  //use the non-QoS for these regardless of whether we have a QoS
  //association or not.
  m_txop->Queue(packet, hdr);
}

void
DmgStaWifiMac::SendAssociationRequest (bool isReassoc)
{
  NS_LOG_FUNCTION (this << GetBssid () << isReassoc);
  WifiMacHeader hdr;
  hdr.SetType (isReassoc ? WIFI_MAC_MGT_REASSOCIATION_REQUEST : WIFI_MAC_MGT_ASSOCIATION_REQUEST);
  hdr.SetAddr1 (GetBssid ());
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetBssid ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  Ptr<Packet> packet = Create<Packet> ();
  if (!isReassoc)
    {
      MgtAssocRequestHeader assoc;
      assoc.SetSsid (GetSsid ());
      /* DMG Capabilities Information Element */
      assoc.AddWifiInformationElement (GetDmgCapabilities ());
      /* EDMG Capabilities Information Element if 802.11ay is supported */
      if (m_isEdmgSupported)
        {
          assoc.AddWifiInformationElement (GetEdmgCapabilities ());
        }
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
    }
  else
    {
      MgtReassocRequestHeader reassoc;
      reassoc.SetCurrentApAddress (GetBssid ());
      reassoc.SetSsid (GetSsid ());
      reassoc.SetListenInterval (0);
      /* DMG Capabilities Information Element */
      reassoc.AddWifiInformationElement (GetDmgCapabilities ());
      /* EDMG Capabilities Information Element if 802.11ay is supported */
      if (m_isEdmgSupported)
        {
          reassoc.AddWifiInformationElement (GetEdmgCapabilities ());
        }
      /* Multi-band Information Element */
      if (m_supportMultiBand)
        {
          reassoc.AddWifiInformationElement (GetMultiBandElement ());
        }
      /* Add Relay Capability Element */
      if (m_redsActivated || m_rdsActivated)
        {
          reassoc.AddWifiInformationElement (GetRelayCapabilitiesElement ());
        }
      if (m_staAvailabilityElement)
        {
          reassoc.AddWifiInformationElement (GetStaAvailabilityElement ());
        }
      packet->AddHeader (reassoc);
    }

  //The standard is not clear on the correct queue for management
  //frames if we are a QoS DMG PCP/AP. The approach taken here is
  //to alwaysuse the non-QoS for these regardless of whether we
  //have a QoS association or not.
  m_txop->Queue (packet, hdr);

  /* For now, we assume station talks to the DMG PCP/AP only */
  SteerAntennaToward (GetBssid ());

  if (m_assocRequestEvent.IsRunning ())
    {
      m_assocRequestEvent.Cancel ();
    }
  m_assocRequestEvent = Simulator::Schedule (m_assocRequestTimeout,
                                             &DmgStaWifiMac::AssocRequestTimeout, this);
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
    case WAIT_BEACON:
      /* we have initiated passive scanning, continue to wait
         and gather beacons
       */
      break;
    case UNASSOCIATED:
      /* we were associated but we missed a bunch of beacons
       * so we should assume we are not associated anymore.
       * We try to initiate a scan now.
       */
      m_linkDown ();
      //// WIGIG ////
      // We should cancel all the events related to the current EDMG/DMG PCP/AP
      m_dtiStartEvent.Cancel ();
      //// WIGIG ////
      if (m_activeScanning)
        {
          StartScanning ();
        }
      break;
    case WAIT_ASSOC_RESP:
      /* we have sent an association request so we do not need to
         re-send an association request right now. We just need to
         wait until either assoc-request-timeout or until
         we get an association response.
       */
      break;
    case REFUSED:
      /* we have sent an association request and received a negative
         association response. We wait until someone restarts an
         association with a given SSID.
       */
      break;
    }
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

  m_txop->Queue (packet, hdr);
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

  m_txop->Queue (packet, hdr);
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
//      hdr.SetType (WIFI_MAC_QOSDATA);
//      hdr.SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);
//      hdr.SetQosNoAmsdu ();
//      MsduAggregator::DeaggregatedMsdus packets = MsduAggregator::Deaggregate (packet);
//      for (MsduAggregator::DeaggregatedMsdusCI i = packets.begin (); i != packets.end (); ++i)
//        {
//          m_edca[AC_BE]->Queue ((*i).first, hdr);
//          NS_LOG_DEBUG ("Frame Length=" << (*i).first->GetSize ());
//        }
    }
  else
    {
      m_edca[AC_BE]->Queue (packet, hdr);
    }
}

void
DmgStaWifiMac::Enqueue (Ptr<Packet> packet, Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << to);
  if (!IsAssociated () && IsBeamformedTrained ())
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
  m_candidateAps.clear ();
  if (m_probeRequestEvent.IsRunning ())
    {
      m_probeRequestEvent.Cancel ();
    }
  if (m_waitBeaconEvent.IsRunning ())
    {
      m_waitBeaconEvent.Cancel ();
    }
  if (GetActiveProbing ())
    {
      SetState (WAIT_PROBE_RESP);
      SendProbeRequest ();
      m_probeRequestEvent = Simulator::Schedule (m_probeRequestTimeout,
                                                 &DmgStaWifiMac::ScanningTimeout,
                                                 this);
    }
  else
    {
      SetState (WAIT_BEACON);
      m_waitBeaconEvent = Simulator::Schedule (m_waitBeaconTimeout,
                                               &DmgStaWifiMac::ScanningTimeout,
                                               this);
    }
}

void
DmgStaWifiMac::ScanningTimeout (void)
{
  NS_LOG_FUNCTION (this);
  if (!m_candidateAps.empty ())
    {
      DmgApInfo bestAp = m_candidateAps.front ();
      m_candidateAps.erase (m_candidateAps.begin ());
      NS_LOG_DEBUG ("Attempting to associate with DMG BSS-ID " << bestAp.m_bssid);
      Time beaconInterval;
      if (bestAp.m_activeProbing)
        {
//          UpdateApInfoFromProbeResp (bestAp.m_probeResp, bestAp.m_apAddr, bestAp.m_bssid);
          beaconInterval = MicroSeconds (bestAp.m_probeResp.GetBeaconIntervalUs ());
        }
      else
        {
//          UpdateApInfoFromBeacon (bestAp.m_beacon, bestAp.m_apAddr, bestAp.m_bssid);
//          beaconInterval = MicroSeconds (bestAp.m_beacon.GetBeaconIntervalUs ());
        }

      Time delay = beaconInterval * m_maxMissedBeacons;
      RestartBeaconWatchdog (delay);
      SetState (WAIT_ASSOC_RESP);
      SendAssociationRequest (false);
    }
  else
    {
      NS_LOG_DEBUG ("Exhausted list of candidate DMG PCP/AP; restart scanning");
      StartScanning ();
    }
}

void
DmgStaWifiMac::AssocRequestTimeout (void)
{
  NS_LOG_FUNCTION (this);
  SetState (WAIT_ASSOC_RESP);
  SendAssociationRequest (false);
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
      m_beaconWatchdog = Simulator::Schedule (m_beaconWatchdogEnd - Simulator::Now (),
                                              &DmgStaWifiMac::MissedBeacons, this);
      return;
    }
  NS_LOG_DEBUG ("DMG Beacon missed");
  SetState (UNASSOCIATED);
  TryToEnsureAssociated ();
}

void
DmgStaWifiMac::RestartBeaconWatchdog (Time delay)
{
  NS_LOG_FUNCTION (this << delay);
  m_beaconWatchdogEnd = std::max (Simulator::Now () + delay, m_beaconWatchdogEnd);
  if (Simulator::GetDelayLeft (m_beaconWatchdog) < delay
      && m_beaconWatchdog.IsExpired ())
    {
      NS_LOG_DEBUG ("Really restart watchdog.");
      m_beaconWatchdog = Simulator::Schedule (delay, &DmgStaWifiMac::MissedBeacons, this);
    }
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

  /* Save BI start time */
  m_biStartTime = Simulator::Now ();

  /* Schedule the next period */
  if (m_nextBeacon == 0)
    {
      StartBeaconTransmissionInterval ();
    }
  else
    {
      /* We will not receive DMG Beacon during this BI */
      m_nextBeacon--;
      if (m_atiPresent)
        {
          StartAnnouncementTransmissionInterval ();
          NS_LOG_DEBUG ("ATI Channel Access Period for Station:" << GetAddress () << " is starting Now");
        }
      else
        {
          StartDataTransmissionInterval ();
          NS_LOG_DEBUG ("DTI Channel Access Period for Station:" << GetAddress () << " is starting Now");
        }
    }
}

void
DmgStaWifiMac::EndBeaconInterval (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("DMG STA Ending BI at " << Simulator::Now ());

  /* Cancel any ISS Retry event in the BI to avoid performing SLS in case no DMG beacon is received in the next BI */
  if (m_restartISSEvent.IsRunning ())
    {
      m_restartISSEvent.Cancel ();
    }
    
  /* Reset State at MacLow */  
  m_low->SLS_Phase_Ended ();
  m_low->MIMO_BFT_Phase_Ended ();
  
  /* Initialize DMG Beacon Reception */
  m_receivedDmgBeacon = false;

  /* Disable Channel Access by CBAP */
//  EndContentionPeriod ();

  /* Start New Beacon Interval */
  if (m_trnScheduleInterval != 0)
    {
      m_nextBtiWithTrn--;
    }
  /* If you were in MIMO mode switch to SISO for the next beacon interval */
  /* Check if there is an ongoing reception */
  Time endRx = StaticCast<DmgWifiPhy> (m_phy)->GetDelayUntilEndRx ();
  if (endRx == NanoSeconds (0))
    {
      /* If there is not switch to SISO mode */
      m_codebook->SetCommunicationMode (SISO_MODE);
    }
  else
    {
      /* If there is wait until the current reception is finished before switching the antenna configuration */
      Simulator::Schedule(endRx, &Codebook::SetCommunicationMode , m_codebook, SISO_MODE);
    }
  if (m_informationUpdateEvent.IsRunning ())
    {
      m_informationUpdateEvent.Cancel ();
    }
  StartBeaconInterval ();
}

void
DmgStaWifiMac::StartBeaconTransmissionInterval (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("DMG STA Starting BTI");
  m_accessPeriod = CHANNEL_ACCESS_BTI;
  /* Re-initialize variables since we expect to receive DMG Beacon */
  m_sectorFeedbackSchedulled = false;
  /* Check if there is an ongoing reception */
  Time endRx = StaticCast<DmgWifiPhy> (m_phy)->GetDelayUntilEndRx ();
  if (endRx == NanoSeconds (0))
    {
      /* If there is not switch to the next Quasi-omni pattern */
      m_codebook->SwitchToNextQuasiPattern ();
    }
  else
    {
      /* If there is wait until the current reception is finished before switching the antenna configuration */
      Simulator::Schedule(endRx, &Codebook::SwitchToNextQuasiPattern , m_codebook);
    }

  /* Note: Handle special case in which we have associated to DMG PCP/AP but we did not receive a DMG Beacon
     due to interference, so we try to use old schedulling information. */
  /**
    * 9.33.6.3 Contention-based access period (CBAP) allocation:
    * When the entire DTI is allocated to CBAP through the CBAP Only field in the DMG Parameters field, then
    * that CBAP is pseudo-static and exists for dot11MaxLostBeacons beacon intervals following the most
    * recently transmitted DMG Beacon that contained the indication, except if the CBAP is canceled by the
    * transmission by the PCP/AP of a DMG Beacon with the CBAP Only field of the DMG Parameters field
    * equal to 0 or an Announce frame with an Extended Schedule element.
    */
  if (IsAssociated () && m_isCbapOnly)
    {
      m_dtiStartEvent = Simulator::Schedule (m_nextDti, &DmgStaWifiMac::StartDataTransmissionInterval, this);
    }
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
      m_dtiStartEvent = Simulator::Schedule (m_abftDuration + m_mbifs, &DmgStaWifiMac::StartDataTransmissionInterval, this);
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
          m_isInitiatorTXSS = true; /* DMG-AP always performs TXSS in BTI */
          /* Sector selected -> Perform Responder TXSS */
          m_slsResponderStateMachine = SLS_RESPONDER_SSW_FBCK;
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
              m_isInitiatorTXSS = true; /* DMG-AP always performs TXSS in BTI */
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
  /* End SSW slot (A-BFT) and no backoff*/
  m_slsResponderStateMachine = SLS_RESPONDER_IDLE;
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
      m_abftSlot->SetAttribute ("Min", DoubleValue (0));
      m_abftSlot->SetAttribute ("Max", DoubleValue (m_remainingSlotsPerABFT - 1));
      slotIndex = m_abftSlot->GetInteger ();
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
  if (m_channelAccessManager->CanAccess ())
    {
      m_sectorSweepStarted = Simulator::Now ();
      m_sectorSweepDuration = CalculateSectorSweepDuration (m_ssFramesPerSlot);
      /* Obtain antenna configuration for the highest received SNR to feed it back in SSW-FBCK Field */
      m_feedbackAntennaConfig = GetBestAntennaConfiguration (address, true, m_maxSnr);
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
DmgStaWifiMac::RegisterBeaconSnr (AntennaID antennaId, SectorID sectorId, AWV_ID awvId, uint8_t trnUnitsRemaining, uint8_t subfieldsRemaining, double snr, Mac48Address apAddress)
{
  NS_LOG_FUNCTION (this << uint16_t (subfieldsRemaining) << uint16_t (trnUnitsRemaining) << snr);
  if (m_groupTraining && m_accessPeriod == CHANNEL_ACCESS_BTI)
    {
      NS_LOG_DEBUG ("Rx Config: " <<  uint16_t (antennaId) << " " <<  uint16_t (sectorId) << " "  << uint16_t (awvId));
      NS_LOG_DEBUG ("Tx Config: " << uint16_t (m_currentTxConfig.first.first) << " " << uint16_t (m_currentTxConfig.first.second));

     AWV_CONFIGURATION_RX currentRxConfig = std::make_pair (std::make_pair (antennaId, sectorId), awvId);
     AWV_CONFIGURATION_TX_RX currentConfig = std::make_pair (m_currentTxConfig, currentRxConfig);
     STATION_SNR_AWV_MAP_I it = m_apSnrAwvMap.find (apAddress);
     if (it != m_apSnrAwvMap.end ())
       {
         SNR_AWV_MAP *snrMap = (SNR_AWV_MAP *) (&it->second);
         (*snrMap)[currentConfig] = snr;
       }
     else
       { SNR_AWV_MAP snrMap;
         snrMap[currentConfig] = snr;
         m_apSnrAwvMap[apAddress] = snrMap;
       }
    }
}

void
DmgStaWifiMac::StartGroupBeamformingTraining (void)
{
  NS_LOG_FUNCTION (this);
  /* Find the antenna config that gave us the best receive SNR */
  AWV_CONFIGURATION_TX_RX config;
  double maxSnr;
  config = GetBestAntennaPatternConfiguration (m_peerStation, maxSnr);
  /* Save the transmit antenna config to feedback to the AP */
  AWV_CONFIGURATION_TX txConfig = config.first;
  AWV_CONFIGURATION_RX rxConfig = config.second;
  /* update the SNR table (m_stationSnrMap) with the newest SNR values (for the best receive config) */
  UpdateSnrTable (m_peerStation);
  /* Check if there has been a change in the best transit config detected for the AP */
  bool updateConfig = (m_feedbackAntennaConfig != txConfig.first);
  NS_LOG_DEBUG ("Best Rx Config: " << uint16_t (rxConfig.first.first) << " " << uint16_t (rxConfig.first.second) << " " << uint16_t (rxConfig.second));
  NS_LOG_DEBUG ("Best Tx Config: " << uint16_t (txConfig.first.first) << " " << uint16_t (txConfig.first.second));
  /* Update the best antenna config (Rx/Tx and RX) and if a change in config has been detected send an Unsolicited
   * Information Response frame to the AP to inform them of the change */
  m_feedbackAntennaConfig = txConfig.first;
  /* Set the BFT ID of the current BFT - if this is the first BFT with the peer STA, initialize it to 0, otherwise increase it by 1 to signal a new BFT */
  BFT_ID_MAP::iterator it = m_bftIdMap.find (m_peerStation);
  if (it != m_bftIdMap.end ())
    {
      uint16_t bftId = it->second + 1;
      m_bftIdMap [m_peerStation] = bftId;
    }
  else
    {
      m_bftIdMap [m_peerStation] = 0;
    }
  m_groupBeamformingCompleted (GroupBfCompletionAttrbitutes (m_peerStation, BeamformingInitiator, m_bftIdMap[m_peerStation],
                                                             rxConfig.first.first, rxConfig.first.second, rxConfig.second, maxSnr));
  if (m_antennaPatternReciprocity)
    {
      UpdateBestAntennaConfiguration (m_peerStation, rxConfig.first, rxConfig.first, maxSnr);
      if (rxConfig.second != NO_AWV_ID)
        {
          UpdateBestAWV (m_peerStation, rxConfig.second, rxConfig.second );
        }
      if ((updateConfig || m_groupBeamformingFailed) && IsAssociated ())
        {
          if (m_groupBeamformingFailed)
            m_updateFailed (m_peerStation);
          SendUnsolicitedTrainingResponse (m_peerStation);
          m_groupBeamformingFailed = true;
        }
    }
  else
    {
      UpdateBestRxAntennaConfiguration (m_peerStation, rxConfig.first, maxSnr);
      if (rxConfig.second != NO_AWV_ID)
        {
          UpdateBestRxAWV (m_peerStation, rxConfig.second);
        }
      /* To do: if there is no antenna reciprocity perform an unsolicted RSS */
    }
  m_groupTraining = false;
}

void
DmgStaWifiMac::UpdateSnrTable (Mac48Address apAddress)
{
   STATION_SNR_AWV_MAP_I it = m_apSnrAwvMap.find (apAddress);
   SNR_AWV_MAP snrPair = it->second;
  /* Update the transmit map */
  AWV_CONFIGURATION_TX txConfig = snrPair.begin()->first.first;
  AWV_CONFIGURATION_RX rxConfig = snrPair.begin()->first.second;
  bool change = false;
  bool first = true;
  for (SNR_AWV_MAP_I iter = snrPair.begin (); iter != snrPair.end (); iter++)
    {
      if (iter->first.first != txConfig)
        {
          txConfig = iter->first.first;
          change = true;
        }
      else
        {
          change = false;
        }
     if (first)
       {
         change = true;
         first = false;
       }
      if (change)
        {
          SNR_AWV_MAP_I highIter = iter;
          SNR snr = iter->second;
          for (SNR_AWV_MAP_I iter1 = iter; iter1 != snrPair.end (); iter1++)
            {
              if ((txConfig == iter1->first.first) && (snr < iter1->second))
                {
                  highIter = iter1;
                  snr = highIter->second;
                }
            }
          MapTxSnr (m_peerStation, rxConfig.first.first, txConfig.first.first, txConfig.first.second, snr);
        }
    }

  /* Update the receive map */
  change = false;
  first = true;
  for (SNR_AWV_MAP_I iter = snrPair.begin (); iter != snrPair.end (); iter++)
    {
      if (iter->first.second != rxConfig)
        {
          rxConfig = iter->first.second;
          change = true;
        }
      else
        {
          change = false;
        }
     if (first)
       {
         change = true;
         first = false;
       }
      if (change)
        {
          SNR_AWV_MAP_I highIter = iter;
          SNR snr = iter->second;
          for (SNR_AWV_MAP_I iter1 = snrPair.begin (); iter1 != snrPair.end (); iter1++)
            {
              if ((rxConfig == iter1->first.second) && (snr < iter1->second))
                {
                  highIter = iter1;
                  snr = highIter->second;
                }
            }
          MapRxSnr (m_peerStation, rxConfig.first.first, rxConfig.first.second, snr);
        }
    }
}

bool
DmgStaWifiMac::DetectChangeInConfiguration (ANTENNA_CONFIGURATION_COMBINATION newTxConfig)
{
  SNR_MAP snrMap = m_oldSnrTxMap;
  SNR_MAP::iterator highIter = snrMap.begin ();
  SNR snr = highIter->second;
  for (SNR_MAP::iterator iter = snrMap.begin (); iter != snrMap.end (); iter++)
    {
      if (snr < iter->second)
        {
          highIter = iter;
          snr = highIter->second;
        }
    }
  if (highIter->first == newTxConfig)
    {
      NS_LOG_DEBUG ("no change in configuration");
      return false;
    }
  else
    {
      NS_LOG_DEBUG ("change in configuration");
      return true;
    }
}

void
DmgStaWifiMac::StartAnnouncementTransmissionInterval (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO (this << "DMG STA Starting ATI at " << Simulator::Now ());
  m_accessPeriod = CHANNEL_ACCESS_ATI;
  /* We started ATI Period we should stay in quasi-omni mode waiting for packets */
  /* Check if there is an ongoing reception */
  Time endRx = StaticCast<DmgWifiPhy> (m_phy)->GetDelayUntilEndRx ();
  if (endRx == NanoSeconds (0))
    {
      /* If there is not switch to quasi-omni mode */
      m_codebook->SetReceivingInQuasiOmniMode ();
    }
  else
    {
      /* If there is wait until the current reception is finished before switching the antenna configuration */
      void (Codebook::*sn) () = &Codebook::SetReceivingInQuasiOmniMode;
      Simulator::Schedule (endRx, sn , m_codebook);
    }
  m_dtiStartEvent = Simulator::Schedule (m_atiDuration, &DmgStaWifiMac::StartDataTransmissionInterval, this);
  m_dmgAtiTxop->InitiateAtiAccessPeriod (m_atiDuration);
}

void
DmgStaWifiMac::StartDataTransmissionInterval (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("DMG STA Starting DTI at " << Simulator::Now ());
  m_accessPeriod = CHANNEL_ACCESS_DTI;

  if (!m_receivedDmgBeacon)
    {
      /*  If we didn´t receive a beacom from our AP in this BI don´t do group beamforming */
      m_groupTraining = false;
    }
  else
    {
      /* Get the relative starting time of the DTI channel access period w.r.t the BI */
      m_nextDti = Simulator::Now () - m_biStartTime;
      NS_LOG_DEBUG ("DTI starts " << m_nextDti << " after the BI");
    }

  /* Schedule the beginning of the next BI interval */
  m_dtiStartTime = Simulator::Now ();
//  m_bhiDuration = m_dtiStartTime - m_biStartTime;
  m_dtiDuration = m_beaconInterval - (Simulator::Now () - m_biStartTime);
  Simulator::Schedule (m_dtiDuration, &DmgStaWifiMac::EndBeaconInterval, this);
  NS_LOG_DEBUG ("Next Beacon Interval will start at " << Simulator::Now () + m_dtiDuration);

  /* Send Association Request if we are not assoicated */
  if (!IsAssociated () && IsBeamformedTrained ())
    {
      /* Check if there is an ongoing reception */
      Time endRx = StaticCast<DmgWifiPhy> (m_phy)->GetDelayUntilEndRx ();
      if (endRx == NanoSeconds (0))
        {
          /* If there is not send the association request */
          /* We allow normal DCA for access */
          SetState (WAIT_ASSOC_RESP);
          Simulator::ScheduleNow (&DmgStaWifiMac::SendAssociationRequest, this, false);
        }
      else
        {
          /* If there is wait until the current reception is finished before sending the request */
          Simulator::Schedule(endRx, &DmgStaWifiMac::SetState, this, WAIT_ASSOC_RESP);
          Simulator::Schedule(endRx, &DmgStaWifiMac::SendAssociationRequest, this, false);
        }
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

      // If we´ve associated and there were TRN fields appended to the beacons in the last BTI - start group beamforming training
      if (m_groupTraining && IsAssociated ())
        {
          StartGroupBeamformingTraining ();
        }
      if (m_isCbapOnly && !m_isCbapSource)
        {
          NS_LOG_INFO ("CBAP allocation only in DTI");
          /* Check if there is an ongoing reception */
          Time endRx = StaticCast<DmgWifiPhy> (m_phy)->GetDelayUntilEndRx ();
          if (endRx == NanoSeconds (0))
            {
              /* If there is not start the contention period */
              StartContentionPeriod (BROADCAST_CBAP, m_dtiDuration);
            }
          else
            {
              /* If there is wait until the current reception is finished before starting the contention period */
              Simulator::Schedule(endRx, &DmgStaWifiMac::StartContentionPeriod, this, BROADCAST_CBAP, m_dtiDuration - endRx );
            }
        }
      else
        {
          Time endRx = StaticCast<DmgWifiPhy> (m_phy)->GetDelayUntilEndRx ();
          AllocationField field;
          for (AllocationFieldList::iterator iter = m_allocationList.begin (); iter != m_allocationList.end (); iter++)
            {
              field = (*iter);
              if (field.GetAllocationType () == SERVICE_PERIOD_ALLOCATION)
                {
                  Time spStart = MicroSeconds (field.GetAllocationStart ());
                  Time spLength = MicroSeconds (field.GetAllocationBlockDuration ());

                  NS_ASSERT_MSG (spStart + spLength <= m_dtiDuration, "Allocation should not exceed DTI period.");
                  /* Check if there is an ongoing reception when the allocation period will start - based on information
                   * from the PHY abut current receptions. if the PHY will still be receiving when the allocation
                   * starts, delay the start until the reception is finished and shorten the duration so that the end time
                   * remains the same */
                  /**
                    * Note NINA: Temporary solution for when we are in the middle of receiving a packet from a station
                    * from another BSS when a service period is supposed to start. The standard is not clear about whether
                    * we end the reception or finish it. For now, the reception is completed and the service period will start
                    * after the end of the reception (it will still finish at the same time and have a reduced duration).
                    */
                  if (spStart < endRx)
                    {
                      if (spStart + spLength < endRx)
                        {
                          spLength = NanoSeconds (0);
                        }
                      else
                        {
                          spLength = spLength - (endRx - spStart);
                        }
                      spStart = endRx;
                    }

                  if (field.GetSourceAid () == m_aid)
                    {
                      uint8_t destAid = field.GetDestinationAid ();
                      Mac48Address destAddress = m_aidMap[destAid];
                      if (field.GetBfControl ().IsBeamformTraining ())
                        {
                          Simulator::Schedule (spStart, &DmgStaWifiMac::StartBeamformingTraining, this, destAid, destAddress, true,
                                               field.GetBfControl ().IsInitiatorTXSS (), field.GetBfControl ().IsResponderTXSS (), spLength);
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
                                               field.GetBfControl ().IsInitiatorTXSS (), field.GetBfControl ().IsResponderTXSS (), spLength);
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
                  /* Check if there is an ongoing reception and if there is, delay the start of the
                   * contention period until the reception is finish, making sure the end time is the same. */
                  Time spStart = MicroSeconds (field.GetAllocationStart ());
                  Time spLength = MicroSeconds (field.GetAllocationBlockDuration ());
                  if (spStart < endRx)
                    {
                      if (spStart + spLength < endRx)
                        {
                          spLength = NanoSeconds (0);
                        }
                      else
                        {
                          spLength = spLength - (endRx - spStart);
                        }
                      spStart = endRx;
                    }

                  Simulator::Schedule (spStart, &DmgStaWifiMac::StartContentionPeriod, this,
                                       field.GetAllocationID (), spLength);
                }
            }
        }
    }
  /* Raise a callback that we have started DTI */
  m_dtiStarted (GetAddress (), m_dtiDuration);
}

void
DmgStaWifiMac::ScheduleAllocationBlocks (AllocationField &field, STA_ROLE role)
{
  NS_LOG_FUNCTION (this);
  Time spStart = MicroSeconds (field.GetAllocationStart ());
  Time spLength = MicroSeconds (field.GetAllocationBlockDuration ());
  Time spPeriod = MicroSeconds (field.GetAllocationBlockPeriod ());
  uint8_t blocks = field.GetNumberOfBlocks ();
  Time endRx = StaticCast<DmgWifiPhy> (m_phy)->GetDelayUntilEndRx ();
  if (spPeriod > 0)
    {
      /* We allocate multiple blocks of this allocation as in (9.33.6 Channel access in scheduled DTI) */
      /* A_start + (i – 1) × A_period */
      for (uint8_t i = 0; i < blocks; i++)
        {
          NS_LOG_INFO ("Schedule SP Block [" << i << "] at " << spStart << " till " << spStart + spLength);
          /* Check if the service period starts while there is an ongoing reception */
          /**
            * Note NINA: Temporary solution for when we are in the middle of receiving a packet from a station
            * from another BSS when a service period is supposed to start. The standard is not clear about whether
            * we end the reception or finish it. For now, the reception is completed and the service period will start
            * after the end of the reception (it will still finish at the same time and have a reduced duration).
            */
          Time spLengthNew = spLength;
          Time spStartNew = spStart;
          if (spStart < endRx)
            {
              /* if does schedule the start after the reception is complete */

              if (spStart + spLength < endRx)
                {
                  spLengthNew = NanoSeconds (0);
                }
              else
                {
                  spLengthNew = spLength - (endRx - spStart);
                }
              spStartNew = endRx;
            }
          Simulator::Schedule (spStartNew, &DmgStaWifiMac::InitiateAllocationPeriod, this,
                               field.GetAllocationID (), field.GetSourceAid (), field.GetDestinationAid (), spLengthNew, role);
          spStart += spLength + spPeriod + GUARD_TIME;
        }
    }
  else
    {
      /* Special case when Allocation Block Period=0, i.e. consecutive blocks *
       * We try to avoid schedulling multiple blocks, so we schedule one big block */
      spLength = spLength * blocks;
      if (spStart < endRx)
        {
          if (spStart + spLength < endRx)
            {
              spLength = NanoSeconds (0);
            }
          else
            {
              spLength = spLength - (endRx - spStart);
            }
          spStart = endRx;
        }
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
          ANTENNA_CONFIGURATION_TX antennaConfigTxSrc = std::get<0> (m_bestAntennaConfig[info.srcRedsAddress]);
          ANTENNA_CONFIGURATION_TX antennaConfigTxDst = std::get<0> (m_bestAntennaConfig[info.dstRedsAddress]);
          Simulator::ScheduleNow (&DmgWifiPhy::ActivateRdsOpereation, GetDmgWifiPhy (),
                                  antennaConfigTxSrc.first, antennaConfigTxSrc.second,
                                  antennaConfigTxDst.first, antennaConfigTxDst.second);
          Simulator::Schedule (spLength, &DmgWifiPhy::SuspendRdsOperation, GetDmgWifiPhy ());
        }
      else // HD-DF
        {
          NS_LOG_INFO ("Protecting the SP by an RDS in HD-DF Mode: Source AID=" << info.srcRedsAid <<
                       " and Destination AID=" << info.dstRedsAid);

          /* Schedule events related to the beginning and end of relay period */
          Simulator::ScheduleNow (&DmgStaWifiMac::InitiateRelayPeriods, this, info);
          Simulator::Schedule (spLength, &DmgStaWifiMac::EndRelayPeriods, this, redsPair);

          /* Schedule an event to direct the antennas toward the source REDS */
          Simulator::ScheduleNow (&DmgStaWifiMac::SteerAntennaToward, this, info.srcRedsAddress, false);

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
              /* Switching back to the direct link so change addresses of all the packets stored in MacLow and QosTxop */
              m_low->ChangeAllocationPacketsAddress (m_currentAllocationID, m_relayLinkInfo.dstRedsAddress);
//              m_edca[AC_BE]->GetWifiMacQueue ()->ChangePacketsReceiverAddress (m_relayLinkInfo.selectedRelayAddress,
//                                                                               m_relayLinkInfo.dstRedsAddress);
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
      m_edca[AC_BE]->InitiateServicePeriodTransmission ();
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
      m_edca[AC_BE]->InitiateServicePeriodTransmission ();
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
      m_edca[AC_BE]->InitiateServicePeriodTransmission ();
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
  NS_LOG_FUNCTION (this << m_relayDataExchanged << m_channelAccessManager->IsReceiving () << m_moreData);
  if (m_relayLinkInfo.rdsDuplexMode == 1) // FD-AF
    {
      if ((!m_relayDataExchanged) && (!m_channelAccessManager->IsReceiving ()) && m_moreData)
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
  requestElement->AddRequestElementID (std::make_pair (IE_DMG_CAPABILITIES, 0));
  requestElement->AddRequestElementID (std::make_pair (IE_RELAY_CAPABILITIES, 0));

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
      m_channelAccessManager->DisableChannelAccess ();
    }
  m_reqElem = element;
  GetDmgWifiPhy ()->StartMeasurement (element->GetMeasurementDuration (), element->GetNumberOfTimeBlocks ());
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
  reportElem->SetNumberOfTimeBlocks (m_reqElem->GetNumberOfTimeBlocks ());
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

  m_txop->Queue (packet, hdr);
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
  m_txop->Queue (packet, hdr);
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

  m_txop->Queue (packet, hdr);
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

  m_txop->Queue (packet, hdr);
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
      Ptr<RelayCapabilitiesElement> capabilitiesElement = StaticCast<RelayCapabilitiesElement> (info.second[std::make_pair (IE_RELAY_CAPABILITIES, 0)]);
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

  m_txop->Queue (packet, hdr);
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

  m_txop->Queue (packet, hdr);
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

  m_txop->Queue (packet, hdr);
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

  m_txop->Queue (packet, hdr);
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

  m_txop->Queue (packet, hdr);
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
//  if (hdr.IsAction ())
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
  if (hdr.IsAction ())
    {
      WifiActionHeader actionHdr;
      Ptr<Packet> packet1 = packet->Copy ();
      packet1->RemoveHeader (actionHdr);
      if (actionHdr.GetAction ().dmgAction == WifiActionHeader::DMG_INFORMATION_RESPONSE)
        {
          NS_LOG_LOGIC ("Transmitted Information Response to " << hdr.GetAddr2 () << " after Group Beamforming");
          m_groupBeamformingFailed = false;
          if (m_informationUpdateEvent.IsRunning ())
            {
              m_informationUpdateEvent.Cancel ();
            }
        }
    }
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
                      /* We are I-TXSS */
                      Simulator::Schedule (spacing, &DmgStaWifiMac::SendInitiatorTransmitSectorSweepFrame, this, hdr.GetAddr1 ());
                    }
                  else
                    {
                      /* We are I-RXSS */
                      Simulator::Schedule (spacing, &DmgStaWifiMac::SendReceiveSectorSweepFrame, this, hdr.GetAddr1 (),
                                           m_totalSectors, BeamformingInitiator);
                    }
                }
              else
                {
                  if (m_isResponderTXSS)
                    {
                      /* We are R-TXSS */
                      Simulator::Schedule (spacing, &DmgStaWifiMac::SendRespodnerTransmitSectorSweepFrame, this, hdr.GetAddr1 ());
                    }
                  else
                    {
                      /* We are R-RXSS */
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
                      /* We are the initiator and the responder is performing TXSS */
                      m_codebook->SetReceivingInQuasiOmniMode ();
                    }
                  else
                    {
                      /* I-RXSS so initiator switches between different receiving sectors */
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
                  /* Start Timeout value: The responder goes to Idle state if it does not receive SSW-FBCK from the initiator
                   * in dot11MaxBFTime time following the end of the RSS. */
                  m_sswFbckTimeout = Simulator::Schedule (dot11MaxBFTime * m_beaconInterval,
                                                          &DmgStaWifiMac::Reset_SLS_Responder_Variables, this);
                }
            }
        }
    }
  else if (hdr.IsSSW_ACK ())
    {
      /* We are SLS Responder, raise callback for SLS Phase completion. */
      Mac48Address address = hdr.GetAddr1 ();
      BEST_ANTENNA_CONFIGURATION info = m_bestAntennaConfig[address];
      ANTENNA_CONFIGURATION antennaConfig;
      double snrValue = std::get<2> (info);;
      if (m_isResponderTXSS)
        {
          antennaConfig = std::get<0> (info);
        }
      else if (!m_isInitiatorTXSS)
        {
          antennaConfig = std::get<1> (info);
        }

      /* Inform WifiRemoteStationManager about link SNR value */
      m_stationManager->RecordLinkSnr (address, snrValue);

      /* Note: According to the standard, the Responder transits to the TXSS Phase Complete when
         we receive non SSW frame and non-SSW-FBCK frame */
      m_dmgSlsTxop->SLS_BFT_Completed ();
      m_performingBFT = false;
      m_slsResponderStateMachine = SLS_RESPONDER_TXSS_PHASE_COMPELTED;
      m_slsCompleted (SlsCompletionAttrbitutes (hdr.GetAddr1 (), CHANNEL_ACCESS_DTI, BeamformingResponder,
                                                m_isInitiatorTXSS, m_isResponderTXSS, m_bftIdMap [hdr.GetAddr1 ()],
                                                antennaConfig.first, antennaConfig.second, m_maxSnr));
      /* Resume data transmission after SLS operation */
      if (m_currentAllocation == CBAP_ALLOCATION)
        {
          m_txop->ResumeTxopTransmission ();
          for (EdcaQueues::iterator i = m_edca.begin (); i != m_edca.end (); ++i)
            {
              i->second->ResumeTxopTransmission ();
            }
        }
    }
  else if (hdr.IsSSW_FBCK ())
    {
      Time sswAckTimeout;
      if (m_isEdmgSupported)
        {
          sswAckTimeout = EDMG_SSW_ACK_TIMEOUT;
        }
      else
        {
          sswAckTimeout = SSW_ACK_TIMEOUT;
        }
      /* We are SLS Initiator, so schedule event for not receiving SSW-ACK, so we restart SSW Feedback process again */
      NS_LOG_INFO ("Schedule SSW-ACK Timeout at " << Simulator::Now () + sswAckTimeout);
      m_slsInitiatorStateMachine = SLS_INITIATOR_SSW_ACK;
      m_sswAckTimeoutEvent = Simulator::Schedule (sswAckTimeout, &DmgStaWifiMac::ResendSswFbckFrame, this);
    }
  else
    {
      DmgWifiMac::FrameTxOk (hdr);
    }
}

void
DmgStaWifiMac::Receive (Ptr<WifiMacQueueItem> mpdu)
{
  NS_LOG_FUNCTION (this << *mpdu);

  const WifiMacHeader* hdr = &mpdu->GetHeader ();
  Ptr<Packet> packet = mpdu->GetPacket ()->Copy ();
  Mac48Address from = hdr->GetAddr2 ();

  if (hdr->GetAddr3 () == GetAddress ())
    {
      NS_LOG_LOGIC ("packet sent by us.");
      return;
    }
  else if (GetDmgWifiPhy ()->GetMuMimoBeamformingTraining () && (hdr->GetAddr1 () == hdr->GetAddr2 ()))
    {
      // During MU MIMO BFT, in the MIMO phase, BRP packets are send with both the TA and RA addresses set to the initiator address
      DmgWifiMac::Receive (mpdu);
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
              DeaggregateAmsduAndForward (mpdu);
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
      // This is a frame aimed at DMG PCP/AP, so we can safely ignore it.
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
              DmgWifiMac::Receive (mpdu);
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
                Ptr<DmgCapabilities> dmgCapabilities = StaticCast<DmgCapabilities> (informationMap[std::make_pair (IE_DMG_CAPABILITIES, 0)]);
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
          DmgWifiMac::Receive (mpdu);
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
                               peerAid, peerAddress, isSource, bf.IsInitiatorTXSS (), bf.IsResponderTXSS (),
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
      NS_LOG_LOGIC ("Received DMG Beacon frame with BSSID=" << hdr->GetAddr1 ());

      ExtDMGBeacon beacon;
      packet->RemoveHeader (beacon);
      bool goodBeacon = false;
      if (GetSsid ().IsBroadcast ()
          || beacon.GetSsid ().IsEqual (GetSsid ()))
        {
          NS_LOG_LOGIC ("DMG Beacon is for our SSID");
          goodBeacon = true;
        }
      if (goodBeacon && m_state == ASSOCIATED)
        {
          m_beaconArrival = Simulator::Now ();
          Time delay = MicroSeconds (beacon.GetBeaconIntervalUs () * m_maxMissedBeacons);
          RestartBeaconWatchdog (delay);
//          UpdateApInfoFromBeacon (beacon, hdr->GetAddr2 (), hdr->GetAddr3 ());
        }

      if (goodBeacon && !m_activeScanning && m_accessPeriod == CHANNEL_ACCESS_BTI)
        {
          /* Check if we have already received a DMG Beacon during the BTI period. */
          if (!m_receivedDmgBeacon)
            {
              m_receivedDmgBeacon = true;
              STATION_SNR_PAIR_MAP_I it = m_stationSnrMap.find (hdr->GetAddr1 ());
              if (it != m_stationSnrMap.end ())
                {
                   m_oldSnrTxMap = m_stationSnrMap[hdr->GetAddr1 ()].first;
                }
              m_stationSnrMap.erase (hdr->GetAddr1 ());

              /* Beacon Interval Field */
              ExtDMGBeaconIntervalCtrlField beaconInterval = beacon.GetBeaconIntervalControlField ();
              m_nextBeacon = beaconInterval.GetNextBeacon ();
              m_atiPresent = beaconInterval.IsATIPresent ();
              m_nextAbft = beaconInterval.GetNextABFT ();
              m_nBI = beaconInterval.GetN_BI ();
              m_ssSlotsPerABFT = beaconInterval.GetABFT_Length ();
              m_ssFramesPerSlot = beaconInterval.GetFSS ();
              if (m_nextAbft == 0)
                {
                  m_isResponderTXSS = beaconInterval.IsResponderTXSS ();
                }
              else if (m_isEdmgSupported)
                {
                  bool isUnsolicitedRssEnabled = beaconInterval.IsUnsolicitedRssEnabled ();
                  if (!isUnsolicitedRssEnabled)
                    {
                      m_isUnsolicitedRssEnabled = false;
                    }
                }
              /* DMG Parameters */
              ExtDMGParameters parameters = beacon.GetDMGParameters ();
              m_isCbapOnly = parameters.Get_CBAP_Only ();
              m_isCbapSource = parameters.Get_CBAP_Source ();

              bool isEdmgSupported = parameters.Get_EDMG_supported ();
              if (!isEdmgSupported)
                {
                  m_isEdmgSupported = false;
                }

              if (m_state == UNASSOCIATED)
                {
                  m_TXSSSpan = beaconInterval.GetTXSS_Span ();
                  m_remainingBIs = m_TXSSSpan;
                  m_completedFragmenteBI = false;

                  /* Next DMG ATI Element */
                  if (m_atiPresent)
                    {
                      Ptr<NextDmgAti> atiElement = StaticCast<NextDmgAti> (beacon.GetInformationElement (std::make_pair (IE_NEXT_DMG_ATI, 0)));
                      m_atiDuration = MicroSeconds (atiElement->GetAtiDuration ());
                    }
                  else
                    {
                      m_atiDuration = MicroSeconds (0);
                    }
                }

              /* Update the remaining number of BIs to cover the whole TXSS */
              m_remainingBIs--;
              if (m_remainingBIs == 0)
                {
                  m_completedFragmenteBI = true;
                  m_remainingBIs = m_TXSSSpan;
                }

              /* Record DMG Capabilities */
              Ptr<DmgCapabilities> capabilities = StaticCast<DmgCapabilities> (beacon.GetInformationElement (std::make_pair (IE_DMG_CAPABILITIES, 0)));
              if (capabilities != 0)
                {
                  StationInformation information;
                  information.first = capabilities;
                  m_informationMap[hdr->GetAddr1 ()] = information;
                  m_stationManager->AddStationDmgCapabilities (hdr->GetAddr2 (), capabilities);
                }
              /* Record EDMG Capabilities if 802.11ay is supported */
              if (m_isEdmgSupported)
                {
                  Ptr<EdmgCapabilities> edmgCapabilities =
                      StaticCast<EdmgCapabilities> (beacon.GetInformationElement (std::make_pair (IE_EXTENSION, IE_EXTENSION_EDMG_CAPABILITIES)));
                  if (edmgCapabilities != 0)
                    {
                      EdmgStationInformation information;
                      information.first = edmgCapabilities;
                      m_edmgInformationMap[hdr->GetAddr1 ()] = information;
                      m_stationManager->AddStationEdmgCapabilities (hdr->GetAddr2 (), edmgCapabilities);
                    }
                  if (GetDmgWifiPhy ()->IsMuMimoSupported ())
                    {
                      m_edmgGroupIdSetElement =
                          StaticCast<EDMGGroupIDSetElement> (beacon.GetInformationElement (std::make_pair (IE_EXTENSION, IE_EXTENSION_EDMG_GROUP_ID_SET)));
                    }
                }

              /* DMG Operation Element */
              Ptr<DmgOperationElement> operationElement
                  = StaticCast<DmgOperationElement> (beacon.GetInformationElement (std::make_pair (IE_DMG_OPERATION, 0)));

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
              if (m_dtiStartEvent.IsRunning ())
                {
                  m_dtiStartEvent.Cancel ();
                  NS_LOG_DEBUG ("Cancel the pre-schedulled DTI event since we received one DMG Beacon");
                }

              /** For EDMG STAs check the existence of other Information Element Fields **/
              if (m_isEdmgSupported)
                {
                  /* EDMG Training Field Schedule Element */
                  Ptr<EdmgTrainingFieldScheduleElement> trainingScheduleElement =
                      StaticCast<EdmgTrainingFieldScheduleElement> (beacon.GetInformationElement (std::make_pair (IE_EXTENSION, IE_EXTENSION_EDMG_TRAINING_FIELD_SCHEDULE)));
                  if (trainingScheduleElement != 0)
                    {
                      m_nextBtiWithTrn = trainingScheduleElement->GetNextBtiWithTrn ();
                      m_trnScheduleInterval = trainingScheduleElement->GetTrnScheduleInterval ();
                      if (m_nextBtiWithTrn == 0)
                        {
                          m_peerStation = hdr->GetAddr1 ();
                          m_groupTraining = true;
                          m_apSnrAwvMap.clear();
                        }
                      else
                        {
                          m_groupTraining = false;
                        }
                    }
                }
               else
                 {
                   m_groupTraining = false;
                 }

              if (!beaconInterval.IsDiscoveryMode ())
                {
                  Time trnDuration = NanoSeconds (0);
                  if (m_isEdmgSupported && !m_nextBtiWithTrn)
                    {
                      trnDuration = NanoSeconds (StaticCast<DmgWifiPhy> (m_phy)->GetBeaconTrnSubfieldDuration ()
                                                 * StaticCast<DmgWifiPhy> (m_phy)->GetBeaconTrnFieldLength () * EDMG_TRN_UNIT_SIZE);
                    }

                  /* This function is triggered on NanoSeconds basis and the duration field is in MicroSeconds basis */
                  Time dmgBeaconDurationUs = MicroSeconds (ceil (static_cast<double> (m_phy->GetLastRxDuration ().GetNanoSeconds ()) / 1000));
                  Time startTime = hdr->GetDuration () + (dmgBeaconDurationUs - m_phy->GetLastRxDuration () + trnDuration) + GetMbifs ();
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
                          m_dtiStartEvent = Simulator::Schedule (startTime, &DmgStaWifiMac::StartDataTransmissionInterval, this);
                          NS_LOG_DEBUG ("DTI for Station:" << GetAddress () << " is scheduled at " << Simulator::Now () + startTime);
                        }
                    }
                }

              /* A STA shall not transmit in the A-BFT of a beacon interval if it does not receive at least one DMG Beacon
               * frame during the BTI of that beacon interval.*/

              /** Check the existance of other Information Element Fields **/

              /* Extended Scheudle Element */
              Ptr<ExtendedScheduleElement> scheduleElement =
                  StaticCast<ExtendedScheduleElement> (beacon.GetInformationElement (std::make_pair (IE_EXTENDED_SCHEDULE, 0)));
              if (scheduleElement != 0)
                {
                  m_allocationList = scheduleElement->GetAllocationFieldList ();
                }
            }

          /* Sector Sweep Field */
          DMG_SSW_Field ssw = beacon.GetSSWField ();
          NS_LOG_DEBUG ("DMG Beacon CDOWN=" << uint16_t (ssw.GetCountDown ()));
          /* Map the antenna configuration, Addr1=BSSID */
          MapTxSnr (hdr->GetAddr1 (), ssw.GetDMGAntennaID (), ssw.GetSectorID (), m_stationManager->GetRxSnr ());
          //Remember the Tx configuration of the beacon when we do training using TRN-R fields in the beacon
          if (m_groupTraining)
            {
              ANTENNA_CONFIGURATION config = std::make_pair (ssw.GetDMGAntennaID (), ssw.GetSectorID ());
              m_currentTxConfig.first = config;

            }
          m_slsResponderStateMachine = SLS_RESPONDER_SECTOR_SELECTOR;
          //// NINA ////
          /* If this is the first beacon ever received, initialize the BFT ID with the AP to 0 */
          BFT_ID_MAP::iterator it = m_bftIdMap.find (from);
          if (it == m_bftIdMap.end ())
            {
              m_bftIdMap[from] = 0;
            }
          //// NINA ////
        }

      if (goodBeacon && m_state == WAIT_BEACON && m_activeScanning)
        {
          NS_LOG_DEBUG ("DMG Beacon received while scanning from " << hdr->GetAddr2 ());
          SnrTag snrTag;
          bool removed = packet->RemovePacketTag (snrTag);
          NS_ASSERT (removed);
          DmgApInfo apInfo;
          apInfo.m_apAddr = hdr->GetAddr2 ();
          apInfo.m_bssid = hdr->GetAddr3 ();
          apInfo.m_activeProbing = false;
          apInfo.m_snr = snrTag.Get ();
          apInfo.m_beacon = beacon;
          UpdateCandidateApList (apInfo);
        }

      return;
    }
  else if (hdr->IsSSW_FBCK ())
    {
      NS_LOG_LOGIC ("Responder: Received SSW-FBCK frame from=" << hdr->GetAddr2 ());

      if (m_performingBFT && (m_peerStationAddress != hdr->GetAddr2 ()))
        {
          NS_LOG_LOGIC ("Responder: Received SSW-FBCK frame from different initiator, so ignore it");
          return;
        }

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
          ANTENNA_CONFIGURATION antennaConfig = std::make_pair (sswFeedback.GetDMGAntenna (), sswFeedback.GetSector ());
          UpdateBestTxAntennaConfiguration (hdr->GetAddr2 (), antennaConfig, sswFeedback.GetSNRReport ());
          if (m_antennaPatternReciprocity && m_isEdmgSupported)
            {
              UpdateBestRxAntennaConfiguration (hdr->GetAddr2 (), antennaConfig, sswFeedback.GetSNRReport ());
            }

          /* We add the station to the list of the stations we can directly communicate with */
          AddForwardingEntry (hdr->GetAddr2 ());

          /* Cancel SSW-FBCK timeout */
          m_sswFbckTimeout.Cancel ();

          if (m_accessPeriod == CHANNEL_ACCESS_ABFT)
            {
              NS_LOG_LOGIC ("Best Tx Antenna Config by this DMG STA to DMG STA=" << hdr->GetAddr2 ()
                            << ": AntennaID=" << static_cast<uint16_t> (antennaConfig.first)
                            << ", SectorID=" << static_cast<uint16_t> (antennaConfig.second));

              /* Inform WifiRemoteStationManager about link SNR value */
              m_stationManager->RecordLinkSnr (hdr->GetAddr2 (), sswFeedback.GetSNRReport ());

              /* Raise an event that we selected the best sector to the DMG AP */
              m_slsResponderStateMachine = SLS_RESPONDER_TXSS_PHASE_COMPELTED;
              m_slsCompleted (SlsCompletionAttrbitutes (hdr->GetAddr2 (), CHANNEL_ACCESS_BHI, BeamformingResponder,
                                                        m_isInitiatorTXSS, m_isResponderTXSS, m_bftIdMap [hdr->GetAddr2 ()],
                                                        antennaConfig.first, antennaConfig.second, m_maxSnr));
              m_groupBeamformingCompleted (GroupBfCompletionAttrbitutes (hdr->GetAddr2 (), BeamformingInitiator, m_bftIdMap [hdr->GetAddr2 ()],
                                                                         antennaConfig.first, antennaConfig.second, NO_AWV_ID, m_maxSnr));
              /* We received SSW-FBCK so we cancel the timeout event and update counters */
              /* The STA shall set FailedRSSAttempts to 0 upon successfully receiving an SSW-Feedback frame during the A-BFT. */
              m_failedRssAttemptsCounter = 0;
              m_abftState = BEAMFORMING_TRAINING_COMPLETED;
            }
          else if (m_accessPeriod == CHANNEL_ACCESS_DTI)
            {
              /* This might be a retry SSW-FBCK frame, so we need to inform the MacLow
               * that we are serving SLS.
               * CHECK: why MacLow receive eventhough we set the NAV duration correctly */
              m_low->SLS_Phase_Started ();

              NS_LOG_LOGIC ("Scheduled SSW-ACK Frame to " << hdr->GetAddr2 () << " at " << Simulator::Now () + m_mbifs);
              m_slsResponderStateMachine = SLS_RESPONDER_TXSS_PHASE_PRECOMPLETE;
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
              NS_LOG_DEBUG ("Probe response is not for our SSID");
              return;
            }
          SnrTag tag;
          bool removed = packet->RemovePacketTag (tag);
          NS_ASSERT (removed);
          NS_LOG_DEBUG ("SnrTag value: " << tag.Get ());
          DmgApInfo apInfo;
          apInfo.m_apAddr = hdr->GetAddr2 ();
          apInfo.m_bssid = hdr->GetAddr3 ();
          apInfo.m_activeProbing = true;
          apInfo.m_snr = tag.Get ();
          apInfo.m_probeResp = probeResp;
          UpdateCandidateApList (apInfo);
        }
      return;
    }
  else if (hdr->IsAssocResp () || hdr->IsReassocResp ())
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
              /** Record DMG AP Capabilities **/
              Ptr<DmgCapabilities> capabilities = StaticCast<DmgCapabilities> (assocResp.GetInformationElement (std::make_pair (IE_DMG_CAPABILITIES, 0)));

              /* Set BSSID */
//              SetBssid (hdr->GetAddr1 ());

              /* Record DMG SC MCSs (1-4) as mandatory modes for data communication */
              AddMcsSupport (from, 1, 4);
              if (capabilities != 0)
                {
                  /* Record SC MCSs range */
                  AddMcsSupport (from, 5, capabilities->GetMaximumScTxMcs ());
                  /* Record OFDM MCSs range */
                  if ((GetDmgWifiPhy ()->GetSupportOfdmPhy ()) && (capabilities->GetMaximumOfdmTxMcs () != 0))
                    {
                      AddMcsSupport (from, 13, capabilities->GetMaximumOfdmTxMcs ());
                    }
                }

              /* Record DMG Capabilities */
              m_stationManager->AddStationDmgCapabilities (hdr->GetAddr2 (), capabilities);
         
              /** Record EDMG Capabilities if 802.11ay is supported **/
              if (m_isEdmgSupported)
                {
                  Ptr<EdmgCapabilities> edmgCapabilities =
                      StaticCast<EdmgCapabilities> (assocResp.GetInformationElement (std::make_pair (IE_EXTENSION, IE_EXTENSION_EDMG_CAPABILITIES)));
                  if (edmgCapabilities != 0)
                    {
                      m_stationManager->RemoveAllSupportedModes (from);
                      /* Record MCS1-4 as mandatory modes for data communication */
                      AddMcsSupport (from, 1, 4);
                      /* Record SC MCS range */
                      AddMcsSupport (from, 5, edmgCapabilities->GetMaximumScMcs ());
                      /* Record OFDM MCS range */
                      if ((GetDmgWifiPhy ()->GetSupportOfdmPhy ()) && (edmgCapabilities->GetMaximumOfdmMcs () != 0))
                        {
                          AddMcsSupport (from, 22, edmgCapabilities->GetMaximumOfdmMcs () + 21);
                        }
                      m_stationManager->AddStationEdmgCapabilities (hdr->GetAddr2 (), edmgCapabilities);

                    }
                }

              /* Change association state */
              m_aid = assocResp.GetAssociationId ();
              MapAidToMacAddress (AID_AP, hdr->GetAddr3 ());
              SetState (ASSOCIATED);
              NS_LOG_DEBUG ("Association completed with " << hdr->GetAddr3 ());

              if (!m_linkUp.IsNull ())
                {
                  m_linkUp ();
                }

              if (m_waitBeaconEvent.IsRunning ())
                {
                  m_waitBeaconEvent.Cancel ();
                }
            }
          else
            {
              NS_LOG_DEBUG ("Association refused");
              if (m_candidateAps.empty ())
                {
                  SetState (REFUSED);
                }
              else
                {
                  ScanningTimeout ();
                }
            }
        }
      return;
  }

  DmgWifiMac::Receive (mpdu);
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
  capabilities->SetRXSSLength (m_codebook->GetTotalNumberOfReceiveSectors ());
  capabilities->SetAmpduParameters (5, 0);      /* Hardcoded Now (Maximum A-MPDU + No restriction) */
  capabilities->SetSupportedMCS (GetDmgWifiPhy ()->GetMaxScRxMcs (), GetDmgWifiPhy ()->GetMaxOfdmRxMcs (),
                                 GetDmgWifiPhy ()->GetMaxScTxMcs (), GetDmgWifiPhy ()->GetMaxOfdmTxMcs (),
                                 GetDmgWifiPhy ()->GetSupportLpScPhy (), false);
  capabilities->SetAppduSupported (false);      /* Currently A-PPDU Agregation is not supported */
  capabilities->SetAntennaPatternReciprocity (m_antennaPatternReciprocity);

  return capabilities;
}

void
DmgStaWifiMac::UpdateCandidateApList (DmgApInfo &newApInfo)
{
  NS_LOG_FUNCTION (this << newApInfo.m_bssid << newApInfo.m_apAddr << newApInfo.m_snr
                   << newApInfo.m_activeProbing << newApInfo.m_beacon << newApInfo.m_probeResp);
  // Remove duplicate DMG PCP/AP Info entry.
  for (DmgApInfoList_I i = m_candidateAps.begin (); i != m_candidateAps.end (); ++i)
    {
      if ((newApInfo.m_bssid == (*i).m_bssid))
        {
          if (newApInfo.m_snr > (*i).m_snr)
            {
              NS_LOG_DEBUG ("Received DMG Beacon with higher SNR than the current stored DMG Beacon");
              m_candidateAps.erase (i);
              break;
            }
          else
            {
              NS_LOG_DEBUG ("Received DMG Beacon with lower SNR than the current stored DMG Beacon, so we keep the old one");
              return;
            }
        }
    }
  // Insert before the entry with lower SNR.
  for (DmgApInfoList_I i = m_candidateAps.begin (); i != m_candidateAps.end (); ++i)
    {
      if (newApInfo.m_snr > (*i).m_snr)
        {
          m_candidateAps.insert (i, newApInfo);
          return;
        }
    }
  // If the new DMG PCP/AP Info is the lowest, insert at the back of the list.
  m_candidateAps.push_back (newApInfo);
}

void
DmgStaWifiMac::SetState (MacState value)
{
  MacState previousState = m_state;
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
