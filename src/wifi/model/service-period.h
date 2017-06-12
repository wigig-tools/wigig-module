/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015,2016 IMDEA Networks
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#ifndef SERVICE_PERIOD_H
#define SERVICE_PERIOD_H

#include "ns3/traced-value.h"
#include "block-ack-manager.h"
#include "dca-txop.h"
#include <map>
#include <list>

class AmpduAggregationTest;

namespace ns3 {

class WifiMac;
class WifiMacParameters;
class QosBlockedDestinations;
class MsduAggregator;
class MpduAggregator;
class MgtAddBaResponseHeader;
class MgtDelBaHeader;
class AggregationCapableTransmissionListener;

class ServicePeriod : public DcaTxop
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
  virtual ~ServicePeriod ();  /**
   * Set type of station with the given type.
   *
   * \param type the type of station.
   */
  void SetTypeOfStation (TypeOfStation type);
  /**
   * Return type of station.
   *
   * \return type of station.
   */
  TypeOfStation GetTypeOfStation (void) const;

  /**
   * Returns the aggregator used to construct A-MSDU subframes.
   *
   * \return the aggregator used to construct A-MSDU subframes.
   */
  Ptr<MsduAggregator> GetMsduAggregator (void) const;
  /**
   * Returns the aggregator used to construct A-MPDU subframes.
   *
   * \return the aggregator used to construct A-MPDU subframes.
   */
  Ptr<MpduAggregator> GetMpduAggregator (void) const;

  /**
   * \param address recipient address of the peer station
   * \param tid traffic ID.
   *
   * \return true if a block ack agreement exists, false otherwise.
   *
   * Checks if a block ack agreement exists with station addressed by
   * <i>recipient</i> for tid <i>tid</i>.
   */
  bool GetBaAgreementExists (Mac48Address address, uint8_t tid) const;
  /**
   * \param address recipient address of peer station involved in block ack mechanism.
   * \param tid traffic ID.
   *
   * \return the number of packets buffered for a specified agreement.
   *
   * Returns number of packets buffered for a specified agreement.
   */
  uint32_t GetNOutstandingPacketsInBa (Mac48Address address, uint8_t tid) const;
  /**
   * \param recipient address of peer station involved in block ack mechanism.
   * \param tid traffic ID.
   *
   * \return the number of packets for a specific agreement that need retransmission.
   *
   * Returns number of packets for a specific agreement that need retransmission.
   */
  uint32_t GetNRetryNeededPackets (Mac48Address recipient, uint8_t tid) const;
  /**
   * \param recipient address of peer station involved in block ack mechanism.
   * \param tid Ttraffic ID of transmitted packet.
   *
   * This function resets the status of OriginatorBlockAckAgreement after the transfer
   * of an A-MPDU with ImmediateBlockAck policy (i.e. no BAR is scheduled).
   */
  void CompleteAmpduTransfer (Mac48Address recipient, uint8_t tid);
  /**
   * Copy BlockAck Agreements
   * \param recipient
   * \param target
   */
  void CopyBlockAckAgreements (Mac48Address recipient, Ptr<EdcaTxopN> target);

  /* dcf notifications forwarded here */
  /**
   * Check if the EDCAF requires access.
   *
   * \return true if the EDCAF requires access,
   *         false otherwise.
   */
  bool NeedsAccess (void) const;
  /**
   * Notify the EDCAF that access has been granted.
   */
  void NotifyAccessGranted (void);

