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
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"

#include "mac-low.h"
#include "mac-tx-middle.h"
#include "wifi-mac-trailer.h"
#include "wifi-mac.h"
#include "random-stream.h"
#include "wifi-mac-queue.h"
#include "msdu-aggregator.h"
#include "mpdu-aggregator.h"
#include "mgt-headers.h"
#include "qos-blocked-destinations.h"
#include "service-period.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT if (m_low != 0) { std::clog << "[mac=" << m_low->GetAddress () << "] "; }

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ServicePeriod");

class ServicePeriod::TransmissionListener : public MacLowTransmissionListener
{
public:
  TransmissionListener (ServicePeriod *sp)
    : MacLowTransmissionListener (),
      m_sp (sp)
  {
  }

  virtual ~TransmissionListener ()
  {
  }

  virtual void GotCts (double snr, WifiMode txMode)
  {
    m_sp->GotCts (snr, txMode);
  }
  virtual void MissedCts (void)
  {
    m_sp->MissedCts ();
  }
  virtual void GotAck (double snr, WifiMode txMode)
  {
    m_sp->GotAck (snr, txMode);
  }
  virtual void MissedAck (void)
  {
    m_sp->MissedAck ();
  }
  virtual void GotBlockAck (const CtrlBAckResponseHeader *blockAck, Mac48Address source, double rxSnr, WifiMode txMode, double dataSnr)
  {
    m_sp->GotBlockAck (blockAck, source, rxSnr, txMode, dataSnr);
  }
  virtual void MissedBlockAck (uint32_t nMpdus)
  {
    m_sp->MissedBlockAck (nMpdus);
  }
  virtual void StartNext (void)
  {
    m_sp->StartNext ();
  }
  virtual void Cancel (void)
  {
    m_sp->Cancel ();
  }
  virtual void EndTxNoAck (void)
  {
    m_sp->EndTxNoAck ();
  }
  virtual Ptr<WifiMacQueue> GetQueue (void)
  {
    return m_sp->GetQueue ();
  }

private:
  ServicePeriod *m_sp;

};

class ServicePeriod::AggregationCapableTransmissionListener : public MacLowAggregationCapableTransmissionListener
{
public:
  AggregationCapableTransmissionListener (ServicePeriod *sp)
    : MacLowAggregationCapableTransmissionListener (),
      m_sp (sp)
  {
  }
  virtual ~AggregationCapableTransmissionListener ()
  {
  }

  virtual void BlockAckInactivityTimeout (Mac48Address address, uint8_t tid)
  {
    m_sp->SendDelbaFrame (address, tid, false);
  }
  virtual Ptr<WifiMacQueue> GetQueue (void)
  {
    return m_sp->GetQueue ();
  }
  virtual  void CompleteTransfer (Mac48Address recipient, uint8_t tid)
  {
    m_sp->CompleteAmpduTransfer (recipient, tid);
  }
  virtual void SetAmpdu (Mac48Address dest, bool enableAmpdu)
  {
    return m_sp->SetAmpduExist (dest, enableAmpdu);
  }
  virtual void CompleteMpduTx (Ptr<const Packet> packet, WifiMacHeader hdr, Time tstamp)
  {
    m_sp->CompleteMpduTx (packet, hdr, tstamp);
  }
  virtual uint16_t GetNextSequenceNumberfor (WifiMacHeader *hdr)
  {
    return m_sp->GetNextSequenceNumberfor (hdr);
  }
  virtual uint16_t PeekNextSequenceNumberfor (WifiMacHeader *hdr)
  {
    return m_sp->PeekNextSequenceNumberfor (hdr);
  }
  virtual Ptr<const Packet> PeekNextPacketInBaQueue (WifiMacHeader &header, Mac48Address recipient, uint8_t tid, Time *timestamp)
  {
    return m_sp->PeekNextRetransmitPacket (header, recipient, tid, timestamp);
  }
  virtual void RemoveFromBaQueue (uint8_t tid, Mac48Address recipient, uint16_t seqnumber)
  {
    m_sp->RemoveRetransmitPacket (tid, recipient, seqnumber);
  }
  virtual bool GetBlockAckAgreementExists (Mac48Address address, uint8_t tid)
  {
    return m_sp->GetBaAgreementExists (address,tid);
  }
  virtual uint32_t GetNOutstandingPackets (Mac48Address address, uint8_t tid)
  {
    return m_sp->GetNOutstandingPacketsInBa (address, tid);
  }
  virtual uint32_t GetNRetryNeededPackets (Mac48Address recipient, uint8_t tid) const
  {
    return m_sp->GetNRetryNeededPackets (recipient, tid);
  }
  virtual Ptr<MsduAggregator> GetMsduAggregator (void) const
  {
    return m_sp->GetMsduAggregator ();
  }
  virtual Ptr<MpduAggregator> GetMpduAggregator (void) const
  {
    return m_sp->GetMpduAggregator ();
  }
  virtual Mac48Address GetSrcAddressForAggregation (const WifiMacHeader &hdr)
  {
    return m_sp->MapSrcAddressForAggregation (hdr);
  }
  virtual Mac48Address GetDestAddressForAggregation (const WifiMacHeader &hdr)
  {
    return m_sp->MapDestAddressForAggregation (hdr);
  }

private:
  ServicePeriod *m_sp;
};

NS_OBJECT_ENSURE_REGISTERED (ServicePeriod);

TypeId
ServicePeriod::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ServicePeriod")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ServicePeriod> ()
    .AddAttribute ("Queue",
                   "The WifiMacQueue object",
                   PointerValue (),
                   MakePointerAccessor (&ServicePeriod::GetQueue),
                   MakePointerChecker<WifiMacQueue> ())
  ;
  return tid;
}

