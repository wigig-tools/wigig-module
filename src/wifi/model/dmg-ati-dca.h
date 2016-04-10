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
 * Author: Hany Assasa <Hany.assasa@gmail.com>
 */
#ifndef DMG_ATI_DCA_H
#define DMG_ATI_DCA_H

#include <stdint.h>
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-mode.h"
#include "ns3/wifi-remote-station-manager.h"
#include "ns3/dcf.h"

namespace ns3 {

class DcfState;
class DcfManager;
class WifiMacQueue;
class MacLow;
class WifiMacParameters;
class MacTxMiddle;
class RandomStream;
class MacStation;
class MacStations;

class DmgAtiDca : public Dcf
{
public:
  static TypeId GetTypeId (void);

  /**
   * typedef for a callback to invoke when a
   * packet transmission was completed successfully.
   */
  typedef Callback <void, Ptr<const Packet>, const WifiMacHeader&> TxPacketOk;
  /**
   * typedef for a callback to invoke when a
   * packet transmission was completed successfully.
   */
  typedef Callback <void, const WifiMacHeader&> TxOk;
  /**
   * typedef for a callback to invoke when a
   * packet transmission was failed.
   */
  typedef Callback <void, const WifiMacHeader&> TxFailed;

  DmgAtiDca ();
  ~DmgAtiDca ();

  /**
   * Set MacLow associated with this DmgAtiDca.
   *
   * \param low MacLow
   */
  void SetLow (Ptr<MacLow> low);
  /**
   * Set DcfManager this DmgAtiDca is associated to.
   *
   * \param manager DcfManager
   */
  void SetManager (DcfManager *manager);
  /**
   * Set WifiRemoteStationsManager this DmgAtiDca is associated to.
   *
   * \param remoteManager WifiRemoteStationManager
   */
  void SetWifiRemoteStationManager (Ptr<WifiRemoteStationManager> remoteManager);
  /**
   * Set MacTxMiddle this DmgAtiDca is associated to.
   *
   * \param txMiddle MacTxMiddle
   */
  void SetTxMiddle (MacTxMiddle *txMiddle);

  /**
   * \param callback the callback to invoke when a
   * packet transmission was completed successfully.
   */
  void SetTxOkCallback (TxPacketOk callback);
  /**
   * \param callback the callback to invoke when a
   * transmission without ACK was completed successfully.
   */
  void SetTxOkNoAckCallback (TxOk callback) ;
  /**
   * \param callback the callback to invoke when a
   * packet transmission was completed unsuccessfully.
   */
  void SetTxFailedCallback (TxFailed callback);
  /**
   * Return the packet queue associated with this DmgAtiDca.
   *
   * \return WifiMacQueue
   */
  Ptr<WifiMacQueue> GetQueue () const;

  //Inherited
  virtual void SetMinCw (uint32_t minCw);
  virtual void SetMaxCw (uint32_t maxCw);
  virtual void SetAifsn (uint32_t aifsn);
  virtual uint32_t GetMinCw (void) const;
  virtual uint32_t GetMaxCw (void) const;
  virtual uint32_t GetAifsn (void) const;

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
  class TransmissionListener;
  class NavListener;
  class PhyListener;
  class Dcf;
  friend class Dcf;
  friend class TransmissionListener;

  DmgAtiDca &operator = (const DmgAtiDca &);
  DmgAtiDca (const DmgAtiDca &o);

  //Inherited from ns3::Object
  /**
   * Return the MacLow associated with this DmgAtiDca.
   *
   * \return MacLow
   */
  Ptr<MacLow> Low (void);
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

  Dcf *m_dcf;
  DcfManager *m_manager;
  TxPacketOk m_txOkCallback;
  TxOk m_txOkNoAckCallback;
  TxFailed m_txFailedCallback;
  Ptr<WifiMacQueue> m_queue;
  MacTxMiddle *m_txMiddle;
  Ptr<MacLow> m_low;
  Ptr<WifiRemoteStationManager> m_stationManager;
  TransmissionListener *m_transmissionListener;
  Ptr<WifiMac> m_wifiMac;

  Ptr<const Packet> m_currentPacket;
  WifiMacHeader m_currentHdr;

  Time m_transmissionStarted;   /* The time of the initiation of transmission */
  Time m_atiDuration;           /* The duration of the ATI*/
  Time m_remainingDuration;     /* The remaining duration till the end of this CBAP allocation*/
  bool m_allowTransmission;
};

} //namespace ns3

#endif /* DMG_ATI_DCA_H */
