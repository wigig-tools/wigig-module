/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#include "ns3/assert.h"
#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "dmg-sls-txop.h"
#include "mac-low.h"
#include "wifi-mac-queue.h"
#include <algorithm>

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT if (m_low != 0) { std::clog << "[mac=" << m_low->GetAddress () << "] "; }

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DmgSlsTxop");

NS_OBJECT_ENSURE_REGISTERED (DmgSlsTxop);

TypeId
DmgSlsTxop::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgSlsTxop")
    .SetParent<Txop> ()
    .SetGroupName ("Wifi")
    .AddConstructor<DmgSlsTxop> ()
//    .AddAttribute ("ResumeCbapBeamforming",
//                   "Whether we should resume an interrupted beamforming training (BFT) due to CBAP ending or"
//                   "just start a new beamforming training at the beginning of the following beacon interval."
//                   "If set to True, we resume the beamforming training from it stopped. Otherwise, we "
//                   "restart BFT at the beginning of the next BI.",
//                    BooleanValue (false),
//                    MakeBooleanAccessor (&DmgSlsTxop::m_resumeCbapBeamforming),
//                    MakeBooleanChecker ())
  ;
  return tid;
}

DmgSlsTxop::DmgSlsTxop ()
{
  NS_LOG_FUNCTION (this);
}

DmgSlsTxop::~DmgSlsTxop ()
{
  NS_LOG_FUNCTION (this);
}

void
DmgSlsTxop::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Txop::DoDispose ();
}

void
DmgSlsTxop::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  InitializeVariables ();
  Txop::DoInitialize ();
}

bool
DmgSlsTxop::ResumeCbapBeamforming (void) const
{
  return m_resumeCbapBeamforming;
}

void
DmgSlsTxop::SetAccessGrantedCallback (AccessGranted callback)
{
  m_accessGrantedCallback = callback;
}

void
DmgSlsTxop::ResumeTXSS (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("Is SLS Requests Queue Empty = " << m_slsRequetsDeque.empty ());
  ResetCw ();
  GenerateBackoff ();
  if (!m_servingSLS || (m_servingSLS && m_slsRole == SLS_RESPONDER))
    {
      InitializeVariables ();
      if (!m_slsRequetsDeque.empty () &&
          !IsAccessRequested () &&
          m_channelAccessManager->IsAccessAllowed ())
        {
          m_slsRole = SLS_INITIATOR;
          m_channelAccessManager->RequestAccess (this);
        }
    }
  else if (m_servingSLS && m_slsRole == SLS_INITIATOR)
    {
      /* We are already performing SLS as initiator, but it failed in the previous BI and there was no enough
       * time in the DTI to continue so we need to continue trying in this new BI. */
      RestartAccessIfNeeded ();
    }
}

void
DmgSlsTxop::Append_SLS_Reqest (Mac48Address peerAddress)
{
  NS_LOG_FUNCTION (this << peerAddress);

  /* Check if we are serving the same peer address (We've received SLS Request due to timeout
     but we are going to do feedback, so we avoid doing new SLS request. */
  if (m_servingSLS &&
      m_peerStation == peerAddress &&
      m_slsRole == SLS_INITIATOR)
    {
      NS_LOG_DEBUG ("We are performing SLS with " << peerAddress << ", so avoid adding new SLS Request");
      return;
    }

  // Check if the Deque has already a previous Beamforming Request to avoid too many beamforming training accesses.
  bool found = false;
  SLS_REQUESTS_DEQUE_I it;
  /* Check if we already have a request for the same peer Address */
  for (it = m_slsRequetsDeque.begin (); it != m_slsRequetsDeque.end (); it++)
    {
      if ((*it) == peerAddress)
        {
          found = true;
          NS_LOG_DEBUG ("Another SLS Request exists for " << peerAddress);
          break;
        }
    }

  if (!found)
    {
      m_slsRequetsDeque.push_front (peerAddress);
    }
}

void
DmgSlsTxop::Initiate_TXOP_Sector_Sweep (Mac48Address peerAddress)
{
  NS_LOG_FUNCTION (this << peerAddress);

  /* Check if we are serving the same peer address (We've received SLS Request due to timeout
     but we are going to do feedback, so we avoid doing new SLS request. */
  if (m_servingSLS && (m_peerStation == peerAddress) && (m_slsRole == SLS_INITIATOR))
    {
      NS_LOG_DEBUG ("We are performing SLS with " << peerAddress << ", so avoid adding new SLS Request");
      return;
    }

  // Check if the Deque has already a previous Beamforming Request to avoid too many beamforming training accesses.
  bool found = false;
  SLS_REQUESTS_DEQUE_I it;
  /* Check if we already have a request for the same peer Address */
  for (it = m_slsRequetsDeque.begin (); it != m_slsRequetsDeque.end (); it++)
    {
      if ((*it) == peerAddress)
        {
          found = true;
          NS_LOG_DEBUG ("Another SLS Request exists for " << peerAddress);
          break;
        }
    }

  if (!found)
    {
      m_slsRequetsDeque.push_front (peerAddress);
      NS_LOG_DEBUG ("AccessRequeted=" << IsAccessRequested () <<
                    ", AccessAllowed=" << m_channelAccessManager->IsAccessAllowed () <<
                    ", ServingSLS=" << m_servingSLS);
      if (!IsAccessRequested ()
          && m_channelAccessManager->IsAccessAllowed ()
          && !m_servingSLS)
        {
          m_slsRole = SLS_INITIATOR;
          m_channelAccessManager->RequestAccess (this);
        }
    }
}

