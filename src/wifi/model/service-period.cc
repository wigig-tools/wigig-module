/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 IMDEA Networks
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

NS_OBJECT_ENSURE_REGISTERED (ServicePeriod);

TypeId
ServicePeriod::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ServicePeriod")
    .SetParent<ns3::DcaTxop> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ServicePeriod> ()
  ;
  return tid;
}

ServicePeriod::ServicePeriod ()
  : m_msduAggregator (0),
    m_mpduAggregator (0),
    m_typeOfStation (DMG_STA),
    m_blockAckType (COMPRESSED_BLOCK_ACK)
{
  NS_LOG_FUNCTION (this);
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
  delete m_qosBlockedDestinations;
  delete m_baManager;
  m_qosBlockedDestinations = 0;
  m_baManager = 0;
  m_msduAggregator = 0;
  m_mpduAggregator = 0;
  DcaTxop::DoDispose ();
}

bool
ServicePeriod::GetBaAgreementExists (Mac48Address address, uint8_t tid) const
{
  return m_baManager->ExistsAgreement (address, tid);
}

uint32_t
ServicePeriod::GetNOutstandingPacketsInBa (Mac48Address address, uint8_t tid) const
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
ServicePeriod::SetTypeOfStation (enum TypeOfStation type)
{
  NS_LOG_FUNCTION (this << static_cast<uint32_t> (type));
  m_typeOfStation = type;
}

TypeOfStation
ServicePeriod::GetTypeOfStation (void) const
{
  NS_LOG_FUNCTION (this);
  return m_typeOfStation;
}

bool
ServicePeriod::NeedsAccess (void) const
{
  NS_LOG_FUNCTION (this);
  return !m_queue->IsEmpty () || m_currentPacket != 0 || m_baManager->HasPackets ();
}

uint16_t
ServicePeriod::GetNextSequenceNumberFor (WifiMacHeader *hdr)
{
  return m_txMiddle->GetNextSequenceNumberFor (hdr);
}

uint16_t
ServicePeriod::PeekNextSequenceNumberFor (WifiMacHeader *hdr)
{
  return m_txMiddle->GetNextSequenceNumberFor (hdr);
}

Ptr<const Packet>
ServicePeriod::PeekNextRetransmitPacket (WifiMacHeader &header, Mac48Address recipient, uint8_t tid, Time *timestamp)
{
  return m_baManager->PeekNextPacketByTidAndAddress (header, recipient, tid, timestamp);
}

void
ServicePeriod::RemoveRetransmitPacket (uint8_t tid, Mac48Address recipient, uint16_t seqnumber)
{
  m_baManager->RemovePacket (tid, recipient, seqnumber);
}

Time
ServicePeriod::GetRemainingDuration (void) const
{
  return m_servicePeriodDuration - (Simulator::Now () - m_transmissionStarted);
}

