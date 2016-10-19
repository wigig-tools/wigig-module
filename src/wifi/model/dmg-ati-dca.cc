/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/assert.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/node.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"

#include "dcf-manager.h"
#include "dmg-ati-dca.h"
#include "mac-low.h"
#include "mac-tx-middle.h"
#include "wifi-mac-queue.h"
#include "wifi-mac-trailer.h"
#include "wifi-mac.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT if (m_low != 0) { std::clog << "[mac=" << m_low->GetAddress () << "] "; }

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DmgAtiDca");

class DmgAtiDca::Dcf : public DcfState
{
public:
  Dcf (DmgAtiDca * txop)
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

  DmgAtiDca *m_txop;
};


/**
 * Listener for MacLow events. Forwards to DmgAtiDca.
 */
class DmgAtiDca::TransmissionListener : public MacLowTransmissionListener
{
public:
  /**
   * Create a TransmissionListener for the given DmgAtiDca.
   *
   * \param txop
   */
  TransmissionListener (DmgAtiDca * txop)
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
    m_txop->GotAck (snr, txMode);
  }
  virtual void MissedAck (void)
  {
    m_txop->MissedAck ();
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
  DmgAtiDca *m_txop;
};

NS_OBJECT_ENSURE_REGISTERED (DmgAtiDca);

TypeId
DmgAtiDca::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DmgAtiDca")
    .SetParent<ns3::Dcf> ()
    .SetGroupName ("Wifi")
    .AddConstructor<DmgAtiDca> ()
    .AddAttribute ("Queue", "The WifiMacQueue object",
                   PointerValue (),
                   MakePointerAccessor (&DmgAtiDca::GetQueue),
                   MakePointerChecker<WifiMacQueue> ())
  ;
  return tid;
}

DmgAtiDca::DmgAtiDca ()
  : m_manager (0),
    m_currentPacket (0)
{
  NS_LOG_FUNCTION (this);
  m_transmissionListener = new DmgAtiDca::TransmissionListener (this);
  m_dcf = new DmgAtiDca::Dcf (this);
  m_queue = CreateObject<WifiMacQueue> ();
  m_allowTransmission = false;
}

DmgAtiDca::~DmgAtiDca ()
{
  NS_LOG_FUNCTION (this);
}

void
DmgAtiDca::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_queue = 0;
  m_low = 0;
  m_stationManager = 0;
  delete m_transmissionListener;
  delete m_dcf;
  m_transmissionListener = 0;
  m_dcf = 0;
  m_txMiddle = 0;
}

void
DmgAtiDca::SetManager (DcfManager *manager)
{
  NS_LOG_FUNCTION (this << manager);
  m_manager = manager;
  m_manager->Add (m_dcf);
}

void DmgAtiDca::SetTxMiddle (MacTxMiddle *txMiddle)
{
  m_txMiddle = txMiddle;
}

void
DmgAtiDca::SetLow (Ptr<MacLow> low)
{
  NS_LOG_FUNCTION (this << low);
  m_low = low;
}

void
DmgAtiDca::SetWifiRemoteStationManager (Ptr<WifiRemoteStationManager> remoteManager)
{
  NS_LOG_FUNCTION (this << remoteManager);
  m_stationManager = remoteManager;
}

void
DmgAtiDca::SetTxOkCallback (TxPacketOk callback)
{
  NS_LOG_FUNCTION (this << &callback);
  m_txOkCallback = callback;
}

void
DmgAtiDca::SetTxOkNoAckCallback (TxOk callback)
{
  NS_LOG_FUNCTION (this << &callback);
  m_txOkNoAckCallback = callback;
}

void
DmgAtiDca::SetTxFailedCallback (TxFailed callback)
{
  NS_LOG_FUNCTION (this << &callback);
  m_txFailedCallback = callback;
}

Ptr<WifiMacQueue >
DmgAtiDca::GetQueue () const
{
  NS_LOG_FUNCTION (this);
  return m_queue;
}

void
DmgAtiDca::SetMinCw (uint32_t minCw)
{
  NS_LOG_FUNCTION (this << minCw);
  m_dcf->SetCwMin (minCw);
}

void
DmgAtiDca::SetMaxCw (uint32_t maxCw)
{
  NS_LOG_FUNCTION (this << maxCw);
  m_dcf->SetCwMax (maxCw);
}

void
DmgAtiDca::SetAifsn (uint32_t aifsn)
{
  NS_LOG_FUNCTION (this << aifsn);
  m_dcf->SetAifsn (aifsn);
}

uint32_t
DmgAtiDca::GetMinCw (void) const
{
  NS_LOG_FUNCTION (this);
  return m_dcf->GetCwMin ();
}

uint32_t
DmgAtiDca::GetMaxCw (void) const
{
  NS_LOG_FUNCTION (this);
  return m_dcf->GetCwMax ();
}

