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
#ifndef SERVICE_PERIOD_H
#define SERVICE_PERIOD_H

#include "ns3/object.h"
#include "ns3/mac48-address.h"
#include "ns3/packet.h"

#include "block-ack-manager.h"
#include "ctrl-headers.h"
#include "qos-utils.h"
#include "wifi-mode.h"
#include "wifi-mac-header.h"
#include "wifi-remote-station-manager.h"

#include <map>
#include <list>

class AmpduAggregationTest;

namespace ns3 {

class MacLow;
class MacTxMiddle;
class WifiMac;
class WifiMacParameters;
class WifiMacQueue;
class QosBlockedDestinations;
class MsduAggregator;
class MpduAggregator;
class MgtAddBaResponseHeader;
class BlockAckManager;
class MgtDelBaHeader;

class ServicePeriod : public Object
{
public:
  // Allow test cases to access private members
  friend class ::AmpduAggregationTest;

  /**
   * typedef for a callback to invoke when a
   * packet transmission was completed successfully.
   */
  typedef Callback <void, const WifiMacHeader&> TxOk;
  /**
   * typedef for a callback to invoke when a
   * packet transmission was completed successfully.
   */
  typedef Callback <void, Ptr<const Packet>, const WifiMacHeader&> TxPacketOk;
  /**
   * typedef for a callback to invoke when a
   * packet transmission was failed.
   */
  typedef Callback <void, const WifiMacHeader&> TxFailed;
  
  std::map<Mac48Address, bool> m_aMpduEnabled;

  static TypeId GetTypeId (void);
  ServicePeriod ();
  virtual ~ServicePeriod ();
  void DoDispose ();

  /**
   * Set MacLow associated with this EdcaTxopN.
   *
   * \param low MacLow
   */
  void SetLow (Ptr<MacLow> low);
  void SetTxMiddle (MacTxMiddle *txMiddle);
  /**
   * \param callback the callback to invoke when a
   * packet transmission was completed successfully.
   */
  void SetTxOkCallback (TxPacketOk callback);
  /**
   * \param callback the callback to invoke when a
   * packet transmission was completed unsuccessfully.
   */
  void SetTxFailedCallback (TxFailed callback);
  /**
   * Set WifiRemoteStationsManager this EdcaTxopN is associated to.
   *
   * \param remoteManager WifiRemoteStationManager
   */
  void SetWifiRemoteStationManager (Ptr<WifiRemoteStationManager> remoteManager);
  /**
   * Set type of station with the given type.
   *
   * \param type
   */
  void SetTypeOfStation (enum TypeOfStation type);
  /**
   * Return type of station.
   *
   * \return type of station
   */
  enum TypeOfStation GetTypeOfStation (void) const;
  /**
   * Return the packet queue associated with this EdcaTxopN.
   *
   * \return WifiMacQueue
   */
  Ptr<WifiMacQueue > GetQueue () const;

  /**
   * Return the MacLow associated with this EdcaTxopN.
   *
   * \return MacLow
   */
  Ptr<MacLow> Low (void);

  Ptr<MsduAggregator> GetMsduAggregator (void) const;
  Ptr<MpduAggregator> GetMpduAggregator (void) const;

  /**
   * \param recipient address of the peer station
   * \param tid traffic ID.
   * \return true if a block ack agreement exists, false otherwise
   *
   * Checks if a block ack agreement exists with station addressed by
   * <i>recipient</i> for tid <i>tid</i>.
   */
  bool GetBaAgreementExists (Mac48Address address, uint8_t tid);
  /**
   * \param recipient address of peer station involved in block ack mechanism.
   * \param tid traffic ID.
   * \return the number of packets buffered for a specified agreement
   *
   * Returns number of packets buffered for a specified agreement.
   */
  uint32_t GetNOutstandingPacketsInBa (Mac48Address address, uint8_t tid);
  /**
   * \param recipient address of peer station involved in block ack mechanism.
   * \param tid traffic ID.
   * \return the number of packets for a specific agreement that need retransmission
   *
   * Returns number of packets for a specific agreement that need retransmission.
   */
  uint32_t GetNRetryNeededPackets (Mac48Address recipient, uint8_t tid) const;
  /**
   * \param recipient address of peer station involved in block ack mechanism.
   * \param tid Ttraffic ID of transmitted packet.
   *
   * This function resets the status of OriginatorBlockAckAgreement after the transfer
   * of an A-MPDU with ImmediateBlockAck policy (i.e. no BAR is scheduled)
   */
  void CompleteAmpduTransfer (Mac48Address recipient, uint8_t tid);

