/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

#include "dmg-beacon-txop.h"
#include "mac-low.h"
#include "wifi-mac-queue.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT if (m_low != 0) { std::clog << "[mac=" << m_low->GetAddress () << "] "; }

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DmgBeaconTxop");

NS_OBJECT_ENSURE_REGISTERED (DmgBeaconTxop);

TypeId
DmgBeaconTxop::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgBeaconTxop")
    .SetParent<Txop> ()
    .SetGroupName ("Wifi")
    .AddConstructor<DmgBeaconTxop> ()
  ;
  return tid;
}

DmgBeaconTxop::DmgBeaconTxop ()
{
  NS_LOG_FUNCTION (this);
}

DmgBeaconTxop::~DmgBeaconTxop ()
{
  NS_LOG_FUNCTION (this);
}

void
DmgBeaconTxop::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  Txop::DoInitialize ();
}

void
DmgBeaconTxop::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Txop::DoDispose ();
}

void
DmgBeaconTxop::PerformCCA (void)
{
  NS_LOG_FUNCTION (this);
  ResetCw ();
  GenerateBackoff ();
  RestartAccessIfNeeded ();
}

void
DmgBeaconTxop::SetAccessGrantedCallback (AccessGranted callback)
{
  m_accessGrantedCallback = callback;
}

void
DmgBeaconTxop::TransmitDmgBeacon (const ExtDMGBeacon &beacon, const WifiMacHeader &hdr, Time BTIRemainingTime)
{
  NS_LOG_FUNCTION (this << &beacon << &hdr << BTIRemainingTime);
  m_currentHdr = hdr;

  /* The Duration field is set to the time remaining until the end of the BTI. */
  m_currentParams.EnableOverrideDurationId (BTIRemainingTime);
  m_currentParams.DisableRts ();
  m_currentParams.DisableAck ();
  m_currentParams.DisableNextData ();

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (beacon);
  GetLow ()->TransmitSingleFrame (Create<WifiMacQueueItem> (packet, m_currentHdr), m_currentParams, this);
}

void
DmgBeaconTxop::RestartAccessIfNeeded (void)
{
  NS_LOG_FUNCTION (this);
  if (!IsAccessRequested ())
    {
      m_channelAccessManager->RequestAccess (this);
    }
}

void
DmgBeaconTxop::NotifyAccessGranted (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_accessRequested);
  m_accessRequested = false;
  m_accessGrantedCallback ();
}

void
DmgBeaconTxop::NotifyInternalCollision (void)
{
  NS_LOG_FUNCTION (this);
  GenerateBackoff ();
  RestartAccessIfNeeded ();
}

void
DmgBeaconTxop::Cancel (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("Transmission cancelled");
}

void
DmgBeaconTxop::EndTxNoAck (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("a transmission that did not require an ACK just finished");
  if (!m_txOkNoAckCallback.IsNull ())
    {
      m_txOkNoAckCallback (m_currentHdr);
    }
}

} //namespace ns3
