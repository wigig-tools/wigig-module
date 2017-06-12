/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/pointer.h"
#include "ns3/boolean.h"

#include "dmg-wifi-mac.h"
#include "mgt-headers.h"
#include "mac-low.h"
#include "dcf-manager.h"
#include "msdu-standard-aggregator.h"
#include "mpdu-standard-aggregator.h"
#include "wifi-mac-queue.h"

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

    /* DMG Operation Element */
    .AddAttribute ("AnnounceCapabilities", "Whether we announce DMG Capabilities.",
                    BooleanValue (true),
                    MakeBooleanAccessor (&DmgWifiMac::m_announceDmgCapabilities),
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

    .AddAttribute ("ServicePeriod",
                   "Queue that manages packets belonging to AC_BE access class in the Service Period.",
                   PointerValue (),
                   MakePointerAccessor (&DmgWifiMac::GetServicePeriod),
                   MakePointerChecker<ServicePeriod> ())
    .AddAttribute ("SPQueue",
                   "Queue that manages packets belonging to AC_BE access class in the Service Period.",
                   PointerValue (),
                   MakePointerAccessor (&DmgWifiMac::GetSPQueue),
                   MakePointerChecker<WifiMacQueue> ())

    .AddTraceSource ("ServicePeriodStarted", "A service period between two DMG STAs has started.",
                   MakeTraceSourceAccessor (&DmgWifiMac::m_servicePeriodStartedCallback),
                     "ns3::DmgWifiMac::ServicePeriodTracedCallback")
    .AddTraceSource ("ServicePeriodEnded", "A service period between two DMG STAs has ended.",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_servicePeriodEndedCallback),
                     "ns3::DmgWifiMac::ServicePeriodTracedCallback")
    .AddTraceSource ("SLSCompleted", "SLS phase is completed",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_slsCompleted),
                     "ns3::Mac48Address::TracedCallback")
    .AddTraceSource ("RlsCompleted",
                     "The RLS procedure has been completed successfully",
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

  /* Service Period Access Scheme */
  m_sp = Create<ServicePeriod> ();
  m_sp->SetLow (m_low);
  m_sp->SetTxMiddle (m_txMiddle);
  m_sp->SetTxOkCallback (MakeCallback (&DmgWifiMac::TxOk, this));
  m_sp->SetTxFailedCallback (MakeCallback (&DmgWifiMac::TxFailed, this));
  m_sp->CompleteConfig ();
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
  m_dca = 0;
  m_sp = 0;
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
  m_phy->RegisterReportSnrCallback (MakeCallback (&DmgWifiMac::ReportSnrValue, this));
  /* At initialization stage, a DMG STA should be in quasi-omni receiving mode */
  m_phy->GetDirectionalAntenna ()->SetInOmniReceivingMode ();
  /* Channel Access Periods */
  m_dmgAtiDca->Initialize ();
  m_dca->Initialize ();
  m_sp->Initialize ();
  /* Initialzie Upper Layers */
  RegularWifiMac::DoInitialize ();
}

