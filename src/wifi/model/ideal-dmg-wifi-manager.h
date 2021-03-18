/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *         Hany Assasa <hany.assasa@gmail.com>
 */

#ifndef IDEAL_DMG_WIFI_MANAGER_H
#define IDEAL_DMG_WIFI_MANAGER_H

#include "ns3/traced-value.h"
#include "wifi-remote-station-manager.h"

namespace ns3 {

/**
 * \brief Ideal rate control algorithm for DMG/EDMG stations.
 * \ingroup wifi
 *
 * This class implements an 'ideal' rate control algorithm
 * similar to RBAR in spirit (see <i>A rate-adaptive MAC
 * protocol for multihop wireless networks</i> by G. Holland,
 * N. Vaidya, and P. Bahl.): every station keeps track of the
 * SNR of every packet received and sends back this SNR to the
 * original transmitter by an out-of-band mechanism. Each
 * transmitter keeps track of the last SNR sent back by a receiver
 * and uses it to pick a transmission mode based on a set
 * of SNR thresholds built from a target BER and transmission
 * mode-specific SNR/BER curves.
 */
class IdealDmgWifiManager : public WifiRemoteStationManager
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  IdealDmgWifiManager ();
  virtual ~IdealDmgWifiManager ();

  void SetupPhy (const Ptr<WifiPhy> phy);


private:
  //overridden from base class
  void DoInitialize (void);
  WifiRemoteStation* DoCreateStation (void) const;
  void DoReportRxOk (WifiRemoteStation *station,
                     double rxSnr, WifiMode txMode);
  void DoReportRtsFailed (WifiRemoteStation *station);
  void DoReportDataFailed (WifiRemoteStation *station);
  void DoReportRtsOk (WifiRemoteStation *station,
                      double ctsSnr, WifiMode ctsMode, double rtsSnr);
  void DoReportDataOk (WifiRemoteStation *station, double ackSnr, WifiMode ackMode,
                       double dataSnr, uint16_t dataChannelWidth, uint8_t dataNss);
  void DoReportAmpduTxStatus (WifiRemoteStation *station,
                              uint8_t nSuccessfulMpdus, uint8_t nFailedMpdus,
                              double rxSnr, double dataSnr, uint16_t dataChannelWidth, uint8_t dataNss);
  void DoReportFinalRtsFailed (WifiRemoteStation *station);
  void DoReportFinalDataFailed (WifiRemoteStation *station);
  WifiTxVector DoGetDataTxVector (WifiRemoteStation *station);
  WifiTxVector DoGetRtsTxVector (WifiRemoteStation *station);
  bool IsLowLatency (void) const;

  /**
   * Reset the station, invoked if the maximum amount of retries has failed.
   */
  void Reset (WifiRemoteStation *station) const;

  /**
   * Return the minimum SNR needed to successfully transmit
   * data with this WifiTxVector at the specified BER.
   *
   * \param txVector WifiTxVector (containing valid mode, and width)
   *
   * \return the minimum SNR for the given WifiTxVector in linear scale
   */
  double GetSnrThreshold (WifiTxVector &txVector) const;
  /**
   * Adds a pair of WifiTxVector and the minimum SNR for that given vector
   * to the list.
   *
   * \param txVector the WifiTxVector storing mode, and channel width.
   * \param snr the minimum SNR for the given txVector in linear scale
   */
  void AddSnrThreshold (WifiTxVector txVector, double snr);

  /**
   * A vector of <snr, WifiTxVector> pair holding the minimum SNR for the
   * WifiTxVector
   */
  typedef std::vector<std::pair<double, WifiTxVector> > Thresholds;

  double m_ber;             //!< The maximum Bit Error Rate acceptable at any transmission mode
  Thresholds m_thresholds;  //!< List of WifiTxVector and the minimum SNR pair
  /**
   * Trace callback for rate change with particular remote station.
   * \param Mac48Address The MAC address of the remote station.
   * \param Mcs The new MCS index to use with the remote station for data coomunication.
   */
  TracedCallback<Mac48Address, uint16_t> m_mcsChanged;
};

} //namespace ns3

#endif /* IDEAL_DMG_WIFI_MANAGER_H */