  /* Event handlers */
  /**
   * Event handler when a CTS timeout has occurred.
   */
  void MissedCts (void);
  /**
   * Event handler when an ACK is received.
   */
  void GotAck (void);
  /**
   * Event handler when a Block ACK is received.
   *
   * \param blockAck block ack.
   * \param recipient address of the recipient.
   * \param rxSnr SNR of the block ack itself.
   * \param txMode wifi mode.
   * \param dataSnr reported data SNR from the peer.
   */
  void GotBlockAck (const CtrlBAckResponseHeader *blockAck, Mac48Address recipient, double rxSnr, WifiMode txMode, double dataSnr);
  /**
   * Event handler when a Block ACK timeout has occurred.
   */
  void MissedBlockAck (uint8_t nMpdus);
  /**
   * Event handler when an ADDBA response is received.
   *
   * \param respHdr ADDBA response header.
   * \param recipient address of the recipient.
   */
  void GotAddBaResponse (const MgtAddBaResponseHeader *respHdr, Mac48Address recipient);
  /**
   * Event handler when a DELBA frame is received.
   *
   * \param delBaHdr DELBA header.
   * \param recipient address of the recipient.
   */
  void GotDelBaFrame (const MgtDelBaHeader *delBaHdr, Mac48Address recipient);
  /**
   * Event handler when an ACK is missed.
   */
  void MissedAck (void);

  /**
   * Start transmission for the next packet if allowed by the TxopLimit.
   */
  void StartNextPacket (void);
  /**
   * Event handler when a transmission that does not require an ACK has completed.
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
   * Get the remaining duration in the current allocation.
   * \return The remaining time in the current allocation.
   */
  Time GetRemainingDuration (void) const;
  /**
   * Change all the packets receive address (Addr1).
   * \param oldAddress The old receiver MAC Address.
   * \param newAddress The new receiver MAC address.
   */
  void ChangePacketsAddress (Mac48Address relayAddress, Mac48Address destAddress);
  /**
   * ٍStart new service period.
   * \param allocationID The ID of the current allocation.
   * \param peerStation The address of the peer station.
   * \param servicePeriodDuration The duration of this service period in microseconds.
   */
  void StartServicePeriod (AllocationID allocationID, Mac48Address peerStation, Time servicePeriodDuration);
  /**
   * Initiate Transmission in this service period.
   */
  void InitiateTransmission (void);
  /**
   * Resume Transmission in this service period.
   * \param servicePeriodDuration The duration of this service period in microseconds.
   */
  void ResumeTransmission (Time servicePeriodDuration);
  /**
   * Check if Block ACK Request should be re-transmitted.
   *
   * \return true if BAR should be re-transmitted,
   *         false otherwise.
   */
  bool NeedBarRetransmission (void);

  /**
   * Check if the current packet should be fragmented.
   *
   * \return true if the current packet should be fragmented,
   *         false otherwise.
   */
  bool NeedFragmentation (void) const;

  /**
   * Get the next fragment from the packet with
   * appropriate Wifi header for the fragment.
   *
   * \param hdr Wi-Fi header.
   *
   * \return the fragment with the current fragment number.
   */
  Ptr<Packet> GetFragmentPacket (WifiMacHeader *hdr);
  /**
   * \param packet packet to send
   * \param hdr header of packet to send.
   *
   * Store the packet in the internal queue until it
   * can be sent safely.
   */
  void Queue (Ptr<const Packet> packet, WifiMacHeader &hdr);
  /**
   * Set the aggregator used to construct A-MSDU subframes.
   *
   * \param aggr pointer to the MSDU aggregator.
   */
  void SetMsduAggregator (Ptr<MsduAggregator> aggr);
  /**
   * Set the aggregator used to construct A-MPDU subframes.
   *
   * \param aggr pointer to the MPDU aggregator.
   */
  void SetMpduAggregator (Ptr<MpduAggregator> aggr);

  /**
   * \param packet packet to send.
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
   * \param threshold block ack threshold value.
   */
  void SetBlockAckThreshold (uint8_t threshold);
  /**
   * Return the current threshold for block ACK mechanism.
   *
   * \return the current threshold for block ACK mechanism.
   */
  uint8_t GetBlockAckThreshold (void) const;

