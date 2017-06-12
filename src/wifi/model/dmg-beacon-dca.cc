/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015,2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@hotmail.com>
 */
#include "ns3/assert.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"

#include "dcf-state.h"
#include "dmg-ap-wifi-mac.h"
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
  : m_transmittingBeacon (false)
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
DmgBeaconDca::SetWifiMac (Ptr<WifiMac> mac)
{
  m_wifiMac = mac;
}

void
DmgBeaconDca::InitiateBTI (void)
{
  m_senseChannel = true;
}

void
DmgBeaconDca::TransmitDmgBeacon (const ExtDMGBeacon &beacon, const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << &beacon << &hdr);
  NS_ASSERT ((m_transmittingBeacon == false) && (!m_dcf->IsAccessRequested ()));
  m_currentHdr = hdr;
  m_currentBeacon = beacon;
  m_transmittingBeacon = true;
  if (m_senseChannel)
    {
      m_senseChannel = false;
      m_manager->RequestAccess (m_dcf);
    }
  else
    {
      NotifyAccessGranted ();
    }
}

void
DmgBeaconDca::RestartAccessIfNeeded (void)
{
  NS_LOG_FUNCTION (this);
  if (m_transmittingBeacon == true && !m_dcf->IsAccessRequested ())
    {
      m_manager->RequestAccess (m_dcf);
    }
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
  Ptr<DmgApWifiMac> wifiMac = StaticCast<DmgApWifiMac> (m_wifiMac);
  /**
   * A STA sending a DMG Beacon or an Announce frame shall set the value of the frame’s timestamp field to equal the
   * value of the STA’s TSF timer at the time that the transmission of the data symbol containing the first bit of
   * the MPDU is started on the air (which can be derived from the PHY-TXPLCPEND.indication primitive), including any
   * transmitting STA’s delays through its local PHY from the MAC-PHY interface to its interface with the WM.
   */
  m_currentBeacon.SetTimestamp (Simulator::Now ().GetMicroSeconds ());

  /* The Duration field is set to the time remaining until the end of the BTI. */
  m_currentParams.EnableOverrideDurationId (wifiMac->GetBTIRemainingTime ());
  m_currentParams.DisableRts ();
  m_currentParams.DisableAck ();
  m_currentParams.DisableNextData ();
  NS_LOG_DEBUG ("DMG Beacon granted access with remaining BTI Period= " << wifiMac->GetBTIRemainingTime ());

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (m_currentBeacon);
  GetLow ()->TransmitSingleFrame (packet, &m_currentHdr, m_currentParams, this);
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
  NS_LOG_DEBUG ("DMG Beacon Collision");
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
  m_transmittingBeacon = false;
  if (!m_txOkNoAckCallback.IsNull ())
    {
      m_txOkNoAckCallback (m_currentHdr);
    }
}

} //namespace ns3
