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
 *
 * Ported from yans-wifi-phy.h by several contributors starting
 * with Nicola Baldo and Dean Armstrong
 */

#ifndef SPECTRUM_DMG_WIFI_PHY_H
#define SPECTRUM_DMG_WIFI_PHY_H

#include "ns3/antenna-model.h"
#include "ns3/spectrum-channel.h"
#include "ns3/spectrum-interference.h"

#include "dmg-wifi-phy.h"
#include "dmg-wifi-spectrum-phy-interface.h"
#include "wifi-phy.h"

namespace ns3 {

/**
 * \ingroup wifi
 *
 * Signal parameters for DMG WiFi.
 */
struct DmgWifiSpectrumSignalParameters : public SpectrumSignalParameters
{

  // inherited from SpectrumSignalParameters
  virtual Ptr<SpectrumSignalParameters> Copy (void);

  /**
   * default constructor
   */
  DmgWifiSpectrumSignalParameters (void);
  /**
   * copy constructor
   * \param p the object to copy from.
   */
  DmgWifiSpectrumSignalParameters (const DmgWifiSpectrumSignalParameters& p);

  /**
   * The packet being transmitted with this signal
   */
  Ptr<Packet> packet;
  /**
   * The type of the PLCP.
   */
  PLCP_FIELD_TYPE plcpFieldType;
  /**
   * TxVector companioned by this transmission.
   */
  WifiTxVector txVector;

};

/**
 * \brief 802.11 PHY layer model
 * \ingroup wifi
 *
 * This PHY implements a spectrum-aware enhancement of the 802.11 SpectrumWifiPhy
 * model.
 *
 * This PHY model depends on a channel loss and delay
 * model as provided by the ns3::SpectrumPropagationLossModel
 * and ns3::PropagationDelayModel classes.
 *
 */
class SpectrumDmgWifiPhy : public DmgWifiPhy
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  SpectrumDmgWifiPhy ();
  virtual ~SpectrumDmgWifiPhy ();

  /**
   * Set the SpectrumChannel this SpectrumWifiPhy is to be connected to.
   *
   * \param channel the SpectrumChannel this SpectrumWifiPhy is to be connected to
   */
  void SetChannel (const Ptr<SpectrumChannel> channel);
  /**
   * Return the Channel this SpectrumWifiPhy is connected to.
   *
   * \return the Channel this SpectrumWifiPhy is connected to
   */
  Ptr<Channel> GetChannel (void) const;

  // The following four methods call to the base WifiPhy class method
  // but also generate a new SpectrumModel if called during runtime

  virtual void SetChannelNumber (uint8_t id);

  virtual void SetFrequency (uint16_t freq);

  virtual void SetChannelWidth (uint16_t channelwidth); //TR++ ChannelWidth needs to be changed for 802.11ad/ay

  virtual void ConfigureStandard (WifiPhyStandard standard);


  void StartRx (Ptr<SpectrumSignalParameters> rxParams);
  void StartTx (Ptr<Packet> packet, WifiTxVector txVector, Time txDuration);
  void TxSubfield (WifiTxVector txVector, PLCP_FIELD_TYPE fieldType, Time txDuration);
  void StartAgcSubfieldTx (WifiTxVector txVector);
  void StartCeSubfieldTx (WifiTxVector txVector);
  void StartTrnSubfieldTx (WifiTxVector txVector);

  void CreateWifiSpectrumPhyInterface (Ptr<NetDevice> device);
  uint32_t GetCenterFrequencyForChannelWidth (WifiTxVector txVector) const;
  Ptr<DmgWifiSpectrumPhyInterface> GetSpectrumPhy (void) const;
  Ptr<const SpectrumModel> GetRxSpectrumModel () const;
  double GetBandBandwidth (void) const;
  uint16_t GetGuardBandwidth (uint16_t currentChannelWidth) const;
  typedef void (* SignalArrivalCallback) (bool signalType, uint32_t senderNodeId, double rxPower, Time duration);

protected:
  // Inherited
  void DoDispose (void);
  void DoInitialize (void);

private:
  Ptr<SpectrumValue> GetTxPowerSpectralDensity (uint16_t centerFrequency, uint16_t channelWidth,
                                                double txPowerW, WifiModulationClass modulationClass) const;
  void ResetSpectrumModel (void);

private:
  Ptr<SpectrumChannel> m_channel;                               //!< SpectrumChannel that this SpectrumWifiPhy is connected to.
  std::vector<uint8_t> m_operationalChannelList;                //!< List of possible channels.
  Ptr<DmgWifiSpectrumPhyInterface> m_wifiSpectrumPhyInterface;  //!< Spectrum phy interface.
  mutable Ptr<const SpectrumModel> m_rxSpectrumModel;           //!< receive spectrum model.
  bool m_disableWifiReception;                                  //!< forces this Phy to fail to sync on any signal.
  TracedCallback<bool, uint32_t, double, Time> m_signalCb;      //!< Signal callback.

};

} //namespace ns3

#endif /* SPECTRUM_DMG_WIFI_PHY_H */
