/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#ifndef DMG_SLS_TXOP_H
#define DMG_SLS_TXOP_H

#include <stdint.h>
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-mode.h"
#include "ext-headers.h"
#include "txop.h"

#include <deque>

namespace ns3 {

enum SLS_ROLE {
  SLS_IDLE,
  SLS_INITIATOR,
  SLS_RESPONDER,
};

typedef std::deque<Mac48Address> SLS_REQUESTS_DEQUE;
typedef SLS_REQUESTS_DEQUE::iterator SLS_REQUESTS_DEQUE_I;
typedef SLS_REQUESTS_DEQUE::const_iterator SLS_REQUESTS_DEQUE_CI;

class DmgSlsTxop : public Txop
{
public:
  static TypeId GetTypeId (void);

  DmgSlsTxop ();
  ~DmgSlsTxop ();

  /**
   * Append a new SLS Reqest.
   * \param peerAddress The MAC address of the responder station.
   */
  void Append_SLS_Reqest (Mac48Address peerAddress);
  /**
   * Start channel access procedure to obtain transmit opprtunity (TXOP) to perform beamforming
   * training in the DTI access period.
   * \param peerAddress The MAC address of the responder station.
   */
  void Initiate_TXOP_Sector_Sweep (Mac48Address peerAddress);
  /**
   * Start responder sector sweep phase.
   * \param peerAddress The MAC address of the initiator station.
   */
  void Start_Responder_Sector_Sweep (Mac48Address peerAddress);
  /**
   * Start initiator sector sweep feedback.
   * \param peerAddress The MAC address of the responder station.
   */
  void Start_Initiator_Feedback (Mac48Address peerAddress);
  /**
   * Initialize SLS related variables.
   */
  void InitializeVariables (void);
  /**
   * Resume any pending TXSS TXOP or SSW-FBCK TXOP.
   */
  void ResumeTXSS (void);
  /**
   * Sector sweep phase failed (Initiator did not receive any RXSS-SSW frame).
   */
  void Sector_Sweep_Phase_Failed (void);
  /**
   * Failed to receive SSW-ACK frame from the initiator.
   */
  void RxSSW_ACK_Failed (void);
  /**
   * This function is called when TXSS SLS beamforming training has successfully completed.
   */
  void SLS_BFT_Completed (void);
  /**
   * This function is called when TXSS SLS beamforming training has failed (Exceeded dot11BFRetryLimit limit).
   */
  void SLS_BFT_Failed (bool retryAccess = true);
  /**
   * Check if the current station is performing SLS TXSS in CBAP.
   * \return true if the station is performing SLS in CBAP, false
   *         otherwise
   */
  bool ServingSLS (void) const;
  /**
   * Transmit single  packet.
   * \param packet the packet we want to transmit.
   * \param hdr header of the packet to send.
   * \param duration The duration in Microseconds.
   */
  void TransmitFrame (Ptr<const Packet> packet, const WifiMacHeader &hdr, Time duration);
  /**
   * typedef for a callback to invoke when access to the channel is granted to start SLS TxOP.
   */
  typedef Callback <void, Mac48Address, SLS_ROLE, bool> AccessGranted;
  /**
   * Access is granted to the channel
   * \param callback the callback to invoke when access is granted
   */
  void SetAccessGrantedCallback (AccessGranted callback);
  /**
   * Whether we should resume an interrupted beamforming training (BFT) due to CBAP ending or
   * just start a new beamforming training at the beginning of the following beacon interval.
   * \return If set to True, we resume the beamforming training from it stopped.
   *         Otherwise, we restart BFT at the beginning of the next BI.
   */
  bool ResumeCbapBeamforming (void) const;

private:
  DmgSlsTxop &operator = (const DmgSlsTxop &);
  DmgSlsTxop (const DmgSlsTxop &o);

  //Inherited from ns3::Object
  void DoInitialize ();
  /* Txop notifications forwarded here */
  /**
   * Notify the Txop that access has been granted.
   */
  void NotifyAccessGranted (void);
  /**
   * Notify the Txop that internal collision has occurred.
   */
  void NotifyInternalCollision (void);
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

  virtual void DoDispose (void);

private:
  AccessGranted m_accessGrantedCallback;
  SLS_REQUESTS_DEQUE m_slsRequetsDeque;             //!< Deque for SLS Beamforming Requests.
  bool m_servingSLS;                                //!< Flag to indicate if we are in SLS phase.
  SLS_ROLE m_slsRole;                               //!< The current SLS role.
  bool m_isFeedback;                                //!< Flag to indicate whether the initiator is in SSW-FBCK state.
  bool m_resumeCbapBeamforming;

};

} //namespace ns3

#endif /* DMG_SLS_TXOP_H */
