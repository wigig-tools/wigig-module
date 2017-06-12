/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#ifndef DMG_ATI_DCA_H
#define DMG_ATI_DCA_H

#include "dca-txop.h"
#include "mac-low.h"
#include "wifi-mac-header.h"
#include "wifi-remote-station-manager.h"

namespace ns3 {

class DmgAtiDca : public DcaTxop
{
public:
  static TypeId GetTypeId (void);

  DmgAtiDca ();
  ~DmgAtiDca ();
  /**
   * \param packet packet to send
   * \param hdr header of packet to send.
   *
   * Store the packet in the internal queue until it
   * can be sent safely.
   */
  void Queue (Ptr<const Packet> packet, const WifiMacHeader &hdr);
  /**
   * Initiate ATI access Period. This function is called by DMG STA.
   * \param atiDuration The duration of the ATI access period in microseconds.
   */
  void InitiateAtiAccessPeriod (Time atiDuration);
  /**
   * Initiate Transmission in this ATI access period. This function is called by DMG PCP/AP.
   * \param atiDuration The duration of the ATI access period in microseconds.
   */
  void InitiateTransmission (Time atiDuration);

private:
  DmgAtiDca &operator = (const DmgAtiDca &);
  DmgAtiDca (const DmgAtiDca &o);

  //Inherited from ns3::Object
  void DoInitialize ();

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

  /* Event handlers */
  /**
   * Event handler when an ACK is received.
   *
   * \param snr
   * \param txMode
   */
  void GotAck (double snr, WifiMode txMode);
  /**
   * Event handler when an ACK is missed.
   */
  void MissedAck (void);
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
   * Request access from DCF manager if needed.
   */
  void StartAccessIfNeeded (void);
  /**
   * Disable transmission.
   */
  void DisableTransmission (void);
  /**
   * Check if DATA should be re-transmitted if ACK was missed.
   *
   * \return true if DATA should be re-transmitted,
   *         false otherwise
   */
  bool NeedDataRetransmission (void);

  virtual void DoDispose (void);

  Ptr<const Packet> m_currentPacket;
  WifiMacHeader m_currentHdr;

  Time m_transmissionStarted;   /* The time of the initiation of transmission */
  Time m_atiDuration;           /* The duration of the ATI*/
  Time m_remainingDuration;     /* The remaining duration till the end of this CBAP allocation*/
  bool m_allowTransmission;
};

} //namespace ns3

#endif /* DMG_ATI_DCA_H */
