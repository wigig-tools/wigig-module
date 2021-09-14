/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/pointer.h"
#include "ns3/boolean.h"

#include "dmg-wifi-mac.h"
#include "dmg-wifi-phy.h"

#include "channel-access-manager.h"
#include "mac-low.h"
#include "mac-tx-middle.h"
#include "mgt-headers.h"
#include "mpdu-aggregator.h"
#include "msdu-aggregator.h"
#include "wifi-mac-queue.h"
#include "wifi-utils.h"
#include "bft-id-tag.h"

#include <algorithm>
#include <queue>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DmgWifiMac");

NS_OBJECT_ENSURE_REGISTERED (DmgWifiMac);

TypeId
DmgWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgWifiMac")
    .SetParent<RegularWifiMac> ()
    .SetGroupName ("Wifi")

    /* DMG Operation Element */
    .AddAttribute ("PcpHandoverSupport", "Whether we support PCP Handover.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&DmgWifiMac::SetPcpHandoverSupport,
                                        &DmgWifiMac::GetPcpHandoverSupport),
                   MakeBooleanChecker ())

    /* Reverse Direction Protocol */
    .AddAttribute ("SupportRDP", "Whether the DMG STA supports Reverse Direction Protocol (RDP)",
                   BooleanValue (false),
                   MakeBooleanAccessor (&DmgWifiMac::m_supportRdp),
                   MakeBooleanChecker ())

      /* DMG Relay Capabilities common between PCP/AP and DMG STA */
    .AddAttribute ("REDSActivated", "Whether the DMG STA is REDS.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&DmgWifiMac::m_redsActivated),
                   MakeBooleanChecker ())
    .AddAttribute ("RDSActivated", "Whether the DMG STA is RDS.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&DmgWifiMac::m_rdsActivated),
                   MakeBooleanChecker ())
    .AddAttribute ("RelayDuplexMode", "The duplex mode of the relay.",
                    EnumValue (RELAY_BOTH),
                    MakeEnumAccessor (&DmgWifiMac::m_relayDuplexMode),
                    MakeEnumChecker (RELAY_FD_AF, "Full Duplex",
                                     RELAY_HD_DF, "Half Duplex",
                                     RELAY_BOTH, "Both"))
    /* EDMG parameters */
    .AddAttribute ("EDMGSupported", "Indicates that STA supports the IEEE 802.11ay protocol",
                    BooleanValue (false),
                    MakeBooleanAccessor (&DmgWifiMac::m_isEdmgSupported),
                    MakeBooleanChecker ())
    .AddAttribute ("UnsolicitedRSSEnabled", "Whether the station can receive unsolicited RSS.",
                    BooleanValue (false),
                    MakeEnumAccessor (&DmgWifiMac::m_isUnsolicitedRssEnabled),
                    MakeBooleanChecker ())
    .AddAttribute ("TrnSequenceLength", "Length of the Golay Sequences used in TRN subfields",
                    EnumValue (TRN_SEQ_LENGTH_NORMAL),
                    MakeEnumAccessor (&DmgWifiMac::m_trnSeqLength),
                    MakeEnumChecker (TRN_SEQ_LENGTH_NORMAL, "Normal Length - 128",
                                     TRN_SEQ_LENGTH_LONG, "Long Length - 256",
                                     TRN_SEQ_LENGTH_SHORT, "Short Length - 64"))
    .AddAttribute ("TrnScheduleInterval", "Periodic interval at which TRN-R fields are present in a BTI",
                    UintegerValue(0),
                    MakeUintegerAccessor(&DmgWifiMac::m_trnScheduleInterval),
                    MakeUintegerChecker<uint8_t> (0, 255))

    /* Antenna Pattern Reciprocity */
    .AddAttribute ("AntennaPatternReciprocity", "Indicates that STA supports reciprocity of the TX/RX antenna patterns",
                    BooleanValue (false),
                    MakeBooleanAccessor (&DmgWifiMac::m_antennaPatternReciprocity),
                    MakeBooleanChecker ())

   /* Use Rx Sectors */
    .AddAttribute ("UseRxSectors", "Indicates whether the STA should use the chosen Rx sectors during operation",
                    BooleanValue (true),
                    MakeBooleanAccessor (&DmgWifiMac::m_useRxSectors),
                    MakeBooleanChecker ())
    .AddAttribute ("InformationUpdateTimeout", "The interval between two consecutive information update attempts.",
                    TimeValue (MilliSeconds (10)),
                    MakeTimeAccessor (&DmgWifiMac::m_informationUpdateTimeout),
                    MakeTimeChecker ())

    /* Link Maintenance Attributes */
    .AddAttribute ("BeamLinkMaintenanceUnit", "The unit used for dot11BeamLinkMaintenanceTime calculation.",
                   EnumValue (UNIT_32US),
                   MakeEnumAccessor (&DmgWifiMac::m_beamlinkMaintenanceUnit),
                   MakeEnumChecker (UNIT_32US, "32US",
                                    UNIT_2000US, "2000US"))
    .AddAttribute ("BeamLinkMaintenanceValue", "The value of the beamlink maintenance used for dot11BeamLinkMaintenanceTime calculation.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&DmgWifiMac::m_beamlinkMaintenanceValue),
                   MakeUintegerChecker<uint8_t> (0, 63))

    /* Beacon Interval Traces */
    .AddTraceSource ("DTIStarted", "The Data Transmission Interval access period started.",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_dtiStarted),
                     "ns3::DmgWifiMac::DtiStartedTracedCallback")

    /* Service Period Allocation Traces */
    .AddTraceSource ("ServicePeriodStarted", "A service period between two DMG STAs has started.",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_servicePeriodStartedCallback),
                     "ns3::DmgWifiMac::ServicePeriodTracedCallback")
    .AddTraceSource ("ServicePeriodEnded", "A service period between two DMG STAs has ended.",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_servicePeriodEndedCallback),
                     "ns3::DmgWifiMac::ServicePeriodTracedCallback")

    /* DMG Beamforming Training Related Traces */
    .AddTraceSource ("SLSInitiatorStateMachine",
                     "Trace the current state of the SLS Initiator state machine.",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_slsInitiatorStateMachine),
                     "ns3::SlsInitiatorTracedValueCallback")
    .AddTraceSource ("SLSResponderStateMachine",
                     "Trace the current state of the SLS Responder state machine.",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_slsResponderStateMachine),
                     "ns3::SlsResponderTracedValueCallback")
    .AddTraceSource ("SLSCompleted", "Sector Level Sweep (SLS) phase is completed.",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_slsCompleted),
                     "ns3::DmgWifiMac::SLSCompletedTracedCallback")
    .AddTraceSource ("BRPCompleted", "BRP for transmit/receive beam refinement is completed",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_brpCompleted),
                     "ns3::DmgWifiMac::BRPCompletedTracedCallback")
    .AddTraceSource ("BeamLinkMaintenanceTimerStateChanged",
                     "The BeamLink maintenance timer associated to a link has expired.",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_beamLinkMaintenanceTimerStateChanged),
                     "ns3::DmgStaWifiMac::BeamLinkMaintenanceTimerStateChangedTracedCallback")

    /* DMG Relaying Related Traces */
    .AddTraceSource ("RlsCompleted",
                     "The Relay Link Setup (RLS) procedure is completed",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_rlsCompleted),
                     "ns3::Mac48Address::TracedCallback")

    /* EDMG Group Beamforming Training Related Traces */
    .AddTraceSource ("GroupBeamformingCompleted", "Group Beamforming is completed",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_groupBeamformingCompleted),
                     "ns3::DmgWifiMac::GroupBeamformingCompletedTracedCallback")

    /* EDMG SU-MIMO Beamforming Training Related Traces */
    .AddTraceSource ("SU_MIMO_StateMachine",
                     "Trace the current state of the SU-MIMO beamforming training state machine.",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_suMimoBfPhase),
                     "ns3::SU_MIMO_BFT_TracedValueCallback")
    .AddTraceSource ("SuMimoSisoPhaseMeasurements",
                     "Trace the SU-MIMO SISO phase measurements.",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_suMimoSisoPhaseMeasurements),
                     "ns3::DmgWifiMac::SuMimoSisoPhaseMeasurementsTracedCallback")
    .AddTraceSource ("SuMimoSisoPhaseCompleted",
                     "SU-MIMO SISO phase beamforming training is completed.",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_suMimoSisoPhaseComplete),
                     "ns3::DmgWifiMac::SuMimoSisoPhaseCompletedTracedCallback")
    .AddTraceSource ("SuMimoMimoCandidatesSelected",
                     "Candidates for MIMO phase of SU MIMO BFT have been selected",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_suMimomMimoCandidatesSelected),
                     "ns3::DmgWifiMac::SuMimoMimoCandidatesSelectedTracedCallback")
    .AddTraceSource ("SuMimoMimoPhaseMeasurements",
                     "Trace the SU-MIMO MIMO phase measurements.",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_suMimoMimoPhaseMeasurements),
                     "ns3::DmgWifiMac::SuMimoMimoPhaseMeasurementsTracedCallback")
    .AddTraceSource ("SuMimoMimoPhaseCompleted",
                     "SU-MIMO MIMO phase beamforming training is completed.",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_suMimoMimoPhaseComplete),
                     "ns3::DmgWifiMac::SuMimoMimoPhaseCompletedTracedCallback")

    /* EDMG MU-MIMO Beamforming Training Related Traces */
    .AddTraceSource ("MU_MIMO_StateMachine",
                     "Trace the current state of the MU-MIMO beamforming training state machine.",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_muMimoBfPhase),
                     "ns3::MU_MIMO_BFT_TracedValueCallback")
    .AddTraceSource ("MuMimoSisoPhaseMeasurements",
                     "Trace the MU-MIMO SISO phase measurements.",
                      MakeTraceSourceAccessor (&DmgWifiMac::m_muMimoSisoPhaseMeasurements),
                      "ns3::DmgWifiMac::MuMimoSisoPhaseMeasurementsTracedCallback")
    .AddTraceSource ("MuMimoSisoPhaseCompleted",
                     "MU-MIMO SISO phase beamforming training is completed.",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_muMimoSisoPhaseComplete),
                     "ns3::DmgWifiMac::MuMimoSisoPhaseCompletedTracedCallback")
    .AddTraceSource ("MuMimoMimoCandidatesSelected",
                     "Candidates for MIMO phase of MU MIMO BFT have been selected",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_muMimomMimoCandidatesSelected),
                     "ns3::DmgWifiMac::MuMimoMimoCandidatesSelectedTracedCallback")
    .AddTraceSource ("MuMimoMimoPhaseMeasurements",
                     "Trace the MU-MIMO MIMO phase measurements.",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_muMimoMimoPhaseMeasurements),
                     "ns3::DmgWifiMac::MuMimoMimoPhaseMeasurementsTracedCallback")
    .AddTraceSource ("MuMimoOptimalConfiguration",
                     "The optimal MU-MIMO Configuration chosen at the end of the MU-MIMO BFT.",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_muMimoOptimalConfig),
                     "ns3::DmgWifiMac::MuMimoOptimalConfigurationTracedCallback")
    .AddTraceSource ("MuMimoMimoPhaseCompleted",
                     "MU-MIMO MIMO phase beamforming training is completed.",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_muMimoMimoPhaseComplete),
                     "ns3::DmgWifiMac::MuMimoMimoPhaseCompletedTracedCallback")
    .AddTraceSource ("MuMimoSisoFbckPolled",
                     "We received a Poll frame during the SISO Fbck phase of MU-MIMO BFT",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_muMimoSisoFbckPolled),
                     "ns3::DmgWifiMac::MuMimoSisoFbckPolledTracedCallback")
  ;
  return tid;
}

DmgWifiMac::DmgWifiMac ()
  : m_maxSnr (0.0),
    m_recordTrnSnrValues (false),
    m_restartISSEvent (),
    m_sswFbckTimeout (),
    m_sswAckTimeoutEvent (),
    m_rssEvent (),
    m_suMimoBeamformingTraining (false),
    m_muMimoBeamformingTraining (false),
    m_isMuMimoInitiator (false),
    m_muMimoFbckTimeout (),
    m_beamLinkMaintenanceTimeout (),
    m_informationUpdateEvent ()

{
  NS_LOG_FUNCTION (this);
  /* DMG Managment TXOP */
  m_txop->SetTxOkNoAckCallback (MakeCallback (&DmgWifiMac::ManagementTxOk, this));
  /* DMG ATI TXOP Initialization */
  m_dmgAtiTxop = CreateObject<DmgAtiTxop> ();
  m_dmgAtiTxop->SetAifsn (0);
  m_dmgAtiTxop->SetMinCw (0);
  m_dmgAtiTxop->SetMaxCw (0);
  m_dmgAtiTxop->SetMacLow (m_low);
  m_dmgAtiTxop->SetChannelAccessManager (m_channelAccessManager);
  m_dmgAtiTxop->SetTxMiddle (m_txMiddle);
  m_dmgAtiTxop->SetTxOkCallback (MakeCallback (&DmgWifiMac::TxOk, this));
  m_dmgAtiTxop->SetTxOkNoAckCallback (MakeCallback (&DmgWifiMac::ManagementTxOk, this));
  /* DMG SLS TXOP Initialization */
  m_dmgSlsTxop = CreateObject<DmgSlsTxop> ();
  m_dmgSlsTxop->SetAifsn (1);
  m_dmgSlsTxop->SetMinCw (15);
  m_dmgSlsTxop->SetMaxCw (1023);
  m_dmgSlsTxop->SetMacLow (m_low);
  m_dmgSlsTxop->SetChannelAccessManager (m_channelAccessManager);
  m_dmgSlsTxop->SetTxMiddle (m_txMiddle);
  m_dmgSlsTxop->SetTxOkNoAckCallback (MakeCallback (&DmgWifiMac::FrameTxOk, this));
  m_dmgSlsTxop->SetAccessGrantedCallback (MakeCallback (&DmgWifiMac::TXSS_TXOP_Access_Granted, this));
  m_edmgGroupIdSetElement = Create <EDMGGroupIDSetElement> ();
  /* EDMG BF TRN variables initialization */
  /* Note: Default setting to one of the combinations that are mandatory to support by all EDMG capable STAs
   * one TRN Unit will be equal to 2 + 8 = 10 subfields to match the TRN Unit when TRN-R subfields are used. */
  m_edmgTrnP = 2;
  m_edmgTrnM = 9;
  m_edmgTrnN = 1;
}

DmgWifiMac::~DmgWifiMac ()
{
  NS_LOG_FUNCTION (this);
}

void
DmgWifiMac::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_dmgAtiTxop = 0;
  m_codebook->Dispose ();
  m_codebook = 0;
  RegularWifiMac::DoDispose ();
}

void
DmgWifiMac::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);

  /* Initialize beamforming training variables */
  Reset_SLS_State_Machine_Variables ();

  /* IEEE 802.11ay SU/MU-MIMO BFT variables */
  m_suMimoBfPhase = SU_WAIT_SU_MIMO_BF_TRAINING;
  m_muMimoBfPhase = MU_WAIT_MU_MIMO_BF_TRAINING;
  m_timeDomainChannelResponseRequested = false;

  /* PHY Layer Information */

  /* Beamforming variables */
  m_chAggregation = false;
  m_requestedBrpTraining = false;
  m_currentLinkMaintained = false;
  m_sectorFeedbackSchedulled = false;
  GetDmgWifiPhy ()->RegisterReportSnrCallback (MakeCallback (&DmgWifiMac::ReportSnrValue, this));

  /* Beam Link Maintenance */
  if (m_beamlinkMaintenanceUnit == UNIT_32US)
    {
      dot11BeamLinkMaintenanceTime = MicroSeconds (m_beamlinkMaintenanceValue * 32);
    }
  else
    {
      dot11BeamLinkMaintenanceTime = MicroSeconds (m_beamlinkMaintenanceValue * 2000);
    }

  /* Initialzie Codebook */
  m_codebook->Initialize ();

  /* Channel Access Periods */
  m_dmgAtiTxop->Initialize ();
  m_dmgSlsTxop->Initialize ();
  m_feedbackSnr = 0.0;
  m_brpCdown = 0;

  /* Initialzie Upper Layers */
  RegularWifiMac::DoInitialize ();
  if (m_isEdmgSupported && (GetDmgWifiPhy ()->IsSuMimoSupported () || GetDmgWifiPhy ()->IsMuMimoSupported ()))
    {
      GetDmgWifiPhy ()->RegisterEndReceiveMimoTRNCallback (MakeCallback (&DmgWifiMac::EndMimoTrnField, this));
      GetDmgWifiPhy ()->RegisterReportMimoSnrCallback (MakeCallback (&DmgWifiMac::ReportMimoSnrValue, this));
    }
}

Ptr<DmgWifiPhy>
DmgWifiMac::GetDmgWifiPhy (void) const
{
  return StaticCast<DmgWifiPhy> (m_phy);
}

void
DmgWifiMac::SetAddress (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  RegularWifiMac::SetAddress (address);
}

void
DmgWifiMac::SetWifiRemoteStationManager (Ptr<WifiRemoteStationManager> stationManager)
{
  NS_LOG_FUNCTION (this << stationManager);
  m_dmgAtiTxop->SetWifiRemoteStationManager (stationManager);
  m_txop->SetWifiRemoteStationManager (stationManager);
  RegularWifiMac::SetWifiRemoteStationManager (stationManager);
}

void
DmgWifiMac::SetSbifs (Time sbifs)
{
  NS_LOG_FUNCTION (this << sbifs);
  m_sbifs = sbifs;
  m_low->SetSbifs (sbifs);
}

void
DmgWifiMac::SetMbifs (Time mbifs)
{
  NS_LOG_FUNCTION (this << mbifs);
  m_mbifs = mbifs;
  m_low->SetMbifs (mbifs);
}

void
DmgWifiMac::SetLbifs (Time lbifs)
{
  NS_LOG_FUNCTION (this << lbifs);
  m_lbifs = lbifs;
  m_low->SetLbifs (lbifs);
}

void
DmgWifiMac::SetBrpifs (Time brpifs)
{
  NS_LOG_FUNCTION (this << brpifs);
  m_brpifs = brpifs;
}

Time
DmgWifiMac::GetSbifs (void) const
{
  return m_sbifs;
}

Time
DmgWifiMac::GetMbifs (void) const
{
  return m_mbifs;
}

Time
DmgWifiMac::GetLbifs (void) const
{
  return m_lbifs;
}

Time
DmgWifiMac::GetBrpifs (void) const
{
  return m_brpifs;
}

void
DmgWifiMac::SetPcpHandoverSupport (bool value)
{
  m_pcpHandoverSupport = value;
}

bool
DmgWifiMac::GetPcpHandoverSupport (void) const
{
  return m_pcpHandoverSupport;
}

void
DmgWifiMac::Configure80211ad (void)
{
  WifiMac::Configure80211ad ();
  /* DMG Beamforming IFS */
  SetSbifs (MicroSeconds (1));
  SetMbifs (GetSifs () * 3);
  SetLbifs (GetSifs () * 6);
  SetBrpifs (MicroSeconds (40));
}

void
DmgWifiMac::Configure80211ay (void)
{
  WifiMac::Configure80211ay ();
  /* EDMG Beamforming IFS */
  SetSbifs (MicroSeconds (1));
  SetMbifs (GetSifs () * 3);
  SetLbifs (GetSifs () * 6);
  SetBrpifs (MicroSeconds (40));
}

void
DmgWifiMac::MapAidToMacAddress (uint16_t aid, Mac48Address address)
{
  NS_LOG_FUNCTION (this << aid << address);
  m_aidMap[aid] = address;
  m_macMap[address] = aid;
}

Time
DmgWifiMac::GetFrameDurationInMicroSeconds (Time duration) const
{
  return MicroSeconds (ceil ((double) duration.GetNanoSeconds () / 1000));
}

Time
DmgWifiMac::GetSprFrameDuration (void) const
{
  return GetFrameDurationInMicroSeconds (m_phy->CalculateTxDuration (SPR_FRAME_SIZE,
                                         m_stationManager->GetDmgLowestScVector (), 0));
}

void
DmgWifiMac::AddMcsSupport (Mac48Address address, uint32_t initialMcs, uint32_t lastMcs)
{
  for (uint32_t j = initialMcs; j <= lastMcs; j++)
    {
      m_stationManager->AddSupportedMode (address, m_phy->GetMode (j));
    }
}

ChannelAccessPeriod
DmgWifiMac::GetCurrentAccessPeriod (void) const
{
  return m_accessPeriod;
}

AllocationType
DmgWifiMac::GetCurrentAllocation (void) const
{
  return m_currentAllocation;
}

void
DmgWifiMac::ResumePendingTXSS (void)
{
  NS_LOG_FUNCTION (this);
  m_dmgSlsTxop->ResumeTXSS ();
}

void
DmgWifiMac::StartContentionPeriod (AllocationID allocationID, Time contentionDuration)
{
  NS_LOG_FUNCTION (this << uint16_t (allocationID) << contentionDuration);
  m_currentAllocation = CBAP_ALLOCATION;
  if (GetTypeOfStation () == DMG_STA)
    {
      /* For the time being we assume in CBAP we communicate with the DMG PCP/AP only */
      SteerAntennaToward (GetBssid ());
    }
  /* Allow Contention Access */
  m_channelAccessManager->AllowChannelAccess ();
  /* Restore previously suspended transmission in LowMac */
  m_low->RestoreAllocationParameters (allocationID);
  /* Signal Txop, QosTxop, and SLS Txop Functions to start channel access */
  m_txop->StartAllocationPeriod (CBAP_ALLOCATION, allocationID, GetBssid (), contentionDuration);
  for (EdcaQueues::iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->StartAllocationPeriod (CBAP_ALLOCATION, allocationID, GetBssid (), contentionDuration);
    }
  ResumePendingTXSS ();
  /* Schedule the end of the contention period */
  Simulator::Schedule (contentionDuration, &DmgWifiMac::EndContentionPeriod, this);
  NS_ASSERT_MSG (Simulator::Now () + contentionDuration <= m_dtiStartTime + m_dtiDuration, "Exceeding DTI Time, Error");
}

void
DmgWifiMac::EndContentionPeriod (void)
{
  NS_LOG_FUNCTION (this);
  // End reception of TRN fields on the Physical Layer
//  StaticCast<DmgWifiPhy> (m_phy)->EndTRNReception ();
  m_channelAccessManager->DisableChannelAccess ();
  /* Signal Management DCA to suspend current transmission */
  m_txop->EndAllocationPeriod ();
  /* Signal EDCA queues to suspend current transmission */
  for (EdcaQueues::iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->EndAllocationPeriod ();
    }
  /* Inform MAC Low to store parameters related to this service period (MPDU/A-MPDU) */
  m_low->EndAllocationPeriod ();
//  m_phy->EndAllocationPeriod ();
}

void
DmgWifiMac::BeamLinkMaintenanceTimeout (void)
{
  NS_LOG_FUNCTION (this);
  m_beamLinkMaintenanceTimerStateChanged (BEAM_LINK_MAINTENANCE_TIMER_EXPIRES,
                                          m_peerStationAid, m_peerStationAddress, Seconds (0.0));
}

void
DmgWifiMac::ScheduleServicePeriod (uint8_t blocks, Time spStart, Time spLength, Time spPeriod,
                                   AllocationID allocationID, uint8_t peerAid, Mac48Address peerAddress, bool isSource)
{
  NS_LOG_FUNCTION (this << blocks << spStart << spLength << spPeriod
                   << static_cast<uint16_t> (allocationID) << static_cast<uint16_t> (peerAid) << peerAddress << isSource);
  /* We allocate multiple blocks of this allocation as in (9.33.6 Channel access in scheduled DTI) */
  /* A_start + (i – 1) × A_period */
  /* Check if there is currently a reception on the PHY layer */
  Time endRx = StaticCast<DmgWifiPhy> (m_phy)->GetDelayUntilEndRx ();
  if (spPeriod > 0)
    {
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
          Simulator::Schedule (spStartNew, &DmgWifiMac::StartServicePeriod, this,
                               allocationID, spLengthNew, peerAid, peerAddress, isSource);
          Simulator::Schedule (spStartNew + spLengthNew, &DmgWifiMac::EndServicePeriod, this);
          spStart += spLength + spPeriod + GUARD_TIME;
        }
    }
  else
    {
      /* Special case when Allocation Block Period=0 i.e. consecutive blocks *
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
      Simulator::Schedule (spStart, &DmgWifiMac::StartServicePeriod, this,
                           allocationID, spLength, peerAid, peerAddress, isSource);
      Simulator::Schedule (spStart + spLength, &DmgWifiMac::EndServicePeriod, this);
    }
}

void
DmgWifiMac::StartServicePeriod (AllocationID allocationID, Time length, uint8_t peerAid, Mac48Address peerAddress, bool isSource)
{
  NS_LOG_FUNCTION (this << length << static_cast<uint16_t> (peerAid) << peerAddress << isSource << Simulator::Now ());
  m_currentAllocationID = allocationID;
  m_currentAllocation = SERVICE_PERIOD_ALLOCATION;
  m_currentAllocationLength = length;
  m_allocationStarted = Simulator::Now ();
  m_peerStationAid = peerAid;
  m_peerStationAddress = peerAddress;
  m_spSource = isSource;
  m_servicePeriodStartedCallback (GetAddress (), peerAddress);
  SteerAntennaToward (peerAddress);
  /* Restore previously suspended transmission in LowMac */
  m_low->RestoreAllocationParameters (allocationID);
  m_edca[AC_BE]->StartAllocationPeriod (SERVICE_PERIOD_ALLOCATION, allocationID, peerAddress, length);
  if (isSource)
    {
      /* Check if we are maintaining the beamformed link during this service period as initiator */
      BeamLinkMaintenanceTableI it = m_beamLinkMaintenanceTable.find (peerAid);
      if (it != m_beamLinkMaintenanceTable.end ())
        {
          BeamLinkMaintenanceInfo info = it->second;
          m_currentLinkMaintained = true;
          m_linkMaintenanceInfo = info;
          m_beamLinkMaintenanceTimeout = Simulator::Schedule (info.beamLinkMaintenanceTime,
                                                              &DmgWifiMac::BeamLinkMaintenanceTimeout, this);
          m_beamLinkMaintenanceTimerStateChanged (BEAM_LINK_MAINTENANCE_TIMER_RELEASE,
                                                  m_peerStationAid, m_peerStationAddress, info.beamLinkMaintenanceTime);
        }
      else
        {
          m_currentLinkMaintained = false;
        }

      /* Start data transmission */
      m_edca[AC_BE]->InitiateServicePeriodTransmission ();
    }
}

void
DmgWifiMac::ResumeServicePeriodTransmission (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_currentAllocation == SERVICE_PERIOD_ALLOCATION, "The current allocation is not SP");
  m_currentAllocationLength = GetRemainingAllocationTime ();
  m_edca[AC_BE]->ResumeTransmission (m_currentAllocationLength);
}

void
DmgWifiMac::SuspendServicePeriodTransmission (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_currentAllocation == SERVICE_PERIOD_ALLOCATION, "The current allocation is not SP");
  m_edca[AC_BE]->DisableChannelAccess ();
  m_suspendedPeriodDuration = GetRemainingAllocationTime ();
}

void
DmgWifiMac::EndServicePeriod (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_currentAllocation == SERVICE_PERIOD_ALLOCATION, "The current allocation is not SP");
  m_servicePeriodEndedCallback (GetAddress (), m_peerStationAddress);
  m_edca[AC_BE]->EndAllocationPeriod ();
  /* Inform MacLow to store parameters related to this service period (MPDU/A-MPDU) */
  m_low->EndAllocationPeriod ();
  /* Check if we have beamlink maintenance timer running */
  if (m_beamLinkMaintenanceTimeout.IsRunning ())
    {
      BeamLinkMaintenanceTableI it = m_beamLinkMaintenanceTable.find (m_peerStationAid);
      BeamLinkMaintenanceInfo info = it->second;
      /* We halt Beam Link Maintenance Timer */
      if (m_beamLinkMaintenanceTimeout.IsRunning ())
        {
          info.beamLinkMaintenanceTime = Simulator::GetDelayLeft (m_beamLinkMaintenanceTimeout);
          m_beamLinkMaintenanceTimeout.Cancel ();
          m_beamLinkMaintenanceTimerStateChanged (BEAM_LINK_MAINTENANCE_TIMER_HALT,
                                                  m_peerStationAid, m_peerStationAddress, info.beamLinkMaintenanceTime);
        }
      else
        {
          info.rest ();
        }
      m_beamLinkMaintenanceTable[m_peerStationAid] = info;
    }
  m_currentLinkMaintained = false;
}

void
DmgWifiMac::AddForwardingEntry (Mac48Address nextHopAddress)
{
  NS_LOG_FUNCTION (this << nextHopAddress);
  DataForwardingTableIterator it = m_dataForwardingTable.find (nextHopAddress);
  if (it == m_dataForwardingTable.end ())
    {
      AccessPeriodInformation info;
      info.isCbapPeriod = true;
      info.nextHopAddress = nextHopAddress;
      m_dataForwardingTable[nextHopAddress] = info;
    }
}

Time
DmgWifiMac::GetRemainingAllocationTime (void) const
{
  return m_currentAllocationLength - (Simulator::Now () - m_allocationStarted);
}

Time
DmgWifiMac::GetRemainingSectorSweepTime (void) const
{
  if (m_isEdmgSupported)
    {
      return m_sectorSweepDuration -edmgSswTxTime - (Simulator::Now () - m_sectorSweepStarted);
    }
  else
    {
      return m_sectorSweepDuration -sswTxTime - (Simulator::Now () - m_sectorSweepStarted);
    }
}

/**************************************************
***************************************************
************* Beamforming Functions ***************
***************************************************
***************************************************/

void
DmgWifiMac::SetCodebook (Ptr<Codebook> codebook)
{
  m_codebook = codebook;
}

Ptr<Codebook>
DmgWifiMac::GetCodebook (void) const
{
  return m_codebook;
}

void
DmgWifiMac::RecordBeamformedLinkMaintenanceValue (BF_Link_Maintenance_Field field)
{
  NS_LOG_FUNCTION (this);
  if (field.GetMaintenanceValue () > 0)
    {
      BeamLinkMaintenanceInfo maintenanceInfo;
      /* Table 8-190b—The Beamformed Link Maintenance negotiation */
      if (field.IsMaster ())
        {
          Time beamLinkMaintenanceTime;
          if (m_beamlinkMaintenanceUnit == UNIT_32US)
            {
              beamLinkMaintenanceTime = MicroSeconds (field.GetMaintenanceValue () * 32);
            }
          else
            {
              beamLinkMaintenanceTime = MicroSeconds (field.GetMaintenanceValue () * 2000);
            }
          maintenanceInfo.beamLinkMaintenanceTime = beamLinkMaintenanceTime;
          maintenanceInfo.negotiatedValue = beamLinkMaintenanceTime;
        }
      else
        {
          maintenanceInfo.beamLinkMaintenanceTime = dot11BeamLinkMaintenanceTime;
          maintenanceInfo.negotiatedValue = dot11BeamLinkMaintenanceTime;
        }
      m_beamLinkMaintenanceTable[m_peerStationAid] = maintenanceInfo;
    }
}

void
DmgWifiMac::Perform_TXSS_TXOP (Mac48Address peerAddress)
{
  NS_LOG_FUNCTION (this << peerAddress);
  m_dmgSlsTxop->Initiate_TXOP_Sector_Sweep (peerAddress);
  /* For future use */
//  BF_Control_Field bf;
//  bf.SetBeamformTraining (true);
//  bf.SetAsInitiatorTXSS (isInitiatorTXSS);
//  bf.SetAsResponderTXSS (isResponderTXSS);
//  bf.SetTotalNumberOfSectors (m_codebook->GetTotalNumberOfTransmitSectors ());
//  bf.SetNumberOfRxDmgAntennas (m_codebook->GetTotalNumberOfAntennas ());
//  DynamicAllocationInfoField info;
//  info.SetAllocationType (CBAP_ALLOCATION);
//  info.SetSourceAID (GetAssociationID ());
//  info.SetDestinationAID (peerAid);
//  SendGrantFrame (peerAddress, MicroSeconds (3000), info, bf);
}

void
DmgWifiMac::TXSS_TXOP_Access_Granted (Mac48Address peerAddress, SLS_ROLE slsRole, bool isFeedback)
{
  NS_LOG_FUNCTION (this << peerAddress << slsRole << isFeedback);
  if (slsRole == SLS_INITIATOR)
    {
      /* We are the SLS initiator */
      if (!isFeedback)
        {
          /* Initialize Beamforming Training Parameters for TXSS TXOP and make sure we have enough time to execute it */
          if (Initialize_Sector_Sweep_Parameters (peerAddress))
            {
              if (!m_performingBFT) // This means that we've started TXSS BFT but it failed
                {
                  /* Remove current Sector Sweep Information with the station we want to perform beamforming training with */
                  m_stationSnrMap.erase (peerAddress);
                  /* Reset variables */
                  m_bfRetryTimes = 0;
                  m_isBeamformingInitiator = true;
                  m_isInitiatorTXSS = true;
                  m_isResponderTXSS = true;
                  m_performingBFT = true;
                  m_peerStationAddress = peerAddress;
                  m_slsInitiatorStateMachine = SLS_INITIATOR_IDLE;
                }
              if (m_restartISSEvent.IsRunning ())
                {
                  /* This happens if we cannot continue beamforming training since the allocation did not have enough time */
                  m_restartISSEvent.Cancel ();
                }
              /* Start Beamforming Training Training as I-TXSS */
              StartBeamformingInitiatorPhase ();
            }
        }
      else
        {
          if (m_isEdmgSupported)
            {
              if (Simulator::Now () + EDMG_SLS_FEEDBACK_PHASE_DURATION <= m_dtiStartTime + m_dtiDuration)
                {
                  SendSswFbckFrame (peerAddress, edmgSswAckTxTime + GetMbifs ());
                }
//          else if (!m_dmgSlsTxop->ResumeCbapBeamforming ())
//            {
//              std::cout << Simulator::Now ().GetNanoSeconds () << ", " << GetAddress () << ", TXSS_TXOP_Access_Granted, "
//                        << ", No time" << std::endl;
//              m_dmgSlsTxop->InitializeVariables ();
//            }
            }
          else
            {
              if (Simulator::Now () + SLS_FEEDBACK_PHASE_DURATION <= m_dtiStartTime + m_dtiDuration)
                {
                  SendSswFbckFrame (peerAddress, sswAckTxTime + GetMbifs ());
                }
//            else if (!m_dmgSlsTxop->ResumeCbapBeamforming ())
//            {
//              std::cout << Simulator::Now ().GetNanoSeconds () << ", " << GetAddress () << ", TXSS_TXOP_Access_Granted, "
//                        << ", No time" << std::endl;
//              m_dmgSlsTxop->InitializeVariables ();
//            }
            }
        }
    }
  else
    {
      /* We are the SLS responder */
      if (Initialize_Sector_Sweep_Parameters (peerAddress))
        {
          m_isBeamformingInitiator = false;
          /* Start Beamforming Training Training as R-TXSS */
          StartBeamformingResponderPhase (peerAddress);
        }
//      else if (!m_dmgSlsTxop->ResumeCbapBeamforming ())
//        {
      //          std::cout << Simulator::Now ().GetNanoSeconds () << ", " << GetAddress () << ", TXSS_TXOP_Access_Granted, "
      //                    << ", No time" << std::endl;
      //          m_dmgSlsTxop->InitializeVariables ();
      //        }
      else
        {
          m_dmgSlsTxop->InitializeVariables ();
        }
    }
}

