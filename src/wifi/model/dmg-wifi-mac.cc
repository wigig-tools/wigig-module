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

#include "mac-low.h"
#include "mgt-headers.h"
#include "dcf-manager.h"
#include "msdu-aggregator.h"
#include "mpdu-aggregator.h"
#include "mac-tx-middle.h"
#include "wifi-mac-queue.h"
#include "wifi-utils.h"

#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DmgWifiMac");

NS_OBJECT_ENSURE_REGISTERED (DmgWifiMac);

TypeId
DmgWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgWifiMac")
    .SetParent<RegularWifiMac> ()
    .SetGroupName ("Wifi")

    /* PHY Layer Capabilities */
    .AddAttribute ("SupportOfdmPhy", "Whether the DMG STA supports OFDM PHY layer.",
                    BooleanValue (true),
                    MakeBooleanAccessor (&DmgWifiMac::m_supportOFDM),
                    MakeBooleanChecker ())
    .AddAttribute ("SupportLpScPhy", "Whether the DMG STA supports LP-SC PHY layer.",
                    BooleanValue (false),
                    MakeBooleanAccessor (&DmgWifiMac::m_supportLpSc),
                    MakeBooleanChecker ())
    .AddAttribute ("MaxScTxMcs", "The maximum supported MCS for transmission by SC PHY.",
                    UintegerValue (12),
                    MakeUintegerAccessor (&DmgWifiMac::m_maxScTxMcs),
                    MakeUintegerChecker<uint8_t> (4, 12))
    .AddAttribute ("MaxScRxMcs", "The maximum supported MCS for reception by SC PHY.",
                    UintegerValue (12),
                    MakeUintegerAccessor (&DmgWifiMac::m_maxScRxMcs),
                    MakeUintegerChecker<uint8_t> (4, 12))
    .AddAttribute ("MaxOfdmTxMcs", "The maximum supported MCS for transmission by OFDM PHY.",
                    UintegerValue (24),
                    MakeUintegerAccessor (&DmgWifiMac::m_maxOfdmTxMcs),
                    MakeUintegerChecker<uint8_t> (18, 24))
    .AddAttribute ("MaxOfdmRxMcs", "The maximum supported MCS for reception by OFDM PHY.",
                    UintegerValue (24),
                    MakeUintegerAccessor (&DmgWifiMac::m_maxOfdmRxMcs),
                    MakeUintegerChecker<uint8_t> (18, 24))

    /* DMG Operation Element */
    .AddAttribute ("PcpHandoverSupport", "Whether we support PCP Handover.",
                    BooleanValue (false),
                    MakeBooleanAccessor (&DmgWifiMac::GetPcpHandoverSupport,
                                         &DmgWifiMac::SetPcpHandoverSupport),
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

    /* Beacon Interval Traces */
    .AddTraceSource ("DTIStarted", "The Data Transmission Interval access period started.",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_dtiStarted),
                     "ns3::DmgWifiMac::DtiStartedTracedCallback")

    .AddTraceSource ("ServicePeriodStarted", "A service period between two DMG STAs has started.",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_servicePeriodStartedCallback),
                     "ns3::DmgWifiMac::ServicePeriodTracedCallback")
    .AddTraceSource ("ServicePeriodEnded", "A service period between two DMG STAs has ended.",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_servicePeriodEndedCallback),
                     "ns3::DmgWifiMac::ServicePeriodTracedCallback")
    .AddTraceSource ("SLSCompleted", "Sector Level Sweep (SLS) phase is completed",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_slsCompleted),
                     "ns3::DmgWifiMac::SLSCompletedTracedCallback")
    .AddTraceSource ("BRPCompleted", "BRP for transmit/recieve beam refinement is completed",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_brpCompleted),
                     "ns3::DmgWifiMac::BRPCompletedTracedCallback")
    .AddTraceSource ("RlsCompleted",
                     "The Relay Link Setup (RLS) procedure is completed",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_rlsCompleted),
                     "ns3::Mac48Address::TracedCallback")
  ;
  return tid;
}

DmgWifiMac::DmgWifiMac ()
{
  NS_LOG_FUNCTION (this);
  /* DMG Managment DCA-TXOP */
  m_dca->SetTxOkNoAckCallback (MakeCallback (&DmgWifiMac::ManagementTxOk, this));
  /* DMG ATI Access Period Initialization */
  m_dmgAtiDca = CreateObject<DmgAtiDca> ();
  m_dmgAtiDca->SetAifsn (0);
  m_dmgAtiDca->SetMinCw (0);
  m_dmgAtiDca->SetMaxCw (0);
  m_dmgAtiDca->SetLow (m_low);
  m_dmgAtiDca->SetManager (m_dcfManager);
  m_dmgAtiDca->SetTxMiddle (m_txMiddle);
  m_dmgAtiDca->SetTxOkCallback (MakeCallback (&DmgWifiMac::TxOk, this));
  m_dmgAtiDca->SetTxOkNoAckCallback (MakeCallback (&DmgWifiMac::ManagementTxOk, this));
  /* DMG SLS Initialization */
  m_dmgSlsDca = CreateObject<DmgSlsDca> ();
  m_dmgSlsDca->SetAifsn (1);
  m_dmgSlsDca->SetMinCw (0);
  m_dmgSlsDca->SetMaxCw (0);
  m_dmgSlsDca->SetLow (m_low);
  m_dmgSlsDca->SetManager (m_dcfManager);
  m_dmgSlsDca->SetTxMiddle (m_txMiddle);
  m_dmgSlsDca->SetTxOkNoAckCallback (MakeCallback (&DmgWifiMac::FrameTxOk, this));
  m_dmgSlsDca->SetAccessGrantedCallback (MakeCallback (&DmgWifiMac::TxssTxopGranted, this));
}

DmgWifiMac::~DmgWifiMac ()
{
  NS_LOG_FUNCTION (this);
}

void
DmgWifiMac::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_dmgAtiDca = 0;
  m_codebook->Dispose ();
  m_codebook = 0;
  RegularWifiMac::DoDispose ();
}