void
DmgSlsTxop::Start_Responder_Sector_Sweep (Mac48Address peerAddress)
{
  NS_LOG_FUNCTION (this << peerAddress);
  NS_ASSERT_MSG (!IsAccessRequested (), "We should not have requested Responder Sector Sweep before.");
  if (m_channelAccessManager->IsAccessAllowed ())
    {
      m_slsRole = SLS_RESPONDER;
      m_peerStation = peerAddress;
      m_channelAccessManager->RequestAccess (this, true);
    }
}

void
DmgSlsTxop::Start_Initiator_Feedback (Mac48Address peerAddress)
{
  NS_LOG_FUNCTION (this << peerAddress);
  /* Check if we are serving the same peer address (We've received SLS Request due to timeout
     but we are going to do feedback, so we avoid doing new SLS request. */
  NS_ASSERT_MSG (m_servingSLS && m_peerStation == peerAddress, "Feedback should be done with the current SLS request");
  NS_ASSERT_MSG (!IsAccessRequested (), "We should not have requested Initiator Sector Sweep before.");
  if (m_channelAccessManager->IsAccessAllowed ())
    {
      m_isFeedback = true;
      m_channelAccessManager->RequestAccess (this, true);
    }
}

void
DmgSlsTxop::NotifyAccessGranted (void)
{
  NS_LOG_FUNCTION (this << m_slsRole);
  NS_ASSERT (m_accessRequested);
  m_accessRequested = false;
  /* We are in different access period, we are not allowed to do BFT */
  if (!m_channelAccessManager->IsAccessAllowed ())
    {
      /* We were granted access to the channel during BHI, so abort access and leave it to the ResumeTXSS function to
         continue beamforming training */
      NS_LOG_DEBUG ("Granted access during BHI, so abort.");
      return;
    }
  if (!m_servingSLS)
    {
      /* Serving new SLS Request */
      if (m_slsRole == SLS_INITIATOR)
        {
          m_peerStation = m_slsRequetsDeque.front ();
        }
      m_servingSLS = true;
      GetLow ()->SLS_Phase_Started ();
      NS_LOG_DEBUG ("Access granted for a new SLS request with " << m_peerStation);
    }
  else
    {
      NS_LOG_DEBUG ("Access granted for an existing SLS request with " << m_peerStation);
    }
  m_accessGrantedCallback (m_peerStation, m_slsRole, m_isFeedback);
}

void
DmgSlsTxop::InitializeVariables (void)
{
  NS_LOG_DEBUG (this);
  m_servingSLS = false;
  m_isFeedback = false;
  m_slsRole = SLS_IDLE;
  m_currentPacket = 0;
}

void
DmgSlsTxop::Sector_Sweep_Phase_Failed (void)
{
  NS_LOG_FUNCTION (this);
  UpdateFailedCw ();
  GenerateBackoff ();
  RestartAccessIfNeeded ();
}

void
DmgSlsTxop::RxSSW_ACK_Failed (void)
{
  NS_LOG_FUNCTION (this);
  /* Initiator failed to receive SSW-ACK from the responder */
  RestartAccessIfNeeded ();
}

void
DmgSlsTxop::SLS_BFT_Completed (void)
{
  NS_LOG_FUNCTION (this);
  if (m_slsRole == SLS_INITIATOR)
    {
      /* Remove the request from the queue */
      m_slsRequetsDeque.pop_front ();
    }
  InitializeVariables ();
  /* Reset Txop state */
  ResetCw ();
  m_cwTrace = GetCw ();
  GenerateBackoff ();
  GetLow ()->SLS_Phase_Ended ();
  RestartAccessIfNeeded ();
}

void
DmgSlsTxop::SLS_BFT_Failed (bool retryAccess)
{
  NS_LOG_FUNCTION (this << retryAccess);
  InitializeVariables ();
  /* Remove the current request from the queue as we exceed the number of trials. */
  m_slsRequetsDeque.pop_front ();
  /* Reset SLS state at the MacLow */
  GetLow ()->SLS_Phase_Ended ();
  if (retryAccess)
    {
      /* Reset SLS Txop state */
      ResetCw ();
      m_cwTrace = GetCw ();
      GenerateBackoff ();
      RestartAccessIfNeeded ();
    }
}

bool
DmgSlsTxop::ServingSLS (void) const
{
  return m_servingSLS;
}

void
DmgSlsTxop::TransmitFrame (Ptr<const Packet> packet, const WifiMacHeader &hdr, Time duration)
{
  NS_LOG_FUNCTION (this << packet << &hdr << duration);
  m_currentHdr = hdr;
  m_currentPacket = packet;
  m_currentParams.EnableOverrideDurationId (duration);
  m_currentParams.DisableRts ();
  m_currentParams.DisableAck ();
  m_currentParams.DisableNextData ();
  GetLow ()->TransmitSingleFrame (Create<WifiMacQueueItem> (packet, m_currentHdr), m_currentParams, this);
}

void
DmgSlsTxop::RestartAccessIfNeeded (void)
{
  NS_LOG_FUNCTION (this);
  if (!IsAccessRequested ()
      && m_channelAccessManager->IsAccessAllowed ()
      && (m_servingSLS || !m_slsRequetsDeque.empty ()))
    {
      m_channelAccessManager->RequestAccess (this);
    }
}

void
DmgSlsTxop::NotifyInternalCollision (void)
{
  NS_LOG_FUNCTION (this);
  GenerateBackoff ();
  RestartAccessIfNeeded ();
}

void
DmgSlsTxop::Cancel (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("Transmission cancelled");
}

void
DmgSlsTxop::EndTxNoAck (void)
{
  NS_LOG_FUNCTION (this);
  if (!m_txOkNoAckCallback.IsNull ())
    {
      m_txOkNoAckCallback (m_currentHdr);
    }
}

} //namespace ns3
