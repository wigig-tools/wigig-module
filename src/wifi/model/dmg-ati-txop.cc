/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

#include "channel-access-manager.h"
#include "dmg-ati-txop.h"
#include "mac-low.h"
#include "mac-tx-middle.h"
#include "wifi-mac-queue.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT if (m_low != 0) { std::clog << "[mac=" << m_low->GetAddress () << "] "; }

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DmgAtiTxop");

NS_OBJECT_ENSURE_REGISTERED (DmgAtiTxop);

TypeId
DmgAtiTxop::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgAtiTxop")
    .SetParent<ns3::Txop> ()
    .SetGroupName ("Wifi")
    .AddConstructor<DmgAtiTxop> ()
  ;
  return tid;
}

DmgAtiTxop::DmgAtiTxop ()
{
  NS_LOG_FUNCTION (this);
  m_allowTransmission = false;
}

DmgAtiTxop::~DmgAtiTxop ()
{
  NS_LOG_FUNCTION (this);
}

void
DmgAtiTxop::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Txop::DoDispose ();
}

void
DmgAtiTxop::Queue (Ptr<const Packet> packet, const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << packet << &hdr);
  m_queue->Enqueue (Create<WifiMacQueueItem> (packet, hdr));
  StartAccessIfNeeded ();
}

void
DmgAtiTxop::RestartAccessIfNeeded (void)
{
  NS_LOG_FUNCTION (this);
  if ((m_currentPacket != 0
       || !m_queue->IsEmpty ())
      && !IsAccessRequested ()
      && m_allowTransmission)
    {
      m_channelAccessManager->RequestAccess (this);
    }
}

void
DmgAtiTxop::StartAccessIfNeeded (void)
{
  NS_LOG_FUNCTION (this);
  if (m_currentPacket == 0
      && !m_queue->IsEmpty ()
      && !IsAccessRequested ()
      && m_allowTransmission)
    {
      m_channelAccessManager->RequestAccess (this);
    }
}

void
DmgAtiTxop::InitiateAtiAccessPeriod (Time atiDuration)
{
  NS_LOG_FUNCTION (this << atiDuration);
  m_atiDuration = atiDuration;
  m_allowTransmission = true;
  m_transmissionStarted = Simulator::Now ();
  Simulator::Schedule (atiDuration, &DmgAtiTxop::DisableTransmission, this);
}

void
DmgAtiTxop::InitiateTransmission (Time atiDuration)
{
  NS_LOG_FUNCTION (this << atiDuration);
  InitiateAtiAccessPeriod (atiDuration);
  StartAccessIfNeeded ();
}

void
DmgAtiTxop::DisableTransmission (void)
{
  NS_LOG_FUNCTION (this);
  m_allowTransmission = false;
}

void
DmgAtiTxop::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  Txop::DoInitialize ();
}

bool
DmgAtiTxop::NeedDataRetransmission (void)
{
  NS_LOG_FUNCTION (this);
  return m_stationManager->NeedRetransmission (m_currentHdr.GetAddr1 (), &m_currentHdr, m_currentPacket);
}

void
DmgAtiTxop::NotifyAccessGranted (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_accessRequested);
  m_accessRequested = false;
  /* Update remaining ATI duration */
  m_remainingDuration = m_atiDuration - (Simulator::Now () - m_transmissionStarted);
  if (m_remainingDuration <= Seconds (0))
    {
      m_allowTransmission = false;
      return;
    }

  if (m_currentPacket == 0)
    {
      if (m_queue->IsEmpty ())
        {
          NS_LOG_DEBUG ("queue empty");
          return;
        }
      Ptr<WifiMacQueueItem> item = m_queue->Dequeue ();
      NS_ASSERT (item != 0);
      m_currentPacket = item->GetPacket ();
      m_currentHdr = item->GetHeader ();
      NS_ASSERT (m_currentPacket != 0);
      uint16_t sequence = m_txMiddle->GetNextSequenceNumberFor (&m_currentHdr);
      m_currentHdr.SetSequenceNumber (sequence);
      m_currentHdr.SetNoMoreFragments ();
      m_currentHdr.SetNoRetry ();
      NS_LOG_DEBUG ("dequeued size=" << m_currentPacket->GetSize () <<
                    ", to=" << m_currentHdr.GetAddr1 () <<
                    ", seq=" << m_currentHdr.GetSequenceControl ());
    }

  m_currentParams.DisableOverrideDurationId ();
  m_currentParams.DisableRts ();
  m_currentParams.DisableNextData ();
  if (m_currentHdr.IsCtl () || m_currentHdr.IsActionNoAck ())
    {
      m_currentParams.DisableAck ();
    }
  else if (m_currentHdr.IsMgt () )
    {
      m_currentParams.EnableAck ();
    }
  GetLow ()->TransmitSingleFrame (Create<WifiMacQueueItem> (m_currentPacket, m_currentHdr), m_currentParams, this);
}

void
DmgAtiTxop::NotifyInternalCollision (void)
{
  NS_LOG_FUNCTION (this);
  RestartAccessIfNeeded ();
}

void
DmgAtiTxop::GotAck (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("got ack. tx done.");
  if (!m_txOkCallback.IsNull ())
    {
      m_txOkCallback (m_currentPacket, m_currentHdr);
    }

  /* we are not fragmenting or we are done fragmenting
   * so we can get rid of that packet now.
   */
  m_currentPacket = 0;
  RestartAccessIfNeeded ();
}

void
DmgAtiTxop::MissedAck (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("missed ack");
  if (!NeedDataRetransmission ())
    {
      NS_LOG_DEBUG ("Ack Fail");
      m_stationManager->ReportFinalDataFailed (m_currentHdr.GetAddr1 (), &m_currentHdr, m_currentPacket->GetSize ());
      if (!m_txFailedCallback.IsNull ())
        {
          m_txFailedCallback (m_currentHdr);
        }
    }
  else
    {
      NS_LOG_DEBUG ("Retransmit");
    }
  RestartAccessIfNeeded ();
}

void
DmgAtiTxop::Cancel (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("transmission cancelled");
}

void
DmgAtiTxop::EndTxNoAck (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("a transmission that did not require an ACK just finished");
  m_currentPacket = 0;
  if (!m_txOkNoAckCallback.IsNull ())
    {
      m_txOkNoAckCallback (m_currentHdr);
    }
  StartAccessIfNeeded ();
}

} //namespace ns3
