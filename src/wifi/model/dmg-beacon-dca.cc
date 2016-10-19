/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015,2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@hotmail.com>
 */
#include "ns3/assert.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/node.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"

#include "dcf-manager.h"
#include "dmg-ap-wifi-mac.h"
#include "dmg-beacon-dca.h"
#include "mac-low.h"
#include "wifi-mac-queue.h"
#include "wifi-mac-trailer.h"
#include "wifi-mac.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT if (m_low != 0) { std::clog << "[mac=" << m_low->GetAddress () << "] "; }

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DmgBeaconDca");

class DmgBeaconDca::Dcf : public DcfState
{
public:
  Dcf (DmgBeaconDca * txop)
    : m_txop (txop)
  {
  }
  virtual bool IsEdca (void) const
  {
    return false;
  }
private:
  virtual void DoNotifyAccessGranted (void)
  {
    m_txop->NotifyAccessGranted ();
  }
  virtual void DoNotifyInternalCollision (void)
  {
    m_txop->NotifyInternalCollision ();
  }
  virtual void DoNotifyCollision (void)
  {
    m_txop->NotifyCollision ();
  }
  virtual void DoNotifyChannelSwitching (void)
  {
  }
  virtual void DoNotifySleep (void)
  {
  }
  virtual void DoNotifyWakeUp (void)
  {
  }

  DmgBeaconDca *m_txop;
};


/**
 * Listener for MacLow events. Forwards to DmgBeaconDca.
 */
class DmgBeaconDca::TransmissionListener : public MacLowTransmissionListener
{
public:
  /**
   * Create a TransmissionListener for the given DmgBeaconDca.
   *
   * \param txop
   */
  TransmissionListener (DmgBeaconDca * txop)
    : MacLowTransmissionListener (),
      m_txop (txop)
  {
  }

  virtual ~TransmissionListener ()
  {
  }
  virtual void GotCts (double snr, WifiMode txMode)
  {
  }
  virtual void MissedCts (void)
  {
  }
  virtual void GotAck (double snr, WifiMode txMode)
  {
  }
  virtual void MissedAck (void)
  {
  }
  virtual void StartNextFragment (void)
  {
  }
  virtual void StartNext (void)
  {
  }
  virtual void Cancel (void)
  {
    m_txop->Cancel ();
  }
  virtual void EndTxNoAck (void)
  {
    m_txop->EndTxNoAck ();
  }

private:
  DmgBeaconDca *m_txop;
};

NS_OBJECT_ENSURE_REGISTERED (DmgBeaconDca);

TypeId
DmgBeaconDca::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgBeaconDca")
    .SetParent<ns3::Dcf> ()
    .SetGroupName ("Wifi")
    .AddConstructor<DmgBeaconDca> ()
  ;
  return tid;
}

DmgBeaconDca::DmgBeaconDca ()
  : m_manager (0),
    m_transmittingBeacon (false)
{
  NS_LOG_FUNCTION (this);
  m_transmissionListener = new DmgBeaconDca::TransmissionListener (this);
  m_dcf = new DmgBeaconDca::Dcf (this);
}

DmgBeaconDca::~DmgBeaconDca ()
{
  NS_LOG_FUNCTION (this);
}

void
DmgBeaconDca::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_low = 0;
  m_stationManager = 0;
  delete m_transmissionListener;
  delete m_dcf;
  m_transmissionListener = 0;
  m_dcf = 0;
}

void
DmgBeaconDca::SetManager (DcfManager *manager)
{
  NS_LOG_FUNCTION (this << manager);
  m_manager = manager;
  m_manager->Add (m_dcf);
}

void
DmgBeaconDca::SetLow (Ptr<MacLow> low)
{
  NS_LOG_FUNCTION (this << low);
  m_low = low;
}

void
DmgBeaconDca::SetWifiRemoteStationManager (Ptr<WifiRemoteStationManager> remoteManager)
{
  NS_LOG_FUNCTION (this << remoteManager);
  m_stationManager = remoteManager;
}

void
DmgBeaconDca::SetTxOkNoAckCallback (TxOk callback)
{
  NS_LOG_FUNCTION (this << &callback);
  m_txOkNoAckCallback = callback;
}

void
DmgBeaconDca::SetTxFailedCallback (TxFailed callback)
{
  NS_LOG_FUNCTION (this << &callback);
  m_txFailedCallback = callback;
}

void
DmgBeaconDca::SetWifiMac (Ptr<WifiMac> mac)
{
  m_wifiMac = mac;
}

void
DmgBeaconDca::SetMinCw (uint32_t minCw)
{
  NS_LOG_FUNCTION (this << minCw);
  m_dcf->SetCwMin (minCw);
}

void
DmgBeaconDca::SetMaxCw (uint32_t maxCw)
{
  NS_LOG_FUNCTION (this << maxCw);
  m_dcf->SetCwMax (maxCw);
}

void
DmgBeaconDca::SetAifsn (uint32_t aifsn)
{
  NS_LOG_FUNCTION (this << aifsn);
  m_dcf->SetAifsn (aifsn);
}

uint32_t
DmgBeaconDca::GetMinCw (void) const
{
  NS_LOG_FUNCTION (this);
  return m_dcf->GetCwMin ();
}

uint32_t
DmgBeaconDca::GetMaxCw (void) const
{
  NS_LOG_FUNCTION (this);
  return m_dcf->GetCwMax ();
}

uint32_t
DmgBeaconDca::GetAifsn (void) const
{
  NS_LOG_FUNCTION (this);
  return m_dcf->GetAifsn ();
}

void
DmgBeaconDca::TransmitDmgBeacon (const ExtDMGBeacon &beacon, const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << &beacon << &hdr);
  NS_ASSERT ((m_transmittingBeacon == false) && !m_dcf->IsAccessRequested ());
  m_currentHdr = hdr;
  m_currentBeacon = beacon;
  m_transmittingBeacon = true;
  m_manager->RequestAccess (m_dcf);
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

Ptr<MacLow>
DmgBeaconDca::Low (void)
{
  NS_LOG_FUNCTION (this);
  return m_low;
}

void
DmgBeaconDca::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  ns3::Dcf::DoInitialize ();
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

  MacLowTransmissionParameters params;
  /* The Duration field is set to the time remaining until the end of the BTI. */
  params.EnableOverrideDurationId (wifiMac->GetBTIRemainingTime ());
  params.DisableRts ();
  params.DisableAck ();
  params.DisableNextData ();
  NS_LOG_DEBUG ("DMG Beacon granted access with remaining BTI Period= " << wifiMac->GetBTIRemainingTime ());
//  std::cout << Simulator::Now () << " DMG Beacon granted access with remaining BTI Period= "
//            << wifiMac->GetBTIRemainingTime () << std::endl;

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (m_currentBeacon);
  Low ()->TransmitSingleFrame (packet, &m_currentHdr, params, m_transmissionListener);
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
//  RestartAccessIfNeeded ();
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

void
DmgBeaconDca::SetTxopLimit (Time txopLimit)
{
  NS_LOG_FUNCTION (this << txopLimit);
  m_dcf->SetTxopLimit (txopLimit);
}

Time
DmgBeaconDca::GetTxopLimit (void) const
{
  NS_LOG_FUNCTION (this);
  return m_dcf->GetTxopLimit ();
}

} //namespace ns3
