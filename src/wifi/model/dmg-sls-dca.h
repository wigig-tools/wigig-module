/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#ifndef DMG_SLS_DCA_H
#define DMG_SLS_DCA_H

#include <stdint.h>
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-mode.h"
#include "ext-headers.h"
#include "dca-txop.h"

#include <queue>

namespace ns3 {

typedef std::queue<Mac48Address> SLS_REQUESTS_QUEUE;

class DmgSlsDca : public DcaTxop
{
public:
  static TypeId GetTypeId (void);

  DmgSlsDca ();
  ~DmgSlsDca ();

  /**
   * Start channel access procedure to obtain TxOP to perform beamforming training.
   * \param peerAddress The MAC address of the responder station.
   */
  void ObtainTxOP (Mac48Address peerAddress, bool feedback);
  /**
   * Resume any pending TxSS TXOP or SSW-FBCk TXOP.
   */
  void ResumeTxSS (void);
  /**
   * Transmit single SSW packet.
   * \param packet the SSW frame body.
   * \param hdr header of the packet to send.
   * \param duration The duration in the SSW Field.
   */
  void TransmitSswFrame (Ptr<const Packet> packet, const WifiMacHeader &hdr, Time duration);
  /**
   * typedef for a callback to invoke when access to the channel is granted to start SLS TxOP.
   */
  typedef Callback <void, Mac48Address, bool> AccessGranted;
  /**
   * Access is granted to the channel
   * \param callback the callback to invoke when access is granted
   */
  void SetAccessGrantedCallback (AccessGranted callback);

private:
  DmgSlsDca &operator = (const DmgSlsDca &);
  DmgSlsDca (const DmgSlsDca &o);

  //Inherited from ns3::Object
  void DoInitialize ();
  /* dcf notifications forwarded here */
  /**
   * Notify the DCF that access has been granted.
   */
  void NotifyAccessGranted (void);
  /**
   * Notify the DCF that internal collision has occurred.
   */
  void NotifyInternalCollision (void);
  /**
   * Notify the DCF that collision has occurred.
   */
  void NotifyCollision (void);
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
  SLS_REQUESTS_QUEUE m_slsRequetsQueue;             //!< Queue for SLS Requests.
  bool m_transmitFeedback;
  Mac48Address m_feedBackAddress;

};

} //namespace ns3

#endif /* DMG_SLS_DCA_H */
