/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019 IMDEA Networks Insitute
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

#ifndef DMG_WIFI_PHY_H
#define DMG_WIFI_PHY_H

#include "wifi-phy.h"
#include "codebook.h"

namespace ns3 {

class DmgWifiChannel;

typedef uint8_t TimeBlockMeasurement;                                         /* Typedef for Time Block Measurement for SPSH. */
typedef std::list<TimeBlockMeasurement> TimeBlockMeasurementList;             /* Typedef for List of Time Block Measurements. */
typedef TimeBlockMeasurementList::const_iterator TimeBlockMeasurementListCI;

enum PLCP_FIELD_TYPE {
  PLCP_80211AD_STF                = 0,
  PLCP_80211AD_CEF                = 1,
  PLCP_80211AD_HDR                = 2,
  PLCP_80211AD_DATA               = 3,
  PLCP_80211AD_PREAMBLE_HDR_DATA  = 4,
  PLCP_80211AD_AGC_SF             = 5,
  PLCP_80211AD_TRN_CE_SF          = 6,
  PLCP_80211AD_TRN_SF             = 7,
  PLCP_80211AY_EDMG_HEADER_A      = 8,
  PLCP_80211AY_EDMG_STF           = 9,
  PLCP_80211AY_EDMG_CEF           = 10,
  PLCP_80211AY_EDMG_HEADER_B      = 11,
  PLCP_80211AY_DATA               = 12,
  PLCP_80211AY_PREAMBLE_HDR_DATA  = 13,
  PLCP_80211AY_TRN_SF             = 14,
};

/**
 * \brief 802.11ad PHY layer model
 * \ingroup wifi
 */
class DmgWifiPhy : public WifiPhy
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  DmgWifiPhy ();
  virtual ~DmgWifiPhy ();

  /**
   * Set the primary 2.16 GHz channel number.
   * \param primaryCh The pimrary 2.16 GHz channel number.
   */
  void SetPrimaryChannelNumber (uint8_t primaryCh);
  /**
   * Get the primary 2.16 GHz channel number.
   * \return The primary 2.16 GHz channel number.
   */
  uint8_t GetPrimaryChannelNumber (void) const;
  /**
   * \param channelWidth the channel width (in MHz)
   */
  virtual void SetChannelWidth (uint16_t channelWidth);
  /**
   * GetChannelConfiguration
   * \return
   */
  EDMG_CHANNEL_CONFIG GetChannelConfiguration (void) const;
  //// NINA ////
  /**
   * Start receiving EDMG TRN Subfield in MIMO mode.
   * \param txVector
   * \param rxPowerDbm The received power in dBm.
   */
  void StartReceiveEdmgTrnSubfield (WifiTxVector txVector, std::vector<double> rxPowerDbm);
  /**
   * End receiving TRN Subfield in MIMO mode.
   * \param event The event related to the reception of this TRN Field.
   */
  void EndReceiveMimoTrnSubfield (WifiTxVector txVector, Ptr<Event> event, std::vector<double> rxPowerW );
  //// NINA ////
  /**
   * Set the DmgWifiChannel this DmgWifiPhy is to be connected to.
   * \param channel the DmgWifiChannel this DmgWifiPhy is to be connected to
   */
  void SetChannel (const Ptr<DmgWifiChannel> channel);
  /**
   * This method is called at initialization to specify whether the node is an AP or not
   * \param ap True if the node is an AP, false otherwise.
   */
  void SetIsAP (bool ap);
  /**
   * Get the delay until the PHY layer ends the reception of a packet.
   * \return The time until the end of the reception (0 if not currently receiving).
   */
  Time GetDelayUntilEndRx(void);
  /**
   * Get pointer to the current DMG Wifi Channel.
   * \return A pointer to the current DMG Wifi Channel.
   */
  virtual Ptr<Channel> GetChannel (void) const;
  /**
   * Configure the PHY-level parameters for Wi-Fi mmWave standards.
   *
   */
  virtual void DoConfigureStandard (void);
  /**
   * Configure WifiPhy with appropriate channel frequency and
   * supported rates for 802.11ad standard.
   */
  void Configure80211ad (void);
  /**
   * Configure WifiPhy with appropriate channel frequency and
   * supported rates for 802.11ay standard.
   */
  void Configure80211ay (void);