void
DmgWifiMac::Reset_SLS_Initiator_Variables (void)
{
  NS_LOG_FUNCTION (this);
  m_slsInitiatorStateMachine = SLS_INITIATOR_IDLE;
  m_performingBFT = false;
  m_bfRetryTimes = 0;
}

void
DmgWifiMac::Reset_SLS_Responder_Variables (void)
{
  NS_LOG_FUNCTION (this);
  m_slsResponderStateMachine = SLS_RESPONDER_IDLE;
  m_performingBFT = false;
}

void
DmgWifiMac::Reset_SLS_State_Machine_Variables (void)
{
  NS_LOG_FUNCTION (this);
  m_slsInitiatorStateMachine = SLS_INITIATOR_IDLE;
  m_slsResponderStateMachine = SLS_RESPONDER_IDLE;
  m_performingBFT = false;
  m_bfRetryTimes = 0;
}

bool
DmgWifiMac::Initialize_Sector_Sweep_Parameters (Mac48Address peerAddress)
{
  NS_LOG_FUNCTION (this << peerAddress);
  NS_ASSERT_MSG (m_currentAllocation == CBAP_ALLOCATION,
                 "Current Allocation is not CBAP and we are performing SLS within CBAP");
  /* Ensure that we have the capabilities of the peer station */
  Ptr<DmgCapabilities> peerCapabilities = GetPeerStationDmgCapabilities (peerAddress);
  NS_ASSERT_MSG (peerCapabilities != 0, "To continue beamforming we should have the capabilities of the peer station.");
  m_peerSectors = peerCapabilities->GetNumberOfSectors ();
  m_peerAntennas = peerCapabilities->GetNumberOfRxDmgAntennas ();
  Time duration = CalculateSectorSweepDuration (m_peerAntennas, m_codebook->GetTotalNumberOfAntennas (),
                                                m_codebook->GetTotalNumberOfTransmitSectors ());
  if (Simulator::Now () + duration <= m_dtiStartTime + m_dtiDuration)
    {
      /* Beamforming Allocation Parameters */
      m_allocationStarted = Simulator::Now ();
      m_currentAllocationLength = duration;
      return true;
    }
  else
    {
      NS_LOG_DEBUG ("No enough time to complete TXSS beamforming training");
      return false;
    }
}

void
DmgWifiMac::StartBeamformingTraining (uint8_t peerAid, Mac48Address peerAddress,
                                      bool isInitiator, bool isInitiatorTXSS, bool isResponderTXSS, Time length)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (peerAid) << peerAddress
                   << isInitiator << isInitiatorTXSS << isResponderTXSS << length);

  /* Ensure that we have the capabilities of the peer station */
  Ptr<DmgCapabilities> peerCapabilities = GetPeerStationDmgCapabilities (peerAddress);
  NS_ASSERT_MSG (peerCapabilities != 0, "To continue beamforming we should have the capabilities of the peer station.");
  m_peerSectors = peerCapabilities->GetNumberOfSectors ();
  m_peerAntennas = peerCapabilities->GetNumberOfRxDmgAntennas ();

  /* Beamforming Allocation Parameters */
  m_allocationStarted = Simulator::Now ();
  m_currentAllocation = SERVICE_PERIOD_ALLOCATION;
  m_currentAllocationLength = length;
  m_peerStationAid = peerAid;
  m_peerStationAddress = peerAddress;
  m_isBeamformingInitiator = isInitiator;
  m_isInitiatorTXSS = isInitiatorTXSS;
  m_isResponderTXSS = isResponderTXSS;

  /* Remove current Sector Sweep Information */
  m_stationSnrMap.erase (peerAddress);

  /* Reset variables */
  m_bfRetryTimes = 0;

  NS_LOG_INFO ("DMG STA Initiating Beamforming with " << peerAddress << " at " << Simulator::Now ());
  StartBeamformingInitiatorPhase ();
}

void
DmgWifiMac::StartBeamformingInitiatorPhase (void)
{
  NS_LOG_FUNCTION (this);
  m_sectorSweepStarted = Simulator::Now ();
  if (m_isBeamformingInitiator)
    {
      NS_LOG_INFO ("DMG STA Starting ISS Phase with Initiator Role at " << Simulator::Now ());
      /** We are the Initiator of the Beamforming Phase **/
      /* Schedule Beamforming Responder Phase */
      Time rssTime = CalculateSectorSweepDuration (m_peerAntennas, m_codebook->GetTotalNumberOfAntennas (),
                                                   m_codebook->GetTotalNumberOfTransmitSectors ());
      NS_LOG_DEBUG ("Initiator: Schedulled RSS Event at " << Simulator::Now () + rssTime);
      m_rssEvent = Simulator::Schedule (rssTime, &DmgWifiMac::StartBeamformingResponderPhase, this, m_peerStationAddress);
      //// NINA ////
      /* Set the BFT ID of the current BFT - if this is the first BFT with the peer STA, initialize it to 0, otherwise increase it by 1 to signal a new BFT */
      BFT_ID_MAP::iterator it = m_bftIdMap.find (m_peerStationAddress);
      if (it != m_bftIdMap.end ())
        {
          uint16_t bftId = it->second + 1;
          m_bftIdMap [m_peerStationAddress] = bftId;
        }
      else
        {
          m_bftIdMap [m_peerStationAddress] = 0;
        }
      //// NINA ////
      if (m_isInitiatorTXSS)
        {
          m_slsInitiatorStateMachine = SLS_INITIATOR_SECTOR_SELECTOR;
          StartTransmitSectorSweep (m_peerStationAddress, BeamformingInitiator);
        }
      else
        {
          StartReceiveSectorSweep (m_peerStationAddress, BeamformingInitiator);
        }
    }
  else
    {
      NS_LOG_INFO ("DMG STA Starting ISS Phase with Responder Role at " << Simulator::Now ());
      m_slsResponderStateMachine = SLS_RESPONDER_IDLE;
      /** We are the Responder of the Beamforming Phase **/
      if (m_isInitiatorTXSS)
        {
          /* I-TXSS so responder stays in Quasi-Omni Receiving Mode */
          m_codebook->StartReceivingInQuasiOmniMode ();

          /* If an ISS is outside the BTI and if the responder has more than one DMG antenna, the initiator repeats its
           * initiator sector sweep for the number of DMG antennas indicated by the responder in the last negotiated
           * Number of RX DMG Antennas field that was transmitted by the responder. Repetitions of the initiator sector
           * sweep are separated by an interval equal to LBIFS. In this case CDOWN indicates the number of sectors
           * until the end of transmission from all initiator’s DMG antennas to all responder’s DMG antennas. At the
           * start of an initiator TXSS, the responder should have its first receive DMG antenna configured to a quasi-
           * omni pattern and should not change its receive antenna configuration for a time corresponding to the value
           * of the last negotiated Total Number of Sectors field transmitted by the initiator multiplied by the time to
           * transmit a single SSW frame, plus appropriate IFSs (10.3.2.3). After this time, the responder may switch to a
           * quasi-omni pattern in another DMG antenna. */
          if (m_codebook->GetTotalNumberOfAntennas () > 1)
            {
              Time switchTime = CalculateSingleAntennaSweepDuration (m_peerAntennas, m_peerSectors) + GetLbifs ();
              Simulator::Schedule (switchTime, &DmgWifiMac::SwitchQuasiOmniPattern, this, switchTime);
            }
        }
      else
        {
          /* I-RXSS so the responder should have its receive antenna array configured to sweep RXSS
           * Length sectors for each of the initiator’s DMG antennas while attempting to receive SSW
           * frames from the initiator. */
          m_codebook->StartSectorSweeping (m_peerStationAddress, ReceiveSectorSweep, 1);
        }
    }
}

void
DmgWifiMac::StartBeamformingResponderPhase (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_sectorSweepStarted = Simulator::Now ();
  if (m_isBeamformingInitiator)
    {
      /** We are the Initiator **/
      NS_LOG_INFO ("DMG STA Starting RSS Phase with Initiator Role at " << Simulator::Now ());
      if (m_isInitiatorTXSS)
        {
          /* We performed Initiator Transmit Sector Sweep (I-TXSS) */
          m_codebook->StartReceivingInQuasiOmniMode ();

          /* If the initiator has more than one DMG antenna, the responder repeats its responder sector sweep for the
           * number of DMG antennas indicated by the initiator in the last negotiated Number of RX DMG Antennas
           * field transmitted by the initiator. At the start of a responder TXSS, the initiator should have its receive
           * antenna array configured to a quasi-omni antenna pattern in one of its DMG antennas for a time
           * corresponding to the value of the last negotiated Total Number of Sectors field transmitted by the responder
           * multiplied by the time to transmit a single SSW frame, plus any appropriate IFSs (9.3.2.3). After this time,
           * the initiator may switch to a quasi-omni pattern in another DMG antenna. */
          if (m_codebook->GetTotalNumberOfAntennas () > 1)
            {
              Time switchTime = CalculateSectorSweepDuration (m_peerSectors) + GetLbifs ();
              Simulator::Schedule (switchTime, &DmgWifiMac::SwitchQuasiOmniPattern, this, switchTime);
            }
        }
      else
        {
          /* We performed Initiator Receive Sector Sweep (I-RXSS) */
          ANTENNA_CONFIGURATION_RX rxConfig = GetBestAntennaConfiguration (address, false, m_maxSnr);
          UpdateBestRxAntennaConfiguration (address, rxConfig, m_maxSnr);
          m_codebook->SetReceivingInDirectionalMode ();
          m_codebook->SetActiveRxSectorID (rxConfig.first, rxConfig.second);
        }
    }
  else
    {
      /** We are the Responder **/
      NS_LOG_INFO ("DMG STA Starting RSS Phase with Responder Role at " << Simulator::Now ());
      /* Process the data of the Initiator phase */
      if (m_isInitiatorTXSS)
        {
          /* Obtain antenna configuration for the highest received SNR to feed it back in SSW-FBCK Field */
          m_feedbackAntennaConfig = GetBestAntennaConfiguration (address, true, m_maxSnr);
        }
      /* Now start doing the specified sweeping in the Responder Phase */
      if (m_isResponderTXSS)
        {
          StartTransmitSectorSweep (address, BeamformingResponder);
        }
      else
        {
          /* The initiator is switching receive sectors at the same time */
          StartReceiveSectorSweep (address, BeamformingResponder);
        }
    }
}

void
DmgWifiMac::SwitchQuasiOmniPattern (Time switchTime)
{
  NS_LOG_FUNCTION (this << switchTime);
  if (m_codebook->SwitchToNextQuasiPattern ())
    {
      NS_LOG_INFO ("DMG STA Switching to the next quasi-omni pattern");
      Simulator::Schedule (switchTime, &DmgWifiMac::SwitchQuasiOmniPattern, this, switchTime);
    }
  else
    {
      NS_LOG_INFO ("DMG STA has concluded all the quasi-omni patterns");
    }
}

void
DmgWifiMac::RestartInitiatorSectorSweep (Mac48Address stationAddress)
{
  NS_LOG_FUNCTION (this << stationAddress);
  m_bfRetryTimes++;
  if (m_bfRetryTimes < dot11BFRetryLimit)
    {
      //if (GetRemainingAllocationTime () >)
      NS_LOG_DEBUG ("BF Retry Times=" << uint16_t (m_bfRetryTimes));
      if (m_currentAllocation == CBAP_ALLOCATION)
        {
          m_dmgSlsTxop->Sector_Sweep_Phase_Failed ();
        }
      else
        {
//          Simulator::Schedule (GetSifs (), &DmgWifiMac::Initiate_TXSS_TXOP, this, stationAddress, true);
        }
    }
  else
    {
      NS_LOG_DEBUG ("Beamforming Retry Times exceeded " << dot11BFRetryLimit);
      Reset_SLS_Initiator_Variables ();
      m_dmgSlsTxop->SLS_BFT_Failed ();
    }
}

void
DmgWifiMac::StartTransmitSectorSweep (Mac48Address address, BeamformingDirection direction)
{
  NS_LOG_FUNCTION (this << address << direction);
  NS_LOG_INFO ("DMG STA Starting TXSS at " << Simulator::Now ());
  /* Inform the codebook to Initiate SLS phase */
  m_codebook->StartSectorSweeping (address, TransmitSectorSweep, m_peerAntennas);
  /* Calculate the correct duration for the sector sweep frame */
  m_sectorSweepDuration = CalculateSectorSweepDuration (m_peerAntennas, m_codebook->GetTotalNumberOfAntennas (),
                                                        m_codebook->GetTotalNumberOfTransmitSectors ());
  if (direction == BeamformingInitiator)
    {
      SendInitiatorTransmitSectorSweepFrame (address);
    }
  else
    {
      SendRespodnerTransmitSectorSweepFrame (address);
    }
}

void
DmgWifiMac::SendInitiatorTransmitSectorSweepFrame (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_CTL_DMG_SSW);

  /* Other Fields */
  hdr.SetAddr1 (address);           // MAC address of the STA that is the intended receiver of the sector sweep.
  hdr.SetAddr2 (GetAddress ());     // MAC address of the transmitter STA of the sector sweep frame.
  hdr.SetNoMoreFragments ();
  hdr.SetNoRetry ();

  Ptr<Packet> packet = Create<Packet> ();
  CtrlDMG_SSW sswFrame;

  DMG_SSW_Field ssw;
  ssw.SetDirection (BeamformingInitiator);
  ssw.SetCountDown (m_codebook->GetRemaingSectorCount ());
  ssw.SetSectorID (m_codebook->GetActiveTxSectorID ());
  ssw.SetDMGAntennaID (m_codebook->GetActiveAntennaID ());

  DMG_SSW_FBCK_Field sswFeedback;
  sswFeedback.IsPartOfISS (true);
  sswFeedback.SetSector (m_codebook->GetTotalNumberOfTransmitSectors ());
  sswFeedback.SetDMGAntenna (m_codebook->GetTotalNumberOfAntennas ());
  sswFeedback.SetPollRequired (false);

  /* Set the fields in SSW Frame */
  sswFrame.SetSswField (ssw);
  sswFrame.SetSswFeedbackField (sswFeedback);
  packet->AddHeader (sswFrame);

  //// NINA ////
  BftIdTag tag;
  tag.Set (m_bftIdMap[address]);
  packet->AddPacketTag (tag);
  //// NINA ////

  NS_LOG_INFO ("Sending SSW Frame " << Simulator::Now () << " with AntennaID=" <<
               static_cast<uint16_t> (ssw.GetDMGAntennaID ()) << ", SectorID=" << static_cast<uint16_t> (ssw.GetSectorID ()));

  /* Transmit control frames directly without DCA + DCF Manager */
  TransmitControlFrame (packet, hdr, GetRemainingSectorSweepTime ());
}

void
DmgWifiMac::SendRespodnerTransmitSectorSweepFrame (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_CTL_DMG_SSW);

  /* Other Fields */
  hdr.SetAddr1 (address);           // MAC address of the STA that is the intended receiver of the sector sweep.
  hdr.SetAddr2 (GetAddress ());     // MAC address of the transmitter STA of the sector sweep frame.
  hdr.SetNoMoreFragments ();
  hdr.SetNoRetry ();

  Ptr<Packet> packet = Create<Packet> ();
  CtrlDMG_SSW sswFrame;

  DMG_SSW_Field ssw;
  ssw.SetDirection (BeamformingResponder);
  ssw.SetCountDown (m_codebook->GetRemaingSectorCount ());
  ssw.SetSectorID (m_codebook->GetActiveTxSectorID ());
  ssw.SetDMGAntennaID (m_codebook->GetActiveAntennaID ());

  DMG_SSW_FBCK_Field sswFeedback;
  sswFeedback.IsPartOfISS (false);
  sswFeedback.SetSector (m_feedbackAntennaConfig.second);
  sswFeedback.SetDMGAntenna (m_feedbackAntennaConfig.first);
  sswFeedback.SetPollRequired (false);
  sswFeedback.SetSNRReport (m_maxSnr);

  /* Set the fields in SSW Frame */
  sswFrame.SetSswField (ssw);
  sswFrame.SetSswFeedbackField (sswFeedback);
  packet->AddHeader (sswFrame);

  //// NINA ////
  BftIdTag tag;
  tag.Set (m_bftIdMap[address]);
  packet->AddPacketTag (tag);
  //// NINA ////

  NS_LOG_INFO ("Sending SSW Frame " << Simulator::Now () << " with AntennaID=" <<
               static_cast<uint16_t> (ssw.GetDMGAntennaID ()) << ", SectorID=" << static_cast<uint16_t> (ssw.GetSectorID ()));

  /* Transmit control frames directly without DCA + DCF Manager */
  TransmitControlFrame (packet, hdr, GetRemainingSectorSweepTime ());
}

void
DmgWifiMac::TransmitControlFrame (Ptr<const Packet> packet, WifiMacHeader &hdr, Time duration)
{
  NS_LOG_FUNCTION (this << packet << &hdr << duration);
  if ((m_accessPeriod == CHANNEL_ACCESS_DTI) && (m_currentAllocation == CBAP_ALLOCATION))
    {
      m_dmgSlsTxop->TransmitFrame (packet, hdr, duration);
    }
  else if ((m_accessPeriod == CHANNEL_ACCESS_ABFT) || (m_currentAllocation == SERVICE_PERIOD_ALLOCATION))
    {
      TransmitControlFrameImmediately (packet, hdr, duration);
    }
}

void
DmgWifiMac::TransmitControlFrameImmediately (Ptr<const Packet> packet, WifiMacHeader &hdr, Time duration)
{
  NS_LOG_FUNCTION (this << packet << &hdr << duration);
  /* Send Frame immediately without DCA + DCF Manager */
  MacLowTransmissionParameters params;
  params.EnableOverrideDurationId (duration);
  params.DisableRts ();
  params.DisableAck ();
  params.DisableNextData ();
  m_low->StartTransmission (Create<WifiMacQueueItem> (packet, hdr),  params,
                            MakeCallback (&DmgWifiMac::FrameTxOk, this));
}

void
DmgWifiMac::TransmitShortSswFrame (Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  /* Send Frame immediately without DCA + DCF Manager */
  MacLowTransmissionParameters params;
  params.DisableRts ();
  params.DisableAck ();
  params.DisableNextData ();
  m_low->StartShortSswTransmission (Create<WifiMacQueueItem> (packet),  params,
                            MakeCallback (&DmgWifiMac::FrameTxOkShortSsw, this));
}

void
DmgWifiMac::ReceiveShortSswFrame (Ptr<Packet> packet, double rxSnr)
{
  NS_LOG_FUNCTION (this << packet);

  ShortSSW shortSsw;
  packet->RemoveHeader (shortSsw);

  /* Check if we are a receiver for the short SSW frame */

  if (shortSsw.GetAddressingMode () == INDIVIDUAL_ADRESS && shortSsw.GetDestinationAID () == GetAssociationID ())
    {
      /* To do: Handle SLS using SSW frames */
    }
  /* If the Short SSW frame is a part of an Initiator TXSS for MU-MIMO BFT - check if the station is MU-MIMO capable
   and if an Edmg Group ID set element has been exchanged before */
  else if (GetDmgWifiPhy ()->IsMuMimoSupported () && (m_edmgGroupIdSetElement->GetNumberofEDMGGroups () != 0))
    {
      /* Check if we are a part of the MU group that this SSW is meant for */
      EDMGGroupTuples edmgGroupTuples = m_edmgGroupIdSetElement->GetEDMGGroupTuples ();
      bool isRecipient = false;
      for (auto const & edmgGroupTuple : edmgGroupTuples)
        if (edmgGroupTuple.groupID == shortSsw.GetDestinationAID ())
          {
            for (auto const & aid : edmgGroupTuple.aidList)
              {
                if (aid == GetAssociationID ())
                  {
                    isRecipient = true;
                    m_edmgMuGroup = edmgGroupTuple;
                    break;
                  }
              }
          }
      if (isRecipient)
        {
          if (m_muMimoBfPhase == MU_WAIT_MU_MIMO_BF_TRAINING)
            {
              /* We received the first short SSW from the initiator so we initialize variables. */
              /* Inform the low and high MAC that we are starting MU-MIMO BFT */
              m_muMimoBfPhase = MU_SISO_TXSS;
              m_low->MIMO_BFT_Phase_Started ();
              m_muMimoBeamformingTraining = true;
              /* Clear the maps that store feedback from old BFT results */
              m_muMimoSisoSnrMap.clear ();
              m_mimoSisoSnrList.clear ();
              uint8_t numberOfAntenans = 8;
              /* Save the BFT ID of the current BFT. */
              BftIdTag tag;
              packet->RemovePacketTag (tag);
              m_muMimoBftIdMap [m_edmgMuGroup.groupID] = tag.Get ();
              // If we have the capabilities of the intitiator get the number of Rx antennas to estimate the duration of the initiator txss - otherwise assume 8 antennas - the max.
              AID_MAP::iterator it = m_aidMap.find (shortSsw.GetSourceAID ());
              if (it != m_aidMap.end ())
                {
                  Ptr<DmgCapabilities> peerCapabilities = GetPeerStationDmgCapabilities (m_aidMap[shortSsw.GetSourceAID ()]);
                  if (peerCapabilities != 0)
                    {
                      numberOfAntenans = peerCapabilities->GetNumberOfRxDmgAntennas ();
                    }
                }
              Time initiatorTxssRemainderDuration = CalculateShortSectorSweepDuration (numberOfAntenans, shortSsw.GetCDOWN ());
              Time sisoFeebackDuration = shortSsw.GetSisoFbckDuration ();
              /* Schedule a timer for the end of the SISO phase - if we do not receive a BRP poll frame asking for feedback assume that MU-MIMO BFT failed */
              m_muMimoFbckTimeout = Simulator::Schedule (initiatorTxssRemainderDuration + sisoFeebackDuration,
                                                             &DmgWifiMac::MuMimoBFTFailed, this);
            }
          /* Save the SNR measured during the reception of the Short SSW frame */
          MIMO_CONFIGURATION config = std::make_tuple(shortSsw.GetCDOWN (), m_codebook->GetActiveAntennaID (), shortSsw.GetRFChainID ());
          NS_LOG_DEBUG ("Short SSW config: " << +shortSsw.GetCDOWN () << " " << +m_codebook->GetActiveAntennaID () << " " << +shortSsw.GetRFChainID () );
          m_muMimoSisoSnrMap[config] = rxSnr;
          m_mimoSisoSnrList.push_back (rxSnr);
        }
    }
}

void
DmgWifiMac::StartReceiveSectorSweep (Mac48Address address, BeamformingDirection direction)
{
  NS_LOG_FUNCTION (this << address << direction);
  NS_LOG_INFO ("DMG STA Starting RXSS with " << address);

  /* A RXSS may be requested only when an initiator/respodner is aware of the capabilities of a responder/initiator, which
   * includes the RXSS Length field. */
  Ptr<DmgCapabilities> peerCapabilities = GetPeerStationDmgCapabilities (address);
  if (peerCapabilities == 0)
    {
      NS_LOG_LOGIC ("Cannot start RXSS since the DMG Capabilities of the peer station is not available");
      return;
    }

  uint8_t RXSSLength = peerCapabilities->GetRXSSLength ();
  if (direction == BeamformingInitiator)
    {
      /* During the initiator RXSS, the initiator shall transmit from each of the initiator’s DMG antennas the number
       * of BF frames indicated by the responder in the last negotiated RXSS Length field transmitted by the responder.
       * Each transmitted BF frame shall be transmitted with the same fixed antenna sector or pattern. The
       * initiator shall set the Sector ID and DMG Antenna ID fields in each transmitted BF frame to a value that
       * uniquely identifies the single sector through which the BF frame is transmitted. */
      m_totalSectors = (m_codebook->GetTotalNumberOfAntennas () * RXSSLength) - 1;
    }
  else
    {
      /* During the responder RXSS, the responder shall transmit the number of SSW frames indicated by the
       * initiator in the initiator’s most recently transmitted RXSS Length field (non-A-BFT) or FSS field (A-BFT)
       * from each of the responder’s DMG antennas, each time with the same antenna sector or pattern fixed for all
       * SSW frames transmission originating from the same DMG antenna. */
      if (m_accessPeriod == CHANNEL_ACCESS_ABFT)
        {
          m_totalSectors = std::min (static_cast<uint> (RXSSLength - 1), static_cast<uint> (m_ssFramesPerSlot - 1));
        }
      else
        {
          m_totalSectors = (m_codebook->GetTotalNumberOfAntennas () * RXSSLength) - 1;
        }
    }

  STATION_ANTENNA_CONFIG_MAP::iterator it = m_bestAntennaConfig.find (address);
  if (it != m_bestAntennaConfig.end ())
    {
      /* Change Tx Antenna Configuration */
      ANTENNA_CONFIGURATION_TX antennaConfigTx = std::get<0> (m_bestAntennaConfig[address]);
      m_codebook->SetActiveTxSectorID (antennaConfigTx.first, antennaConfigTx.second);
    }
  else
    {
      NS_LOG_DEBUG ("Cannot start RXSS since no antenna configuration available for DMG STA=" << address);
      return;
    }

  SendReceiveSectorSweepFrame (address, m_totalSectors, direction);
}

void
DmgWifiMac::SendReceiveSectorSweepFrame (Mac48Address address, uint16_t count, BeamformingDirection direction)
{
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_CTL_DMG_SSW);

  /* Header Duration*/
  hdr.SetDuration (GetRemainingAllocationTime ());

  /* Other Fields */
  hdr.SetAddr1 (address);           // MAC address of the STA that is the intended receiver of the sector sweep.
  hdr.SetAddr2 (GetAddress ());     // MAC address of the transmitter STA of the sector sweep frame.
  hdr.SetNoMoreFragments ();
  hdr.SetNoRetry ();

  Ptr<Packet> packet = Create<Packet> ();
  CtrlDMG_SSW sswFrame;

  DMG_SSW_Field ssw;
  ssw.SetDirection (direction);
  ssw.SetCountDown (count);
  ssw.SetSectorID (m_codebook->GetActiveTxSectorID ());
  ssw.SetDMGAntennaID (m_codebook->GetActiveAntennaID ());

  DMG_SSW_FBCK_Field sswFeedback;
  sswFeedback.IsPartOfISS (true);
  sswFeedback.SetSector (m_codebook->GetRemaingSectorCount ());
  sswFeedback.SetDMGAntenna (m_codebook->GetTotalNumberOfAntennas ());
  sswFeedback.SetPollRequired (false);

  /* Set the fields in SSW Frame */
  sswFrame.SetSswField (ssw);
  sswFrame.SetSswFeedbackField (sswFeedback);
  packet->AddHeader (sswFrame);

  //// NINA ////
  BftIdTag tag;
  tag.Set (m_bftIdMap[address]);
  packet->AddPacketTag (tag);
  //// NINA ////

  NS_LOG_INFO ("Sending SSW Frame " << Simulator::Now ()
               << " with AntennaID=" << static_cast<uint16_t> (m_codebook->GetActiveAntennaID ())
               << ", SectorID=" << static_cast<uint16_t> (m_codebook->GetActiveTxSectorID ()));

  /* Transmit control frames directly without DCA + DCF Manager */
  TransmitControlFrameImmediately (packet, hdr, GetRemainingSectorSweepTime ());
}

void
DmgWifiMac::SendSswFbckFrame (Mac48Address receiver, Time duration)
{
  NS_LOG_FUNCTION (this << receiver << duration);
  if (m_channelAccessManager->CanAccess ())
    {
      WifiMacHeader hdr;
      hdr.SetType (WIFI_MAC_CTL_DMG_SSW_FBCK);
      hdr.SetAddr1 (receiver);        // Receiver.
      hdr.SetAddr2 (GetAddress ());   // Transmiter.

      Ptr<Packet> packet = Create<Packet> ();
      packet->AddHeader (hdr);

      CtrlDMG_SSW_FBCK fbck;          // SSW-FBCK Frame.
      DMG_SSW_FBCK_Field feedback;    // SSW-FBCK Field.

      if (m_isResponderTXSS)
        {
          /* Responder is TXSS so obtain antenna configuration for the highest received SNR to feed it back */
          m_feedbackAntennaConfig = GetBestAntennaConfiguration (receiver, true, m_maxSnr);
          feedback.IsPartOfISS (false);
          feedback.SetSector (m_feedbackAntennaConfig.second);
          feedback.SetDMGAntenna (m_feedbackAntennaConfig.first);
          feedback.SetSNRReport (m_maxSnr);
        }
      else
        {
          /* At the start of an SSW ACK, the initiator should have its receive antenna array configured to a quasi-omni
           * antenna pattern using the DMG antenna through which it received with the highest quality during the RSS,
           * or the best receive sector if an RXSS has been performed during the RSS, and should not change its receive
           * antenna configuration while it attempts to receive from the responder until the expected end of the SSW ACK. */
          ANTENNA_CONFIGURATION_RX rxConfig = GetBestAntennaConfiguration (receiver, false, m_maxSnr);
          UpdateBestRxAntennaConfiguration (receiver, rxConfig, m_maxSnr);
          m_codebook->SetReceivingInDirectionalMode ();
          m_codebook->SetActiveRxSectorID (rxConfig.first, rxConfig.second);
        }

      BRP_Request_Field request;
      /* Currently, we do not support MID + BC Subphases */
      request.SetMID_REQ (false);
      request.SetBC_REQ (false);

      BF_Link_Maintenance_Field maintenance;
      maintenance.SetUnitIndex (m_beamlinkMaintenanceUnit);
      maintenance.SetMaintenanceValue (m_beamlinkMaintenanceValue);
      maintenance.SetAsMaster (true); /* Master of data transfer */

      fbck.SetSswFeedbackField (feedback);
      fbck.SetBrpRequestField (request);
      fbck.SetBfLinkMaintenanceField (maintenance);

      packet->AddHeader (fbck);

      /* Reset Feedback Flag */
      m_sectorFeedbackSchedulled = false;
      NS_LOG_INFO ("Sending SSW-FBCK Frame to " << receiver << " at " << Simulator::Now ());

      /* The SSW-Feedback frame shall be transmitted through the sector identified by the value of the Sector
       * Select field and DMG Antenna Select field received from the responder during the preceding responder TXSS. */
      SteerTxAntennaToward (receiver);

      /* Transmit control frames directly without the Channel Access Manager */
      TransmitControlFrame (packet, hdr, duration);
    }
  else
    {
      NS_LOG_INFO ("Medium is busy, Abort Sending SSW-FBCK");
    }
}

void
DmgWifiMac::ResendSswFbckFrame (void)
{
  NS_LOG_FUNCTION (this);
  /* The initiator may restart the SSW Feedback up to dot11BFRetryLimit times if it does not receive an SSW-ACK frame
   * from the responder in MBIFS time following the completion of the SSW Feedback. The initiator shall restart the
   * SSW Feedback PIFS time following the expected end of the SSW ACK by the responder, provided there is sufficient
   * time left in the allocation for the initiator to begin the SSW Feedback followed by an SSW ACK from the responder
   * in SIFS time. If there is not sufficient time left in the allocation for the completion of the SSW Feedback and
   * SSW ACK, the initiator shall restart the SSW Feedback at the start of the following allocation between the initiator
   * and the responder.*/
  m_bfRetryTimes++;
  if (m_bfRetryTimes < dot11BFRetryLimit)
    {
      Time sswFbckDuration;
      if (m_currentAllocation == CBAP_ALLOCATION)
        {
          if (m_isEdmgSupported)
            {
              sswFbckDuration = edmgSswAckTxTime + GetMbifs ();
            }
          else
            {
              sswFbckDuration = sswAckTxTime + GetMbifs ();
            }
        }
      else
        {
          sswFbckDuration = GetRemainingAllocationTime ();
        }
      Simulator::Schedule (GetPifs (), &DmgSlsTxop::RxSSW_ACK_Failed, m_dmgSlsTxop);
    }
  else
    {
      NS_LOG_DEBUG ("Beamforming Retry Times exceeded " << dot11BFRetryLimit);
      Reset_SLS_Initiator_Variables ();
      m_dmgSlsTxop->SLS_BFT_Failed ();
    }
}

void
DmgWifiMac::SendSswAckFrame (Mac48Address receiver, Time sswFbckDuration)
{
  NS_LOG_FUNCTION (this << receiver << sswFbckDuration);
  /* send a SSW Feedback when you receive a SSW Slot after MBIFS. */
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_CTL_DMG_SSW_ACK);
  hdr.SetAddr1 (receiver);        // Receiver.
  hdr.SetAddr2 (GetAddress ());   // Transmiter.
  /* The Duration field is set until the end of the current allocation */
  Time sswAck;
  if (m_isEdmgSupported)
    {
      sswAck = edmgSswAckTxTime;
    }
  else
    {
      sswAck = sswAckTxTime;
    }
  Time duration = sswFbckDuration - (GetMbifs () + sswAck);
  NS_ASSERT (duration.IsPositive ());

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (hdr);

  CtrlDMG_SSW_FBCK ackFrame;      // SSW-ACK Frame.
  DMG_SSW_FBCK_Field feedback;    // SSW-FBCK Field.

  if (m_isInitiatorTXSS)
    {
      /* Initiator is TXSS so obtain antenna configuration for the highest received SNR to feed it back */
      m_feedbackAntennaConfig = GetBestAntennaConfiguration (receiver, true, m_maxSnr);
      feedback.IsPartOfISS (false);
      feedback.SetSector (m_feedbackAntennaConfig.second);
      feedback.SetDMGAntenna (m_feedbackAntennaConfig.first);
      feedback.SetSNRReport (m_maxSnr);
    }

  BRP_Request_Field request;
  request.SetMID_REQ (false);
  request.SetBC_REQ (false);

  BF_Link_Maintenance_Field maintenance;
  maintenance.SetUnitIndex (m_beamlinkMaintenanceUnit);
  maintenance.SetMaintenanceValue (m_beamlinkMaintenanceValue);
  maintenance.SetAsMaster (false); /* Slave of data transfer */

  ackFrame.SetSswFeedbackField (feedback);
  ackFrame.SetBrpRequestField (request);
  ackFrame.SetBfLinkMaintenanceField (maintenance);

  packet->AddHeader (ackFrame);
  NS_LOG_INFO ("Sending SSW-ACK Frame to " << receiver << " at " << Simulator::Now ());

  /* Set the best sector for transmission */
  SteerAntennaToward (receiver);

  /* Transmit control frames directly without DCA + DCF Manager */
  TransmitControlFrame (packet, hdr, duration);
}

void
DmgWifiMac::PrintSnrConfiguration (SNR_MAP &snrMap)
{
  if (snrMap.begin () == snrMap.end ())
    {
      std::cout << "No SNR Information Availalbe" << std::endl;
    }
  else
    {
      for (SNR_MAP::iterator it = snrMap.begin (); it != snrMap.end (); it++)
        {
          ANTENNA_CONFIGURATION_COMBINATION config = it->first;
          printf ("My AntennaID: %d, Peer AntennaID: %d, Peer SectorID: %2d, SNR: %+2.2f dB\n",
                  std::get<0> (config), std::get<1> (config), std::get<2> (config), RatioToDb (it->second));
        }
    }
}

void
DmgWifiMac::PrintSnrTable (void)
{
  std::cout << "****************************************************************" << std::endl;
  std::cout << " SNR Dump for Sector Level Sweep for Station: " << GetAddress () << std::endl;
  std::cout << "****************************************************************" << std::endl;
  for (STATION_SNR_PAIR_MAP_CI it = m_stationSnrMap.begin (); it != m_stationSnrMap.end (); it++)
    {
      SNR_PAIR snrPair = it->second;
      std::cout << "Peer DMG STA: " << it->first << std::endl;
      std::cout << "***********************************************" << std::endl;
      std::cout << "Transmit Sector Sweep (TXSS) SNRs: " << std::endl;
      std::cout << "***********************************************" << std::endl;
      PrintSnrConfiguration (snrPair.first);
      std::cout << "***********************************************" << std::endl;
      std::cout << "Receive Sector Sweep (RXSS) SNRs: " << std::endl;
      std::cout << "***********************************************" << std::endl;
      PrintSnrConfiguration (snrPair.second);
      std::cout << "***********************************************" << std::endl;
    }
}

