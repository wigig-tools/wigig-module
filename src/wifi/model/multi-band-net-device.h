/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015, 2016 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#ifndef MULTI_BAND_NET_DEVICE_H
#define MULTI_BAND_NET_DEVICE_H

#include "ns3/mac48-address.h"
#include "ns3/net-device.h"
#include "ns3/packet.h"
#include "ns3/traced-callback.h"
#include "wifi-phy-standard.h"
#include <string>
#include <map>

namespace ns3 {

class WifiRemoteStationManager;
class WifiChannel;
class WifiPhy;
class WifiMac;

/* Note only one technology should be operational (Tx/Rx Data) at anytime */

typedef struct {
  Ptr<WifiPhy> Phy;
  Ptr<WifiMac> Mac;
  Ptr<WifiRemoteStationManager> StationManager;
  enum WifiPhyStandard Standard;
  bool Operational;                                 /* Flag to indicate whether this technology is operational or not */
} WifiTechnology;

typedef std::map<enum WifiPhyStandard, WifiTechnology> WifiTechnologyList;

/* Typedef to map each station with specific access technology */
typedef std::map<Mac48Address, Ptr<WifiMac> > TransmissionTechnologyMap;

/**
 * \brief Hold together all Wifi-related objects.
 * \ingroup wifi
 *
 * This class holds together ns3::WifiTransparentFstDevice for both 802.11ad and legancy 802.11
 */
class MultiBandNetDevice : public NetDevice
{
public:
  static TypeId GetTypeId (void);

  MultiBandNetDevice ();
  virtual ~MultiBandNetDevice ();

  /**
   * Add new Wifi band.
   * \param technology the new Wifi technology
   */
  void AddNewWifiTechnology (Ptr<WifiPhy> phy, Ptr<WifiMac> mac, Ptr<WifiRemoteStationManager> station,
                             enum WifiPhyStandard standard, bool opertional);
  /**
   * \returns
   */
  WifiTechnologyList GetWifiTechnologyList (void) const;
  /**
   * Switch the current Wifi Technology.
   * \param Standard The standard to use.
   */
  void SwitchTechnology (enum WifiPhyStandard standard);
  /**
   * Switch the current Wifi Technology.
   * \param standard The new standard to use for communication.
   * \param address The address of the peer station.
   * \param isInitiator Initiator of the FST session.
   */
  void BandChanged (enum WifiPhyStandard standard, Mac48Address address, bool isInitiator);
  /**
   * Execute Fast Session Transfer from the current band to the supplied band in MultibandElement.
   * \param address The address of the station to execute FST operation with.
   */
  void EstablishFastSessionTransferSession (Mac48Address address);
  /**
   * \param standard The standard for which the returned Station Manager correspondes to.
   * \return
   */
  Ptr<WifiRemoteStationManager> GetCurrentRemoteStationManager (enum WifiPhyStandard standard) const;
  /**
   * \param standard The standard for which the returned WifiMac correspondes to.
   * \return
   */
  Ptr<WifiMac> GetTechnologyMac (enum WifiPhyStandard standard);
  /**
   * \param standard The standard for which the returned WifiPhy correspondes to.
   * \return
   */
  Ptr<WifiPhy> GetTechnologyPhy (enum WifiPhyStandard standard);
  /**
   * \returns the mac we are currently using.
   */
  Ptr<WifiMac> GetMac (void) const;
  /**
   * \returns the phy we are currently using.
   */
  Ptr<WifiPhy> GetPhy (void) const;
  /**
   * \returns the remote station manager we are currently using.
   */
  Ptr<WifiRemoteStationManager> GetRemoteStationManager (void) const;

  //inherited from NetDevice base class.
  virtual void SetIfIndex (const uint32_t index);
  virtual uint32_t GetIfIndex (void) const;
  virtual Ptr<Channel> GetChannel (void) const;
  virtual void SetAddress (Address address);
  virtual Address GetAddress (void) const;
  virtual bool SetMtu (const uint16_t mtu);
  virtual uint16_t GetMtu (void) const;
  virtual bool IsLinkUp (void) const;
  virtual void AddLinkChangeCallback (Callback<void> callback);
  virtual bool IsBroadcast (void) const;
  virtual Address GetBroadcast (void) const;
  virtual bool IsMulticast (void) const;
  virtual Address GetMulticast (Ipv4Address multicastGroup) const;
  virtual bool IsPointToPoint (void) const;
  virtual bool IsBridge (void) const;
  virtual bool Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber);
  virtual Ptr<Node> GetNode (void) const;
  virtual void SetNode (Ptr<Node> node);
  virtual bool NeedsArp (void) const;
  virtual void SetReceiveCallback (NetDevice::ReceiveCallback cb);
  virtual Address GetMulticast (Ipv6Address addr) const;
  virtual bool SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber);
  virtual void SetPromiscReceiveCallback (PromiscReceiveCallback cb);
  virtual bool SupportsSendFrom (void) const;

