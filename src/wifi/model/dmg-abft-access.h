#ifndef DMG_ABFT_ACCESS_H
#define DMG_ABF_TACCESS_H

#include <stdint.h>
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"

#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-mode.h"
#include "ns3/wifi-remote-station-manager.h"

namespace ns3 {

class WifiMacQueue;
class MacLow;
class WifiMacParameters;
class MacTxMiddle;

/**
 *
 */
class DmgAbftAccess : public Object
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

  DmgAbftAccess ();
  ~DmgAbftAccess ();

  /**
   * Set MacLow associated with this DcaTxop.
   *
   * \param low MacLow
   */
  void SetLow (Ptr<MacLow> low);
  /**
   * Set WifiRemoteStationsManager this DcaTxop is associated to.
   *
   * \param remoteManager WifiRemoteStationManager
   */
  void SetWifiRemoteStationManager (Ptr<WifiRemoteStationManager> remoteManager);
  /**
   * Set MacTxMiddle this DcaTxop is associated to.
   *
   * \param txMiddle MacTxMiddle
   */
  void SetTxMiddle (MacTxMiddle *txMiddle);
  /**
   * Set the upper layer MAC this DcaTxop is associated to.
   *
   * \return WifiMac
   */
  void SetWifiMac (Ptr<WifiMac> mac);

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
   * Return the packet queue associated with this DcaTxop.
   *
   * \return WifiMacQueue
   */
  Ptr<WifiMacQueue> GetQueue () const;
  /**
   * Return the upper layer MAC this DcaTxop is associated to.
   *
   * \return WifiMac
   */
  Ptr<WifiMac> GetWifiMac () const;

  /**
   * \param packet packet to send
   * \param hdr header of packet to send.
   *
   * Store the packet in the internal queue until it
   * can be sent safely.
   */
  void Queue (Ptr<const Packet> packet, const WifiMacHeader &hdr);

private:
  class TransmissionListener;
  class NavListener;
  class PhyListener;
  friend class TransmissionListener;

  DmgAbftAccess &operator = (const DmgAbftAccess &);
  DmgAbftAccess (const DmgAbftAccess &o);

  /**
   * Return the MacLow associated with this DcaTxop.
   *
   * \return MacLow
   */
  Ptr<MacLow> Low (void);
  void DoInitialize ();
  /* dcf notifications forwarded here */
  /**
   * Check if the DCF requires access.
   *
   * \return true if the DCF requires access,
   *         false otherwise
   */
  bool NeedsAccess (void) const;

  /**
   * Notify the DCF that access has been granted.
   */
  void NotifyAccessGranted (void);
  /**
   * When a channel switching occurs, enqueued packets are removed.
   */
  void NotifyChannelSwitching (void);
  /**
   * When sleep operation occurs, if there is a pending packet transmission,
   * it will be reinserted to the front of the queue.
   */
  void NotifySleep (void);
  /**
   * When wake up operation occurs, channel access will be restarted
   */
  void NotifyWakeUp (void);

  /* Event handlers */
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
   * Request access from DCF manager if needed.
   */
  void StartAccessIfNeeded (void);

  virtual void DoDispose (void);

  TxPacketOk m_txOkCallback;
  TxOk m_txOkNoAckCallback;
  TxFailed m_txFailedCallback;
  Ptr<WifiMacQueue> m_queue;
  MacTxMiddle *m_txMiddle;
  Ptr<MacLow> m_low;
  Ptr<WifiRemoteStationManager> m_stationManager;
  TransmissionListener *m_transmissionListener;
  Ptr<WifiMac> m_wifiMac;

  bool m_accessOngoing;
  Ptr<const Packet> m_currentPacket;
  WifiMacHeader m_currentHdr;

};

} //namespace ns3

#endif /* DMG_ABFT_ACCESS_H */
