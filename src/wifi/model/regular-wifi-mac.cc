/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 * Copyright (c) 2015-2019 IMDEA Networks Institute
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Hany Assasa <hany.assasa@gmail.com>
 */

#include "regular-wifi-mac.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "mac-rx-middle.h"
#include "mac-tx-middle.h"
#include "mac-low.h"
#include "dcf-manager.h"
#include "msdu-aggregator.h"
#include "mpdu-aggregator.h"
#include "wifi-utils.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RegularWifiMac");

NS_OBJECT_ENSURE_REGISTERED (RegularWifiMac);

RegularWifiMac::RegularWifiMac () :
  m_htSupported (0),
  m_vhtSupported (0),
  m_erpSupported (0),
  m_dsssSupported (0),
  m_dmgSupported (0),
  m_heSupported (0)
{
  NS_LOG_FUNCTION (this);
  m_rxMiddle = Create<MacRxMiddle> ();
  m_rxMiddle->SetForwardCallback (MakeCallback (&RegularWifiMac::Receive, this));

  m_txMiddle = Create<MacTxMiddle> ();

  m_low = CreateObject<MacLow> ();
  m_low->SetRxCallback (MakeCallback (&MacRxMiddle::Receive, m_rxMiddle));
  m_low->SetMacHigh (this);

  m_dcfManager = CreateObject<DcfManager> ();
  m_dcfManager->SetupLow (m_low);

  m_dca = CreateObject<DcaTxop> ();
  m_dca->SetLow (m_low);
  m_dca->SetManager (m_dcfManager);
  m_dca->SetTxMiddle (m_txMiddle);
  m_dca->SetTxOkCallback (MakeCallback (&RegularWifiMac::TxOk, this));
  m_dca->SetTxFailedCallback (MakeCallback (&RegularWifiMac::TxFailed, this));
  m_dca->SetTxDroppedCallback (MakeCallback (&RegularWifiMac::NotifyTxDrop, this));

  /* FST Variables */
  m_fstId = 0;

  //Construct the EDCAFs. The ordering is important - highest
  //priority (Table 9-1 UP-to-AC mapping; IEEE 802.11-2012) must be created
  //first.
  SetupEdcaQueue (AC_VO);
  SetupEdcaQueue (AC_VI);
  SetupEdcaQueue (AC_BE);
  SetupEdcaQueue (AC_BK);
}

RegularWifiMac::~RegularWifiMac ()
{
  NS_LOG_FUNCTION (this);
}

void
RegularWifiMac::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  m_dca->Initialize ();

  for (EdcaQueues::const_iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->Initialize ();
    }
}

void
RegularWifiMac::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_rxMiddle = 0;
  m_txMiddle = 0;

  m_low->Dispose ();
  m_low = 0;

  m_phy = 0;
  m_stationManager = 0;

  m_dca->Dispose ();
  m_dca = 0;

  for (EdcaQueues::iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->Dispose ();
      i->second = 0;
    }

  m_dcfManager->Dispose ();
  m_dcfManager = 0;
}

void
RegularWifiMac::MacTxOk (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  FstSessionMap::iterator it = m_fstSessionMap.find (address);
  if (it != m_fstSessionMap.end ())
    {
      FstSession *fstSession = static_cast<FstSession*> (&it->second);
      if ((fstSession->CurrentState == FST_SETUP_COMPLETION_STATE) && fstSession->LinkLossCountDownEvent.IsRunning ())
        {
          NS_LOG_INFO ("Transmitted MPDU Successfully, so reset Link Count Down Timer");
          fstSession->LinkLossCountDownEvent.Cancel ();
          fstSession->LinkLossCountDownEvent = Simulator::Schedule (MicroSeconds (fstSession->LLT * 32),
                                                                    &RegularWifiMac::ChangeBand, this,
                                                                    address, fstSession->NewBandId, fstSession->IsInitiator);
        }
    }
}

void
RegularWifiMac::MacRxOk (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  FstSessionMap::iterator it = m_fstSessionMap.find (address);
  if (it != m_fstSessionMap.end ())
    {
      FstSession *fstSession = static_cast<FstSession*> (&it->second);
      if ((fstSession->CurrentState == FST_SETUP_COMPLETION_STATE) && fstSession->LinkLossCountDownEvent.IsRunning ())
        {
          NS_LOG_INFO ("Received MPDU Successfully, so reset Link Count Down Timer");
          fstSession->LinkLossCountDownEvent.Cancel ();
          fstSession->LinkLossCountDownEvent = Simulator::Schedule (MicroSeconds (fstSession->LLT * 32),
                                                                    &RegularWifiMac::ChangeBand, this,
                                                                    address, fstSession->NewBandId, fstSession->IsInitiator);
        }
    }
}

void
RegularWifiMac::SetWifiRemoteStationManager (const Ptr<WifiRemoteStationManager> stationManager)
{
  NS_LOG_FUNCTION (this << stationManager);
  m_stationManager = stationManager;

  /* Connect trace source for FST */
  m_stationManager->RegisterTxOkCallback (MakeCallback (&RegularWifiMac::MacTxOk, this));
  m_stationManager->RegisterRxOkCallback (MakeCallback (&RegularWifiMac::MacRxOk, this));

  m_stationManager->SetHtSupported (GetHtSupported ());
  m_stationManager->SetVhtSupported (GetVhtSupported ());
  m_stationManager->SetHeSupported (GetHeSupported ());
  m_stationManager->SetDmgSupported (GetDmgSupported ());
  m_low->SetWifiRemoteStationManager (stationManager);

  m_dca->SetWifiRemoteStationManager (stationManager);

  for (EdcaQueues::const_iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->SetWifiRemoteStationManager (stationManager);
    }
}

Ptr<WifiRemoteStationManager>
RegularWifiMac::GetWifiRemoteStationManager () const
{
  return m_stationManager;
}

void
RegularWifiMac::SetVoMaxAmsduSize (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_voMaxAmsduSize = size;
  ConfigureAggregation ();
}

void
RegularWifiMac::SetViMaxAmsduSize (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_viMaxAmsduSize = size;
  ConfigureAggregation ();
}

void
RegularWifiMac::SetBeMaxAmsduSize (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_beMaxAmsduSize = size;
  ConfigureAggregation ();
}

void
RegularWifiMac::SetBkMaxAmsduSize (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_bkMaxAmsduSize = size;
  ConfigureAggregation ();
}

void
RegularWifiMac::SetVoMaxAmpduSize (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_voMaxAmpduSize = size;
  ConfigureAggregation ();
}

void
RegularWifiMac::SetViMaxAmpduSize (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_viMaxAmpduSize = size;
  ConfigureAggregation ();
}