void
DmgWifiMac::DoInitialize (void)
{
  /* PHY Layer Information */
  if (!m_supportOFDM)
    {
      m_maxOfdmTxMcs = 0;
      m_maxOfdmRxMcs = 0;
    }
  m_requestedBrpTraining = false;
  StaticCast<DmgWifiPhy> (m_phy)->RegisterReportSnrCallback (MakeCallback (&DmgWifiMac::ReportSnrValue, this));
  /* At initialization stage, a DMG STA should be in quasi-omni receiving mode */
  m_codebook->SetReceivingInQuasiOmniMode ();
  /* Channel Access Periods */
  m_dmgAtiDca->Initialize ();
  m_dca->Initialize ();
  /* Initialzie Upper Layers */
  RegularWifiMac::DoInitialize ();
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
  m_dmgAtiDca->SetWifiRemoteStationManager (stationManager);
  m_dca->SetWifiRemoteStationManager (stationManager);
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
DmgWifiMac::StartContentionPeriod (AllocationID allocationID, Time contentionDuration)
{
  NS_LOG_FUNCTION (this << contentionDuration);
  m_currentAllocation = CBAP_ALLOCATION;
  if (GetTypeOfStation () == DMG_STA)
    {
      /* For the time being we assume in CBAP we communicate with the PCP/AP only */
      SteerAntennaToward (GetBssid ());
    }
  /* Allow Contention Access */
  m_dcfManager->AllowChannelAccess ();
  /* Restore previously suspended transmission in LowMac */
  m_low->RestoreAllocationParameters (allocationID);
  /* Signal DCA, EDCA, and SLS DCA Functions to start channel access */
  m_dca->StartAllocationPeriod (CBAP_ALLOCATION, allocationID, GetBssid (), contentionDuration);
  for (EdcaQueues::iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->StartAllocationPeriod (CBAP_ALLOCATION, allocationID, GetBssid (), contentionDuration);
    }
  m_dmgSlsDca->ResumeTxSS ();
  /* Schedule the end of the contention period */
  Simulator::Schedule (contentionDuration, &DmgWifiMac::EndContentionPeriod, this);
  NS_ASSERT_MSG (Simulator::Now () + contentionDuration <= m_dtiStartTime + m_dtiDuration, "Exceeding DTI Time, Error");
}

void
DmgWifiMac::EndContentionPeriod (void)
{
  NS_LOG_FUNCTION (this);
  m_dcfManager->DisableChannelAccess ();
  /* Signal Management DCA to suspend current transmission */
  m_dca->EndAllocationPeriod ();
  /* Signal EDCA queues to suspend current transmission */
  for (EdcaQueues::iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->EndAllocationPeriod ();
    }
  /* Inform MAC Low to store parameters related to this service period (MPDU/A-MPDU) */
  if (!m_low->StoredCurrentAllocation ())
    {
      m_low->StoreAllocationParameters ();
    }
}

void
DmgWifiMac::BeamLinkMaintenanceTimeout (void)
{
  NS_LOG_FUNCTION (this);
  m_beamLinkMaintenanceTimerExpired (m_peerStationAid, m_peerStationAddress, GetRemainingAllocationTime ());
}

void
DmgWifiMac::ScheduleServicePeriod (uint8_t blocks, Time spStart, Time spLength, Time spPeriod,
                                   AllocationID allocationID, uint8_t peerAid, Mac48Address peerAddress, bool isSource)
{
  NS_LOG_FUNCTION (this << blocks << spStart << spLength << spPeriod
                   << static_cast<uint16_t> (allocationID) << static_cast<uint16_t> (peerAid) << peerAddress << isSource);
  /* We allocate multiple blocks of this allocation as in (9.33.6 Channel access in scheduled DTI) */
  /* A_start + (i – 1) × A_period */
  if (spPeriod > 0)
    {
      for (uint8_t i = 0; i < blocks; i++)
        {
          NS_LOG_INFO ("Schedule SP Block [" << i << "] at " << spStart << " till " << spStart + spLength);
          Simulator::Schedule (spStart, &DmgWifiMac::StartServicePeriod, this,
                               allocationID, spLength, peerAid, peerAddress, isSource);
          Simulator::Schedule (spStart + spLength, &DmgWifiMac::EndServicePeriod, this);
          spStart += spLength + spPeriod + GUARD_TIME;
        }
    }
  else
    {
      /* Special case when Allocation Block Period=0 i.e. consecutive blocks *
       * We try to avoid schedulling multiple blocks, so we schedule one big block */
      spLength = spLength * blocks;
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
      m_edca[AC_BE]->InitiateTransmission ();
    }
  /* Check if we are maintaining the beamformed link during this service period */
  BeamLinkMaintenanceTableI it = m_beamLinkMaintenanceTable.find (peerAid);
  if (it != m_beamLinkMaintenanceTable.end ())
    {
      BeamLinkMaintenanceInfo info = it->second;
      m_currentLinkMaintained = true;
      m_currentBeamLinkMaintenanceInfo = info;
      m_beamLinkMaintenanceTimeout = Simulator::Schedule (MicroSeconds (info.beamLinkMaintenanceTime),
                                                          &DmgWifiMac::BeamLinkMaintenanceTimeout, this);
    }
  else
    {
      m_currentLinkMaintained = false;
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
  /* Check if we have beamlink maintenance timer running */
  if (m_beamLinkMaintenanceTimeout.IsRunning ())
    {
      BeamLinkMaintenanceTableI it = m_beamLinkMaintenanceTable.find (m_peerStationAid);
      BeamLinkMaintenanceInfo info = it->second;
      info.beamLinkMaintenanceTime -= m_currentAllocationLength.GetMicroSeconds ();
      m_beamLinkMaintenanceTable[m_peerStationAid] = info;
      m_beamLinkMaintenanceTimeout.Cancel ();
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
  return m_sectorSweepDuration -sswTxTime - (Simulator::Now () - m_sectorSweepStarted);
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
      if (field.IsMaster ())
        {
          uint32_t beamLinkMaintenanceTime;
          if (m_beamlinkMaintenanceUnit == UNIT_32US)
            {
              beamLinkMaintenanceTime = field.GetMaintenanceValue () * 32;
            }
          else
            {
              beamLinkMaintenanceTime = field.GetMaintenanceValue () * 2000;
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
DmgWifiMac::InitiateTxssCbap (Mac48Address peerAddress)
{
  NS_LOG_FUNCTION (this << peerAddress);
  StartTxssTxop (peerAddress, true);
}

void
DmgWifiMac::StartTxssTxop (Mac48Address peerAddress, bool isInitiator)
{
  NS_LOG_FUNCTION (this << peerAddress << isInitiator);
  m_isBeamformingInitiator = isInitiator;
  m_dmgSlsDca->ObtainTxOP (peerAddress, false);
  /* For future use */
//  BF_Control_Field bf;
//  bf.SetBeamformTraining (true);
//  bf.SetAsInitiatorTxss (isInitiatorTxss);
//  bf.SetAsResponderTxss (isResponderTxss);
//  bf.SetTotalNumberOfSectors (m_codebook->GetTotalNumberOfTransmitSectors ());
//  bf.SetNumberOfRxDmgAntennas (m_codebook->GetTotalNumberOfAntennas ());
//  DynamicAllocationInfoField info;
//  info.SetAllocationType (CBAP_ALLOCATION);
//  info.SetSourceAID (GetAssociationID ());
//  info.SetDestinationAID (peerAid);
//  SendGrantFrame (peerAddress, MicroSeconds (3000), info, bf);
}

void
DmgWifiMac::StartSswFeedbackTxop (Mac48Address peerAddress)
{
  NS_LOG_FUNCTION (this << peerAddress);
  m_dmgSlsDca->ObtainTxOP (peerAddress, true);
}

void
DmgWifiMac::TxssTxopGranted (Mac48Address peerAddress, bool feedback)
{
  NS_LOG_FUNCTION (this << peerAddress << feedback);

  NS_ASSERT_MSG (m_currentAllocation == CBAP_ALLOCATION,
                 "Current Allocation is not CBAP and we are performing SLS within CBAP");

  if (!feedback)
    {
      /* Ensure that we have the capabilities of the peer station */
      Ptr<DmgCapabilities> peerCapabilities = GetPeerStationDmgCapabilities (peerAddress);
      NS_ASSERT_MSG (peerCapabilities != 0, "To continue beamforming we should have the capabilities of the peer station.");
      m_peerSectors = peerCapabilities->GetNumberOfSectors ();
      m_peerAntennas = peerCapabilities->GetNumberOfRxDmgAntennas ();

      Time duration = CalculateMySectorSweepDuration ();
      if (Simulator::Now () + duration <= m_dtiStartTime + m_dtiDuration)
        {
          /* Beamforming Allocation Parameters */
          m_allocationStarted = Simulator::Now ();
          m_currentAllocationLength = duration;
          m_peerStationAddress = peerAddress;
          m_isInitiatorTXSS = true;
          m_isResponderTXSS = true;

          if (m_isBeamformingInitiator)
            {
              NS_LOG_INFO ("DMG STA Initiating I-TxSS TxOP with " << peerAddress << " at " << Simulator::Now ());

              /* Remove current Sector Sweep Information with the station we want to train with */
              m_stationSnrMap.erase (peerAddress);

              StartBeamformingInitiatorPhase ();
            }
          else
            {
              NS_LOG_INFO ("DMG STA Initiating R-TxSS TxOP with " << peerAddress << " at " << Simulator::Now ());
              StartBeamformingResponderPhase (m_peerStationAddress);
            }
        }
    }
  else
    {
      Time duration = sswAckTxTime + GetMbifs ();
      SendSswFbckFrame (peerAddress, duration);
    }
}

void
DmgWifiMac::StartBeamformingTraining (uint8_t peerAid, Mac48Address peerAddress,
                                      bool isInitiator, bool isInitiatorTxss, bool isResponderTxss, Time length)
{
  NS_LOG_FUNCTION (this << static_cast<uint16_t> (peerAid) << peerAddress
                   << isInitiator << isInitiatorTxss << isResponderTxss << length);

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
  m_isInitiatorTXSS = isInitiatorTxss;
  m_isResponderTXSS = isResponderTxss;

  /* Remove current Sector Sweep Information */
  m_stationSnrMap.erase (peerAddress);

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
      /* Reset variables */
      m_bfRetryTimes = 0;
      /* Schedule Beamforming Responder Phase */
      Time rssTime = CalculateSectorSweepDuration (m_peerAntennas, m_codebook->GetTotalNumberOfAntennas (),
                                                   m_codebook->GetTotalNumberOfTransmitSectors ()) + GetMbifs ();
      m_rssEvent = Simulator::Schedule (rssTime, &DmgWifiMac::StartBeamformingResponderPhase, this, m_peerStationAddress);
      if (m_isInitiatorTXSS)
        {
          m_slsInitiatorStateMachine = INITIATOR_IDLE;
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
      /* We are the Responder of the Beamforming Phase */
      if (m_isInitiatorTXSS)
        {
          /* I-TxSS so responder stays in Quasi-Omni Receiving Mode */
          m_codebook->StartReceivingInQuasiOmniMode ();
        }
      else
        {
          /* I-RxSS so the responder should have its receive antenna array configured to sweep RXSS
           * Length sectors for each of the initiator’s DMG antennas while attempting to receive SSW
           * frames from the initiator. */
          m_codebook->StartSectorSweeping (m_peerStationAddress, ReceiveSectorSweep, 1);
        }
    }
}

void
DmgWifiMac::StartBeamformingResponderPhase (Mac48Address address)
{
  NS_LOG_FUNCTION (this);
  m_sectorSweepStarted = Simulator::Now ();
  if (m_isBeamformingInitiator)
    {
      /** We are the Initiator **/
      NS_LOG_INFO ("DMG STA Starting RSS Phase with Initiator Role at " << Simulator::Now ());
      if (m_isInitiatorTXSS)
        {
          /* We performed Initiator Transmit Sector Sweep (I-TxSS) */
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
          /* We performed Initiator Receive Sector Sweep (I-RxSS) */
          ANTENNA_CONFIGURATION_RX rxConfig = GetBestAntennaConfiguration (address, false);
          UpdateBestRxAntennaConfiguration (address, rxConfig);
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
          m_feedbackAntennaConfig = GetBestAntennaConfiguration (address, true);
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
      Simulator::Schedule (GetSifs (), &DmgWifiMac::InitiateTxssCbap, this, stationAddress);
    }
}

void
DmgWifiMac::StartTransmitSectorSweep (Mac48Address address, BeamformingDirection direction)
{
  NS_LOG_FUNCTION (this << address << direction);
  NS_LOG_INFO ("DMG STA Starting TxSS at " << Simulator::Now ());
  /* Inform the codebook to Initiate SLS phase */
  m_codebook->StartSectorSweeping (address, TransmitSectorSweep, m_peerAntennas);
  /* Calculate the correct duration for the sector sweep frame */
  m_sectorSweepDuration = CalculateSectorSweepDuration (m_peerAntennas, m_codebook->GetTotalNumberOfAntennas (),
                                                        m_codebook->GetTotalNumberOfTransmitSectors ());
  if (direction == BeamformingInitiator)
    {
      SendInitiatorTransmitSectorSweepFrame (address);
    }
  else if (direction == BeamformingResponder)
    {
      SendRespodnerTransmitSectorSweepFrame (address);
    }
}

void
DmgWifiMac::SendInitiatorTransmitSectorSweepFrame (Mac48Address address)
{
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

  NS_LOG_INFO ("Sending SSW Frame " << Simulator::Now () << " with "
               << static_cast<uint16_t> (ssw.GetSectorID ()) << " " << static_cast<uint16_t> (ssw.GetDMGAntennaID ()));

  /* Transmit control frames directly without DCA + DCF Manager */
  TransmitControlFrame (packet, hdr, GetRemainingSectorSweepTime ());
}

void
DmgWifiMac::SendRespodnerTransmitSectorSweepFrame (Mac48Address address)
{
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
  sswFeedback.SetSector (m_feedbackAntennaConfig.first);
  sswFeedback.SetDMGAntenna (m_feedbackAntennaConfig.second);
  sswFeedback.SetPollRequired (false);

  /* Set the fields in SSW Frame */
  sswFrame.SetSswField (ssw);
  sswFrame.SetSswFeedbackField (sswFeedback);
  packet->AddHeader (sswFrame);

  NS_LOG_INFO ("Sending SSW Frame " << Simulator::Now () << " with "
               << static_cast<uint16_t> (ssw.GetSectorID ()) << " " << static_cast<uint16_t> (ssw.GetDMGAntennaID ()));

  /* Transmit control frames directly without DCA + DCF Manager */
  TransmitControlFrame (packet, hdr, GetRemainingSectorSweepTime ());
}

void
DmgWifiMac::TransmitControlFrame (Ptr<const Packet> packet, WifiMacHeader &hdr, Time duration)
{
  NS_LOG_FUNCTION (this << packet << &hdr << duration);
  if (m_currentAllocation == CBAP_ALLOCATION)
    {
      m_dmgSlsDca->TransmitSswFrame (packet, hdr, duration);
    }
  else
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
  m_low->StartTransmission (packet,
                            &hdr,
                            params,
                            MakeCallback (&DmgWifiMac::FrameTxOk, this));
}

void
DmgWifiMac::StartReceiveSectorSweep (Mac48Address address, BeamformingDirection direction)
{
  NS_LOG_FUNCTION (this << address << direction);
  NS_LOG_INFO ("DMG STA Starting RxSS at " << Simulator::Now ());

  /* A RXSS may be requested only when an initiator/respodner is aware of the capabilities of a responder/initiator, which
   * includes the RXSS Length field. */
  Ptr<DmgCapabilities> peerCapabilities = GetPeerStationDmgCapabilities (address);
  if (peerCapabilities == 0)
    {
      NS_LOG_LOGIC ("Cannot start RxSS since the DMG Capabilities of the peer station is not available");
      return;
    }

  uint8_t rxssLength = peerCapabilities->GetRxssLength ();
  if (direction == BeamformingInitiator)
    {
      /* During the initiator RXSS, the initiator shall transmit from each of the initiator’s DMG antennas the number
       * of BF frames indicated by the responder in the last negotiated RXSS Length field transmitted by the responder.
       * Each transmitted BF frame shall be transmitted with the same fixed antenna sector or pattern. The
       * initiator shall set the Sector ID and DMG Antenna ID fields in each transmitted BF frame to a value that
       * uniquely identifies the single sector through which the BF frame is transmitted. */
      m_totalSectors = (m_codebook->GetTotalNumberOfAntennas () * rxssLength) - 1;
    }
  else
    {
      /* During the responder RXSS, the responder shall transmit the number of SSW frames indicated by the
       * initiator in the initiator’s most recently transmitted RXSS Length field (non-A-BFT) or FSS field (A-BFT)
       * from each of the responder’s DMG antennas, each time with the same antenna sector or pattern fixed for all
       * SSW frames transmission originating from the same DMG antenna. */
      if (m_accessPeriod == CHANNEL_ACCESS_ABFT)
        {
          m_totalSectors = std::min (static_cast<uint> (rxssLength - 1), static_cast<uint> (m_ssFramesPerSlot - 1));
        }
      else
        {
          m_totalSectors = (m_codebook->GetTotalNumberOfAntennas () * rxssLength) - 1;
        }
    }

  STATION_ANTENNA_CONFIG_MAP::iterator it = m_bestAntennaConfig.find (address);
  if (it != m_bestAntennaConfig.end ())
    {
      /* Change Tx Antenna Configuration */
      ANTENNA_CONFIGURATION_TX antennaConfigTx = m_bestAntennaConfig[address].first;
      m_codebook->SetActiveTxSectorID (antennaConfigTx.first, antennaConfigTx.second);
    }
  else
    {
      NS_LOG_DEBUG ("Cannot start RxSS since no antenna configuration available for DMG STA=" << address);
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

  NS_LOG_INFO ("Sending SSW Frame " << Simulator::Now () << " with "
               << static_cast<uint16_t> (m_codebook->GetActiveTxSectorID ()) << " "
               << static_cast<uint16_t> (m_codebook->GetActiveAntennaID ()));

  /* Transmit control frames directly without DCA + DCF Manager */
  TransmitControlFrameImmediately (packet, hdr, GetRemainingSectorSweepTime ());
}

void
DmgWifiMac::SendSswFbckFrame (Mac48Address receiver, Time duration)
{
  NS_LOG_FUNCTION (this << receiver << duration);

  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_CTL_DMG_SSW_FBCK);
  hdr.SetAddr1 (receiver);        // Receiver.
  hdr.SetAddr2 (GetAddress ());   // Transmiter.

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (hdr);

  CtrlDMG_SSW_FBCK fbck;          // SSW-FBCK Frame.
  DMG_SSW_FBCK_Field feedback;    // SSW-FBCK Field.

  /* We are the initiator of SLS */
  if (m_isResponderTXSS)
    {
      /* Responder is TxSS so obtain antenna configuration for the highest received SNR to feed it back */
      m_feedbackAntennaConfig = GetBestAntennaConfiguration (receiver, true);
      feedback.IsPartOfISS (false);
      feedback.SetSector (m_feedbackAntennaConfig.first);
      feedback.SetDMGAntenna (m_feedbackAntennaConfig.second);
    }
  else
    {
      /* At the start of an SSW ACK, the initiator should have its receive antenna array configured to a quasi-omni
       * antenna pattern using the DMG antenna through which it received with the highest quality during the RSS,
       * or the best receive sector if an RXSS has been performed during the RSS, and should not change its receive
       * antenna configuration while it attempts to receive from the responder until the expected end of the SSW ACK. */
      ANTENNA_CONFIGURATION_RX rxConfig = GetBestAntennaConfiguration (receiver, false);
      UpdateBestRxAntennaConfiguration (receiver, rxConfig);
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

  /* Transmit control frames directly without DCA + DCF Manager */
  TransmitControlFrame (packet, hdr, duration);
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
      //if (GetRemainingAllocationTime () >)
      Simulator::Schedule (GetPifs (), &DmgWifiMac::SendSswFbckFrame, this, m_peerStationAddress, GetRemainingAllocationTime ());
    }
  else
    {
      // Failed
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
  Time duration = sswFbckDuration - (GetMbifs () + sswAckTxTime);
  NS_ASSERT (duration.IsPositive ());

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (hdr);

  CtrlDMG_SSW_FBCK ackFrame;      // SSW-ACK Frame.
  DMG_SSW_FBCK_Field feedback;    // SSW-FBCK Field.

  if (m_isInitiatorTXSS)
    {
      /* Initiator is TxSS so obtain antenna configuration for the highest received SNR to feed it back */
      m_feedbackAntennaConfig = GetBestAntennaConfiguration (receiver, true);
      feedback.IsPartOfISS (false);
      feedback.SetSector (m_feedbackAntennaConfig.first);
      feedback.SetDMGAntenna (m_feedbackAntennaConfig.second);
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
DmgWifiMac::PrintSnrConfiguration (SNR_MAP snrMap)
{
  if (snrMap.begin () == snrMap.end ())
    {
      std::cout << "No SNR Information Availalbe" << std::endl;
    }
  else
    {
      for (SNR_MAP::iterator it = snrMap.begin (); it != snrMap.end (); it++)
        {
          ANTENNA_CONFIGURATION config = it->first;
          printf ("AntennaID: %d, SectorID: %2d, SNR: %+2.2f dB\n",
                  config.second, config.first, RatioToDb (it->second));
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
      std::cout << "Tansmit Sector Sweep (TxSS) SNRs: " << std::endl;
      std::cout << "***********************************************" << std::endl;
      PrintSnrConfiguration (snrPair.first);
      std::cout << "***********************************************" << std::endl;
      std::cout << "Receive Sector Sweep (RxSS) SNRs: " << std::endl;
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
DmgWifiMac::MapTxSnr (Mac48Address address, SectorID sectorID, AntennaID antennaID, double snr)
{
  NS_LOG_FUNCTION (this << address << uint16_t (sectorID) << uint16_t (antennaID) << RatioToDb (snr));
  STATION_SNR_PAIR_MAP_I it = m_stationSnrMap.find (address);
  if (it != m_stationSnrMap.end ())
    {
      SNR_PAIR *snrPair = (SNR_PAIR *) (&it->second);
      SNR_MAP_TX *snrMap = (SNR_MAP_TX *) (&snrPair->first);
      ANTENNA_CONFIGURATION config = std::make_pair (sectorID, antennaID);
      (*snrMap)[config] = snr;
    }
  else
    {
      ANTENNA_CONFIGURATION config = std::make_pair (sectorID, antennaID);
      SNR_MAP_TX snrTx;
      SNR_MAP_RX snrRx;
      snrTx[config] = snr;
      SNR_PAIR snrPair = std::make_pair (snrTx, snrRx);
      m_stationSnrMap[address] = snrPair;
    }
}

void
DmgWifiMac::MapRxSnr (Mac48Address address, SectorID sectorID, AntennaID antennaID, double snr)
{
  NS_LOG_FUNCTION (this << address << uint16_t (sectorID) << uint16_t (antennaID) << snr);
  STATION_SNR_PAIR_MAP::iterator it = m_stationSnrMap.find (address);
  if (it != m_stationSnrMap.end ())
    {
      SNR_PAIR *snrPair = (SNR_PAIR *) (&it->second);
      SNR_MAP_RX *snrMap = (SNR_MAP_RX *) (&snrPair->second);
      ANTENNA_CONFIGURATION config = std::make_pair (sectorID, antennaID);
      (*snrMap)[config] = snr;
    }
  else
    {
      ANTENNA_CONFIGURATION config = std::make_pair (sectorID, antennaID);
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

  m_dca->Queue (packet, hdr);
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

  m_dca->Queue (packet, hdr);
}

void
DmgWifiMac::SteerTxAntennaToward (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  STATION_ANTENNA_CONFIG_MAP::iterator it = m_bestAntennaConfig.find (address);
  if (it != m_bestAntennaConfig.end ())
    {
      ANTENNA_CONFIGURATION_TX antennaConfigTx = m_bestAntennaConfig[address].first;
      /* Change Tx Antenna Configuration */
      NS_LOG_DEBUG ("Change Transmit Antenna Sector Config to SectorID=" << static_cast<uint16_t> (antennaConfigTx.first)
                    << ", AntennaID=" << static_cast<uint16_t> (antennaConfigTx.second));
      m_codebook->SetActiveTxSectorID (antennaConfigTx.first, antennaConfigTx.second);
    }
  else
    {
      NS_LOG_DEBUG ("No antenna configuration available for DMG STA=" << address);
    }
}

void
DmgWifiMac::SteerAntennaToward (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  STATION_ANTENNA_CONFIG_MAP::iterator it = m_bestAntennaConfig.find (address);
  if (it != m_bestAntennaConfig.end ())
    {
      ANTENNA_CONFIGURATION_TX antennaConfigTx = m_bestAntennaConfig[address].first;
      ANTENNA_CONFIGURATION_RX antennaConfigRx = m_bestAntennaConfig[address].second;

      /* Change Tx Antenna Configuration */
      NS_LOG_DEBUG ("Change Transmit Antenna Config to SectorID=" << static_cast<uint16_t> (antennaConfigTx.first)
                    << ", AntennaID=" << static_cast<uint16_t> (antennaConfigTx.second));
      m_codebook->SetActiveTxSectorID (antennaConfigTx.first, antennaConfigTx.second);

      /* Change Rx Antenna Configuration */
      if ((antennaConfigRx.first != NO_ANTENNA_CONFIG) && (antennaConfigRx.second != NO_ANTENNA_CONFIG))
        {
          NS_LOG_DEBUG ("Change Receive Antenna Config to SectorID=" << static_cast<uint16_t> (antennaConfigRx.first)
                        << ", AntennaID=" << static_cast<uint16_t> (antennaConfigRx.second));
          m_codebook->SetReceivingInDirectionalMode ();
          m_codebook->SetActiveRxSectorID (antennaConfigRx.first, antennaConfigRx.second);
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

  m_dca->Queue (packet, hdr);
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
  ANTENNA_CONFIGURATION_TX antennaConfigTx = m_bestAntennaConfig[receiver].first;
  m_codebook->SetActiveTxSectorID (antennaConfigTx.first, antennaConfigTx.second);

  NS_LOG_INFO ("Sending BRP Frame to " << receiver << " at " << Simulator::Now ()
               << " with AntennaID=" << static_cast<uint16_t> (antennaConfigTx.second)
               << " SectorID=" << static_cast<uint16_t> (antennaConfigTx.first));

  if (m_accessPeriod == CHANNEL_ACCESS_ATI)
    {
      m_dmgAtiDca->Queue (packet, hdr);
    }
  else
    {
      /* Transmit control frames directly without DCA + DCF Manager */
      TransmitControlFrameImmediately (packet, hdr, MicroSeconds (0));
    }
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
                            double snr, bool isTxTrn)
{
  NS_LOG_FUNCTION (this << uint16_t (antennaID) << uint16_t (sectorID) << uint16_t (subfieldsRemaining)
                   << uint16_t (trnUnitsRemaining) << snr << isTxTrn);
  if (m_recordTrnSnrValues)
    {
      /* Add the SNR of the TRN Subfield */
      m_trn2Snr.push_back (snr);

      /* Check if this is the last TRN Subfield, so we extract the best Tx/RX sector/AWV */
      if ((trnUnitsRemaining == 0) && (subfieldsRemaining == 0))
        {
          /* Iterate over all the SNR values and get the ID of the AWV with the highest AWVs */
          uint8_t awvID = std::distance (m_trn2Snr.begin (), std::max_element ( m_trn2Snr.begin (), m_trn2Snr.end ()));
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

Time
DmgWifiMac::GetSectorSweepDuration (uint8_t sectors)
{
  return ((sswTxTime) * sectors + GetSbifs () * (sectors - 1));
}

Time
DmgWifiMac::GetSectorSweepSlotTime (uint8_t fss)
{
  Time time;
  time = aAirPropagationTime
       + GetSectorSweepDuration (fss)  /* aSSDuration */
       + sswFbckTxTime
       + GetMbifs () * 2;
  time = MicroSeconds (ceil ((double) time.GetNanoSeconds () / 1000));
  return time;
}

Time
DmgWifiMac::CalculateSectorSweepDuration (uint8_t sectors)
{
  Time duration;
  duration = (sectors - 1) * GetSbifs ();
  duration += sectors * sswTxTime;
  return duration;
}

Time
DmgWifiMac::CalculateSectorSweepDuration (uint8_t peerAntennas, uint8_t myAntennas, uint8_t mySectors)
{
  Time duration;
  uint8_t antennas = peerAntennas * myAntennas;
  duration = (antennas - 1) * GetLbifs ();
  duration += (mySectors - antennas) * GetSbifs ();
  duration += peerAntennas * mySectors * sswTxTime;
  return MicroSeconds (ceil (static_cast<double> (duration.GetNanoSeconds ()) / 1000));
}

Time
DmgWifiMac::CalculateMySectorSweepDuration (void)
{
  uint8_t myAntennas = m_codebook->GetTotalNumberOfSectors ();
  uint8_t mySectors = m_codebook->GetTotalNumberOfTransmitSectors ();
  Time duration;
  duration = (myAntennas - 1) * GetLbifs ();
  duration += (mySectors - myAntennas) * GetSbifs ();
  duration += mySectors * sswTxTime;
  return MicroSeconds (ceil (static_cast<double> (duration.GetNanoSeconds ()) / 1000));
}

Time
DmgWifiMac::CalculateBeamformingTrainingDuration (uint8_t initiatorSectors, uint8_t responderSectors)
{
  Time duration;
  duration += (initiatorSectors + responderSectors - 2) * GetSbifs ();
  duration += (initiatorSectors + responderSectors) *  (sswTxTime + aAirPropagationTime);
  duration += sswFbckTxTime + sswAckTxTime + 2 * aAirPropagationTime;
  duration += GetMbifs () * 3;
  return duration;
}

void
DmgWifiMac::StorePeerDmgCapabilities (Ptr<DmgWifiMac> wifiMac)
{
  StationInformation information;
  information.first = wifiMac->GetDmgCapabilities ();
  m_informationMap[wifiMac->GetAddress ()] = information;
  MapAidToMacAddress (wifiMac->GetAssociationID (), wifiMac->GetAddress ());
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

Time
DmgWifiMac::ComputeBeamformingAllocationSize (Mac48Address responderAddress, bool isInitiatorTxss, bool isResponderTxss)
{
  NS_LOG_FUNCTION (this << responderAddress << isInitiatorTxss << isResponderTxss);
  // An initiator shall determine the capabilities of the responder prior to initiating BF training with the
  // responder if the responder is associated. A STA may obtain the capabilities of other STAs through the
  // Information Request and Information Response frames (10.29.1) or following a STA’s association with the
  // PBSS/infrastructure BSS. The initiator should use its own capabilities and the capabilities of the responder
  // to compute the required allocation size to perform BF training and BF training related timeouts.
  Ptr<DmgCapabilities> capabilities = GetPeerStationDmgCapabilities (responderAddress);
  if (capabilities)
    {
      uint8_t initiatorSectors, responderSectors;
      if (isInitiatorTxss && isResponderTxss)
        {
          initiatorSectors = m_codebook->GetTotalNumberOfTransmitSectors ();
          responderSectors = capabilities->GetNumberOfSectors ();
        }
      else if (isInitiatorTxss && !isResponderTxss)
        {
          initiatorSectors = m_codebook->GetTotalNumberOfTransmitSectors ();
          responderSectors = m_codebook->GetTotalNumberOfReceiveSectors ();
        }
      else if (!isInitiatorTxss && isResponderTxss)
        {
          initiatorSectors = capabilities->GetRxssLength ();
          responderSectors = capabilities->GetNumberOfSectors ();
        }
      else
        {
          initiatorSectors = capabilities->GetRxssLength ();
          responderSectors = m_codebook->GetTotalNumberOfReceiveSectors ();
        }
      NS_LOG_DEBUG ("InitiatorSectors=" << uint16_t (initiatorSectors) << ", ResponderSectors=" << uint16_t (responderSectors));
      return CalculateBeamformingTrainingDuration (initiatorSectors, responderSectors);
    }
  else
    {
      return NanoSeconds (0);
    }
}

void
DmgWifiMac::UpdateBestTxAntennaConfiguration (const Mac48Address stationAddress, ANTENNA_CONFIGURATION_TX antennaConfigTx)
{
  STATION_ANTENNA_CONFIG_MAP_I it = m_bestAntennaConfig.find (stationAddress);
  if (it != m_bestAntennaConfig.end ())
    {
      BEST_ANTENNA_CONFIGURATION *antennaConfig = &(it->second);
      antennaConfig->first = antennaConfigTx;
    }
  else
    {
      ANTENNA_CONFIGURATION_RX antennaConfigRx = std::make_pair (NO_ANTENNA_CONFIG, NO_ANTENNA_CONFIG);
      m_bestAntennaConfig[stationAddress] = std::make_pair (antennaConfigTx, antennaConfigRx);
    }
}

void
DmgWifiMac::UpdateBestRxAntennaConfiguration (const Mac48Address stationAddress, ANTENNA_CONFIGURATION_RX antennaConfigRx)
{
  STATION_ANTENNA_CONFIG_MAP_I it = m_bestAntennaConfig.find (stationAddress);
  if (it != m_bestAntennaConfig.end ())
    {
      BEST_ANTENNA_CONFIGURATION *antennaConfig = &(it->second);
      antennaConfig->second = antennaConfigRx;
    }
  else
    {
      ANTENNA_CONFIGURATION_RX antennaConfigTx = std::make_pair (NO_ANTENNA_CONFIG, NO_ANTENNA_CONFIG);
      m_bestAntennaConfig[stationAddress] = std::make_pair (antennaConfigTx, antennaConfigRx);
    }
}

void
DmgWifiMac::UpdateBestAntennaConfiguration (const Mac48Address stationAddress,
                                            ANTENNA_CONFIGURATION_TX txConfig, ANTENNA_CONFIGURATION_RX rxConfig)
{
  STATION_ANTENNA_CONFIG_MAP_I it = m_bestAntennaConfig.find (stationAddress);
  if (it != m_bestAntennaConfig.end ())
    {
      BEST_ANTENNA_CONFIGURATION *antennaConfig = &(it->second);
      antennaConfig->first = txConfig;
      antennaConfig->second = rxConfig;
    }
  else
    {
      m_bestAntennaConfig[stationAddress] = std::make_pair (txConfig, rxConfig);
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
          return antennaConfig.first;
        }
      else
        {
          return antennaConfig.second;
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
  SNR_PAIR snrPair = m_stationSnrMap [stationAddress];
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
  for (SNR_MAP::iterator iter = snrMap.begin (); iter != snrMap.end (); iter++)
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

void
DmgWifiMac::TxOk (Ptr<const Packet> currentPacket, const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this);
  RegularWifiMac::TxOk (currentPacket, hdr);
}

void
DmgWifiMac::ReceiveSectorSweepFrame (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  CtrlDMG_SSW sswFrame;
  packet->RemoveHeader (sswFrame);
  DMG_SSW_Field ssw = sswFrame.GetSswField ();
  DMG_SSW_FBCK_Field sswFeedback = sswFrame.GetSswFeedbackField ();

  if (ssw.GetDirection () == BeamformingInitiator)
    {
      NS_LOG_LOGIC ("Responder: Received SSW frame as part of ISS from Initiator=" << hdr->GetAddr2 ());
      if (m_rssEvent.IsExpired ())
        {
          Time rssTime = hdr->GetDuration () + GetMbifs ();
          if ((m_currentAllocation == CBAP_ALLOCATION) && (ssw.GetRXSSLength () == 0))
            {
              /* We receive the SSW Frame during TxOP in CBAP so we initialize the parameters */
              /* IEEE 802.11-2016 10.38.2.2.3 Initiator RXSS
               * During a CBAP, an initiator shall not obtain a TXOP with an initiator RXSS */
              m_rssEvent = Simulator::Schedule (rssTime, &DmgWifiMac::StartTxssTxop, this, hdr->GetAddr2 (), false);

              /* Remove current Sector Sweep Information with the station we want to train with */
              m_stationSnrMap.erase (hdr->GetAddr2 ());

              NS_LOG_LOGIC ("Initiate TxSS TxOP for Responder=" << GetAddress () << " at " << Simulator::Now () + rssTime);
            }
          else
            {
              m_rssEvent = Simulator::Schedule (rssTime, &DmgWifiMac::StartBeamformingResponderPhase, this, hdr->GetAddr2 ());
              NS_LOG_LOGIC ("Scheduled RSS Period for Responder=" << GetAddress () << " at " << Simulator::Now () + rssTime);
            }
        }

      if (m_isInitiatorTXSS)
        {
          /* Initiator is TxSS and we store SNR to report it back to the initiator */
          MapTxSnr (hdr->GetAddr2 (), ssw.GetSectorID (), ssw.GetDMGAntennaID (), m_stationManager->GetRxSnr ());
        }
      else
        {
          /* Initiator is RxSS and we store SNR to select the best Rx Sector with the initiator */
          MapRxSnr (hdr->GetAddr2 (),
                    m_codebook->GetActiveRxSectorID (),
                    m_codebook->GetActiveAntennaID (),
                    m_stationManager->GetRxSnr ());
//          m_codebook->GetNextSector ();
        }
    }
  else
    {
      NS_LOG_LOGIC ("Initiator: Received SSW frame as part of RSS from Responder=" << hdr->GetAddr2 ());

      /* If we receive one SSW Frame at least, then we schedule SSW-FBCK */
      if (!m_sectorFeedbackSchedulled)
        {
          m_restartISSEvent.Cancel ();
          m_sectorFeedbackSchedulled = true;

          /* The SSW Frame we received is part of RSS */
          /* Not part of ISS i.e. the SSW Feedback Field Contains the Feedbeck of the ISS */
          sswFeedback.IsPartOfISS (false);

          /* Set the best TX antenna configuration reported by the SSW-FBCK Field */
          DMG_SSW_FBCK_Field sswFeedback = sswFrame.GetSswFeedbackField ();

          if (m_isInitiatorTXSS)
            {
              /* The Sector Sweep Frame contains feedback about the the best Tx Sector used by the initiator */
              ANTENNA_CONFIGURATION_TX antennaConfigTx = std::make_pair (sswFeedback.GetSector (), sswFeedback.GetDMGAntenna ());
              UpdateBestTxAntennaConfiguration (hdr->GetAddr2 (), antennaConfigTx);
              NS_LOG_LOGIC ("Best TX Antenna Sector Config by this DMG STA to DMG STA=" << hdr->GetAddr2 ()
                            << ": SectorID=" << static_cast<uint16_t> (antennaConfigTx.first)
                            << ", AntennaID=" << static_cast<uint16_t> (antennaConfigTx.second));
            }

          Time sswFbckTime = GetSectorSweepDuration (ssw.GetCountDown ()) + GetMbifs ();
          if (m_currentAllocation == CBAP_ALLOCATION)
            {
              Simulator::Schedule (sswFbckTime, &DmgWifiMac::StartSswFeedbackTxop, this, hdr->GetAddr2 ());
              NS_LOG_LOGIC ("Scheduled SSW-FBCK TxSS to " << hdr->GetAddr2 ()
                            << " at " << Simulator::Now () + sswFbckTime);
            }
          else
            {
              Simulator::Schedule (sswFbckTime, &DmgWifiMac::SendSswFbckFrame, this, hdr->GetAddr2 (), GetRemainingAllocationTime ());
              NS_LOG_LOGIC ("Scheduled SSW-FBCK Frame to " << hdr->GetAddr2 ()
                            << " at " << Simulator::Now () + sswFbckTime);
            }
        }

      if (m_isResponderTXSS)
        {
          /* Responder is TxSS and we store SNR to report it back to the responder */
          MapTxSnr (hdr->GetAddr2 (), ssw.GetSectorID (), ssw.GetDMGAntennaID (), m_stationManager->GetRxSnr ());
        }
      else
        {
          /* Responder is RxSS and we store SNR to select the best Rx Sector with the responder */
          MapRxSnr (hdr->GetAddr2 (),
                    m_codebook->GetActiveRxSectorID (),
                    m_codebook->GetActiveAntennaID (),
                    m_stationManager->GetRxSnr ());
//          m_codebook->GetNextSector ();
        }
    }
}

void
DmgWifiMac::Receive (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << packet << hdr);
  Mac48Address from = hdr->GetAddr2 ();

  if (hdr->IsSSW ())
    {
      ReceiveSectorSweepFrame (packet, hdr);
      return;
    }
  else if (hdr->IsSSW_ACK ())
    {
      NS_LOG_LOGIC ("Received SSW-ACK frame from=" << hdr->GetAddr2 ());

      /* We are the SLS Initiator */
      CtrlDMG_SSW_ACK sswAck;
      packet->RemoveHeader (sswAck);

      /* Check Beamformed link maintenance */
      RecordBeamformedLinkMaintenanceValue (sswAck.GetBfLinkMaintenanceField ());

      /* We add the station to the list of the stations we can directly communicate with */
      AddForwardingEntry (hdr->GetAddr2 ());

      /* Cancel SSW-Feeback timer */
      m_sswAckTimeoutEvent.Cancel ();

      /* Raise a callback indicating we finished the BRP phase */
      ANTENNA_CONFIGURATION_TX antennaConfigTx = m_bestAntennaConfig[hdr->GetAddr2 ()].first;
      m_slsCompleted (hdr->GetAddr2 (), CHANNEL_ACCESS_DTI, BeamformingInitiator, m_isInitiatorTXSS, m_isResponderTXSS,
                      antennaConfigTx.first, antennaConfigTx.second);

      /* Check if we need to start BRP phase following SLS phase */
      BRP_Request_Field brpRequest = sswAck.GetBrpRequestField ();
      if ((brpRequest.GetL_RX () > 0) || brpRequest.GetTX_TRN_REQ ())
        {
          /* BRP setup sub-phase is skipped in this case*/
          m_executeBRPinATI = false;
          InitiateBrpTransaction (hdr->GetAddr2 (), brpRequest.GetL_RX (), brpRequest.GetTX_TRN_REQ ());
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
            RegularWifiMac::Receive (packet, hdr);
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

            default:
              packet->AddHeader (actionHdr);
              RegularWifiMac::Receive (packet, hdr);
              return;
            }
        default:
          packet->AddHeader (actionHdr);
          RegularWifiMac::Receive (packet, hdr);
          return;
        }
    }

  // Invoke the receive handler of our parent class to deal with any
  // other frames. Specifically, this will handle Block Ack-related
  // Management Action and FST frames.
  RegularWifiMac::Receive (packet, hdr);
}

} // namespace ns3