ServicePeriod::ServicePeriod ()
  : m_currentPacket (0),
    m_msduAggregator (0),
    m_mpduAggregator (0),
    m_typeOfStation (DMG_STA),
    m_blockAckType (COMPRESSED_BLOCK_ACK)
{
  NS_LOG_FUNCTION (this);
  m_transmissionListener = new ServicePeriod::TransmissionListener (this);
  m_blockAckListener = new ServicePeriod::AggregationCapableTransmissionListener (this);
  m_queue = CreateObject<WifiMacQueue> ();
  m_qosBlockedDestinations = new QosBlockedDestinations ();
  m_baManager = new BlockAckManager ();
  m_baManager->SetQueue (m_queue);
  m_baManager->SetBlockAckType (m_blockAckType);
  m_baManager->SetBlockDestinationCallback (MakeCallback (&QosBlockedDestinations::Block, m_qosBlockedDestinations));
  m_baManager->SetUnblockDestinationCallback (MakeCallback (&QosBlockedDestinations::Unblock, m_qosBlockedDestinations));
  m_baManager->SetMaxPacketDelay (m_queue->GetMaxDelay ());
  m_baManager->SetTxOkCallback (MakeCallback (&ServicePeriod::BaTxOk, this));
  m_baManager->SetTxFailedCallback (MakeCallback (&ServicePeriod::BaTxFailed, this));
}

ServicePeriod::~ServicePeriod ()
{
  NS_LOG_FUNCTION (this);
}

void
ServicePeriod::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_queue = 0;
  m_low = 0;
  m_stationManager = 0;
  delete m_transmissionListener;
  delete m_qosBlockedDestinations;
  delete m_baManager;
  delete m_blockAckListener;
  m_transmissionListener = 0;
  m_qosBlockedDestinations = 0;
  m_baManager = 0;
  m_blockAckListener = 0;
  m_txMiddle = 0;
  m_msduAggregator = 0;
  m_mpduAggregator = 0;
}

bool
ServicePeriod::GetBaAgreementExists (Mac48Address address, uint8_t tid)
{
  return m_baManager->ExistsAgreement (address, tid);
}

uint32_t
ServicePeriod::GetNOutstandingPacketsInBa (Mac48Address address, uint8_t tid)
{
  return m_baManager->GetNBufferedPackets (address, tid);
}

uint32_t
ServicePeriod::GetNRetryNeededPackets (Mac48Address recipient, uint8_t tid) const
{
  return m_baManager->GetNRetryNeededPackets (recipient, tid);
}

void
ServicePeriod::CompleteAmpduTransfer (Mac48Address recipient, uint8_t tid)
{
  m_baManager->CompleteAmpduExchange (recipient, tid);
}

void
ServicePeriod::SetTxOkCallback (TxPacketOk callback)
{
  NS_LOG_FUNCTION (this << &callback);
  m_txOkCallback = callback;
}

void
ServicePeriod::SetTxFailedCallback (TxFailed callback)
{
  NS_LOG_FUNCTION (this << &callback);
  m_txFailedCallback = callback;
}

void
ServicePeriod::SetWifiRemoteStationManager (Ptr<WifiRemoteStationManager> remoteManager)
{
  NS_LOG_FUNCTION (this << remoteManager);
  m_stationManager = remoteManager;
  m_baManager->SetWifiRemoteStationManager (m_stationManager);
}

void
ServicePeriod::SetTypeOfStation (enum TypeOfStation type)
{
  NS_LOG_FUNCTION (this << static_cast<uint32_t> (type));
  m_typeOfStation = type;
}

enum TypeOfStation
ServicePeriod::GetTypeOfStation (void) const
{
  NS_LOG_FUNCTION (this);
  return m_typeOfStation;
}

Ptr<WifiMacQueue>
ServicePeriod::GetQueue () const
{
  NS_LOG_FUNCTION (this);
  return m_queue;
}

void
ServicePeriod::SetTxMiddle (MacTxMiddle *txMiddle)
{
  NS_LOG_FUNCTION (this << txMiddle);
  m_txMiddle = txMiddle;
}

Ptr<MacLow>
ServicePeriod::Low (void)
{
  NS_LOG_FUNCTION (this);
  return m_low;
}

void
ServicePeriod::SetLow (Ptr<MacLow> low)
{
  NS_LOG_FUNCTION (this << low);
  m_low = low;
}

bool
ServicePeriod::NeedsAccess (void) const
{
  NS_LOG_FUNCTION (this);
  return !m_queue->IsEmpty () || m_currentPacket != 0 || m_baManager->HasPackets ();
}

uint16_t
ServicePeriod::GetNextSequenceNumberfor (WifiMacHeader *hdr)
{
  return m_txMiddle->GetNextSequenceNumberfor (hdr);
}

uint16_t
ServicePeriod::PeekNextSequenceNumberfor (WifiMacHeader *hdr)
{
  return m_txMiddle->PeekNextSequenceNumberfor (hdr);
}

Ptr<const Packet>
ServicePeriod::PeekNextRetransmitPacket (WifiMacHeader &header,Mac48Address recipient, uint8_t tid, Time *timestamp)
{
  return m_baManager->PeekNextPacket (header,recipient,tid, timestamp);
}

void
ServicePeriod::RemoveRetransmitPacket (uint8_t tid, Mac48Address recipient, uint16_t seqnumber)
{
  m_baManager->RemovePacket (tid, recipient, seqnumber);
}

void
ServicePeriod::AllowChannelAccess (void)
{
  m_accessAllowed = true;
}

void
ServicePeriod::DisableChannelAccess (void)
{
  m_accessAllowed = false;
}