Ptr<ServicePeriod>
DmgWifiMac::GetServicePeriod () const
{
  return m_sp;
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
  m_sp->SetWifiRemoteStationManager (stationManager);
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
DmgWifiMac::ConfigureAggregation (void)
{
  NS_LOG_FUNCTION (this);
  if (m_sp->GetMsduAggregator () != 0)
    {
      m_sp->GetMsduAggregator ()->SetMaxAmsduSize (m_beMaxAmsduSize);
    }
  if (m_sp->GetMpduAggregator () != 0)
    {
      m_sp->GetMpduAggregator ()->SetMaxAmpduSize (m_beMaxAmpduSize);
    }
  RegularWifiMac::ConfigureAggregation ();
}

void
DmgWifiMac::EnableAggregation (void)
{
  NS_LOG_FUNCTION (this);
  if (m_sp->GetMsduAggregator () == 0)
    {
      Ptr<MsduStandardAggregator> msduAggregator = CreateObject<MsduStandardAggregator> ();
      m_sp->SetMsduAggregator (msduAggregator);
    }
//  if (m_sp->GetMpduAggregator () == 0)
//    {
//      Ptr<MpduStandardAggregator> mpduAggregator = CreateObject<MpduStandardAggregator> ();
//      m_sp->SetMpduAggregator (mpduAggregator);
//    }
  RegularWifiMac::EnableAggregation ();
}

void
DmgWifiMac::DisableAggregation (void)
{
  NS_LOG_FUNCTION (this);
  m_sp->SetMsduAggregator (0);
  m_sp->SetMpduAggregator (0);
  RegularWifiMac::DisableAggregation ();
}

void
DmgWifiMac::SendAddBaResponse (const MgtAddBaRequestHeader *reqHdr, Mac48Address originator)
{
  NS_LOG_FUNCTION (this);
  if ((m_currentAllocation == CBAP_ALLOCATION) || (GetTypeOfStation () == DMG_ADHOC))
    {
      RegularWifiMac::SendAddBaResponse (reqHdr, originator);
    }
  else
    {
//      m_sp->SendAddBaResponse (reqHdr, originator);
    }
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
  /* Restore previously suspended transmission */
  m_low->RestoreAllocationParameters (allocationID);
  if (GetTypeOfStation () == DMG_STA)
    {
      /* For the time being we assume in CBAP we communicate with the PCP/AP only */
      SteerAntennaToward (GetBssid ());
    }
  /* Allow Contention Access */
  m_dcfManager->AllowChannelAccess ();
  /* Signal DCA & EDCA Functions to start channel access */
  m_dca->InitiateTransmission (allocationID, contentionDuration);
  for (EdcaQueues::iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->InitiateTransmission (allocationID, contentionDuration);
    }
}

void
DmgWifiMac::EndContentionPeriod (void)
{
  NS_LOG_FUNCTION (this);
  m_dcfManager->DisableChannelAccess ();
  /* Signal Management DCA to suspend current transmission */
  m_dca->EndCurrentContentionPeriod ();
  /* Signal EDCA queues to suspend current transmission */
  for (EdcaQueues::iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->EndCurrentContentionPeriod ();
    }
  /* Inform MAC Low to store parameters related to this CBAP period (MPDU/A-MPDU) */
  if (!m_low->IsTransmissionSuspended ())
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
                   << uint32_t (allocationID) << uint32_t (peerAid) << peerAddress << isSource);
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
  NS_LOG_FUNCTION (this << length << uint32_t (peerAid) << peerAddress << isSource << Simulator::Now ());
  m_currentAllocationID = allocationID;
  m_currentAllocation = SERVICE_PERIOD_ALLOCATION;
  m_currentAllocationLength = length;
  m_allocationStarted = Simulator::Now ();
  m_peerStationAid = peerAid;
  m_peerStationAddress = peerAddress;
  m_spSource = isSource;
  m_servicePeriodStartedCallback (GetAddress (), peerAddress);
  SteerAntennaToward (peerAddress);
  m_sp->StartServicePeriod (allocationID, peerAddress, length);
  if (isSource)
    {
      m_sp->InitiateTransmission ();
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
  NS_ASSERT (m_currentAllocation == SERVICE_PERIOD_ALLOCATION);
  m_currentAllocationLength = GetRemainingAllocationTime ();
  m_sp->ResumeTransmission (m_currentAllocationLength);
}

void
DmgWifiMac::SuspendServicePeriodTransmission (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_currentAllocation == SERVICE_PERIOD_ALLOCATION);
  m_sp->DisableChannelAccess ();
  m_suspendedPeriodDuration = GetRemainingAllocationTime ();
}

void
DmgWifiMac::EndServicePeriod (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_currentAllocation == SERVICE_PERIOD_ALLOCATION);
  m_servicePeriodEndedCallback (GetAddress (), m_peerStationAddress);
  m_sp->DisableChannelAccess ();
  m_sp->EndCurrentServicePeriod ();
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

Ptr<WifiMacQueue>
DmgWifiMac::GetSPQueue (void) const
{
  return m_sp->GetQueue ();
}

Time
DmgWifiMac::GetRemainingAllocationTime (void) const
{
  return m_currentAllocationLength - (Simulator::Now () - m_allocationStarted);
}

