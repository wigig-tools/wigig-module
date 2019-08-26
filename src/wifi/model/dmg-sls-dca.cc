/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/assert.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/pointer.h"

#include "dcf-state.h"
#include "dmg-sls-dca.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT if (m_low != 0) { std::clog << "[mac=" << m_low->GetAddress () << "] "; }

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DmgSlsDca");

NS_OBJECT_ENSURE_REGISTERED (DmgSlsDca);

TypeId
DmgSlsDca::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgSlsDca")
    .SetParent<DcaTxop> ()
    .SetGroupName ("Wifi")
    .AddConstructor<DmgSlsDca> ()
  ;
  return tid;
}

DmgSlsDca::DmgSlsDca ()
{
  NS_LOG_FUNCTION (this);
  m_transmitFeedback = false;
}

DmgSlsDca::~DmgSlsDca ()
{
  NS_LOG_FUNCTION (this);
}

void
DmgSlsDca::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  DcaTxop::DoDispose ();
}

void
DmgSlsDca::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  DcaTxop::DoInitialize ();
}

void
DmgSlsDca::SetAccessGrantedCallback (AccessGranted callback)
{
  m_accessGrantedCallback = callback;
}

void
DmgSlsDca::ObtainTxOP (Mac48Address peerAddress, bool feedback)
{
  NS_LOG_FUNCTION (this << peerAddress << feedback << m_dcf->IsAccessRequested () << m_manager->IsAccessAllowed ());
  m_transmitFeedback = feedback;
  if (!feedback)
    {
      m_slsRequetsQueue.push (peerAddress);
    }
  if (!m_dcf->IsAccessRequested () && m_manager->IsAccessAllowed ())
    {
      m_dcf->ResetCw ();
      m_dcf->StartBackoffNow (m_rng->GetInteger (0, m_dcf->GetCw ()));
      m_manager->RequestAccess (m_dcf);
    }
}

void
DmgSlsDca::ResumeTxSS (void)
{
  NS_LOG_FUNCTION (this << m_slsRequetsQueue.empty () << m_transmitFeedback);
  if (!m_dcf->IsAccessRequested () &&
      m_manager->IsAccessAllowed () &&
      (!m_slsRequetsQueue.empty () || m_transmitFeedback))
    {
      m_dcf->ResetCw ();
      m_dcf->StartBackoffNow (m_rng->GetInteger (0, m_dcf->GetCw ()));
      m_manager->RequestAccess (m_dcf);
    }
}

void
DmgSlsDca::NotifyAccessGranted (void)
{
  NS_LOG_FUNCTION (this);
  if (!m_transmitFeedback)
    {
      m_feedBackAddress = m_slsRequetsQueue.front ();
      m_slsRequetsQueue.pop ();
      m_accessGrantedCallback (m_feedBackAddress, false);
    }
  else
    {
      m_accessGrantedCallback (m_feedBackAddress, true);
      m_transmitFeedback = false;
    }
}

void
DmgSlsDca::TransmitSswFrame (Ptr<const Packet> packet, const WifiMacHeader &hdr, Time duration)
{
  NS_LOG_FUNCTION (this << packet << &hdr << duration);
  m_currentHdr = hdr;
  m_currentParams.EnableOverrideDurationId (duration);
  m_currentParams.DisableRts ();
  m_currentParams.DisableAck ();
  m_currentParams.DisableNextData ();
  GetLow ()->TransmitSingleFrame (packet, &hdr, m_currentParams, this);
}

void
DmgSlsDca::RestartAccessIfNeeded (void)
{
  NS_LOG_FUNCTION (this);
  if (!m_dcf->IsAccessRequested () && m_manager->IsAccessAllowed ())
    {
      m_manager->RequestAccess (m_dcf);
    }
}

void
DmgSlsDca::NotifyInternalCollision (void)
{
  NS_LOG_FUNCTION (this);
  NotifyCollision ();
}

void
DmgSlsDca::NotifyCollision (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("Medium is busy, collision");
  m_dcf->StartBackoffNow (m_rng->GetInteger (0, m_dcf->GetCw ()));
  RestartAccessIfNeeded ();
}

void
DmgSlsDca::Cancel (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("Transmission cancelled");
}

void
DmgSlsDca::EndTxNoAck (void)
{
  NS_LOG_FUNCTION (this);
  if (!m_txOkNoAckCallback.IsNull ())
    {
      m_txOkNoAckCallback (m_currentHdr);
    }
}

} //namespace ns3
