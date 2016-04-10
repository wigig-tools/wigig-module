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
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */
#ifndef MULTI_BAND_NET_DEVICE_H
#define MULTI_BAND_NET_DEVICE_H

#include "ns3/net-device.h"
#include "ns3/packet.h"
#include "ns3/traced-callback.h"
#include "ns3/mac48-address.h"
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
   * \param
   * \param isInitiator
   */
  void BandChanged (enum WifiPhyStandard standard, Mac48Address address, bool isInitiator);
  /**
   * Execute Fast Session Transfer from the current band to the supplied band in MultibandElement.
   * \param address The address of the station to execute FST operation with.
   */
  void EstablishFastSessionTransferSession (Mac48Address address);
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

  uint32_t m_ifIndex;
  bool m_linkUp;
  TracedCallback<> m_linkChanges;
  bool m_configComplete;

  Ptr<WifiPhy> m_phy;
  Ptr<WifiMac> m_mac;
  Ptr<WifiRemoteStationManager> m_stationManager;   //!< Current Active Station Manager.
  enum WifiPhyStandard m_standard;                  //!< Current Active standard.

  NetDevice::ReceiveCallback m_forwardUp;
  NetDevice::PromiscReceiveCallback m_promiscRx;
  TracedCallback<Ptr<const Packet>, Mac48Address> m_rxLogger;
  TracedCallback<Ptr<const Packet>, Mac48Address> m_txLogger;

  Ptr<Node> m_node;                 //!< Node to which this device is attached to.
  mutable uint16_t m_mtu;           //!< Common MTU Size supported by underlying devices.
  WifiTechnologyList m_list;        //!< List of bands supported by this device.
  Mac48Address m_address;           //!< Address of this Multi-Band Device (Mac48Address)

};

} //namespace ns3

#endif /* MULTI_BAND_NET_DEVICE_H */