void
DmgWifiMac::MapTxSnr (Mac48Address address, SECTOR_ID sectorID, ANTENNA_ID antennaID, double snr)
{
  NS_LOG_FUNCTION (this << address << uint (sectorID) << uint (antennaID) << snr);
  STATION_SNR_PAIR_MAP::iterator it = m_stationSnrMap.find (address);
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
DmgWifiMac::MapRxSnr (Mac48Address address, SECTOR_ID sectorID, ANTENNA_ID antennaID, double snr)
{
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

void
DmgWifiMac::TransmitControlFrameImmediately (Ptr<const Packet> packet, WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << packet);
  /* Send Frame immediately without DCA + DCF Manager */
  MacLowTransmissionParameters params;
  params.EnableOverrideDurationId (hdr.GetDuration ());
  params.DisableRts ();
  params.DisableAck ();
  params.DisableNextData ();
  m_low->StartTransmission (packet,
                            &hdr,
                            params,
                            MakeCallback (&DmgWifiMac::FrameTxOk, this));
}

void
DmgWifiMac::SendSswFbckAfterRss (Mac48Address receiver)
{
  NS_LOG_FUNCTION (this << receiver);
  /* send a SSW Feedback when you receive a SSW Slot after MBIFS. */
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_CTL_DMG_SSW_FBCK);
  hdr.SetAddr1 (receiver);        // Receiver.
  hdr.SetAddr2 (GetAddress ());   // Transmiter.
  hdr.SetDuration (Seconds (0));  /* The Duration field is set to 0, when the SSW-Feedback frame is transmitted within an A-BFT */

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (hdr);

  CtrlDMG_SSW_FBCK fbck;          // SSW-FBCK Frame.
  DMG_SSW_FBCK_Field feedback;    // SSW-FBCK Field.
  feedback.IsPartOfISS (false);
  /* Obtain antenna configuration for the highest received SNR from DMG STA to feed it back */
  m_feedbackAntennaConfig = GetBestAntennaConfiguration (receiver, true);
  feedback.SetSector (m_feedbackAntennaConfig.first);
  feedback.SetDMGAntenna (m_feedbackAntennaConfig.second);

  BRP_Request_Field request;
  /* Currently, we do not support MID + BC Subphases */
  request.SetMID_REQ (false);
  request.SetBC_REQ (false);
  /* Request for Receive Antenna Training only */
  request.SetL_RX (m_sectorCount);
  request.SetTX_TRN_REQ (false);

  BF_Link_Maintenance_Field maintenance;
  maintenance.SetAsMaster (true); /* Master of data transfer */

  fbck.SetSswFeedbackField (feedback);
  fbck.SetBrpRequestField (request);
  fbck.SetBfLinkMaintenanceField (maintenance);

  packet->AddHeader (fbck);
  NS_LOG_INFO ("Sending SSW-FBCK Frame to " << receiver << " at " << Simulator::Now ());

  /* Set the best sector for transmission */
  ANTENNA_CONFIGURATION_TX antennaConfigTx = m_bestAntennaConfig[receiver].first;
  m_phy->GetDirectionalAntenna ()->SetCurrentTxSectorID (antennaConfigTx.first);
  m_phy->GetDirectionalAntenna ()->SetCurrentTxAntennaID (antennaConfigTx.second);

  TransmitControlFrameImmediately ( packet, hdr);
}

/* Information Request and Response Exchange */