  /**
   * Set Codebook
   * \param codebook The codebook for antennas and sectors patterns.
   */
  void SetCodebook (Ptr<Codebook> codebook);
  /**
   * Get Codebook
   * \return A pointer to the current codebook.
   */
  Ptr<Codebook> GetCodebook (void) const;
  /**
   * Set whether the DMG STA supports OFDM PHY layer.
   * \param value
   */
  void SetSupportOfdmPhy (bool value);
  /**
   * Get whether the DMG STA supports OFDM PHY layer.
   * \return True if the DMG STA supports OFDM PHY layer; otherwise false.
   */
  bool GetSupportOfdmPhy (void) const;
  /**
   * Set whether the DMG STA supports LP-SC PHY layer.
   * \param value
   */
  void SetSupportLpScPhy (bool value);
  /**
   * Get whether the DMG STA supports LP-SC PHY layer.
   * \return True if the DMG STA supports LP-SC PHY layer; otherwise false.
   */
  bool GetSupportLpScPhy (void) const;
  /**
   * Get the maximum supported MCS for transmission by SC PHY.
   * \return
   */
  uint8_t GetMaxScTxMcs (void) const;
  /**
   * Get the maximum supported MCS for reception by SC PHY.
   * \return
   */
  uint8_t GetMaxScRxMcs (void) const;
  /**
   * Get the maximum supported MCS for transmission by OFDM PHY.
   * \return
   */
  uint8_t GetMaxOfdmTxMcs (void) const;
  /**
   * Get the maximum supported MCS for reception by OFDM PHY.
   * \return
   */
  uint8_t GetMaxOfdmRxMcs (void) const;
  /**
   * Get the maximum supported MCS by EDMG SC PHY.
   * \return
   */
  uint8_t GetMaxScMcs (void) const;
  /**
   * Get the maximum supported MCS by EDMG OFDM PHY.
   * \return
   */
  uint8_t GetMaxOfdmMcs (void) const;
  /**
   * Get the maximum supported PHY rate by EDMG PHY.
   * \return
   */
  uint8_t GetMaxPhyRate (void) const;
  /**
   * Check whether SU-MIMO communication supported by the EDMG STA.
   * \return Return True if S-MIMO is supported by the EDMG STA, otherwise false.
   */
  bool IsSuMimoSupported (void) const;
  /**
   * Check whether MU-MIMO communication supported by the EDMG STA.
   * \return Return True if S-MIMO is supported by the EDMG STA, otherwise false.
   */
  bool IsMuMimoSupported (void) const;
  /**
   * True if the EDMG STA is in the middle of the SU-MIMO Beamforming training protocol.
   * \param value if the EDMG STA is performing the SU-MIMO beamforming training protocol.
   */
  void SetSuMimoBeamformingTraining (bool value);
  /**
   * True if the EDMG STA is in the middle of the MU-MIMO Beamforming training protocol.
   * \param value if the EDMG STA is performing the MU-MIMO beamforming training protocol.
   */
  void SetMuMimoBeamformingTraining (bool value);
  /**
   * Check whether EDMG STA is in the middle of the MU-MIMO Beamforming training protocol.
   * \return value if the EDMG STA is performing the MU-MIMO beamforming training protocol.
   */
  bool GetMuMimoBeamformingTraining (void) const;
  /**
   * Set the number of TXSS packets that the peer station is sending in the current MIMO BRP TXSS.
   * \param number Number of TXSS packets send in the MIMO BRP TXSS.
   */
  void SetPeerTxssPackets (uint8_t number);
  /**
   * Set the number of times TXSS packets need to be repeated for Rx training of this station in the current MIMO BRP TXSS.
   * \param number Number of times TXSS packets need to be repeated for Rx training.
   */
  void SetTxssRepeat (uint8_t number);

  /**
   * End current allocation period.
   */
  void EndAllocationPeriod (void);
  /**
   * This method is called when the DMG STA works in RDS mode.
   * \param srcSector The sector to use for communicating with the Source REDS.
   * \param srcAntenna The antenna to use for communicating with the Source REDS.
   * \param dstSector The sector to use for communicating with the Destination REDS.
   * \param dstAntenna The antenna to use for communicating with the Destination REDS.
   */
  virtual void ActivateRdsOpereation (uint8_t srcSector, uint8_t srcAntenna,
                                      uint8_t dstSector, uint8_t dstAntenna);
  /**
   * Disable RDS Opereation.
   */
  void ResumeRdsOperation (void);
  /**
   * Disable RDS Opereation.
   */
  void SuspendRdsOperation (void);
  /**
   * Report Measurement Ready Callback.
   */
  typedef Callback<void, TimeBlockMeasurementList> ReportMeasurementCallback;
  /**
   * Start power measurement for spatial sharing.
   * \param measurementDuration The duration of the measurement block.
   * \param blocks The number of time blocks to do measurement.
   */
  void StartMeasurement (uint16_t measurementDuration, uint8_t blocks);
  /**
   * Register Measurement Results Ready callback.
   * \param callback
   */
  void RegisterMeasurementResultsReady (ReportMeasurementCallback callback);
  /**
   * Typedef for SNR Callback.
   */
  typedef Callback<void, AntennaID, SectorID, uint8_t, uint8_t, uint8_t, double, bool, uint8_t> ReportSnrCallback;
  /**
   * Register Report SNR Callback.
   * \param callback The SNR callback.
   */
  void RegisterReportSnrCallback (ReportSnrCallback callback);
  /**
   * Typedef for SNR callback when training using EDMG TRN-R fields appended to beacons.
   */
  typedef Callback<void, AntennaID, SectorID, AWV_ID, uint8_t, uint8_t, double, Mac48Address> BeaconTrainingCallback;
  /**
   * Beacon training callback - report the receive SNR to DmgStaWIfiMac.
   * \param callback
   */
  void RegisterBeaconTrainingCallback (BeaconTrainingCallback callback);
  //// NINA ////
  /**
   * Typedef for SNR Callback when training for SU-MIMO.
   */
  typedef Callback<void, AntennaList, std::vector<double> > ReportMimoSnrCallback;
  /**
   * Register Report SU-MIMO SNR Callback.
   * \param callback The SNR SU-MIMO callback.
   */
  void RegisterReportMimoSnrCallback (ReportMimoSnrCallback callback);
  //// NINA ////
  /**
   * Typedef for callback when TRN field has been received during MUMO beamforming training.
   */
  typedef Callback<void> EndReceiveMimoTRNCallback;
  /**
   * End receive TRN subfields - report the end of Rx of TRN field during MIMO BF training to DmgWifiMac.
   * \param callback
   */
  void RegisterEndReceiveMimoTRNCallback (EndReceiveMimoTRNCallback callback);
  /**
   * Return the number of TRN Units appended to the last received packet
   * (Used when TRN aubfields are appended to the DMG beacons in the BHI)
   * \return The length of the TRN field of the last received packet.
   */
  uint8_t GetBeaconTrnFieldLength (void) const;
  /**
   * Return the duration of a single TRN subfield that is appended to the last received packet
   * (Used when TRN aubfields are appended to the DMG beacons in the BHI)
   * \return TRN subfield duration.
   */
  Time GetBeaconTrnSubfieldDuration (void) const;
  /**
   * Return the duration of the EDMG TRN Field that is appended to the last received packet
   * \return TRN Field duration.
   */
  Time GetEdmgTrnFieldDuration (void) const;
  /**
   * Calculates the duration of the EDMG TRN subfields. The EDMG TRN subfield duration depends on the TRN Sequence
   * Length, the number of Tx Chains used to transmit the packet, whether aggregation is used, the structure of the
   * TRN field and which type of modulation is used for transmission, as described in section 29.12.3, 802.11ay - Draft 4.
   * \return EDMG TRN Subfield Duration.
   */
  Time CalculateEdmgTrnSubfieldDuration (const WifiTxVector &txVector);
  /**
   * Checks if in the middle of reception of TRN fields the reception will be cancelled due to an end of the
   * allocation period. If it is, adds the additional interference from the TRN fields assuming it will be the same as
   * the interference during the reception of the preamble and payload of the packed. Resets all necessary  variables.
   */
  void EndTRNReception (void);
  /**
   *
   * \return the preamble detection duration, which is the time correlation needs to detect the start of an incoming frame.
   */
  virtual Time GetPreambleDetectionDuration (void);

