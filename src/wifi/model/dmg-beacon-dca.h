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
#ifndef DMG_BEACON_DCA_H
#define DMG_BEACON_DCA_H

#include <stdint.h>
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-mode.h"
#include "ns3/wifi-remote-station-manager.h"
#include "ns3/dcf.h"
#include "ext-headers.h"

namespace ns3 {

class DcfState;
class DcfManager;
class WifiMacQueue;
class MacLow;
class WifiMacParameters;
class WifiMac;

class DmgBeaconDca : public Dcf
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

  DmgBeaconDca ();
  ~DmgBeaconDca ();

  /**
   * Set MacLow associated with this DcaTxop.
   *
   * \param low MacLow
   */
  void SetLow (Ptr<MacLow> low);
  /**
   * Set DcfManager this DcaTxop is associated to.
   *
   * \param manager DcfManager
   */
  void SetManager (DcfManager *manager);
  /**
   * Set WifiRemoteStationsManager this DcaTxop is associated to.
   *
   * \param remoteManager WifiRemoteStationManager
   */
  void SetWifiRemoteStationManager (Ptr<WifiRemoteStationManager> remoteManager);
  /**
   * Set the upper layer MAC this DcaTxop is associated to.
   *
   * \return WifiMac
   */
  void SetWifiMac (Ptr<WifiMac> mac);
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
   * Return the packet queue associated with this DcaTxop.
   *
   * \return WifiMacQueue
   */
  Ptr<WifiMacQueue> GetQueue (void) const;

  //Inherited
  virtual void SetMinCw (uint32_t minCw);
  virtual void SetMaxCw (uint32_t maxCw);
  virtual void SetAifsn (uint32_t aifsn);
  virtual uint32_t GetMinCw (void) const;
  virtual uint32_t GetMaxCw (void) const;
  virtual uint32_t GetAifsn (void) const;

  /**
   * \param beacon The DMG Beacon Body.
   * \param hdr header of packet to send.
   *
   * Start channel access procedure to transmit one DMG Beacon.
   */
  void TransmitDmgBeacon (const ExtDMGBeacon &beacon, const WifiMacHeader &hdr);

private:
  class TransmissionListener;
  class NavListener;
  class PhyListener;
  class Dcf;
  friend class Dcf;
  friend class TransmissionListener;

  DmgBeaconDca &operator = (const DmgBeaconDca &);
  DmgBeaconDca (const DmgBeaconDca &o);

  //Inherited from ns3::Object
  /**
   * Return the MacLow associated with this DcaTxop.
   *
   * \return MacLow
   */
  Ptr<MacLow> Low (void);
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

  Dcf *m_dcf;
  DcfManager *m_manager;
  TxOk m_txOkNoAckCallback;
  TxFailed m_txFailedCallback;
  Ptr<MacLow> m_low;
  Ptr<WifiRemoteStationManager> m_stationManager;
  TransmissionListener *m_transmissionListener;
  Ptr<WifiMac> m_wifiMac;

  ExtDMGBeacon m_currentBeacon;
  WifiMacHeader m_currentHdr;
  bool m_transmittingBeacon;
};

} //namespace ns3

#endif /* DMG_BEACON_DCA_H */
