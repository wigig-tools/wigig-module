/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#ifndef DMG_BEACON_DCA_H
#define DMG_BEACON_DCA_H

#include <stdint.h>
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-mode.h"
#include "ext-headers.h"
#include "dca-txop.h"

namespace ns3 {

class DmgBeaconDca : public DcaTxop
{
public:
  static TypeId GetTypeId (void);

  DmgBeaconDca ();
  ~DmgBeaconDca ();

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
  DmgBeaconDca &operator = (const DmgBeaconDca &);
  DmgBeaconDca (const DmgBeaconDca &o);

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

};

} //namespace ns3

#endif /* DMG_BEACON_DCA_H */