void
ServicePeriod::NotifyAccessGranted (void)
{
  NS_LOG_FUNCTION (this);
  /* Update SP duration */
  m_remainingDuration = m_servicePeriodDuration - (Simulator::Now () - m_transmissionStarted);
  if (m_remainingDuration <= Seconds (0))
    {
      m_accessAllowed = false;
      return;
    }

  if (m_currentPacket == 0)
    {
      if (m_baManager->HasBar (m_currentBar))
        {
          SendBlockAckRequest (m_currentBar);
          return;
        }
      /* check if packets need retransmission are stored in BlockAckManager */
      m_currentPacket = m_baManager->GetNextPacket (m_currentHdr);
      if (m_currentPacket == 0)
        {
          /* Check if there is any available packets for the destination DMG STA in this SP */
          if (m_queue->PeekFirstAvailableByAddress (&m_currentHdr, m_currentPacketTimestamp,
                                                    WifiMacHeader::ADDR1, m_peerStation, m_qosBlockedDestinations) == 0)
            {
              return;
            }
          if (m_currentHdr.IsQosData () && !m_currentHdr.GetAddr1 ().IsBroadcast ()
              && !m_baManager->ExistsAgreement (m_currentHdr.GetAddr1 (), m_currentHdr.GetQosTid ())
              && SetupBlockAckIfNeeded ())
            {
              return;
            }

          m_currentPacket = m_queue->DequeueByAddress (&m_currentHdr, WifiMacHeader::ADDR1,
                                                       m_peerStation, m_qosBlockedDestinations);
          if (m_currentPacket != 0)
            {
              uint16_t sequence = m_txMiddle->GetNextSequenceNumberfor (&m_currentHdr);
              m_currentHdr.SetSequenceNumber (sequence);
              m_stationManager->UpdateFragmentationThreshold ();
              m_currentHdr.SetFragmentNumber (0);
              m_currentHdr.SetNoMoreFragments ();
              m_currentHdr.SetNoRetry ();
              m_fragmentNumber = 0;
              NS_LOG_DEBUG ("dequeued size=" << m_currentPacket->GetSize () <<
                            ", to=" << m_currentHdr.GetAddr1 () <<
                            ", seq=" << m_currentHdr.GetSequenceControl ());
              if (m_currentHdr.IsQosData () && !m_currentHdr.GetAddr1 ().IsBroadcast ())
                {
                  VerifyBlockAck ();
                }
            }
        }
    }

  if (m_currentPacket != 0)
    {
      MacLowTransmissionParameters params;
      params.SetAsBoundedTransmission ();
      params.SetMaximumTransmissionDuration (m_remainingDuration);
      params.EnableOverrideDurationId (m_remainingDuration);
      params.SetTransmitInSericePeriod ();
      if (m_currentHdr.GetType () == WIFI_MAC_CTL_BACKREQ)
        {
          SendBlockAckRequest (m_currentBar);
        }
      else
        {
          if (m_currentHdr.IsQosData () && m_currentHdr.IsQosBlockAck ())
            {
              params.DisableAck ();
            }
          else
            {
              params.EnableAck ();
            }
          if (((m_currentHdr.IsQosData () && !m_currentHdr.IsQosAmsdu ())
              || (m_currentHdr.IsData () && !m_currentHdr.IsQosData () && m_currentHdr.IsQosAmsdu ()))
              && (m_blockAckThreshold == 0 || m_blockAckType == BASIC_BLOCK_ACK)
              && NeedFragmentation ())
            {
              //With COMPRESSED_BLOCK_ACK fragmentation must be avoided.
              params.DisableRts ();
              WifiMacHeader hdr;
              Ptr<Packet> fragment = GetFragmentPacket (&hdr);
              if (IsLastFragment ())
                {
                  NS_LOG_DEBUG ("fragmenting last fragment size=" << fragment->GetSize ());
                  params.DisableNextData ();
                }
              else
                {
                  NS_LOG_DEBUG ("fragmenting size=" << fragment->GetSize ());
                  params.EnableNextData (GetNextFragmentSize ());
                }
              m_low->StartTransmission (fragment, &hdr, params,
                                        m_transmissionListener);
            }
          else
            {
              WifiMacHeader peekedHdr;
              Time tstamp;
              if (m_currentHdr.IsQosData ()
                  && m_queue->PeekByTidAndAddress (&peekedHdr, m_currentHdr.GetQosTid (),
                                                   WifiMacHeader::ADDR1, m_currentHdr.GetAddr1 (), &tstamp)
                  && !m_currentHdr.GetAddr1 ().IsBroadcast ()
                  && m_msduAggregator != 0 && !m_currentHdr.IsRetry ())
                {
                  /* here is performed MSDU aggregation */
                  Ptr<Packet> currentAggregatedPacket = Create<Packet> ();
                  m_msduAggregator->Aggregate (m_currentPacket, currentAggregatedPacket,
                                               MapSrcAddressForAggregation (peekedHdr),
                                               MapDestAddressForAggregation (peekedHdr));
                  bool aggregated = false;
                  bool isAmsdu = false;
                  Ptr<const Packet> peekedPacket = m_queue->PeekByTidAndAddress (&peekedHdr, m_currentHdr.GetQosTid (),
                                                                                 WifiMacHeader::ADDR1,
                                                                                 m_currentHdr.GetAddr1 (), &tstamp);
                  while (peekedPacket != 0)
                    {
                      aggregated = m_msduAggregator->Aggregate (peekedPacket, currentAggregatedPacket,
                                                                MapSrcAddressForAggregation (peekedHdr),
                                                                MapDestAddressForAggregation (peekedHdr));
                      if (aggregated)
                        {
                          isAmsdu = true;
                          m_queue->Remove (peekedPacket);
                        }
                      else
                        {
                          break;
                        }
                      peekedPacket = m_queue->PeekByTidAndAddress (&peekedHdr, m_currentHdr.GetQosTid (),
                                                                   WifiMacHeader::ADDR1, m_currentHdr.GetAddr1 (), &tstamp);
                    }
                  if (isAmsdu)
                    {
                      m_currentHdr.SetQosAmsdu ();
                      m_currentHdr.SetAddr3 (m_low->GetBssid ());
                      m_currentPacket = currentAggregatedPacket;
                      currentAggregatedPacket = 0;
                      NS_LOG_DEBUG ("tx unicast A-MSDU");
                    }
                }
              params.DisableNextData ();
              m_low->StartTransmission (m_currentPacket, &m_currentHdr,
                                        params, m_transmissionListener);
              if (!GetAmpduExist (m_currentHdr.GetAddr1 ()))
                {
                  CompleteTx ();
                }
            }
        }
    }
}

void
ServicePeriod::GotCts (double snr, WifiMode txMode)
{
  NS_LOG_FUNCTION (this << snr << txMode);
  NS_LOG_DEBUG ("got cts");
}

