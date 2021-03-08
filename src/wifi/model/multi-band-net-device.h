/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
 * Author: Hany Assasa <hany.assasa@gmail.com>
 */

#ifndef MULTI_BAND_NET_DEVICE_H
#define MULTI_BAND_NET_DEVICE_H

#include "ns3/mac48-address.h"
#include "ns3/net-device.h"
#include "ns3/traced-callback.h"
#include "wifi-phy-standard.h"
#include <map>

namespace ns3 {

class WifiRemoteStationManager;
class WifiPhy;
class WifiMac;

/* Note only one technology should be operational (Tx/Rx Data) at anytime */

struct WifiTechnology {
  Ptr<WifiPhy> Phy;
  Ptr<WifiMac> Mac;
  Ptr<WifiRemoteStationManager> StationManager;
  WifiPhyStandard Standard;
  bool Operational;                                 /* Flag to indicate whether this technology is operational or not */
} ;

typedef std::map<WifiPhyStandard, WifiTechnology> WifiTechnologyList;

/* Typedef to map each station with specific access technology */
typedef std::map<Mac48Address, Ptr<WifiMac> > TransmissionTechnologyMap;

/**
 * \brief Hold together all Wifi-related objects.
 * \ingroup wifi
 *
 * This class holds together ns3::WifiTransparentFstDevice for both 802.11ad/ay and legancy 802.11
 */
class MultiBandNetDevice : public NetDevice
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  MultiBandNetDevice ();
  virtual ~MultiBandNetDevice ();

  /**
   * Add new Wifi technology.
   * \param phy Pointer to the WifiPhy class of the new Wifi techology.
   * \param mac Pointer to the WifiMac class of the new Wifi techology.
   * \param station Pointer to the WifiRemoteStation class of the new Wifi techology.
   * \param standard The IEEE 802.11 standard supported by the new Wifi technology.
   * \param opertional.
   */
  void AddNewWifiTechnology (Ptr<WifiPhy> phy, Ptr<WifiMac> mac, Ptr<WifiRemoteStationManager> station,
                             WifiPhyStandard standard, bool opertional);
  /**
   * Returns lost of the supported Wifi technologies by this
   * \returns
   */
  WifiTechnologyList GetWifiTechnologyList (void) const;
  /**
   * Switch the current Wifi Technology.
   * \param Standard The standard to use.
   */
  void SwitchTechnology (WifiPhyStandard standard);
  /**
   * Switch the current Wifi Technology.
   * \param standard The new standard to use for communication.
   * \param address The address of the peer station.
   * \param isInitiator Initiator of the FST session.
   */
  void BandChanged (WifiPhyStandard standard, Mac48Address address, bool isInitiator);
  /**
   * Execute Fast Session Transfer from the current band to the supplied band in MultibandElement.
   * \param address The address of the station to execute FST operation with.
   */
  void EstablishFastSessionTransferSession (Mac48Address address);
  /**
   * \param standard The standard for which the returned Station Manager correspondes to.
   * \return
   */
  Ptr<WifiRemoteStationManager> GetCurrentRemoteStationManager (WifiPhyStandard standard) const;
  /**
   * \param standard The standard for which the returned WifiMac correspondes to.
   * \return
   */
  Ptr<WifiMac> GetTechnologyMac (WifiPhyStandard standard);
  /**
   * \param standard The standard for which the returned WifiPhy correspondes to.
   * \return
   */
  Ptr<WifiPhy> GetTechnologyPhy (WifiPhyStandard standard);
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
  void SetIfIndex (const uint32_t index);
  uint32_t GetIfIndex (void) const;
  Ptr<Channel> GetChannel (void) const;
  void SetAddress (Address address);
  Address GetAddress (void) const;
  bool SetMtu (const uint16_t mtu);
  uint16_t GetMtu (void) const;
  bool IsLinkUp (void) const;
  void AddLinkChangeCallback (Callback<void> callback);
  bool IsBroadcast (void) const;
  Address GetBroadcast (void) const;
  bool IsMulticast (void) const;
  Address GetMulticast (Ipv4Address multicastGroup) const;
  bool IsPointToPoint (void) const;
  bool IsBridge (void) const;
  bool Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber);
  Ptr<Node> GetNode (void) const;
  void SetNode (const Ptr<Node> node);
  bool NeedsArp (void) const;
  void SetReceiveCallback (NetDevice::ReceiveCallback cb);
  Address GetMulticast (Ipv6Address addr) const;
  bool SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber);
  void SetPromiscReceiveCallback (PromiscReceiveCallback cb);
  bool SupportsSendFrom (void) const;


protected:
  void DoDispose (void);
  void DoInitialize (void);
  /**
   * Receive a packet from the lower layer and pass the
   * packet up the stack.
   *
   * \param packet the packet to forward up
   * \param from the source address
   * \param to the destination address
   */
  void ForwardUp (Ptr<const Packet> packet, Mac48Address from, Mac48Address to);


private:
  /**
   * \brief Copy constructor
   * \param o object to copy
   *
   * Defined and unimplemented to avoid misuse
   */
  MultiBandNetDevice (const MultiBandNetDevice &o);

  /**
   * \brief Assignment operator
   * \param o object to copy
   * \returns the copied object
   *
   * Defined and unimplemented to avoid misuse
   */
  MultiBandNetDevice &operator = (const MultiBandNetDevice &o);

  /// This value conforms to the 802.11 specification
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
   * Complete the configuration of this Wi-Fi device by
   * connecting all lower components (e.g. MAC, WifiRemoteStation) together.
   */
  void CompleteConfig (void);

  Ptr<Node> m_node;                           		//!< Node to which this device is attached to.
  Ptr<WifiPhy> m_phy;                               //!< Current Active PHY layer.
  Ptr<WifiMac> m_mac;                               //!< Current Active MAC layer.
  Ptr<WifiRemoteStationManager> m_stationManager;   //!< Current Active Station Manager.
  Ptr<NetDeviceQueueInterface> m_queueInterface;    //!< NetDevice queue interface
  WifiPhyStandard m_standard;                  		//!< Current Active standard.

  NetDevice::ReceiveCallback m_forwardUp; //!< forward up callback
  NetDevice::PromiscReceiveCallback m_promiscRx; //!< promiscious receive callback

  TracedCallback<Ptr<const Packet>, Mac48Address> m_rxLogger; //!< receive trace callback
  TracedCallback<Ptr<const Packet>, Mac48Address> m_txLogger; //!< transmit trace callback

  WifiTechnologyList m_list;                  //!< List of technologies supported by this device.
  TransmissionTechnologyMap m_technologyMap;  //!< Map between peer station and the corresponding transmission technology.
  Mac48Address m_address;                     //!< Address of this Multi-Band Device (Mac48Address).

  uint32_t m_ifIndex; //!< IF index
  bool m_linkUp; //!< link up
  TracedCallback<> m_linkChanges; //!< link change callback
  mutable uint16_t m_mtu; //!< MTU
  bool m_configComplete; //!< configuration complete
};

} //namespace ns3

#endif /* MULTI_BAND_NET_DEVICE_H */
