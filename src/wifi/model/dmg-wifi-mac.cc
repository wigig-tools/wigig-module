/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Hany Assasa <Hany.assasa@gmail.com>
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

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DmgWifiMac");

NS_OBJECT_ENSURE_REGISTERED (DmgWifiMac);

TypeId
DmgWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgWifiMac")
    .SetParent<RegularWifiMac> ()
    .SetGroupName ("Wifi")
    .AddAttribute ("PcpHandoverSupport", "Whether we support PCP Handover.",
                    BooleanValue (false),
                    MakeBooleanAccessor (&DmgWifiMac::GetPcpHandoverSupport,
                                         &DmgWifiMac::SetPcpHandoverSupport),
                    MakeBooleanChecker ())
    .AddAttribute ("ATIDuration", "The duration of the ATI Period.",
                    TimeValue (MicroSeconds (500)),
                    MakeTimeAccessor (&DmgWifiMac::m_atiDuration),
                    MakeTimeChecker ())
    .AddAttribute ("SupportRDP", "Whether the DMG STA supports Reverse Direction Protocol (RDP)",
                    BooleanValue (false),
                    MakeBooleanAccessor (&DmgWifiMac::m_supportRdp),
                    MakeBooleanChecker ())
    /* Relay support */
    .AddAttribute ("REDSActivated", "Whether the DMG STA is REDS.",
                    BooleanValue (false),
                    MakeBooleanAccessor (&DmgWifiMac::m_redsActivated),
                    MakeBooleanChecker ())
    .AddAttribute ("RDSActivated", "Whether the DMG STA is RDS.",
                    BooleanValue (false),
                    MakeBooleanAccessor (&DmgWifiMac::m_rdsActivated),
                    MakeBooleanChecker ())
//    .AddAttribute ("RelayDuplexMode", "Set relay duplex mode.",
//                    BooleanValue (false),
//                    MakeBooleanAccessor (&DmgWifiMac::RelayDuplexMode),
//                    MakeBooleanChecker ())
    .AddTraceSource ("RlsCompleted",
                     "The RLS procedure has been completed successfully",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_rlsCompleted),
                     "ns3::Mac48Address::TracedCallback")
    .AddTraceSource ("SLSCompleted", "SLS phase is completed",
                     MakeTraceSourceAccessor (&DmgWifiMac::m_slsCompleted),
                     "ns3::Mac48Address::TracedCallback")
  ;
  return tid;
}