void
ServicePeriod::MissedCts (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("missed cts");
  if (!NeedRtsRetransmission ())
    {
      NS_LOG_DEBUG ("Cts Fail");
      bool resetCurrentPacket = true;
      m_stationManager->ReportFinalRtsFailed (m_currentHdr.GetAddr1 (), &m_currentHdr);
      if (!m_txFailedCallback.IsNull ())
        {
          m_txFailedCallback (m_currentHdr);
        }
      if (GetAmpduExist (m_currentHdr.GetAddr1 ()))
        {
          m_low->FlushAggregateQueue ();
          uint8_t tid = 0;
          if (m_currentHdr.IsQosData ())
            {
              tid = m_currentHdr.GetQosTid ();
            }
          else if (m_currentHdr.IsBlockAckReq ())
            {
              CtrlBAckRequestHeader baReqHdr;
              m_currentPacket->PeekHeader (baReqHdr);
              tid = baReqHdr.GetTidInfo ();
            }
          else
            {
              NS_FATAL_ERROR ("Current packet is not Qos Data nor BlockAckReq");
            }

          if (GetBaAgreementExists (m_currentHdr.GetAddr1 (), tid))
            {
              NS_LOG_DEBUG ("Transmit Block Ack Request");
              CtrlBAckRequestHeader reqHdr;
              reqHdr.SetType (COMPRESSED_BLOCK_ACK);
              reqHdr.SetStartingSequence (m_txMiddle->PeekNextSequenceNumberfor (&m_currentHdr));
              reqHdr.SetTidInfo (tid);
              reqHdr.SetHtImmediateAck (true);
              Ptr<Packet> bar = Create<Packet> ();
              bar->AddHeader (reqHdr);
              Bar request (bar, m_currentHdr.GetAddr1 (), tid, reqHdr.MustSendHtImmediateAck ());
              m_currentBar = request;
              WifiMacHeader hdr;
              hdr.SetType (WIFI_MAC_CTL_BACKREQ);
              hdr.SetAddr1 (request.recipient);
              hdr.SetAddr2 (m_low->GetAddress ());
              hdr.SetAddr3 (m_low->GetBssid ());
              hdr.SetDsNotTo ();
              hdr.SetDsNotFrom ();
              hdr.SetNoRetry ();
              hdr.SetNoMoreFragments ();
              m_currentPacket = request.bar;
              m_currentHdr = hdr;
              resetCurrentPacket = false;
            }
        }
      if (resetCurrentPacket == true)
        {
          m_currentPacket = 0;
        }
    }
  RestartAccessIfNeeded ();
}

void
ServicePeriod::NotifyChannelSwitching (void)
{
  NS_LOG_FUNCTION (this);
  m_queue->Flush ();
  m_currentPacket = 0;
}

void
ServicePeriod::NotifySleep (void)
{
  NS_LOG_FUNCTION (this);
  if (m_currentPacket != 0)
    {
      m_queue->PushFront (m_currentPacket, m_currentHdr);
      m_currentPacket = 0;
    }
}

void
ServicePeriod::NotifyWakeUp (void)
{
  NS_LOG_FUNCTION (this);
  RestartAccessIfNeeded ();
}

void
ServicePeriod::GotAck (double snr, WifiMode txMode)
{
  NS_LOG_FUNCTION (this << snr << txMode);
  if (!NeedFragmentation ()
      || IsLastFragment ()
      || m_currentHdr.IsQosAmsdu ())
    {
      NS_LOG_DEBUG ("got ack. tx done.");
      if (!m_txOkCallback.IsNull ())
        {
          m_txOkCallback (m_currentPacket, m_currentHdr);
        }

      if (m_currentHdr.IsAction ())
        {
          WifiActionHeader actionHdr;
          Ptr<Packet> p = m_currentPacket->Copy ();
          p->RemoveHeader (actionHdr);
          if (actionHdr.GetCategory () == WifiActionHeader::BLOCK_ACK
              && actionHdr.GetAction ().blockAck == WifiActionHeader::BLOCK_ACK_DELBA)
            {
              MgtDelBaHeader delBa;
              p->PeekHeader (delBa);
              if (delBa.IsByOriginator ())
                {
                  m_baManager->TearDownBlockAck (m_currentHdr.GetAddr1 (), delBa.GetTid ());
                }
              else
                {
                  m_low->DestroyBlockAckAgreement (m_currentHdr.GetAddr1 (), delBa.GetTid ());
                }
            }
        }
      m_currentPacket = 0;
      RestartAccessIfNeeded ();
    }
  else
    {
      NS_LOG_DEBUG ("got ack. tx not done, size=" << m_currentPacket->GetSize ());
    }
}

void
ServicePeriod::MissedAck (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("missed ack");
  if (!NeedDataRetransmission ())
    {
      NS_LOG_DEBUG ("Ack Fail");
      m_stationManager->ReportFinalDataFailed (m_currentHdr.GetAddr1 (), &m_currentHdr);
      bool resetCurrentPacket = true;
      if (!m_txFailedCallback.IsNull ())
        {
          m_txFailedCallback (m_currentHdr);
        }
      if (GetAmpduExist (m_currentHdr.GetAddr1 ()))
        {
          uint8_t tid = 0;
          if (m_currentHdr.IsQosData ())
            {
              tid = m_currentHdr.GetQosTid ();
            }
          else
            {
              NS_FATAL_ERROR ("Current packet is not Qos Data");
            }

          if (GetBaAgreementExists (m_currentHdr.GetAddr1 (), tid))
            {
              //send Block ACK Request in order to shift WinStart at the receiver
              NS_LOG_DEBUG ("Transmit Block Ack Request");
              CtrlBAckRequestHeader reqHdr;
              reqHdr.SetType (COMPRESSED_BLOCK_ACK);
              reqHdr.SetStartingSequence (m_txMiddle->PeekNextSequenceNumberfor (&m_currentHdr));
              reqHdr.SetTidInfo (tid);
              reqHdr.SetHtImmediateAck (true);
              Ptr<Packet> bar = Create<Packet> ();
              bar->AddHeader (reqHdr);
              Bar request (bar, m_currentHdr.GetAddr1 (), tid, reqHdr.MustSendHtImmediateAck ());
              m_currentBar = request;
              WifiMacHeader hdr;
              hdr.SetType (WIFI_MAC_CTL_BACKREQ);
              hdr.SetAddr1 (request.recipient);
              hdr.SetAddr2 (m_low->GetAddress ());
              hdr.SetAddr3 (m_low->GetBssid ());
              hdr.SetDsNotTo ();
              hdr.SetDsNotFrom ();
              hdr.SetNoRetry ();
              hdr.SetNoMoreFragments ();
              m_currentPacket = request.bar;
              m_currentHdr = hdr;
              resetCurrentPacket = false;
            }
        }

      if (resetCurrentPacket == true)
        {
          m_currentPacket = 0;
        }
    }
  else
    {
      NS_LOG_DEBUG ("Retransmit");
      m_currentHdr.SetRetry ();
    }
  RestartAccessIfNeeded ();
}