void
RegularWifiMac::SetBeMaxAmpduSize (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_beMaxAmpduSize = size;
  ConfigureAggregation ();
}

void
RegularWifiMac::SetBkMaxAmpduSize (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_bkMaxAmpduSize = size;
  ConfigureAggregation ();
}

void
RegularWifiMac::SetVoBlockAckThreshold (uint8_t threshold)
{
  NS_LOG_FUNCTION (this << +threshold);
  GetVOQueue ()->SetBlockAckThreshold (threshold);
}

void
RegularWifiMac::SetViBlockAckThreshold (uint8_t threshold)
{
  NS_LOG_FUNCTION (this << +threshold);
  GetVIQueue ()->SetBlockAckThreshold (threshold);
}

void
RegularWifiMac::SetBeBlockAckThreshold (uint8_t threshold)
{
  NS_LOG_FUNCTION (this << +threshold);
  GetBEQueue ()->SetBlockAckThreshold (threshold);
}

void
RegularWifiMac::SetBkBlockAckThreshold (uint8_t threshold)
{
  NS_LOG_FUNCTION (this << +threshold);
  GetBKQueue ()->SetBlockAckThreshold (threshold);
}

void
RegularWifiMac::SetVoBlockAckInactivityTimeout (uint16_t timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  GetVOQueue ()->SetBlockAckInactivityTimeout (timeout);
}

void
RegularWifiMac::SetViBlockAckInactivityTimeout (uint16_t timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  GetVIQueue ()->SetBlockAckInactivityTimeout (timeout);
}

void
RegularWifiMac::SetBeBlockAckInactivityTimeout (uint16_t timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  GetBEQueue ()->SetBlockAckInactivityTimeout (timeout);
}

void
RegularWifiMac::SetBkBlockAckInactivityTimeout (uint16_t timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  GetBKQueue ()->SetBlockAckInactivityTimeout (timeout);
}

void
RegularWifiMac::SetupEdcaQueue (AcIndex ac)
{
  NS_LOG_FUNCTION (this << ac);

  //Our caller shouldn't be attempting to setup a queue that is
  //already configured.
  NS_ASSERT (m_edca.find (ac) == m_edca.end ());

  Ptr<EdcaTxopN> edca = CreateObject<EdcaTxopN> ();
  edca->SetLow (m_low);
  edca->SetManager (m_dcfManager);
  edca->SetTxMiddle (m_txMiddle);
  edca->SetTxOkCallback (MakeCallback (&RegularWifiMac::TxOk, this));
  edca->SetTxFailedCallback (MakeCallback (&RegularWifiMac::TxFailed, this));
  edca->SetTxDroppedCallback (MakeCallback (&RegularWifiMac::NotifyTxDrop, this));
  edca->SetAccessCategory (ac);
  edca->CompleteConfig ();

  m_edca.insert (std::make_pair (ac, edca));
}

void
RegularWifiMac::SetTypeOfStation (TypeOfStation type)
{
  NS_LOG_FUNCTION (this << type);
  for (EdcaQueues::const_iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->SetTypeOfStation (type);
    }
  m_type = type;
}

TypeOfStation
RegularWifiMac::GetTypeOfStation (void) const
{
  NS_LOG_FUNCTION (this);
  return m_type;
}

Ptr<DcaTxop>
RegularWifiMac::GetDcaTxop () const
{
  return m_dca;
}

Ptr<EdcaTxopN>
RegularWifiMac::GetVOQueue () const
{
  return m_edca.find (AC_VO)->second;
}

Ptr<EdcaTxopN>
RegularWifiMac::GetVIQueue () const
{
  return m_edca.find (AC_VI)->second;
}

Ptr<EdcaTxopN>
RegularWifiMac::GetBEQueue () const
{
  return m_edca.find (AC_BE)->second;
}

Ptr<EdcaTxopN>
RegularWifiMac::GetBKQueue () const
{
  return m_edca.find (AC_BK)->second;
}

void
RegularWifiMac::SetWifiPhy (const Ptr<WifiPhy> phy)
{
  NS_LOG_FUNCTION (this << phy);
  m_phy = phy;
  m_dcfManager->SetupPhyListener (phy);
  m_low->SetPhy (phy);
}

Ptr<WifiPhy>
RegularWifiMac::GetWifiPhy (void) const
{
  NS_LOG_FUNCTION (this);
  return m_phy;
}

void
RegularWifiMac::ResetWifiPhy (void)
{
  NS_LOG_FUNCTION (this);
  m_low->ResetPhy ();
  m_dcfManager->RemovePhyListener (m_phy);
  m_phy = 0;
}

void
RegularWifiMac::SetForwardUpCallback (ForwardUpCallback upCallback)
{
  NS_LOG_FUNCTION (this);
  m_forwardUp = upCallback;
}

void
RegularWifiMac::SetLinkUpCallback (Callback<void> linkUp)
{
  NS_LOG_FUNCTION (this);
  m_linkUp = linkUp;
}

void
RegularWifiMac::SetLinkDownCallback (Callback<void> linkDown)
{
  NS_LOG_FUNCTION (this);
  m_linkDown = linkDown;
}

void
RegularWifiMac::SetQosSupported (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_qosSupported = enable;
}

bool
RegularWifiMac::GetQosSupported () const
{
  return m_qosSupported;
}

void
RegularWifiMac::SetVhtSupported (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_vhtSupported = enable;
  if (enable)
    {
      SetQosSupported (true);
    }
  if (!enable && !m_htSupported)
    {
      DisableAggregation ();
    }
  else
    {
      EnableAggregation ();
    }
}

void
RegularWifiMac::SetHtSupported (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_htSupported = enable;
  if (enable)
    {
      SetQosSupported (true);
    }
  if (!enable && !m_vhtSupported)
    {
      DisableAggregation ();
    }
  else
    {
      EnableAggregation ();
    }
}

void
RegularWifiMac::SetHeSupported (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_heSupported = enable;
  if (enable)
    {
      SetQosSupported (true);
    }
  if (!enable && !m_htSupported && !m_vhtSupported)
    {
      DisableAggregation ();
    }
  else
    {
      EnableAggregation ();
    }
}

bool
RegularWifiMac::GetVhtSupported () const
{
  return m_vhtSupported;
}

bool
RegularWifiMac::GetHtSupported () const
{
  return m_htSupported;
}

bool
RegularWifiMac::GetHeSupported () const
{
  return m_heSupported;
}

bool
RegularWifiMac::GetDmgSupported () const
{
  return m_dmgSupported;
}