  /**
   * Get TRN field duration.
   * \param txVector
   * \return TRN field duration.
   */
  Time GetTRN_Field_Duration (WifiTxVector &txVector);
  /**
   * Start receiving the PHY preamble of a PPDU (i.e. the first bit of the preamble has arrived).
   *
   * \param ppdu the arriving PPDU
   * \param rxPowerList a list of receive power in W between each pair of active Tx and Rx antennas (has size 1 for SISO, > 1 for MIMO)
   */
  void StartReceivePreamble (Ptr<WifiPpdu> ppdu, std::vector<double> rxPowerList);

  /**
   * Start receiving the PHY header of a PPDU (i.e. after the end of receiving the preamble).
   *
   * \param event the event holding incoming PPDU's information
   */
  virtual void StartReceiveHeader (Ptr<Event> event);

  /**
   * Continue receiving the PHY header of a PPDU (i.e. after the end of receiving the non-HT header part).
   *
   * \param event the event holding incoming PPDU's information
   */
  virtual void ContinueReceiveHeader (Ptr<Event> event);

  /**
   * Start receiving the PSDU (i.e. the first symbol of the PSDU has arrived).
   *
   * \param event the event holding incoming PPDU's information
   */
  void StartReceivePayload (Ptr<Event> event);

  /**
   * The last symbol of the PPDU has arrived.
   *
   * \param event the corresponding event of the first time the packet arrives (also storing packet and TxVector information)
   */
  void EndReceive (Ptr<Event> event);

  /**
   * \param psdu the PSDU to send
   * \param txVector the TXVECTOR that has TX parameters such as mode, the transmission mode to use to send
   *        this PSDU, and txPowerLevel, a power level to use to send the whole PPDU. The real transmission
   *        power is calculated as txPowerMin + txPowerLevel * (txPowerMax - txPowerMin) / nTxLevels
   */
  virtual void Send (Ptr<const WifiPsdu> psdu, WifiTxVector txVector);

  /**
   * \param ppdu the PPDU to send
   */
  virtual void StartTx (Ptr<WifiPpdu> ppdu);