void
ServicePeriod::MissedBlockAck (uint32_t nMpdus)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("missed block ack");
  uint8_t tid = 0;
  if (m_currentHdr.IsQosData ())
    {
      tid = m_currentHdr.GetQosTid ();
    }
  else if (m_currentHdr.IsBlockAckReq ())
    {
      CtrlBAckRequestHeader baReqHdr;
      m_currentPacket->PeekHeader (baReqHdr);
      tid = baReqHdr.GetTidInfo ();
    }
  else if (m_currentHdr.IsBlockAck ())
    {
      CtrlBAckResponseHeader baRespHdr;
      m_currentPacket->PeekHeader (baRespHdr);
      tid = baRespHdr.GetTidInfo ();
    }
  if (GetAmpduExist (m_currentHdr.GetAddr1 ()))
    {
      m_stationManager->ReportAmpduTxStatus (m_currentHdr.GetAddr1 (), tid, 0, nMpdus, 0, 0);
    }
  if (NeedBarRetransmission ())
    {
      if (!GetAmpduExist (m_currentHdr.GetAddr1 ()))
        {
          //should i report this to station addressed by ADDR1?
          NS_LOG_DEBUG ("Retransmit block ack request");
          m_currentHdr.SetRetry ();
        }
      else
        {
          //standard says when loosing a BlockAck originator may send a BAR page 139
          NS_LOG_DEBUG ("Transmit Block Ack Request");
          CtrlBAckRequestHeader reqHdr;
          reqHdr.SetType (COMPRESSED_BLOCK_ACK);
          if (m_currentHdr.IsQosData ())
            {
              reqHdr.SetStartingSequence (m_currentHdr.GetSequenceNumber ());
            }
          else if (m_currentHdr.IsBlockAckReq ())
            {
              CtrlBAckRequestHeader baReqHdr;
              m_currentPacket->PeekHeader (baReqHdr);
              reqHdr.SetStartingSequence (baReqHdr.GetStartingSequence ());
            }
          else if (m_currentHdr.IsBlockAck ())
            {
              CtrlBAckResponseHeader baRespHdr;
              m_currentPacket->PeekHeader (baRespHdr);
              reqHdr.SetStartingSequence (m_currentHdr.GetSequenceNumber ());
            }
          reqHdr.SetTidInfo (tid);
          reqHdr.SetHtImmediateAck (true);
          Ptr<Packet> bar = Create<Packet> ();
          bar->AddHeader (reqHdr);
          Bar request (bar, m_currentHdr.GetAddr1 (), tid, reqHdr.MustSendHtImmediateAck ());
          m_currentBar = request;
          WifiMacHeader hdr;
          hdr.SetType (WIFI_MAC_CTL_BACKREQ);
          hdr.SetAddr1 (request.recipient);
          hdr.SetAddr2 (m_low->GetAddress ());
          hdr.SetAddr3 (m_low->GetBssid ());
          hdr.SetDsNotTo ();
          hdr.SetDsNotFrom ();
          hdr.SetNoRetry ();
          hdr.SetNoMoreFragments ();

          m_currentPacket = request.bar;
          m_currentHdr = hdr;
        }
    }
  else
    {
      NS_LOG_DEBUG ("Block Ack Request Fail");
      m_currentPacket = 0;
    }
}

Ptr<MsduAggregator>
ServicePeriod::GetMsduAggregator (void) const
{
  return m_msduAggregator;
}

Ptr<MpduAggregator>
ServicePeriod::GetMpduAggregator (void) const
{
  return m_mpduAggregator;
}

void
ServicePeriod::RestartAccessIfNeeded (void)
{
  NS_LOG_FUNCTION (this);
  if ((m_currentPacket != 0
       || !m_queue->IsEmpty ()
       || m_baManager->HasPackets ())
       && m_accessAllowed)
    {
      NotifyAccessGranted ();
    }
}

void
ServicePeriod::StartAccessIfNeeded (void)
{
  NS_LOG_FUNCTION (this);
  if (m_currentPacket == 0
      && (!m_queue->IsEmpty () || m_baManager->HasPackets ())
      && m_accessAllowed)
    {
      NotifyAccessGranted ();
    }
}

void
ServicePeriod::StartAccess (void)
{
  NS_LOG_FUNCTION (this);
  if (!m_queue->IsEmpty () || m_baManager->HasPackets ())
    {
      NotifyAccessGranted ();
    }
}

void
ServicePeriod::InitiateTransmission (Mac48Address peerStation, Time servicePeriodDuration)
{
  NS_LOG_FUNCTION (this << peerStation << servicePeriodDuration);
  m_peerStation = peerStation;
  m_servicePeriodDuration = servicePeriodDuration;
  m_transmissionStarted = Simulator::Now ();
  m_accessAllowed = true;
  StartAccess ();
}

void
ServicePeriod::PushFront (Ptr<const Packet> packet, const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << packet << &hdr);
  m_stationManager->PrepareForQueue (hdr.GetAddr1 (), &hdr, packet);
  m_queue->PushFront (packet, hdr);
  StartAccessIfNeeded ();
}

void
ServicePeriod::Queue (Ptr<const Packet> packet, const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << packet << &hdr << hdr.GetAddr1 ());
  m_stationManager->PrepareForQueue (hdr.GetAddr1 (), &hdr, packet);
  m_queue->Enqueue (packet, hdr);
  StartAccessIfNeeded ();
}

bool
ServicePeriod::NeedRtsRetransmission (void)
{
  NS_LOG_FUNCTION (this);
  return m_stationManager->NeedRtsRetransmission (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                                  m_currentPacket);
}

bool
ServicePeriod::NeedDataRetransmission (void)
{
  NS_LOG_FUNCTION (this);
  return m_stationManager->NeedDataRetransmission (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                                   m_currentPacket);
}

