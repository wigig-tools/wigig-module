/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015,2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@hotmail.com>
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
   * Initiate new BTI access period. This enforcces DMG Beacon DCA Class to sense the channel before
   * the first DMG Beacon transmission.
   */
  void InitiateBTI (void);
  /**
   * \param beacon The DMG Beacon Body.
   * \param hdr header of packet to send.
   *
   * Start channel access procedure to transmit one DMG Beacon.
   */
  void TransmitDmgBeacon (const ExtDMGBeacon &beacon, const WifiMacHeader &hdr);
  /**
   * Set the upper layer MAC this DcaTxop is associated to.
   *
   * \return WifiMac
   */
  void SetWifiMac (Ptr<WifiMac> mac);

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

  Ptr<WifiMac> m_wifiMac;
  ExtDMGBeacon m_currentBeacon;
  WifiMacHeader m_currentHdr;
  bool m_transmittingBeacon;
  bool m_senseChannel;
};

} //namespace ns3

#endif /* DMG_BEACON_DCA_H */