  /**
   * Return the corresponding WifiMode for the given MCS index.
   * \param index The index of the MCS.
   * \return The corresponding WifiMode for the given MCS index.
   */
  static WifiMode GetDmgMcs (uint8_t index);
  /**
   * Return the corresponding WifiMode for the given MCS index.
   * \para, modulation The modulation class.
   * \param index The index of the MCS.
   * \return The corresponding WifiMode for the given MCS index.
   */
  static WifiMode GetEdmgMcs (WifiModulationClass modulation, uint8_t index);
  /**
   * Return a WifiMode for Control PHY.
   *
   * \return a WifiMode for Control PHY.
   */
  static WifiMode GetDMG_MCS0 (void);
  /**
   * Return a WifiMode for SC PHY with MCS1.
   *
   * \return a WifiMode for SC PHY with MCS1.
   */
  static WifiMode GetDMG_MCS1 (void);
  /**
   * Return a WifiMode for SC PHY with MCS2.
   *
   * \return a WifiMode for SC PHY with MCS2.
   */
  static WifiMode GetDMG_MCS2 (void);
  /**
   * Return a WifiMode for SC PHY with MCS3.
   *
   * \return a WifiMode for SC PHY with MCS3.
   */
  static WifiMode GetDMG_MCS3 (void);
  /**
   * Return a WifiMode for SC PHY with MCS4.
   *
   * \return a WifiMode for SC PHY with MCS4.
   */
  static WifiMode GetDMG_MCS4 (void);
  /**
   * Return a WifiMode for SC PHY with MCS5.
   *
   * \return a WifiMode for SC PHY with MCS5.
   */
  static WifiMode GetDMG_MCS5 (void);
  /**
   * Return a WifiMode for SC PHY with MCS6.
   *
   * \return a WifiMode for SC PHY with MCS6.
   */
  static WifiMode GetDMG_MCS6 (void);
  /**
   * Return a WifiMode for SC PHY with MCS7.
   *
   * \return a WifiMode for SC PHY with MCS7.
   */
  static WifiMode GetDMG_MCS7 (void);
  /**
   * Return a WifiMode for SC PHY with MCS8.
   *
   * \return a WifiMode for SC PHY with MCS8.
   */
  static WifiMode GetDMG_MCS8 (void);
  /**
   * Return a WifiMode for SC PHY with MCS9.
   *
   * \return a WifiMode for SC PHY with MCS9.
   */
  static WifiMode GetDMG_MCS9 (void);
  /**
   * Return a WifiMode for SC PHY with MCS9.1 defined 802.11-2016.
   *
   * \return a WifiMode for SC PHY with MCS9.1 defined 802.11-2016.
   */
  static WifiMode GetDMG_MCS9_1 (void);
  /**
   * Return a WifiMode for SC PHY with MCS10.
   *
   * \return a WifiMode for SC PHY with MCS10.
   */
  static WifiMode GetDMG_MCS10 (void);
  /**
   * Return a WifiMode for SC PHY with MCS11.
   *
   * \return a WifiMode for SC PHY with MCS11.
   */
  static WifiMode GetDMG_MCS11 (void);
  /**
   * Return a WifiMode for SC PHY with MCS12.
   *
   * \return a WifiMode for SC PHY with MCS12.
   */
  static WifiMode GetDMG_MCS12 (void);
  /**
   * Return a WifiMode for SC PHY with MCS12.1 defined 802.11-2016.
   *
   * \return a WifiMode for SC PHY with MCS12.1 defined 802.11-2016.
   */
  static WifiMode GetDMG_MCS12_1 (void);
  /**
   * Return a WifiMode for SC PHY with MCS12.2 defined 802.11-2016.
   *
   * \return a WifiMode for SC PHY with MCS12.2 defined 802.11-2016.
   */
  static WifiMode GetDMG_MCS12_2 (void);
  /**
   * Return a WifiMode for SC PHY with MCS12.3 defined 802.11-2016.
   *
   * \return a WifiMode for SC PHY with MCS12.3 defined 802.11-2016.
   */
  static WifiMode GetDMG_MCS12_3 (void);
  /**
   * Return a WifiMode for SC PHY with MCS12.4 defined 802.11-2016.
   *
   * \return a WifiMode for SC PHY with MCS12.4 defined 802.11-2016.
   */
  static WifiMode GetDMG_MCS12_4 (void);
  /**
   * Return a WifiMode for SC PHY with MCS12.5 defined 802.11-2016.
   *
   * \return a WifiMode for SC PHY with MCS12.5 defined 802.11-2016.
   */
  static WifiMode GetDMG_MCS12_5 (void);
  /**
   * Return a WifiMode for SC PHY with MCS12.6 defined 802.11-2016.
   *
   * \return a WifiMode for SC PHY with MCS12.6 defined 802.11-2016.
   */
  static WifiMode GetDMG_MCS12_6 (void);
  /**
   * Return a WifiMode for OFDM PHY with MCS13.
   *
   * \return a WifiMode for OFDM PHY with MCS13.
   */
  static WifiMode GetDMG_MCS13 (void);
  /**
   * Return a WifiMode for OFDM PHY with MCS14.
   *
   * \return a WifiMode for OFDM PHY with MCS14.
   */
  static WifiMode GetDMG_MCS14 (void);
  /**
   * Return a WifiMode for OFDM PHY with MCS15.
   *
   * \return a WifiMode for OFDM PHY with MCS15.
   */
  static WifiMode GetDMG_MCS15 (void);
  /**
   * Return a WifiMode for OFDM PHY with MCS16.
   *
   * \return a WifiMode for OFDM PHY with MCS16.
   */
  static WifiMode GetDMG_MCS16 (void);
  /**
   * Return a WifiMode for OFDM PHY with MCS17.
   *
   * \return a WifiMode for OFDM PHY with MCS17.
   */
  static WifiMode GetDMG_MCS17 (void);
  /**
   * Return a WifiMode for OFDM PHY with MCS18.
   *
   * \return a WifiMode for OFDM PHY with MCS18.
   */
  static WifiMode GetDMG_MCS18 (void);
  /**
   * Return a WifiMode for OFDM PHY with MCS19.
   *
   * \return a WifiMode for OFDM PHY with MCS19.
   */
  static WifiMode GetDMG_MCS19 (void);
  /**
   * Return a WifiMode for OFDM PHY with MCS20.
   *
   * \return a WifiMode for OFDM PHY with MCS20.
   */
  static WifiMode GetDMG_MCS20 (void);
  /**
   * Return a WifiMode for OFDM PHY with MCS21.
   *
   * \return a WifiMode for OFDM PHY with MCS21.
   */
  static WifiMode GetDMG_MCS21 (void);
  /**
   * Return a WifiMode for OFDM PHY with MCS22.
   *
   * \return a WifiMode for OFDM PHY with MCS22.
   */
  static WifiMode GetDMG_MCS22 (void);
  /**
   * Return a WifiMode for OFDM PHY with MCS23.
   *
   * \return a WifiMode for OFDM PHY with MCS23.
   */
  static WifiMode GetDMG_MCS23 (void);
  /**
   * Return a WifiMode for OFDM PHY with MCS24.
   *
   * \return a WifiMode for OFDM PHY with MCS24.
   */
  static WifiMode GetDMG_MCS24 (void);
  /**
   * Return a WifiMode for Low Power SC PHY with MCS25.
   *
   * \return a WifiMode for Low Power SC PHY with MCS25.
   */
  static WifiMode GetDMG_MCS25 (void);
  /**
   * Return a WifiMode for Low Power SC PHY with MCS26.
   *
   * \return a WifiMode for Low Power SC PHY with MCS26.
   */
  static WifiMode GetDMG_MCS26 (void);
  /**
   * Return a WifiMode for Low Power SC PHY with MCS27.
   *
   * \return a WifiMode for Low Power SC PHY with MCS27.
   */
  static WifiMode GetDMG_MCS27 (void);
  /**
   * Return a WifiMode for Low Power SC PHY with MCS28.
   *
   * \return a WifiMode for Low Power SC PHY with MCS28.
   */
  static WifiMode GetDMG_MCS28 (void);
  /**
   * Return a WifiMode for Low Power SC PHY with MCS29.
   *
   * \return a WifiMode for Low Power SC PHY with MCS29.
   */
  static WifiMode GetDMG_MCS29 (void);
  /**
   * Return a WifiMode for Low Power SC PHY with MCS30.
   *
   * \return a WifiMode for Low Power SC PHY with MCS30.
   */
  static WifiMode GetDMG_MCS30 (void);
  /**
   * Return a WifiMode for Low Power SC PHY with MCS31.
   *
   * \return a WifiMode for Low Power SC PHY with MCS31.
   */
  static WifiMode GetDMG_MCS31 (void);

