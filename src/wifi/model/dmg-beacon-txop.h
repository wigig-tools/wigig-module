/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#ifndef DMG_BEACON_TXOP_H
#define DMG_BEACON_TXOP_H

#include <stdint.h>
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ext-headers.h"
#include "txop.h"

namespace ns3 {

class DmgBeaconTxop : public Txop
{
public:
  static TypeId GetTypeId (void);

  DmgBeaconTxop ();
  ~DmgBeaconTxop ();

  /**
   * Transmit single DMG Beacon..
   * \param beacon The DMG Beacon Body.
   * \param hdr header of packet to send.
   * \param BTIRemainingTime The remaining time in BTI access period.
   */
  void TransmitDmgBeacon (const ExtDMGBeacon &beacon, const WifiMacHeader &hdr, Time BTIRemainingTime);
  /**
   * Perform Clear Channel Assessment Procedure.
   */
  void PerformCCA (void);
  /**
   * typedef for a callback to invoke when a
   * packet transmission was completed successfully.
   */
  typedef Callback <void> AccessGranted;
  /**
   * CCA procedure is completed and access is granted.
   * \param callback the callback to invoke when access is granted
   */
  void SetAccessGrantedCallback (AccessGranted callback);

private:
  DmgBeaconTxop &operator = (const DmgBeaconTxop &);
  DmgBeaconTxop (const DmgBeaconTxop &o);

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

};

} //namespace ns3

#endif /* DMG_BEACON_TXOP_H */
