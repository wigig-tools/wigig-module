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

#ifndef DMG_WIFI_CHANNEL_H
#define DMG_WIFI_CHANNEL_H

#include "ns3/channel.h"
#include "dmg-wifi-phy.h"

namespace ns3 {

class NetDevice;
class PropagationLossModel;
class PropagationDelayModel;
class Packet;
class Time;
class WifiPpdu;
class WifiTxVector;

enum ACTIVITY_TYPE {
  TX_ACTIVITY = 0,
  RX_ACTIVITY = 1,
};

/**
 * \brief a channel to interconnect ns3::DmgWifiPhy objects.
 * \ingroup wifi
 *
 * This class is expected to be used in tandem with the ns3::DmgWifiPhy
 * class and supports an ns3::PropagationLossModel and an
 * ns3::PropagationDelayModel.  By default, no propagation models are set;
 * it is the caller's responsibility to set them before using the channel.
 */
class DmgWifiChannel : public Channel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  DmgWifiChannel ();
  virtual ~DmgWifiChannel ();

  //inherited from Channel.
  virtual std::size_t GetNDevices (void) const;
  virtual Ptr<NetDevice> GetDevice (std::size_t i) const;

  /**
   * Adds the given DmgWifiPhy to the PHY list
   *
   * \param phy the DmgWifiPhy to be added to the PHY list
   */
  void Add (Ptr<DmgWifiPhy> phy);

  /**
   * \param loss the new propagation loss model.
   */
  void SetPropagationLossModel (const Ptr<PropagationLossModel> loss);
  /**
   * \param delay the new propagation delay model.
   */
  void SetPropagationDelayModel (const Ptr<PropagationDelayModel> delay);

  /**
   * \param sender the PHY object from which the packet is originating.
   * \param ppdu the PPDU to send
   * \param txPowerDbm the TX power associated to the packet, in dBm
   *
   * This method should not be invoked by normal users. It is
   * currently invoked only from DmgWifiPhy::StartTx.  The channel
   * attempts to deliver the PPDU to all other DmgWifiPhy objects
   * on the channel (except for the sender).
   */
  void Send (Ptr<DmgWifiPhy> sender, Ptr<const WifiPpdu> ppdu, double txPowerDbm) const;
  /**
   * Send AGC Subfield
   * \param sender
   * \param txPowerDbm
   * \param txVector
   */
  void SendAgcSubfield (Ptr<DmgWifiPhy> sender, double txPowerDbm, WifiTxVector txVector) const;
  /**
   * Send Channel Estimation (CE) Subfield of the TRN-Unit.
   * \param sender
   * \param txPowerDbm
   * \param txVector
   */
  void SendTrnCeSubfield (Ptr<DmgWifiPhy> sender, double txPowerDbm, WifiTxVector txVector) const;
  /**
   * Send TRN Subfield.
   * \param sender the device from which the packet is originating.
   * \param txPowerDbm the tx power associated to the packet.
   * \param txVector the TXVECTOR associated to the packet.
   */
  void SendTrnSubfield (Ptr<DmgWifiPhy> sender, double txPowerDbm, WifiTxVector txVector) const;

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
  void AddBlockage (double (*blockage)(), Ptr<DmgWifiPhy> srcWifiPhy, Ptr<DmgWifiPhy> dstWifiPhy);
  void RemoveBlockage (void);
  void AddPacketDropper (bool (*dropper)(), Ptr<DmgWifiPhy> srcWifiPhy, Ptr<DmgWifiPhy> dstWifiPhy);
  void RemovePacketDropper (void);
  /**
   * Record PHY Activity.
   * \param srcID the ID of the transmitting node.
   * \param dstID the ID of the receiving node.
   * \param duration the duration of the activity in nanoseconds.
   * \param power the power of the transmitted or received part of the PLCP.
   * \param fieldType the type of the PLCP field being transmitted or received.
   * \param activityType the type of the PHY activity.
   */
  void RecordPhyActivity (uint32_t srcID, uint32_t dstID, Time duration, double power,
                          PLCP_FIELD_TYPE fieldType, ACTIVITY_TYPE activityType) const;
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
   * A vector of pointers to DmgWifiPhy.
   */
  typedef std::vector<Ptr<DmgWifiPhy> > PhyList;