  /**
   * Return a WifiMode for EDMG Control PHY with MCS0.
   *
   * \return a WifiMode for EDMG Control PHY with MCS0.
   */
  static WifiMode GetEDMG_MCS0 (void);
  /**
   * Return a WifiMode for EDMG SC PHY with MCS1.
   *
   * \return a WifiMode for EDMG SC PHY with MCS1.
   */
  static WifiMode GetEDMG_SC_MCS1 (void);
  /**
   * Return a WifiMode for EDMG SC PHY with MCS2.
   *
   * \return a WifiMode for EDMG SC PHY with MCS2.
   */
  static WifiMode GetEDMG_SC_MCS2 (void);
  /**
   * Return a WifiMode for EDMG SC PHY with MCS3.
   *
   * \return a WifiMode for EDMG SC PHY with MCS3.
   */
  static WifiMode GetEDMG_SC_MCS3 (void);
  /**
   * Return a WifiMode for EDMG SC PHY with MCS4.
   *
   * \return a WifiMode for EDMG SC PHY with MCS4.
   */
  static WifiMode GetEDMG_SC_MCS4 (void);
  /**
   * Return a WifiMode for EDMG SC PHY with MCS5.
   *
   * \return a WifiMode for EDMG SC PHY with MCS5.
   */
  static WifiMode GetEDMG_SC_MCS5 (void);
  /**
   * Return a WifiMode for EDMG SC PHY with MCS6.
   *
   * \return a WifiMode for EDMG SC PHY with MCS6.
   */
  static WifiMode GetEDMG_SC_MCS6 (void);
  /**
   * Return a WifiMode for EDMG SC PHY with MCS7.
   *
   * \return a WifiMode for EDMG SC PHY with MCS7.
   */
  static WifiMode GetEDMG_SC_MCS7 (void);
  /**
   * Return a WifiMode for EDMG SC PHY with MCS8.
   *
   * \return a WifiMode for EDMG SC PHY with MCS8.
   */
  static WifiMode GetEDMG_SC_MCS8 (void);
  /**
   * Return a WifiMode for EDMG SC PHY with MCS9.
   *
   * \return a WifiMode for EDMG SC PHY with MCS9.
   */
  static WifiMode GetEDMG_SC_MCS9 (void);
  /**
   * Return a WifiMode for EDMG SC PHY with MCS10.
   *
   * \return a WifiMode for EDMG SC PHY with MCS10.
   */
  static WifiMode GetEDMG_SC_MCS10 (void);
  /**
   * Return a WifiMode for EDMG SC PHY with MCS11.
   *
   * \return a WifiMode for EDMG SC PHY with MCS11.
   */
  static WifiMode GetEDMG_SC_MCS11 (void);
  /**
   * Return a WifiMode for EDMG SC PHY with MCS12.
   *
   * \return a WifiMode for EDMG SC PHY with MCS12.
   */
  static WifiMode GetEDMG_SC_MCS12 (void);
  /**
   * Return a WifiMode for EDMG SC PHY with MCS13.
   *
   * \return a WifiMode for EDMG SC PHY with MCS13.
   */
  static WifiMode GetEDMG_SC_MCS13 (void);
  /**
   * Return a WifiMode for EDMG SC PHY with MCS14.
   *
   * \return a WifiMode for EDMG SC PHY with MCS14.
   */
  static WifiMode GetEDMG_SC_MCS14 (void);
  /**
   * Return a WifiMode for EDMG SC PHY with MCS15.
   *
   * \return a WifiMode for EDMG SC PHY with MCS15.
   */
  static WifiMode GetEDMG_SC_MCS15 (void);
  /**
   * Return a WifiMode for EDMG SC PHY with MCS16.
   *
   * \return a WifiMode for EDMG SC PHY with MCS16.
   */
  static WifiMode GetEDMG_SC_MCS16 (void);
  /**
   * Return a WifiMode for EDMG SC PHY with MCS17.
   *
   * \return a WifiMode for EDMG SC PHY with MCS17.
   */
  static WifiMode GetEDMG_SC_MCS17 (void);
  /**
   * Return a WifiMode for EDMG SC PHY with MCS18.
   *
   * \return a WifiMode for EDMG SC PHY with MCS18.
   */
  static WifiMode GetEDMG_SC_MCS18 (void);
  /**
   * Return a WifiMode for EDMG SC PHY with MCS19.
   *
   * \return a WifiMode for EDMG SC PHY with MCS19.
   */
  static WifiMode GetEDMG_SC_MCS19 (void);
  /**
   * Return a WifiMode for EDMG SC PHY with MCS20.
   *
   * \return a WifiMode for EDMG SC PHY with MCS20.
   */
  static WifiMode GetEDMG_SC_MCS20 (void);
  /**
   * Return a WifiMode for EDMG SC PHY with MCS21.
   *
   * \return a WifiMode for EDMG SC PHY with MCS21.
   */
  static WifiMode GetEDMG_SC_MCS21 (void);