void
DmgWifiMac::PrintBeamRefinementMeasurements (void)
{
  std::cout << "*********************************************************" << std::endl;
  std::cout << " Beam Refinement SNR Dump for Station: " << GetAddress () << std::endl;
  std::cout << "*********************************************************" << std::endl;
  for (TRN2SNR_MAP_CI i = m_trn2snrMap.begin (); i != m_trn2snrMap.end (); i++)
    {
      std::cout << "Peer DMG STA: " << i->first << std::endl;
      std::cout << "***********************************************" << std::endl;
      for (uint16_t j = 0; j < i->second.size (); j++)
        {
          printf ("AWV[%2d]: %+2.2f dB\n", j, RatioToDb (i->second[j]));
        }
    }
}

void
DmgWifiMac::PrintGroupBeamformingMeasurements (void)
{
  std::cout << "*********************************************************" << std::endl;
  std::cout << " Group Beamforming SNR Measurements for Station: " << GetAddress () << std::endl;
  std::cout << "*********************************************************" << std::endl;
  for (STATION_SNR_AWV_MAP_I i = m_apSnrAwvMap.begin (); i != m_apSnrAwvMap.end (); i++)
    {
      SNR_AWV_MAP snrMap = i->second;
      std::cout << "Peer DMG AP: " << i->first << std::endl;
      std::cout << "***********************************************" << std::endl;
      for (SNR_AWV_MAP::iterator i = snrMap.begin (); i != snrMap.end (); i++)
        {
          AWV_CONFIGURATION_TX_RX config = i->first;
          printf ("Tx AntennaID: %d, Tx SectorID: %2d, Rx AntennaID: %d, Rx SectorID: %2d, Rx AwvID: %2d, SNR: %+2.2f dB\n",
                  config.first.first.first, config.first.first.second,
                  config.second.first.first, config.second.first.second, config.second.second,  RatioToDb (i->second));
        }
    }
}

void
DmgWifiMac::MapTxSnr (Mac48Address address, AntennaID RxAntennaID, AntennaID TxAntennaID, SectorID sectorID, double snr)
{
  NS_LOG_FUNCTION (this << address << uint16_t (RxAntennaID)<< uint16_t (TxAntennaID) << uint16_t (sectorID) << RatioToDb (snr));
  STATION_SNR_PAIR_MAP_I it = m_stationSnrMap.find (address);
  ANTENNA_CONFIGURATION_COMBINATION config = std::make_tuple (RxAntennaID, TxAntennaID, sectorID);
  if (it != m_stationSnrMap.end ())
    {
      SNR_PAIR *snrPair = (SNR_PAIR *) (&it->second);
      SNR_MAP_TX *snrMap = (SNR_MAP_TX *) (&snrPair->first);
      (*snrMap)[config] = snr;
    }
  else
    {
      SNR_MAP_TX snrTx;
      SNR_MAP_RX snrRx;
      snrTx[config] = snr;
      SNR_PAIR snrPair = std::make_pair (snrTx, snrRx);
      m_stationSnrMap[address] = snrPair;
    }
}

void
DmgWifiMac::MapTxSnr (Mac48Address address, AntennaID antennaID, SectorID sectorID, double snr)
{
  MapTxSnr (address, m_codebook->GetActiveAntennaID (), antennaID, sectorID, snr);
}

void
DmgWifiMac::MapRxSnr (Mac48Address address, AntennaID antennaID, SectorID sectorID, double snr)
{
  NS_LOG_FUNCTION (this << address << uint16_t (antennaID) << uint16_t (sectorID) << snr);
  STATION_SNR_PAIR_MAP::iterator it = m_stationSnrMap.find (address);
  ANTENNA_CONFIGURATION_COMBINATION config = std::make_tuple (m_codebook->GetActiveAntennaID (), antennaID, sectorID);
  if (it != m_stationSnrMap.end ())
    {
      SNR_PAIR *snrPair = (SNR_PAIR *) (&it->second);
      SNR_MAP_RX *snrMap = (SNR_MAP_RX *) (&snrPair->second);
      (*snrMap)[config] = snr;
    }
  else
    {
      SNR_MAP_TX snrTx;
      SNR_MAP_RX snrRx;
      snrRx[config] = snr;
      SNR_PAIR snrPair = std::make_pair (snrTx, snrRx);
      m_stationSnrMap[address] = snrPair;
    }
}

/* Information Request and Response Exchange */

