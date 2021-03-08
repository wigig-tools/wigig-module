/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Institute
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

#ifndef DMG_WIFI_HELPER_H
#define DMG_WIFI_HELPER_H

#include "ns3/dmg-wifi-channel.h"
#include "dmg-wifi-mac-helper.h"
#include "spectrum-wifi-helper.h"
#include "wifi-helper.h"

namespace ns3 {

/**
 * \brief manage and create wifi channel objects for the IEEE 802.11ad/ay models.
 *
 * The intent of this class is to make it easy to create a channel object
 * which implements the Dmg channel model. The Dmg channel model is described
 * in "Yet Another Network Simulator", http://cutebugs.net/files/wns2-Dmg.pdf
 */
class DmgWifiChannelHelper
{
public:
  /**
   * Create a channel helper without any parameter set. The user must set
   * them all to be able to call Create later.
   */
  DmgWifiChannelHelper ();

  /**
   * Create a channel helper in a default working state. By default, we create
   * a channel model with a propagation delay equal to a constant, the speed of light,
   * and a propagation loss based on a log distance model with a reference loss of 46.6777 dB
   * at reference distance of 1m.
   * \returns DmgWifiChannelHelper
   */
  static DmgWifiChannelHelper Default (void);

  /**
   * \param name the name of the model to add
   * \param n0 the name of the attribute to set
   * \param v0 the value of the attribute to set
   * \param n1 the name of the attribute to set
   * \param v1 the value of the attribute to set
   * \param n2 the name of the attribute to set
   * \param v2 the value of the attribute to set
   * \param n3 the name of the attribute to set
   * \param v3 the value of the attribute to set
   * \param n4 the name of the attribute to set
   * \param v4 the value of the attribute to set
   * \param n5 the name of the attribute to set
   * \param v5 the value of the attribute to set
   * \param n6 the name of the attribute to set
   * \param v6 the value of the attribute to set
   * \param n7 the name of the attribute to set
   * \param v7 the value of the attribute to set
   *
   * Add a propagation loss model to the set of currently-configured loss models.
   * This method is additive to allow you to construct complex propagation loss models
   * such as a log distance + jakes model, etc.
   *
   * The order in which PropagationLossModels are added may be significant. Some
   * propagation models are dependent of the "txPower" (eg. Nakagami model), and
   * are therefore not commutative. The final receive power (excluding receiver
   * gains) are calculated in the order the models are added.
   */
  void AddPropagationLoss (std::string name,
                           std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
                           std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                           std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
                           std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
                           std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
                           std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
                           std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
                           std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());
  /**
   * \param name the name of the model to set
   * \param n0 the name of the attribute to set
   * \param v0 the value of the attribute to set
   * \param n1 the name of the attribute to set
   * \param v1 the value of the attribute to set
   * \param n2 the name of the attribute to set
   * \param v2 the value of the attribute to set
   * \param n3 the name of the attribute to set
   * \param v3 the value of the attribute to set
   * \param n4 the name of the attribute to set
   * \param v4 the value of the attribute to set
   * \param n5 the name of the attribute to set
   * \param v5 the value of the attribute to set
   * \param n6 the name of the attribute to set
   * \param v6 the value of the attribute to set
   * \param n7 the name of the attribute to set
   * \param v7 the value of the attribute to set
   *
   * Configure a propagation delay for this channel.
   */
  void SetPropagationDelay (std::string name,
                            std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
                            std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                            std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
                            std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
                            std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
                            std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
                            std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
                            std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());

  /**
   * \returns a new channel
   *
   * Create a channel based on the configuration parameters set previously.
   */
  Ptr<DmgWifiChannel> Create (void) const;