void
DmgWifiMac::SendInformationRequest (Mac48Address to, ExtInformationRequest &requestHdr)
{
  NS_LOG_FUNCTION (this << to);
  WifiMacHeader hdr;
  hdr.SetAction ();
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
  hdr.SetAction ();
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
      m_phy->GetDirectionalAntenna ()->SetCurrentTxSectorID (antennaConfigTx.first);
      m_phy->GetDirectionalAntenna ()->SetCurrentTxAntennaID (antennaConfigTx.second);
      NS_LOG_DEBUG ("Change Transmit Antenna Sector Config to SectorID=" << uint32_t (antennaConfigTx.first)
                    << ", AntennaID=" << uint32_t (antennaConfigTx.second));
      m_phy->GetDirectionalAntenna ()->SetInOmniReceivingMode ();
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
      m_phy->GetDirectionalAntenna ()->SetCurrentTxSectorID (antennaConfigTx.first);
      m_phy->GetDirectionalAntenna ()->SetCurrentTxAntennaID (antennaConfigTx.second);
      NS_LOG_DEBUG ("Change Transmit Antenna Sector Config to SectorID=" << uint32_t (antennaConfigTx.first)
                    << ", AntennaID=" << uint32_t (antennaConfigTx.second));

      /* Change Rx Antenna Configuration */
      if ((antennaConfigRx.first != NO_ANTENNA_CONFIG) && (antennaConfigRx.second != NO_ANTENNA_CONFIG))
        {
          m_phy->GetDirectionalAntenna ()->SetInDirectionalReceivingMode ();
          m_phy->GetDirectionalAntenna ()->SetCurrentRxSectorID (antennaConfigRx.first);
          m_phy->GetDirectionalAntenna ()->SetCurrentRxAntennaID (antennaConfigRx.second);
          NS_LOG_DEBUG ("Change Receive Antenna Sector Config to SectorID=" << uint32_t (antennaConfigRx.first)
                        << ", AntennaID=" << uint32_t (antennaConfigRx.second));
        }
      else
        {
          m_phy->GetDirectionalAntenna ()->SetInOmniReceivingMode ();
        }
    }
  else
    {
      NS_LOG_DEBUG ("No antenna configuration available for DMG STA=" << address);
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
  hdr.SetAction ();
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
 * Functions for Beamrefinement Protocol Setup and Transaction.
 */
void
DmgWifiMac::SendBrpFrame (Mac48Address receiver, BRP_Request_Field &requestField, BeamRefinementElement &element)
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
  hdr.SetActionNoAck ();
  hdr.SetAddr1 (receiver);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();
  /* Special Fields for BeamRefinement */
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
  m_phy->GetDirectionalAntenna ()->SetCurrentTxSectorID (antennaConfigTx.first);
  m_phy->GetDirectionalAntenna ()->SetCurrentTxAntennaID (antennaConfigTx.second);

  NS_LOG_INFO ("Sending BRP Frame to " << receiver << " at " << Simulator::Now ()
               << " with " << uint32_t (m_phy->GetDirectionalAntenna ()->GetCurrentTxSectorID ())
               << " " << uint32_t (m_phy->GetDirectionalAntenna ()->GetCurrentTxAntennaID ()));

  m_dmgAtiDca->Queue (packet, hdr);
}

/*
 *  Currently, we use BRP to train receive sector since we did not have RXSS in A-BFT
 */
void
DmgWifiMac::InitiateBrpSetupSubphase (Mac48Address receiver)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("Initiating BRP Setup Subphase with " << receiver << " at " << Simulator::Now ());

  /* Set flags related to the BRP Setup Phase */
  m_isBrpResponder[receiver] = false;
  m_isBrpSetupCompleted[receiver] = false;
  m_raisedBrpSetupCompleted[receiver] = false;

  BRP_Request_Field requestField;
  /* Currently, we do not support MID + BC Subphases */
  requestField.SetMID_REQ (false);
  requestField.SetBC_REQ (false);
  requestField.SetL_RX (m_phy->GetDirectionalAntenna ()->GetNumberOfSectors ());
  requestField.SetTX_TRN_REQ (false);
  requestField.SetTXSectorID (m_phy->GetDirectionalAntenna ()->GetCurrentTxSectorID ());
  requestField.SetTXAntennaID (m_phy->GetDirectionalAntenna ()->GetCurrentTxAntennaID ());

  BeamRefinementElement element;
  /* The BRP Setup subphase starts with the initiator sending BRP with Capability Request = 1*/
  element.SetAsBeamRefinementInitiator (true);
  element.SetCapabilityRequest (true);

  SendBrpFrame (receiver, requestField, element);
}

void
DmgWifiMac::ReportSnrValue (SECTOR_ID sectorID, ANTENNA_ID antennaID, uint8_t fieldsRemaining, double snr, bool isTxTrn)
{
  NS_LOG_FUNCTION (this << uint (sectorID) << uint (antennaID) << uint (fieldsRemaining) << snr << isTxTrn);
  if (m_recordTrnSnrValues)
    {
      if (isTxTrn)
        {
          MapTxSnr (m_peerStation, sectorID, antennaID, snr);
        }
      else
        {
          MapRxSnr (m_peerStation, sectorID, antennaID, snr);
        }
    }

  /* Check if this is the last TRN-R Field, so we extract the best RX sector */
  if (fieldsRemaining == 0)
    {
      if (!isTxTrn && m_recordTrnSnrValues)
        {
          BEST_ANTENNA_CONFIGURATION *antennaConfig = &m_bestAntennaConfig[m_peerStation];
          ANTENNA_CONFIGURATION_RX rxConfig = GetBestAntennaConfiguration (m_peerStation, false);
          antennaConfig->second = rxConfig;
          m_recordTrnSnrValues = false;
          NS_LOG_INFO ("Received last TRN-R Field, the best RX antenna sector config from " << m_peerStation
                       << " by "  << GetAddress ()
                       << " is SectorID=" << uint (rxConfig.first) << ", AntennaID=" << uint (rxConfig.second));
        }
    }
}