void
DmgWifiMac::SendInformationRequest (Mac48Address to, ExtInformationRequest &requestHdr)
{
  NS_LOG_FUNCTION (this << to);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.dmgAction = WifiActionHeader::DMG_INFORMATION_REQUEST;
  actionHdr.SetAction (WifiActionHeader::DMG, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (requestHdr);
  packet->AddHeader (actionHdr);

  m_txop->Queue (packet, hdr);
}

void
DmgWifiMac::SendInformationResponse (Mac48Address to, ExtInformationResponse &responseHdr)
{
  NS_LOG_FUNCTION (this << to);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.dmgAction = WifiActionHeader::DMG_INFORMATION_RESPONSE;
  actionHdr.SetAction (WifiActionHeader::DMG, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (responseHdr);
  packet->AddHeader (actionHdr);

  //// NINA ////
  /* In case we're sending an unsolicited Information Response as part of Group BFT, send a tag with the BFT ID */
  BFT_ID_MAP::iterator it = m_bftIdMap.find (to);
  if (it != m_bftIdMap.end ())
    {
      BftIdTag tag;
      tag.Set (m_bftIdMap[to]);
      packet->AddPacketTag (tag);
    }
  //// NINA ////
  m_txop->Queue (packet, hdr);
}

void
DmgWifiMac::SendUnsolicitedTrainingResponse (Mac48Address receiver)
{
  NS_LOG_FUNCTION (this << receiver);

  uint16_t numberOfMeasurments = 0;
  SNR_MAP_TX snrMap;
  STATION_SNR_PAIR_MAP::iterator it = m_stationSnrMap.find (receiver);
  if (it != m_stationSnrMap.end ())
    {
      SNR_PAIR snrPair =  it->second;
      snrMap =  snrPair.first;
      numberOfMeasurments = snrMap.size();
    }

  ExtInformationResponse responseHdr;
  responseHdr.SetSubjectAddress (receiver);
  Ptr<RequestElement> requestElement = Create<RequestElement> ();
  responseHdr.SetRequestInformationElement (requestElement);
  /* The Information Response frame shall carry DMGCapabilities Element for the transmitter STA */
  responseHdr.AddDmgCapabilitiesElement (GetDmgCapabilities ());
  /* The Information Response frame shall carry EDMGCapabilities Element for the transmitter STA */
  //responseHdr.AddEdmgCapabilitiesElement (GetEdmgCapabilities ());

  /* Add a beam refinement element  */
  Ptr<BeamRefinementElement> beamElement = Create<BeamRefinementElement> ();
  beamElement->SetAsBeamRefinementInitiator (false);
  beamElement->SetTxTrainResponse (false);
  beamElement->SetRxTrainResponse (false);
  beamElement->SetTxTrnOk (false);
  beamElement->SetSnrPresent (true);
  beamElement->SetChannelMeasurementPresent (false);
  beamElement->SetExtendedNumberOfMeasurements (numberOfMeasurments);
  beamElement->SetSectorIdOrderPresent (true);
  beamElement->SetCapabilityRequest (false);
  beamElement->SetEdmgExtensionFlag (true);
  beamElement->SetEdmgChannelMeasurementPresent (true);
  beamElement->SetSswFrameType (DMG_BEACON_FRAME);

  responseHdr.SetBeamRefinementElement (beamElement);

  /* Add a Channel Measurement Feedback Element */
  Ptr<ChannelMeasurementFeedbackElement> channelElement = Create<ChannelMeasurementFeedbackElement> ();
  /* Add an EDMG Channel Measurement Feedback Element */
  Ptr<EDMGChannelMeasurementFeedbackElement> edmgChannelElement = Create<EDMGChannelMeasurementFeedbackElement> ();
  for (SNR_MAP::iterator it = snrMap.begin (); it != snrMap.end (); it++)
    {
      ANTENNA_CONFIGURATION_COMBINATION config = it->first;
      uint8_t snr = MapSnrToInt (it->second);
      channelElement->AddSnrItem (snr);
      EDMGSectorIDOrder order;
      order.RXAntennaID = std::get<0> (config);
      order.TXAntennaID = std::get<1> (config);
      order.SectorID = std::get<2> (config);
      edmgChannelElement->Add_EDMG_SectorIDOrder (order);
    }

  responseHdr.SetChannelMeasurementElement (channelElement);
  responseHdr.SetEdmgChannelMeasurementElement (edmgChannelElement);

  if (m_informationUpdateEvent.IsRunning ())
    {
      m_informationUpdateEvent.Cancel ();
    }
  m_informationUpdateEvent = Simulator::Schedule (m_informationUpdateTimeout,
                                             &DmgWifiMac::SendUnsolicitedTrainingResponse, this, receiver);
  SendInformationResponse (receiver, responseHdr);
}

uint8_t
DmgWifiMac::MapSnrToInt (double snr)
{
  double dB = RatioToDb (snr);
  if (dB <= -8)
    {
      return 0;
    }
  else if (dB >= 55.75)
    {
      return 255;
    }
  else
    {
      double x = (8 + dB)/0.25;
      return std::ceil(x);
    }
}

double
DmgWifiMac::MapIntToSnr (uint8_t snr)
{
  if (snr == 0)
    return DbToRatio (-8);
  else if (snr == 255)
    return DbToRatio (55.75);
  else
    {
      double dB = snr * 0.25 - 8;
      return DbToRatio (dB);
    }
}

void
DmgWifiMac::SteerTxAntennaToward (Mac48Address address, bool isData)
{
  NS_LOG_FUNCTION (this << address);
  DATA_COMMUNICATION_MODE dataMode = GetStationDataCommunicationMode (address);
  if (dataMode != DATA_MODE_SISO && isData)
    SteerMimoTxAntennaToward (address);
  else
    {
      SteerSisoTxAntennaToward (address);
    }

}

void
DmgWifiMac::SteerAntennaToward (Mac48Address address, bool isData)
{
  NS_LOG_FUNCTION (this << address);
  DATA_COMMUNICATION_MODE dataMode = GetStationDataCommunicationMode (address);
  if (dataMode != DATA_MODE_SISO && isData)
    SteerMimoAntennaToward (address);
  else
    {
      SteerSisoAntennaToward (address);
    }
}

void
DmgWifiMac::SteerSisoTxAntennaToward (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_codebook->SetCommunicationMode (SISO_MODE);
  STATION_ANTENNA_CONFIG_MAP::iterator it = m_bestAntennaConfig.find (address);
  if (it != m_bestAntennaConfig.end ())
    {
      ANTENNA_CONFIGURATION_TX antennaConfigTx = std::get<0> (m_bestAntennaConfig[address]);
      /* Change Tx Antenna Configuration */
      NS_LOG_DEBUG ("Change Transmit Antenna Sector Config to AntennaID=" << static_cast<uint16_t> (antennaConfigTx.first)
                    << ", SectorID=" << static_cast<uint16_t> (antennaConfigTx.second));
      m_codebook->SetActiveTxSectorID (antennaConfigTx.first, antennaConfigTx.second);
      /* Check if there is a AWV TX configuration saved for the STA - if there is set the TX AWV ID*/
      STATION_AWV_MAP::iterator it = m_bestAwvConfig.find (address);
      if (it != m_bestAwvConfig.end ())
        {
          BEST_AWV_ID *antennaConfig = &(it->second);
          if (antennaConfig->first != NO_AWV_ID)
            {
              m_codebook->SetActiveTxAwvID (antennaConfig->first);
            }
        }

    }
  else
    {
      NS_LOG_DEBUG ("No antenna configuration available for DMG STA=" << address);
    }
}

void
DmgWifiMac::SteerSisoAntennaToward (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_codebook->SetCommunicationMode (SISO_MODE);
  STATION_ANTENNA_CONFIG_MAP::iterator it = m_bestAntennaConfig.find (address);
  if (it != m_bestAntennaConfig.end ())
    {
      ANTENNA_CONFIGURATION_TX antennaConfigTx = std::get<0> (m_bestAntennaConfig[address]);
      ANTENNA_CONFIGURATION_RX antennaConfigRx = std::get<1> (m_bestAntennaConfig[address]);

      /* Change Tx Antenna Configuration */
      NS_LOG_DEBUG ("Change Transmit Antenna Config to AntennaID=" << static_cast<uint16_t> (antennaConfigTx.first)
                    << ", SectorID=" << static_cast<uint16_t> (antennaConfigTx.second));

      m_codebook->SetActiveTxSectorID (antennaConfigTx.first, antennaConfigTx.second);
      /* Check if there is a AWV TX configuration saved for the STA - if there is set the TX AWV ID*/
      STATION_AWV_MAP::iterator it = m_bestAwvConfig.find (address);
      if (it != m_bestAwvConfig.end ())
        {
          BEST_AWV_ID *antennaConfig = &(it->second);
          if (antennaConfig->first != NO_AWV_ID)
            {
              m_codebook->SetActiveTxAwvID (antennaConfig->first);
            }
        }
      /* Change Rx Antenna Configuration */
      if ((antennaConfigRx.first != NO_ANTENNA_CONFIG) && (antennaConfigRx.second != NO_ANTENNA_CONFIG) && m_useRxSectors)
        {
          NS_LOG_DEBUG ("Change Receive Antenna Config to AntennaID=" << static_cast<uint16_t> (antennaConfigRx.first)
                        << ", SectorID=" << static_cast<uint16_t> (antennaConfigRx.second));
          m_codebook->SetReceivingInDirectionalMode ();
          m_codebook->SetActiveRxSectorID (antennaConfigRx.first, antennaConfigRx.second);
          /* Check if there is a AWV RX configuration saved for the STA - if there is set the RX AWV ID*/
          if (it != m_bestAwvConfig.end ())
            {
              BEST_AWV_ID *antennaConfig = &(it->second);
              if (antennaConfig->second != NO_AWV_ID)
                {
                  m_codebook->SetActiveRxAwvID (antennaConfig->second);
                }
            }
        }
      else
        {
          m_codebook->SetReceivingInQuasiOmniMode ();
        }
    }
  else
    {
      NS_LOG_DEBUG ("No Tx/Rx antenna configuration available for DMG STA=" << address);
      m_codebook->SetReceivingInQuasiOmniMode ();
    }
}

void
DmgWifiMac::SteerMimoTxAntennaToward (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  DATA_COMMUNICATION_MODE dataMode = GetStationDataCommunicationMode (address);
  STATION_MIMO_ANTENNA_CONFIG_INDEX_MAP::iterator it = m_bestMimoAntennaConfig.find (address);
  NS_ABORT_MSG_IF (it == m_bestMimoAntennaConfig.end (), "The station should already have the optimal Tx antenna config for MIMO communication");
  uint8_t txIndex = std::get<0> (m_bestMimoAntennaConfig[address]);
  MIMO_AWV_CONFIGURATION txConfigCombination;
  if (txIndex != NO_ANTENNA_CONFIG)
    {
      if (dataMode == DATA_MODE_SU_MIMO)
        {
          NS_LOG_DEBUG ("Setting up Tx config for SU-MIMO communication");
          txConfigCombination = m_suMimoTxCombinations[address].at (txIndex);
        }
      else
        {
          NS_LOG_DEBUG ("Setting up Tx config for MU-MIMO communication");
        }
      m_codebook->SetCommunicationMode (MIMO_MODE);
      for (auto const &txConfig : txConfigCombination)
        {
          NS_LOG_DEBUG ("Activate Transmit Antenna with AntennaID=" << static_cast<uint16_t> (txConfig.first.first)
                            << ", to SectorID=" << static_cast<uint16_t> (txConfig.first.second)
                            << ", AwvID=" << static_cast<uint16_t> (txConfig.second));
          m_codebook->SetActiveTxSectorID (txConfig.first.first, txConfig.first.second);
          if (txConfig.second != NO_AWV_ID)
            {
              m_codebook->SetActiveTxAwvID (txConfig.second);
            }
        }
    }
  else
    {
      NS_LOG_DEBUG("The station should not be transmitting in MIMO configuration");
      SteerSisoTxAntennaToward (address);
    }
}

void
DmgWifiMac::SteerMimoAntennaToward (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  SteerMimoTxAntennaToward (address);
  SteerMimoRxAntennaToward (address);
}

void
DmgWifiMac::SteerMimoRxAntennaToward (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  DATA_COMMUNICATION_MODE dataMode = GetStationDataCommunicationMode (address);
  NS_ABORT_MSG_IF (dataMode == DATA_MODE_SISO, "Data mode for this station should be MIMO");
  STATION_MIMO_ANTENNA_CONFIG_INDEX_MAP::iterator it = m_bestMimoAntennaConfig.find (address);
  NS_ABORT_MSG_IF (it == m_bestMimoAntennaConfig.end (), "The station should already have the optimal Rx antenna config for MIMO communication");
  uint8_t rxIndex = std::get<1> (m_bestMimoAntennaConfig[address]);
  MIMO_AWV_CONFIGURATION rxConfigCombination;
  if (rxIndex != NO_ANTENNA_CONFIG)
    {
      if (dataMode == DATA_MODE_SU_MIMO)
        {
          NS_LOG_DEBUG ("Setting up Rx config for SU-MIMO communication");
          rxConfigCombination = m_suMimoRxCombinations[address].at (rxIndex);
        }
      else
        {
          NS_LOG_DEBUG ("Setting up Rx config for MU-MIMO communication");
        }
      m_codebook->SetCommunicationMode (MIMO_MODE);
      for (auto const &rxConfig : rxConfigCombination)
        {
          NS_LOG_DEBUG ("Activate Receive Antenna with AntennaID=" << static_cast<uint16_t> (rxConfig.first.first)
                            << ", to SectorID=" << static_cast<uint16_t> (rxConfig.first.second)
                            << ", AwvID=" << static_cast<uint16_t> (rxConfig.second));
          m_codebook->SetActiveRxSectorID (rxConfig.first.first, rxConfig.first.second);
          if (rxConfig.second != NO_AWV_ID)
            {
              m_codebook->SetActiveRxAwvID (rxConfig.second);
            }
        }
    }
  else
    {
      NS_LOG_DEBUG("The station should not be receiving in MIMO configuration");
      m_codebook->SetReceivingInQuasiOmniMode ();
    }
}

RelayCapabilitiesInfo
DmgWifiMac::GetRelayCapabilitiesInfo (void) const
{
  RelayCapabilitiesInfo info;
  info.SetRelaySupportability (m_rdsActivated);
  info.SetRelayUsability (m_redsActivated);
  info.SetRelayPermission (true);     /* Used by PCP/AP only */
  info.SetAcPower (true);
  info.SetRelayPreference (true);
  info.SetDuplex (m_relayDuplexMode);
  info.SetCooperation (false);        /* Only Link Switching Type supported */
  return info;
}

Ptr<RelayCapabilitiesElement>
DmgWifiMac::GetRelayCapabilitiesElement (void) const
{
  Ptr<RelayCapabilitiesElement> relayElement = Create<RelayCapabilitiesElement> ();
  RelayCapabilitiesInfo info = GetRelayCapabilitiesInfo ();
  relayElement->SetRelayCapabilitiesInfo (info);
  return relayElement;
}

void
DmgWifiMac::SendRelaySearchResponse (Mac48Address to, uint8_t token)
{
  NS_LOG_FUNCTION (this << to << token);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  ExtRelaySearchResponseHeader responseHdr;
  responseHdr.SetDialogToken (token);
  responseHdr.SetStatusCode (0);
  responseHdr.SetRelayCapableList (m_rdsList);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.dmgAction = WifiActionHeader::DMG_RELAY_SEARCH_RESPONSE;
  actionHdr.SetAction (WifiActionHeader::DMG, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (responseHdr);
  packet->AddHeader (actionHdr);

  m_txop->Queue (packet, hdr);
}

/**
 * Functions for Beam Refinement Protocol Setup and Transaction Execution.
 */
void
DmgWifiMac::SendEmptyBrpFrame (Mac48Address receiver, BRP_Request_Field &requestField, BeamRefinementElement &element)
{
  NS_LOG_FUNCTION (this << receiver);
  SendBrpFrame (receiver, requestField, element, false, TRN_T, 0);
}

void
DmgWifiMac::SendBrpFrame (Mac48Address receiver, BRP_Request_Field &requestField, BeamRefinementElement &element,
                          bool requestBeamRefinement, PacketType packetType, uint8_t trainingFieldLength)
{
  NS_LOG_FUNCTION (this << receiver);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION_NO_ACK);
  hdr.SetAddr1 (receiver);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();
  hdr.SetFragmentNumber (0);
  hdr.SetNoMoreFragments ();
  hdr.SetNoRetry ();

  /* Special Fields for Beam Refinement */
  if (requestBeamRefinement)
    {
      hdr.RequestBeamRefinement ();
      hdr.SetPacketType (packetType);
      hdr.SetTrainngFieldLength (trainingFieldLength);
    }

  ExtBrpFrame brpFrame;
  brpFrame.SetDialogToken (0);
  brpFrame.SetBrpRequestField (requestField);
  brpFrame.SetBeamRefinementElement (element);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.unprotectedAction = WifiActionHeader::UNPROTECTED_DMG_BRP;
  actionHdr.SetAction (WifiActionHeader::UNPROTECTED_DMG, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (brpFrame);
  packet->AddHeader (actionHdr);

  /* Set the best sector for tansmission with this station */
  ANTENNA_CONFIGURATION_TX antennaConfigTx = std::get<0> (m_bestAntennaConfig[receiver]);
  m_codebook->SetActiveTxSectorID (antennaConfigTx.first, antennaConfigTx.second);

  NS_LOG_INFO ("Sending BRP Frame to " << receiver << " at " << Simulator::Now ()
               << " with AntennaID=" << static_cast<uint16_t> (antennaConfigTx.first)
               << " SectorID=" << static_cast<uint16_t> (antennaConfigTx.second));

  if (m_accessPeriod == CHANNEL_ACCESS_ATI)
    {
      m_dmgAtiTxop->Queue (packet, hdr);
    }
  else
    {
      /* Transmit control frames directly without DCA + DCF Manager */
      TransmitControlFrameImmediately (packet, hdr, MicroSeconds (0));
    }
}

void
DmgWifiMac::SendEmptyMimoBrpFrame (Mac48Address receiver, BRP_Request_Field &requestField,
                                   BeamRefinementElement &element, EdmgBrpRequestElement &edmgRequest)
{
  NS_LOG_FUNCTION (this << receiver);
  SendMimoBrpFrame (receiver, requestField, element, edmgRequest, false, TRN_T, 0);
}

void
DmgWifiMac::SendMimoBrpFrame (Mac48Address receiver, BRP_Request_Field &requestField,
                              BeamRefinementElement &element, EdmgBrpRequestElement &edmgRequest,
                              bool requestBeamRefinement, PacketType packetType, uint8_t trainingFieldLength)
{
  NS_LOG_FUNCTION (this << receiver);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION_NO_ACK);
  hdr.SetAddr1 (receiver);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();
  hdr.SetFragmentNumber (0);
  hdr.SetNoMoreFragments ();
  hdr.SetNoRetry ();

  /* Special Fields for Beam Refinement */
  if (requestBeamRefinement)
    {
      hdr.RequestBeamRefinement ();
      hdr.SetPacketType (packetType);
      hdr.SetEdmgTrainingFieldLength (trainingFieldLength);
    }

  ExtBrpFrame brpFrame;
  brpFrame.SetDialogToken (0);
  brpFrame.SetBrpRequestField (requestField);
  brpFrame.SetBeamRefinementElement (element);
  brpFrame.SetEdmgBrpRequestElement (&edmgRequest);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.unprotectedAction = WifiActionHeader::UNPROTECTED_DMG_BRP;
  actionHdr.SetAction (WifiActionHeader::UNPROTECTED_DMG, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (brpFrame);
  packet->AddHeader (actionHdr);

  //// NINA ////
  BftIdTag tag;
  tag.Set (m_bftIdMap[receiver]);
  packet->AddPacketTag (tag);
  //// NINA ////

  /* Set the best sector for transmission with this station */
  /* MIMO BRP packets are send with spatial expansion and mapping a single stream across all transmit chains */
  m_codebook->SetCommunicationMode (MIMO_MODE);
  for (auto const &txConfig : m_mimoConfigTraining)
    {
      NS_LOG_DEBUG ("Activate Transmit Antenna with AntennaID=" << static_cast<uint16_t> (txConfig.first.first)
                        << ", to SectorID=" << static_cast<uint16_t> (txConfig.first.second)
                        << ", AwvID=" << static_cast<uint16_t> (txConfig.second));
      m_codebook->SetActiveTxSectorID (txConfig.first.first, txConfig.first.second);
      if (txConfig.second != NO_AWV_ID)
        {
          m_codebook->SetActiveTxAwvID (txConfig.second);
        }
    }
  NS_LOG_INFO ("Sending MIMO BRP Frame to " << receiver << " at " << Simulator::Now ());
  /* Transmit control frames directly without DCA + DCF Manager */
  TransmitControlFrameImmediately (packet, hdr, MicroSeconds (0));
}

void
DmgWifiMac::SendFeedbackMimoBrpFrame (Mac48Address receiver, BRP_Request_Field &requestField,
                                      BeamRefinementElement &element, EdmgBrpRequestElement *edmgRequest,
                                      std::vector<ChannelMeasurementFeedbackElement> channel, std::vector<EDMGChannelMeasurementFeedbackElement> edmgChannel)
{
  NS_LOG_FUNCTION (this << receiver);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION_NO_ACK);
  hdr.SetAddr1 (receiver);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();
  hdr.SetFragmentNumber (0);
  hdr.SetNoMoreFragments ();
  hdr.SetNoRetry ();

  ExtBrpFrame brpFrame;
  brpFrame.SetDialogToken (0);
  brpFrame.SetBrpRequestField (requestField);
  brpFrame.SetBeamRefinementElement (element);
  brpFrame.SetEdmgBrpRequestElement (edmgRequest);
  for (auto &channelE : channel)
    brpFrame.AddChannelMeasurementFeedback (&channelE);
  for (auto &edmgChannelE : edmgChannel)
    brpFrame.AddEdmgChannelMeasurementFeedback (&edmgChannelE);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.unprotectedAction = WifiActionHeader::UNPROTECTED_DMG_BRP;
  actionHdr.SetAction (WifiActionHeader::UNPROTECTED_DMG, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (brpFrame);
  packet->AddHeader (actionHdr);

  /* Set the best sector for transmission with this station */
  if (m_muMimoBfPhase != MU_SISO_FBCK)
    {
      /* MIMO BRP packets are send with spatial expansion and mapping a single stream across all transmit chains */
      m_codebook->SetCommunicationMode (MIMO_MODE);
      for (auto const &txConfig : m_mimoConfigTraining)
        {
          NS_LOG_DEBUG ("Activate Transmit Antenna with AntennaID=" << static_cast<uint16_t> (txConfig.first.first)
                            << ", to SectorID=" << static_cast<uint16_t> (txConfig.first.second)
                            << ", AwvID=" << static_cast<uint16_t> (txConfig.second));
          m_codebook->SetActiveTxSectorID (txConfig.first.first, txConfig.first.second);
          if (txConfig.second != NO_AWV_ID)
            {
              m_codebook->SetActiveTxAwvID (txConfig.second);
            }
        }
    }
  else
    {
      ANTENNA_CONFIGURATION_TX antennaConfigTx = std::get<0> (m_bestAntennaConfig[receiver]);
      m_codebook->SetActiveTxSectorID (antennaConfigTx.first, antennaConfigTx.second);
    }

  NS_LOG_INFO ("Sending MIMO BRP Frame with Feedback to " << receiver << " at " << Simulator::Now ());

  /* Transmit control frames directly without DCA + DCF Manager */
  TransmitControlFrameImmediately (packet, hdr, MicroSeconds (0));
}

/*
 *  Currently, we use BRP to train receive sector since we did not have RXSS in A-BFT.
 */
void
DmgWifiMac::InitiateBrpSetupSubphase (BRP_TRAINING_TYPE type, Mac48Address receiver)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("Initiating BRP Setup Subphase with " << receiver << " at " << Simulator::Now ());

  /* Set flags related to the BRP Setup Subphase */
  m_isBrpResponder[receiver] = false;
  m_isBrpSetupCompleted[receiver] = false;
  m_raisedBrpSetupCompleted[receiver] = false;

  BeamRefinementElement element;
  /* The BRP Setup subphase starts with the initiator sending BRP with Capability Request = 1 */
  element.SetAsBeamRefinementInitiator (true);
  element.SetCapabilityRequest (true);

  BRP_Request_Field requestField;
  /* Currently, we do not support MID + BC Subphases */
  requestField.SetMID_REQ (false);
  requestField.SetBC_REQ (false);
  if ((type == BRP_TRN_R) || (type == BRP_TRN_T_R))
    {
      requestField.SetL_RX (m_codebook->GetTotalNumberOfReceiveSectors ());
    }
  if ((type == BRP_TRN_T) || (type == BRP_TRN_T_R))
    {
      requestField.SetTX_TRN_REQ (true);
      element.SetSnrRequested (true);
      element.SetChannelMeasurementRequested (true);
      element.SetNumberOfTapsRequested (TAPS_1);
      element.SetSectorIdOrderRequested (true);
    }
  requestField.SetTXSectorID (m_codebook->GetActiveTxSectorID ());
  requestField.SetTXAntennaID (m_codebook->GetActiveAntennaID ());

  SendEmptyBrpFrame (receiver, requestField, element);
}

void
DmgWifiMac::ReportSnrValue (AntennaID antennaID, SectorID sectorID, uint8_t trnUnitsRemaining, uint8_t subfieldsRemaining,
                            uint8_t pSubfieldsRemaining, double snr, bool isTxTrn, uint8_t index)
{
  NS_LOG_FUNCTION (this << uint16_t (antennaID) << uint16_t (sectorID) << uint16_t (subfieldsRemaining)
                   << uint16_t (trnUnitsRemaining) << snr << isTxTrn);
  if (m_recordTrnSnrValues)
    {
      /* Add the SNR of the TRN Subfield */
      m_trn2Snr.push_back (snr);

      /* Check if this is the last TRN Subfield, so we extract the best Tx/RX sector/AWV */
      if ((trnUnitsRemaining == 0) && (subfieldsRemaining == 0) && (pSubfieldsRemaining == 0))
        {
          /* Iterate over all the SNR values and get the ID of the AWV with the highest AWVs */
          uint8_t awvID = std::distance (m_trn2Snr.begin (), std::max_element (m_trn2Snr.begin (), m_trn2Snr.end ()));
          awvID = awvID/index;
          m_recordTrnSnrValues = false;

          if (isTxTrn)
            {
              /* Feedback the ID of the best AWV ID for TRN-TX */
              BRP_Request_Field requestField;
              requestField.SetTXAntennaID (m_codebook->GetActiveAntennaID ());
              requestField.SetTXSectorID (m_codebook->GetActiveTxSectorID ());

              BeamRefinementElement element;
              element.SetTxTrainResponse (true);
              element.SetTxTrnOk (true);
              element.SetBsFbck (awvID);
              element.SetBsFbckAntennaID (antennaID);

              Simulator::Schedule (GetSifs (), &DmgWifiMac::SendEmptyBrpFrame, this, m_peerStation, requestField, element);

              m_trn2snrMap[m_peerStation] = m_trn2Snr;
              NS_LOG_INFO ("Received last TRN-T Subfield for transmit beam refinement from "
                           << m_peerStation << " to " << GetAddress ()
                           << ". Send BRP Feedback with Best AWV ID=" << uint16_t (awvID)
                           << " with SNR=" <<  RatioToDb (m_trn2Snr[awvID]) << " dB");
            }
          else
            {
              NS_LOG_INFO ("Received last TRN-R Subfield for receive beam refinement from "
                           << GetAddress () << " to " << m_peerStation
                           << ". Best AWV ID=" << uint16_t (awvID));
            }

          m_trn2Snr.clear ();
        }
    }
}

void
DmgWifiMac::InitiateBrpTransaction (Mac48Address receiver, uint8_t L_RX, bool TX_TRN_REQ)
{
  NS_LOG_FUNCTION (this << receiver << uint16_t (L_RX) << TX_TRN_REQ);
  NS_LOG_INFO ("Start BRP Transactions with " << receiver << " at " << Simulator::Now ());

  BRP_Request_Field requestField;
  requestField.SetMID_REQ (false);
  requestField.SetBC_REQ (false);
  requestField.SetL_RX (L_RX);
  requestField.SetTX_TRN_REQ (TX_TRN_REQ);
  requestField.SetTXAntennaID (m_codebook->GetActiveAntennaID ());
  requestField.SetTXSectorID (m_codebook->GetActiveTxSectorID ());

  BeamRefinementElement element;
  element.SetAsBeamRefinementInitiator (true);
  element.SetTxTrainResponse (false);
  element.SetRxTrainResponse (false);
  element.SetTxTrnOk (false);
  element.SetCapabilityRequest (false);

  /* Transaction Information */
  m_peerStation = receiver;

  /* Send BRP Frame terminating the setup phase from the responder side */
//  SendBrpFrame (receiver, requestField, element);

  if (TX_TRN_REQ)
    {
      /* Inform the codebook to start iterating over the custom AWVs within this sector */
      m_codebook->InitiateBRP (m_codebook->GetActiveAntennaID (), m_codebook->GetActiveTxSectorID (), RefineTransmitSector);
      /* Request transmit training */
      SendBrpFrame (receiver, requestField, element, true, TRN_T,
                    m_codebook->GetNumberOfAWVs (m_codebook->GetActiveAntennaID (), m_codebook->GetActiveTxSectorID ()));
    }
  else
    {
      /* Request receive training */
      SendEmptyBrpFrame (receiver, requestField, element);
    }
}

//void
//DmgWifiMac::InitiateBrpTransaction (Mac48Address receiver, BRP_Request_Field &requestField)
//{
//  NS_LOG_FUNCTION (this << receiver);
//  NS_LOG_INFO ("Start BRP Transactions with " << receiver << " at " << Simulator::Now ());
//  BeamRefinementElement element;
//  element.SetAsBeamRefinementInitiator (true);
//  element.SetTxTrainResponse (false);
//  element.SetRxTrainResponse (false);
//  element.SetTxTrnOk (false);
//  element.SetCapabilityRequest (false);
//  /* Transaction Information */
//  m_peerStation = receiver;
//  if (requestField.GetTX_TRN_REQ ())
//    {
//      /* Request transmit training */
//      SendBrpFrame (receiver, requestField, element, true, TRN_T, m_codebook->GetNumberOfAWVs ());
//    }
//  else
//    {
//      /* Request receive training */
//      SendBrpFrame (receiver, requestField, element);
//    }
//}

/* EDMG TRN Set functions */
void
DmgWifiMac::SetTrnSeqLength (TRN_SEQ_LENGTH trnSeqLen)
{
  m_trnSeqLength = trnSeqLen;
}

void
DmgWifiMac::Set_EDMG_TRN_P (uint8_t edmgTrnP)
{
  if ((edmgTrnP == 0) || (edmgTrnP == 1) || (edmgTrnP == 2) || (edmgTrnP == 4))
    m_edmgTrnP = edmgTrnP;
  else
    NS_ABORT_MSG ("Unvalid EDMG TRN Unit P value - EDMG TRN Unit P must be equal to 0, 1, 2, 4");
}

void
DmgWifiMac::Set_EDMG_TRN_M (uint8_t edmgTrnM)
{
  NS_ABORT_MSG_IF (((edmgTrnM < 1) || (edmgTrnM > 16)), "Unvalid EDMG TRN Unit M value - EDMG TRN Unit M must be between 1 and 16");
  m_edmgTrnM = edmgTrnM;
}

void
DmgWifiMac::Set_EDMG_TRN_N (uint8_t edmgTrnN)
{
  NS_ABORT_MSG_IF ((m_edmgTrnM % edmgTrnN) != 0, "The value of EDMG TRN Unit M must be an integer multiple of the value of EDMFG TRN Unit N value");
  if ((edmgTrnN == 1) || (edmgTrnN == 2) || (edmgTrnN == 3) || (edmgTrnN == 4)  || (edmgTrnN == 8))
    m_edmgTrnN = edmgTrnN;
  else
    NS_ABORT_MSG ("Unvalid EDMG TRN Unit N value - EDMG TRN Unit N must be equal to 1, 2, 3, 4 or 8");
}

void
DmgWifiMac::Set_RxPerTxUnits (uint8_t rxPerTxUnits)
{
  m_rxPerTxUnits = rxPerTxUnits;
}

/* EDMG TRN Get functions */
TRN_SEQ_LENGTH
DmgWifiMac::GetTrnSeqLength (void) const
{
  return m_trnSeqLength;
}

uint8_t
DmgWifiMac::Get_EDMG_TRN_P (void) const
{
  return m_edmgTrnP;
}

uint8_t
DmgWifiMac::Get_EDMG_TRN_M (void) const
{
  return m_edmgTrnM;
}

uint8_t
DmgWifiMac::Get_EDMG_TRN_N (void) const
{
  return m_edmgTrnN;
}

uint8_t
DmgWifiMac::Get_RxPerTxUnits (void) const
{
  return m_rxPerTxUnits;
}

bool
DmgWifiMac::GetChannelAggregation (void) const
{
  return m_chAggregation;
}

uint8_t
DmgWifiMac::GetBrpCdown (void) const
{
  return m_brpCdown;
}

/* EDMG SU-MIMO Beamforming functions */

//// NINA ////
void
DmgWifiMac::StartSuMimoBeamforming (Mac48Address responder, bool isBrpTxssNeeded, std::vector<AntennaID> antennaIds, bool useAwvs)
{
  /* Check that all necessary conditions are satisfied before starting SU-MIMO BFT */
  NS_LOG_FUNCTION(this << responder);
  NS_ABORT_MSG_IF (!GetDmgWifiPhy ()->IsSuMimoSupported (), "The initiator STA needs to support SU-MIMO transmissions");
  Ptr<EdmgCapabilities> capabilities = GetPeerStationEdmgCapabilities (responder);
  if (capabilities != 0)
    {
      Ptr<BeamformingCapabilitySubelement> beamformingCapability = StaticCast<BeamformingCapabilitySubelement> (capabilities->GetSubElement (BEAMFORMING_CAPABILITY_SUBELEMENT));
      if (beamformingCapability != 0)
        {
          NS_ABORT_MSG_IF (!beamformingCapability->GetSuMimoSupported (), "The responder STA needs to support SU-MIMO transmissions");
        }
    }
  else
    NS_ABORT_MSG ("We need to have the responder STA EDMG capabilities before starting SU-MIMO beamforming");
  STATION_ANTENNA_CONFIG_MAP::iterator it = m_bestAntennaConfig.find (responder);
  NS_ABORT_MSG_IF (it == m_bestAntennaConfig.end (), "The STAs need to have a control link established before starting SU-MIMO BF training");
  NS_ABORT_MSG_IF (antennaIds.size () > m_codebook->GetTotalNumberOfAntennas (),
                   "The number of antennas used in the SU-MIMO BF must be smaller than the total number of antennas of the STA");

  if (isBrpTxssNeeded)
    {
      /* Set up the antenna combinations to test in each packet of the MIMO BRP TXSS and calculate the number of MIMO BRP TXSS packets that we need
       * if there are multiple antennas which are connected to the same RF Chain we need multiple BRP packets to train them, otherwise we just need one. */
      m_txssPackets = m_codebook->SetUpMimoBrpTxss (antennaIds, responder);
      m_txssRepeat = m_txssPackets;
      m_codebook->SetUseAWVsMimoBft (useAwvs);
    }
  m_low->MIMO_BFT_Phase_Started ();
  // Set the antenna configuration to be used for transmitting BRP frames with spatial expansion - we use the optimal configuration for the user with all antennas
  for (auto & antenna: antennaIds)
    {
      ANTENNA_CONFIGURATION_TX antennaConfigTx = std::get<0> (m_bestAntennaConfig[responder]);
      ANTENNA_CONFIGURATION config = std::make_pair (antenna, antennaConfigTx.second);
      AWV_CONFIGURATION pattern = std::make_pair (config, NO_AWV_ID);
      m_mimoConfigTraining.push_back (pattern);
    }
  // Update the BFT id between the peer stations
  BFT_ID_MAP::iterator it1 = m_bftIdMap.find (responder);
  uint16_t bftId = it1->second + 1;
  m_bftIdMap [m_peerStationAddress] = bftId;

  if (isBrpTxssNeeded)
    StartMimoBrpTxssSetup (responder, antennaIds);
  else
    StartSuSisoFeedback (responder, antennaIds);
}

void
DmgWifiMac::StartMimoBrpTxssSetup (Mac48Address responder, std::vector<AntennaID> antennaIds)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("Initiating MIMO BRP TXSS Setup Subphase with " << responder);

  /* Set flags related to the BRP Setup Subphase */
  m_isBrpResponder[responder] = false;
  m_isMimoBrpSetupCompleted[responder] = false;
  m_suMimoBfPhase = SU_SISO_SETUP_PHASE;

  BeamRefinementElement element;
  /* The BRP Setup subphase starts with the initiator sending BRP with Capability Request = 1 */
  element.SetAsBeamRefinementInitiator (true);
  element.SetCapabilityRequest (true);
  element.SetSnrRequested (true);
  element.SetSectorIdOrderRequested (true);
  element.SetEdmgExtensionFlag (true);
  element.SetBfTrainingType (SU_MIMO_BF);

  BRP_Request_Field requestField;
  /* Currently, we do not support MID + BC Subphases */
  requestField.SetMID_REQ (false);
  requestField.SetBC_REQ (false);

  EdmgBrpRequestElement edmgRequestElement;
  /* The BRP frame sent by the initiator during the setup phase to start the SISO phase shall have the BRP-TXSS,
   * TXSS-INITIATOR and TXSS-MIMO fields within the EDMG BRP Request element all set to 1. */
  edmgRequestElement.SetBRP_TXSS (true);
  edmgRequestElement.SetTXSS_Initiator (true);
  edmgRequestElement.SetTXSS_MIMO (true);
  /* The L-RX field within the EDMG BRP Request element or EDMG BRP field in the BRP
   * frames transmitted during the setup phase of a MIMO BRP TXSS shall be set to 0 */
  edmgRequestElement.SetL_RX (0);

  edmgRequestElement.SetRequestedEDMG_TRN_UnitP (m_edmgTrnP);
  edmgRequestElement.SetRequestedEDMG_TRN_UnitM (m_edmgTrnM);
  edmgRequestElement.SetRequestedEDMG_TRN_UnitN (m_edmgTrnN);

  edmgRequestElement.SetTXSectorID (m_codebook->GetActiveTxSectorID ());
  edmgRequestElement.SetTX_Antenna_Mask (antennaIds);

  /* The TXSS_Packets and TXSS_Repeat fields indicate the number of BRP packets needed for transmit and receive training. */
  edmgRequestElement.SetTXSS_Packets (m_txssPackets);
  edmgRequestElement.SetTXSS_Repeat (m_txssRepeat);

  SendEmptyMimoBrpFrame (responder, requestField, element, edmgRequestElement);

}

void
DmgWifiMac::StartSuSisoFeedback (Mac48Address responder, std::vector<AntennaID> antennaIds)
{

}

void
DmgWifiMac::StartMimoBrpTxss (void)
{
  NS_LOG_FUNCTION (this << m_peerStation);
  NS_LOG_INFO ("DMG STA Starting MIMO BRP TXSS");

  /* To do: Calculate the correct duration for initiator TXSS (or full SISO Phase?) */
//  m_sectorSweepDuration = CalculateSectorSweepDuration (m_peerAntennas, m_codebook->GetTotalNumberOfAntennas (),
//                                                        m_codebook->GetTotalNumberOfTransmitSectors ());

  if (m_isBrpResponder[m_peerStation])
    {
      m_suMimoBfPhase = SU_SISO_RESPONDER_TXSS;
    }
  else
    {
      m_suMimoBfPhase = SU_SISO_INITIATOR_TXSS;
    }
  m_brpCdown = m_txssPackets * m_peerTxssRepeat - 1;
  m_remainingTxssPackets = m_txssPackets - 1;
  m_peerTxssRepeat = m_peerTxssRepeat - 1;
  //Set up the lists of sectors that will be tested for each antenna in this MIMO BRP Packet
  bool firstCombination = true;
  m_codebook->InitializeMimoSectorSweeping (m_peerStation, TransmitSectorSweep, firstCombination);
  SendMimoBrpTxssFrame (m_peerStation);
}

void
DmgWifiMac::SendMimoBrpTxssFrame (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  BeamRefinementElement element;
  element.SetAsBeamRefinementInitiator (!m_isBrpResponder[address]);
  element.SetBfTrainingType (SU_MIMO_BF);
  BRP_Request_Field requestField;
  EdmgBrpRequestElement edmgRequestElement;
  edmgRequestElement.SetTXSectorID (m_codebook->GetActiveTxSectorID ());
  /* Get antenna IDs of antennas to be trained in this packet from codebook */
  edmgRequestElement.SetTX_Antenna_Mask (m_codebook->GetCurrentMimoAntennaIdList ());
  edmgRequestElement.SetBRP_CDOWN (m_brpCdown);
  //* Get the Maximum number of sectors to be tested from all the antennas
  double numberOfSubfields = m_codebook->GetNumberOfTrnSubfieldsForMimoBrpTxss ();
  uint8_t trnUnits = uint8_t (ceil (numberOfSubfields / (m_edmgTrnM / m_edmgTrnN)));
  SendMimoBrpFrame (address, requestField, element, edmgRequestElement, true, TRN_T, trnUnits);
}

void
DmgWifiMac::EndMimoTrnField (void)
{
  NS_LOG_FUNCTION (this);
  if (m_brpCdown == 0)
    {
      if ((m_suMimoBfPhase == SU_SISO_INITIATOR_TXSS) || (m_suMimoBfPhase == SU_SISO_RESPONDER_TXSS))
        {
          m_suMimoSisoPhaseMeasurements (m_peerStation, m_suMimoSisoSnrMap, m_edmgTrnN, m_bftIdMap [m_peerStation]);
          Simulator::Schedule (m_mbifs, &DmgWifiMac::SendSuMimoTxssFeedback, this);
          if (m_isBrpResponder[m_peerStation])
            m_suMimoBfPhase = SU_SISO_RESPONDER_FBCK;
          else
            m_suMimoBfPhase = SU_SISO_INITIATOR_FBCK;
          m_recordTrnSnrValues = false;
        }
      else if (m_suMimoBfPhase == SU_MIMO_INITIATOR_SMBT)
        {
          m_suMimoBfPhase = SU_MIMO_RESPONDER_SMBT;
          Simulator::Schedule (m_mbifs, &DmgWifiMac::StartSuMimoBfTrainingSubphase, this);
        }
      else if (m_suMimoBfPhase == SU_MIMO_RESPONDER_SMBT)
        {
          m_suMimoBfPhase = SU_MIMO_FBCK_PHASE;
          Simulator::Schedule (m_mbifs, &DmgWifiMac::SendSuMimoBfFeedbackFrame, this);
        }
      else if (m_muMimoBfPhase == MU_MIMO_BF_TRAINING)
        {
           m_recordTrnSnrValues = false;
           m_muMimoBfPhase = MU_MIMO_BF_FBCK;
           m_codebook->SetReceivingInQuasiOmniMode ();
        }
    }
}

void
DmgWifiMac::SendSuMimoTxssFeedback (void)
{
  BeamRefinementElement element;
  element.SetSnrPresent (true);
  element.SetSectorIdOrderPresent (true);
  element.SetEdmgExtensionFlag (true);
  element.SetEdmgChannelMeasurementPresent (true);
  element.SetBfTrainingType (SU_MIMO_BF);
  element.SetSswFrameType (BRP_FRAME);

  BRP_Request_Field requestField;
  /* Currently, we do not support MID + BC Subphases */
  requestField.SetMID_REQ (false);
  requestField.SetBC_REQ (false);

  EdmgBrpRequestElement edmgRequestElement;
  /* Add a Channel Measurement Feedback Element */
  std::vector<ChannelMeasurementFeedbackElement> channelElements;
  /* Add an EDMG Channel Measurement Feedback Element */
  std::vector<EDMGChannelMeasurementFeedbackElement> edmgChannelElements;

  std::sort(m_mimoSisoSnrList.begin (), m_mimoSisoSnrList.end ());
  /* To make sure that the size of the packet payload is below the maximum size specified in the standard
   * for DMG CTRL mode (1023 Bytes),the maximum amount of measurements that we can feedback is 189. Therefore,
   * we only feedback the highest 189 measurements */
  double minSnr;
  if (m_mimoSisoSnrList.size () > 189)
    {
      minSnr = m_mimoSisoSnrList.at (m_mimoSisoSnrList.size () - 189 - 1);
    }
  else
    {
      minSnr = m_mimoSisoSnrList.front () - 0.1;
    }
  uint16_t numberOfMeasurments = 0;
  uint8_t numberOfMeasurmentsElement = 0;
  SNR_LIST_ITERATOR start;
  Ptr<ChannelMeasurementFeedbackElement> channelElement = Create<ChannelMeasurementFeedbackElement> ();
  Ptr<EDMGChannelMeasurementFeedbackElement> edmgChannelElement = Create<EDMGChannelMeasurementFeedbackElement> ();
  /* Fill in the feedback in Channel Measurement Feedback and EDMG Channel Measurement Feedback Elements. The maximum size of the
   * information elements is 255 bytes which corresponds to 63 measurements, therefore if we have more than 63 measurements, we need to split
   * the feedback into multiple Channel Measurement Feedback and EDMG Channel Measurement Feedback Elements. */
  for (SU_MIMO_SNR_MAP::iterator it = m_suMimoSisoSnrMap.begin (); it != m_suMimoSisoSnrMap.end (); it++)
    {
      start = it->second.begin();
      SNR_LIST_ITERATOR snrIt = it->second.begin ();
      while (snrIt != it->second.end ()) {
          while (numberOfMeasurmentsElement < 63 && snrIt != it->second.end ())
            {
              if (*snrIt > minSnr)
                {
                  uint8_t snr = MapSnrToInt (*snrIt);
                  channelElement->AddSnrItem (snr);
                  EDMGSectorIDOrder order;
                  order.RXAntennaID = std::get<1> (it->first);
                  order.TXAntennaID = std::get<2> (it->first);
                  uint32_t awv = std::distance(start,snrIt) + 1;
                  order.SectorID = awv / m_edmgTrnN;
                  edmgChannelElement->Add_EDMG_SectorIDOrder (order);
                  edmgChannelElement->Add_BRP_CDOWN (std::get<0> (it->first));
                  numberOfMeasurmentsElement++ ;
                  numberOfMeasurments++ ;
                }
              snrIt++ ;
            }
          if (numberOfMeasurmentsElement == 63)
            {
              numberOfMeasurmentsElement = 0;
              channelElements.push_back (*channelElement);
              edmgChannelElements.push_back (*edmgChannelElement);
              channelElement = Create<ChannelMeasurementFeedbackElement> ();
              edmgChannelElement = Create<EDMGChannelMeasurementFeedbackElement> ();
            }
        }
    }
  if (numberOfMeasurmentsElement != 0)
    {
      channelElements.push_back (*channelElement);
      edmgChannelElements.push_back (*edmgChannelElement);
    }
  element.SetExtendedNumberOfMeasurements (numberOfMeasurments);
  SendFeedbackMimoBrpFrame (m_peerStation, requestField, element, &edmgRequestElement, channelElements, edmgChannelElements);
}

void
DmgWifiMac::StartSuMimoMimoPhase (Mac48Address from, MIMO_ANTENNA_COMBINATIONS_LIST candidates,
                                  uint8_t txCombinationsRequested, bool useAwvs)
{
  NS_LOG_FUNCTION (this << from << uint16_t (txCombinationsRequested) << useAwvs);
  m_peerStation = from;
  m_codebook->SetUseAWVsMimoBft (useAwvs);
  NS_ABORT_MSG_IF (txCombinationsRequested > 64, "Number of Tx Combinations requested is too big");
  m_txSectorCombinationsRequested = txCombinationsRequested;
  // Note: For now we assume that only one antenna is connected to each RF Chain - all candidates have the same antenna combination
  Antenna2SectorList candidateSectors;
  std::vector<AntennaID> candidateAntennas;
  SectorIDList rxSectors;
  for (MIMO_ANTENNA_COMBINATIONS_LIST_I it = candidates.begin (); it != candidates.end (); it++)
    {
      for (MIMO_ANTENNA_COMBINATION::iterator it1 = it->begin (); it1 != it->end (); it1++)
        {
          Antenna2SectorListI iter = candidateSectors.find (it1->first);
          if (iter != candidateSectors.end ())
            {
              iter->second.push_back (it1->second);
            }
          else
            {
              SectorIDList sectors;
              sectors.push_back (it1->second);
              candidateSectors[it1->first] = sectors;
              candidateAntennas.push_back (it1->first);
            }
          if (std::find (rxSectors.begin (), rxSectors.end (), it1->second) == rxSectors.end ())
            {
              rxSectors.push_back (it1->second);
            }
        }
    }
  /* Note: While on the Tx side we need to test all possible combinations of sectors to measure the mutual interference they cause
   * each other, on the Rx side the measurements done at an Rx antenna are completely independent of the antenna configuration of the second
   * antenna, allowing us to reduce the training time by only testing each combination once and determing all possible combinations in postprocessing
   * by combining the measurements */
  bool trainAllRxSectors = true;
  Antenna2SectorList rxCandidateSectors;
  if (trainAllRxSectors)
    {
      rxCandidateSectors = m_codebook->GetRxSectorsList ();
    }
  else
    {
      for (auto antenna : candidateAntennas)
        {
          rxCandidateSectors[antenna] = rxSectors;
        }
    }
  m_suMimomMimoCandidatesSelected (from, candidateSectors, rxCandidateSectors, m_bftIdMap [from]);
  double numberOfSubfields = m_codebook->GetSMBT_Subfields (from, candidateAntennas, candidateSectors, rxCandidateSectors);
  m_rxCombinationsTested = numberOfSubfields;
  if (numberOfSubfields > 16)
    {
      NS_ABORT_MSG_IF (ceil (numberOfSubfields / 16) > 255, "Number of requested TRN Units is too large");
      m_lTxRx = uint8_t (ceil (numberOfSubfields / 16));
      m_edmgTrnMRequested = uint8_t (std::ceil (numberOfSubfields / m_lTxRx));
    }
  else
    {
      m_edmgTrnMRequested = numberOfSubfields;
      m_lTxRx = 1;
    }
  Simulator::Schedule (m_mbifs, &DmgWifiMac::SendSuMimoSetupFrame, this);
}

void
DmgWifiMac::SendSuMimoSetupFrame (void)
{
  NS_LOG_FUNCTION (this);
  MimoSetupControlElement setupElement;
  setupElement.SetMimoBeamformingType (SU_MIMO_BEAMFORMING);
  // Currently we only support non-reciprocal MIMO phase
  setupElement.SetMimoPhaseType (MIMO_PHASE_NON_RECPIROCAL);
  setupElement.SetAsInitiator (!m_isBrpResponder[m_peerStation]);
  setupElement.SetLTxRx (m_lTxRx);
  setupElement.SetRequestedEDMGTRNUnitM (m_edmgTrnMRequested);
  setupElement.SetNumberOfTXSectorCombinationsRequested (m_txSectorCombinationsRequested);
  // Ask for time domain channel response
//  setupElement.SetChannelMeasurementRequested (true);
//  setupElement.SetNumberOfTapsRequested ();

  // Send MIMO BF Setup frame
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION_NO_ACK);
  hdr.SetAddr1 (m_peerStation);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();
  hdr.SetFragmentNumber (0);
  hdr.SetNoMoreFragments ();
  hdr.SetNoRetry ();

  ExtMimoBfSetupFrame setupFrame;
  setupFrame.SetMimoSetupControlElement (setupElement);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.unprotectedAction = WifiActionHeader::UNPROTECTED_MIMO_BF_SETUP;
  actionHdr.SetAction (WifiActionHeader::UNPROTECTED_DMG, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (setupFrame);
  packet->AddHeader (actionHdr);

  /* Set the best sector for transmission with this station */
  /* MIMO BF setup packets are send with spatial expansion and mapping a single stream across all transmit chains */
  m_codebook->SetCommunicationMode (MIMO_MODE);
  for (auto const &txConfig : m_mimoConfigTraining)
    {
      NS_LOG_DEBUG ("Activate Transmit Antenna with AntennaID=" << static_cast<uint16_t> (txConfig.first.first)
                        << ", to SectorID=" << static_cast<uint16_t> (txConfig.first.second)
                        << ", AwvID=" << static_cast<uint16_t> (txConfig.second));
      m_codebook->SetActiveTxSectorID (txConfig.first.first, txConfig.first.second);
      if (txConfig.second != NO_AWV_ID)
        {
          m_codebook->SetActiveTxAwvID (txConfig.second);
        }
    }

  NS_LOG_INFO ("Sending MIMO BF Setup frame to " << m_peerStation << " at " << Simulator::Now ());

  /* Transmit control frames directly without TXOP + Channel Access Manager */
  TransmitControlFrameImmediately (packet, hdr, MicroSeconds (0));
}

void
DmgWifiMac::StartSuMimoBfTrainingSubphase (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("EDMG STA Starting SMBT with " << m_peerStation);

  /* To do:Calculate the correct duration for SMBT (or full MIMO Phase?) */
//  m_sectorSweepDuration = CalculateSectorSweepDuration (m_peerAntennas, m_codebook->GetTotalNumberOfAntennas (),
//                                                        m_codebook->GetTotalNumberOfTransmitSectors ());

  //Set up the lists of sectors that will be tested for each antenna in this MIMO BRP Packet
  bool firstCombination = true;
  m_codebook->InitializeMimoSectorSweeping (m_peerStation, TransmitSectorSweep, firstCombination);
  if (m_isBrpResponder[m_peerStation])
    {
      m_suMimoBfPhase = SU_MIMO_RESPONDER_SMBT;
    }
  else
    {
      m_suMimoBfPhase = SU_MIMO_INITIATOR_SMBT;
    }
  // Count the number of packets according to the number of Units needed to test all combinations - if we are testing AWVs we test all possible combinations
  m_numberOfUnitsRemaining = (m_codebook->CountMimoNumberOfTxSubfields (m_peerStation) * m_peerLTxRx) ;
  NS_ABORT_MSG_IF ((std::ceil (m_numberOfUnitsRemaining / 255.0) - 1) > 63, "Number of BRP packets needed is too large");
  m_brpCdown = (std::ceil (m_numberOfUnitsRemaining / 255.0)) - 1;
  SendMimoBfTrainingBrpFrame (m_peerStation);
}

void
DmgWifiMac::SendMimoBfTrainingBrpFrame (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  BeamRefinementElement element;
  if (m_suMimoBeamformingTraining)
    {
      element.SetAsBeamRefinementInitiator (!m_isBrpResponder[address]);
      element.SetBfTrainingType (SU_MIMO_BF);
    }
  else if (m_muMimoBeamformingTraining)
    {
      element.SetAsBeamRefinementInitiator (true);
      element.SetBfTrainingType (MU_MIMO_BF);
    }
  BRP_Request_Field requestField;
  EdmgBrpRequestElement edmgRequestElement;
  edmgRequestElement.SetTXSectorID (m_codebook->GetActiveTxSectorID ());
  /* Get antenna mask of antennas to be trained from codebook */
  edmgRequestElement.SetTX_Antenna_Mask (m_codebook->GetCurrentMimoAntennaIdList ());
  edmgRequestElement.SetBRP_CDOWN (m_brpCdown);
  // if there are multiple SMBT packets, calculate the number of TRN units in the packet
  uint8_t trnUnits;
  if (m_brpCdown != 0)
    {
      trnUnits = (255 / m_peerLTxRx) * m_peerLTxRx;
      m_numberOfUnitsRemaining = m_numberOfUnitsRemaining - trnUnits;
    }
  else
    {
      trnUnits = m_numberOfUnitsRemaining;
    }
  SendMimoBrpFrame (address, requestField, element, edmgRequestElement, true, TRN_RT, trnUnits);
}

void
DmgWifiMac::SendSuMimoBfFeedbackFrame (void)
{
  NS_LOG_FUNCTION (this);
  // Choose the optimal Tx sector combinations to be fed back to the peer station
  BEST_TX_COMBINATIONS_AWV_IDS bestCombinations = FindBestTxCombinations (m_peerTxSectorCombinationsRequested,
                                                                          m_rxCombinationsTested, m_peerAntennaIds.size (),
                                                                          m_codebook->GetCurrentMimoAntennaIdList ().size (),
                                                                          m_mimoSnrList, true);

  // HANY: Pass MIMO SNR measurements to the user, we need to map thse SNR measuremetns to a particular TX & RX combinations.
//  std::cout << "SendSuMimoBfFeedbackFrame from " << GetAddress () << " to " << m_peerStation << std::endl;
//  for (auto b : m_mimoSnrList)
//    {
//      for (auto s : b.second)
//        {
//          std::cout << uint16_t (b.first) << ", " << RatioToDb (s) << std::endl;
//        }
//    }
//  std::cout << "Number of Measurements recorded: " << m_mimoSnrList.size () << std::endl;

  ExtMimoBfFeedbackFrame feedbackFrame;
  MIMOFeedbackControl feedbackElement;
  feedbackElement.SetMimoBeamformingType (SU_MIMO_BEAMFORMING);
  feedbackElement.SetLinkTypeAsInitiator (!m_isBrpResponder[m_peerStation]);
  feedbackElement.SetComebackDelay (0);
  feedbackElement.SetChannelMeasurementPresent (m_timeDomainChannelResponseRequested);
  if (m_timeDomainChannelResponseRequested)
    {
      feedbackElement.SetNumberOfTapsPresent (m_numberOfTapsRequested);
      // To do: add time domain channel measurement
    }
  feedbackElement.SetNumberOfTXSectorCombinationsPresent (bestCombinations.size ());
  feedbackElement.SetNumberOfTxAntennas (m_peerAntennaIds.size ());
  feedbackElement.SetNumberOfRxAntennas (m_codebook->GetCurrentMimoAntennaIdList ().size ());
  Ptr<ChannelMeasurementFeedbackElement> channelElement = Create<ChannelMeasurementFeedbackElement> ();
  Ptr<EDMGChannelMeasurementFeedbackElement> edmgChannelElement = Create<EDMGChannelMeasurementFeedbackElement> ();
  uint8_t numberOfMeasurementsElement = 0;
  // Delete the results from previous trainings
  m_suMimoRxCombinations.erase (m_peerStation);
  for (BEST_TX_COMBINATIONS_AWV_IDS::iterator it = bestCombinations.begin (); it != bestCombinations.end (); it++)
    {
      MIMO_AWV_CONFIGURATION rxCombination = m_codebook->GetMimoConfigFromRxAwvId (it->second, m_peerStation);
      BEST_ANTENNA_SU_MIMO_COMBINATIONS::iterator it1 = m_suMimoRxCombinations.find (m_peerStation);
      if (it1 != m_suMimoRxCombinations.end ())
        {
          MIMO_AWV_CONFIGURATIONS *rxConfigs = &(it1->second);
          rxConfigs->push_back (rxCombination);
        }
      else
        {
          MIMO_AWV_CONFIGURATIONS rxConfigs;
          rxConfigs.push_back (rxCombination);
          m_suMimoRxCombinations[m_peerStation] = rxConfigs;
        }

      uint16_t txId = it->first;
      MIMO_SNR_LIST measurements;
      for (auto & rxId : it->second)
        {
          measurements.push_back (m_mimoSnrList.at ((txId - 1) * m_rxCombinationsTested + rxId.second - 1));
        }
      /* Check that the BRP CDOWN of all measurements matches - check that all measurements are from the same Tx IDx. */
      BRP_CDOWN firstBrpCdown = measurements.at (0).first;
      for (auto & measurement : measurements)
        {
          if (measurement.first != firstBrpCdown)
            {
              NS_ABORT_MSG ("Measurements must have the same BRP index since they must be connected to the same Tx config");
            }
        }
      /* Calculate the index of Tx Combination taking into account the BRP CDOWN of the packet it was received in */
      uint16_t indexAdjust = 0;
      for (auto &measurement : m_mimoSnrList)
        {
          if (measurement.first > measurements.at (0).first)
            indexAdjust++;
        }
      txId = txId - (indexAdjust / m_rxCombinationsTested);
      uint8_t snrIndex = 0;
      for (auto tx_antenna : m_peerAntennaIds)
        {
          uint8_t rxIndex = 0;
          for (auto rx_antenna : m_codebook->GetCurrentMimoAntennaIdList ())
            {
              uint8_t snr = MapSnrToInt (measurements.at (rxIndex).second.at (snrIndex));
              channelElement->AddSnrItem (snr);
              EDMGSectorIDOrder order;
              order.RXAntennaID = rx_antenna;
              order.TXAntennaID = tx_antenna;
              order.SectorID = txId;
              edmgChannelElement->Add_EDMG_SectorIDOrder (order);
              edmgChannelElement->Add_BRP_CDOWN (measurements.at (rxIndex).first);
              snrIndex++;
              rxIndex++;
            }
        }
      numberOfMeasurementsElement+= snrIndex;
      if (numberOfMeasurementsElement + snrIndex > 63)
        {
          numberOfMeasurementsElement = 0;
          feedbackFrame.AddChannelMeasurementFeedbackElement (channelElement);
          feedbackFrame.AddEdmgChannelMeasurementFeedbackElement (edmgChannelElement);
          channelElement = Create<ChannelMeasurementFeedbackElement> ();
          edmgChannelElement = Create<EDMGChannelMeasurementFeedbackElement> ();
        }
    }
  // Send MIMO BF Feedback frame
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION_NO_ACK);
  hdr.SetAddr1 (m_peerStation);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();
  hdr.SetFragmentNumber (0);
  hdr.SetNoMoreFragments ();
  hdr.SetNoRetry ();

  feedbackFrame.SetMimoFeedbackControlElement (feedbackElement);
  feedbackFrame.AddChannelMeasurementFeedbackElement (channelElement);
  feedbackFrame.AddEdmgChannelMeasurementFeedbackElement (edmgChannelElement);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.unprotectedAction = WifiActionHeader::UNPROTECTED_MIMO_BF_FEEDBACK;
  actionHdr.SetAction (WifiActionHeader::UNPROTECTED_DMG, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (feedbackFrame);
  packet->AddHeader (actionHdr);

  /* Set the best sector for transmission with this station */
  /* MIMO BF Feedback packets are send with spatial expansion and mapping a single stream across all transmit chains */
  m_codebook->SetCommunicationMode (MIMO_MODE);
  for (auto const &txConfig : m_mimoConfigTraining)
    {
      NS_LOG_DEBUG ("Activate Transmit Antenna with AntennaID=" << static_cast<uint16_t> (txConfig.first.first)
                        << ", to SectorID=" << static_cast<uint16_t> (txConfig.first.second)
                        << ", AwvID=" << static_cast<uint16_t> (txConfig.second));
      m_codebook->SetActiveTxSectorID (txConfig.first.first, txConfig.first.second);
      if (txConfig.second != NO_AWV_ID)
        {
          m_codebook->SetActiveTxAwvID (txConfig.second);
        }
    }

  NS_LOG_INFO ("Sending MIMO BF Feedback frame to " << m_peerStation);

  /* Transmit control frames directly without DCA + DCF Manager */
  TransmitControlFrameImmediately (packet, hdr, MicroSeconds (0));
}

void
DmgWifiMac::FindAllValidCombinations (uint16_t offset, uint16_t nStreams, MIMO_FEEDBACK_SORTED_MAPS &txRxCombinations,
                                      std::vector<std::vector<uint16_t> > &validCombinations, std::vector<uint16_t> &currentCombination,
                                      std::vector<uint16_t> indexes)
{
  if (nStreams == 0)
    {
      /* We have a combination of nStreams Tx-Rx pairs, check if it's a valid one - no two Tx-Rx pairs in the combination should have
    * the same Tx or Rx Id since we want to establish independent streams. */
      bool foundValidCombination = true;
      for (auto it = currentCombination.begin (); it != currentCombination.end () - 1; it++)
        {
          for (auto insideIt = it + 1; insideIt != currentCombination.end (); insideIt++)
            {
              if ((std::get<0> (txRxCombinations.at (*it).begin ()->second) == std::get<0> (txRxCombinations.at (*insideIt).begin ()->second))
                  || (std::get<1> (txRxCombinations.at (*it).begin ()->second) == std::get<1> (txRxCombinations.at (*insideIt).begin ()->second)))
                foundValidCombination = false;
            }
        }
      /* If we have a valid combination, add it to the list of valid combinations. */
      if (foundValidCombination)
        validCombinations.push_back (currentCombination);
      return;
    }
  for (uint16_t i = offset; i <= indexes.size() - nStreams; ++i)
    {
      currentCombination.push_back(indexes[i]);
      FindAllValidCombinations (i+1, nStreams-1, txRxCombinations, validCombinations, currentCombination, indexes);
      currentCombination.pop_back ();
    }
}

void
DmgWifiMac::FindAllValidTxRxPairs (uint16_t offset, uint8_t nStreams, uint8_t nRx,
                                   std::vector<std::vector<uint16_t>> &validTxRxPairs, std::vector<uint16_t> &currentCombination,
                                   std::vector<uint16_t> indexes)
{
  if (nStreams == 0)
    {
      bool foundValidCombination = true;
      /* If we have nStreams streams established check if this is a valid combination */
      for (auto it = currentCombination.begin (); it != currentCombination.end () - 1; it++)
        {
          for (auto insideIt = it + 1; insideIt != currentCombination.end (); insideIt++)
            {
              /* Match the indexes to the correct Tx antenna Id and Rx antenna Id */
              uint8_t txId1 = std::floor (*it / static_cast<double> (nRx));
              uint8_t txId2 = std::floor (*insideIt / static_cast<double> (nRx));
              uint8_t rxId1 = (*it % nRx);
              uint8_t rxId2 = (*insideIt % nRx);
              /* If the Tx or Rx Antenna Id is the same this is not a valid combination */
              if (txId1 == txId2 || rxId1 == rxId2)
                foundValidCombination = false;
            }
        }
      if (foundValidCombination)
        {
          validTxRxPairs.push_back (currentCombination);
        }
      return;
    }
  /* Continue iterating until we have found all possible combinations */
  for (uint16_t i = offset; i <= indexes.size() - nStreams; ++i)
    {
      /* Continue adding streams until we reach nStreams */
      currentCombination.push_back (indexes[i]);
      FindAllValidTxRxPairs(i+1, nStreams-1, nRx, validTxRxPairs, currentCombination, indexes);
      currentCombination.pop_back ();
    }
}

MIMO_ANTENNA_COMBINATIONS_LIST
DmgWifiMac::FindKBestCombinations (uint16_t k, uint8_t numberOfStreams, uint8_t numberOfRxAntennas, MIMO_FEEDBACK_MAP feedback)
{
  MIMO_FEEDBACK_SORTED_MAPS combinations;
  // Split the map into different maps according to the combination of Tx Antenna Id and Rx Antenna Id,
  // sorting the maps in descending order according to the SNR.
  for (int i = 0; i < (numberOfStreams * numberOfRxAntennas); i++)
    {
      MIMO_FEEDBACK_SORTED_MAP TxRxCombination;
      TX_ANTENNA_ID txId = std::get<0> (feedback.begin ()->first);
      RX_ANTENNA_ID rxId = std::get<1> (feedback.begin ()->first);
      for (MIMO_FEEDBACK_MAP::iterator it = feedback.begin (); it != feedback.end ();)
        {
          if ((std::get<0> (it->first) == txId) && (std::get<1> (it->first) == rxId))
            {
              TxRxCombination.insert (std::make_pair(it->second, it->first));
              feedback.erase (it++);
            }
          else
            {
              ++it;
            }
        }
      combinations.push_back (TxRxCombination);
    }

  // Keep only the top K measurements for each Tx-Rx combination in order to reduce the number of calculations.
  for (MIMO_FEEDBACK_SORTED_MAPS_I it = combinations.begin (); it!= combinations.end (); it++)
    {
      if ((*it).size () > k)
        {
          MIMO_FEEDBACK_SORTED_MAP_I iter = it->begin ();
          std::advance (iter, k);
          (*it).erase (iter, it->end ());
        }
    }

  /* Find all possible valid combinations of Tx-Rx pairs - the combinations should have the matching between the Tx and Rx antennas
   * for all independent streams we want to establish and no Tx or Rx antenna should appear more than once in the different streams.
   * We use a recursive function called FindAllValidCombinations to find the valid combinations. */
  std::vector<std::vector<uint16_t> > validCombinations;
  std::vector<uint16_t> currentCombination;
  std::vector<uint16_t> indexes;
  for (uint16_t i = 0; i < combinations.size (); i++)
    indexes.push_back (i);
  FindAllValidCombinations (0, numberOfStreams, combinations, validCombinations,currentCombination, indexes);

  /* Check all valid combinations */
  MIMO_CANDIDATE_MAP candidateCombinations;
  for (auto combination : validCombinations)
    {
      /* Set an iterator at the start of the feedback map that has the Tx-Rx combination for each stream specified in the combination
       * (+ save the index in the vector combinations) */
      std::vector<std::pair<uint16_t, MIMO_FEEDBACK_SORTED_MAP_I>> combinationIterators;
      for (auto index : combination)
        {
          MIMO_FEEDBACK_SORTED_MAP_I it = combinations.at (index).begin ();
          combinationIterators.push_back (std::make_pair (index, it));
        }
      /* Find all possible combinations of antenna configurations (now looking at the Tx Antenna ID, Rx Antenna ID and the Tx Sector ID)
       * for the given Tx-Rx combinations and calculate the joint SNR by adding up the feedback SNRs of the individual configurations */
      bool endOfFinalList = false;
      // while we haven't checked all possible combinations
      while (!endOfFinalList)
        {
          MIMO_FEEDBACK_COMBINATION combination;
          SNR combinationSnr = 0;
          bool endOfList = true;
          // for each member of the combination
          for (auto && iterator : combinationIterators)
            {
              // add the antenna configuration to the list
              combination.push_back (iterator.second->second);
              // add the SNR to the joint SNR
              combinationSnr += iterator.second->first;
              // If the previous combination reached the end of the feedback configurations - move to the next configuration of the list.
              if (endOfList)
                {
                  iterator.second++;
                  // If we reach the end of the feedback configurations - reset the iterator at the start and signal to the next combination that it needs to move forward.
                  if (iterator.second == combinations.at (iterator.first).end ())
                    {
                      iterator.second = combinations.at (iterator.first).begin ();
                      endOfList = true;
                    }
                  // otherwise signal to the next combination to stay at the same feedback configuration
                  else
                    endOfList = false;
                }
            }
          // If the last combination reached the end of the feedback configuration list, we have finished checking all possible combinations.
          if (endOfList)
            endOfFinalList = true;
          candidateCombinations.insert (std::make_pair (combinationSnr, combination));
        }
    }

  /* Create a list of the K best Tx combinations according to the highest joint SNR,
   * making sure to not have any duplicate Tx combinations. The feedback candidates take into
   * account both the Tx Antenna ID and the Rx Antenna ID (we need this to make sure we are
   * training independent streams), but now we want to create a list of only Tx Antenna ID, Sector
   * ID pairs (since we are generating only a list of Tx sectors to train) so here we remove any
   * combinations which all have the same Tx Antenna ID, Sector ID pairs but different Rx IDs */
  MIMO_ANTENNA_COMBINATIONS_LIST kBestCombinations;
  for (MIMO_CANDIDATE_MAP_I it = candidateCombinations.begin (); it != candidateCombinations.end (); it++)
    {
      // Create a MIMO antenna combination from the feedback candidate by removing the Rx antenna ID.
      MIMO_ANTENNA_COMBINATION combinaton;
      for (MIMO_FEEDBACK_COMBINATION_I insideIt = it->second.begin (); insideIt != it->second.end (); insideIt++)
        {
          ANTENNA_CONFIGURATION config = std::make_pair (std::get<0> (*insideIt), std::get<2> (*insideIt));
          combinaton.push_back (config);
        }
      // Check if this combination has already been added, and if it hasn't been add it to the list of candidates
      MIMO_ANTENNA_COMBINATIONS_LIST::iterator sameConfig = std::find (kBestCombinations.begin (), kBestCombinations.end (), combinaton);
      if (sameConfig == kBestCombinations.end ())
        {
          kBestCombinations.push_back (combinaton);
        }
      // If the list of candidates is K break since we have the full list of candidates
      if (kBestCombinations.size () == k)
        break;
    }
  return kBestCombinations;
}

BEST_TX_COMBINATIONS_AWV_IDS
DmgWifiMac::FindBestTxCombinations (uint8_t nBestCombinations, uint16_t rxCombinationsTested, uint8_t nTxAntennas,
                                    uint8_t nRxAntennas, MIMO_SNR_LIST measurements, bool differentRxCombinations)
{
  BEST_TX_COMBINATIONS_AWV_IDS bestCombinations;
  std::vector<uint16_t> txIds;
  SNR_MEASUREMENT_AWV_IDs_QUEUE minSnr;
  uint16_t txCombinationsTested = measurements.size () / (rxCombinationsTested);
  std::priority_queue<std::pair<double, SU_MIMO_ANTENNA2ANTENNA>> antenna2antennaQueue;

  // Find all possible valid combinations of valid Tx-Rx pairs for the nTxAntennas streams we want to set up
  std::vector<std::vector<uint16_t>> validTxRxPairs;
  std::vector<uint16_t> currentCombination;
  std::vector<uint16_t> indexes;
  for (uint16_t i = 0; i < nTxAntennas * nRxAntennas; i++)
    indexes.push_back (i);
  if (nTxAntennas <= nRxAntennas)
    FindAllValidTxRxPairs (0, nTxAntennas, nRxAntennas, validTxRxPairs, currentCombination, indexes);
  else
    FindAllValidTxRxPairs (0, nRxAntennas, nRxAntennas, validTxRxPairs, currentCombination, indexes);

  /* For each Tx combination tested create all possible Rx combinations with the different addresses. */
  MIMO_SNR_LIST_I txStartIt = measurements.begin ();
  MIMO_SNR_LIST_I txEndIt = measurements.begin () ;

  for (uint16_t i = 0; i < txCombinationsTested; i++)
    {
      /* Initialize the iterators */
      txStartIt = txEndIt;
      txEndIt = txStartIt + rxCombinationsTested;
      std::vector<std::pair<MIMO_SNR_LIST_I, uint16_t>> iter;
      for (uint8_t j = 0; j < nRxAntennas; j++)
        {
          iter.push_back (std::make_pair (txStartIt, 1));
        }

      bool endOfFinalList = false;
      // While we haven't checked all possible combinations
      while (!endOfFinalList)
        {
          MIMO_SNR_LIST combination;
          std::vector<uint16_t> rxAwvIdx;
          bool endOfList = true;
          // For each Rx antenna
          for (auto &iterator : iter)
            {
              // Add the SNR Measurement and the Rx AWV Id of the antenna to the list
              combination.push_back (*(iterator.first));
              rxAwvIdx.push_back (iterator.second);
              // If the previous antenna reached the end of measurements of the current Tx combination move forward
              if (endOfList)
                {
                  iterator.first++;
                  iterator.second++;
                  // If we reach the next Tx combination, go to the start of the current Tx combination and signal to the next iterator to move forward
                  if (iterator.first == txEndIt)
                    {
                      iterator.first = txStartIt;
                      iterator.second = 1;
                      endOfList = true;
                    }
                  // otherwise signal to the next combination to stay at the same position
                  else
                    endOfList = false;
                }
            }
          // If the last antennna reached the end of the current Tx combination, we have finished checking all possible combinations.
          if (endOfList)
            endOfFinalList = true;

          double maxMinSnr;
          bool firstMax = true;
          uint8_t bestTxRxPairIdx = 0; // TO-DO (Check with NINA)
          uint8_t indexPairs = 0;
          /* For this Rx combination check all valid Tx-Rx pairs for the different streams */
          for (auto & validTxRxPair : validTxRxPairs)
            {
              bool firstSnr = true;
              /* Find the minimum SINR of all streams */
              double minSnr = 0; // TO-DO (Check with NINA)
              for (auto txRxPair : validTxRxPair)
                {
                  uint8_t index = (txRxPair + 1) % nRxAntennas;
                  if (index == 0)
                    index = nRxAntennas;
                  if (firstSnr || combination.at (index-1).second.at (txRxPair) < minSnr)
                    {
                      minSnr = combination.at (index-1).second.at (txRxPair);
                      firstSnr = false;
                    }
                }
              /* Find the Tx-Rx pair that gives the maximum minimum SINR */
              if (firstMax || minSnr > maxMinSnr)
                {
                  maxMinSnr = minSnr;
                  bestTxRxPairIdx = indexPairs;
                  firstMax = false;
                }
              indexPairs++;
            }
          /* Save the Tx AWV id and the Rx AWV Ids that correspond to the combination we are currently testing and the minimum SINR associated with it. */
          MEASUREMENT_AWV_IDs measurementAwvId;
          measurementAwvId.first = i+1;
          std::map<RX_ANTENNA_ID, uint16_t> rxAwvIds;
          for (uint8_t k = 0; k < nRxAntennas; k++)
            {
              rxAwvIds[k + 1] = rxAwvIdx.at (k);
            }
          measurementAwvId.second = rxAwvIds;
          minSnr.push (std::make_pair (maxMinSnr, measurementAwvId));
          if (m_suMimoBeamformingTraining)
            {
              SU_MIMO_ANTENNA2ANTENNA antenna2antenna;
              for (auto txRxPair : validTxRxPairs.at (bestTxRxPairIdx))
                {
                  uint16_t txId = std::floor (txRxPair / static_cast<double> (nRxAntennas)) + 1;
                  uint16_t rxId = (txRxPair % nRxAntennas) + 1;
                  antenna2antenna[txId] = rxId;

                }
                antenna2antennaQueue.push (std::make_pair (maxMinSnr, antenna2antenna));
            }
        }
    }
  if (m_suMimoBeamformingTraining)
    {
      m_suMimoMimoPhaseMeasurements (MimoPhaseMeasurementsAttributes (m_peerStation, measurements, minSnr, differentRxCombinations,
                                     nTxAntennas, nRxAntennas, rxCombinationsTested, m_bftIdMap[m_peerStation]), antenna2antennaQueue.top ().second);
    }
  else
    {
      m_muMimoMimoPhaseMeasurements (MimoPhaseMeasurementsAttributes (m_peerStation, measurements, minSnr, differentRxCombinations,
                                     nTxAntennas, nRxAntennas, rxCombinationsTested, m_muMimoBftIdMap [m_edmgMuGroup.groupID]), m_edmgMuGroup.groupID);
    }
  /* Find the top combinations according to the maximum minimum SINR */
  while (bestCombinations.size () != nBestCombinations && !minSnr.empty ())
    {
      /* If we want to feedback multiple combinations with the same Tx AWV ID and different Rx AWV Ids
        * or we don't already have a combination with the given Tx AWV id - add this combination to the best ones */
      if (differentRxCombinations || (std::find (txIds.begin (), txIds.end (), minSnr.top ().second.first) == txIds.end ()))
        {
          txIds.push_back (minSnr.top ().second.first);
          bestCombinations.push_back (minSnr.top ().second);
        }
      NS_LOG_DEBUG (minSnr.top ().first);
      minSnr.pop ();
    }
  return bestCombinations;
}

MIMO_FEEDBACK_COMBINATION
DmgWifiMac::FindOptimalMuMimoConfig (uint8_t nTx, uint8_t nRx, MIMO_FEEDBACK_MAP feedback, std::vector<uint16_t> txAwvIds)
{
  // Find all possible valid combinations of Tx-Rx pairs
  std::vector<std::vector<uint16_t>> validTxRxPairs;
  std::vector<uint16_t> currentCombination;
  std::vector<uint16_t> indexes;
  for (uint16_t i = 0; i < nTx * nRx; i++)
    indexes.push_back (i);
  FindAllValidTxRxPairs (0, nTx, nRx, validTxRxPairs, currentCombination, indexes);
  std::vector<MIMO_FEEDBACK_COMBINATION> candidates;
  std::vector<SNR> minSNRs;
  /* For all Tx configurations we have received feedback */
  for (auto & txAwvId : txAwvIds)
    {
      /* Check all possible combinations of Tx-Rx pairs */
      for (auto & txRxPairs : validTxRxPairs)
        {
          MIMO_FEEDBACK_COMBINATION configs;
          SNR minSNR;
          bool firstConfig = true;
          /* for all streams we want to establish */
          for (auto & txRxPair : txRxPairs)
            {
              /* match the index to the Tx Antenna Id and the responder STA AID */
              uint8_t txId = std::floor (txRxPair / static_cast <double> (nRx));
              uint8_t rxId = (txRxPair % nRx);
              if (rxId == 0)
                rxId = nRx;
              uint8_t txAntennaId = m_codebook->GetCurrentMimoAntennaIdList ().at (txId);
              uint8_t rxAid = m_edmgMuGroup.aidList.at (rxId - 1);
              /* Check if the STA has sent back feedback for this TX configuration */
              MIMO_FEEDBACK_CONFIGURATION config = std::make_tuple (txAntennaId, rxAid, txAwvId);
              MIMO_FEEDBACK_MAP::iterator it = feedback.find (config);
              if (it != feedback.end ())
                {
                  /* if we have feedback add the feedback config and check if it's the stream with the min SINR */
                  configs.push_back (config);
                  if (firstConfig || it->second < minSNR)
                    {
                      minSNR = it->second;
                      firstConfig = false;
                    }
                }
            }
          /* If for this config we have received feedback from all STAs, it's a valid config to choose from*/
          if (configs.size () == txRxPairs.size ())
            {
              candidates.push_back (configs);
              minSNRs.push_back (minSNR);
            }
        }
    }
  NS_ABORT_MSG_IF (minSNRs.empty (), "We have not received full feedback for any candidate so we can not choose the optimal MU-MIMO configuration");
  /* Choose the configuration that gives the maximum minimum per-stream SINR */
  uint16_t maxIdx = std::distance(minSNRs.begin (), std::max_element (minSNRs.begin (), minSNRs.end ()));
  return candidates.at (maxIdx);
}

DATA_COMMUNICATION_MODE
DmgWifiMac::GetStationDataCommunicationMode (Mac48Address station)
{
  DataCommunicationModeTable::iterator it = m_dataCommunicationModeTable.find (station);
  if (it != m_dataCommunicationModeTable.end ())
    return m_dataCommunicationModeTable[station];
  else
    return DATA_MODE_SISO;
}

uint8_t
DmgWifiMac::GetStationNStreams (Mac48Address station)
{
  DataCommunicationModeTable::iterator it = m_dataCommunicationModeTable.find (station);
  if (it != m_dataCommunicationModeTable.end ())
    {
      if (m_dataCommunicationModeTable[station] == DATA_MODE_SISO)
        return 1;
      else if (m_dataCommunicationModeTable[station] == DATA_MODE_SU_MIMO)
        {
          return m_suMimoTxCombinations[station].at (0).size ();
        }
      else
        {
          // To do: For MU-MIMO
          return 1;
        }
    }
  else
    return 1;
}

void
DmgWifiMac::ReportMimoSnrValue (AntennaList patternList, std::vector<double> snr)
{
  NS_LOG_FUNCTION (this);
  if (m_recordTrnSnrValues)
    {
      if ((m_suMimoBfPhase == SU_SISO_INITIATOR_TXSS) || (m_suMimoBfPhase == SU_SISO_RESPONDER_TXSS))
        {
          uint8_t snrIndex = 0;
          for (auto tx_antenna : m_peerAntennaIds)
            {
              for (auto rx_antenna : patternList)
                {
                  /* Add the SNR of the TRN Subfield for a given Tx-Rx antenna configuration */
                  MIMO_CONFIGURATION config = std::make_tuple (m_brpCdown, rx_antenna, tx_antenna);
                  m_suMimoSisoSnrMap[config].push_back (snr.at (snrIndex));
                  m_mimoSisoSnrList.push_back (snr.at (snrIndex));
                  snrIndex++;
                }
            }
        }
      else if ((m_suMimoBfPhase == SU_MIMO_INITIATOR_SMBT) ||
               (m_suMimoBfPhase == SU_MIMO_RESPONDER_SMBT) ||
               (m_muMimoBfPhase == MU_MIMO_BF_TRAINING))
        {
          MIMO_SNR_MEASUREMENT measurement = std::make_pair (m_brpCdown, snr);
          m_mimoSnrList.push_back (measurement);
          for (auto s : snr)
            {
              NS_LOG_DEBUG (RatioToDb (s));
            }
          NS_LOG_DEBUG ("Number of Measurements recorded: " << m_mimoSnrList.size ());
        }
      else
        NS_LOG_ERROR ("Should not be recording SNR values in this phase");
    }
}

void
DmgWifiMac::StartMuMimoBeamforming (bool isInitiatorTxssNeeded, uint8_t edmgGroupId)
{
  /* Check that all necessary conditions are satisfied before starting MU-MIMO BFT */
  NS_LOG_FUNCTION (this << isInitiatorTxssNeeded << uint16_t (edmgGroupId));
  NS_ABORT_MSG_IF (!GetDmgWifiPhy ()->IsMuMimoSupported (), "The initiator EDMG STA needs to support MU-MIMO transmissions");
  if (m_edmgGroupIdSetElement->GetNumberofEDMGGroups () != 0)
    {
      EDMGGroupTuples edmgGroupTuples = m_edmgGroupIdSetElement->GetEDMGGroupTuples ();
      bool foundEdmgGroup = false;
      for (auto const & edmgGroupTuple : edmgGroupTuples)
        if (edmgGroupTuple.groupID == edmgGroupId)
          {
            foundEdmgGroup = true;
            m_edmgMuGroup = edmgGroupTuple;
            break;
          }
      if (!foundEdmgGroup)
         NS_ABORT_MSG_IF (foundEdmgGroup, "The MU group with EDMG Group ID " << uint16_t (edmgGroupId) << " does not exist");
    }
  else
    {
      NS_ABORT_MSG ("An EDMG Group ID Set element needs to be transmitted before starting MU-MIMO BFT");
    }
  NS_ABORT_MSG_IF (m_codebook->GetTotalNumberOfRFChains () == 1,
                   "The initiator EDMG STA needs to have more than one RF Chain in order to perform DL MU-MIMO transmissions.");

  /* Signal to the high and low MAC that MU-MIMO BFT will be begin and that the STA is the initiator */
  m_low->MIMO_BFT_Phase_Started ();
  m_muMimoBeamformingTraining = true;
  m_isMuMimoInitiator = true;
  /* Set the BFT id for the MU group - set it to 0 if it´s the first training or increase it by 1 otherwise. */
  MU_MIMO_BFT_ID_MAP::iterator it = m_muMimoBftIdMap.find (edmgGroupId);
  if (it != m_muMimoBftIdMap.end ())
    {
      uint16_t bftId = it->second + 1;
      m_muMimoBftIdMap [edmgGroupId] = bftId;
    }
  else
    {
      m_muMimoBftIdMap [edmgGroupId] = 0;
    }
  if (isInitiatorTxssNeeded)
    StartMuMimoInitiatorTXSS ();
  else
    StartMuMimoSisoFeedback ();
}

void
DmgWifiMac::StartMuMimoInitiatorTXSS (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("EDMG STA Starting Initiator TXSS as part of the SISO phase of MU-MIMO BFT");

  /* Calculate the correct duration for initiator TXSS and for the SISO Feedback  */
   m_sectorSweepDuration = CalculateShortSectorSweepDuration (m_codebook->GetTotalNumberOfAntennas (),
                                                              m_codebook->GetTotalNumberOfTransmitSectors ());
   m_sisoFbckDuration = CalculateSisoFeedbackDuration ();

  m_muMimoBfPhase = MU_SISO_TXSS;
  //Set up the codebook to start sweeping through all Tx Sectors and DMG antennas
  m_codebook->StartMuMimoInitiatorTXSS ();
  /* Send the first Short SSW frame */
  SendMuMimoInitiatorTxssFrame ();
}

void
DmgWifiMac::SendMuMimoInitiatorTxssFrame (void)
{
  NS_LOG_FUNCTION (this << "CDOWN = " << m_codebook->GetRemaingSectorCount ());
  ShortSSW shortSsw;
  /* Set all fields as specified in the standard */
  shortSsw.SetDirection (BEAMFORMING_INITIATOR);
  shortSsw.SetAddressingMode (GROUP_ADDRESS);
  shortSsw.SetSourceAID (GetAssociationID ());
  shortSsw.SetDestinationAID (m_edmgMuGroup.groupID);
  shortSsw.SetCDOWN (m_codebook->GetRemaingSectorCount ());
  shortSsw.SetRFChainID (m_codebook->GetActiveRFChainID ());
  shortSsw.SetSisoFbckDuration (m_sisoFbckDuration);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (shortSsw);

  /* Add the BFT ID tag for the MU group */
  BftIdTag tag;
  tag.Set (m_muMimoBftIdMap [m_edmgMuGroup.groupID]);
  packet->AddPacketTag (tag);

  NS_LOG_INFO ("Sending short SSW Frame " << Simulator::Now () << " with AntennaID="
               << static_cast<uint16_t> (m_codebook->GetActiveAntennaID ())
               << ", SectorID=" << static_cast<uint16_t> (m_codebook->GetActiveTxSectorID ()));

  TransmitShortSswFrame (packet);
}

void
DmgWifiMac::StartMuMimoSisoFeedback (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("Initiating SISO Feedback phase of MU-MIMO BFT with MU group " << uint16_t (m_edmgMuGroup.groupID));
  m_muMimoBfPhase = MU_SISO_FBCK;
  m_currentMuGroupMember = m_edmgMuGroup.aidList.begin ();
  m_edmgTrnM = 0;
  m_peerLTxRx = 0;
  m_muMimoFeedbackMap.clear ();
  SendBrpFbckPollFrame ();
}

void
DmgWifiMac::SendBrpFbckPollFrame (void)
{
  NS_LOG_FUNCTION (this);

  Mac48Address receiver = m_aidMap[*m_currentMuGroupMember];
  NS_LOG_LOGIC ("Sending BRP frame asking for feedback to EDMG STA " << receiver);

  BeamRefinementElement element;
  element.SetAsBeamRefinementInitiator (true);
  element.SetBfTrainingType (MU_MIMO_BF);
  element.SetTxssFbckReq (true);
  element.SetSnrRequested (true);
  element.SetSectorIdOrderRequested (true);
  element.SetChannelMeasurementPresent (false);
  element.SetEdmgChannelMeasurementPresent (false);

  BRP_Request_Field requestField;
  /* Currently, we do not support MID + BC Subphases */
  requestField.SetMID_REQ (false);
  requestField.SetBC_REQ (false);
  SendEmptyBrpFrame (receiver, requestField, element);
}

void
DmgWifiMac::SendBrpFbckFrame (Mac48Address station, bool useAwvsinMimoPhase)
{
  NS_LOG_FUNCTION (this << station << useAwvsinMimoPhase);
  m_muMimoSisoPhaseMeasurements (station, m_muMimoSisoSnrMap, m_edmgMuGroup.groupID, m_muMimoBftIdMap [m_edmgMuGroup.groupID]);
  BeamRefinementElement element;
  element.SetBfTrainingType (MU_MIMO_BF);
  element.SetSnrPresent (true);
  element.SetSectorIdOrderPresent (true);
  element.SetLinkType (false);
  element.SetEdmgExtensionFlag (true);
  element.SetEdmgChannelMeasurementPresent (true);

  BRP_Request_Field requestField;
  /* Currently, we do not support MID + BC Subphases */
  requestField.SetMID_REQ (false);
  requestField.SetBC_REQ (false);

  EdmgBrpRequestElement edmgRequestElement;
  /* Calculate the number of units needed for receive training in the MIMO phase of MU-MIMO BFT.
   * For now we train all sectors or sectors + AWVs - since we didn´t do any UL training we can´t choose candidates. */
  m_codebook->SetUseAWVsMimoBft (useAwvsinMimoPhase);
  m_rxCombinationsTested = m_codebook->GetTotalNumberOfReceiveSectorsOrAWVs ();
  /* If we need more than 16 subfields we need multiple units */
  if (m_rxCombinationsTested > 16)
    {
      NS_ABORT_MSG_IF (ceil (m_rxCombinationsTested / 16.0) > 255, "Number of requested TRN Units is too large");
      m_lTxRx = uint8_t (ceil (m_rxCombinationsTested / 16.0));
      m_edmgTrnMRequested = uint8_t (std::ceil (static_cast <double> (m_rxCombinationsTested) / m_lTxRx));
    }
  else
    {
      m_edmgTrnMRequested = m_rxCombinationsTested;
      m_lTxRx = 1;
    }
  edmgRequestElement.SetL_TX_RX (m_lTxRx);
  edmgRequestElement.SetRequestedEDMG_TRN_UnitM (m_edmgTrnMRequested);
  /* Add a Channel Measurement Feedback Element */
  std::vector<ChannelMeasurementFeedbackElement> channelElements;
  /* Add an EDMG Channel Measurement Feedback Element */
  std::vector<EDMGChannelMeasurementFeedbackElement> edmgChannelElements;

  // If the last initiator TXSS was performed using Short SSW frames
  if (!m_muMimoSisoSnrMap.empty ())
    {
      element.SetSswFrameType (SHORT_SSW_FRAME);
      std::sort (m_mimoSisoSnrList.begin (), m_mimoSisoSnrList.end ());
      /* To make sure that the size of the packet payload is below the maximum size specified in the standard
       * for DMG CTRL mode (1023 Bytes),the maximum amount of measurements that we can feedback is 189. Therefore,
       * we only feedback the highest 189 measurements */
      double minSnr;
      if (m_mimoSisoSnrList.size () > 190)
        {
          minSnr = m_mimoSisoSnrList.at (m_mimoSisoSnrList.size () - 189 - 1);
        }
      else
        {
          minSnr = m_mimoSisoSnrList.front () - 0.1;
        }
      uint16_t numberOfMeasurments = 0;
      uint8_t numberOfMeasurmentsElement = 0;
      Ptr<ChannelMeasurementFeedbackElement> channelElement = Create<ChannelMeasurementFeedbackElement> ();
      Ptr<EDMGChannelMeasurementFeedbackElement> edmgChannelElement = Create<EDMGChannelMeasurementFeedbackElement> ();
      /* Fill in the feedback in Channel Measurement Feedback and EDMG Channel Measurement Feedback Elements. The maximum size of the
       * information elements is 255 bytes which corresponds to 63 measurements, therefore if we have more than 63 measurements, we need to split
       * the feedback into multiple Channel Measurement Feedback and EDMG Channel Measurement Feedback Elements. */
      for (MU_MIMO_SNR_MAP::iterator it = m_muMimoSisoSnrMap.begin (); it != m_muMimoSisoSnrMap.end (); it++)
        {
          if (numberOfMeasurmentsElement == 63)
            {
              numberOfMeasurmentsElement = 0;
              channelElements.push_back (*channelElement);
              edmgChannelElements.push_back (*edmgChannelElement);
              channelElement = Create<ChannelMeasurementFeedbackElement> ();
              edmgChannelElement = Create<EDMGChannelMeasurementFeedbackElement> ();
            }
          if (it->second > minSnr)
            {
              uint8_t snr = MapSnrToInt (it->second);
              channelElement->AddSnrItem (snr);
              EDMGSectorIDOrder order;
              order.RXAntennaID = std::get<1> (it->first);
              order.TXAntennaID = std::get<2> (it->first);
              order.SectorID = std::get<0> (it->first);
              edmgChannelElement->Add_EDMG_SectorIDOrder (order);
              numberOfMeasurmentsElement++ ;
              numberOfMeasurments++ ;
            }
        }
      channelElements.push_back (*channelElement);
      edmgChannelElements.push_back (*edmgChannelElement);
      element.SetExtendedNumberOfMeasurements (numberOfMeasurments);
    }
  else
    {
      // If the last initiator TXSS was performed using SSW frames or Beacon frames
      STATION_SNR_PAIR_MAP::iterator it = m_stationSnrMap.find (station);
      if (it != m_stationSnrMap.end ())
        {
          element.SetSswFrameType (SSW_FRAME);
          for (SNR_MAP::iterator it1 = it->second.first.begin (); it1 != it->second.first.end (); it1++)
            {
              m_mimoSisoSnrList.push_back (it1->second);
            }
          std::sort (m_mimoSisoSnrList.begin (), m_mimoSisoSnrList.end ());
          /* To make sure that the size of the packet payload is below the maximum size specified in the standard
           * for DMG CTRL mode (1023 Bytes), the maximum amount of measurements that we can feedback is 189. Therefore,
           * we only feedback the highest 189 measurements */
          double minSnr;
          if (m_mimoSisoSnrList.size () > 190)
            {
              minSnr = m_mimoSisoSnrList.at (m_mimoSisoSnrList.size () - 189 - 1);
            }
          else
            {
              minSnr = m_mimoSisoSnrList.front () - 0.1;
            }
          uint16_t numberOfMeasurments = 0;
          uint8_t numberOfMeasurmentsElement = 0;
          Ptr<ChannelMeasurementFeedbackElement> channelElement = Create<ChannelMeasurementFeedbackElement> ();
          Ptr<EDMGChannelMeasurementFeedbackElement> edmgChannelElement = Create<EDMGChannelMeasurementFeedbackElement> ();
          /* Fill in the feedback in Channel Measurement Feedback and EDMG Channel Measurement Feedback Elements. The maximum size of the
           * information elements is 255 bytes which corresponds to 63 measurements, therefore if we have more than 63 measurements, we need to split
           * the feedback into multiple Channel Measurement Feedback and EDMG Channel Measurement Feedback Elements. */
          for (SNR_MAP::iterator it1 = it->second.first.begin (); it1 != it->second.first.end (); it1++)
            {
              if (numberOfMeasurmentsElement == 63)
                {
                  numberOfMeasurmentsElement = 0;
                  channelElements.push_back (*channelElement);
                  edmgChannelElements.push_back (*edmgChannelElement);
                  channelElement = Create<ChannelMeasurementFeedbackElement> ();
                  edmgChannelElement = Create<EDMGChannelMeasurementFeedbackElement> ();
                }
              if (it1->second > minSnr)
                {
                  uint8_t snr = MapSnrToInt (it1->second);
                  channelElement->AddSnrItem (snr);
                  EDMGSectorIDOrder order;
                  order.RXAntennaID = std::get<0> (it1->first);
                  order.TXAntennaID = std::get<1> (it1->first);
                  order.SectorID = std::get<2> (it1->first);
                  edmgChannelElement->Add_EDMG_SectorIDOrder (order);
                  numberOfMeasurmentsElement++ ;
                  numberOfMeasurments++ ;
                }
            }
          channelElements.push_back (*channelElement);
          edmgChannelElements.push_back (*edmgChannelElement);
          element.SetExtendedNumberOfMeasurements (numberOfMeasurments);
        }
      else
        {
          NS_LOG_INFO ("There is no previous initiator TXSS");
        }
    }
  SendFeedbackMimoBrpFrame (station, requestField, element, &edmgRequestElement, channelElements, edmgChannelElements);
}

void
DmgWifiMac::StartMuMimoMimoPhase (MIMO_ANTENNA_COMBINATIONS_LIST candidates, bool useAwvs)
{
  NS_LOG_FUNCTION (this << useAwvs);
  // Note: For now we assume that only one antenna is connected to each RF Chain - all candidates have the same antenna combination
  // Create the lists of antenna combinations and candidate sectors per antenna.
  Antenna2SectorList candidateSectors;
  std::vector<AntennaID> candidateAntennas;
  for (MIMO_ANTENNA_COMBINATIONS_LIST_I it = candidates.begin (); it != candidates.end (); it++)
    {
      for (MIMO_ANTENNA_COMBINATION::iterator it1 = it->begin (); it1 != it->end (); it1++)
        {
          Antenna2SectorListI iter = candidateSectors.find(it1->first);
          if (iter != candidateSectors.end ())
            {
              iter->second.push_back(it1->second);
            }
          else
            {
              SectorIDList sectors;
              sectors.push_back (it1->second);
              candidateSectors[it1->first] = sectors;
              candidateAntennas.push_back (it1->first);
            }
        }
    }
  m_muMimomMimoCandidatesSelected (m_edmgMuGroup.groupID, candidateSectors, m_muMimoBftIdMap [m_edmgMuGroup.groupID]);
  /* Create a MIMO configuration to be used when transmitting the packet to multiple stations using spatial expansion. For now we assume that
   * the number of stations being trained is equal to the number of antennas being trained and we steer each antenna toward the optimal sector for
   * one station from the group. Note that if we want to train more stations than antennas this will not work. Also, we assume that the given sector is
   * optimal for all antennas since we don´t save optimal sector per antenna (needs to be done in the future). */
  uint8_t index = 0;
  for (auto & antenna: candidateAntennas)
    {
      ANTENNA_CONFIGURATION_TX antennaConfigTx = std::get<0> (m_bestAntennaConfig[m_aidMap[m_edmgMuGroup.aidList.at (index)]]);
      ANTENNA_CONFIGURATION config = std::make_pair (antenna, antennaConfigTx.second);
      AWV_CONFIGURATION pattern = std::make_pair (config, NO_AWV_ID);
      m_mimoConfigTraining.push_back (pattern);
      index++;
    }
  /* Set up the codebook with the lists of candidates that we want try when transmitting the MIMO TRN subfilds */
  m_codebook->SetUseAWVsMimoBft (useAwvs);
  m_codebook->SetUpMuMimoSectorSweeping (GetAddress (), candidateAntennas, candidateSectors);
  /* In the MIMO BF Subphase we send the minimum MIMO setup frames necessary to reach all responders.
   * For now we send a frame using the optimal sector for each station - this can be lowered by finding multiple stations that can all
   * receive correctly when using the same Tx sector. */
  m_currentMuGroupMember = m_edmgMuGroup.aidList.begin ();
  m_muMimoBfPhase = MU_MIMO_BF_SETUP;
  Simulator::Schedule (m_mbifs, &DmgWifiMac::SendMuMimoSetupFrame, this);
}

void
DmgWifiMac::SendMuMimoSetupFrame (void)
{
  NS_LOG_FUNCTION (this);
  MimoSetupControlElement setupElement;
  setupElement.SetMimoBeamformingType (MU_MIMO_BEAMFORMING);
  // Currently we only support non-reciprocal MIMO phase
  setupElement.SetMimoPhaseType (MIMO_PHASE_NON_RECPIROCAL);
  setupElement.SetAsInitiator (true);
  setupElement.SetEDMGGroupID (m_edmgMuGroup.groupID);
  setupElement.SetGroupUserMask (GenerateEdmgMuGroupMask ());
  // Ask for time domain channel response
//  setupElement.SetChannelMeasurementRequested (true);
//  setupElement.SetNumberOfTapsRequested ();

  // Send MIMO BF Setup frame
  Mac48Address to;
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION_NO_ACK);
  hdr.SetAddr1 (to.GetBroadcast ());
  hdr.SetAddr2 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();
  hdr.SetFragmentNumber (0);
  hdr.SetNoMoreFragments ();
  hdr.SetNoRetry ();

  ExtMimoBfSetupFrame setupFrame;
  setupFrame.SetMimoSetupControlElement (setupElement);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.unprotectedAction = WifiActionHeader::UNPROTECTED_MIMO_BF_SETUP;
  actionHdr.SetAction (WifiActionHeader::UNPROTECTED_DMG, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (setupFrame);
  packet->AddHeader (actionHdr);

  /* For now we transmit as many setup frames as there are users in the MIMO group using the optimal sectors for each user.
   * Should be optimized in the future if multiple STAs can receive frames send with the same Tx sector. */
  /* MIMO BF setup packets are send with spatial expansion and mapping a single stream across all transmit chains */
  m_codebook->SetCommunicationMode (MIMO_MODE);
  for (auto const &antenna : m_codebook->GetTotalAntennaIdList ())
    {
      ANTENNA_CONFIGURATION_TX antennaConfigTx = std::get<0> (m_bestAntennaConfig[m_aidMap[*m_currentMuGroupMember]]);
      ANTENNA_CONFIGURATION config = std::make_pair (antenna, antennaConfigTx.second);
      AWV_CONFIGURATION pattern = std::make_pair (config, NO_AWV_ID);
      NS_LOG_DEBUG ("Activate Transmit Antenna with AntennaID=" << static_cast<uint16_t> (pattern.first.first)
                        << ", to SectorID=" << static_cast<uint16_t> (pattern.first.second)
                        << ", AwvID=" << static_cast<uint16_t> (pattern.second));
      m_codebook->SetActiveTxSectorID (pattern.first.first, pattern.first.second);
      if (pattern.second != NO_AWV_ID)
        {
          m_codebook->SetActiveTxAwvID (pattern.second);
        }
    }

  NS_LOG_INFO ("Sending broadcast MIMO BF Setup frame at " << Simulator::Now ());

  /* Transmit control frames directly without DCA + DCF Manager */
  TransmitControlFrameImmediately (packet, hdr, MicroSeconds (0));
}

uint32_t
DmgWifiMac::GenerateEdmgMuGroupMask (void)
{
  NS_LOG_FUNCTION (this);
  uint32_t muGroupMask = 0;
  uint32_t bitNumber = 0;
  // Set the bit conected to each STA in the MU group to 1 if it's included in the MIMO phase training
  for (auto staAid : m_edmgMuGroup.aidList)
    {
      MU_GROUP_MAP::iterator it = m_edmgMuGroupMap.find (staAid);
      if ((it != m_edmgMuGroupMap.end ()) && (m_edmgMuGroupMap[staAid] == true))
        {
          muGroupMask |= (1 & 0x1) << bitNumber;
        }
      bitNumber++;
    }
  return muGroupMask;
}

userMaskConfig
DmgWifiMac::IsIncludedInUserGroup (uint32_t groupUserMaskField)
{
  NS_LOG_FUNCTION (this << groupUserMaskField);
  uint32_t bitNumber = 0;
  uint8_t numUser = 0;
  userMaskConfig config;
  for (auto staAid : m_edmgMuGroup.aidList)
    {
      // Count the number of STAs
      uint8_t isIncluded = (groupUserMaskField >> bitNumber) & 0x1;
      if (isIncluded == 1)
        numUser++;
      if (staAid == GetAssociationID ())
        {
          if (isIncluded == 1)
            {
              config.first = true;
              config.second = numUser;
              return config;
            }
          else
            {
              config.first = false;
              return config;
            }
        }
      bitNumber++;
    }
  NS_ABORT_MSG ("Station is not a part of MU group that is training");
}

Ptr<EDMGGroupIDSetElement>
DmgWifiMac::GetEdmgGroupIdSetElement (void) const
{
  return m_edmgGroupIdSetElement;
}

void
DmgWifiMac::StartMuMimoBfTrainingSubphase (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("DMG STA Starting MU MIMO BF Training");

  /* To do:Calculate the correct duration for SMBT (or full MIMO Phase?) */
//  m_sectorSweepDuration = CalculateSectorSweepDuration (m_peerAntennas, m_codebook->GetTotalNumberOfAntennas (),
//                                                        m_codebook->GetTotalNumberOfTransmitSectors ());

  //Set up the lists of sectors that will be tested for each antenna in this MIMO BRP Packet
  bool firstCombination = true;
  m_codebook->InitializeMimoSectorSweeping (GetAddress (), TransmitSectorSweep, firstCombination);
  m_muMimoBfPhase = MU_MIMO_BF_TRAINING;
  GetDmgWifiPhy ()->SetMuMimoBeamformingTraining (true);
  // Count the number of packets according to the number of Units needed to test all Tx and Rx combinations - if we are testing AWVs we test all possible combinations.
  m_numberOfUnitsRemaining = (m_codebook->CountMimoNumberOfTxSubfields (GetAddress ()) * m_peerLTxRx) ;
  NS_ABORT_MSG_IF ((std::ceil (m_numberOfUnitsRemaining / 255.0) - 1) > 63, "Number of BRP packets needed is too large");
  m_brpCdown = (std::ceil (m_numberOfUnitsRemaining / 255.0)) - 1;
  SendMimoBfTrainingBrpFrame (GetAddress ());
}

void
DmgWifiMac::StartMuMimoBfFeedbackSubphase (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("Initiating MIMO Feedback phase of MU-MIMO BFT with MU group " << m_edmgMuGroup.groupID);
  m_muMimoBfPhase = MU_MIMO_BF_FBCK;
  // Poll all stations that participated in the MIMO phase for feedback.
  m_currentMuGroupMember = m_edmgMuGroup.aidList.begin ();
  bool foundResponder = false;
  while (!foundResponder)
    {
      MU_GROUP_MAP::iterator it = m_edmgMuGroupMap.find (*m_currentMuGroupMember);
      if (it != m_edmgMuGroupMap.end () && (m_edmgMuGroupMap[*m_currentMuGroupMember] == true))
        foundResponder = true;
      else
        {
          m_currentMuGroupMember++;
          if (m_currentMuGroupMember == m_edmgMuGroup.aidList.end ())
            break;
        }
    }
  m_muMimoFeedbackMap.clear ();
  SendMimoBfPollFrame ();
}

void
DmgWifiMac::SendMimoBfPollFrame (void)
{
  NS_LOG_FUNCTION (this);
  Mac48Address receiver = m_aidMap[*m_currentMuGroupMember];
  NS_LOG_LOGIC ("Sending MIMO Poll frame asking for feedback to EDMG STA " << receiver);

  MimoPollControlElement element;
  element.SetMimoBeamformingType (MU_MIMO_BEAMFORMING);
  element.SetPollType (POLL_MIMO_BF_FEEDBACK);

  // Send MIMO BF Poll frame
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION_NO_ACK);
  hdr.SetAddr1 (receiver);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();
  hdr.SetFragmentNumber (0);
  hdr.SetNoMoreFragments ();
  hdr.SetNoRetry ();

  ExtMimoBfPollFrame pollFrame;
  pollFrame.SetMimoPollControlElement (element);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.unprotectedAction = WifiActionHeader::UNPROTECTED_MIMO_BF_POLL;
  actionHdr.SetAction (WifiActionHeader::UNPROTECTED_DMG, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (pollFrame);
  packet->AddHeader (actionHdr);

  /* Set the best sector for transmission with this station */
  /* MIMO BF poll packets are send with spatial expansion and mapping a single stream across all transmit chains */
  m_codebook->SetCommunicationMode (MIMO_MODE);
  for (auto const &txConfig : m_mimoConfigTraining)
    {
      NS_LOG_DEBUG ("Activate Transmit Antenna with AntennaID=" << static_cast<uint16_t> (txConfig.first.first)
                        << ", to SectorID=" << static_cast<uint16_t> (txConfig.first.second)
                        << ", AwvID=" << static_cast<uint16_t> (txConfig.second));
      m_codebook->SetActiveTxSectorID (txConfig.first.first, txConfig.first.second);
      if (txConfig.second != NO_AWV_ID)
        {
          m_codebook->SetActiveTxAwvID (txConfig.second);
        }
    }

  NS_LOG_INFO ("Sending MIMO BF Poll frame to " << receiver << " at " << Simulator::Now ());

  /* Transmit control frames directly without DCA + DCF Manager */
  TransmitControlFrameImmediately (packet, hdr, MicroSeconds (0));
}

void
DmgWifiMac::SendMuMimoBfFeedbackFrame (Mac48Address station)
{
  NS_LOG_FUNCTION (this << station);
  m_peerStation = station;
  ExtMimoBfFeedbackFrame feedbackFrame;
  MIMOFeedbackControl feedbackElement;
  feedbackElement.SetMimoBeamformingType (MU_MIMO_BEAMFORMING);
  feedbackElement.SetLinkTypeAsInitiator (true);
  feedbackElement.SetComebackDelay (0);
  feedbackElement.SetChannelMeasurementPresent (m_timeDomainChannelResponseRequested);
  if (m_timeDomainChannelResponseRequested)
    {
      feedbackElement.SetNumberOfTapsPresent (m_numberOfTapsRequested);
      // To do: add time domain channel measurement
    }
  // if the number of Tx combinations tested in the MIMO phase is less than 64 give feedback for all the Tx combination
  // - if not give feedback for the top 64 combinations.
  uint8_t numberOfTxCombinationsTested = std::ceil((m_mimoSnrList.size ()) / static_cast<double> (m_rxCombinationsTested));
  uint8_t nBestCombinations;
  if (numberOfTxCombinationsTested > 64)
    nBestCombinations = 64;
  else
    nBestCombinations = numberOfTxCombinationsTested;
  uint8_t nTxAntennas = m_peerAntennaIds.size ();
  uint8_t nRxAntennas = m_codebook->GetCurrentMimoAntennaIdList ().size ();
  // Find the top Tx combinations tested
  BEST_TX_COMBINATIONS_AWV_IDS bestCombinations = FindBestTxCombinations (nBestCombinations, m_rxCombinationsTested,
                                                                          nTxAntennas, nRxAntennas, m_mimoSnrList, false);
  feedbackElement.SetNumberOfTXSectorCombinationsPresent (nBestCombinations);
  feedbackElement.SetNumberOfTxAntennas (nTxAntennas);
  feedbackElement.SetNumberOfRxAntennas (nRxAntennas);

  Ptr<ChannelMeasurementFeedbackElement> channelElement = Create<ChannelMeasurementFeedbackElement> ();
  Ptr<EDMGChannelMeasurementFeedbackElement> edmgChannelElement = Create<EDMGChannelMeasurementFeedbackElement> ();
  uint8_t numberOfMeasurementsElement = 0;
  uint16_t sisoIdSubsetIndex = 0;
  for (BEST_TX_COMBINATIONS_AWV_IDS::iterator it = bestCombinations.begin (); it != bestCombinations.end (); it++)
    {
      uint16_t txId = it->first;
      MIMO_SNR_LIST measurements;
      for (auto & rxId : it->second)
        {
          measurements.push_back (m_mimoSnrList.at ((txId - 1) * m_rxCombinationsTested + rxId.second - 1));
        }
      /* Check that the BRP CDOWN of all measurements matches - check that all measurements are from the same Tx IDx. */
      BRP_CDOWN firstBrpCdown = measurements.at (0).first;
      for (auto & measurement : measurements)
        {
          if (measurement.first != firstBrpCdown)
            {
              NS_ABORT_MSG ("Measurements must have the same BRP index since they must be connected to the same Tx config");
            }
        }
      /* Calculate the index of Tx Combination taking into account the BRP CDOWN of the packet it was received in */
      uint16_t indexAdjust = 0;
      for (auto &measurement : m_mimoSnrList)
        {
          if (measurement.first > measurements.at (0).first)
            indexAdjust++;
        }
      txId = txId - (indexAdjust / m_rxCombinationsTested);
      uint8_t snrIndex = 0;
      for (auto tx_antenna : m_peerAntennaIds)
        {
          uint8_t rxIndex = 0;
          for (auto rx_antenna : m_codebook->GetCurrentMimoAntennaIdList ())
            {
              /* Map the Idx of the measurement that is being fed back to the SISO ID Subset index -
               * to be able to find the correct Rx config in the selection subphase. */
              SNR_MEASUREMENT_INDEX measurementIdx = std::make_pair (it->second[rxIndex + 1], snrIndex);
              m_sisoIdSubsetIndexRxMap [sisoIdSubsetIndex] = measurementIdx;
              sisoIdSubsetIndex++;
              uint8_t snr = MapSnrToInt (measurements.at (rxIndex).second.at (snrIndex));
              channelElement->AddSnrItem (snr);
              EDMGSectorIDOrder order;
              order.RXAntennaID = rx_antenna;
              order.TXAntennaID = tx_antenna;
              order.SectorID = txId;
              edmgChannelElement->Add_EDMG_SectorIDOrder (order);
              edmgChannelElement->Add_BRP_CDOWN (measurements.at (rxIndex).first);
              snrIndex++;
              rxIndex++;
            }
        }
      numberOfMeasurementsElement+= snrIndex;
      if (numberOfMeasurementsElement + snrIndex > 63)
        {
          numberOfMeasurementsElement = 0;
          feedbackFrame.AddChannelMeasurementFeedbackElement (channelElement);
          feedbackFrame.AddEdmgChannelMeasurementFeedbackElement (edmgChannelElement);
          channelElement = Create<ChannelMeasurementFeedbackElement> ();
          edmgChannelElement = Create<EDMGChannelMeasurementFeedbackElement> ();
        }
    }

  // Send MIMO BF Setup frame
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION_NO_ACK);
  hdr.SetAddr1 (station);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();
  hdr.SetFragmentNumber (0);
  hdr.SetNoMoreFragments ();
  hdr.SetNoRetry ();

  feedbackFrame.SetMimoFeedbackControlElement (feedbackElement);
  feedbackFrame.AddChannelMeasurementFeedbackElement (channelElement);
  feedbackFrame.AddEdmgChannelMeasurementFeedbackElement (edmgChannelElement);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.unprotectedAction = WifiActionHeader::UNPROTECTED_MIMO_BF_FEEDBACK;
  actionHdr.SetAction (WifiActionHeader::UNPROTECTED_DMG, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (feedbackFrame);
  packet->AddHeader (actionHdr);

  /* Set the best sector for transmission with this station */
  /* MIMO BF Feedback packets are send with spatial expansion and mapping a single stream across all transmit chains */
  m_codebook->SetCommunicationMode (MIMO_MODE);
  for (auto const &txConfig : m_mimoConfigTraining)
    {
      NS_LOG_DEBUG ("Activate Transmit Antenna with AntennaID=" << static_cast<uint16_t> (txConfig.first.first)
                        << ", to SectorID=" << static_cast<uint16_t> (txConfig.first.second)
                        << ", AwvID=" << static_cast<uint16_t> (txConfig.second));
      m_codebook->SetActiveTxSectorID (txConfig.first.first, txConfig.first.second);
      if (txConfig.second != NO_AWV_ID)
        {
          m_codebook->SetActiveTxAwvID (txConfig.second);
        }
    }

  NS_LOG_INFO ("Sending MIMO BF Feedback frame to " << station << " at " << Simulator::Now ());
  /* Transmit control frames directly without DCA + DCF Manager */
  TransmitControlFrameImmediately (packet, hdr, MicroSeconds (0));
}

void
DmgWifiMac::StartMuMimoSelectionSubphase (void)
{
  NS_LOG_FUNCTION (this);
  m_muMimoTxCombinations.erase (m_edmgMuGroup.groupID);
  /* Select the optimal comfiguration for MU MIMO */
  MIMO_FEEDBACK_COMBINATION optimalConfigs = FindOptimalMuMimoConfig (m_codebook->GetCurrentMimoAntennaIdList ().size (),
                                                                      m_edmgMuGroup.aidList.size (),
                                                                      m_muMimoFeedbackMap, m_txAwvIdList);
  /* For each STA in the MU group save the SISO IS Subset Index that corresponds to the optimal MU MIMO config that will be chosen. */
  MU_MIMO_ANTENNA2RESPONDER antenna2responder;
  for (auto & aid : m_edmgMuGroup.aidList)
    {
      for (auto & config : optimalConfigs)
        {
          if (std::get<1> (config) == aid)
            {
              m_sisoIdSubsetIndexList.push_back (m_sisoIdSubsetIndexMap[config]);
              antenna2responder[std::get<0> (config)] = m_aidMap[aid];
            }
        }
    }
  uint16_t txId = std::get<2> (optimalConfigs.at (0));
  /* Find and save the optimal MIMO Tx Configuration for future use */
  MIMO_AWV_CONFIGURATION txCombination = m_codebook->GetMimoConfigFromTxAwvId (txId, GetAddress ());
  m_muMimoOptimalConfig (txCombination, m_edmgMuGroup.groupID, m_muMimoBftIdMap [m_edmgMuGroup.groupID], antenna2responder, true);
  BEST_ANTENNA_MU_MIMO_COMBINATIONS::iterator it1 = m_muMimoTxCombinations.find (m_edmgMuGroup.groupID);
  if (it1 != m_muMimoTxCombinations.end ())
    {
      MIMO_AWV_CONFIGURATIONS *txConfigs = &(it1->second);
      txConfigs->push_back (txCombination);
    }
  else
    {
      MIMO_AWV_CONFIGURATIONS txConfigs;
      txConfigs.push_back (txCombination);
      m_muMimoTxCombinations[m_edmgMuGroup.groupID] = txConfigs;
    }
  /* Start sending MIMO Selection frames to reach all the responders in the MU group and inform them of the optimal MU MIMO config chosen. */
  m_muMimoBfPhase = MU_MIMO_BF_SELECTION;
  m_currentMuGroupMember = m_edmgMuGroup.aidList.begin ();
  SendMuMimoBfSelectionFrame ();
}

void
DmgWifiMac::SendMuMimoBfSelectionFrame (void)
{
  NS_LOG_FUNCTION (this);
  MIMOSelectionControlElement element;
  element.SetMultiUserTransmissionConfigurationType (MU_NonReciprocal);
  element.SetEDMGGroupID (m_edmgMuGroup.groupID);
  /* For now we only select one MU configuration  to be used and only have one user per antenna in the configuration */
  element.SetNumberOfMultiUserConfigurations (1);
  /* Tell each user from the MU Group the SISO ID Subset Index that corresponds to the optimal MU MIMO Config. */
  for (uint8_t i = 0; i < m_edmgMuGroup.aidList.size (); i++)
    {
       uint32_t muGroupMask = (1 & 0x1) << i;
       NonReciprocalTransmissionConfig config;
       config.nonReciprocalConfigGroupUserMask = muGroupMask;
       config.configList.push_back (m_sisoIdSubsetIndexList.at (i));
       element.AddNonReciprocalMUBFTrainingBasedTransmissionConfig (config);
    }

  // Send MIMO BF Selection frame
  Mac48Address to;
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION_NO_ACK);
  hdr.SetAddr1 (to.GetBroadcast ());
  hdr.SetAddr2 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();
  hdr.SetFragmentNumber (0);
  hdr.SetNoMoreFragments ();
  hdr.SetNoRetry ();

  ExtMimoBFSelectionFrame selectionFrame;
  selectionFrame.SetMIMOSelectionControlElement (element);
  selectionFrame.SetEDMGGroupIDSetElement (*m_edmgGroupIdSetElement);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.unprotectedAction = WifiActionHeader::UNPROTECTED_MIMO_BF_SELECTION;
  actionHdr.SetAction (WifiActionHeader::UNPROTECTED_DMG, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (selectionFrame);
  packet->AddHeader (actionHdr);

  /* For now we transmit as many setup frames as there are users in the MIMO group using the optimal sectors for each user.
   * Should be optimized in the future if multiple STAs can receive frames send with the same Tx sector. */
  /* MIMO BF Selection packets are send with spatial expansion and mapping a single stream across all transmit chains */
  m_codebook->SetCommunicationMode (MIMO_MODE);
  for (auto const &antenna : m_codebook->GetCurrentMimoAntennaIdList ())
    {
      ANTENNA_CONFIGURATION_TX antennaConfigTx = std::get<0> (m_bestAntennaConfig[m_aidMap[*m_currentMuGroupMember]]);
      ANTENNA_CONFIGURATION config = std::make_pair (antenna, antennaConfigTx.second);
      AWV_CONFIGURATION pattern = std::make_pair (config, NO_AWV_ID);
      NS_LOG_DEBUG ("Activate Transmit Antenna with AntennaID=" << static_cast<uint16_t> (pattern.first.first)
                        << ", to SectorID=" << static_cast<uint16_t> (pattern.first.second)
                        << ", AwvID=" << static_cast<uint16_t> (pattern.second));
      m_codebook->SetActiveTxSectorID (pattern.first.first, pattern.first.second);
      if (pattern.second != NO_AWV_ID)
        {
          m_codebook->SetActiveTxAwvID (pattern.second);
        }
    }

  NS_LOG_INFO ("Sending broadcast MIMO BF Selection frame at " << Simulator::Now ());

  /* Transmit control frames directly without DCA + DCF Manager */
  TransmitControlFrameImmediately (packet, hdr, MicroSeconds (0));
}

void
DmgWifiMac::MuMimoBFTFailed (void)
{
  NS_LOG_FUNCTION (this);
  m_muMimoBfPhase = MU_WAIT_MU_MIMO_BF_TRAINING;
  m_low->MIMO_BFT_Phase_Ended ();
  m_muMimoBeamformingTraining = false;
}

void
DmgWifiMac::RegisterMuMimoSisoFbckPolled (Mac48Address from)
{
  NS_LOG_FUNCTION (this << from);
  m_muMimoSisoFbckPolled (from);
}

void
DmgWifiMac::RegisterMuMimoSisoPhaseComplete (MIMO_FEEDBACK_MAP muMimoFbckMap, uint8_t nRFChains, uint8_t nStas, uint8_t muGroup, uint16_t bftID)
{
  m_muMimoSisoPhaseComplete (muMimoFbckMap, nRFChains, nStas, muGroup, bftID);
}
//// NINA ////

Time
DmgWifiMac::GetSectorSweepDuration (uint8_t sectors)
{
  if (m_isEdmgSupported)
    {
      return ((edmgSswTxTime) * sectors + GetSbifs () * (sectors - 1));
    }
  else
    {
      return ((sswTxTime) * sectors + GetSbifs () * (sectors - 1));
    }
}

Time
DmgWifiMac::GetSectorSweepSlotTime (uint8_t fss)
{
  Time sswFbck;
  if (m_isEdmgSupported)
    {
      sswFbck = edmgSswFbckTxTime;
    }
  else
    {
      sswFbck = sswFbckTxTime;
    }
  Time time;
  time = aAirPropagationTime
       + GetSectorSweepDuration (fss)  /* aSSDuration */
       + sswFbck
       + GetMbifs () * 2;
  time = MicroSeconds (ceil ((double) time.GetNanoSeconds () / 1000));
  return time;
}

Time
DmgWifiMac::CalculateSectorSweepDuration (uint8_t sectors)
{
  Time ssw;
  if (m_isEdmgSupported)
    {
      ssw = edmgSswTxTime;
    }
  else
    {
      ssw = sswTxTime;
    }
  Time duration;
  duration = (sectors - 1) * GetSbifs ();
  duration += sectors * ssw;
  return duration;
}

Time
DmgWifiMac::CalculateSingleAntennaSweepDuration (uint8_t antennas, uint8_t sectors)
{
  Time ssw;
  if (m_isEdmgSupported)
    {
      ssw = edmgSswTxTime;
    }
  else
    {
      ssw = sswTxTime;
    }
  Time duration = Seconds (0);
  duration += (antennas - 1) * GetLbifs ();      /* Inter-time for switching between different antennas. */
  duration += uint16_t ((sectors - antennas)) * GetSbifs ();
  duration += uint16_t (sectors) * ssw;
  return MicroSeconds ((ceil (static_cast<double> (duration.GetNanoSeconds ()) / 1000)));
}

Time
DmgWifiMac::CalculateSectorSweepDuration (uint8_t peerAntennas, uint8_t myAntennas, uint8_t mySectors)
{
  Time ssw;
  if (m_isEdmgSupported)
    {
      ssw = edmgSswTxTime;
    }
  else
    {
      ssw = sswTxTime;
    }
  Time duration = Seconds (0);
  duration += (myAntennas * peerAntennas - 1) * GetLbifs ();      /* Inter-time for switching between different antennas. */
  duration += uint16_t ((mySectors - myAntennas) * peerAntennas) * GetSbifs ();
  duration += uint16_t (mySectors * peerAntennas) * ssw;
  duration += GetMbifs ();
  return MicroSeconds ((ceil (static_cast<double> (duration.GetNanoSeconds ()) / 1000)));
}

Time
DmgWifiMac::CalculateShortSectorSweepDuration (uint8_t antennas, uint8_t sectors)
{
  /* Note: The IFS times between packets are not specified in the standard and need to be confirmed - especially when switching
   * antennas connected to different RF Chains. Also it is not clear whether the initiator takes into account that some responders
   * might have multiple receive antennas to train and repeats the sector sweep for this purpose - for now we assume not. */
  Time duration = Seconds (0);
  duration += (antennas - 1) * GetLbifs ();      /* Inter-time for switching between different antennas. */
  duration += uint16_t ((sectors - antennas)) * GetSbifs ();
  duration += uint16_t (sectors) * edmgShortSswTxTime;
  duration += GetMbifs ();
  return MicroSeconds ((ceil (static_cast<double> (duration.GetNanoSeconds ()) / 1000)));
}

Time
DmgWifiMac::CalculateSisoFeedbackDuration ()
{
  /* The SISO feedback duration is composed of:
   * - BRP poll frame to each responder
   * - BRP frame with feedback from each responder
   * - IFS of Mbifs between all frames
   * */
  uint8_t edmgGroupSize = m_edmgMuGroup.aidList.size ();
  Time duration = Seconds (0);
  duration += edmgGroupSize * edmgBrpPollFrame;
  duration += edmgGroupSize * maxEdmgCtrlFrame;
  duration += (edmgGroupSize * 2 - 1) * GetMbifs ();
  duration += edmgGroupSize * 2 * aAirPropagationTime;
  return MicroSeconds ((ceil (static_cast<double> (duration.GetNanoSeconds ()) / 1000)));
}

Time
DmgWifiMac::CalculateBeamformingTrainingDuration (uint8_t initiatorSectors, uint8_t responderSectors)
{
  Time ssw, sswFbck, sswAck;
  if (m_isEdmgSupported)
    {
      ssw = edmgSswTxTime;
      sswFbck = edmgSswFbckTxTime;
      sswAck = edmgSswAckTxTime;
    }
  else
    {
      ssw = sswTxTime;
      sswFbck = sswFbckTxTime;
      sswAck = sswAckTxTime;
    }
  Time duration;
  duration += (initiatorSectors + responderSectors - 2) * GetSbifs ();
  duration += (initiatorSectors + responderSectors) *  (ssw + aAirPropagationTime);
  duration += sswFbck + sswAck + 2 * aAirPropagationTime;
  duration += GetMbifs () * 3;
  return duration;
}

Time
DmgWifiMac::CalculateTotalBeamformingTrainingDuration (uint8_t initiatorAntennas, uint8_t initiatorSectors,
                                                       uint8_t responderAntennas, uint8_t responderSectors)
{
  Time ssw, sswFbck, sswAck;
  if (m_isEdmgSupported)
    {
      ssw = edmgSswTxTime;
      sswFbck = edmgSswFbckTxTime;
      sswAck = edmgSswAckTxTime;
    }
  else
    {
      ssw = sswTxTime;
      sswFbck = sswFbckTxTime;
      sswAck = sswAckTxTime;
    }
  Time duration = Seconds (0);
//  duration += CalculateSectorSweepDuration (responderAntennas, initiatorAntennas, initiatorSectors);
//  duration += CalculateSectorSweepDuration (initiatorAntennas, responderAntennas, responderSectors);
  duration += (initiatorAntennas * responderAntennas - 1) * GetLbifs ();      /* Initiator: Inter-time for switching between different antennas. */
  duration += uint16_t ((initiatorSectors - initiatorAntennas) * responderAntennas) * GetSbifs ();
  duration += uint16_t (initiatorSectors * responderAntennas) * (ssw + aAirPropagationTime);
  duration += GetMbifs ();  /* Inter-time between Initiator and Responder */
  duration += (initiatorAntennas * responderAntennas - 1) * GetLbifs ();      /* Responder: Inter-time for switching between different antennas. */
  duration += uint16_t ((responderSectors - responderAntennas) * initiatorAntennas) * GetSbifs ();
  duration += uint16_t (responderSectors * initiatorAntennas) * (ssw + aAirPropagationTime);
  duration += sswFbck + sswAck + 2 * (GetMbifs () + aAirPropagationTime);
  return duration;
}

Ptr<EdmgCapabilities>
DmgWifiMac::GetEdmgCapabilities (void) const
{
  Ptr<EdmgCapabilities> capabilities = Create<EdmgCapabilities> ();
  /* Core Capabilities Information Field */
  capabilities->SetAmpduParameters (9, 0);      /* Hardcoded Now (Maximum A-MPDU + No restriction) */
  capabilities->SetTrnParameters (true, true, true, true, true, true, true, true, true, true, true, true); /* All TRN parameters are supported */
  capabilities->SetSupportedMCS (GetDmgWifiPhy ()->GetMaxScMcs (), GetDmgWifiPhy ()->GetMaxOfdmMcs (),
                                 GetDmgWifiPhy ()->GetMaxPhyRate (), false); /* SC MCS6 and OFDM MCS5 are not supported yet */
  /* Set beamforming capability subelement */
  Ptr<BeamformingCapabilitySubelement> beamformingCapabilities = Create<BeamformingCapabilitySubelement> ();
  beamformingCapabilities->SetSuMimoSupported (GetDmgWifiPhy ()->IsSuMimoSupported ());
  capabilities->AddSubElement (beamformingCapabilities);
  /* Set PHY capabilities subelement */
  Ptr<PhyCapabilitiesSubelement> phyCapabilities = Create<PhyCapabilitiesSubelement> ();
  if (GetDmgWifiPhy ()->IsSuMimoSupported ())
    {
      phyCapabilities->SetScMaxNumberofSuMimoSpatialStreamsSupported (m_codebook->GetTotalNumberOfRFChains ());
      if (GetDmgWifiPhy ()->GetSupportOfdmPhy ())
        {
          phyCapabilities->SetOfdmMaxNumberofSuMimoSpatialStreamsSupported (m_codebook->GetTotalNumberOfRFChains ());
        }
    }
  beamformingCapabilities->SetMuMimoSupported (GetDmgWifiPhy ()->IsMuMimoSupported ());
  capabilities->AddSubElement (phyCapabilities);
  /* Set supported channels subelements */
//  Ptr<SupportedChannelsSubelement> supportedChannels = Create<SupportedChannelsSubelement> ();
//  capabilities->AddSubElement (supportedChannels);
  return capabilities;
}

void
DmgWifiMac::StorePeerDmgCapabilities (Ptr<DmgWifiMac> wifiMac)
{
  NS_LOG_FUNCTION (this << wifiMac->GetAddress ());
  StationInformation information;
  information.first = wifiMac->GetDmgCapabilities ();
  m_informationMap[wifiMac->GetAddress ()] = information;
  MapAidToMacAddress (wifiMac->GetAssociationID (), wifiMac->GetAddress ());
  m_stationManager->AddStationDmgCapabilities (wifiMac->GetAddress (), wifiMac->GetDmgCapabilities ());
}

Ptr<DmgCapabilities>
DmgWifiMac::GetPeerStationDmgCapabilities (Mac48Address stationAddress) const
{
  NS_LOG_FUNCTION (this << stationAddress);
  InformationMapCI it = m_informationMap.find (stationAddress);
  if (it != m_informationMap.end ())
    {
      /* We already have information about the DMG STA */
      StationInformation info = static_cast<StationInformation> (it->second);
      return static_cast<Ptr<DmgCapabilities> > (info.first);
    }
  else
    {
      return 0;
    }
}

void
DmgWifiMac::StorePeerEdmgCapabilities (Ptr<DmgWifiMac> wifiMac)
{
  NS_LOG_FUNCTION (this << wifiMac->GetAddress ());
  EdmgStationInformation information;
  information.first = wifiMac->GetEdmgCapabilities ();
  m_edmgInformationMap[wifiMac->GetAddress ()] = information;
}

Ptr<EdmgCapabilities>
DmgWifiMac::GetPeerStationEdmgCapabilities (Mac48Address stationAddress) const
{
  NS_LOG_FUNCTION (this << stationAddress);
  EdmgInformationMapCI it = m_edmgInformationMap.find (stationAddress);
  if (it != m_edmgInformationMap.end ())
    {
      /* We already have information about the DMG STA */
      EdmgStationInformation info = static_cast<EdmgStationInformation> (it->second);
      return static_cast<Ptr<EdmgCapabilities> > (info.first);
    }
  else
    {
      return 0;
    }
}
Time
DmgWifiMac::ComputeBeamformingAllocationSize (Mac48Address responderAddress, bool isInitiatorTXSS, bool isResponderTXSS)
{
  NS_LOG_FUNCTION (this << responderAddress << isInitiatorTXSS << isResponderTXSS);
  // An initiator shall determine the capabilities of the responder prior to initiating BF training with the
  // responder if the responder is associated. A STA may obtain the capabilities of other STAs through the
  // Information Request and Information Response frames (10.29.1) or following a STA’s association with the
  // PBSS/infrastructure BSS. The initiator should use its own capabilities and the capabilities of the responder
  // to compute the required allocation size to perform BF training and BF training related timeouts.
  Ptr<DmgCapabilities> capabilities = GetPeerStationDmgCapabilities (responderAddress);
  if (capabilities)
    {
      uint8_t initiatorSectors, responderSectors;
      if (isInitiatorTXSS && isResponderTXSS)
        {
          initiatorSectors = m_codebook->GetTotalNumberOfTransmitSectors ();
          responderSectors = capabilities->GetNumberOfSectors ();
        }
      else if (isInitiatorTXSS && !isResponderTXSS)
        {
          initiatorSectors = m_codebook->GetTotalNumberOfTransmitSectors ();
          responderSectors = m_codebook->GetTotalNumberOfReceiveSectors ();
        }
      else if (!isInitiatorTXSS && isResponderTXSS)
        {
          initiatorSectors = capabilities->GetRXSSLength ();
          responderSectors = capabilities->GetNumberOfSectors ();
        }
      else
        {
          initiatorSectors = capabilities->GetRXSSLength ();
          responderSectors = m_codebook->GetTotalNumberOfReceiveSectors ();
        }
      NS_LOG_DEBUG ("InitiatorSectors=" << uint16_t (initiatorSectors) << ", ResponderSectors=" << uint16_t (responderSectors));
      return CalculateTotalBeamformingTrainingDuration (m_codebook->GetTotalNumberOfAntennas (), initiatorSectors,
                                                        capabilities->GetNumberOfRxDmgAntennas (), responderSectors);
    }
  else
    {
      return NanoSeconds (0);
    }
}

void
DmgWifiMac::UpdateBestTxAntennaConfiguration (const Mac48Address stationAddress, ANTENNA_CONFIGURATION_TX antennaConfigTx, double snr)
{
  NS_LOG_FUNCTION (this << stationAddress << snr);
  STATION_ANTENNA_CONFIG_MAP_I it = m_bestAntennaConfig.find (stationAddress);
  if (it != m_bestAntennaConfig.end ())
    {
      BEST_ANTENNA_CONFIGURATION *antennaConfig = &(it->second);
      std::get<0> (*antennaConfig) = antennaConfigTx;
      std::get<2> (*antennaConfig) = snr;
    }
  else
    {
      ANTENNA_CONFIGURATION_RX antennaConfigRx = std::make_pair (NO_ANTENNA_CONFIG, NO_ANTENNA_CONFIG);
      m_bestAntennaConfig[stationAddress] = std::make_tuple (antennaConfigTx, antennaConfigRx, snr);
    }
}

void
DmgWifiMac::UpdateBestRxAntennaConfiguration (const Mac48Address stationAddress, ANTENNA_CONFIGURATION_RX antennaConfigRx, double snr)
{
  NS_LOG_FUNCTION (this << stationAddress << snr);
  STATION_ANTENNA_CONFIG_MAP_I it = m_bestAntennaConfig.find (stationAddress);
  if (it != m_bestAntennaConfig.end ())
    {
      BEST_ANTENNA_CONFIGURATION *antennaConfig = &(it->second);
      std::get<1> (*antennaConfig) = antennaConfigRx;
      std::get<2> (*antennaConfig) = snr;
    }
  else
    {
      ANTENNA_CONFIGURATION_RX antennaConfigTx = std::make_pair (NO_ANTENNA_CONFIG, NO_ANTENNA_CONFIG);
      m_bestAntennaConfig[stationAddress] = std::make_tuple (antennaConfigTx, antennaConfigRx, snr);
    }
}

void
DmgWifiMac::UpdateBestAntennaConfiguration (const Mac48Address stationAddress, ANTENNA_CONFIGURATION_TX txConfig,
                                            ANTENNA_CONFIGURATION_RX rxConfig, double snr)
{
  NS_LOG_FUNCTION (this << stationAddress << snr);
  STATION_ANTENNA_CONFIG_MAP_I it = m_bestAntennaConfig.find (stationAddress);
  if (it != m_bestAntennaConfig.end ())
    {
      BEST_ANTENNA_CONFIGURATION *antennaConfig = &(it->second);
      std::get<0> (*antennaConfig) = txConfig;
      std::get<1> (*antennaConfig) = rxConfig;
      std::get<2> (*antennaConfig) = snr;
    }
  else
    {
      m_bestAntennaConfig[stationAddress] = std::make_tuple (txConfig, rxConfig, snr);
    }
}

void
DmgWifiMac::UpdateBestMimoTxAntennaConfigurationIndex (const Mac48Address stationAddress, uint8_t txIndex)
{
  NS_LOG_FUNCTION (this << stationAddress << txIndex);
  STATION_MIMO_ANTENNA_CONFIG_INDEX_MAP::iterator it = m_bestMimoAntennaConfig.find (stationAddress);
  if (it != m_bestMimoAntennaConfig.end ())
    {
      BEST_MIMO_ANTENNA_CONFIG_INDEX *antennaConfigIndex = &(it->second);
      std::get<0> (*antennaConfigIndex) = txIndex;
    }
  else
    {
      m_bestMimoAntennaConfig[stationAddress] = std::make_pair (txIndex, NO_ANTENNA_CONFIG);
    }
}

void
DmgWifiMac::UpdateBestMimoRxAntennaConfigurationIndex (const Mac48Address stationAddress, uint8_t rxIndex)
{
  NS_LOG_FUNCTION (this << stationAddress << rxIndex);
  STATION_MIMO_ANTENNA_CONFIG_INDEX_MAP::iterator it = m_bestMimoAntennaConfig.find (stationAddress);
  if (it != m_bestMimoAntennaConfig.end ())
    {
      BEST_MIMO_ANTENNA_CONFIG_INDEX *antennaConfigIndex = &(it->second);
      std::get<1> (*antennaConfigIndex) = rxIndex;
    }
  else
    {
      m_bestMimoAntennaConfig[stationAddress] = std::make_pair (NO_ANTENNA_CONFIG, rxIndex);
    }
}

ANTENNA_CONFIGURATION
DmgWifiMac::GetAntennaConfiguration (const Mac48Address stationAddress, bool isTxConfiguration) const
{
  STATION_ANTENNA_CONFIG_MAP_CI it = m_bestAntennaConfig.find (stationAddress);
  if (it != m_bestAntennaConfig.end ())
    {
      BEST_ANTENNA_CONFIGURATION antennaConfig = it->second;
      if (isTxConfiguration)
        {
          return std::get<0> (antennaConfig);
        }
      else
        {
          return std::get<1> (antennaConfig);
        }
    }
  else
    {
      NS_ABORT_MSG ("Cannot find antenna configuration for communication with DMG STA=" << stationAddress);
    }
}

ANTENNA_CONFIGURATION
DmgWifiMac::GetBestAntennaConfiguration (const Mac48Address stationAddress, bool isTxConfiguration)
{
  double maxSnr;
  return GetBestAntennaConfiguration (stationAddress, isTxConfiguration, maxSnr);
}

ANTENNA_CONFIGURATION
DmgWifiMac::GetBestAntennaConfiguration (const Mac48Address stationAddress, bool isTxConfiguration, double &maxSnr)
{
  SNR_PAIR snrPair = m_stationSnrMap[stationAddress];
  SNR_MAP snrMap;
  if (isTxConfiguration)
    {
      snrMap = snrPair.first;
    }
  else
    {
      snrMap = snrPair.second;
    }

  SNR_MAP::iterator highIter = snrMap.begin ();
  SNR snr = highIter->second;
  maxSnr = snr;
  for (SNR_MAP::iterator iter = snrMap.begin (); iter != snrMap.end (); iter++)
    {
      if (snr < iter->second)
        {
          highIter = iter;
          snr = highIter->second;
          maxSnr = snr;
        }
    }
  ANTENNA_CONFIGURATION_COMBINATION config = highIter->first;
  return std::make_pair (std::get<1> (config), std::get<2> (config));
}

void
DmgWifiMac::UpdateBestTxAWV (const Mac48Address stationAddress, AWV_ID_TX awvIDTx)
{
  STATION_AWV_MAP::iterator it = m_bestAwvConfig.find (stationAddress);
  if (it != m_bestAwvConfig.end ())
    {
      BEST_AWV_ID *antennaConfig = &(it->second);
      antennaConfig->first = awvIDTx;
    }
  else
    {
      AWV_ID_RX awvIDRx = NO_AWV_ID ;
      m_bestAwvConfig[stationAddress] = std::make_pair (awvIDTx, awvIDRx);
    }
}

void
DmgWifiMac::UpdateBestRxAWV (const Mac48Address stationAddress, AWV_ID_RX awvIDRx)
{
  STATION_AWV_MAP::iterator it = m_bestAwvConfig.find (stationAddress);
  if (it != m_bestAwvConfig.end ())
    {
      BEST_AWV_ID *antennaConfig = &(it->second);
      antennaConfig->second = awvIDRx;
    }
  else
    {
      AWV_ID_TX awvIDTx = NO_AWV_ID;
      m_bestAwvConfig[stationAddress] = std::make_pair (awvIDTx, awvIDRx);
    }
}

void
DmgWifiMac::UpdateBestAWV (const Mac48Address stationAddress,
                           AWV_ID_TX awvIDtx, AWV_ID_RX awvIDrx)
{
  STATION_AWV_MAP::iterator it = m_bestAwvConfig.find (stationAddress);
  if (it != m_bestAwvConfig.end ())
    {
      BEST_AWV_ID *antennaConfig = &(it->second);
      antennaConfig->first = awvIDtx;
      antennaConfig->second = awvIDrx;
    }
  else
    {
      m_bestAwvConfig[stationAddress] = std::make_pair (awvIDtx, awvIDrx);
    }
}

DmgWifiMac::AWV_CONFIGURATION_TX_RX DmgWifiMac::GetBestAntennaPatternConfiguration(Mac48Address peerAp, double &maxSnr)
{
  STATION_SNR_AWV_MAP_I it = m_apSnrAwvMap.find (peerAp);
  if (it != m_apSnrAwvMap.end ())
    {
      SNR_AWV_MAP::iterator highIter = it->second.begin ();
      SNR snr = highIter->second;
      maxSnr = snr;
      for (SNR_AWV_MAP_I iter = it->second.begin (); iter != it->second.end (); iter++)
        {
          if (snr < iter->second)
            {
              highIter = iter;
              snr = highIter->second;
              maxSnr = snr;

            }
        }
      return highIter->first;
    }
  else
    {
      AWV_CONFIGURATION_TX_RX config;
      return config;
    }
}

void
DmgWifiMac::ManagementTxOk (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this);
  /* We need check which ActionFrame it is */
  if (hdr.IsActionNoAck ())
    {
      if (!m_isBrpSetupCompleted[hdr.GetAddr1 ()])
        {
          /* We finished transmitting BRP Frame in setup phase, switch to quasi omni mode for receiving */
          m_codebook->SetReceivingInQuasiOmniMode ();
        }
      else if (m_isBrpSetupCompleted[hdr.GetAddr1 ()] && !m_raisedBrpSetupCompleted[hdr.GetAddr1 ()])
        {
          /* BRP Setup is completed from the initiator side */
          m_raisedBrpSetupCompleted[hdr.GetAddr1 ()] = true;
          BrpSetupCompleted (hdr.GetAddr1 ());
        }

      if (m_requestedBrpTraining && (GetTypeOfStation () == DMG_AP))
        {
          /* If we finished BRP Phase i.e. Receive Sector Training, then start BRP with another station*/
          m_requestedBrpTraining = false;
          NotifyBrpPhaseCompleted ();
        }
    }
}

