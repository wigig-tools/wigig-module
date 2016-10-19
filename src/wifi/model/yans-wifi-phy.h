/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Ghada Badawy <gbadawy@gmail.com>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef YANS_WIFI_PHY_H
#define YANS_WIFI_PHY_H

#include "wifi-phy.h"

namespace ns3 {

class YansWifiChannel;

/**
 * \brief 802.11 PHY layer model
 * \ingroup wifi
 *
 * This PHY implements a model of 802.11a. The model
 * implemented here is based on the model described
 * in "Yet Another Network Simulator",
 * (http://cutebugs.net/files/wns2-yans.pdf).
 *
 *
 * This PHY model depends on a channel loss and delay
 * model as provided by the ns3::PropagationLossModel
 * and ns3::PropagationDelayModel classes, both of which are
 * members of the ns3::YansWifiChannel class.
 */
class YansWifiPhy : public WifiPhy
{
public:
  static TypeId GetTypeId (void);

  YansWifiPhy ();
  virtual ~YansWifiPhy ();

  /**
   * Set the YansWifiChannel this YansWifiPhy is to be connected to.
   *
   * \param channel the YansWifiChannel this YansWifiPhy is to be connected to
   */
  void SetChannel (Ptr<YansWifiChannel> channel);

  /**
   * Starting receiving the plcp of a packet (i.e. the first bit of the preamble has arrived).
   *
   * \param packet the arriving packet
   * \param rxPowerDbm the receive power in dBm
   * \param txVector the TXVECTOR of the arriving packet
   * \param preamble the preamble of the arriving packet
   * \param mpdutype the type of the MPDU as defined in WifiPhy::mpduType.
   * \param rxDuration the duration needed for the reception of the packet
   */
  void StartReceivePreambleAndHeader (Ptr<Packet> packet,
                                      double rxPowerDbm,
                                      WifiTxVector txVector,
                                      WifiPreamble preamble,
                                      enum mpduType mpdutype,
                                      Time rxDuration);
  /**
   * Starting receiving the payload of a packet (i.e. the first bit of the packet has arrived).
   *
   * \param packet the arriving packet
   * \param txVector the TXVECTOR of the arriving packet
   * \param preamble the preamble of the arriving packet
   * \param mpdutype the type of the MPDU as defined in WifiPhy::mpduType.
   * \param event the corresponding event of the first time the packet arrives
   */
  void StartReceivePacket (Ptr<Packet> packet,
                           WifiTxVector txVector,
                           WifiPreamble preamble,
                           enum mpduType mpdutype,
                           Ptr<InterferenceHelper::Event> event);

  virtual void SetReceiveOkCallback (WifiPhy::RxOkCallback callback);
  virtual void SetReceiveErrorCallback (WifiPhy::RxErrorCallback callback);
  virtual void SendPacket (Ptr<const Packet> packet, WifiTxVector txVector, enum WifiPreamble preamble);
  virtual void SendPacket (Ptr<const Packet> packet, WifiTxVector txVector, enum WifiPreamble preamble, enum mpduType mpdutype);

  /**
   * Send TRN Field to the destinated station.
   * \param txVector TxVector companioned by this transmission.
   * \param fieldsRemaining The number of TRN Fields remaining till the end of transmission.
   */
  void SendTrnField (WifiTxVector txVector, uint8_t fieldsRemaining);
  /**
   * Start receiving TRN Field
   * \param txVector
   * \param rxPowerDbm The received power in dBm.
   */
  void StartReceiveTrnField (WifiTxVector txVector, double rxPowerDbm, uint8_t fieldsRemaining);
  /**
   * Receive TRN Field
   * \param event The event related to the reception of this TRN Field.
   */
  void EndReceiveTrnField (uint8_t sectorId, uint8_t antennaId,
                           WifiTxVector txVector, uint8_t fieldsRemaining, Ptr<InterferenceHelper::Event> event);
  /**
   * This method is called once all the TRN Fields are received.
   */
  void EndReceiveTrnFields (void);

  virtual void RegisterListener (WifiPhyListener *listener);
  virtual void UnregisterListener (WifiPhyListener *listener);
  virtual void SetSleepMode (void);
  virtual void ResumeFromSleep (void);
  virtual Ptr<WifiChannel> GetChannel (void) const;

  /**
   * Register Report SNR Callback.
   * \param callback
   */
  virtual void RegisterReportSnrCallback (ReportSnrCallback callback);
  /**
   * This method is called when the station works in RDS mode.
   * \param srcSector The sector to use for communicating with the Source REDS.
   * \param srcAntenna The antenna to use for communicating with the Source REDS.
   * \param dstSector The sector to use for communicating with the Destination REDS.
   * \param dstAntenna The antenna to use for communicating with the Destination REDS.
   */
  virtual void ActivateRdsOpereation (uint8_t srcSector, uint8_t srcAntenna,
                                      uint8_t dstSector, uint8_t dstAntenna);

  /**
   * Resume RDS Opereation
   */
  void ResumeRdsOperation (void);
  /**
   * Suspend RDS Opereation.
   */
  virtual void SuspendRdsOperation (void);

protected:
  // Inherited
  virtual void DoDispose (void);
  virtual bool DoChannelSwitch (uint16_t id);
  virtual bool DoFrequencySwitch (uint32_t frequency);

private:
  /**
   * The last bit of the PSDU has arrived.
   *
   * \param packet the packet that the last bit has arrived
   * \param preamble the preamble of the arriving packet
   * \param aMpdu the type of the packet (0 is not A-MPDU, 1 is a MPDU that is part of an A-MPDU and 2 is the last MPDU in an A-MPDU)
   *        and the A-MPDU reference number (must be a different value for each A-MPDU but the same for each subframe within one A-MPDU)
   * \param event the corresponding event of the first time the packet arrives
   */
  void EndPsduReceive (Ptr<Packet> packet, enum WifiPreamble preamble, enum mpduType mpdutype, Ptr<InterferenceHelper::Event> event);
  /**
   * The last bit of the PSDU has arrived but we are still waiting for the TRN Fields to be received.
   *
   * \param packet the packet that the last bit has arrived
   * \param preamble the preamble of the arriving packet
   * \param aMpdu the type of the packet (0 is not A-MPDU, 1 is a MPDU that is part of an A-MPDU and 2 is the last MPDU in an A-MPDU)
   *        and the A-MPDU reference number (must be a different value for each A-MPDU but the same for each subframe within one A-MPDU)
   * \param event the corresponding event of the first time the packet arrives
   */
  void EndPsduOnlyReceive (Ptr<Packet> packet, PacketType packetType, enum WifiPreamble preamble, enum mpduType mpdutype, Ptr<InterferenceHelper::Event> event);

  Ptr<YansWifiChannel> m_channel;        //!< YansWifiChannel that this YansWifiPhy is connected to
 
  /* Variables to support 802.11ad */
  bool m_rdsActivated;                    //!< Flag to indicate if RDS is activated;
  ReportSnrCallback m_reportSnrCallback;  //!< Callback to support
  bool m_psduSuccess;                     //!< Flag if the PSDU has been received successfully.
  uint8_t m_srcSector;
  uint8_t m_srcAntenna;
  uint8_t m_dstSector;
  uint8_t m_dstAntenna;
  uint8_t m_rdsSector;
  uint8_t m_rdsAntenna;

};

} //namespace ns3

#endif /* YANS_WIFI_PHY_H */