void
DmgWifiMac::InitiateBrpTransaction (Mac48Address receiver)
{
  NS_LOG_FUNCTION (this << receiver);
  NS_LOG_INFO ("Start BRP Transactions with " << receiver << " at " << Simulator::Now ());

  /* We can't start Transcation without BRP Setup subphase */
  NS_ASSERT (m_isBrpSetupCompleted[receiver]);

  BRP_Request_Field requestField;
  requestField.SetMID_REQ (false);
  requestField.SetBC_REQ (false);
  requestField.SetL_RX (m_phy->GetDirectionalAntenna ()->GetNumberOfSectors ());
  requestField.SetTX_TRN_REQ (false);

  BeamRefinementElement element;
  element.SetAsBeamRefinementInitiator (true);
  element.SetCapabilityRequest (false);

  /* Set the antenna config to the best TX config */
  m_feedbackAntennaConfig = GetBestAntennaConfiguration (receiver, true);
  m_phy->GetDirectionalAntenna ()->SetCurrentTxSectorID (m_feedbackAntennaConfig.first);
  m_phy->GetDirectionalAntenna ()->SetCurrentTxAntennaID (m_feedbackAntennaConfig.second);

  /* Transaction Information */
  m_peerStation = receiver;

  /* Send BRP Frame terminating the setup phase from the responder side */
  SendBrpFrame (receiver, requestField, element);
}

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
DmgWifiMac::CalculateBeamformingTrainingDuration (uint8_t peerSectors)
{
  Time duration;
  duration += (GetNumberOfSectors () + peerSectors - 2) * GetSbifs ();
  duration += (GetNumberOfSectors () + peerSectors) *  (sswTxTime + aAirPropagationTime);
  duration += sswFbckTxTime + sswAckTxTime + 2 * aAirPropagationTime;
  duration += GetMbifs () * 3;
  return duration;
}

uint8_t
DmgWifiMac::GetNumberOfSectors (void) const
{
  return m_phy->GetDirectionalAntenna ()->GetNumberOfSectors ();
}