//// NINA ////
void
DmgWifiMac::FrameTxOk (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this);
  /* We need check which ActionFrame it is */
  if (hdr.IsActionNoAck ())
    {
      Time edmgTrnFieldDuration = GetDmgWifiPhy ()->GetEdmgTrnFieldDuration ();
      if (m_muMimoBeamformingTraining)
        {
          Simulator::Schedule (edmgTrnFieldDuration, &DmgWifiMac::FrameTxOkMuMimoBFT, this, hdr);
        }
      else
        {
          Simulator::Schedule (edmgTrnFieldDuration, &DmgWifiMac::FrameTxOkSuMimoBFT, this, hdr);
        }

    }
}

void
DmgWifiMac::FrameTxOkSuMimoBFT (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this);

  if (!m_isMimoBrpSetupCompleted[hdr.GetAddr1 ()])
    {
      /* We finished transmitting BRP Frame in BRP MIMO TXSS setup phase for initiator, switch to quasi omni mode for receiving */
      m_codebook->SetReceivingInQuasiOmniMode ();
    }
  else if (m_isMimoBrpSetupCompleted[hdr.GetAddr1 ()] && !m_suMimoBeamformingTraining)
    {
      /* We finished transmitting BRP Frame in BRP MIMO TXSS setup phase for responder, switch to quasi omni mode for receiving */
      m_suMimoBeamformingTraining = true;
      m_suMimoSisoSnrMap.clear ();
      m_mimoSisoSnrList.clear ();
      GetDmgWifiPhy ()->SetSuMimoBeamformingTraining (true);
      m_codebook->SetReceivingInQuasiOmniMode ();
      m_codebook->SetUseAWVsMimoBft (false);
    }
  else if (m_isMimoBrpSetupCompleted[hdr.GetAddr1 ()] && m_suMimoBeamformingTraining)
    {
      if (m_suMimoBfPhase == SU_MIMO_SETUP_PHASE)
        {
          /* We finished transmitting MIMO BF Setup frame for initiator, wait for response from responder */
          if (!m_isBrpResponder[hdr.GetAddr1 ()])
            {
              m_codebook->SetReceivingInQuasiOmniMode ();
            }
          else
            {
              /* We finished transmitting MIMO BF Setup frame for responder, set up for initiator SMBT */
              bool firstCombination = true;
              m_mimoSnrList.clear ();
              m_codebook->InitializeMimoSectorSweeping (m_peerStation, ReceiveSectorSweep, firstCombination);
            }
        }
      else
        {
          /* In the middle of MIMO BRP TXSS or SMBT */
          // Shut off all antennas except one for the Tx/Rx of the next packet
          m_codebook->SetCommunicationMode (SISO_MODE);
          if (m_brpCdown == 0)
            {
              /* MIMO BRP Initiator TXSS has been completed wait for feedback from responder */
              if (!m_isBrpResponder[hdr.GetAddr1 ()] && m_suMimoBfPhase == SU_SISO_INITIATOR_TXSS)
                {
                  m_suMimoBfPhase = SU_SISO_RESPONDER_FBCK;
                  m_codebook->SetReceivingInQuasiOmniMode ();
                }
              /* Just sent Responder Feedback, start responder BRP TXSS */
              else if (m_isBrpResponder[hdr.GetAddr1 ()] && (m_suMimoBfPhase == SU_SISO_RESPONDER_FBCK))
                {
                  Simulator::Schedule (m_mbifs, &DmgWifiMac::StartMimoBrpTxss, this);
                }
              /* MIMO BRP Responder TXSS has been completed wait for feedback from initiator */
              else if (m_isBrpResponder[hdr.GetAddr1 ()] && (m_suMimoBfPhase == SU_SISO_RESPONDER_TXSS))
                {
                  m_suMimoBfPhase = SU_SISO_INITIATOR_FBCK;
                  m_codebook->SetReceivingInQuasiOmniMode ();
                }
              /* Just sent Initiator Feedback, wait for ACK from responder */
              //              else if (!m_isBrpResponder[hdr.GetAddr1 ()] && m_suMimoBfPhase == SISO_INITIATOR_FBCK)
              //                {

              //                }
              // Initiator SMBT has been completed, wait for responder SMBT
              else if (!m_isBrpResponder[hdr.GetAddr1 ()] && (m_suMimoBfPhase == SU_MIMO_INITIATOR_SMBT))
                {
                  m_suMimoBfPhase = SU_MIMO_RESPONDER_SMBT;
                  m_recordTrnSnrValues = true;
                  m_codebook->SetReceivingInQuasiOmniMode ();
                  // Set up codebook to start switching the receive combinations that we want to test in the responder SMBT
                  bool firstCombination = true;
                  m_codebook->InitializeMimoSectorSweeping (m_peerStation, ReceiveSectorSweep, firstCombination);
                }
              // Responder SMBT has been completed wait for feedback from initiator
              else if (m_isBrpResponder[hdr.GetAddr1 ()] && (m_suMimoBfPhase == SU_MIMO_RESPONDER_SMBT))
                {
                  m_suMimoBfPhase = SU_MIMO_FBCK_PHASE;
                  m_recordTrnSnrValues = false;
                  m_codebook->SetReceivingInQuasiOmniMode ();
                }
              else if (!m_isBrpResponder[hdr.GetAddr1 ()] && (m_suMimoBfPhase == SU_MIMO_FBCK_PHASE))
                {
                  m_codebook->SetReceivingInQuasiOmniMode ();
                }
              else if (m_isBrpResponder[hdr.GetAddr1 ()] && (m_suMimoBfPhase == SU_MIMO_FBCK_PHASE))
                {
                  m_suMimoBeamformingTraining = false;
                  GetDmgWifiPhy ()->SetSuMimoBeamformingTraining (false);
                  m_suMimoBfPhase = SU_WAIT_SU_MIMO_BF_TRAINING;
                  m_codebook->SetReceivingInQuasiOmniMode ();
                  m_dataCommunicationModeTable[hdr.GetAddr1 ()] = DATA_MODE_SU_MIMO;
                  m_low->MIMO_BFT_Phase_Ended ();
                  m_mimoConfigTraining.clear ();
                  m_suMimoMimoPhaseComplete (hdr.GetAddr1 ());
                }
            }
          else
            {
              m_brpCdown--;
              // We're in the MIMO SMBT phase
              if ((m_suMimoBfPhase == SU_MIMO_INITIATOR_SMBT) || (m_suMimoBfPhase == SU_MIMO_RESPONDER_SMBT))
                {
                  Simulator::Schedule (GetSifs (), &DmgWifiMac::SendMimoBfTrainingBrpFrame, this, hdr.GetAddr1 ());
                }
              // We're in the MIMO BRP TXSS Phase
              else
                {
                  if (m_remainingTxssPackets != 0)
                    {
                      m_remainingTxssPackets--;
                      // Switch to the next combination of antennas to be trained
                      bool firstCombination = false;
                      m_codebook->InitializeMimoSectorSweeping (hdr.GetAddr1 (), TransmitSectorSweep, firstCombination);
                      Simulator::Schedule (GetSifs (), &DmgWifiMac::SendMimoBrpTxssFrame, this, hdr.GetAddr1 ());
                    }
                  else if (m_peerTxssRepeat != 0)
                    {
                      m_peerTxssRepeat--;
                      m_remainingTxssPackets = m_txssPackets;
                      // Go to the first combination of antennas to be trained for new repetition
                      bool firstCombination = true;
                      m_codebook->InitializeMimoSectorSweeping (hdr.GetAddr1 (), TransmitSectorSweep, firstCombination);
                      Simulator::Schedule (GetSifs (), &DmgWifiMac::SendMimoBrpTxssFrame, this, hdr.GetAddr1 ());
                    }
                  else
                    {
                      NS_ABORT_MSG ("Wrong values for brpCdown");
                    }
                }
            }
        }
    }
}