void
RegularWifiMac::SetDmgSupported (bool enable)
{
  NS_LOG_FUNCTION (this);
  m_dmgSupported = enable;
  if (enable)
    {
      SetQosSupported (true);
      EnableAggregation ();
    }
  else
    {
      DisableAggregation ();
    }
}

bool
RegularWifiMac::GetErpSupported () const
{
  return m_erpSupported;
}

void
RegularWifiMac::SetErpSupported (bool enable)
{
  NS_LOG_FUNCTION (this);
  if (enable)
    {
      SetDsssSupported (true);
    }
  m_erpSupported = enable;
}

void
RegularWifiMac::SetDsssSupported (bool enable)
{
  NS_LOG_FUNCTION (this);
  m_dsssSupported = enable;
}

bool
RegularWifiMac::GetDsssSupported () const
{
  return m_dsssSupported;
}

void
RegularWifiMac::SetCtsToSelfSupported (bool enable)
{
  NS_LOG_FUNCTION (this);
  m_low->SetCtsToSelfSupported (enable);
}

bool
RegularWifiMac::GetCtsToSelfSupported () const
{
  return m_low->GetCtsToSelfSupported ();
}

void
RegularWifiMac::SetSlot (Time slotTime)
{
  NS_LOG_FUNCTION (this << slotTime);
  m_dcfManager->SetSlot (slotTime);
  m_low->SetSlotTime (slotTime);
}

Time
RegularWifiMac::GetSlot (void) const
{
  return m_low->GetSlotTime ();
}

void
RegularWifiMac::SetSifs (Time sifs)
{
  NS_LOG_FUNCTION (this << sifs);
  m_dcfManager->SetSifs (sifs);
  m_low->SetSifs (sifs);
}

Time
RegularWifiMac::GetSifs (void) const
{
  return m_low->GetSifs ();
}

void
RegularWifiMac::SetEifsNoDifs (Time eifsNoDifs)
{
  NS_LOG_FUNCTION (this << eifsNoDifs);
  m_dcfManager->SetEifsNoDifs (eifsNoDifs);
}

Time
RegularWifiMac::GetEifsNoDifs (void) const
{
  return m_dcfManager->GetEifsNoDifs ();
}

void
RegularWifiMac::SetRifs (Time rifs)
{
  NS_LOG_FUNCTION (this << rifs);
  m_low->SetRifs (rifs);
}

Time
RegularWifiMac::GetRifs (void) const
{
  return m_low->GetRifs ();
}

void
RegularWifiMac::SetPifs (Time pifs)
{
  NS_LOG_FUNCTION (this << pifs);
  m_low->SetPifs (pifs);
}

Time
RegularWifiMac::GetPifs (void) const
{
  return m_low->GetPifs ();
}

void
RegularWifiMac::SetAckTimeout (Time ackTimeout)
{
  NS_LOG_FUNCTION (this << ackTimeout);
  m_low->SetAckTimeout (ackTimeout);
}

Time
RegularWifiMac::GetAckTimeout (void) const
{
  return m_low->GetAckTimeout ();
}

void
RegularWifiMac::SetCtsTimeout (Time ctsTimeout)
{
  NS_LOG_FUNCTION (this << ctsTimeout);
  m_low->SetCtsTimeout (ctsTimeout);
}

Time
RegularWifiMac::GetCtsTimeout (void) const
{
  return m_low->GetCtsTimeout ();
}

void
RegularWifiMac::SetBasicBlockAckTimeout (Time blockAckTimeout)
{
  NS_LOG_FUNCTION (this << blockAckTimeout);
  m_low->SetBasicBlockAckTimeout (blockAckTimeout);
}

Time
RegularWifiMac::GetBasicBlockAckTimeout (void) const
{
  return m_low->GetBasicBlockAckTimeout ();
}

void
RegularWifiMac::SetCompressedBlockAckTimeout (Time blockAckTimeout)
{
  NS_LOG_FUNCTION (this << blockAckTimeout);
  m_low->SetCompressedBlockAckTimeout (blockAckTimeout);
}

Time
RegularWifiMac::GetCompressedBlockAckTimeout (void) const
{
  return m_low->GetCompressedBlockAckTimeout ();
}

void
RegularWifiMac::SetAddress (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_low->SetAddress (address);
}

Mac48Address
RegularWifiMac::GetAddress (void) const
{
  return m_low->GetAddress ();
}

void
RegularWifiMac::SetSsid (Ssid ssid)
{
  NS_LOG_FUNCTION (this << ssid);
  m_ssid = ssid;
}

Ssid
RegularWifiMac::GetSsid (void) const
{
  return m_ssid;
}

void
RegularWifiMac::SetBssid (Mac48Address bssid)
{
  NS_LOG_FUNCTION (this << bssid);
  m_low->SetBssid (bssid);
}

Mac48Address
RegularWifiMac::GetBssid (void) const
{
  return m_low->GetBssid ();
}

enum MacState
RegularWifiMac::GetMacState (void) const
{
  return m_state;
}

void
RegularWifiMac::SetPromisc (void)
{
  m_low->SetPromisc ();
}

void
RegularWifiMac::SetShortSlotTimeSupported (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_shortSlotTimeSupported = enable;
}

bool
RegularWifiMac::GetShortSlotTimeSupported (void) const
{
  return m_shortSlotTimeSupported;
}

void
RegularWifiMac::SetRifsSupported (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_rifsSupported = enable;
}

bool
RegularWifiMac::GetRifsSupported (void) const
{
  return m_rifsSupported;
}

void
RegularWifiMac::Enqueue (Ptr<const Packet> packet,
                         Mac48Address to, Mac48Address from)
{
  //We expect RegularWifiMac subclasses which do support forwarding (e.g.,
  //AP) to override this method. Therefore, we throw a fatal error if
  //someone tries to invoke this method on a class which has not done
  //this.
  NS_FATAL_ERROR ("This MAC entity (" << this << ", " << GetAddress ()
                                      << ") does not support Enqueue() with from address");
}

bool
RegularWifiMac::SupportsSendFrom (void) const
{
  return false;
}

void
RegularWifiMac::ForwardUp (Ptr<Packet> packet, Mac48Address from, Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << from << to);
  m_forwardUp (packet, from, to);
}

/**
 * Functions for Fast Session Transfer.
 */