  /**
   * Check if the EDCAF requires access.
   *
   * \return true if the EDCAF requires access,
   *         false otherwise
   */
  bool NeedsAccess (void) const;
  /**
   * Notify the EDCAF that access has been granted.
   */
  void NotifyAccessGranted (void);

  /**
   * When a channel switching occurs, enqueued packets are removed.
   */
  void NotifyChannelSwitching (void);
  /**
   * When sleep operation occurs, re-insert pending packet into front of the queue
   */
  void NotifySleep (void);
  /**
   * When wake up operation occurs, restart channel access
   */
  void NotifyWakeUp (void);

  /* Event handlers */
  /**
   * Event handler when a CTS is received.
   *
   * \param snr
   * \param txMode
   */
  void GotCts (double snr, WifiMode txMode);
  /**
   * Event handler when a CTS timeout has occurred.
   */
  void MissedCts (void);
  /**
   * Event handler when an ACK is received.
   *
   * \param snr
   * \param txMode
   */
  void GotAck (double snr, WifiMode txMode);
  /**
   * Event handler when a Block ACK is received.
   *
   * \param blockAck
   * \param recipient
   * \param rxSnr SNR of the block ack itself
   * \param txMode
   * \param dataSnr reported data SNR from the peer
   */
  void GotBlockAck (const CtrlBAckResponseHeader *blockAck, Mac48Address recipient, double rxSnr, WifiMode txMode, double dataSnr);
  /**
   * Event handler when a Block ACK timeout has occurred.
   */
  void MissedBlockAck (uint32_t nMpdus);
  void GotAddBaResponse (const MgtAddBaResponseHeader *respHdr, Mac48Address recipient);
  void GotDelBaFrame (const MgtDelBaHeader *delBaHdr, Mac48Address recipient);
  /**
   * Event handler when an ACK is missed.
   */
  void MissedAck (void);
  /**
   * Start transmission for the next fragment.
   * This is called for fragment only.
   */
  void StartNext (void);
  /**
   * Cancel the transmission.
   */
  void Cancel (void);
  /**
   * Event handler when a transmission that
   * does not require an ACK has completed.
   */
  void EndTxNoAck (void);
  /**
   * Restart access request if needed.
   */
  void RestartAccessIfNeeded (void);
  /**
   * Request access.
   */
  void StartAccessIfNeeded (void);
  /**
   * Start access.
   */
  void StartAccess (void);
  /**
   * Initiate Transmission in this service period.
   * \param peerStation The address of the peer station.
   * \param servicePeriodDuration The duration of this service period in microseconds.
   */
  void InitiateTransmission (Mac48Address peerStation, Time servicePeriodDuration);
  /**
   * SendAddBaResponse
   * \param reqHdr
   * \param originator
   */
  void SendAddBaResponse (const MgtAddBaRequestHeader *reqHdr, Mac48Address originator);
  /**
   * Check if RTS should be re-transmitted if CTS was missed.
   *
   * \return true if RTS should be re-transmitted,
   *         false otherwise
   */
  bool NeedRtsRetransmission (void);
  /**
   * Check if DATA should be re-transmitted if ACK was missed.
   *
   * \return true if DATA should be re-transmitted,
   *         false otherwise
   */
  bool NeedDataRetransmission (void);
  /**
   * Check if Block ACK Request should be re-transmitted.
   *
   * \return true if BAR should be re-transmitted,
   *         false otherwise
   */
  bool NeedBarRetransmission (void);
  /**
   * Check if the current packet should be fragmented.
   *
   * \return true if the current packet should be fragmented,
   *         false otherwise
   */
  bool NeedFragmentation (void) const;
  /**
   * Calculate the size of the next fragment.
   *
   * \return the size of the next fragment
   */
  uint32_t GetNextFragmentSize (void);
  /**
   * Calculate the size of the current fragment.
   *
   * \return the size of the current fragment
   */
  uint32_t GetFragmentSize (void);
  /**
   * Calculate the offset for the current fragment.
   *
   * \return the offset for the current fragment
   */
  uint32_t GetFragmentOffset (void);
  /**
   * Check if the current fragment is the last fragment.
   *
   * \return true if the current fragment is the last fragment,
   *         false otherwise
   */
  bool IsLastFragment (void) const;
  /**
   * Continue to the next fragment. This method simply
   * increments the internal variable that keep track
   * of the current fragment number.
   */
  void NextFragment (void);
  /**
   * Get the next fragment from the packet with
   * appropriate Wifi header for the fragment.
   *
   * \param hdr
   * \return the fragment with the current fragment number
   */
  Ptr<Packet> GetFragmentPacket (WifiMacHeader *hdr);