void
DmgWifiMac::FrameTxOkMuMimoBFT (const WifiMacHeader &hdr)
{
  /* Finished sending a BRP poll frame to a member of the MU group being trained */
  if ((m_muMimoBfPhase == MU_SISO_FBCK) && m_isMuMimoInitiator)
    {
      m_currentMuGroupMember++;
      /* Calculate the maximum time it might take for a responder to send back feedback - if no response
       * arrives by that time move on to the next user in the MU group or trigger callback for end of SISO phase
       * if there are no more users left */
      Time feedbackDuration = maxEdmgCtrlFrame + GetMbifs () + 2 * aAirPropagationTime;
      if (m_currentMuGroupMember != m_edmgMuGroup.aidList.end ())
        {
          m_muMimoFbckTimeout = Simulator::Schedule (feedbackDuration, &DmgWifiMac::SendBrpFbckPollFrame, this);
        }
      else
        {
          m_muMimoFbckTimeout = Simulator::Schedule (feedbackDuration, &DmgWifiMac::RegisterMuMimoSisoPhaseComplete, this,
                                                     m_muMimoFeedbackMap, m_codebook->GetTotalNumberOfRFChains (), m_edmgMuGroup.aidList.size (),
                                                     m_edmgMuGroup.groupID, m_muMimoBftIdMap [m_edmgMuGroup.groupID]);
        }
    }
  else if ((m_muMimoBfPhase == MU_MIMO_BF_SETUP) && m_isMuMimoInitiator)
    {
      m_currentMuGroupMember++;
      if (m_currentMuGroupMember != m_edmgMuGroup.aidList.end ())
        Simulator::Schedule (GetSifs (), &DmgWifiMac::SendMuMimoSetupFrame, this);
      else
        {
          // If no stations from the MU group participate in the MIMO phase training, go straight to the selection subphase, otherwise start the MIMO phase training.
          if (GenerateEdmgMuGroupMask () == 0)
            {
              Simulator::Schedule (m_mbifs, &DmgWifiMac::StartMuMimoSelectionSubphase, this);
            }
          else
            {
              Simulator::Schedule (m_mbifs, &DmgWifiMac::StartMuMimoBfTrainingSubphase, this);
            }
        }
    }
  else if (m_muMimoBfPhase == MU_MIMO_BF_TRAINING)
    {
      /* In the middle of MU MIMO BF Training  */
      // Shut off all antennas except one for the Tx/Rx of the next packet
      m_codebook->SetCommunicationMode (SISO_MODE);
      if (m_brpCdown == 0)
        {
          Simulator::Schedule (m_mbifs, &DmgWifiMac::StartMuMimoBfFeedbackSubphase, this);
        }
      else
        {
          m_brpCdown--;
          Simulator::Schedule (GetSifs (), &DmgWifiMac::SendMimoBfTrainingBrpFrame, this, GetAddress ());
        }
    }
  else if ((m_muMimoBfPhase == MU_MIMO_BF_FBCK) && m_isMuMimoInitiator)
    {
      m_currentMuGroupMember++;
      bool foundResponder = false;
      while (!foundResponder && m_currentMuGroupMember != m_edmgMuGroup.aidList.end ())
        {
          MU_GROUP_MAP::iterator it = m_edmgMuGroupMap.find (*m_currentMuGroupMember);
          if (it != m_edmgMuGroupMap.end () && (m_edmgMuGroupMap[*m_currentMuGroupMember] == true))
            foundResponder = true;
          else
            {
              m_currentMuGroupMember++;
            }
        }
      /* Calculate the maximum time it might take for a responder to send back feedback - if no response
       * arrives by that time move on to the next user in the MU group or trigger callback for end of Feedback phase
       * if there are no more users left */
      Time feedbackDuration = maxEdmgCtrlFrame + GetSifs () + 2 * aAirPropagationTime;
      if (m_currentMuGroupMember != m_edmgMuGroup.aidList.end ())
        {
          m_muMimoFbckTimeout = Simulator::Schedule (feedbackDuration, &DmgWifiMac::SendMimoBfPollFrame, this);
        }
      else
        {
          m_muMimoFbckTimeout = Simulator::Schedule (feedbackDuration, &DmgWifiMac::StartMuMimoSelectionSubphase, this);
        }
    }
  else if ((m_muMimoBfPhase == MU_MIMO_BF_SELECTION) && m_isMuMimoInitiator)
    {
      m_currentMuGroupMember++;
      if (m_currentMuGroupMember != m_edmgMuGroup.aidList.end ())
        Simulator::Schedule (GetSifs (), &DmgWifiMac::SendMuMimoBfSelectionFrame, this);
      else
        {
          m_muMimoBeamformingTraining = false;
          m_isMuMimoInitiator = false;
          GetDmgWifiPhy ()->SetMuMimoBeamformingTraining (false);
          m_muMimoBfPhase = MU_WAIT_MU_MIMO_BF_TRAINING;
          m_codebook->SetReceivingInQuasiOmniMode ();
          m_mimoConfigTraining.clear ();
          for (auto user : m_edmgMuGroup.aidList)
            {
              m_dataCommunicationModeTable[m_aidMap[user]] = DATA_MODE_MU_MIMO;
            }
          m_low->MIMO_BFT_Phase_Ended ();
          m_muMimoMimoPhaseComplete ();
        }
    }
}

