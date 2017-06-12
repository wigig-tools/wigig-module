/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006,2007 INRIA
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
 * Author: Mathieu Lacage, <mathieu.lacage@sophia.inria.fr>
 */

#ifndef YANS_WIFI_CHANNEL_H
#define YANS_WIFI_CHANNEL_H

#include "ns3/channel.h"
#include "yans-wifi-phy.h"

namespace ns3 {

class NetDevice;
class PropagationLossModel;
class PropagationDelayModel;

/**
 * \brief a channel to interconnect ns3::YansWifiPhy objects.
 * \ingroup wifi
 *
 * This class is expected to be used in tandem with the ns3::YansWifiPhy
 * class and supports an ns3::PropagationLossModel and an 
 * ns3::PropagationDelayModel.  By default, no propagation models are set; 
 * it is the caller's responsibility to set them before using the channel.
 */
class YansWifiChannel : public Channel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  YansWifiChannel ();
  virtual ~YansWifiChannel ();

  //inherited from Channel.
  virtual uint32_t GetNDevices (void) const;
  virtual Ptr<NetDevice> GetDevice (uint32_t i) const;

  /**
   * Adds the given YansWifiPhy to the PHY list
   *
   * \param phy the YansWifiPhy to be added to the PHY list
   */
  void Add (Ptr<YansWifiPhy> phy);

  /**
   * \param loss the new propagation loss model.
   */
  void SetPropagationLossModel (Ptr<PropagationLossModel> loss);
  /**
   * \param delay the new propagation delay model.
   */
  void SetPropagationDelayModel (Ptr<PropagationDelayModel> delay);

  /**
   * \param sender the phy object from which the packet is originating.
   * \param packet the packet to send
   * \param txPowerDbm the tx power associated to the packet, in dBm
   * \param duration the transmission duration associated with the packet
   *
   * This method should not be invoked by normal users. It is
   * currently invoked only from YansWifiPhy::StartTx.  The channel
   * attempts to deliver the packet to all other YansWifiPhy objects
   * on the channel (except for the sender).
   */
  void Send (Ptr<YansWifiPhy> sender, Ptr<const Packet> packet, double txPowerDbm, Time duration) const;

  /**
   * Send TRN Field.
   * \param sender the device from which the packet is originating.
   * \param txPowerDbm the tx power associated to the packet.
   * \param txVector the TXVECTOR associated to the packet.
   */
  void SendTrn (Ptr<YansWifiPhy> sender, double txPowerDbm, WifiTxVector txVector, uint8_t fieldsRemaining) const;
  /**
   * Assign a fixed random variable stream number to the random variables
   * used by this model.  Return the number of streams (possibly zero) that
   * have been assigned.
   *
   * \param stream first stream index to use
   *
   * \return the number of stream indices assigned by this model
   */
  int64_t AssignStreams (int64_t stream);
  /**
   * Add bloackage on a certain path between two WifiPhy objects.
   * \param srcWifiPhy
   * \param dstWifiPhy
   */
  void AddBlockage (double (*blockage)(), Ptr<WifiPhy> srcWifiPhy, Ptr<WifiPhy> dstWifiPhy);
  void RemoveBlockage (void);
  void AddPacketDropper (bool (*dropper)(), Ptr<WifiPhy> srcWifiPhy, Ptr<WifiPhy> dstWifiPhy);
  void RemovePacketDropper (void);
  /**
   * Load received signal strength file (Experimental mode).
   * \param fileName The path to the file.
   * \param updateFreuqnecy The update freuqnecy of the results.
   */
  void LoadReceivedSignalStrengthFile (std::string fileName, Time updateFreuqnecy);
  /**
   * Update current signal strength value
   */
  void UpdateSignalStrengthValue (void);

private:
  /**
   * A vector of pointers to YansWifiPhy.
   */
  typedef std::vector<Ptr<YansWifiPhy> > PhyList;

  /**
   * This method is scheduled by Send for each associated YansWifiPhy.
   * The method then calls the corresponding YansWifiPhy that the first
   * bit of the packet has arrived.
   *
   * \param receiver the device to which the packet is destined
   * \param packet the packet being sent
   * \param txPowerDbm the tx power associated to the packet being sent (dBm)
   * \param duration the transmission duration associated with the packet being sent
   */
  void Receive (Ptr<YansWifiPhy> receiver, Ptr<Packet> packet, double txPowerDbm, Time duration) const;
  /**
   * \param i index of the corresponding YansWifiPhy in the PHY list.
   * \param txVector the TXVECTOR of the packet.
   * \param txPowerDbm the transmitted signal strength [dBm].
   */
  void ReceiveTrn (uint32_t i, Ptr<YansWifiPhy> sender, WifiTxVector txVector, double txPowerDbm, uint8_t fieldsRemaining) const;

  PhyList m_phyList;                   //!< List of YansWifiPhys connected to this YansWifiChannel
  Ptr<PropagationLossModel> m_loss;    //!< Propagation loss model
  Ptr<PropagationDelayModel> m_delay;  //!< Propagation delay model
  double (*m_blockage) ();              //!< Blockage model.
  bool (*m_packetDropper) ();           //!< Packet Dropper Model.
  Ptr<WifiPhy> m_srcWifiPhy;
  Ptr<WifiPhy> m_dstWifiPhy;
  std::vector<double> m_receivedSignalStrength;    //!< List of received signal strength.
  uint64_t m_currentSignalStrengthIndex;           //!< Index of the current signal strength.
  bool m_experimentalMode;                         //!< Experimental mode used for injecting signal strength values.
  Time m_updateFrequency;                          //!< Update frequency of the results.

};

} //namespace ns3

#endif /* YANS_WIFI_CHANNEL_H */