void
ServicePeriod::NotifyAccessGranted (void)
{
  NS_LOG_FUNCTION (this);
  /* Update remaining SP duration */
  m_remainingDuration = GetRemainingDuration ();
  if (m_remainingDuration <= Seconds (0))
    {
      m_accessAllowed = false;
      return;
    }

  if (!m_low->RestoredSuspendedTransmission ())
    {
      m_low->ResumeTransmission (m_remainingDuration, this);
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
          Ptr<const WifiMacQueueItem> item = m_queue->PeekFirstAvailableByAddress (WifiMacHeader::ADDR1,
                                                                                   m_peerStation, m_qosBlockedDestinations);
          if (item == 0)
            {
              NS_LOG_DEBUG ("no available packets in the queue");
              return;
            }
          m_currentHdr = item->GetHeader ();
          m_currentPacketTimestamp = item->GetTimeStamp ();
          if (m_currentHdr.IsQosData () && !m_currentHdr.GetAddr1 ().IsBroadcast ()
              && m_stationManager->GetQosSupported (m_currentHdr.GetAddr1 ())
              && !m_baManager->ExistsAgreement (m_currentHdr.GetAddr1 (), m_currentHdr.GetQosTid ())
              && SetupBlockAckIfNeeded ())
            {
              return;
            }

          item = m_queue->DequeueByAddress (WifiMacHeader::ADDR1, m_peerStation, m_qosBlockedDestinations);
          m_currentPacket = item->GetPacket ();
          m_currentHdr = item->GetHeader ();
          m_currentPacketTimestamp = item->GetTimeStamp ();
          NS_ASSERT (m_currentPacket != 0);

          if (m_currentPacket != 0)
            {
              uint16_t sequence = m_txMiddle->GetNextSequenceNumberFor (&m_currentHdr);
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

//  std::cout << "MaxQueueSize=" << m_queue->GetMaxPackets () << std::endl;
//  std::cout << "CurrentQueueSize=" << m_queue->GetNPackets () << std::endl;

  if (m_currentPacket != 0)
    {
      m_currentParams.SetAsBoundedTransmission ();
      m_currentParams.SetMaximumTransmissionDuration (m_remainingDuration);
//      params.EnableOverrideDurationId (m_remainingDuration);
      m_currentParams.DisableOverrideDurationId ();
      m_currentParams.SetTransmitInSericePeriod ();
      if (m_currentHdr.GetType () == WIFI_MAC_CTL_BACKREQ)
        {
          SendBlockAckRequest (m_currentBar);
        }
      else
        {
          if (m_currentHdr.IsQosData () && m_currentHdr.IsQosBlockAck ())
            {
              m_currentParams.DisableAck ();
            }
          else
            {
              m_currentParams.EnableAck ();
            }
          if (((m_currentHdr.IsQosData () && !m_currentHdr.IsQosAmsdu ())
              || (m_currentHdr.IsData () && !m_currentHdr.IsQosData () && m_currentHdr.IsQosAmsdu ()))
              && (m_blockAckThreshold == 0 || m_blockAckType == BASIC_BLOCK_ACK)
              && NeedFragmentation ())
            {
              //With COMPRESSED_BLOCK_ACK fragmentation must be avoided.
              m_currentParams.DisableRts ();
              WifiMacHeader hdr;
              Ptr<Packet> fragment = GetFragmentPacket (&hdr);
              if (IsLastFragment ())
                {
                  NS_LOG_DEBUG ("fragmenting last fragment size=" << fragment->GetSize ());
                  m_currentParams.DisableNextData ();
                }
              else
                {
                  NS_LOG_DEBUG ("fragmenting size=" << fragment->GetSize ());
                  m_currentParams.EnableNextData (GetNextFragmentSize ());
                }
              m_low->StartTransmission (fragment, &hdr, m_currentParams, this);
            }
          else
            {
              WifiMacHeader peekedHdr;
              Ptr<const WifiMacQueueItem> item;
              if (m_currentHdr.IsQosData ()
                  && (item = m_queue->PeekByTidAndAddress (m_currentHdr.GetQosTid (),
                                                           WifiMacHeader::ADDR1, m_currentHdr.GetAddr1 ()))
                  && !m_currentHdr.GetAddr1 ().IsBroadcast ()
                  && m_msduAggregator != 0 && !m_currentHdr.IsRetry ())
                {
                  peekedHdr = item->GetHeader ();
                  /* here is performed aggregation */
                  Ptr<Packet> currentAggregatedPacket = Create<Packet> ();
                  m_msduAggregator->Aggregate (m_currentPacket, currentAggregatedPacket,
                                               MapSrcAddressForAggregation (peekedHdr),
                                               MapDestAddressForAggregation (peekedHdr));
                  bool aggregated = false;
                  bool isAmsdu = false;
                  Ptr<const WifiMacQueueItem> peekedItem = m_queue->PeekByTidAndAddress (m_currentHdr.GetQosTid (),
                                                                                         WifiMacHeader::ADDR1,
                                                                                         m_currentHdr.GetAddr1 ());
                  while (peekedItem != 0)
                    {
                      peekedHdr = peekedItem->GetHeader ();
                      aggregated = m_msduAggregator->Aggregate (peekedItem->GetPacket (), currentAggregatedPacket,
                                                                MapSrcAddressForAggregation (peekedHdr),
                                                                MapDestAddressForAggregation (peekedHdr));
                      if (aggregated)
                        {
                          isAmsdu = true;
                          m_queue->Remove (peekedItem->GetPacket ());
                        }
                      else
                        {
                          break;
                        }
                      peekedItem = m_queue->PeekByTidAndAddress (m_currentHdr.GetQosTid (),
                                                                 WifiMacHeader::ADDR1, m_currentHdr.GetAddr1 ());
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
              m_currentParams.DisableNextData ();

              /* Check if more MSDUs are buffered for transmission */
              if (m_queue->HasPacketsForReceiver (m_peerStation))
                {
                  m_currentHdr.SetMoreData ();
                }

              m_low->StartTransmission (m_currentPacket, &m_currentHdr, m_currentParams, this);
              if (!GetAmpduExist (m_currentHdr.GetAddr1 ()))
                {
                  CompleteTx ();
                }
            }
        }
    }
}

void
ServicePeriod::MissedCts (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("missed cts");
  if (!NeedRtsRetransmission (m_currentPacket, m_currentHdr))
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
          m_low->FlushAggregateQueue (AC_BE);
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
              reqHdr.SetStartingSequence (m_txMiddle->PeekNextSequenceNumberFor (&m_currentHdr));
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
ServicePeriod::GotAck (void)
{
  NS_LOG_FUNCTION (this);
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

  /* Callback if we missed Ack/BlockAck*/
  if (!m_missedAck.IsNull ())
    {
      m_missedAck (m_currentHdr);
    }

  if (!NeedDataRetransmission (m_currentPacket, m_currentHdr))
    {
      NS_LOG_DEBUG ("Ack Fail");
      m_stationManager->ReportFinalDataFailed (m_currentHdr.GetAddr1 (), &m_currentHdr);
      bool resetCurrentPacket = true;
      if (!m_txFailedCallback.IsNull ())
        {
          m_txFailedCallback (m_currentHdr);
        }
      if (GetAmpduExist (m_currentHdr.GetAddr1 ()) || m_currentHdr.IsQosData ())
        {
          uint8_t tid = GetTid (m_currentPacket, m_currentHdr);

          if (GetBaAgreementExists (m_currentHdr.GetAddr1 (), tid))
            {
              //send Block ACK Request in order to shift WinStart at the receiver
              NS_LOG_DEBUG ("Transmit Block Ack Request");
              CtrlBAckRequestHeader reqHdr;
              reqHdr.SetType (COMPRESSED_BLOCK_ACK);
              reqHdr.SetStartingSequence (m_txMiddle->PeekNextSequenceNumberFor (&m_currentHdr));
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
ServicePeriod::MissedBlockAck (uint8_t nMpdus)
{
  NS_LOG_FUNCTION (this << (uint16_t)nMpdus);
  uint8_t tid = GetTid (m_currentPacket, m_currentHdr);
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
       && m_accessAllowed
       && !m_low->IsTransmissionSuspended ())
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
      && m_accessAllowed
      && !m_low->IsTransmissionSuspended ())
    {
      NotifyAccessGranted ();
    }
}

void
ServicePeriod::ChangePacketsAddress (Mac48Address oldAddress, Mac48Address newAddress)
{
  NS_LOG_FUNCTION (this << oldAddress << newAddress);
  m_queue->ChangePacketsReceiverAddress (oldAddress, newAddress);
}

void
ServicePeriod::AllowChannelAccess (void)
{
  NS_LOG_FUNCTION (this);
  m_accessAllowed = true;
}

void
ServicePeriod::DisableChannelAccess (void)
{
  NS_LOG_FUNCTION (this);
  m_accessAllowed = false;
}

void
ServicePeriod::EndCurrentServicePeriod (void)
{
  NS_LOG_FUNCTION (this);
  /* Store parameters related to this service period which include MSDU/A-MSDU */
  if (m_currentPacket != 0)
    {
      m_storedPackets[m_allocationID] = std::make_pair (m_currentPacket, m_currentHdr);
      m_currentPacket = 0;
    }
  /* Inform MAC Low to store parameters related to this service period (MPDU/A-MPDU) */
  if (!m_low->IsTransmissionSuspended ())
    {
      m_low->StoreAllocationParameters ();
    }
}

void
ServicePeriod::StartServicePeriod (AllocationID allocationID, Mac48Address peerStation, Time servicePeriodDuration)
{
  NS_LOG_FUNCTION (this << allocationID << peerStation << servicePeriodDuration);
  m_allocationID = allocationID;
  m_peerStation = peerStation;
  m_servicePeriodDuration = servicePeriodDuration;
  m_transmissionStarted = Simulator::Now ();
}

void
ServicePeriod::InitiateTransmission (void)
{
  NS_LOG_FUNCTION (this << m_queue->IsEmpty ());
  m_accessAllowed = true;

  /* Restore previously suspended transmission */
  m_low->RestoreAllocationParameters (m_allocationID);

  /* Check if we have stored packet for this service period (MSDU/A-MSDU) */
  StoredPacketsCI it = m_storedPackets.find (m_allocationID);
  if (it != m_storedPackets.end ())
    {
      PacketInformation info = it->second;
      m_currentPacket = info.first;
      m_currentHdr = info.second;
    }

  /* Start access if we have packets in the queue or packets that need retransmit or non-restored transmission */
  if (!m_queue->IsEmpty () || m_baManager->HasPackets () || !m_low->RestoredSuspendedTransmission ())
    {
      NotifyAccessGranted ();
    }
}

void
ServicePeriod::ResumeTransmission (Time servicePeriodDuration)
{
  NS_LOG_FUNCTION (this << servicePeriodDuration);
  m_servicePeriodDuration = servicePeriodDuration;
  m_transmissionStarted = Simulator::Now ();
  m_accessAllowed = true;
  if (!m_queue->IsEmpty () || m_baManager->HasPackets ())
    {
      NotifyAccessGranted ();
    }
}

void
ServicePeriod::PushFront (Ptr<const Packet> packet, const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << packet << &hdr);
  m_stationManager->PrepareForQueue (hdr.GetAddr1 (), &hdr, packet);
  m_queue->PushFront (Create<WifiMacQueueItem> (packet, hdr));
  StartAccessIfNeeded ();
}

void
ServicePeriod::Queue (Ptr<const Packet> packet, WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << packet << &hdr << hdr.GetAddr1 ());
  m_stationManager->PrepareForQueue (hdr.GetAddr1 (), &hdr, packet);
  m_queue->Enqueue (Create<WifiMacQueueItem> (packet, hdr));
  StartAccessIfNeeded ();
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
ServicePeriod::StartNextPacket (void)
{
//  NS_LOG_FUNCTION (this);
//  WifiMacHeader hdr;
//  Time tstamp;

//  Ptr<const Packet> peekedPacket = m_queue->PeekByTidAndAddress (&hdr,
//                                                                 m_currentHdr.GetQosTid (),
//                                                                 WifiMacHeader::ADDR1,
//                                                                 m_currentHdr.GetAddr1 (),
//                                                                 &tstamp);
//  if (peekedPacket == 0)
//    {
//      return;
//    }

//  MacLowTransmissionParameters params;
//  params.DisableOverrideDurationId ();
//  if (m_currentHdr.IsQosData () && m_currentHdr.IsQosBlockAck ())
//    {
//      params.DisableAck ();
//    }
//  else
//    {
//      params.EnableAck ();
//    }

//  WifiTxVector dataTxVector = m_stationManager->GetDataTxVector (m_currentHdr.GetAddr1 (), &m_currentHdr, peekedPacket);
//  if (m_stationManager->NeedRts (m_currentHdr.GetAddr1 (), &m_currentHdr, peekedPacket, dataTxVector))
//    {
//      params.EnableRts ();
//    }
//  else
//    {
//      params.DisableRts ();
//    }

//  /* Update SP duration */
//  m_remainingDuration = GetRemainingDuration ();
//  if (m_remainingDuration <= Seconds (0))
//    {
//      m_accessAllowed = false;
//      return;
//    }
//  else
//    {
//      NS_LOG_DEBUG ("start next packet");
//      m_currentPacket = m_queue->DequeueByTidAndAddress (&hdr,
//                                                         m_currentHdr.GetQosTid (),
//                                                         WifiMacHeader::ADDR1,
//                                                         m_currentHdr.GetAddr1 ());
//      Low ()->StartTransmission (m_currentPacket, &m_currentHdr, params, m_transmissionListener);
//    }
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
  if (m_stationManager->HasVhtSupported ()
      || m_stationManager->HasHeSupported ()
      || GetAmpduExist (m_currentHdr.GetAddr1 ())
      || (m_stationManager->HasHtSupported ()
          && m_currentHdr.IsQosData ()
          && GetBaAgreementExists (m_currentHdr.GetAddr1 (), GetTid (m_currentPacket, m_currentHdr))
          && GetMpduAggregator ()->GetMaxAmpduSize () >= m_currentPacket->GetSize ()))
    {
      //MSDU is not fragmented when it is transmitted using an HT-immediate or
      //HT-delayed Block Ack agreement or when it is carried in an A-MPDU.
      return false;
    }
  bool needTxopFragmentation = false;
  if (GetTxopLimit () > NanoSeconds (0) && m_currentHdr.IsData ())
    {
      needTxopFragmentation = (GetLow ()->CalculateOverallTxTime (m_currentPacket, &m_currentHdr, m_currentParams) > GetTxopLimit ());
    }
  return (needTxopFragmentation || m_stationManager->NeedFragmentation (m_currentHdr.GetAddr1 (), &m_currentHdr, m_currentPacket));
}

uint32_t
ServicePeriod::GetFragmentSize (void) const
{
  NS_LOG_FUNCTION (this);
  return m_stationManager->GetFragmentSize (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                            m_currentPacket, m_fragmentNumber);

}

uint32_t
ServicePeriod::GetNextFragmentSize (void) const
{
  NS_LOG_FUNCTION (this);
  return m_stationManager->GetFragmentSize (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                            m_currentPacket, m_fragmentNumber + 1);
}

uint32_t
ServicePeriod::GetFragmentOffset (void) const
{
  NS_LOG_FUNCTION (this);
  return m_stationManager->GetFragmentOffset (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                              m_currentPacket, m_fragmentNumber);
}

bool
ServicePeriod::IsLastFragment (void) const
{
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
//  NS_LOG_FUNCTION (this << &hdr);
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
  if ((m_baManager->ExistsAgreementInState (recipient, tid, OriginatorBlockAckAgreement::ESTABLISHED))
      && (GetMpduAggregator () == 0 || GetMpduAggregator ()->GetMaxAmpduSize () == 0))
    {
      m_currentHdr.SetQosAckPolicy (WifiMacHeader::BLOCK_ACK);
    }
}

bool
ServicePeriod::GetAmpduExist (Mac48Address dest) const
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
  if ((m_blockAckThreshold > 0 && packets >= m_blockAckThreshold)
      || (m_mpduAggregator != 0 && m_mpduAggregator->GetMaxAmpduSize () > 0 && packets > 1)
      || m_stationManager->HasVhtSupported ()
      || m_stationManager->HasHeSupported ())
    {
      /* Block ack setup */
      uint16_t startingSequence = m_txMiddle->GetNextSeqNumberByTidAndAddress (tid, recipient);
      SendAddBaRequest (recipient, tid, startingSequence, m_blockAckInactivityTimeout, true);
      return true;
    }
  return false;
}

void
ServicePeriod::SendBlockAckRequest (const Bar &bar)
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
  hdr.SetNoOrder ();

  m_currentPacket = bar.bar;
  m_currentHdr = hdr;

  m_currentParams.DisableRts ();
  m_currentParams.DisableNextData ();
  m_currentParams.SetAsBoundedTransmission ();
  m_currentParams.SetMaximumTransmissionDuration (m_remainingDuration);
  m_currentParams.EnableOverrideDurationId (m_remainingDuration);
  m_currentParams.SetTransmitInSericePeriod ();
  if (bar.immediate)
    {
      if (m_blockAckType == BASIC_BLOCK_ACK)
        {
          m_currentParams.EnableBasicBlockAck ();
        }
      else if (m_blockAckType == COMPRESSED_BLOCK_ACK)
        {
          m_currentParams.EnableCompressedBlockAck ();
        }
      else if (m_blockAckType == MULTI_TID_BLOCK_ACK)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported");
        }
    }
  else
    {
      //Delayed block ack
      m_currentParams.EnableAck ();
    }
  m_low->StartTransmission (m_currentPacket, &m_currentHdr, m_currentParams, this);
}

void
ServicePeriod::SetMissedAckCallback (TxFailed callback)
{
  NS_LOG_FUNCTION (this << &callback);
  m_missedAck = callback;
}

void
ServicePeriod::CompleteConfig (void)
{
  NS_LOG_FUNCTION (this);
  m_baManager->SetTxMiddle (m_txMiddle);
//  m_low->RegisterEdcaForAc (AC_BE, m_this);
  m_baManager->SetBlockAckInactivityCallback (MakeCallback (&ServicePeriod::SendDelbaFrame, this));
}

void
ServicePeriod::SetBlockAckThreshold (uint8_t threshold)
{
  NS_LOG_FUNCTION (this << (uint16_t)threshold);
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
  NS_LOG_FUNCTION (this << dest << (uint16_t)tid << startSeq << timeout << immediateBAck);
  NS_LOG_DEBUG ("sent ADDBA request to " << dest);
  WifiMacHeader hdr;
  hdr.SetAction ();
  hdr.SetAddr1 (dest);
  hdr.SetAddr2 (m_low->GetAddress ());
  hdr.SetAddr3 (m_low->GetAddress ());
  hdr.SetDsNotTo ();
  hdr.SetDsNotFrom ();
  hdr.SetNoOrder ();

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

  uint16_t sequence = m_txMiddle->GetNextSequenceNumberFor (&m_currentHdr);
  m_currentHdr.SetSequenceNumber (sequence);
  m_stationManager->UpdateFragmentationThreshold ();
  m_currentHdr.SetFragmentNumber (0);
  m_currentHdr.SetNoMoreFragments ();
  m_currentHdr.SetNoRetry ();

  m_currentParams.EnableAck ();
  m_currentParams.DisableRts ();
  m_currentParams.DisableNextData ();
  m_currentParams.DisableOverrideDurationId ();
  if (m_stationManager->HasDmgSupported () && GetTypeOfStation () != DMG_ADHOC)
    {
      m_currentParams.SetAsBoundedTransmission ();
      m_currentParams.SetMaximumTransmissionDuration (m_remainingDuration);
    }
  m_low->StartTransmission (m_currentPacket, &m_currentHdr, m_currentParams, this);
}

void
ServicePeriod::SendDelbaFrame (Mac48Address addr, uint8_t tid, bool byOriginator)
{
  NS_LOG_FUNCTION (this << addr << (uint16_t)tid << byOriginator);
  WifiMacHeader hdr;
  hdr.SetAction ();
  hdr.SetAddr1 (addr);
  hdr.SetAddr2 (m_low->GetAddress ());
  hdr.SetAddr3 (m_low->GetAddress ());
  hdr.SetDsNotTo ();
  hdr.SetDsNotFrom ();
  hdr.SetNoOrder ();

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