  /**
   * Return a WifiMode for EDMG OFDM PHY with MCS1.
   *
   * \return a WifiMode for EDMG OFDM PHY with MCS1.
   */
  static WifiMode GetEDMG_OFDM_MCS1 (void);
  /**
   * Return a WifiMode for EDMG OFDM PHY with MCS2.
   *
   * \return a WifiMode for EDMG OFDM PHY with MCS2.
   */
  static WifiMode GetEDMG_OFDM_MCS2 (void);
  /**
   * Return a WifiMode for EDMG OFDM PHY with MCS3.
   *
   * \return a WifiMode for EDMG OFDM PHY with MCS3.
   */
  static WifiMode GetEDMG_OFDM_MCS3 (void);
  /**
   * Return a WifiMode for EDMG OFDM PHY with MCS4.
   *
   * \return a WifiMode for EDMG OFDM PHY with MCS4.
   */
  static WifiMode GetEDMG_OFDM_MCS4 (void);
  /**
   * Return a WifiMode for EDMG OFDM PHY with MCS5.
   *
   * \return a WifiMode for EDMG OFDM PHY with MCS5.
   */
  static WifiMode GetEDMG_OFDM_MCS5 (void);
  /**
   * Return a WifiMode for EDMG OFDM PHY with MCS6.
   *
   * \return a WifiMode for EDMG OFDM PHY with MCS6.
   */
  static WifiMode GetEDMG_OFDM_MCS6 (void);
  /**
   * Return a WifiMode for EDMG OFDM PHY with MCS7.
   *
   * \return a WifiMode for EDMG OFDM PHY with MCS7.
   */
  static WifiMode GetEDMG_OFDM_MCS7 (void);
  /**
   * Return a WifiMode for EDMG OFDM PHY with MCS8.
   *
   * \return a WifiMode for EDMG OFDM PHY with MCS8.
   */
  static WifiMode GetEDMG_OFDM_MCS8 (void);
  /**
   * Return a WifiMode for EDMG OFDM PHY with MCS9.
   *
   * \return a WifiMode for EDMG OFDM PHY with MCS9.
   */
  static WifiMode GetEDMG_OFDM_MCS9 (void);
  /**
   * Return a WifiMode for EDMG OFDM PHY with MCS10.
   *
   * \return a WifiMode for EDMG OFDM PHY with MCS10.
   */
  static WifiMode GetEDMG_OFDM_MCS10 (void);
  /**
   * Return a WifiMode for EDMG OFDM PHY with MCS11.
   *
   * \return a WifiMode for EDMG OFDM PHY with MCS11.
   */
  static WifiMode GetEDMG_OFDM_MCS11 (void);
  /**
   * Return a WifiMode for EDMG OFDM PHY with MCS12.
   *
   * \return a WifiMode for EDMG OFDM PHY with MCS12.
   */
  static WifiMode GetEDMG_OFDM_MCS12 (void);
  /**
   * Return a WifiMode for EDMG OFDM PHY with MCS13.
   *
   * \return a WifiMode for EDMG OFDM PHY with MCS13.
   */
  static WifiMode GetEDMG_OFDM_MCS13 (void);
  /**
   * Return a WifiMode for EDMG OFDM PHY with MCS14.
   *
   * \return a WifiMode for EDMG OFDM PHY with MCS14.
   */
  static WifiMode GetEDMG_OFDM_MCS14 (void);
  /**
   * Return a WifiMode for EDMG OFDM PHY with MCS15.
   *
   * \return a WifiMode for EDMG OFDM PHY with MCS15.
   */
  static WifiMode GetEDMG_OFDM_MCS15 (void);
  /**
   * Return a WifiMode for EDMG OFDM PHY with MCS16.
   *
   * \return a WifiMode for EDMG OFDM PHY with MCS16.
   */
  static WifiMode GetEDMG_OFDM_MCS16 (void);
  /**
   * Return a WifiMode for EDMG OFDM PHY with MCS17.
   *
   * \return a WifiMode for EDMG OFDM PHY with MCS17.
   */
  static WifiMode GetEDMG_OFDM_MCS17 (void);
  /**
   * Return a WifiMode for EDMG OFDM PHY with MCS18.
   *
   * \return a WifiMode for EDMG OFDM PHY with MCS18.
   */
  static WifiMode GetEDMG_OFDM_MCS18 (void);
  /**
   * Return a WifiMode for EDMG OFDM PHY with MCS19.
   *
   * \return a WifiMode for EDMG OFDM PHY with MCS19.
   */
  static WifiMode GetEDMG_OFDM_MCS19 (void);
  /**
   * Return a WifiMode for EDMG OFDM PHY with MCS20.
   *
   * \return a WifiMode for EDMG OFDM PHY with MCS20.
   */
  static WifiMode GetEDMG_OFDM_MCS20 (void);

protected:
  friend class DmgWifiChannel;
  // Inherited
  virtual void DoInitialize (void);
  virtual void DoDispose (void);