bool
ServicePeriod::NeedBarRetransmission (void)
{
  uint8_t tid = 0;
  uint16_t seqNumber = 0;
  if (m_currentHdr.IsQosData ())
    {
      tid = m_currentHdr.GetQosTid ();
      seqNumber = m_currentHdr.GetSequenceNumber ();
    }
  else if (m_currentHdr.IsBlockAckReq ())
    {
      CtrlBAckRequestHeader baReqHdr;
      m_currentPacket->PeekHeader (baReqHdr);
      tid = baReqHdr.GetTidInfo ();
      seqNumber = baReqHdr.GetStartingSequence ();
    }
  else if (m_currentHdr.IsBlockAck ())
    {
      CtrlBAckResponseHeader baRespHdr;
      m_currentPacket->PeekHeader (baRespHdr);
      tid = baRespHdr.GetTidInfo ();
      seqNumber = m_currentHdr.GetSequenceNumber ();
    }
  return m_baManager->NeedBarRetransmission (tid, seqNumber, m_currentHdr.GetAddr1 ());
}

void
ServicePeriod::NextFragment (void)
{
  NS_LOG_FUNCTION (this);
  m_fragmentNumber++;
}

void
ServicePeriod::StartNext (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("start next packet fragment");
  /* Update SP duration */
  m_servicePeriodDuration = m_servicePeriodDuration - (Simulator::Now () - m_transmissionStarted);
  if (m_servicePeriodDuration <= Seconds (0))
    return;

  /* this callback is used only for fragments. */
  NextFragment ();
  WifiMacHeader hdr;
  Ptr<Packet> fragment = GetFragmentPacket (&hdr);
  MacLowTransmissionParameters params;
  params.EnableAck ();
  params.DisableRts ();
  params.SetAsBoundedTransmission ();
  params.SetMaximumTransmissionDuration (m_remainingDuration);
  params.EnableOverrideDurationId (m_remainingDuration);
  params.SetTransmitInSericePeriod ();
  if (IsLastFragment ())
    {
      params.DisableNextData ();
    }
  else
    {
      params.EnableNextData (GetNextFragmentSize ());
    }
  Low ()->StartTransmission (fragment, &hdr, params, m_transmissionListener);
}

void
ServicePeriod::Cancel (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("transmission cancelled");
}

void
ServicePeriod::EndTxNoAck (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("a transmission that did not require an ACK just finished");
  m_currentPacket = 0;
  StartAccessIfNeeded ();
}

bool
ServicePeriod::NeedFragmentation (void) const
{
  NS_LOG_FUNCTION (this);
  return m_stationManager->NeedFragmentation (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                              m_currentPacket);
}

uint32_t
ServicePeriod::GetFragmentSize (void)
{
  NS_LOG_FUNCTION (this);
  return m_stationManager->GetFragmentSize (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                            m_currentPacket, m_fragmentNumber);
}

uint32_t
ServicePeriod::GetNextFragmentSize (void)
{
  NS_LOG_FUNCTION (this);
  return m_stationManager->GetFragmentSize (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                            m_currentPacket, m_fragmentNumber + 1);
}

uint32_t
ServicePeriod::GetFragmentOffset (void)
{
  NS_LOG_FUNCTION (this);
  return m_stationManager->GetFragmentOffset (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                              m_currentPacket, m_fragmentNumber);
}

bool
ServicePeriod::IsLastFragment (void) const
{
  NS_LOG_FUNCTION (this);
  return m_stationManager->IsLastFragment (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                           m_currentPacket, m_fragmentNumber);
}

Ptr<Packet>
ServicePeriod::GetFragmentPacket (WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << hdr);
  *hdr = m_currentHdr;
  hdr->SetFragmentNumber (m_fragmentNumber);
  uint32_t startOffset = GetFragmentOffset ();
  Ptr<Packet> fragment;
  if (IsLastFragment ())
    {
      hdr->SetNoMoreFragments ();
    }
  else
    {
      hdr->SetMoreFragments ();
    }
  fragment = m_currentPacket->CreateFragment (startOffset,
                                              GetFragmentSize ());
  return fragment;
}

Mac48Address
ServicePeriod::MapSrcAddressForAggregation (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << &hdr);
  Mac48Address retval;
  if (m_typeOfStation == DMG_STA)
    {
      retval = hdr.GetAddr2 ();
    }
  else
    {
      retval = hdr.GetAddr3 ();
    }
  return retval;
}

Mac48Address
ServicePeriod::MapDestAddressForAggregation (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << &hdr);
  return hdr.GetAddr1 ();
}

void
ServicePeriod::SetMsduAggregator (Ptr<MsduAggregator> aggr)
{
  NS_LOG_FUNCTION (this << aggr);
  m_msduAggregator = aggr;
}

void
ServicePeriod::SetMpduAggregator (Ptr<MpduAggregator> aggr)
{
  NS_LOG_FUNCTION (this << aggr);
  m_mpduAggregator = aggr;
}

void
ServicePeriod::GotAddBaResponse (const MgtAddBaResponseHeader *respHdr, Mac48Address recipient)
{
  NS_LOG_FUNCTION (this << respHdr << recipient);
  NS_LOG_DEBUG ("received ADDBA response from " << recipient);
  uint8_t tid = respHdr->GetTid ();
  if (m_baManager->ExistsAgreementInState (recipient, tid, OriginatorBlockAckAgreement::PENDING))
    {
      if (respHdr->GetStatusCode ().IsSuccess ())
        {
          NS_LOG_DEBUG ("block ack agreement established with " << recipient);
          m_baManager->UpdateAgreement (respHdr, recipient);
        }
      else
        {
          NS_LOG_DEBUG ("discard ADDBA response" << recipient);
          m_baManager->NotifyAgreementUnsuccessful (recipient, tid);
        }
    }
  RestartAccessIfNeeded ();
}

void
ServicePeriod::GotDelBaFrame (const MgtDelBaHeader *delBaHdr, Mac48Address recipient)
{
  NS_LOG_FUNCTION (this << delBaHdr << recipient);
  NS_LOG_DEBUG ("received DELBA frame from=" << recipient);
  m_baManager->TearDownBlockAck (recipient, delBaHdr->GetTid ());
}