  /**
   * Set the Block Ack inactivity timeout.
   *
   * \param timeout the Block Ack inactivity timeout.
   */
  void SetBlockAckInactivityTimeout (uint16_t timeout);
  /**
   * Sends DELBA frame to cancel a block ack agreement with sta
   * addressed by <i>addr</i> for tid <i>tid</i>.
   *
   * \param addr address of the recipient.
   * \param tid traffic ID.
   * \param byOriginator flag to indicate whether this is set by the originator.
   */
  void SendDelbaFrame (Mac48Address addr, uint8_t tid, bool byOriginator);
  /**
   * Stores an MPDU (part of an A-MPDU) in blockackagreement (i.e. the sender is waiting
   * for a blockack containing the sequence number of this MPDU).
   * It also calls NotifyMpdu transmission that updates the status of OriginatorBlockAckAgreement.
   *
   * \param packet received packet.
   * \param hdr received Wi-Fi header.
   * \param tstamp timestamp.
   */
  void CompleteMpduTx (Ptr<const Packet> packet, WifiMacHeader hdr, Time tstamp);
  /**
   * Return whether A-MPDU is used to transmit data to a peer station.
   *
   * \param dest address of peer station
   * \returns true if A-MPDU is used by the peer station
   */
  bool GetAmpduExist (Mac48Address dest) const;
  /**
   * Set indication whether A-MPDU is used to transmit data to a peer station.
   *
   * \param dest address of peer station.
   * \param enableAmpdu flag whether A-MPDU is used or not.
   */
  void SetAmpduExist (Mac48Address dest, bool enableAmpdu);

  /**
   * Return the next sequence number for the given header.
   *
   * \param hdr Wi-Fi header.
   *
   * \return the next sequence number.
   */
  uint16_t GetNextSequenceNumberFor (WifiMacHeader *hdr);
  /**
   * Return the next sequence number for the Traffic ID and destination, but do not pick it (i.e. the current sequence number remains unchanged).
   *
   * \param hdr Wi-Fi header.
   *
   * \return the next sequence number.
   */
  uint16_t PeekNextSequenceNumberFor (WifiMacHeader *hdr);
  /**
   * Remove a packet after you peek in the retransmit queue and get it.
   *
   * \param tid traffic ID of the packet to be removed.
   * \param recipient address of the recipient the packet was intended for.
   * \param seqnumber sequence number of the packet to be removed.
   */
  void RemoveRetransmitPacket (uint8_t tid, Mac48Address recipient, uint16_t seqnumber);
  /**
   * Peek in retransmit queue and get the next packet without removing it from the queue.
   *
   * \param header Wi-Fi header.
   * \param recipient address of the recipient.
   * \param tid traffic ID.
   * \param timestamp the timestamp.
   * \returns the packet.
   */
  Ptr<const Packet> PeekNextRetransmitPacket (WifiMacHeader &header, Mac48Address recipient, uint8_t tid, Time *timestamp);
  /**
   * The packet we sent was successfully received by the receiver.
   *
   * \param hdr the header of the packet that we successfully sent.
   */
  void BaTxOk (const WifiMacHeader &hdr);
  /**
   * The packet we sent was successfully received by the receiver.
   *
   * \param hdr the header of the packet that we failed to sent.
   */
  void BaTxFailed (const WifiMacHeader &hdr);

  /**
   * TracedCallback signature for Access Granted events.
   *
   * \param address the mac address of the granted station
   * \param queueSize the size of the Wifi Queue
   */
  typedef void (* AccessGrantedCallback)(Mac48Address address, uint32_t queueSize);

  void AllowChannelAccess (void);
  void DisableChannelAccess (void);
  void EndCurrentServicePeriod (void);
  /**
   * This functions are used only to correctly set source address in A-MSDU subframes.
   * If aggregating sta is a STA (in an infrastructured network):
   *   SA = Address2
   * If aggregating sta is an AP
   *   SA = Address3
   *
   * \param hdr Wi-Fi header
   * \return Mac48Address
   */
  Mac48Address MapSrcAddressForAggregation (const WifiMacHeader &hdr);
  /**
   * This functions are used only to correctly set destination address in A-MSDU subframes.
   * If aggregating sta is a STA (in an infrastructured network):
   *   DA = Address3
   * If aggregating sta is an AP
   *   DA = Address1
   *
   * \param hdr Wi-Fi header
   * \return Mac48Address
   */
  Mac48Address MapDestAddressForAggregation (const WifiMacHeader &hdr);
  /**
   * \param callback the callback to invoke when an ACK/BlockAck
   * is missed from the peer side
   */
  void SetMissedAckCallback (TxFailed callback);

private:
  friend class AggregationCapableTransmissionListener;