  /**
   * This method is scheduled by Send for each associated DmgWifiPhy.
   * The method PPDU calls the corresponding DmgWifiPhy that the first
   * bit of the packet has arrived.
   *
   * \param receiver the device to which the packet is destined
   * \param ppdu the PPDU being sent
   * \param txPowerDbm the TX power associated to the packet being sent (dBm)
   */
  static void Receive (Ptr<DmgWifiPhy> receiver, Ptr<WifiPpdu> ppdu, double txPowerDbm);
  /**
   * Generic function for receiving any subfield in TRN-Block.
   * \param i
   * \param sender
   * \param txVector
   * \param txPowerDbm
   * \param txAntennaGainDbi The transmit gain of the antenna at the sender once this subfield is transmitted.
   * \param type PLCP field type.
   * \return Received power over the subfield.
   */
  double ReceiveSubfield (uint32_t i, Ptr<DmgWifiPhy> sender, WifiTxVector txVector,
                          double txPowerDbm, double txAntennaGainDbi,
                          Time duration, PLCP_FIELD_TYPE type) const;
  /**
   * Receive AGC Subfield.
   * \param i
   * \param sender
   * \param txVector
   * \param txPowerDbm
   */
  void ReceiveAgcSubfield (uint32_t i, Ptr<DmgWifiPhy> sender, WifiTxVector txVector, double txPowerDbm, double txAntennaGainDbi) const;
  /**
   * Receive TRN-CE Subfield.
   * \param i
   * \param sender
   * \param txVector
   * \param txPowerDbm
   */
  void ReceiveTrnCeSubfield (uint32_t i, Ptr<DmgWifiPhy> sender, WifiTxVector txVector, double txPowerDbm, double txAntennaGainDbi) const;
  /**
   * Receive TRN Subfield in TRN Unit.
   * \param i index of the corresponding DmgWifiPhy in the PHY list.
   * \param txVector the TXVECTOR of the packet.
   * \param txPowerDbm the transmitted signal strength [dBm].
   * \param txAntennaGainDbi The gain of the transmit antenna in dBi.
   */
  void ReceiveTrnSubfield (uint32_t i, Ptr<DmgWifiPhy> sender, WifiTxVector txVector,
                           double txPowerDbm, double txAntennaGainDbi) const;

  PhyList m_phyList;                   //!< List of DmgWifiPhys connected to this DmgWifiChannel
  Ptr<PropagationLossModel> m_loss;    //!< Propagation loss model
  Ptr<PropagationDelayModel> m_delay;  //!< Propagation delay model
  double (*m_blockage) ();             //!< Blockage model.
  bool (*m_packetDropper) ();          //!< Packet Dropper Model.
  Ptr<WifiPhy> m_srcWifiPhy;
  Ptr<WifiPhy> m_dstWifiPhy;
  std::vector<double> m_receivedSignalStrength;    //!< List of received signal strength.
  uint64_t m_currentSignalStrengthIndex;           //!< Index of the current signal strength.
  bool m_experimentalMode;                         //!< Experimental mode used for injecting signal strength values.
  Time m_updateFrequency;                          //!< Update frequency of the results.

  /**
   * TracedCallback signature for reporting PHY activities.
   *
   * \param srcID the ID of the transmitting node.
   * \param dstID the ID of the receiving node.
   * \param duration the duration of the activity in nanoseconds.
   * \param power the power of the transmitted or received part of the PLCP.
   * \param fieldType the type of the PLCP field being transmitted or received.
   * \param activityType the type of the PHY activity.
   */
  typedef void (* PhyActivityTraceCallback)(uint32_t srcID,
                                            uint32_t dstID,
                                            Time duration,
                                            double power,
                                            uint16_t fieldType,
                                            uint16_t activityType);
  /**
   * A trace source that emulates a channel activity tracker to monitor power changes.
   */
  typedef TracedCallback<uint32_t, uint32_t, Time, double, uint16_t, uint16_t> PhyActivityTrace;
  PhyActivityTrace m_phyActivityTrace;

};

} //namespace ns3

#endif /* DMG_WIFI_CHANNEL_H */