void
RegularWifiMac::SetupFSTSession (Mac48Address staAddress)
{
  NS_LOG_FUNCTION (this << staAddress);

  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (staAddress);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  ExtFstSetupRequest requestHdr;

  /* Generate new FST Session ID */
  m_fstId++;

  SessionTransitionElement sessionTransition;
  Band newBand, oldBand;
  newBand.Band_ID = Band_4_9GHz;
  newBand.Setup = 1;
  newBand.Operation = 1;
  sessionTransition.SetNewBand (newBand);
  oldBand.Band_ID = Band_60GHz;
  oldBand.Setup = 1;
  oldBand.Operation = 1;
  sessionTransition.SetOldBand (oldBand);
  sessionTransition.SetFstsID (m_fstId);
  sessionTransition.SetSessionControl (SessionType_InfrastructureBSS, false);

  requestHdr.SetSessionTransition (sessionTransition);
  requestHdr.SetLlt (m_llt);
  requestHdr.SetMultiBand (GetMultiBandElement ());
  requestHdr.SetDialogToken (10);

  /* We are the initiator of the FST session */
  FstSession fstSession;
  fstSession.ID = m_fstId;
  fstSession.CurrentState = FST_INITIAL_STATE;
  fstSession.IsInitiator = true;
  fstSession.NewBandId = Band_4_9GHz;
  fstSession.LLT = m_llt;
  m_fstSessionMap[staAddress] = fstSession;

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.fstAction = WifiActionHeader::FST_SETUP_REQUEST;
  actionHdr.SetAction (WifiActionHeader::FST, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (requestHdr);
  packet->AddHeader (actionHdr);

  m_dca->Queue (packet, hdr);
}

void
RegularWifiMac::SendFstSetupResponse (Mac48Address to, uint8_t token, uint16_t status, SessionTransitionElement sessionTransition)
{
  NS_LOG_FUNCTION (this << to << token << status);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  ExtFstSetupResponse responseHdr;
  responseHdr.SetDialogToken (token);
  responseHdr.SetStatusCode (status);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.fstAction = WifiActionHeader::FST_SETUP_RESPONSE;
  actionHdr.SetAction (WifiActionHeader::FST, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (responseHdr);
  packet->AddHeader (actionHdr);

  m_dca->Queue (packet, hdr);
}

void
RegularWifiMac::SendFstAckRequest (Mac48Address to, uint8_t dialog, uint32_t fstsID)
{
  NS_LOG_FUNCTION (this << to << dialog);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  ExtFstAckResponse requestHdr;
  requestHdr.SetDialogToken (dialog);
  requestHdr.SetFstsID (fstsID);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.fstAction = WifiActionHeader::FST_ACK_REQUEST;
  actionHdr.SetAction (WifiActionHeader::FST, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (requestHdr);
  packet->AddHeader (actionHdr);

  m_dca->Queue (packet, hdr);
}

void
RegularWifiMac::SendFstAckResponse (Mac48Address to, uint8_t dialog, uint32_t fstsID)
{
  NS_LOG_FUNCTION (this << to << dialog);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  ExtFstAckResponse responseHdr;
  responseHdr.SetDialogToken (dialog);
  responseHdr.SetFstsID (fstsID);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.fstAction = WifiActionHeader::FST_ACK_RESPONSE;
  actionHdr.SetAction (WifiActionHeader::FST, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (responseHdr);
  packet->AddHeader (actionHdr);

  m_dca->Queue (packet, hdr);
}

void
RegularWifiMac::SendFstTearDownFrame (Mac48Address to)
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

  ExtFstTearDown frame;
  frame.SetFstsID (0);

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.fstAction = WifiActionHeader::FST_TEAR_DOWN;
  actionHdr.SetAction (WifiActionHeader::FST, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (frame);
  packet->AddHeader (actionHdr);

  m_dca->Queue (packet, hdr);
}

void
RegularWifiMac::NotifyBandChanged (enum WifiPhyStandard, Mac48Address address, bool isInitiator)
{
  NS_LOG_FUNCTION (this << address << isInitiator);
  /* If we are the initiator, we send FST Ack Request in the new band */
  if (isInitiator)
    {
      /* We transfer FST ACK Request in the new frequency band */
      FstSession *fstSession = &m_fstSessionMap[address];
      SendFstAckRequest (address, 0, fstSession->ID);
    }
}

void
RegularWifiMac::TxOk (Ptr<const Packet> currentPacket, const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << hdr);
  if (hdr.IsAction ())
    {
      WifiActionHeader actionHdr;
      Ptr<Packet> packet = currentPacket->Copy ();
      packet->RemoveHeader (actionHdr);

      if (actionHdr.GetCategory () == WifiActionHeader::FST)
        {
          switch (actionHdr.GetAction ().fstAction)
            {
            case WifiActionHeader::FST_SETUP_RESPONSE:
              {
                NS_LOG_LOGIC ("FST Responder: Received ACK for FST Response, so transit to FST_SETUP_COMPLETION_STATE");
                FstSession *fstSession = &m_fstSessionMap[hdr.GetAddr1 ()];
                fstSession->CurrentState = FST_SETUP_COMPLETION_STATE;
                /* We are the responder of the FST session and we got ACK for FST Setup Response */          
                if (fstSession->LLT == 0)
                  {
                    NS_LOG_LOGIC ("FST Responder: LLT=0, so transit to FST_TRANSITION_DONE_STATE");
                    fstSession->CurrentState = FST_TRANSITION_DONE_STATE;
                    ChangeBand (hdr.GetAddr1 (), fstSession->NewBandId, false);
                  }
                else
                  {
                    NS_LOG_LOGIC ("FST Responder: LLT>0, so start Link Loss Countdown");
                    /* Initiate a Link Loss Timer */
                    fstSession->LinkLossCountDownEvent = Simulator::Schedule (MicroSeconds (fstSession->LLT * 32),
                                                                              &RegularWifiMac::ChangeBand, this,
                                                                              hdr.GetAddr1 (), fstSession->NewBandId, false);
                  }
                return;
              }
            case WifiActionHeader::FST_ACK_RESPONSE:
              {
                /* We are the Responder of the FST session and we got ACK for FST ACK Response */
                NS_LOG_LOGIC ("FST Responder: Transmitted FST ACK Response successfully, so transit to FST_TRANSITION_CONFIRMED_STATE");
                FstSession *fstSession = &m_fstSessionMap[hdr.GetAddr1 ()];
                fstSession->CurrentState = FST_TRANSITION_CONFIRMED_STATE;
                return;
              }
            default:
              break;
            }
        }
    }
  m_txOkCallback (hdr);
}

void
RegularWifiMac::ChangeBand (Mac48Address peerStation, BandID bandId, bool isInitiator)
{
  NS_LOG_FUNCTION (this << peerStation << bandId << isInitiator);
  if (bandId == Band_60GHz)
    {
      m_bandChangedCallback (WIFI_PHY_STANDARD_80211ad, peerStation, isInitiator);
    }
  else if (bandId == Band_4_9GHz)
    {
      m_bandChangedCallback (WIFI_PHY_STANDARD_80211n_5GHZ, peerStation, isInitiator);
    }
  else if (bandId == Band_2_4GHz)
    {
      m_bandChangedCallback (WIFI_PHY_STANDARD_80211n_2_4GHZ, peerStation, isInitiator);
    }
}

void
RegularWifiMac::Receive (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << packet << hdr);

  Mac48Address to = hdr->GetAddr1 ();
  Mac48Address from = hdr->GetAddr2 ();

  //We don't know how to deal with any frame that is not addressed to
  //us (and odds are there is nothing sensible we could do anyway),
  //so we ignore such frames.
  //
  //The derived class may also do some such filtering, but it doesn't
  //hurt to have it here too as a backstop.
  if (to != GetAddress ())
    {
      return;
    }

  if (hdr->IsMgt () && hdr->IsAction ())
    {
      //There is currently only any reason for Management Action
      //frames to be flying about if we are a QoS STA.
      NS_ASSERT (m_qosSupported);

      WifiActionHeader actionHdr;
      packet->RemoveHeader (actionHdr);

      switch (actionHdr.GetCategory ())
        {
        case WifiActionHeader::BLOCK_ACK:

          switch (actionHdr.GetAction ().blockAck)
            {
            case WifiActionHeader::BLOCK_ACK_ADDBA_REQUEST:
              {
                MgtAddBaRequestHeader reqHdr;
                packet->RemoveHeader (reqHdr);

                //We've received an ADDBA Request. Our policy here is
                //to automatically accept it, so we get the ADDBA
                //Response on it's way immediately.
                SendAddBaResponse (&reqHdr, from);
                //This frame is now completely dealt with, so we're done.
                return;
              }
            case WifiActionHeader::BLOCK_ACK_ADDBA_RESPONSE:
              {
                MgtAddBaResponseHeader respHdr;
                packet->RemoveHeader (respHdr);

                //We've received an ADDBA Response. We assume that it
                //indicates success after an ADDBA Request we have
                //sent (we could, in principle, check this, but it
                //seems a waste given the level of the current model)
                //and act by locally establishing the agreement on
                //the appropriate queue.
                AcIndex ac = QosUtilsMapTidToAc (respHdr.GetTid ());
                m_edca[ac]->GotAddBaResponse (&respHdr, from);
                //This frame is now completely dealt with, so we're done.
                return;
              }
            case WifiActionHeader::BLOCK_ACK_DELBA:
              {
                MgtDelBaHeader delBaHdr;
                packet->RemoveHeader (delBaHdr);

                if (delBaHdr.IsByOriginator ())
                  {
                    //This DELBA frame was sent by the originator, so
                    //this means that an ingoing established
                    //agreement exists in MacLow and we need to
                    //destroy it.
                    m_low->DestroyBlockAckAgreement (from, delBaHdr.GetTid ());
                  }
                else
                  {
                    //We must have been the originator. We need to
                    //tell the correct queue that the agreement has
                    //been torn down
                    AcIndex ac = QosUtilsMapTidToAc (delBaHdr.GetTid ());
                    m_edca[ac]->GotDelBaFrame (&delBaHdr, from);
                  }
                //This frame is now completely dealt with, so we're done.
                return;
              }
            default:
              NS_FATAL_ERROR ("Unsupported Action field in Block Ack Action frame");
              return;
            }
          /* Fast Session Transfer */
          case WifiActionHeader::FST:
            switch (actionHdr.GetAction ().fstAction)
              {
              case WifiActionHeader::FST_SETUP_REQUEST:
                {
                  ExtFstSetupRequest requestHdr;
                  packet->RemoveHeader (requestHdr);
                  /* We are the responder of the FST, create new entry for FST Session */
                  FstSession fstSession;
                  fstSession.ID = requestHdr.GetSessionTransition ().GetFstsID ();
                  fstSession.CurrentState = FST_INITIAL_STATE;
                  fstSession.IsInitiator = false;
                  fstSession.NewBandId = static_cast<BandID> (requestHdr.GetSessionTransition ().GetNewBand ().Band_ID);
                  fstSession.LLT = requestHdr.GetLlt ();
                  m_fstSessionMap[hdr->GetAddr2 ()] = fstSession;
                  NS_LOG_LOGIC ("FST Responder: Received FST Setup Request with LLT=" << m_llt);
                  /* Send FST Response to the Initiator */
                  SendFstSetupResponse (hdr->GetAddr2 (), requestHdr.GetDialogToken (), 0, requestHdr.GetSessionTransition ());
                  return;
                }
              case WifiActionHeader::FST_SETUP_RESPONSE:
                {
                  ExtFstSetupResponse responseHdr;
                  packet->RemoveHeader (responseHdr);
                  /* We are the initiator of the FST */
                  if (responseHdr.GetStatusCode () == 0)
                    {
                      /* Move the Initiator to the Setup Completion State */
                      NS_LOG_LOGIC ("FST Initiator: Received FST Setup Response with Status=0, so transit to FST_SETUP_COMPLETION_STATE");
                      FstSession *fstSession = &m_fstSessionMap[hdr->GetAddr2 ()];
                      fstSession->CurrentState = FST_SETUP_COMPLETION_STATE;
                      if (fstSession->LLT == 0)
                        {
                          NS_LOG_LOGIC ("FST Initiator: Received FST Setup Response with LLT=0, so transit to FST_TRANSITION_DONE_STATE");
                          fstSession->CurrentState = FST_TRANSITION_DONE_STATE;
                          /* Optionally transmit Broadcast FST Setup Response */

                          /* Check the new band */
                          ChangeBand (hdr->GetAddr2 (), fstSession->NewBandId, true);
                        }
                      else
                        {
                          NS_LOG_LOGIC ("FST Initiator: Received FST Setup Response with LLT>0, so Initiate Link Loss Countdown");
                          /* Initiate a Link Loss Timer */
                          fstSession->LinkLossCountDownEvent = Simulator::Schedule (MicroSeconds (fstSession->LLT * 32),
                                                                                    &RegularWifiMac::ChangeBand, this,
                                                                                    hdr->GetAddr2 (), fstSession->NewBandId, true);
                        }
                    }
                  else
                    {
                      NS_LOG_DEBUG ("FST Failed with " << hdr->GetAddr2 ());
                    }
                  return;
                }
              case WifiActionHeader::FST_TEAR_DOWN:
                {
                  ExtFstTearDown teardownHdr;
                  packet->RemoveHeader (teardownHdr);
                  FstSession *fstSession = &m_fstSessionMap[hdr->GetAddr2 ()];
                  fstSession->CurrentState = FST_INITIAL_STATE;
                  NS_LOG_DEBUG ("FST session with ID= " << teardownHdr.GetFstsID ()<< "is terminated");
                  return;
                }
              case WifiActionHeader::FST_ACK_REQUEST:
                {
                  ExtFstAckRequest requestHdr;
                  packet->RemoveHeader (requestHdr);
                  SendFstAckResponse (hdr->GetAddr2 (), requestHdr.GetDialogToken (), requestHdr.GetFstsID ());
                  NS_LOG_LOGIC ("FST Responder: Received FST ACK Request for FSTS ID=" << requestHdr.GetFstsID ()
                                << " so transmit FST ACK Response");
                  return;
                }
              case WifiActionHeader::FST_ACK_RESPONSE:
                {
                  ExtFstAckResponse responseHdr;
                  packet->RemoveHeader (responseHdr);
                  /* We are the initiator, Get FST Session */
                  FstSession *fstSession = &m_fstSessionMap[hdr->GetAddr2 ()];
                  fstSession->CurrentState = FST_TRANSITION_CONFIRMED_STATE;
                  NS_LOG_LOGIC ("FST Initiator: Received FST ACK Response for FSTS ID=" << responseHdr.GetFstsID ()
                                << " so transit to FST_TRANSITION_CONFIRMED_STATE");
                  return;
                }
              default:
                NS_FATAL_ERROR ("Unsupported Action field in Block Ack Action frame");
                return;
              }
        default:
          NS_FATAL_ERROR ("Unsupported Action frame received");
          return;
        }
    }
  NS_FATAL_ERROR ("Don't know how to handle frame (type=" << hdr->GetType ());
}

void
RegularWifiMac::DeaggregateAmsduAndForward (Ptr<Packet> aggregatedPacket, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << aggregatedPacket << hdr);
  MsduAggregator::DeaggregatedMsdus packets = MsduAggregator::Deaggregate (aggregatedPacket);
  for (MsduAggregator::DeaggregatedMsdusCI i = packets.begin ();
       i != packets.end (); ++i)
    {
      ForwardUp ((*i).first, (*i).second.GetSourceAddr (),
                 (*i).second.GetDestinationAddr ());
    }
}

void
RegularWifiMac::SendAddBaResponse (const MgtAddBaRequestHeader *reqHdr,
                                   Mac48Address originator)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (originator);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  hdr.SetNoOrder ();

  MgtAddBaResponseHeader respHdr;
  StatusCode code;
  code.SetSuccess ();
  respHdr.SetStatusCode (code);
  //Here a control about queues type?
  respHdr.SetAmsduSupport (reqHdr->IsAmsduSupported ());

  if (reqHdr->IsImmediateBlockAck ())
    {
      respHdr.SetImmediateBlockAck ();
    }
  else
    {
      respHdr.SetDelayedBlockAck ();
    }
  respHdr.SetTid (reqHdr->GetTid ());
  //For now there's not no control about limit of reception. We
  //assume that receiver has no limit on reception. However we assume
  //that a receiver sets a bufferSize in order to satisfy next
  //equation: (bufferSize + 1) % 16 = 0 So if a recipient is able to
  //buffer a packet, it should be also able to buffer all possible
  //packet's fragments. See section 7.3.1.14 in IEEE802.11e for more details.
  respHdr.SetBufferSize (1023);
  respHdr.SetTimeout (reqHdr->GetTimeout ());

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.blockAck = WifiActionHeader::BLOCK_ACK_ADDBA_RESPONSE;
  actionHdr.SetAction (WifiActionHeader::BLOCK_ACK, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (respHdr);
  packet->AddHeader (actionHdr);

  //We need to notify our MacLow object as it will have to buffer all
  //correctly received packets for this Block Ack session
  m_low->CreateBlockAckAgreement (&respHdr, originator,
                                  reqHdr->GetStartingSequence ());

  //It is unclear which queue this frame should go into. For now we
  //bung it into the queue corresponding to the TID for which we are
  //establishing an agreement, and push it to the head.
  m_edca[QosUtilsMapTidToAc (reqHdr->GetTid ())]->PushFront (packet, hdr);
}

TypeId
RegularWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RegularWifiMac")
    .SetParent<WifiMac> ()
    .SetGroupName ("Wifi")
    .AddAttribute ("QosSupported",
                   "This Boolean attribute is set to enable 802.11e/WMM-style QoS support at this STA.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RegularWifiMac::SetQosSupported,
                                        &RegularWifiMac::GetQosSupported),
                   MakeBooleanChecker ())
    .AddAttribute ("HtSupported",
                   "This Boolean attribute is set to enable 802.11n support at this STA.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RegularWifiMac::SetHtSupported,
                                        &RegularWifiMac::GetHtSupported),
                   MakeBooleanChecker ())
    .AddAttribute ("VhtSupported",
                   "This Boolean attribute is set to enable 802.11ac support at this STA.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RegularWifiMac::SetVhtSupported,
                                        &RegularWifiMac::GetVhtSupported),
                   MakeBooleanChecker ())
    .AddAttribute ("HeSupported",
                   "This Boolean attribute is set to enable 802.11ax support at this STA.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RegularWifiMac::SetHeSupported,
                                        &RegularWifiMac::GetHeSupported),
                   MakeBooleanChecker ())
    .AddAttribute ("DmgSupported",
                   "This Boolean attribute is set to enable 802.11ad support at this STA",
                    BooleanValue (false),
                    MakeBooleanAccessor (&RegularWifiMac::SetDmgSupported,
                                         &RegularWifiMac::GetDmgSupported),
                    MakeBooleanChecker ())

    /* Fast Session Transfer Support */
    .AddAttribute ("LLT", "The value of link loss timeout in microseconds",
                    UintegerValue (0),
                    MakeUintegerAccessor (&RegularWifiMac::m_llt),
                    MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("FstTimeout", "The timeout value of FST session in TUs.",
                    UintegerValue (10),
                    MakeUintegerAccessor (&RegularWifiMac::m_fstTimeout),
                    MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("SupportMultiBand",
                   "Support multi-band operation for fast session transfer.",
                    BooleanValue (false),
                    MakeBooleanAccessor (&RegularWifiMac::m_supportMultiBand),
                    MakeBooleanChecker ())

    .AddAttribute ("CtsToSelfSupported",
                   "Use CTS to Self when using a rate that is not in the basic rate set.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RegularWifiMac::SetCtsToSelfSupported,
                                        &RegularWifiMac::GetCtsToSelfSupported),
                   MakeBooleanChecker ())
    .AddAttribute ("VO_MaxAmsduSize",
                   "Maximum length in bytes of an A-MSDU for AC_VO access class. "
                   "Value 0 means A-MSDU is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetVoMaxAmsduSize),
                   MakeUintegerChecker<uint32_t> (0, 11426))
    .AddAttribute ("VI_MaxAmsduSize",
                   "Maximum length in bytes of an A-MSDU for AC_VI access class."
                   "Value 0 means A-MSDU is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetViMaxAmsduSize),
                   MakeUintegerChecker<uint32_t> (0, 11426))
    .AddAttribute ("BE_MaxAmsduSize",
                   "Maximum length in bytes of an A-MSDU for AC_BE access class."
                   "Value 0 means A-MSDU is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetBeMaxAmsduSize),
                   MakeUintegerChecker<uint32_t> (0, 11426))
    .AddAttribute ("BK_MaxAmsduSize",
                   "Maximum length in bytes of an A-MSDU for AC_BK access class."
                   "Value 0 means A-MSDU is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetBkMaxAmsduSize),
                   MakeUintegerChecker<uint32_t> (0, 11426))
    .AddAttribute ("VO_MaxAmpduSize",
                   "Maximum length in bytes of an A-MPDU for AC_VO access class."
                   "Value 0 means A-MPDU is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetVoMaxAmpduSize),
                   MakeUintegerChecker<uint32_t> (0, 262143))
    .AddAttribute ("VI_MaxAmpduSize",
                   "Maximum length in bytes of an A-MPDU for AC_VI access class."
                   "Value 0 means A-MPDU is disabled for that AC.",
                   UintegerValue (65535),
                   MakeUintegerAccessor (&RegularWifiMac::SetViMaxAmpduSize),
                   MakeUintegerChecker<uint32_t> (0, 262143))
    .AddAttribute ("BE_MaxAmpduSize",
                   "Maximum length in bytes of an A-MPDU for AC_BE access class."
                   "Value 0 means A-MPDU is disabled for that AC.",
                   UintegerValue (65535),
                   MakeUintegerAccessor (&RegularWifiMac::SetBeMaxAmpduSize),
                   MakeUintegerChecker<uint32_t> (0, 262143))
    .AddAttribute ("BK_MaxAmpduSize",
                   "Maximum length in bytes of an A-MPDU for AC_BK access class."
                   "Value 0 means A-MPDU is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetBkMaxAmpduSize),
                   MakeUintegerChecker<uint32_t> (0, 262143))
    .AddAttribute ("VO_BlockAckThreshold",
                   "If number of packets in VO queue reaches this value, "
                   "block ack mechanism is used. If this value is 0, block ack is never used."
                   "When A-MPDU is enabled, block ack mechanism is used regardless of this value.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetVoBlockAckThreshold),
                   MakeUintegerChecker<uint8_t> (0, 64))
    .AddAttribute ("VI_BlockAckThreshold",
                   "If number of packets in VI queue reaches this value, "
                   "block ack mechanism is used. If this value is 0, block ack is never used."
                   "When A-MPDU is enabled, block ack mechanism is used regardless of this value.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetViBlockAckThreshold),
                   MakeUintegerChecker<uint8_t> (0, 64))
    .AddAttribute ("BE_BlockAckThreshold",
                   "If number of packets in BE queue reaches this value, "
                   "block ack mechanism is used. If this value is 0, block ack is never used."
                   "When A-MPDU is enabled, block ack mechanism is used regardless of this value.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetBeBlockAckThreshold),
                   MakeUintegerChecker<uint8_t> (0, 64))
    .AddAttribute ("BK_BlockAckThreshold",
                   "If number of packets in BK queue reaches this value, "
                   "block ack mechanism is used. If this value is 0, block ack is never used."
                   "When A-MPDU is enabled, block ack mechanism is used regardless of this value.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetBkBlockAckThreshold),
                   MakeUintegerChecker<uint8_t> (0, 64))
    .AddAttribute ("VO_BlockAckInactivityTimeout",
                   "Represents max time (blocks of 1024 micro seconds) allowed for block ack"
                   "inactivity for AC_VO. If this value isn't equal to 0 a timer start after that a"
                   "block ack setup is completed and will be reset every time that a block ack"
                   "frame is received. If this value is 0, block ack inactivity timeout won't be used.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetVoBlockAckInactivityTimeout),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("VI_BlockAckInactivityTimeout",
                   "Represents max time (blocks of 1024 micro seconds) allowed for block ack"
                   "inactivity for AC_VI. If this value isn't equal to 0 a timer start after that a"
                   "block ack setup is completed and will be reset every time that a block ack"
                   "frame is received. If this value is 0, block ack inactivity timeout won't be used.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetViBlockAckInactivityTimeout),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("BE_BlockAckInactivityTimeout",
                   "Represents max time (blocks of 1024 micro seconds) allowed for block ack"
                   "inactivity for AC_BE. If this value isn't equal to 0 a timer start after that a"
                   "block ack setup is completed and will be reset every time that a block ack"
                   "frame is received. If this value is 0, block ack inactivity timeout won't be used.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetBeBlockAckInactivityTimeout),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("BK_BlockAckInactivityTimeout",
                   "Represents max time (blocks of 1024 micro seconds) allowed for block ack"
                   "inactivity for AC_BK. If this value isn't equal to 0 a timer start after that a"
                   "block ack setup is completed and will be reset every time that a block ack"
                   "frame is received. If this value is 0, block ack inactivity timeout won't be used.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetBkBlockAckInactivityTimeout),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("ShortSlotTimeSupported",
                   "Whether or not short slot time is supported (only used by ERP APs or STAs).",
                   BooleanValue (true),
                   MakeBooleanAccessor (&RegularWifiMac::GetShortSlotTimeSupported,
                                        &RegularWifiMac::SetShortSlotTimeSupported),
                   MakeBooleanChecker ())
    .AddAttribute ("RifsSupported",
                   "Whether or not RIFS is supported (only used by HT APs or STAs).",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RegularWifiMac::GetRifsSupported,
                                        &RegularWifiMac::SetRifsSupported),
                   MakeBooleanChecker ())
    .AddAttribute ("DcaTxop",
                   "The DcaTxop object.",
                   PointerValue (),
                   MakePointerAccessor (&RegularWifiMac::GetDcaTxop),
                   MakePointerChecker<DcaTxop> ())
    .AddAttribute ("VO_EdcaTxopN",
                   "Queue that manages packets belonging to AC_VO access class.",
                   PointerValue (),
                   MakePointerAccessor (&RegularWifiMac::GetVOQueue),
                   MakePointerChecker<EdcaTxopN> ())
    .AddAttribute ("VI_EdcaTxopN",
                   "Queue that manages packets belonging to AC_VI access class.",
                   PointerValue (),
                   MakePointerAccessor (&RegularWifiMac::GetVIQueue),
                   MakePointerChecker<EdcaTxopN> ())
    .AddAttribute ("BE_EdcaTxopN",
                   "Queue that manages packets belonging to AC_BE access class.",
                   PointerValue (),
                   MakePointerAccessor (&RegularWifiMac::GetBEQueue),
                   MakePointerChecker<EdcaTxopN> ())
    .AddAttribute ("BK_EdcaTxopN",
                   "Queue that manages packets belonging to AC_BK access class.",
                   PointerValue (),
                   MakePointerAccessor (&RegularWifiMac::GetBKQueue),
                   MakePointerChecker<EdcaTxopN> ())
    .AddAttribute ("MacLow",
                   "Access the mac low layer responsible for packet transmition.",
                   PointerValue (),
                   MakePointerAccessor (&RegularWifiMac::m_low),
                   MakePointerChecker<MacLow> ())
    .AddTraceSource ("TxOkHeader",
                     "The header of successfully transmitted packet.",
                     MakeTraceSourceAccessor (&RegularWifiMac::m_txOkCallback),
                     "ns3::WifiMacHeader::TracedCallback")
    .AddTraceSource ("TxErrHeader",
                     "The header of unsuccessfully transmitted packet.",
                     MakeTraceSourceAccessor (&RegularWifiMac::m_txErrCallback),
                     "ns3::WifiMacHeader::TracedCallback")
  ;
  return tid;
}

