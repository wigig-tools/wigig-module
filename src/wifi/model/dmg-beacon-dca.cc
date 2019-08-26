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
#include "dmg-beacon-dca.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT if (m_low != 0) { std::clog << "[mac=" << m_low->GetAddress () << "] "; }

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DmgBeaconDca");

NS_OBJECT_ENSURE_REGISTERED (DmgBeaconDca);

TypeId
DmgBeaconDca::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgBeaconDca")
    .SetParent<DcaTxop> ()
    .SetGroupName ("Wifi")
    .AddConstructor<DmgBeaconDca> ()
  ;
  return tid;
}

DmgBeaconDca::DmgBeaconDca ()
{
  NS_LOG_FUNCTION (this);
}

DmgBeaconDca::~DmgBeaconDca ()
{
  NS_LOG_FUNCTION (this);
}

void
DmgBeaconDca::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  DcaTxop::DoDispose ();
}

void
DmgBeaconDca::PerformCCA (void)
{
  NS_LOG_FUNCTION (this);
  if (!m_dcf->IsAccessRequested ())
    {
    //  NS_ASSERT ((!m_dcf->IsAccessRequested ()));
      m_manager->RequestAccess (m_dcf);
    }
}

void
DmgBeaconDca::SetAccessGrantedCallback (AccessGranted callback)
{
  m_accessGrantedCallback = callback;
}

void
DmgBeaconDca::TransmitDmgBeacon (const ExtDMGBeacon &beacon, const WifiMacHeader &hdr, Time BTIRemainingTime)
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
  GetLow ()->TransmitSingleFrame (packet, &hdr, m_currentParams, this);
}

void
DmgBeaconDca::RestartAccessIfNeeded (void)
{
  NS_LOG_FUNCTION (this);
}

void
DmgBeaconDca::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  DcaTxop::DoInitialize ();
}

void
DmgBeaconDca::NotifyAccessGranted (void)
{
  NS_LOG_FUNCTION (this);
  m_accessGrantedCallback ();
}

void
DmgBeaconDca::NotifyInternalCollision (void)
{
  NS_LOG_FUNCTION (this);
  NotifyCollision ();
}

void
DmgBeaconDca::NotifyCollision (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("Medium is busy, collision");
}

void
DmgBeaconDca::Cancel (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("Transmission cancelled");
}

void
DmgBeaconDca::EndTxNoAck (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("a transmission that did not require an ACK just finished");
  if (!m_txOkNoAckCallback.IsNull ())
    {
      m_txOkNoAckCallback (m_currentHdr);
    }
}

} //namespace ns3