uint8_t
DmgWifiMac::GetNumberOfAntennas (void) const
{
  return m_phy->GetDirectionalAntenna ()->GetNumberOfAntennas ();
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
          /* We finished transmitting BRP Frame in setup phase, switch to omni mode for receiving */
          m_phy->GetDirectionalAntenna ()->SetInOmniReceivingMode ();
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
DmgWifiMac::Receive (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << packet << hdr);
  Mac48Address from = hdr->GetAddr2 ();

  if (hdr->IsAction () || hdr->IsActionNoAck ())
    {
      WifiActionHeader actionHdr;
      packet->RemoveHeader (actionHdr);

      switch (actionHdr.GetCategory ())
        {
        case WifiActionHeader::BLOCK_ACK:
          {
            if ((m_currentAllocation == CBAP_ALLOCATION) || (GetTypeOfStation () == DMG_ADHOC))
              {
                packet->AddHeader (actionHdr);
                RegularWifiMac::Receive (packet, hdr);
                return;
              }
            else
              {
                switch (actionHdr.GetAction ().blockAck)
                  {
                  case WifiActionHeader::BLOCK_ACK_ADDBA_REQUEST:
                    {
                      MgtAddBaRequestHeader reqHdr;
                      packet->RemoveHeader (reqHdr);
//                      m_sp->SendAddBaResponse (&reqHdr, from);
                      return;
                    }
                  case WifiActionHeader::BLOCK_ACK_ADDBA_RESPONSE:
                    {
                      MgtAddBaResponseHeader respHdr;
                      packet->RemoveHeader (respHdr);
//                      m_sp->GotAddBaResponse (&respHdr, from);
                      return;
                    }
                  case WifiActionHeader::BLOCK_ACK_DELBA:
                    {
                      MgtDelBaHeader delBaHdr;
                      packet->RemoveHeader (delBaHdr);
                      if (delBaHdr.IsByOriginator ())
                        {
                          m_low->DestroyBlockAckAgreement (from, delBaHdr.GetTid ());
                        }
                      else
                        {
                          m_sp->GotDelBaFrame (&delBaHdr, from);
                        }
                      return;
                    }
                  default:
                    NS_FATAL_ERROR ("Unsupported Action field in Block Ack Action frame");
                    return;
                  }
              }
          }

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

                if (!m_isBrpSetupCompleted[from])
                  {
                    /* We are in BRP Setup Subphase */
                    if (element.IsBeamRefinementInitiator () && element.IsCapabilityRequest ())
                      {
                        /* We are the Responder of the BRP Setup */
                        m_isBrpResponder[from] = true;
                        m_isBrpSetupCompleted[from] = false;

                        /* Reply back to the Initiator */
                        BRP_Request_Field replyRequestField;
                        replyRequestField.SetL_RX (m_phy->GetDirectionalAntenna ()->GetNumberOfSectors ());
                        replyRequestField.SetTX_TRN_REQ (false);

                        BeamRefinementElement replyElement;
                        replyElement.SetAsBeamRefinementInitiator (false);
                        replyElement.SetCapabilityRequest (false);

                        /* Set the antenna config to the best TX config */
                        m_feedbackAntennaConfig = GetBestAntennaConfiguration (from, true);
                        m_phy->GetDirectionalAntenna ()->SetCurrentTxSectorID (m_feedbackAntennaConfig.first);
                        m_phy->GetDirectionalAntenna ()->SetCurrentTxAntennaID (m_feedbackAntennaConfig.second);

                        NS_LOG_LOGIC ("BRP Setup Subphase is being terminated by Responder=" << GetAddress ()
                                      << " at " << Simulator::Now ());

                        /* Send BRP Frame terminating the setup phase from the responder side */
                        SendBrpFrame (from, replyRequestField, replyElement);
                      }
                    else if (!element.IsBeamRefinementInitiator () && !element.IsCapabilityRequest ())
                      {
                        /* BRP Setup subphase is terminated by responder */
                        BRP_Request_Field replyRequestField;
                        BeamRefinementElement replyElement;
                        replyElement.SetAsBeamRefinementInitiator (true);
                        replyElement.SetCapabilityRequest (false);

                        NS_LOG_LOGIC ("BRP Setup Subphase is being terminated by Initiator=" << GetAddress ()
                                      << " at " << Simulator::Now ());

                        /* Send BRP Frame terminating the setup phase from the initiator side */
                        SendBrpFrame (from, replyRequestField, replyElement);

                        /* BRP Setup is terminated */
                        m_isBrpSetupCompleted[from] = true;
                      }
                    else if (element.IsBeamRefinementInitiator () && !element.IsCapabilityRequest ())
                      {
                        /* BRP Setup subphase is terminated by initiator */
                        m_isBrpSetupCompleted[from] = true;

                        NS_LOG_LOGIC ("BRP Setup Subphase between Initiator=" << from
                                      << " and Responder=" << GetAddress () << " is terminated at " << Simulator::Now ());
                      }
                  }
                else
                  {
                    NS_LOG_INFO ("Received BRP Transaction Frame from " << from << " at " << Simulator::Now ());
                    /* BRP Setup Subphase is completed we are in Transaction state */
                    BRP_Request_Field replyRequestField;
                    BeamRefinementElement replyElement;

                    /* Check if the BRP Transaction is for us or not */
                    m_recordTrnSnrValues = element.IsRxTrainResponse ();

                    if (requestField.GetL_RX () > 0)
                      {
                        /* Receive Beam refinement training is requested, send Rx-Train Response */
                        replyElement.SetRxTrainResponse (true);
                      }

                    if (m_isBrpResponder[from])
                      {
                        /* Request for Rx-Train Request */
                        replyRequestField.SetL_RX (m_phy->GetDirectionalAntenna ()->GetNumberOfSectors ());
                        /* Get the address of the peer station we are training our Rx sectors with */
                        m_peerStation = from;
                      }

                    if (replyElement.IsRxTrainResponse ())
                      {
                        m_requestedBrpTraining = true;
                        SendBrpFrame (from, replyRequestField, replyElement, true, TRN_R, requestField.GetL_RX ());
                      }

                  }
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

DmgWifiMac::ANTENNA_CONFIGURATION
DmgWifiMac::GetBestAntennaConfiguration (const Mac48Address stationAddress, bool isTxConfiguration)
{
  double maxSnr;
  return GetBestAntennaConfiguration (stationAddress, isTxConfiguration, maxSnr);
}

DmgWifiMac::ANTENNA_CONFIGURATION
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

} // namespace ns3