  /**
   * Start IEEE 802.11ad AGC Subfields transmission.
   * \param txVector TxVector companioned by this transmission.
   */
  void StartAgcSubfieldsTx (WifiTxVector txVector);
  /**
   * Send IEEE 802.11ad AGC Subfield.
   * \param txVector TxVector companioned by this transmission.
   * \param fieldsRemaining The number of remaining AGC Subfields.
   */
  void SendAgcSubfield (WifiTxVector txVector, uint8_t fieldsRemaining);
  /**
   * Start IEEE 802.11ad AGC Subfield transmission.
   * \param txVector TxVector companioned by this transmission.
   */
  virtual void StartAgcSubfieldTx (WifiTxVector txVector);
  /**
   * Start IEEE 802.11ad TRN Unit transmission.
   * \param txVector TxVector companioned by this transmission.
   */
  void StartTrnUnitTx (WifiTxVector txVector);
  /**
   * Send IEEE 802.11ad Channel Estimation (CE) subfield.
   * \param txVector TxVector companioned by this transmission.
   */
  void SendCeSubfield (WifiTxVector txVector);
  /**
   * Start IEEE 802.11ad TRN-CE Subfield transmission.
   * \param txVector TxVector companioned by this transmission.
   */
  virtual void StartCeSubfieldTx (WifiTxVector txVector);
  /**
   * Start IEEE 802.11ad TRN Subfields of the same TRN-Unit transmission.
   * \param txVector TxVector companioned by this transmission.
   */
  void StartTrnSubfieldsTx (WifiTxVector txVector);
  /**
   * Send TRN Field to the destinated station.
   * \param txVector TxVector companioned by this transmission.
   */
  void SendTrnSubfield (WifiTxVector txVector);
  /**
   * Start TRN-SF transmission.
   * \param txVector TxVector companioned by this transmission.
   */
  virtual void StartTrnSubfieldTx (WifiTxVector txVector);
  /**
   * Start transmission of an EDMG TRN field.
   * \param txVector TxVector companioned by this transmission.
   */
  void StartEdmgTrnFieldTx (WifiTxVector txVector);
  /**
   * Start transmission of T transional TRN-SF.
   * \param txVector TxVector companioned by this transmission.
   */
  void SendTSubfields (WifiTxVector txVector);
  /**
   * Start EDMG TRN unit transmission.
   * \param txVector TxVector companioned by this transmission.
   */
  void StartEdmgTrnUnitTx (WifiTxVector txVector);
  /**
   * Start transmission of P TRN-SF at the start of a EDMG TRN-Unit M.
   * \param txVector TxVector companioned by this transmission.
   */
  void SendPSubfields (WifiTxVector txVector);
  /**
   * Start TRN Subfields of the same EDMG TRN-Unit transmission.
   * \param txVector TxVector companioned by this transmission.
   */
  void SendEdmgTrnSubfield (WifiTxVector txVector);
  /**
   * Start EDMG TRN-SF transmission.
   * \param txVector TxVector companioned by this transmission.
   */
  virtual void StartEdmgTrnSubfieldTx (WifiTxVector txVector);

  /**
   * Start receiving IEEE 802.11ad AGC Subfield.
   * \param txVector
   * \param rxPowerDbm The received power in dBm.
   */
  void StartReceiveAgcSubfield (WifiTxVector txVector, double rxPowerDbm);
  /**
   * Start receiving CE Subfield.
   * \param txVector
   * \param rxPowerDbm The received power in dBm.
   */
  void StartReceiveCeSubfield (WifiTxVector txVector, double rxPowerDbm);
  /**
   * Start receiving TRN Subfield.
   * \param txVector
   * \param rxPowerDbm The received power in dBm.
   */
  void StartReceiveTrnSubfield (WifiTxVector txVector, double rxPowerDbm);
  /**
   * Start receiving EDMG TRN Subfield.
   * \param txVector
   * \param rxPowerDbm The received power in dBm.
   */
  void StartReceiveEdmgTrnSubfield (WifiTxVector txVector, double rxPowerDbm);
  /**
   * End receiving TRN Subfield.
   * \param event The event related to the reception of this TRN Field.
   */
  void EndReceiveTrnSubfield (SectorID sectorId, AntennaID antennaId,
                              WifiTxVector txVector, Ptr<Event> event);
  /**
   * End receiving TRN Subfield appended to a beacon.
   * \param event The event related to the reception of this TRN Field.
   */
  void EndReceiveBeaconTrnSubfield (SectorID sectorId, AntennaID antennaId, AWV_ID awvId,
                              WifiTxVector txVector, Ptr<Event> event);
  /**
   * This method is called once all the TRN Units are received.
   */
  void EndReceiveTrnField (bool isBeacon = 0);
  /**
   * Returns whether the PHY layer is currently receiving a TRN Field.
   * \return True if the node is receiving a TRN field, false otherwise.
   */
  bool IsReceivingTRNField (void) const;
  /**
   * Starting receiving the PPDU after having detected the medium is idle or after a reception switch.
   *
   * \param event the event holding incoming PPDU's information
   * \param rxPowerW the receive power in W
   */
  virtual void StartRx (Ptr<Event> event, double rxPowerW);

protected:
  /* EDMG PHY Layer Information */
  uint8_t m_primaryChannel;                     //!< The primary 2.16 GHz channel.
  EDMG_CHANNEL_CONFIG m_channelConfiguration;   //!< The current channel configuration.

private:
  /**
   * The last bit of the PSDU has arrived but we are still waiting for the TRN Fields to be received.
   *
   * \param packet the packet that the last bit has arrived
   * \param txVector the TXVECTOR of the arriving packet
   * \param preamble the preamble of the arriving packet
   * \param aMpdu the type of the packet (0 is not A-MPDU, 1 is a MPDU that is part of an A-MPDU and 2 is the last MPDU in an A-MPDU)
   *        and the A-MPDU reference number (must be a different value for each A-MPDU but the same for each subframe within one A-MPDU)
   * \param event the corresponding event of the first time the packet arrives
   */
  void EndPsduOnlyReceive (Ptr<Packet> packet, WifiTxVector txVector, WifiPreamble preamble,
                           MpduType mpdutype, Ptr<Event> event);
  /**
   * Special function for AGC-Rx Subfields Reception.
   * \param remainingRxFields The number of remaining AGC RX Subfields
   */
  void PrepareForAGC_RX_Reception (uint8_t remainingAgcRxSubields);
  /**
   * MeasurementUnitEnded
   */
  virtual void MeasurementUnitEnded (void);
  /**
   * EndMeasurement
   */
  virtual void EndMeasurement (void);

private:
  /**
   * Set WiGig channel configuration based on the given primary channel number and channel bandwidth.
   */
  void SetChannelConfiguration (void);

private:
  Ptr<DmgWifiChannel> m_channel;          //!< Poiner to the DmgWifiChannel class that this DmgWifiPhy is connected to.
  Ptr<Codebook> m_codebook;               //!< Pointer to the beamforming codebook.