void
ServicePeriod::GotBlockAck (const CtrlBAckResponseHeader *blockAck, Mac48Address recipient, double rxSnr, WifiMode txMode, double dataSnr)
{
  NS_LOG_FUNCTION (this << blockAck << recipient << rxSnr << txMode.GetUniqueName () << dataSnr);
  NS_LOG_DEBUG ("got block ack from=" << recipient);
  m_baManager->NotifyGotBlockAck (blockAck, recipient, rxSnr, txMode, dataSnr);
  if (!m_txOkCallback.IsNull ())
    {
      m_txOkCallback (m_currentPacket, m_currentHdr);
    }
  m_currentPacket = 0;
  RestartAccessIfNeeded ();
}

void
ServicePeriod::VerifyBlockAck (void)
{
  NS_LOG_FUNCTION (this);
  uint8_t tid = m_currentHdr.GetQosTid ();
  Mac48Address recipient = m_currentHdr.GetAddr1 ();
  uint16_t sequence = m_currentHdr.GetSequenceNumber ();
  if (m_baManager->ExistsAgreementInState (recipient, tid, OriginatorBlockAckAgreement::INACTIVE))
    {
      m_baManager->SwitchToBlockAckIfNeeded (recipient, tid, sequence);
    }
  if ((m_baManager->ExistsAgreementInState (recipient, tid, OriginatorBlockAckAgreement::ESTABLISHED)) && (GetMpduAggregator () == 0))
    {
      m_currentHdr.SetQosAckPolicy (WifiMacHeader::BLOCK_ACK);
    }
}

bool
ServicePeriod::GetAmpduExist (Mac48Address dest)
{
  NS_LOG_FUNCTION (this << dest);
  if (m_aMpduEnabled.find (dest) != m_aMpduEnabled.end ())
    {
      return m_aMpduEnabled.find (dest)->second;
    }
  return false;
}

void
ServicePeriod::SetAmpduExist (Mac48Address dest, bool enableAmpdu)
{
  NS_LOG_FUNCTION (this << dest << enableAmpdu);
  if (m_aMpduEnabled.find (dest) != m_aMpduEnabled.end () && m_aMpduEnabled.find (dest)->second != enableAmpdu)
    {
      m_aMpduEnabled.erase (m_aMpduEnabled.find (dest));
    }
  if (m_aMpduEnabled.find (dest) == m_aMpduEnabled.end ())
    {
      m_aMpduEnabled.insert (std::make_pair (dest, enableAmpdu));
    }
}

void
ServicePeriod::CompleteTx (void)
{
  NS_LOG_FUNCTION (this);
  if (m_currentHdr.IsQosData () && m_currentHdr.IsQosBlockAck ())
    {
      if (!m_currentHdr.IsRetry ())
        {
          m_baManager->StorePacket (m_currentPacket, m_currentHdr, m_currentPacketTimestamp);
        }
      m_baManager->NotifyMpduTransmission (m_currentHdr.GetAddr1 (), m_currentHdr.GetQosTid (),
                                           m_txMiddle->GetNextSeqNumberByTidAndAddress (m_currentHdr.GetQosTid (),
                                                                                        m_currentHdr.GetAddr1 ()), WifiMacHeader::BLOCK_ACK);
    }
}

void
ServicePeriod::CompleteMpduTx (Ptr<const Packet> packet, WifiMacHeader hdr, Time tstamp)
{
  NS_ASSERT (hdr.IsQosData ());
  m_baManager->StorePacket (packet, hdr, tstamp);
  m_baManager->NotifyMpduTransmission (hdr.GetAddr1 (), hdr.GetQosTid (),
                                       m_txMiddle->GetNextSeqNumberByTidAndAddress (hdr.GetQosTid (),
                                                                                    hdr.GetAddr1 ()), WifiMacHeader::NORMAL_ACK);
}

bool
ServicePeriod::SetupBlockAckIfNeeded ()
{
  NS_LOG_FUNCTION (this);
  uint8_t tid = m_currentHdr.GetQosTid ();
  Mac48Address recipient = m_currentHdr.GetAddr1 ();

  uint32_t packets = m_queue->GetNPacketsByTidAndAddress (tid, WifiMacHeader::ADDR1, recipient);

  if ((m_blockAckThreshold > 0 && packets >= m_blockAckThreshold) || (packets > 1 && m_mpduAggregator != 0))
    {
      /* Block ack setup */
      uint16_t startingSequence = m_txMiddle->GetNextSeqNumberByTidAndAddress (tid, recipient);
      SendAddBaRequest (recipient, tid, startingSequence, m_blockAckInactivityTimeout, true);
      return true;
    }
  return false;
}

void
ServicePeriod::SendBlockAckRequest (const struct Bar &bar)
{
  NS_LOG_FUNCTION (this << &bar);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_CTL_BACKREQ);
  hdr.SetAddr1 (bar.recipient);
  hdr.SetAddr2 (m_low->GetAddress ());
  hdr.SetAddr3 (m_low->GetBssid ());
  hdr.SetDsNotTo ();
  hdr.SetDsNotFrom ();
  hdr.SetNoRetry ();
  hdr.SetNoMoreFragments ();

  m_currentPacket = bar.bar;
  m_currentHdr = hdr;

  MacLowTransmissionParameters params;
  params.DisableRts ();
  params.DisableNextData ();
  params.SetAsBoundedTransmission ();
  params.SetMaximumTransmissionDuration (m_remainingDuration);
  params.EnableOverrideDurationId (m_remainingDuration);
  params.SetTransmitInSericePeriod ();
  if (bar.immediate)
    {
      if (m_blockAckType == BASIC_BLOCK_ACK)
        {
          params.EnableBasicBlockAck ();
        }
      else if (m_blockAckType == COMPRESSED_BLOCK_ACK)
        {
          params.EnableCompressedBlockAck ();
        }
      else if (m_blockAckType == MULTI_TID_BLOCK_ACK)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported");
        }
    }
  else
    {
      //Delayed block ack
      params.EnableAck ();
    }
  m_low->StartTransmission (m_currentPacket, &m_currentHdr, params, m_transmissionListener);
}