void
DmgWifiMac::FrameTxOkShortSsw (void)
{
  NS_LOG_FUNCTION (this);
  /* If we are sending the Short SSW as part of MU-MIMO BFT */
  if (m_muMimoBeamformingTraining && (m_muMimoBfPhase == MU_SISO_TXSS))
    {
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
          Simulator::Schedule (spacing, &DmgWifiMac::SendMuMimoInitiatorTxssFrame, this);
        }
      else
        {
          /* We have finished Initiator TXSS */
          Simulator::Schedule (m_mbifs, &DmgWifiMac::StartMuMimoSisoFeedback, this);
        }
    }
}
//// NINA ////

void
DmgWifiMac::TxOk (Ptr<const Packet> currentPacket, const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this);
  if (m_currentLinkMaintained && hdr.IsQosData () && m_currentAllocation == SERVICE_PERIOD_ALLOCATION)
    {
      /* Report the current value */
      m_beamLinkMaintenanceTimerStateChanged (BEAM_LINK_MAINTENANCE_TIMER_RESET,
                                              m_peerStationAid, m_peerStationAddress,
                                              Simulator::GetDelayLeft (m_beamLinkMaintenanceTimeout));
      /* Setup and release the new timer */
      m_beamLinkMaintenanceTimeout.Cancel ();
      m_linkMaintenanceInfo.rest ();
      m_beamLinkMaintenanceTimeout = Simulator::Schedule (m_linkMaintenanceInfo.beamLinkMaintenanceTime,
                                                          &DmgWifiMac::BeamLinkMaintenanceTimeout, this);
      m_beamLinkMaintenanceTimerStateChanged (BEAM_LINK_MAINTENANCE_TIMER_SETUP_RELEASE,
                                              m_peerStationAid, m_peerStationAddress,
                                              m_linkMaintenanceInfo.beamLinkMaintenanceTime);
    }
  RegularWifiMac::TxOk (currentPacket, hdr);
}

void
DmgWifiMac::ReceiveSectorSweepFrame (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << packet);

  CtrlDMG_SSW sswFrame;
  packet->RemoveHeader (sswFrame);

  /* Ensure that we do not receive sectors from other stations while we are already performing BFT with a particular station */
  if ((m_performingBFT) && (m_peerStationAddress != hdr->GetAddr2 ()))
    {
      NS_LOG_INFO ("Received SSW frame different DMG STA=" << hdr->GetAddr2 ());
      return;
    }

  //// NINA ////
  BftIdTag tag;
  packet->RemovePacketTag (tag);
  m_bftIdMap [hdr->GetAddr2 ()] = tag.Get ();
  //// NINA ////

  DMG_SSW_Field ssw = sswFrame.GetSswField ();
  DMG_SSW_FBCK_Field sswFeedback = sswFrame.GetSswFeedbackField ();

  if (ssw.GetDirection () == BeamformingInitiator)
    {
      NS_LOG_LOGIC ("Responder: Received SSW frame as part of ISS from Initiator=" << hdr->GetAddr2 ());
      if (m_rssEvent.IsExpired ())
        {
          Time rssTime = hdr->GetDuration ();
          if (m_currentAllocation == CBAP_ALLOCATION)
            {
              /* We received the first SSW from the initiator during CBAP allocation, so we initialize variables. */
              /* Remove current Sector Sweep Information with the station we want to perform beamforming training with */
              m_stationSnrMap.erase (hdr->GetAddr2 ());
              /* Initialize some of the BFT variables here */
              m_isInitiatorTXSS = true;
              m_isResponderTXSS = true;
              /* Lock the responder on this initiator */
              m_performingBFT = true;
              m_low->SLS_Phase_Started ();
              m_peerStationAddress = hdr->GetAddr2 ();
              /* Cancel any SSW-FBCK frame timeout (This might happen if the initiator does not receive SSW frames from the
                 responder but the respodner already schedulled SSW-FBCK timeout event.) */
              if (m_sswFbckTimeout.IsRunning ())
                {
                  m_sswFbckTimeout.Cancel ();
                }
              m_rssEvent = Simulator::Schedule (rssTime, &DmgSlsTxop::Start_Responder_Sector_Sweep, m_dmgSlsTxop, hdr->GetAddr2 ());
            }
          else
            {
              m_rssEvent = Simulator::Schedule (rssTime, &DmgWifiMac::StartBeamformingResponderPhase, this, hdr->GetAddr2 ());
            }
          NS_LOG_LOGIC ("Scheduled RSS Period for Responder=" << GetAddress () << " at " << Simulator::Now () + rssTime);
        }

      if (m_isInitiatorTXSS)
        {
          /* Initiator is TXSS and we store SNR to report it back to the initiator */
          MapTxSnr (hdr->GetAddr2 (), ssw.GetDMGAntennaID (), ssw.GetSectorID (), m_stationManager->GetRxSnr ());
        }
      else
        {
          /* Initiator is RXSS and we store SNR to select the best Rx Sector with the initiator */
          MapRxSnr (hdr->GetAddr2 (),
                    m_codebook->GetActiveAntennaID (),
                    m_codebook->GetActiveRxSectorID (),
                    m_stationManager->GetRxSnr ());
          //          m_codebook->GetNextSector ();
        }
    }
  else
    {
      NS_LOG_LOGIC ("Initiator: Received SSW frame as part of RSS from Responder=" << hdr->GetAddr2 ());

      /* If we receive one SSW Frame at least from the responder, then we schedule SSW-FBCK */
      if (!m_sectorFeedbackSchedulled)
        {
          NS_LOG_DEBUG ("Cancel Restart ISS event.");
          m_restartISSEvent.Cancel ();
          m_sectorFeedbackSchedulled = true;

          /* The SSW Frame we received is part of RSS */
          /* Not part of ISS i.e. the SSW Feedback Field Contains the Feedbeck of the ISS */
          sswFeedback.IsPartOfISS (false);

          /* Set the best TX antenna configuration reported by the SSW-FBCK Field */
          if (m_isInitiatorTXSS)
            {
              /* The Sector Sweep Frame contains feedback about the the best Tx Sector used by the initiator */
              ANTENNA_CONFIGURATION_TX antennaConfigTx = std::make_pair (sswFeedback.GetDMGAntenna (), sswFeedback.GetSector ());
              UpdateBestTxAntennaConfiguration (hdr->GetAddr2 (), antennaConfigTx, sswFeedback.GetSNRReport ());
              if (m_antennaPatternReciprocity && m_isEdmgSupported)
                {
                  UpdateBestRxAntennaConfiguration (hdr->GetAddr2 (), antennaConfigTx, sswFeedback.GetSNRReport ());
                }
              NS_LOG_LOGIC ("Best TX Antenna Sector Config by this DMG STA to DMG STA=" << hdr->GetAddr2 ()
                            << ": AntennaID=" << static_cast<uint16_t> (antennaConfigTx.first)
                            << ", SectorID=" << static_cast<uint16_t> (antennaConfigTx.second));
            }

          Time sswFbckStartTime;
          if (m_currentAllocation == CBAP_ALLOCATION)
            {
              sswFbckStartTime = GetSectorSweepDuration (ssw.GetCountDown ()) + GetMbifs ();
              Simulator::Schedule (sswFbckStartTime, &DmgSlsTxop::Start_Initiator_Feedback, m_dmgSlsTxop, hdr->GetAddr2 ());
            }
          else
            {
              Time sswFbckDuration = GetRemainingAllocationTime ();
              sswFbckStartTime = hdr->GetDuration ();
              Simulator::Schedule (sswFbckStartTime, &DmgWifiMac::SendSswFbckFrame, this, hdr->GetAddr2 (), sswFbckDuration);
            }
          NS_LOG_LOGIC ("Scheduled SSW-FBCK Frame to " << hdr->GetAddr2 ()
                        << " at " << Simulator::Now () + sswFbckStartTime);
        }

      if (m_isResponderTXSS)
        {
          /* Responder is TXSS and we store SNR to report it back to the responder */
          MapTxSnr (hdr->GetAddr2 (), ssw.GetDMGAntennaID (), ssw.GetSectorID (), m_stationManager->GetRxSnr ());
        }
      else
        {
          /* Responder is RXSS and we store SNR to select the best Rx Sector with the responder */
          MapRxSnr (hdr->GetAddr2 (),
                    m_codebook->GetActiveAntennaID (),
                    m_codebook->GetActiveRxSectorID (),
                    m_stationManager->GetRxSnr ());
          //          m_codebook->GetNextSector ();
        }
    }
}