void
RegularWifiMac::FinishConfigureStandard (WifiPhyStandard standard)
{
  NS_LOG_FUNCTION (this << standard);
  uint32_t cwmin = 0;
  uint32_t cwmax = 0;
  switch (standard)
    {
    case WIFI_PHY_STANDARD_80211ad:
      SetDmgSupported (true);
      cwmin = 15;
      cwmax = 1023;
      break;
    case WIFI_PHY_STANDARD_80211ax_5GHZ:
      SetHeSupported (true);
    case WIFI_PHY_STANDARD_80211ac:
      SetVhtSupported (true);
    case WIFI_PHY_STANDARD_80211n_5GHZ:
      SetHtSupported (true);
      cwmin = 15;
      cwmax = 1023;
      break;
    case WIFI_PHY_STANDARD_80211ax_2_4GHZ:
      SetHeSupported (true);
    case WIFI_PHY_STANDARD_80211n_2_4GHZ:
      SetHtSupported (true);
    case WIFI_PHY_STANDARD_80211g:
      SetErpSupported (true);
    case WIFI_PHY_STANDARD_holland:
    case WIFI_PHY_STANDARD_80211a:
    case WIFI_PHY_STANDARD_80211_10MHZ:
    case WIFI_PHY_STANDARD_80211_5MHZ:
      cwmin = 15;
      cwmax = 1023;
      break;
    case WIFI_PHY_STANDARD_80211b:
      SetDsssSupported (true);
      cwmin = 31;
      cwmax = 1023;
      break;
    default:
      NS_FATAL_ERROR ("Unsupported WifiPhyStandard in RegularWifiMac::FinishConfigureStandard ()");
    }

  ConfigureContentionWindow (cwmin, cwmax);
}