protected:
  virtual void DoDispose (void);
  virtual void DoInitialize (void);
  virtual void NotifyNewAggregate (void);
  /**
   * Receive a packet from the lower layer and pass the
   * packet up the stack.
   *
   * \param packet
   * \param from
   * \param to
   */
  void ForwardUp (Ptr<Packet> packet, Mac48Address from, Mac48Address to);

private:
  // This value conforms to the 802.11 specification
  static const uint16_t MAX_MSDU_SIZE = 7920;

  /**
   * Set that the link is up. A link is always up in ad-hoc mode.
   * For a STA, a link is up when the STA is associated with an AP.
   */
  void LinkUp (void);
  /**
   * Set that the link is down (i.e. STA is not associated).
   */
  void LinkDown (void);
  /**
   * Return the WifiChannel this device is connected to.
   *
   * \return WifiChannel
   */
  Ptr<WifiChannel> DoGetChannel (void) const;
  /**
   * Complete the configuration of this Wi-Fi device by
   * connecting all lower components (e.g. MAC, WifiRemoteStation) together.
   */
  void CompleteConfig (void);

  /**
   * \brief Determine the tx queue for a given packet
   * \param item the packet
   *
   * Modelled after the Linux function ieee80211_select_queue (net/mac80211/wme.c).
   * A SocketPriority tag is attached to the packet (or the existing one is
   * replaced) to carry the user priority, which is set to the three most
   * significant bits of the DS field (TOS field in case of IPv4 and Traffic
   * Class field in case of IPv6). The Access Category corresponding to the
   * user priority according to the QosUtilsMapTidToAc function is returned.
   *
   * The following table shows the mapping for the Diffserv Per Hop Behaviors.
   *
   * PHB  | TOS (binary) | UP  | Access Category
   * -----|--------------|-----|-----------------
   * EF   |   101110xx   |  5  |     AC_VI
   * AF11 |   001010xx   |  1  |     AC_BK
   * AF21 |   010010xx   |  2  |     AC_BK
   * AF31 |   011010xx   |  3  |     AC_BE
   * AF41 |   100010xx   |  4  |     AC_VI
   * AF12 |   001100xx   |  1  |     AC_BK
   * AF22 |   010100xx   |  2  |     AC_BK
   * AF32 |   011100xx   |  3  |     AC_BE
   * AF42 |   100100xx   |  4  |     AC_VI
   * AF13 |   001110xx   |  1  |     AC_BK
   * AF23 |   010110xx   |  2  |     AC_BK
   * AF33 |   011110xx   |  3  |     AC_BE
   * AF43 |   100110xx   |  4  |     AC_VI
   * CS0  |   000000xx   |  0  |     AC_BE
   * CS1  |   001000xx   |  1  |     AC_BK
   * CS2  |   010000xx   |  2  |     AC_BK
   * CS3  |   011000xx   |  3  |     AC_BE
   * CS4  |   100000xx   |  4  |     AC_VI
   * CS5  |   101000xx   |  5  |     AC_VI
   * CS6  |   110000xx   |  6  |     AC_VO
   * CS7  |   111000xx   |  7  |     AC_VO
   *
   * This method is called by the traffic control layer before enqueuing a
   * packet in the queue disc, if a queue disc is installed on the outgoing
   * device, or passing a packet to the device, otherwise.
   */
  uint8_t SelectQueue (Ptr<QueueItem> item) const;

  uint32_t m_ifIndex;
  bool m_linkUp;
  TracedCallback<> m_linkChanges;
  bool m_configComplete;

  Ptr<WifiPhy> m_phy;                               //!< Current Active PHY layer.
  Ptr<WifiMac> m_mac;                               //!< Current Active MAC layer.
  Ptr<WifiRemoteStationManager> m_stationManager;   //!< Current Active Station Manager.
  Ptr<NetDeviceQueueInterface> m_queueInterface;    //!< NetDevice queue interface
  enum WifiPhyStandard m_standard;                  //!< Current Active standard.

  NetDevice::ReceiveCallback m_forwardUp;
  NetDevice::PromiscReceiveCallback m_promiscRx;
  TracedCallback<Ptr<const Packet>, Mac48Address> m_rxLogger;
  TracedCallback<Ptr<const Packet>, Mac48Address> m_txLogger;

  Ptr<Node> m_node;                           //!< Node to which this device is attached to.
  mutable uint16_t m_mtu;                     //!< Common MTU Size supported by underlying devices.
  WifiTechnologyList m_list;                  //!< List of technologies supported by this device.
  TransmissionTechnologyMap m_technologyMap;  //!< Map between peer station and the corresponding transmission technology.
  Mac48Address m_address;                     //!< Address of this Multi-Band Device (Mac48Address).

};

} //namespace ns3

#endif /* MULTI_BAND_NET_DEVICE_H */