  /**
   * \param packet packet to send
   * \param hdr header of packet to send.
   *
   * Store the packet in the internal queue until it
   * can be sent safely.
   */
  void Queue (Ptr<const Packet> packet, const WifiMacHeader &hdr);

  void SetMsduAggregator (Ptr<MsduAggregator> aggr);
  void SetMpduAggregator (Ptr<MpduAggregator> aggr);

  /**
   * \param packet packet to send
   * \param hdr header of packet to send.
   *
   * Store the packet in the front of the internal queue until it
   * can be sent safely.
   */
  void PushFront (Ptr<const Packet> packet, const WifiMacHeader &hdr);

  /**
   * Complete block ACK configuration.
   */
  void CompleteConfig (void);

  /**
   * Set threshold for block ACK mechanism. If number of packets in the
   * queue reaches the threshold, block ACK mechanism is used.
   *
   * \param threshold
   */
  void SetBlockAckThreshold (uint8_t threshold);
  /**
   * Return the current threshold for block ACK mechanism.
   *
   * \return the current threshold for block ACK mechanism
   */
  uint8_t GetBlockAckThreshold (void) const;

  void SetBlockAckInactivityTimeout (uint16_t timeout);
  void SendDelbaFrame (Mac48Address addr, uint8_t tid, bool byOriginator);
  void CompleteMpduTx (Ptr<const Packet> packet, WifiMacHeader hdr, Time tstamp);
  bool GetAmpduExist (Mac48Address dest);
  void SetAmpduExist (Mac48Address dest, bool enableAmpdu);

  /**
   * Return the next sequence number for the given header.
   *
   * \param hdr Wi-Fi header
   *
   * \return the next sequence number
   */
  uint16_t GetNextSequenceNumberfor (WifiMacHeader *hdr);
  /**
   * Return the next sequence number for the Traffic ID and destination, but do not pick it (i.e. the current sequence number remains unchanged).
   *
   * \param hdr Wi-Fi header
   *
   * \return the next sequence number
   */
  uint16_t PeekNextSequenceNumberfor (WifiMacHeader *hdr);
  /**
   * Remove a packet after you peek in the retransmit queue and get it
   */
  void RemoveRetransmitPacket (uint8_t tid, Mac48Address recipient, uint16_t seqnumber);
  /*
   * Peek in retransmit queue and get the next packet without removing it from the queue
   */
  Ptr<const Packet> PeekNextRetransmitPacket (WifiMacHeader &header, Mac48Address recipient, uint8_t tid, Time *timestamp);
  /**
   * The packet we sent was successfully received by the receiver
   *
   * \param hdr the header of the packet that we successfully sent
   */
  void BaTxOk (const WifiMacHeader &hdr);
  /**
   * The packet we sent was successfully received by the receiver
   *
   * \param hdr the header of the packet that we failed to sent
   */
  void BaTxFailed (const WifiMacHeader &hdr);