  /**
  * Assign a fixed random variable stream number to the random variables
  * used by the channel.  Typically this corresponds to random variables
  * used in the propagation loss models.  Return the number of streams
  * (possibly zero) that have been assigned.
  *
  * \param c NetDeviceContainer of the set of net devices for which the
  *          WifiNetDevice should be modified to use fixed streams
  * \param stream first stream index to use
  *
  * \return the number of stream indices assigned by this helper
  */
  int64_t AssignStreams (Ptr<DmgWifiChannel> c, int64_t stream);


private:
  std::vector<ObjectFactory> m_propagationLoss; ///< vector of propagation loss models
  ObjectFactory m_propagationDelay; ///< propagation delay model
};


/**
 * \brief Make it easy to create and manage PHY objects for the Dmg model.
 *
 * The Dmg PHY model is described in "Yet Another Network Simulator",
 * http://cutebugs.net/files/wns2-Dmg.pdf
 *
 * The Pcap and ascii traces generated by the EnableAscii and EnablePcap methods defined
 * in this class correspond to PHY-level traces and come to us via WifiPhyHelper
 *
 */
class DmgWifiPhyHelper : public WifiPhyHelper
{
public:
  /**
   * Create a phy helper without any parameter set. The user must set
   * them all to be able to call Install later.
   */
  DmgWifiPhyHelper ();

  /**
   * Create a phy helper in a default working state.
   * \returns a default DmgWifiPhyHelper
   */
  static DmgWifiPhyHelper Default (void);

  /**
   * \param channel the channel to associate to this helper
   *
   * Every PHY created by a call to Install is associated to this channel.
   */
  void SetChannel (Ptr<DmgWifiChannel> channel);
  /**
   * \param channelName The name of the channel to associate to this helper
   *
   * Every PHY created by a call to Install is associated to this channel.
   */
  void SetChannel (std::string channelName);

protected:
  friend class DmgWifiHelper;

  /**
   * Set the default values for the WiGig PHY layer.
   */
  void SetDefaultWiGigPhyValues (void);
  /**
   * \param node the node on which we wish to create a wifi PHY
   * \param device the device within which this PHY will be created
   * \returns a newly-created PHY object.
   *
   * This method implements the pure virtual method defined in \ref ns3::WifiPhyHelper.
   */
  virtual Ptr<WifiPhy> Create (Ptr<Node> node, Ptr<NetDevice> device) const;

  Ptr<DmgWifiChannel> m_channel; ///< Dmg wifi channel

};

/**
 * \brief Make it easy to create and manage PHY objects for the spectrum model.
 *
 * The Pcap and ascii traces generated by the EnableAscii and EnablePcap methods defined
 * in this class correspond to PHY-level traces and come to us via WifiPhyHelper
 *
 */
class SpectrumDmgWifiPhyHelper : public DmgWifiPhyHelper
{
public:
  /**
   * Create a phy helper without any parameter set. The user must set
   * them all to be able to call Install later.
   */
  SpectrumDmgWifiPhyHelper ();

  /**
   * Create a phy helper in a default working state.
   * \returns a default SpectrumWifPhyHelper
   */
  static SpectrumDmgWifiPhyHelper Default (void);

  /**
   * \param channel the channel to associate to this helper
   *
   * Every PHY created by a call to Install is associated to this channel.
   */
  void SetChannel (Ptr<SpectrumChannel> channel);
  /**
   * \param channelName The name of the channel to associate to this helper
   *
   * Every PHY created by a call to Install is associated to this channel.
   */
  void SetChannel (std::string channelName);

private:
  friend class DmgWifiHelper;
  /**
   * \param node the node on which we wish to create a wifi PHY
   * \param device the device within which this PHY will be created
   * \returns a newly-created PHY object.
   *
   * This method implements the pure virtual method defined in \ref ns3::WifiPhyHelper.
   */
  virtual Ptr<WifiPhy> Create (Ptr<Node> node, Ptr<NetDevice> device) const;

  Ptr<SpectrumChannel> m_channel; ///< the channel
};

/**
 * \brief helps to create WifiNetDevice objects
 *
 * This class can help to create a large set of similar
 * WifiNetDevice objects and to configure a large set of
 * their attributes during creation.
 */
class DmgWifiHelper : public WifiHelper
{
public:
  virtual ~DmgWifiHelper ();

  /**
   * Create a Wifi helper in an empty state: all its parameters
   * must be set before calling ns3::WifiHelper::Install
   *
   * The default state is defined as being an Adhoc MAC layer with an ARF rate control algorithm
   * and both objects using their default attribute values.
   * By default, configure MAC and PHY for 802.11a.
   */
  DmgWifiHelper ();