void
ServicePeriod::CompleteConfig (void)
{
  NS_LOG_FUNCTION (this);
  m_baManager->SetTxMiddle (m_txMiddle);
  m_low->RegisterBlockAckListenerForAc (AC_BE, m_blockAckListener);
  m_baManager->SetBlockAckInactivityCallback (MakeCallback (&ServicePeriod::SendDelbaFrame, this));
}

void
ServicePeriod::SetBlockAckThreshold (uint8_t threshold)
{
  NS_LOG_FUNCTION (this << static_cast<uint32_t> (threshold));
  m_blockAckThreshold = threshold;
  m_baManager->SetBlockAckThreshold (threshold);
}

void
ServicePeriod::SetBlockAckInactivityTimeout (uint16_t timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  m_blockAckInactivityTimeout = timeout;
}

uint8_t
ServicePeriod::GetBlockAckThreshold (void) const
{
  NS_LOG_FUNCTION (this);
  return m_blockAckThreshold;
}

void
ServicePeriod::SendAddBaRequest (Mac48Address dest, uint8_t tid, uint16_t startSeq,
                                 uint16_t timeout, bool immediateBAck)
{
  NS_LOG_FUNCTION (this << dest << static_cast<uint32_t> (tid) << startSeq << timeout << immediateBAck);
  NS_LOG_DEBUG ("sent ADDBA request to " << dest);
  WifiMacHeader hdr;
  hdr.SetAction ();
  hdr.SetAddr1 (dest);
  hdr.SetAddr2 (m_low->GetAddress ());
  hdr.SetAddr3 (m_low->GetAddress ());
  hdr.SetDsNotTo ();
  hdr.SetDsNotFrom ();

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.blockAck = WifiActionHeader::BLOCK_ACK_ADDBA_REQUEST;
  actionHdr.SetAction (WifiActionHeader::BLOCK_ACK, action);

  Ptr<Packet> packet = Create<Packet> ();
  /*Setting ADDBARequest header*/
  MgtAddBaRequestHeader reqHdr;
  reqHdr.SetAmsduSupport (true);
  if (immediateBAck)
    {
      reqHdr.SetImmediateBlockAck ();
    }
  else
    {
      reqHdr.SetDelayedBlockAck ();
    }
  reqHdr.SetTid (tid);
  /* For now we don't use buffer size field in the ADDBA request frame. The recipient
   * will choose how many packets it can receive under block ack.
   */
  reqHdr.SetBufferSize (0);
  reqHdr.SetTimeout (timeout);
  reqHdr.SetStartingSequence (startSeq);

  m_baManager->CreateAgreement (&reqHdr, dest);

  packet->AddHeader (reqHdr);
  packet->AddHeader (actionHdr);

  m_currentPacket = packet;
  m_currentHdr = hdr;

  uint16_t sequence = m_txMiddle->GetNextSequenceNumberfor (&m_currentHdr);
  m_currentHdr.SetSequenceNumber (sequence);
  m_stationManager->UpdateFragmentationThreshold ();
  m_currentHdr.SetFragmentNumber (0);
  m_currentHdr.SetNoMoreFragments ();
  m_currentHdr.SetNoRetry ();

  MacLowTransmissionParameters params;
  params.EnableAck ();
  params.DisableRts ();
  params.DisableNextData ();
  params.DisableOverrideDurationId ();
  params.SetAsBoundedTransmission ();
  params.SetMaximumTransmissionDuration (m_remainingDuration);
  params.SetTransmitInSericePeriod ();

  m_low->StartTransmission (m_currentPacket, &m_currentHdr, params,
                            m_transmissionListener);
}

void
ServicePeriod::SendAddBaResponse (const MgtAddBaRequestHeader *reqHdr,
                                  Mac48Address originator)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
  hdr.SetAction ();
  hdr.SetAddr1 (originator);
  hdr.SetAddr2 (m_low->GetAddress ());
  hdr.SetAddr3 (m_low->GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();

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

  m_currentPacket = packet;
  m_currentHdr = hdr;

  uint16_t sequence = m_txMiddle->GetNextSequenceNumberfor (&m_currentHdr);
  m_currentHdr.SetSequenceNumber (sequence);
  m_stationManager->UpdateFragmentationThreshold ();
  m_currentHdr.SetFragmentNumber (0);
  m_currentHdr.SetNoMoreFragments ();
  m_currentHdr.SetNoRetry ();

  MacLowTransmissionParameters params;
  params.EnableAck ();
  params.DisableRts ();
  params.DisableNextData ();
  params.DisableOverrideDurationId ();
  params.SetAsBoundedTransmission ();
  params.SetMaximumTransmissionDuration (m_remainingDuration);
  params.SetTransmitInSericePeriod ();

  m_low->StartTransmission (m_currentPacket, &m_currentHdr, params,
                            m_transmissionListener);
}

void
ServicePeriod::SendDelbaFrame (Mac48Address addr, uint8_t tid, bool byOriginator)
{
  NS_LOG_FUNCTION (this << addr << static_cast<uint32_t> (tid) << byOriginator);
  WifiMacHeader hdr;
  hdr.SetAction ();
  hdr.SetAddr1 (addr);
  hdr.SetAddr2 (m_low->GetAddress ());
  hdr.SetAddr3 (m_low->GetAddress ());
  hdr.SetDsNotTo ();
  hdr.SetDsNotFrom ();

  MgtDelBaHeader delbaHdr;
  delbaHdr.SetTid (tid);
  if (byOriginator)
    {
      delbaHdr.SetByOriginator ();
    }
  else
    {
      delbaHdr.SetByRecipient ();
    }

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.blockAck = WifiActionHeader::BLOCK_ACK_DELBA;
  actionHdr.SetAction (WifiActionHeader::BLOCK_ACK, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (delbaHdr);
  packet->AddHeader (actionHdr);

  PushFront (packet, hdr);
}

void
ServicePeriod::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
}

void
ServicePeriod::BaTxOk (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << hdr);
  if (!m_txOkCallback.IsNull ())
    {
      m_txOkCallback (m_currentPacket, m_currentHdr);
    }
}

void
ServicePeriod::BaTxFailed (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << hdr);
  if (!m_txFailedCallback.IsNull ())
    {
      m_txFailedCallback (m_currentHdr);
    }
}

} //namespace ns3