void
RegularWifiMac::ConfigureContentionWindow (uint32_t cwMin, uint32_t cwMax)
{
  bool isDsssOnly = m_dsssSupported && !m_erpSupported;
  //The special value of AC_BE_NQOS which exists in the Access
  //Category enumeration allows us to configure plain old DCF.
  ConfigureDcf (m_dca, cwMin, cwMax, isDsssOnly, AC_BE_NQOS);

  //Now we configure the EDCA functions
  for (EdcaQueues::const_iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      ConfigureDcf (i->second, cwMin, cwMax, isDsssOnly, i->first);
    }
}

void
RegularWifiMac::TxFailed (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << hdr);
  m_txErrCallback (hdr);
}

void
RegularWifiMac::ConfigureAggregation (void)
{
  NS_LOG_FUNCTION (this);
  if (GetVOQueue ()->GetMsduAggregator () != 0)
    {
      GetVOQueue ()->GetMsduAggregator ()->SetMaxAmsduSize (m_voMaxAmsduSize);
    }
  if (GetVIQueue ()->GetMsduAggregator () != 0)
    {
      GetVIQueue ()->GetMsduAggregator ()->SetMaxAmsduSize (m_viMaxAmsduSize);
    }
  if (GetBEQueue ()->GetMsduAggregator () != 0)
    {
      GetBEQueue ()->GetMsduAggregator ()->SetMaxAmsduSize (m_beMaxAmsduSize);
    }
  if (GetBKQueue ()->GetMsduAggregator () != 0)
    {
      GetBKQueue ()->GetMsduAggregator ()->SetMaxAmsduSize (m_bkMaxAmsduSize);
    }
  if (GetVOQueue ()->GetMpduAggregator () != 0)
    {
      GetVOQueue ()->GetMpduAggregator ()->SetMaxAmpduSize (m_voMaxAmpduSize);
    }
  if (GetVIQueue ()->GetMpduAggregator () != 0)
    {
      GetVIQueue ()->GetMpduAggregator ()->SetMaxAmpduSize (m_viMaxAmpduSize);
    }
  if (GetBEQueue ()->GetMpduAggregator () != 0)
    {
      GetBEQueue ()->GetMpduAggregator ()->SetMaxAmpduSize (m_beMaxAmpduSize);
    }
  if (GetBKQueue ()->GetMpduAggregator () != 0)
    {
      GetBKQueue ()->GetMpduAggregator ()->SetMaxAmpduSize (m_bkMaxAmpduSize);
    }
}

void
RegularWifiMac::EnableAggregation (void)
{
  NS_LOG_FUNCTION (this);
  for (EdcaQueues::const_iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      if (i->second->GetMsduAggregator () == 0)
        {
          Ptr<MsduAggregator> msduAggregator = CreateObject<MsduAggregator> ();
          i->second->SetMsduAggregator (msduAggregator);
        }
      if (i->second->GetMpduAggregator () == 0)
        {
          Ptr<MpduAggregator> mpduAggregator = CreateObject<MpduAggregator> ();
          i->second->SetMpduAggregator (mpduAggregator);
        }
    }
  ConfigureAggregation ();
}

void
RegularWifiMac::DisableAggregation (void)
{
  NS_LOG_FUNCTION (this);
  for (EdcaQueues::const_iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->SetMsduAggregator (0);
      i->second->SetMpduAggregator (0);
    }
}

} //namespace ns3