  /**
   * If number of packets in the queue reaches m_blockAckThreshold value, an ADDBA Request frame
   * is sent to destination in order to setup a block ack.
   *
   * \return true if we tried to set up block ACK, false otherwise.
   */
  bool SetupBlockAckIfNeeded ();
  /**
   * Sends an ADDBA Request to establish a block ack agreement with sta
   * addressed by <i>recipient</i> for tid <i>tid</i>.
   *
   * \param recipient address of the recipient.
   * \param tid traffic ID.
   * \param startSeq starting sequence.
   * \param timeout timeout value.
   * \param immediateBAck flag to indicate whether immediate block ack is used.
   */
  void SendAddBaRequest (Mac48Address recipient, uint8_t tid, uint16_t startSeq,
                         uint16_t timeout, bool immediateBAck);
  /**
   * After that all packets, for which a block ack agreement was established, have been
   * transmitted, we have to send a block ack request.
   *
   * \param bar the block ack request.
   */
  void SendBlockAckRequest (const Bar &bar);
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

  /**
   * Calculate the size of the next fragment.
   *
   * \return the size of the next fragment.
   */
  uint32_t GetNextFragmentSize (void) const;
  /**
   * Calculate the size of the current fragment.
   *
   * \return the size of the current fragment.
   */
  uint32_t GetFragmentSize (void) const;
  /**
   * Calculate the offset for the current fragment.
   *
   * \return the offset for the current fragment.
   */
  uint32_t GetFragmentOffset (void) const;
  /**
   * Check if the current fragment is the last fragment.
   *
   * \return true if the current fragment is the last fragment,
   *         false otherwise.
   */
  bool IsLastFragment (void) const;

  void DoDispose (void);
  void DoInitialize (void);

  Ptr<MsduAggregator> m_msduAggregator;             //!< A-MSDU aggregator
  Ptr<MpduAggregator> m_mpduAggregator;             //!< A-MPDU aggregator
  TypeOfStation m_typeOfStation;                    //!< the type of station
  QosBlockedDestinations *m_qosBlockedDestinations; //!< QOS blocked destinations
  BlockAckManager *m_baManager;                     //!< the Block ACK manager
  uint8_t m_blockAckThreshold;                      //!< the Block ACK threshold
  BlockAckType m_blockAckType;                      //!< the Block ACK type
  Time m_currentPacketTimestamp;                    //!< the current packet timestamp
  uint16_t m_blockAckInactivityTimeout;             //!< the Block ACK inactivity timeout
  Bar m_currentBar;                                 //!< the current BAR
  bool m_isAccessRequestedForRts;                   //!< flag whether access is requested to transmit a RTS frame

  /* Store packet and header for service period */
  typedef std::pair<Ptr<const Packet>, WifiMacHeader> PacketInformation;
  typedef std::map<AllocationID, PacketInformation> StoredPackets;
  typedef StoredPackets::const_iterator StoredPacketsCI;
  StoredPackets m_storedPackets;

  bool m_accessOngoing;
  bool m_accessAllowed;
  AllocationID m_allocationID;      /* Allocation ID for the current service period */
  Mac48Address m_peerStation;       /* The address of the peer station (Destination DMG STA or Destination REDS). */
  Time m_remainingDuration;         /* The remaining duration till the end of this CBAP allocation*/
  Time m_servicePeriodDuration;     /* The total duration of the service period. */
  Time m_transmissionStarted;       /* The time of the initiation of transmission */
  TxFailed m_missedAck;             /* Missed Ack/BlockAck from the peer station */

  TracedCallback<Mac48Address, uint32_t> m_accessGrantedTrace;

};

} //namespace ns3

#endif /* SERVICE_PERIOD_H */