  /* DMG Relay Variables */
  bool m_rdsActivated;                    //!< Flag to indicate if RDS is activated.
  ReportSnrCallback m_reportSnrCallback;  //!< Callback to support.
  uint8_t m_srcSector;                    //!< The ID of the sector used for communication with the source REDS.
  uint8_t m_srcAntenna;                   //!< The ID of the Antenna used for communication with the source REDS.
  uint8_t m_dstSector;                    //!< The ID of the sector used for communication with the destination REDS.
  uint8_t m_dstAntenna;                   //!< The ID of the Antenna used for communication with the destination REDS.
  uint8_t m_rdsSector;
  uint8_t m_rdsAntenna;

  /* Reception status variables */
  bool m_psduSuccess;                     //!< Flag to indicate if the PSDU has been received successfully.

  /* Channel Measurements Variables */
  uint16_t m_measurementUnit;
  uint8_t m_measurementBlocks;
  ReportMeasurementCallback m_reportMeasurementCallback;
  TimeBlockMeasurementList m_measurementList;
  uint8_t m_lastRcpiValue;              //!< The Received channel power indicator (RCPI) value of the last received packet.

  /* DMG PHY Layer Parameters */
  bool m_supportOFDM;                   //!< Flag to indicate whether we support OFDM PHY layer.
  bool m_supportLpSc;                   //!< Flag to indicate whether we support LP-SC PHY layer.
  uint8_t m_maxScTxMcs;                 //!< The maximum supported MCS for transmission by SC PHY.
  uint8_t m_maxScRxMcs;                 //!< The maximum supported MCS for reception by SC PHY.
  uint8_t m_maxOfdmTxMcs;               //!< The maximum supported MCS for transmission by OFDM PHY.
  uint8_t m_maxOfdmRxMcs;               //!< The maximum supported MCS for reception by OFDM PHY.

  /* EDMG PHY Layer Information */
  uint8_t m_maxScMcs;                   //!< The maximum supported MCS for communication by SC PHY.
  uint8_t m_maxOfdmMcs;                 //!< The maximum supported MCS for communication by OFDM PHY.
  uint16_t m_maxPhyRate;                //!< The maximum supported receive PHY data rate, in units of 100 Mbps, over all channel bandwidths and spatial streams.

  /* IEEE 802.11ay Parameters */
  //static EDMG_SC_TIMING m_edmgScTiming; //!< EDMG SC Timing.
  BeaconTrainingCallback m_beaconTrainingCallback; //!< Callback to support beamforming training using TRN-R fields appended to beacons.
  uint8_t m_beaconTrnFieldLength;       //!< The length of TRN field appended to the DMG beacon that we have received.
  Time m_edmgTrnSubfieldDuration;       //!< Duration of the EDMG TRN Subfields.
  Time m_edmgTrnFieldDuration;          //!< Duration of the EDMG TRN Field.
  Mac48Address m_currentSender;         //!< MAC address of the STA that we are currently receiving from.
  bool m_isAp;                          //!< Flag to specify whether the STA is a PCP/AP or STA.
  /* Save the previous receive config before we start receiving TRN-R fields - so that we can return to it after the packet has been received. */
  bool m_isQuasiOmni;                   //!< Flag to specify whether we were receiving in Quasi-Omni mode before the start of TRN-R reception.
  AntennaID m_oldAntennaID;             //!< Antenna ID of the antenna that we using for reception before starting sweeping with TRN-R fields.
  SectorID m_oldSectorID;               //!< Sector ID of the sector that we using for reception before starting sweeping with TRN-R fields.
  AWV_ID m_oldAwvID;                    //!< AWV ID of the AWV that we using for reception before starting sweeping with TRN-R fields.
  bool m_receivingTRNfield;             //!< Flag to signal to the MAC that we are in the process of receiving TRN fields so it should not change the antenna configuration.
  double m_trnReceivePower;             //!< Temporary variable to store the receive power for the packet which contains TRN fields.

  /* IEEE 802.11ay MIMO Parameters */
  bool m_muMimoSupported;               //!< Flag to specify if the STA supports the DL MU-MIMO protocol.
  bool m_reciprocalMuMimoSupported;     //!< Flag to specify if the STA supports the reciprocal MU-MIMO protocol.
  bool m_suMimoSupported;               //!< Flag to specify if the STA supports the SU-MIMO protocol.
  bool m_isGrantRequired;               //!< Flag to specify if the STA requires the reception of a Grant Frame to set up a MIMO configuration.
  bool m_hybridBfMuMimoSupported;       //!< Flag to specify if the STA supports the Hybrid Beamforming protocol during MU-MIMO transmission.
  bool m_hybridBfSuMimoSupported;       //!< Flag to specify if the STA supports the Hybrid Beamforming protocol during SU-MIMO transmission.
  uint8_t m_largestNgSupported;         //!< The largest subcarrier grouping that the STA supports for the beamforming feedback matrix used during digital feedback.
  bool m_dynamicGroupingSupported;      //!< Flag to specify if the STA supports dynamic subcarrier grouping for the beamforming feedback matrix during digital feedback.
  bool m_suMimoBeamformingTraining;     //!< Flag to indicate whether the station is perfroming the SU-MIMO Beamforming training protocol.
  bool m_muMimoBeamformingTraining;     //!< Flag to indicate whether the station is perfroming the MU-MIMO Beamforming training protocol.
  EndReceiveMimoTRNCallback m_endRxMimoTrnCallback;
  //// NINA ////
  uint8_t m_peerTxssPackets;            //!< The number of TXSS packets send by the peer station in the MIMO BRP TXSS.
  uint8_t m_txssRepeat;                 //!< The number of TXSS packets requested for RX training during the MIMO BRP TXSS.
  ReportMimoSnrCallback m_reportMimoSnrCallback;  //!< Callback for reporting SU-MIMO SNR values.
  bool m_recordSnrValues;                 //!< Whether or not the SNR measurements should be recorded (used to avoid double measurements with the same config due to the size of the TRN field).
  //// NINA ////

};

} //namespace ns3

#endif /* DMG_WIFI_PHY_H */