void
DmgWifiMac::Receive (Ptr<WifiMacQueueItem> mpdu)
{
  NS_LOG_FUNCTION (this << *mpdu);

  const WifiMacHeader* hdr = &mpdu->GetHeader ();
  Ptr<Packet> packet = mpdu->GetPacket ()->Copy ();
  Mac48Address from = hdr->GetAddr2 ();

  if (hdr->IsSSW ())
    {
      ReceiveSectorSweepFrame (packet, hdr);
      return;
    }
  else if (hdr->IsSSW_ACK ())
    {
      NS_LOG_LOGIC ("Initiator: Received SSW-ACK frame from=" << from);

      /* We are the SLS Initiator */
      CtrlDMG_SSW_ACK sswAck;
      packet->RemoveHeader (sswAck);

      /* Check Beamformed link maintenance */
      RecordBeamformedLinkMaintenanceValue (sswAck.GetBfLinkMaintenanceField ());

      /* We add the station to the list of the stations we can directly communicate with */
      AddForwardingEntry (from);

      /* Cancel SSW-Feedback timer */
      m_sswAckTimeoutEvent.Cancel ();

      /* Get best antenna configuration */
      Mac48Address address = from;
      BEST_ANTENNA_CONFIGURATION info = m_bestAntennaConfig[address];
      ANTENNA_CONFIGURATION_TX antennaConfigTx = std::get<0> (info);
      double snr = std::get<2> (info);

      /* Inform WifiRemoteStationManager about link SNR value */
      m_stationManager->RecordLinkSnr (address, snr);
      m_slsInitiatorStateMachine = SLS_INITIATOR_TXSS_PHASE_COMPELTED;

      /* Raise a callback indicating we've completed the SLS phase */
      m_slsCompleted (SlsCompletionAttrbitutes (from, CHANNEL_ACCESS_DTI, BeamformingInitiator,
                                                m_isInitiatorTXSS, m_isResponderTXSS, m_bftIdMap [from],
                                                antennaConfigTx.first, antennaConfigTx.second, m_maxSnr));

      /* Inform DMG SLS TXOP that we've received the SSW-ACK frame */
      m_dmgSlsTxop->SLS_BFT_Completed ();

      /* Check if we need to start BRP phase following SLS phase */
      BRP_Request_Field brpRequest = sswAck.GetBrpRequestField ();
      if ((brpRequest.GetL_RX () > 0) || brpRequest.GetTX_TRN_REQ ())
        {
          /* BRP setup sub-phase is skipped in this case*/
          m_executeBRPinATI = false;
          InitiateBrpTransaction (from, brpRequest.GetL_RX (), brpRequest.GetTX_TRN_REQ ());
        }

      /* Resume data transmission after SLS operation */
      if (m_currentAllocation == CBAP_ALLOCATION)
        {
          m_txop->ResumeTxopTransmission ();
          for (EdcaQueues::iterator i = m_edca.begin (); i != m_edca.end (); ++i)
            {
              i->second->ResumeTxopTransmission ();
            }
        }

      return;
    }

  else if (hdr->IsAction () || hdr->IsActionNoAck ())
    {
      WifiActionHeader actionHdr;
      packet->RemoveHeader (actionHdr);

      switch (actionHdr.GetCategory ())
        {
        case WifiActionHeader::BLOCK_ACK:
          packet->AddHeader (actionHdr);
          RegularWifiMac::Receive (mpdu);
          return;

        case WifiActionHeader::DMG:
          switch (actionHdr.GetAction ().dmgAction)
            {
            case WifiActionHeader::DMG_RELAY_ACK_REQUEST:
              {
                ExtRelaySearchRequestHeader requestHdr;
                packet->RemoveHeader (requestHdr);
                return;
              }
            case WifiActionHeader::DMG_RELAY_ACK_RESPONSE:
              {
                ExtRelaySearchResponseHeader responseHdr;
                packet->RemoveHeader (responseHdr);
                return;
              }
            default:
              NS_FATAL_ERROR ("Unsupported Action frame received");
              return;
            }

        case WifiActionHeader::UNPROTECTED_DMG:
          switch (actionHdr.GetAction ().unprotectedAction)
            {
            case WifiActionHeader::UNPROTECTED_DMG_ANNOUNCE:
              {
                ExtAnnounceFrame announceHdr;
                packet->RemoveHeader (announceHdr);
                return;
              }

            case WifiActionHeader::UNPROTECTED_DMG_BRP:
              {
                ExtBrpFrame brpFrame;
                packet->RemoveHeader (brpFrame);

                BRP_Request_Field requestField;
                requestField = brpFrame.GetBrpRequestField ();

                BeamRefinementElement element;
                element = brpFrame.GetBeamRefinementElement ();

                Ptr<EdmgBrpRequestElement> edmgElement;
                edmgElement = brpFrame.GetEdmgBrpRequestElement ();

                if (edmgElement != 0)
                  {
                    //// NINA ////
                    /* We have received a request to start a MIMO BRP TXSS as the SISO part of SU-MIMO BF Training */
                    if (edmgElement->GetBRP_TXSS () && edmgElement->GetTXSS_Initiator () && edmgElement->GetTXSS_MIMO ())
                      {
                        /* For now, we assume that we support all values of P, N and M and that they will be the same for initiator and responder training. */
                        m_edmgTrnP = edmgElement->GetRequestedEDMG_TRN_UnitP ();
                        m_edmgTrnM = edmgElement->GetRequestedEDMG_TRN_UnitM ();
                        m_edmgTrnN = edmgElement->GetRequestedEDMG_TRN_UnitN ();
                        m_recordTrnSnrValues = true;
                        m_peerStation = from;
                        m_peerTxssPackets = edmgElement->GetTXSS_Packets ();
                        GetDmgWifiPhy ()->SetPeerTxssPackets (m_peerTxssPackets);
                        m_peerTxssRepeat = edmgElement->GetTXSS_Repeat ();
                        m_isBrpResponder[from] = true;
                        m_isMimoBrpSetupCompleted[from] = true;
                        m_suMimoBfPhase = SU_SISO_SETUP_PHASE;
                        m_low->MIMO_BFT_Phase_Started ();

                        /* Reply back to the Initiator */
                        BeamRefinementElement replyElement;
                        replyElement.SetAsBeamRefinementInitiator (false);
                        replyElement.SetCapabilityRequest (false);

                        BRP_Request_Field replyRequestField;
                        EdmgBrpRequestElement edmgReplyRequestElement;

                        edmgReplyRequestElement.SetBRP_TXSS (true);
                        edmgReplyRequestElement.SetTXSS_Initiator (false);

                        // Get a list of the antenna IDs of all the antennas in the codebook.
                        std::vector<AntennaID> antennaIds = m_codebook->GetTotalAntennaIdList ();
                        edmgReplyRequestElement.SetTX_Antenna_Mask (antennaIds);
                        for (auto & antenna: antennaIds)
                          {
                            ANTENNA_CONFIGURATION_TX antennaConfigTx = std::get<0> (m_bestAntennaConfig[from]);
                            ANTENNA_CONFIGURATION config = std::make_pair (antenna, antennaConfigTx.second);
                            AWV_CONFIGURATION pattern = std::make_pair (config, NO_AWV_ID);
                            m_mimoConfigTraining.push_back (pattern);
                          }
                        /* Set up the antenna combinations to test in each packet of the MIMO BRP TXSS and calculate
                         * the number of MIMO BRP TXSS packets that we need if there are multiple antennas which are connected
                         * to the same RF Chain we need multiple BRP packets to train them, otherwise we just need one. */
                        m_txssPackets = m_codebook->SetUpMimoBrpTxss (antennaIds, from);
                        m_txssRepeat = m_txssPackets;
                        GetDmgWifiPhy ()->SetTxssRepeat (m_txssRepeat);
                        edmgReplyRequestElement.SetTXSS_Packets (m_txssPackets);
                        edmgReplyRequestElement.SetTXSS_Repeat (m_txssRepeat);
                        NS_LOG_LOGIC ("MIMO BRP TXSS Setup Subphase is being terminated by Responder=" << GetAddress ());

                        // Update the BFT ID according to the initiator.
                        BftIdTag tag;
                        packet->RemovePacketTag (tag);
                        m_bftIdMap [from] = tag.Get ();

                        /* Send BRP Frame terminating the setup phase from the responder side */
                        Simulator::Schedule (m_mbifs, &DmgWifiMac::SendEmptyMimoBrpFrame, this, from,
                                             replyRequestField, replyElement, edmgReplyRequestElement);
                      }
                    /* We have received a reply to the request to start a MIMO BRP TXSS as the SISO part of SU-MIMO BF Training */
                    else if (edmgElement->GetBRP_TXSS () && !edmgElement->GetTXSS_Initiator ())
                      {
                        m_peerStation = from;
                        m_peerTxssPackets = edmgElement->GetTXSS_Packets ();
                        m_peerTxssRepeat = edmgElement->GetTXSS_Repeat ();
                        m_isMimoBrpSetupCompleted[from] = true;

                        NS_LOG_LOGIC ("MIMO BRP TXSS Setup Subphase between Initiator=" << from
                                      << " and Responder=" << GetAddress () << " is terminated");

                        m_suMimoBeamformingTraining = true;
                        m_suMimoSisoSnrMap.clear ();
                        m_mimoSisoSnrList.clear ();
                        GetDmgWifiPhy ()->SetSuMimoBeamformingTraining (true);
                        Simulator::Schedule (m_mbifs, &DmgWifiMac::StartMimoBrpTxss, this);
                      }
                    /* We have received a BRP frame with feedback for the training */
                    else if (element.IsSnrPresent ())
                      {
                        uint8_t peerAid = m_macMap[from];
                        ChannelMeasurementFeedbackElementList channelElementList;
                        channelElementList = brpFrame.GetChannelMeasurementFeedbackList ();
                        EDMGChannelMeasurementFeedbackElementList edmgChannelElementList;
                        edmgChannelElementList = brpFrame.GetEdmgChannelMeasurementFeedbackList ();
                        uint8_t index = 0;
                        /*  Save the feedback received
                         *  Note: We assume that there is an equal number of Channel Measurement Elements and EDMG Channel Measurement Elements
                         *  and that the Channel Measurement element and the corresponding EDMG channel measurement element at the same position
                         *  in the lists contain feedback for the same number of measurements
                         */
                        for (EDMGChannelMeasurementFeedbackElementListI it = edmgChannelElementList.begin (); it != edmgChannelElementList.end (); it++)
                          {
                            Ptr<ChannelMeasurementFeedbackElement> channelElement = channelElementList.at (index);
                            EDMGSectorIDOrder_List sectorIdList = (*it)->Get_EDMG_SectorIDOrder_List ();
                            SNR_INT_LIST snrList = channelElement->GetSnrList ();
                            for (uint8_t i = 0; i < sectorIdList.size (); i++)
                              {
                                /* if the feedback frame is for SU-MIMO BFT */
                                if (element.GetBfTrainingType () == SU_MIMO_BF && m_isMimoBrpSetupCompleted[from])
                                  {
                                    SectorID sector = m_codebook->GetSectorIdMimoBrpTxss (sectorIdList.at (i).TXAntennaID, sectorIdList.at (i).SectorID);
                                    MIMO_FEEDBACK_CONFIGURATION feedbackConfig
                                        = std::make_tuple (sectorIdList.at (i).TXAntennaID, sectorIdList.at (i).RXAntennaID,
                                                           sector);
                                    //In case of multiple measurements for the same combination (if TRN subfields are repeated), save the maximum SNR
                                    MIMO_FEEDBACK_MAP::iterator iter = m_suMimoFeedbackMap.find (feedbackConfig);
                                    if (iter != m_suMimoFeedbackMap.end ())
                                      {
                                        if (MapIntToSnr (snrList.at (i)) > m_suMimoFeedbackMap[feedbackConfig])
                                          m_suMimoFeedbackMap[feedbackConfig] = MapIntToSnr (snrList.at (i));
                                      }
                                    else
                                      m_suMimoFeedbackMap[feedbackConfig] = MapIntToSnr (snrList.at (i));
                                  }
                                /* If the feedback frame is for MU-MIMO BFT */
                                else if ((element.GetBfTrainingType () == MU_MIMO_BF) && m_muMimoBeamformingTraining)
                                  {
                                    /* We use the same structure to save the feedback for SU-MIMO and MU-MIMO BFT. In the case of MU-MIMO
                                     * we save the AID of the STA instead of the Rx Antenna ID - this allows us to re-use the same selection
                                     * algorithms. */
                                    MIMO_FEEDBACK_CONFIGURATION feedbackConfig;
                                    /* check the type of the last training */
                                    if (element.GetSectorSweepFrameType () == SHORT_SSW_FRAME)
                                      {
                                        ANTENNA_CONFIGURATION antennaConfig =
                                            m_codebook->GetAntennaConfigurationShortSSW (sectorIdList.at (i).SectorID);
                                        feedbackConfig = std::make_tuple (antennaConfig.first, peerAid, antennaConfig.second);
                                      }
                                    else if (element.GetSectorSweepFrameType () == SSW_FRAME)
                                      {
                                        feedbackConfig = std::make_tuple (sectorIdList.at (i).TXAntennaID, peerAid,
                                                                          sectorIdList.at (i).SectorID);
                                      }
                                    /* If we receive feedback from multiple receive antennas for the same Tx Config, we only save the highest one. */
                                    auto it = m_muMimoFeedbackMap.find (feedbackConfig);
                                    if ((it == m_muMimoFeedbackMap.end ()) ||
                                        (MapIntToSnr (snrList.at (i)) > m_muMimoFeedbackMap [feedbackConfig]))
                                      {
                                        m_muMimoFeedbackMap [feedbackConfig] = MapIntToSnr (snrList.at (i));
                                      }
                                  }
                              }
                            index++;
                          }
                        /* If we have received responder feedback for SU-MIMO BF, prepare for the responder TXSS */
                        if ((m_suMimoBfPhase == SU_SISO_RESPONDER_FBCK) && !m_isBrpResponder[from])
                          {
                            GetDmgWifiPhy ()->SetPeerTxssPackets (m_peerTxssPackets);
                            GetDmgWifiPhy ()->SetTxssRepeat (m_txssRepeat);
                            m_suMimoBfPhase = SU_SISO_RESPONDER_TXSS;
                            m_recordTrnSnrValues = true;
                          }
                        /* If we have received initiator feedback for SU-MIMO BF, send an ACK to finish the SISO phase of the SU-MIMO BFT */
                        else if ((m_suMimoBfPhase == SU_SISO_INITIATOR_FBCK) && m_isBrpResponder[from])
                          {
                            m_suMimoBfPhase = SU_SISO_RESPONDER_TXSS;
                            /* Sent an ACK to the initiator */
                            BeamRefinementElement ackElement;
                            ackElement.SetAsBeamRefinementInitiator (false);
                            ackElement.SetCapabilityRequest (false);
                            ackElement.SetTxTrnOk (true);

                            BRP_Request_Field ackRequestField;
                            EdmgBrpRequestElement edmgAckRequestElement;
                            edmgAckRequestElement.SetTXSS_Initiator (false);

                            NS_LOG_LOGIC ("SISO phase is being terminated by Responder=" << GetAddress ());

                            /* Send BRP Frame with ACK terminating the SISO phase from the responder side */
                            Simulator::Schedule (m_mbifs, &DmgWifiMac::SendEmptyMimoBrpFrame, this, from, ackRequestField,
                                                 ackElement, edmgAckRequestElement);
                          }
                        /* If we have received feedback for MU-MIMO BF send a BRP poll frame to the next user from the MU group or end the SISO phase of MU-MIMO BFT */
                        else if ((m_muMimoBfPhase == MU_SISO_FBCK) && m_muMimoBeamformingTraining)
                          {
                            /* The number of TRN subfields for Rx training in the following MIMO phase will be chosen by the maximum requested from each responder */
                            if (edmgElement->GetRequestedEDMG_TRN_UnitM () > m_edmgTrnM)
                              {
                                m_edmgTrnM = edmgElement->GetRequestedEDMG_TRN_UnitM ();
                              }
                            if (edmgElement->GetL_TX_RX () > m_peerLTxRx)
                              {
                                m_peerLTxRx = edmgElement->GetL_TX_RX ();
                                m_rxPerTxUnits = m_peerLTxRx;
                              }
                            /* Specify that this user should participate in the MIMO phase training - for now, all STAs that gave feedback in the SISO phase
                             * participate in  the MIMO phase. Later we can remove those STAs that are not expected to suffer significant interference. */
                            m_edmgMuGroupMap[peerAid] = true;
                            m_muMimoFbckTimeout.Cancel ();
                            /* If there are more members in the MU-Group that need be polled for feedback go to the next one */
                            if (m_currentMuGroupMember != m_edmgMuGroup.aidList.end ())
                              {
                                Simulator::Schedule (GetMbifs (), &DmgWifiMac::SendBrpFbckPollFrame, this);
                              }
                            else
                              {
                                /* Otherwise the SISO phase of MU MIMO BFT is complete */
                                m_muMimoSisoPhaseComplete (m_muMimoFeedbackMap, m_codebook->GetTotalNumberOfRFChains (),
                                                           m_edmgMuGroup.aidList.size (), m_edmgMuGroup.groupID, m_muMimoBftIdMap [m_edmgMuGroup.groupID]);
                              }
                          }
                      }
                    /* We have received a BRP frame with acknowledgement from the responder terminating the SISO phase of the SU-MIMO BFT */
                    else if (m_isMimoBrpSetupCompleted[from] && element.IsTxTrnOk ())
                      {
                        m_suMimoBfPhase = SU_MIMO_SETUP_PHASE;
                        //Inform the user that SISO phase has completed - he chooses the algorithm to select the candidate and starts the MIMO phase
                        m_suMimoSisoPhaseComplete (from, m_suMimoFeedbackMap, m_codebook->GetCurrentMimoAntennaIdList ().size (),
                                                   m_peerAntennaIds.size (), m_bftIdMap [from]);
                      }
                    /* We have received a BRP transaction frame */
                    else if (m_isMimoBrpSetupCompleted[from] || (m_muMimoBeamformingTraining && m_recordTrnSnrValues))
                      {
                        if (m_suMimoBfPhase == SU_SISO_SETUP_PHASE)
                          {
                            m_suMimoBfPhase = SU_SISO_INITIATOR_TXSS;
                          }
                        if (m_suMimoBfPhase == SU_MIMO_SETUP_PHASE)
                          {
                            m_suMimoBfPhase = SU_MIMO_INITIATOR_SMBT;
                          }
                        if (m_muMimoBfPhase == MU_MIMO_BF_SETUP)
                          {
                            m_muMimoBfPhase = MU_MIMO_BF_TRAINING;
                          }
                        m_brpCdown = edmgElement->GetBRP_CDOWN ();
                        m_peerAntennaIds = edmgElement->GetTX_Antenna_Mask ();
                      }
                  }
                //// NINA ////
                /* We have received a BRP Poll frame from the Initiator of the MU-MIMO BFT asking for SISO feedback */
                if ((element.GetBfTrainingType () == MU_MIMO_BF) && element.IsTxssFbckReq ())
                  {
                    m_muMimoBfPhase = MU_SISO_FBCK;
                    // We received a poll frame from the initiator so cancel the timeout for failure of MU-MIMO BFT
                    if (m_muMimoFbckTimeout.IsRunning ())
                      m_muMimoFbckTimeout.Cancel ();
                    // If there was no previous initiator TXSS initialize variables to start MU-MIMO BFT.
                    if (!m_muMimoBeamformingTraining)
                      {
                        m_muMimoBeamformingTraining = true;
                        m_low->MIMO_BFT_Phase_Started ();
                      }
                    /* Raise a callback that we received a poll for feedback during SISO Fbck */
                    Simulator::Schedule (m_mbifs, &DmgWifiMac::RegisterMuMimoSisoFbckPolled, this, from);
                    //Simulator::Schedule (GetMbifs (), &DmgWifiMac::SendBrpFbckFrame, this, from);
                  }
                /* We are in BRP Transaction state */
                if (requestField.GetTX_TRN_REQ ())
                  {
                    /* We are the responder of BRP-TX, so we record the SNR values of TRN-Tx */
                    m_recordTrnSnrValues = true;
                    m_peerStation = from;
                  }
                else if (element.IsTxTrainResponse ())
                  {
                    /* We received reply for TRN-Tx Training */
                    m_brpCompleted (from, RefineTransmitSector,
                                    m_codebook->GetActiveAntennaID (), m_codebook->GetActiveTxSectorID (), element.GetBsFbck ());
                  }

                //                if (!m_isBrpSetupCompleted[from])
                //                  {
                //                    /* We are in BRP Setup Subphase */
                //                    if (element.IsBeamRefinementInitiator () && element.IsCapabilityRequest ())
                //                      {
                //                        /* We are the Responder of the BRP Setup */
                //                        m_isBrpResponder[from] = true;
                //                        m_isBrpSetupCompleted[from] = false;

                //                        /* Reply back to the Initiator */
                //                        BRP_Request_Field replyRequestField;
                //                        replyRequestField.SetL_RX (m_codebook->GetTotalNumberOfReceiveSectors ());
                //                        replyRequestField.SetTX_TRN_REQ (false);

                //                        BeamRefinementElement replyElement;
                //                        replyElement.SetAsBeamRefinementInitiator (false);
                //                        replyElement.SetCapabilityRequest (false);

                //                        /* Set the antenna config to the best TX config */
                //                        m_feedbackAntennaConfig = GetBestAntennaConfiguration (from, true);
                //                        m_codebook->SetActiveTxSectorID (m_feedbackAntennaConfig.first, m_feedbackAntennaConfig.second);

                //                        NS_LOG_LOGIC ("BRP Setup Subphase is being terminated by Responder=" << GetAddress ()
                //                                      << " at " << Simulator::Now ());

                //                        /* Send BRP Frame terminating the setup phase from the responder side */
                //                        SendBrpFrame (from, replyRequestField, replyElement);
                //                      }
                //                    else if (!element.IsBeamRefinementInitiator () && !element.IsCapabilityRequest ())
                //                      {
                //                        /* BRP Setup subphase is terminated by responder */
                //                        BRP_Request_Field replyRequestField;
                //                        BeamRefinementElement replyElement;
                //                        replyElement.SetAsBeamRefinementInitiator (true);
                //                        replyElement.SetCapabilityRequest (false);

                //                        NS_LOG_LOGIC ("BRP Setup Subphase is being terminated by Initiator=" << GetAddress ()
                //                                      << " at " << Simulator::Now ());

                //                        /* Send BRP Frame terminating the setup phase from the initiator side */
                //                        SendBrpFrame (from, replyRequestField, replyElement);

                //                        /* BRP Setup is terminated */
                //                        m_isBrpSetupCompleted[from] = true;
                //                      }
                //                    else if (element.IsBeamRefinementInitiator () && !element.IsCapabilityRequest ())
                //                      {
                //                        /* BRP Setup subphase is terminated by initiator */
                //                        m_isBrpSetupCompleted[from] = true;

                //                        NS_LOG_LOGIC ("BRP Setup Subphase between Initiator=" << from
                //                                      << " and Responder=" << GetAddress () << " is terminated at " << Simulator::Now ());
                //                      }
                //                  }
                //                else
                //                  {
                //                    NS_LOG_INFO ("Received BRP Transaction Frame from " << from << " at " << Simulator::Now ());

                //                    BRP_Request_Field replyRequestField;
                //                    BeamRefinementElement replyElement;

                //                    /* Check if the BRP Transaction is for us or not */
                //                    m_recordTrnSnrValues = element.IsRxTrainResponse ();

                //                    if (requestField.GetL_RX () > 0)
                //                      {
                //                        /* Receive Beam refinement training is requested, send Rx-Train Response */
                //                        replyElement.SetRxTrainResponse (true);
                //                      }

                //                    if (m_isBrpResponder[from])
                //                      {
                //                        /* Request for Rx-Train Request */
                //                        replyRequestField.SetL_RX (m_codebook->GetTotalNumberOfReceiveSectors ());
                //                        /* Get the address of the peer station we are training our Rx sectors with */
                //                        m_peerStation = from;
                //                      }

                //                    if (replyElement.IsRxTrainResponse ())
                //                      {
                //                        m_requestedBrpTraining = true;
                //                        SendBrpFrame (from, replyRequestField, replyElement, true, TRN_R, requestField.GetL_RX ());
                //                      }

                //                  }
                return;
              }
              // We have received a MIMO BF Setup frame to set up the MIMO phase of SU/MU MIMO beamforming training
              //// NINA ////
            case WifiActionHeader::UNPROTECTED_MIMO_BF_SETUP:
              {
                NS_LOG_LOGIC ("Received MIMO BF Setup frame from " << hdr->GetAddr2 ());
                ExtMimoBfSetupFrame setupFrame;
                packet->RemoveHeader (setupFrame);

                MimoSetupControlElement setupElement;
                setupElement = setupFrame.GetMimoSetupControlElement ();

                if (m_suMimoBeamformingTraining && m_isMimoBrpSetupCompleted[from])
                  {
                    m_edmgTrnM = setupElement.GetRequestedEDMGTRNUnitM ();
                    m_peerLTxRx = setupElement.GetLTxRx ();
                    m_rxPerTxUnits = m_peerLTxRx;
                    m_peerTxSectorCombinationsRequested = setupElement.GetNumberOfTXSectorCombinationsRequested ();
                    m_timeDomainChannelResponseRequested = setupElement.IsChannelMeasurementRequested ();
                    // make sure to do the conversion from bits to actual number of taps requested
                    if (m_timeDomainChannelResponseRequested)
                      m_numberOfTapsRequested = setupElement.GetNumberOfTapsRequested ();
                    // If we are the responder send a MIMO setup frame
                    if (m_isBrpResponder[from])
                      {
                        m_suMimoBfPhase = SU_MIMO_SETUP_PHASE;
                        m_suMimoSisoPhaseComplete (from, m_suMimoFeedbackMap, m_codebook->GetCurrentMimoAntennaIdList ().size (),
                                                   m_peerAntennaIds.size (), m_bftIdMap [from]);
                        m_recordTrnSnrValues = true;
                      }
                    // If we are the initiator start the MIMO BF training Subphase
                    else
                      {
                        m_mimoSnrList.clear ();
                        Simulator::Schedule (m_mbifs, &DmgWifiMac::StartSuMimoBfTrainingSubphase, this);
                      }
                  }
                else if (m_muMimoBeamformingTraining)
                  {
                    m_muMimoBfPhase = MU_MIMO_BF_SETUP;
                    /* If this is the first setup frame we have received and we are a part of the MU Group being
                     * trained that should participate in the MIMO training, set up the codebook for the following training phase
                     * and signal to the MAC and PHY that we should train using the TRN subfields. */
                    if (!m_recordTrnSnrValues && m_edmgMuGroup.groupID == setupElement.GetEDMGGroupID () &&
                        IsIncludedInUserGroup (setupElement.GetGroupUserMask ()).first)
                      {
                        m_mimoSnrList.clear ();
                        GetDmgWifiPhy ()->SetMuMimoBeamformingTraining (true);
                        m_recordTrnSnrValues = true;
                        m_codebook->SetUpMimoBrpTxss (m_codebook->GetTotalAntennaIdList (), from);
                        // for now we train all receive sectors - can't choose candidates since no UL training was done in the SISO phase.
                        bool firstCombination = true;
                        m_codebook->InitializeMimoSectorSweeping (from, ReceiveSectorSweep, firstCombination);
                        for (auto & antenna: m_codebook->GetTotalAntennaIdList ())
                          {
                            ANTENNA_CONFIGURATION_TX antennaConfigTx = std::get<0> (m_bestAntennaConfig[from]);
                            ANTENNA_CONFIGURATION config = std::make_pair (antenna, antennaConfigTx.second);
                            AWV_CONFIGURATION pattern = std::make_pair (config, NO_AWV_ID);
                            m_mimoConfigTraining.push_back (pattern);
                          }
                      }
                  }
                return;
              }
            case WifiActionHeader::UNPROTECTED_MIMO_BF_FEEDBACK:
              {
                NS_LOG_LOGIC ("Received MIMO BF Feedback frame from " << hdr->GetAddr2 ());
                ExtMimoBfFeedbackFrame feedbackFrame;
                packet->RemoveHeader (feedbackFrame);

                MIMOFeedbackControl feedbackElement;
                feedbackElement = feedbackFrame.GetMimoFeedbackControlElement ();

                ChannelMeasurementFeedbackElementList channelList;
                channelList = feedbackFrame.GetListOfChannelMeasurementFeedback ();

                EDMGChannelMeasurementFeedbackElementList edmgChannelList;
                edmgChannelList = feedbackFrame.GetListOfEDMGChannelMeasurementFeedback ();

                if ((m_suMimoBeamformingTraining && m_isMimoBrpSetupCompleted[from]) ||
                    (m_muMimoBeamformingTraining && (m_muMimoBfPhase == MU_MIMO_BF_FBCK)))
                  {
                    // Delete any existing results from previous trainings
                    if (m_suMimoBeamformingTraining)
                      {
                        m_suMimoTxCombinations.erase (from);
                      }
                    uint8_t index = 0;
                    uint16_t sisoIdSubsetIndex = 0;
                    if (m_muMimoBeamformingTraining)
                      {
                        m_muMimoFbckTimeout.Cancel ();
                      }
                    uint8_t peerAid = m_macMap[from];
                    /*  Save the feedback received
                     *  Note: We assume that there is an equal number of Channel Measurement Elements and EDMG Channel Measurement Elements
                     *  and that the Channel Measurement element and the corresponding EDMG channel measurement element at the same position
                     *  in the lists contain feedback for the same number of measurements
                     */
                    uint8_t numberOfTxAntennas = feedbackElement.GetNumberOfTxAntennas ();
                    uint8_t numberOfRxAntennas = feedbackElement.GetNumberOfRxAntennas ();
                    uint8_t numberOfCombinations = feedbackElement.GetNumberOfTXSectorCombinationsPresent ();
                    for (EDMGChannelMeasurementFeedbackElementListI it = edmgChannelList.begin (); it != edmgChannelList.end (); it++)
                      {
                        uint8_t numberOfCombinationsElement;
                        if (numberOfCombinations * numberOfRxAntennas * numberOfTxAntennas > 63)
                          {
                            numberOfCombinationsElement = 63 / (numberOfRxAntennas * numberOfTxAntennas);
                            numberOfCombinations -= numberOfCombinationsElement;
                          }
                        else
                          {
                            numberOfCombinationsElement = numberOfCombinations;
                          }
                        Ptr<ChannelMeasurementFeedbackElement> channelElement = channelList.at (index);
                        EDMGSectorIDOrder_List sectorIdList = (*it)->Get_EDMG_SectorIDOrder_List ();
                        BRP_CDOWN_LIST brpCdownList = (*it)->Get_BRP_CDOWN_List ();
                        SNR_INT_LIST snrList = channelElement->GetSnrList ();
                        //T o do in the future: save the snr of the feedback and use for future hybrid beamforming
                        /* Get the Index of the Tx configurations send, match them to a given Tx Combination and save them
                         * for future MIMO transmissions */
                        for (uint8_t i = 0; i < numberOfCombinationsElement; i++)
                          {
                            Mac48Address address;
                            if (m_suMimoBeamformingTraining)
                              address = from;
                            else
                              address = GetAddress ();
                            uint16_t txId = sectorIdList.at (i * (numberOfRxAntennas * numberOfTxAntennas)).SectorID;
                            uint8_t brpPackets = (std::ceil ((m_codebook->CountMimoNumberOfTxSubfields (address) * m_peerLTxRx)  / 255.0)) - 1;
                            uint8_t trnUnits = 255 / m_peerLTxRx;
                            uint8_t brpCdown = brpCdownList.at (i * (numberOfRxAntennas * numberOfTxAntennas));
                            for (uint8_t j = brpPackets; j > brpCdown; j--)
                              {
                                txId+= trnUnits;
                              }
                            if (m_suMimoBeamformingTraining)
                              {
                                MIMO_AWV_CONFIGURATION txCombination = m_codebook->GetMimoConfigFromTxAwvId (txId, from);
                                BEST_ANTENNA_SU_MIMO_COMBINATIONS::iterator it1 = m_suMimoTxCombinations.find (from);
                                if (it1 != m_suMimoTxCombinations.end ())
                                  {
                                    MIMO_AWV_CONFIGURATIONS *txConfigs = &(it1->second);
                                    txConfigs->push_back (txCombination);
                                  }
                                else
                                  {
                                    MIMO_AWV_CONFIGURATIONS txConfigs;
                                    txConfigs.push_back (txCombination);
                                    m_suMimoTxCombinations[from] = txConfigs;
                                  }
                              }
                            else
                              {
                                auto it = std::find (m_txAwvIdList.begin (), m_txAwvIdList.end (), txId);
                                if (it == m_txAwvIdList.end ())
                                  m_txAwvIdList.push_back (txId);
                                for (uint8_t m = 1; m <= numberOfTxAntennas; m++)
                                  {
                                    for (uint8_t n = 1; n <= numberOfRxAntennas; n++)
                                      {
                                        uint8_t idx = i * (numberOfRxAntennas * numberOfTxAntennas) + (m - 1) * numberOfRxAntennas + (n - 1);
                                        MIMO_FEEDBACK_CONFIGURATION feedbackConfig =
                                            std::make_tuple (sectorIdList.at (idx).TXAntennaID, peerAid, txId);
                                        m_muMimoFeedbackMap [feedbackConfig] = MapIntToSnr (snrList.at (idx));
                                        m_sisoIdSubsetIndexMap [feedbackConfig] = sisoIdSubsetIndex;
                                        sisoIdSubsetIndex++;
                                      }
                                  }
                              }
                          }
                        index++;
                      }
                    // Responder receives feedback from initiator, send feedback to initiator.
                    if (m_suMimoBeamformingTraining && m_isBrpResponder[from])
                      {
                        Simulator::Schedule (GetSifs (), &DmgWifiMac::SendSuMimoBfFeedbackFrame, this);
                      }
                    // Initiator receives feedback from responder, SU-MIMO BF training has been completed.
                    else if (m_suMimoBeamformingTraining)
                      {
                        m_suMimoBeamformingTraining = false;
                        GetDmgWifiPhy ()->SetSuMimoBeamformingTraining (false);
                        m_suMimoBfPhase = SU_WAIT_SU_MIMO_BF_TRAINING;
                        m_codebook->SetReceivingInQuasiOmniMode ();
                        m_dataCommunicationModeTable[from] = DATA_MODE_SU_MIMO;
                        m_low->MIMO_BFT_Phase_Ended ();
                        m_mimoConfigTraining.clear ();
                        m_suMimoMimoPhaseComplete (from);
                      }
                    else if (m_muMimoBeamformingTraining)
                      {
                        if (m_currentMuGroupMember != m_edmgMuGroup.aidList.end ())
                          {
                            Simulator::Schedule (GetSifs (), &DmgWifiMac::SendMimoBfPollFrame, this);
                          }
                        else
                          {
                            Simulator::Schedule (GetMbifs (), &DmgWifiMac::StartMuMimoSelectionSubphase, this);
                          }
                      }
                  }
                return;
              }
            case WifiActionHeader::UNPROTECTED_MIMO_BF_POLL :
              {
                NS_LOG_LOGIC ("Received MIMO BF Poll frame from " << hdr->GetAddr2 ());
                if (GetDmgWifiPhy ()->GetMuMimoBeamformingTraining ())
                  {
                    m_muMimoBfPhase = MU_MIMO_BF_FBCK;
                    Simulator::Schedule (GetSifs (), &DmgWifiMac::SendMuMimoBfFeedbackFrame, this, from);
                  }
                return;
              }
            case WifiActionHeader::UNPROTECTED_MIMO_BF_SELECTION :
              {
                NS_LOG_LOGIC ("Received MIMO BF Selection frame from " << hdr->GetAddr2 ());
                ExtMimoBFSelectionFrame selectionFrame;
                packet->RemoveHeader (selectionFrame);

                MIMOSelectionControlElement element = selectionFrame.GetMIMOSelectionControlElement ();
                // Check if we are a part of the MU Group that this frame is meant for
                if (m_muMimoBeamformingTraining && (element.GetEDMGGroupID () == m_edmgMuGroup.groupID))
                  {
                    m_muMimoRxCombinations.erase (element.GetEDMGGroupID ());
                    std::map<RX_ANTENNA_ID, uint16_t> rxAwvIds;
                    MultiUserTransmissionConfigType muType = element.GetMultiUserTransmissionConfigurationType ();
                    if (muType == MU_NonReciprocal)
                      {
                        NonReciprocalTransmissionConfigList configList = element.GetNonReciprocalTransmissionConfigList ();
                        for (auto & config : configList)
                          {
                            // Find if we are included in this configuration list
                            userMaskConfig maskConfig = IsIncludedInUserGroup (config.nonReciprocalConfigGroupUserMask);
                            if (maskConfig.first)
                              {
                                // Save the Rx Antenna ID and Rx AWV Id that correspond to the Rx configuration the station should use
                                uint16_t sisoIdSubsetIdx = config.configList.at (maskConfig.second - 1);
                                SNR_MEASUREMENT_INDEX measurementIdx = m_sisoIdSubsetIndexRxMap [sisoIdSubsetIdx];
                                uint16_t rxSectorId = measurementIdx.first  % m_rxCombinationsTested;
                                if (rxSectorId == 0)
                                  rxSectorId = m_rxCombinationsTested;
                                /* to do: save the Tx-Rx IDx pair in the phy to signal which signal we should decode */
                                // uint8_t txAntennaIdx =  std::ceil (measurementIdx.second / static_cast <double> (m_codebook->GetCurrentMimoAntennaIdList ().size ()));
                                uint8_t rxAntennaIdx = (measurementIdx.second % m_codebook->GetCurrentMimoAntennaIdList ().size ());
                                if (rxAntennaIdx == 0)
                                  rxAntennaIdx = m_codebook->GetCurrentMimoAntennaIdList ().size ();
                                rxAwvIds [m_codebook->GetCurrentMimoAntennaIdList ().at (rxAntennaIdx - 1)] = rxSectorId;
                              }
                          }
                      }
                    /* Find the MIMO RX combination associated with the Rx Indices and save them for future MIMO transmissions. */
                    MIMO_AWV_CONFIGURATION rxCombination = m_codebook->GetMimoConfigFromRxAwvId (rxAwvIds, from);
                    MU_MIMO_ANTENNA2RESPONDER antenna2responder;
                    m_muMimoOptimalConfig (rxCombination, element.GetEDMGGroupID (), m_muMimoBftIdMap [element.GetEDMGGroupID ()], antenna2responder, false);
                    BEST_ANTENNA_MU_MIMO_COMBINATIONS::iterator it1 = m_muMimoRxCombinations.find (element.GetEDMGGroupID ());
                    if (it1 != m_muMimoRxCombinations.end ())
                      {
                        MIMO_AWV_CONFIGURATIONS *rxConfigs = &(it1->second);
                        rxConfigs->push_back (rxCombination);
                      }
                    else
                      {
                        MIMO_AWV_CONFIGURATIONS rxConfigs;
                        rxConfigs.push_back (rxCombination);
                        m_muMimoRxCombinations[element.GetEDMGGroupID ()] = rxConfigs;
                      }
                    m_muMimoBeamformingTraining = false;
                    GetDmgWifiPhy ()->SetMuMimoBeamformingTraining (false);
                    m_muMimoBfPhase = MU_WAIT_MU_MIMO_BF_TRAINING;
                    m_codebook->SetReceivingInQuasiOmniMode ();
                    m_dataCommunicationModeTable[from] = DATA_MODE_MU_MIMO;
                    m_mimoConfigTraining.clear ();
                    m_low->MIMO_BFT_Phase_Ended ();
                    m_muMimoMimoPhaseComplete ();
                  }
              }

            default:
              packet->AddHeader (actionHdr);
              RegularWifiMac::Receive (mpdu);
              return;
            }
        default:
          packet->AddHeader (actionHdr);
          RegularWifiMac::Receive (mpdu);
          return;
        }
    }

  // Invoke the receive handler of our parent class to deal with any
  // other frames. Specifically, this will handle Block Ack-related
  // Management Action and FST frames.
  RegularWifiMac::Receive (mpdu);
}

} // namespace ns3