  /**
   * \param phy the PHY helper to create PHY objects
   * \param mac the MAC helper to create MAC objects
   * \param first lower bound on the set of nodes on which a wifi device must be created
   * \param last upper bound on the set of nodes on which a wifi device must be created
   * \returns a device container which contains all the devices created by this method.
   */
  NetDeviceContainer Install (const DmgWifiPhyHelper &phy,
                              const DmgWifiMacHelper &mac,
                              NodeContainer::Iterator first,
                              NodeContainer::Iterator last) const;
  /**
   * \param phy the PHY helper to create PHY objects
   * \param mac the MAC helper to create MAC objects
   * \param c the set of nodes on which a wifi device must be created
   * \returns a device container which contains all the devices created by this method.
   */
  NetDeviceContainer Install (const DmgWifiPhyHelper &phy,
                              const DmgWifiMacHelper &mac, NodeContainer c) const;
  /**
   * \param phy the PHY helper to create PHY objects
   * \param mac the MAC helper to create MAC objects
   * \param node the node on which a wifi device must be created
   * \returns a device container which contains all the devices created by this method.
   */
  NetDeviceContainer Install (const DmgWifiPhyHelper &phy,
                              const DmgWifiMacHelper &mac, Ptr<Node> node) const;
  /**
   * \param phy the PHY helper to create PHY objects
   * \param mac the MAC helper to create MAC objects
   * \param nodeName the name of node on which a wifi device must be created
   * \returns a device container which contains all the devices created by this method.
   */
  NetDeviceContainer Install (const DmgWifiPhyHelper &phy,
                              const DmgWifiMacHelper &mac, std::string nodeName) const;
  /**
   * \param name the name of the codebook to set.
   * \param n0 the name of the attribute to set
   * \param v0 the value of the attribute to set
   * \param n1 the name of the attribute to set
   * \param v1 the value of the attribute to set
   * \param n2 the name of the attribute to set
   * \param v2 the value of the attribute to set
   * \param n3 the name of the attribute to set
   * \param v3 the value of the attribute to set
   * \param n4 the name of the attribute to set
   * \param v4 the value of the attribute to set
   * \param n5 the name of the attribute to set
   * \param v5 the value of the attribute to set
   * \param n6 the name of the attribute to set
   * \param v6 the value of the attribute to set
   * \param n7 the name of the attribute to set
   * \param v7 the value of the attribute to set
   *
   * Set the codebook type and its attributes to use when Install is called.
   */
  void SetCodebook (std::string name,
                    std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
                    std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                    std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
                    std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
                    std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
                    std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
                    std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
                    std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());
  /**
   * \param phy the PHY helper to create PHY objects
   * \param mac the MAC helper to create MAC objects
   * \param first lower bound on the set of nodes on which a wifi device must be created
   * \param last upper bound on the set of nodes on which a wifi device must be created
   * \returns a device container which contains all the devices created by this method.
   */
  NetDeviceContainer Install (const SpectrumDmgWifiPhyHelper &phyHelper,
                              const DmgWifiMacHelper &macHelper,
                              NodeContainer::Iterator first,
                              NodeContainer::Iterator last, bool installCodebook = true) const;

  /**
   * \param phy the PHY helper to create PHY objects
   * \param mac the MAC helper to create MAC objects
   * \param c the set of nodes on which a wifi device must be created
   * \returns a device container which contains all the devices created by this method.
   */
  NetDeviceContainer Install (const SpectrumDmgWifiPhyHelper &phy,
                              const DmgWifiMacHelper &mac, NodeContainer c, bool installCodebook = true) const;
  /**
   * \param phy the PHY helper to create PHY objects
   * \param mac the MAC helper to create MAC objects
   * \param node the node on which a wifi device must be created
   * \returns a device container which contains all the devices created by this method.
   */
  NetDeviceContainer Install (const SpectrumDmgWifiPhyHelper &phy,
                              const DmgWifiMacHelper &mac, Ptr<Node> node, bool installCodebook = true) const;
  /**
   * \param phy the PHY helper to create PHY objects
   * \param mac the MAC helper to create MAC objects
   * \param nodeName the name of node on which a wifi device must be created
   * \returns a device container which contains all the devices created by this method.
   */
  NetDeviceContainer Install (const SpectrumDmgWifiPhyHelper &phy,
                              const DmgWifiMacHelper &mac, std::string nodeName, bool installCodebook = true) const;

private:
  ObjectFactory m_codeBook;                  ///< Codebook factory for all the devices

};

} //namespace ns3

#endif /* DMG_WIFI_HELPER_H */