uint32_t
DmgAtiDca::GetAifsn (void) const
{
  NS_LOG_FUNCTION (this);
  return m_dcf->GetAifsn ();
}

void
DmgAtiDca::Queue (Ptr<const Packet> packet, const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << packet << &hdr);
  m_stationManager->PrepareForQueue (hdr.GetAddr1 (), &hdr, packet);
  m_queue->Enqueue (packet, hdr);
  StartAccessIfNeeded ();
}

void
DmgAtiDca::RestartAccessIfNeeded (void)
{
  NS_LOG_FUNCTION (this);
  if ((m_currentPacket != 0
       || !m_queue->IsEmpty ())
      && !m_dcf->IsAccessRequested ()
      && m_allowTransmission)
    {
      m_manager->RequestAccess (m_dcf);
    }
}

void
DmgAtiDca::StartAccessIfNeeded (void)
{
  NS_LOG_FUNCTION (this);
  if (m_currentPacket == 0
      && !m_queue->IsEmpty ()
      && !m_dcf->IsAccessRequested ()
      && m_allowTransmission)
    {
      m_manager->RequestAccess (m_dcf);
    }
}

void
DmgAtiDca::InitiateAtiAccessPeriod (Time atiDuration)
{
  NS_LOG_FUNCTION (this << atiDuration);
  m_atiDuration = atiDuration;
  m_allowTransmission = true;
  m_transmissionStarted = Simulator::Now ();
  m_queue->Empty ();
  Simulator::Schedule (atiDuration, &DmgAtiDca::DisableTransmission, this);
}

void
DmgAtiDca::InitiateTransmission (Time atiDuration)
{
  NS_LOG_FUNCTION (this << atiDuration);
  InitiateAtiAccessPeriod (atiDuration);
  StartAccessIfNeeded ();
}

void
DmgAtiDca::DisableTransmission (void)
{
  NS_LOG_FUNCTION (this);
  m_allowTransmission = false;
}

Ptr<MacLow>
DmgAtiDca::Low (void)
{
  NS_LOG_FUNCTION (this);
  return m_low;
}

void
DmgAtiDca::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  ns3::Dcf::DoInitialize ();
}

bool
DmgAtiDca::NeedDataRetransmission (void)
{
  NS_LOG_FUNCTION (this);
  return m_stationManager->NeedDataRetransmission (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                                   m_currentPacket);
}

void
DmgAtiDca::NotifyAccessGranted (void)
{
  NS_LOG_FUNCTION (this);

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
      m_currentPacket = m_queue->Dequeue (&m_currentHdr);
      NS_ASSERT (m_currentPacket != 0);
      uint16_t sequence = m_txMiddle->GetNextSequenceNumberfor (&m_currentHdr);
      m_currentHdr.SetSequenceNumber (sequence);
      m_currentHdr.SetNoMoreFragments ();
      m_currentHdr.SetNoRetry ();
      NS_LOG_DEBUG ("dequeued size=" << m_currentPacket->GetSize () <<
                    ", to=" << m_currentHdr.GetAddr1 () <<
                    ", seq=" << m_currentHdr.GetSequenceControl ());
    }

  MacLowTransmissionParameters params;
  params.SetAsBoundedTransmission ();
  params.SetMaximumTransmissionDuration (m_remainingDuration);
  params.DisableOverrideDurationId ();
  params.DisableRts ();
  params.DisableNextData ();
  if (m_currentHdr.IsCtl () || m_currentHdr.IsActionNoAck ())
    {
      params.DisableAck ();
    }
  else if (m_currentHdr.IsMgt () )
    {
      params.EnableAck ();
    }
  Low ()->TransmitSingleFrame (m_currentPacket, &m_currentHdr, params, m_transmissionListener);
}

void
DmgAtiDca::NotifyInternalCollision (void)
{
  NS_LOG_FUNCTION (this);
  NotifyCollision ();
}

void
DmgAtiDca::NotifyCollision (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("collision");
  RestartAccessIfNeeded ();
}

void
DmgAtiDca::GotAck (double snr, WifiMode txMode)
{
  NS_LOG_FUNCTION (this << snr << txMode);
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
DmgAtiDca::MissedAck (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("missed ack");
  if (!NeedDataRetransmission ())
    {
      NS_LOG_DEBUG ("Ack Fail");
      m_stationManager->ReportFinalDataFailed (m_currentHdr.GetAddr1 (), &m_currentHdr);
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
DmgAtiDca::Cancel (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("transmission cancelled");
}

void
DmgAtiDca::EndTxNoAck (void)
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

void
DmgAtiDca::SetTxopLimit (Time txopLimit)
{
  NS_LOG_FUNCTION (this << txopLimit);
  m_dcf->SetTxopLimit (txopLimit);
}

Time
DmgAtiDca::GetTxopLimit (void) const
{
  NS_LOG_FUNCTION (this);
  return m_dcf->GetTxopLimit ();
}

} //namespace ns3