DmgWifiMac::DmgWifiMac ()
{
  NS_LOG_FUNCTION (this);

  /* DMG Managment DCA-TXOP */
  m_dca = CreateObject<DcaTxop> ();
  m_dca->SetLow (m_low);
  m_dca->SetManager (m_dcfManager);
  m_dca->SetTxMiddle (m_txMiddle);
  m_dca->SetTxOkCallback (MakeCallback (&DmgWifiMac::TxOk, this));
  m_dca->SetTxOkNoAckCallback (MakeCallback (&DmgWifiMac::ManagementTxOk, this));

  /* DMG Beacon DCF Manager */
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

  m_relayMode = false;
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
  m_requestedBrpTraining = false;
  m_rdsOperational = false;
  m_dmgAtiDca->Initialize ();
  m_dca->Initialize ();
  m_sp->Initialize ();
  m_phy->RegisterReportSnrCallback (MakeCallback (&DmgWifiMac::ReportSnrValue, this));
  /* At initialization stage, a DMG STA should be in Omni receiving mode */
  m_phy->GetDirectionalAntenna ()->SetInOmniReceivingMode ();
  RegularWifiMac::DoInitialize ();
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
DmgWifiMac::SetDMGAntennaCount (uint8_t antennas)
{
  m_antennaCount = antennas;
}

void
DmgWifiMac::SetDMGSectorCount (uint8_t sectors)
{
  m_sectorCount = sectors;
}

uint8_t
DmgWifiMac::GetDMGAntennaCount (void) const
{
  return m_antennaCount;
}

uint8_t
DmgWifiMac::GetDMGSectorCount (void) const
{
  return m_sectorCount;
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
  /* DMG Beaforming IFS */
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
  RegularWifiMac::EnableAggregation ();
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
  ConfigureAggregation ();
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
  if (m_currentAllocation == CBAP_ALLOCATION)
    {
      RegularWifiMac::SendAddBaResponse (reqHdr, originator);
    }
  else
    {
      m_sp->SendAddBaResponse (reqHdr, originator);
    }
}

void
DmgWifiMac::StartContentionPeriod (Time contentionDuration)
{
  NS_LOG_FUNCTION (this);
  m_currentAllocation = CBAP_ALLOCATION;
  m_dcfManager->AllowChannelAccess ();
  /* Signal Management DCA to start channel access*/
  m_dca->InitiateTransmission (contentionDuration);
  /* Signal EDCA queues to start channel access */
  for (EdcaQueues::iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->InitiateTransmission (contentionDuration);
    }
}

void
DmgWifiMac::StartServicePeriod (Time length, Mac48Address peerStation, bool isSource)
{
  NS_LOG_FUNCTION (this << length << peerStation << isSource);
  m_currentAllocation = SERVICE_PERIOD_ALLOCATION;
  m_currentAllocationLength = length;
  m_allocationStarted = Simulator::Now ();
  if (isSource)
    {
      m_sp->InitiateTransmission (peerStation, length);
    }
  else
    {
      ANTENNA_CONFIGURATION_RX antennaConfigRx = m_bestAntennaConfig[peerStation].second;
      if ((antennaConfigRx.first != 0) && (antennaConfigRx.second != 0))
        {
          m_phy->GetDirectionalAntenna ()->SetCurrentRxSectorID (antennaConfigRx.first);
          m_phy->GetDirectionalAntenna ()->SetCurrentRxAntennaID (antennaConfigRx.second);
        }
      else
        {
          m_phy->GetDirectionalAntenna ()->SetInOmniReceivingMode ();
        }
    }
}

void
DmgWifiMac::EndServicePeriod ()
{
  NS_LOG_FUNCTION (this);
  m_sp->DisableChannelAccess ();
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

/**
 * Functions for Relay Discovery/Selection/RLS/Tear Down
 */
Ptr<RelayCapabilitiesElement>
DmgWifiMac::GetRelayCapabilities (void) const
{
  Ptr<RelayCapabilitiesElement> relayElement = Create<RelayCapabilitiesElement> ();
  RelayCapabilitiesInfo info;
  info.SetRelaySupportability (m_rdsActivated);
  info.SetRelayUsability (m_redsActivated);
  info.SetRelayPermission (true);
  info.SetAcPower (true);
  info.SetRelayPreference (true);
  info.SetDuplex (RELAY_FD_AF);
  info.SetCooperation (false);  /* Only Link Switching Type */
  relayElement->SetRelayCapabilitiesInfo (info);
  return relayElement;
}

Ptr<RelayTransferParameterSetElement>
DmgWifiMac::GetRelayTransferParameterSet (void) const
{
  Ptr<RelayTransferParameterSetElement> element = Create<RelayTransferParameterSetElement> ();
  element->SetDuplexMode (true);         /* FD-AF Mode */
  element->SetCooperationMode (false);   /* Link Switching Type */
  element->SetTxMode (false);            /* Normal mode */
  return element;
}

void
DmgWifiMac::SendRelaySearchRequest (uint8_t token, uint16_t destinationAid)
{
  NS_LOG_FUNCTION (this << token << destinationAid);
  WifiMacHeader hdr;
  hdr.SetAction ();
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

void
DmgWifiMac::SendChannelMeasurementRequest (Mac48Address to, uint8_t token)
{
  NS_LOG_FUNCTION (this << to << token);
  WifiMacHeader hdr;
  hdr.SetAction ();
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
DmgWifiMac::SendChannelMeasurementReport (Mac48Address to, uint8_t token, ChannelMeasurementInfoList &measurementList)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
  hdr.SetAction ();
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
DmgWifiMac::SetupRls (Mac48Address to, uint8_t token, uint16_t source_aid, uint16_t relay_aid, uint16_t destination_aid)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
  hdr.SetAction ();
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetBssid ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  ExtRlsRequest requestHdr;
  requestHdr.SetDialogToken (token);
  requestHdr.SetSourceAid (source_aid);
  requestHdr.SetRelayAid (relay_aid);
  requestHdr.SetDestinationAid (destination_aid);

  RelayCapabilitiesInfo info;
  requestHdr.SetSourceCapabilityInformation (info);
  requestHdr.SetRelayCapabilityInformation (info);
  requestHdr.SetDestinationCapabilityInformation (info);
  requestHdr.SetRelayTransferParameterSet (GetRelayTransferParameterSet ());

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
DmgWifiMac::SendRlsResponse (Mac48Address to, uint8_t token)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
  hdr.SetAction ();
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetBssid ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  ExtRlsResponse reponseHdr;
  reponseHdr.SetDialogToken (token);
  reponseHdr.SetDestinationStatusCode (0);
  reponseHdr.SetRelayStatusCode (0);

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
DmgWifiMac::SendRlsAnnouncment (Mac48Address to, uint16_t destination_aid, uint16_t relay_aid, uint16_t source_aid)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
  hdr.SetAction ();
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
DmgWifiMac::SendRelayTeardown (Mac48Address to, uint16_t source_aid, uint16_t destination_aid, uint16_t relay_aid)
{
  NS_LOG_FUNCTION (this << to << source_aid << destination_aid << relay_aid);
  WifiMacHeader hdr;
  hdr.SetAction ();
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetBssid ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();

  ExtRlsTearDown frame;
  frame.SetSourceAid (source_aid);
  frame.SetDestinationAid (destination_aid);
  frame.SetRelayAid (relay_aid);

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
DmgWifiMac::TeardownRelay (Mac48Address to, Mac48Address apAddress, Mac48Address destinationAddress,
                           uint16_t source_aid, uint16_t destination_aid, uint16_t relay_aid)
{
  SendRelayTeardown (to, source_aid, destination_aid, relay_aid);
  SendRelayTeardown (apAddress, source_aid, destination_aid, relay_aid);
  SendRelayTeardown (destinationAddress, source_aid, destination_aid, relay_aid);
}

void
DmgWifiMac::SendSswFbckAfterRss (Mac48Address receiver)
{
  NS_LOG_FUNCTION (this);
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
  maintenance.SetIsMaster (true); /* Master of data transfer */

  fbck.SetSswFeedbackField (feedback);
  fbck.SetBrpRequestField (request);
  fbck.SetBfLinkMaintenanceField (maintenance);

  packet->AddHeader (fbck);
  NS_LOG_INFO ("Sending SSW-FBCK Frame to " << receiver << " at " << Simulator::Now ());

  /* Set the best sector for transmission */
  ANTENNA_CONFIGURATION_TX antennaConfigTx = m_bestAntennaConfig[receiver].first;
  m_phy->GetDirectionalAntenna ()->SetCurrentTxSectorID (antennaConfigTx.first);
  m_phy->GetDirectionalAntenna ()->SetCurrentTxAntennaID (antennaConfigTx.second);

  /* Send Control Frames directly without DCA + DCF Manager */
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

/* Information Request and Response */

void
DmgWifiMac::SendInformationRequest (Mac48Address to, ExtInformationRequest &requestHdr)
{
  NS_LOG_FUNCTION (this);
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
DmgWifiMac::SendInformationRsponse (Mac48Address to, ExtInformationResponse &responseHdr)
{
  NS_LOG_FUNCTION (this);
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
DmgWifiMac::ChangeActiveTxSector (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  STATION_ANTENNA_CONFIG_MAP::iterator it = m_bestAntennaConfig.find (address);
  if (it != m_bestAntennaConfig.end ())
    {
      if (m_relayMode && !m_rdsActivated)
        {
          address = m_selectedRelayAddress;
        }
      ANTENNA_CONFIGURATION_TX antennaConfigTx = m_bestAntennaConfig[address].first;
      m_phy->GetDirectionalAntenna ()->SetCurrentTxSectorID (antennaConfigTx.first);
      m_phy->GetDirectionalAntenna ()->SetCurrentTxAntennaID (antennaConfigTx.second);
      NS_LOG_DEBUG ("Change Transmit Antenna Sector Config to SectorID=" << uint32_t (antennaConfigTx.first)
                    << ", AntennaID=" << uint32_t (antennaConfigTx.second));
    }
  else
    {
      NS_LOG_DEBUG ("No training information available with " << address);
    }
}

void
DmgWifiMac::ChangeActiveRxSector (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  STATION_ANTENNA_CONFIG_MAP::iterator it = m_bestAntennaConfig.find (address);
  if (it != m_bestAntennaConfig.end ())
    {
      ANTENNA_CONFIGURATION_RX antennaConfigRx = m_bestAntennaConfig[address].first;
      m_phy->GetDirectionalAntenna ()->SetInDirectionalReceivingMode ();
      m_phy->GetDirectionalAntenna ()->SetCurrentRxSectorID (antennaConfigRx.first);
      m_phy->GetDirectionalAntenna ()->SetCurrentRxAntennaID (antennaConfigRx.second);
      NS_LOG_DEBUG ("Change Receive Antenna Sector Config to SectorID=" << uint32_t (antennaConfigRx.first)
                    << ", AntennaID=" << uint32_t (antennaConfigRx.second));
    }
  else
    {
      NS_LOG_DEBUG ("No training information available with " << address);
    }
}

void
DmgWifiMac::StayInOmniReceiveMode (void)
{
  m_phy->GetDirectionalAntenna ()->SetInOmniReceivingMode ();
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

/* Currently, we use BRP to train receive sector since we did not have RXSS in A-BFT */
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
  if (hdr.IsAction ())
    {
      WifiActionHeader actionHdr;
      Ptr<Packet> packet = currentPacket->Copy ();
      packet->RemoveHeader (actionHdr);
      switch (actionHdr.GetCategory ())
        {
        case WifiActionHeader::DMG:
          switch (actionHdr.GetAction ().dmgAction)
            {
            case WifiActionHeader::DMG_RLS_RESPONSE:
              {
                if (m_rdsActivated)
                  {
                    /* We are the RDS, so inform the PHY layer to work in Full Duplex Mode */
                    ANTENNA_CONFIGURATION_TX antennaConfigTxSrc = m_bestAntennaConfig[Mac48Address ("00:00:00:00:00:03")].first;
                    ANTENNA_CONFIGURATION_TX antennaConfigTxDst = m_bestAntennaConfig[Mac48Address ("00:00:00:00:00:04")].first;
                    m_phy->ActivateRdsOpereation (antennaConfigTxSrc.first, antennaConfigTxSrc.second,
                                                  antennaConfigTxDst.first, antennaConfigTxDst.second);
                    m_rdsOperational = true;
                  }
                return;
              }
            case WifiActionHeader::DMG_RLS_ANNOUNCEMENT:
              {
                /* We are the Source REDS, so we steer our antenna to RDS */
                ExtRlsAnnouncment rlsAnnouncement;
                packet->RemoveHeader (rlsAnnouncement);
                if (rlsAnnouncement.GetStatusCode () == 0)
                  {
                    ChangeActiveTxSector (m_selectedRelayAddress);
                    m_rlsCompleted (m_selectedRelayAddress);
                  }
                return;
              }
            default:
              break;
            }
        default:
          break;
        }
    }
  RegularWifiMac::TxOk (currentPacket, hdr);
}

void
DmgWifiMac::Receive (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << packet << hdr);

  if (hdr->IsAction () || hdr->IsActionNoAck ())
    {
      WifiActionHeader actionHdr;
      packet->RemoveHeader (actionHdr);

      switch (actionHdr.GetCategory ())
        {
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

                if (!m_isBrpSetupCompleted[hdr->GetAddr2 ()])
                  {
                    /* We are in BRP Setup Subphase */
                    if (element.IsBeamRefinementInitiator () && element.IsCapabilityRequest ())
                      {
                        /* We are the Responder of the BRP Setup */
                        m_isBrpResponder[hdr->GetAddr2 ()] = true;
                        m_isBrpSetupCompleted[hdr->GetAddr2 ()] = false;

                        /* Reply back to the Initiator */
                        BRP_Request_Field replyRequestField;
                        replyRequestField.SetL_RX (m_phy->GetDirectionalAntenna ()->GetNumberOfSectors ());
                        replyRequestField.SetTX_TRN_REQ (false);

                        BeamRefinementElement replyElement;
                        replyElement.SetAsBeamRefinementInitiator (false);
                        replyElement.SetCapabilityRequest (false);

                        /* Set the antenna config to the best TX config */
                        m_feedbackAntennaConfig = GetBestAntennaConfiguration (hdr->GetAddr2 (), true);
                        m_phy->GetDirectionalAntenna ()->SetCurrentTxSectorID (m_feedbackAntennaConfig.first);
                        m_phy->GetDirectionalAntenna ()->SetCurrentTxAntennaID (m_feedbackAntennaConfig.second);

                        NS_LOG_LOGIC ("BRP Setup Subphase is being terminated by Responder=" << GetAddress ()
                                      << " at " << Simulator::Now ());

                        /* Send BRP Frame terminating the setup phase from the responder side */
                        SendBrpFrame (hdr->GetAddr2 (), replyRequestField, replyElement);
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
                        SendBrpFrame (hdr->GetAddr2 (), replyRequestField, replyElement);

                        /* BRP Setup is terminated */
                        m_isBrpSetupCompleted[hdr->GetAddr2 ()] = true;
                      }
                    else if (element.IsBeamRefinementInitiator () && !element.IsCapabilityRequest ())
                      {
                        /* BRP Setup subphase is terminated by initiator */
                        m_isBrpSetupCompleted[hdr->GetAddr2 ()] = true;

                        NS_LOG_LOGIC ("BRP Setup Subphase between Initiator=" << hdr->GetAddr2 ()
                                      << " and Responder=" << GetAddress () << " is terminated at " << Simulator::Now ());
                      }
                  }
                else
                  {
                    NS_LOG_INFO ("Received BRP Transaction Frame from " << hdr->GetAddr2 () << " at " << Simulator::Now ());
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

                    if (m_isBrpResponder[hdr->GetAddr2 ()])
                      {
                        /* Request for Rx-Train Request */
                        replyRequestField.SetL_RX (m_phy->GetDirectionalAntenna ()->GetNumberOfSectors ());
                        /* Get the address of the peer station we are training our Rx sectors with */
                        m_peerStation = hdr->GetAddr2 ();
                      }

                    if (replyElement.IsRxTrainResponse ())
                      {
                        m_requestedBrpTraining = true;
                        SendBrpFrame (hdr->GetAddr2 (), replyRequestField, replyElement, true, TRN_R, requestField.GetL_RX ());
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
  // Management Action frames.
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
      if (snr <= iter->second)
        {
          highIter = iter;
          snr = highIter->second;
          maxSnr = snr;
        }
    }
  return highIter->first;
}

} // namespace ns3