  /**
   * TracedCallback signature for Access Granted events.
   *
   * \param address the mac address of the granted station
   * \param queueSize the size of the Wifi Queue
   */
  typedef void (* AccessGrantedCallback)(Mac48Address address, uint32_t queueSize);

  void AllowChannelAccess ();
  void DisableChannelAccess ();

private:

  void DoInitialize ();
  /**
   * This functions are used only to correctly set addresses in a-msdu subframe.
   * If aggregating sta is a STA (in an infrastructured network):
   *   SA = Address2
   *   DA = Address3
   * If aggregating sta is an AP
   *   SA = Address3
   *   DA = Address1
   *
   * \param hdr
   * \return Mac48Address
   */
  Mac48Address MapSrcAddressForAggregation (const WifiMacHeader &hdr);
  Mac48Address MapDestAddressForAggregation (const WifiMacHeader &hdr);
  ServicePeriod &operator = (const ServicePeriod &);
  ServicePeriod (const ServicePeriod &);

  /**
   * If number of packets in the queue reaches m_blockAckThreshold value, an ADDBA Request frame
   * is sent to destination in order to setup a block ack.
   *
   * \return true if we tried to set up block ACK, false otherwise
   */
  bool SetupBlockAckIfNeeded ();
  /**
   * Sends an ADDBA Request to establish a block ack agreement with sta
   * addressed by <i>recipient</i> for tid <i>tid</i>.
   *
   * \param recipient
   * \param tid
   * \param startSeq
   * \param timeout
   * \param immediateBAck
   */
  void SendAddBaRequest (Mac48Address recipient, uint8_t tid, uint16_t startSeq,
                         uint16_t timeout, bool immediateBAck);
  /**
   * After that all packets, for which a block ack agreement was established, have been
   * transmitted, we have to send a block ack request.
   *
   * \param bar
   */
  void SendBlockAckRequest (const struct Bar &bar);
  /**
   * For now is typically invoked to complete transmission of a packets sent with ack policy
   * Block Ack: the packet is buffered and dcf is reset.
   */
  void CompleteTx (void);
  /**
   * Verifies if dequeued packet has to be transmitted with ack policy Block Ack. This happens
   * if an established block ack agreement exists with the receiver.
   */
  void VerifyBlockAck (void);

  class TransmissionListener;
  class AggregationCapableTransmissionListener;
  friend class TransmissionListener;

  Ptr<WifiMacQueue> m_queue;
  TxPacketOk m_txOkCallback;
  TxFailed m_txFailedCallback;
  Ptr<MacLow> m_low;
  MacTxMiddle *m_txMiddle;
  TransmissionListener *m_transmissionListener;
  AggregationCapableTransmissionListener *m_blockAckListener;
  Ptr<WifiRemoteStationManager> m_stationManager;
  uint8_t m_fragmentNumber;

  /* current packet could be a simple MSDU or, if an aggregator for this queue is
     present, could be an A-MSDU.
   */
  Ptr<const Packet> m_currentPacket;

  WifiMacHeader m_currentHdr;
  Ptr<MsduAggregator> m_msduAggregator;
  Ptr<MpduAggregator> m_mpduAggregator;
  TypeOfStation m_typeOfStation;
  QosBlockedDestinations *m_qosBlockedDestinations;
  BlockAckManager *m_baManager;
  /*
   * Represents the minimum number of packets for use of block ack.
   */
  uint8_t m_blockAckThreshold;
  enum BlockAckType m_blockAckType;
  Time m_currentPacketTimestamp;
  uint16_t m_blockAckInactivityTimeout;
  struct Bar m_currentBar;
  Ptr<WifiMac> m_wifiMac;

  bool m_accessOngoing;
  bool m_accessAllowed;
  
  Mac48Address m_peerStation;       /* The address of the peer station. */
  Time m_remainingDuration;         /* The remaining duration till the end of this CBAP allocation*/
  Time m_servicePeriodDuration;     /* The total duration of the service period. */
  Time m_transmissionStarted;       /* The time of the initiation of transmission */

  TracedCallback<Mac48Address, uint32_t> m_accessGrantedTrace;

};

} //namespace ns3

#endif /* SERVICE_PERIOD_H */
