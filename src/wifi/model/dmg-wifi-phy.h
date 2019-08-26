/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2017 IMDEA Networks
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

/* BRP PHY Parameters */
#define aBRPminSCblocks       18
#define aBRPTRNBlock          4992
#define aSCGILength           64
#define aBRPminOFDMblocks     20
#define aSCBlockSize          512
#define TRNUnit               NanoSeconds (ceil (aBRPTRNBlock * 0.57))
#define OFDMSCMin             (aBRPminSCblocks * aSCBlockSize + aSCGILength) * 0.57
#define OFDMBRPMin            aBRPminOFDMblocks * 242
#define AGC_SF_DURATION       NanoSeconds (182)     /* AGC Subfield Duration. */
#define TRN_CE_DURATION       NanoSeconds (655)     /* TRN Channel Estimation Subfield Duration. */
#define TRN_SUBFIELD_DURATION NanoSeconds (364)     /* TRN Subfield Duration. */
#define TRN_UNIT_SIZE         4                     /* The number of TRN Subfield within TRN Unit. */

typedef uint8_t TimeBlockMeasurement;               /* Typedef for Time Block Measurement for SPSH. */
typedef std::list<TimeBlockMeasurement> TimeBlockMeasurementList; /* Typedef for List of Time Block Measurements. */
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
   * Set the DmgWifiChannel this DmgWifiPhy is to be connected to.
   * \param channel the DmgWifiChannel this DmgWifiPhy is to be connected to
   */
  void SetChannel (const Ptr<DmgWifiChannel> channel);
  /**
   * \param packet the packet to send.
   * \param txVector the TXVECTOR that has tx parameters such as mode, the transmission mode to use to send
   *        this packet, and txPowerLevel, a power level to use to send this packet. The real transmission
   *        power is calculated as txPowerMin + txPowerLevel * (txPowerMax - txPowerMin) / nTxLevels.
   * \param txDuration duration of the transmission.
   */
  void StartTx (Ptr<Packet> packet, WifiTxVector txVector, Time txDuration);

  /**
   * Start AGC Subfields transmission.
   * \param txVector TxVector companioned by this transmission.
   */
  void StartAgcSubfieldsTx (WifiTxVector txVector);
  /**
   * Send AGC Subfield.
   * \param txVector TxVector companioned by this transmission.
   * \param fieldsRemaining The number of remaining AGC Subfields.
   */
  void SendAgcSubfield (WifiTxVector txVector, uint8_t fieldsRemaining);
  /**
   * Start AGC Subfield transmission.
   * \param txVector TxVector companioned by this transmission.
   */
  virtual void StartAgcSubfieldTx (WifiTxVector txVector);
  /**
   * Start TRN Unit transmission.
   * \param txVector TxVector companioned by this transmission.
   */
  void StartTrnUnitTx (WifiTxVector txVector);
  /**
   * Send Channel Estimation (CE) subfield.
   * \param txVector TxVector companioned by this transmission.
   */
  void SendCeSubfield (WifiTxVector txVector);
  /**
   * Start TRN-CE Subfield transmission.
   * \param txVector TxVector companioned by this transmission.
   */
  virtual void StartCeSubfieldTx (WifiTxVector txVector);
  /**
   * Start TRN Subfields of the same TRN-Unit transmission.
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
   * Starting receiving the plcp of a packet (i.e. the first bit of the preamble has arrived).
   *
   * \param packet the arriving packet
   * \param rxPowerW the receive power in W
   * \param rxDuration the duration needed for the reception of the packet
   */
  virtual void StartReceivePreambleAndHeader (Ptr<Packet> packet,
                                              double rxPowerW,
                                              Time rxDuration);
  /**
   * Starting receiving the payload of a packet (i.e. the first bit of the packet has arrived).
   *
   * \param packet the arriving packet
   * \param txVector the TXVECTOR of the arriving packet
   * \param mpdutype the type of the MPDU as defined in WifiPhy::MpduType.
   * \param event the corresponding event of the first time the packet arrives
   */
  void StartReceivePacket (Ptr<Packet> packet,
                           WifiTxVector txVector,
                           MpduType mpdutype,
                           Ptr<Event> event);
  /**
   * Start receiving AGC Subfield.
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
   * End receiving TRN Subfield.
   * \param event The event related to the reception of this TRN Field.
   */
  void EndReceiveTrnSubfield (SectorID sectorId, AntennaID antennaId,
                              WifiTxVector txVector, Ptr<Event> event);
  /**
   * This method is called once all the TRN Units are received.
   */
  void EndReceiveTrnField (void);
  /**
   * Get pointer to the current DMG Wifi Channel.
   * \return A pointer to the current DMG Wifi Channel.
   */
  virtual Ptr<Channel> GetChannel (void) const;
  /**
   * Configure the PHY-level parameters for different Wi-Fi standard.
   *
   * \param standard the Wi-Fi standard
   */
  virtual void DoConfigureStandard (void);

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
  virtual void ResumeRdsOperation (void);
  /**
   * Disable RDS Opereation.
   */
  virtual void SuspendRdsOperation (void);
  /**
   * Report Measurement Ready Callback
   */
  typedef Callback<void, TimeBlockMeasurementList> ReportMeasurementCallback;
  /**
   * Start Measurement.
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
  typedef Callback<void, AntennaID, SectorID, uint8_t, uint8_t, double, bool> ReportSnrCallback;
  /**
   * Register Report SNR Callback.
   * \param callback The SNR callback.
   */
  void RegisterReportSnrCallback (ReportSnrCallback callback);

  /**
   * \param txVector the transmission parameters used for this packet
   *
   * \return the WifiMode used for the transmission of the PLCP header
   */
  virtual WifiMode GetPlcpHeaderMode (WifiTxVector txVector);
  /**
   * \param txVector the transmission parameters used for this packet
   *
   * \return the duration of the PLCP header
   */
  virtual Time GetPlcpHeaderDuration (WifiTxVector txVector);
  /**
   * \param txVector the transmission parameters used for this packet
   *
   * \return the duration of the PLCP preamble
   */
  virtual Time GetPlcpPreambleDuration (WifiTxVector txVector);
  /**
   * \param size the number of bytes in the packet to send
   * \param txVector the TXVECTOR used for the transmission of this packet
   * \param frequency the channel center frequency (MHz)
   * \param mpdutype the type of the MPDU as defined in WifiPhy::MpduType.
   * \param incFlag this flag is used to indicate that the static variables need to be update or not. This function is called a couple of times for the same packet so static variables should not be increased each time
   *
   * \return the duration of the payload
   */
  virtual Time GetPayloadDuration (uint32_t size, WifiTxVector txVector, uint16_t frequency, MpduType mpdutype, uint8_t incFlag);
  /**
   * \param packet the packet to send
   * \param txVector the TXVECTOR that has tx parameters such as mode, the transmission mode to use to send
   *        this packet, and txPowerLevel, a power level to use to send this packet. The real transmission
   *        power is calculated as txPowerMin + txPowerLevel * (txPowerMax - txPowerMin) / nTxLevels
   * \param mpdutype the type of the MPDU as defined in WifiPhy::MpduType.
   */
  virtual void SendPacket (Ptr<const Packet> packet, WifiTxVector txVector, Time frameDuration, MpduType mpdutype = NORMAL_MPDU);
  /**
   * Starting receiving the packet after having detected the medium is idle or after a reception switch.
   *
   * \param packet the arriving packet
   * \param txVector the TXVECTOR of the arriving packet
   * \param mpdutype the type of the MPDU as defined in WifiPhy::MpduType.
   * \param rxPowerW the receive power in W
   * \param rxDuration the duration needed for the reception of the packet
   * \param rxDuration the duration needed for the reception of the packet plus the TRN fields.
   * \param event the corresponding event of the first time the packet arrives
   */
  void StartRx (Ptr<Packet> packet,
                WifiTxVector txVector,
                MpduType mpdutype,
                double rxPowerW,
                Time rxDuration,
                Time totalDuration,
                Ptr<Event> event);
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

protected:
  // Inherited
  virtual void DoDispose (void);

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

  virtual void MeasurementUnitEnded (void);
  virtual void EndMeasurement (void);

private:
  Ptr<DmgWifiChannel> m_channel;        //!< DmgWifiChannel that this DmgWifiPhy is connected to
  Ptr<Codebook> m_codebook;            //!< Pointer to the beamforming code book.
  /* Relay Variables */
  bool m_rdsActivated;                    //!< Flag to indicate if RDS is activated;
  ReportSnrCallback m_reportSnrCallback;  //!< Callback to support
  bool m_psduSuccess;                     //!< Flag if the PSDU has been received successfully.
  uint8_t m_srcSector;                    //!< The ID of the sector used for communication with the source REDS.
  uint8_t m_srcAntenna;                   //!< The ID of the Antenna used for communication with the source REDS.
  uint8_t m_dstSector;                    //!< The ID of the sector used for communication with the destination REDS.
  uint8_t m_dstAntenna;                   //!< The ID of the Antenna used for communication with the destination REDS.
  uint8_t m_rdsSector;
  uint8_t m_rdsAntenna;
  /* Channel Measurements Variables */
  uint16_t m_measurementUnit;
  uint8_t m_measurementBlocks;
  ReportMeasurementCallback m_reportMeasurementCallback;
  TimeBlockMeasurementList m_measurementList;
  /* DMG PHY Layer Parameters */
  bool m_supportOFDM;                   //!< Flag to indicate whether we support OFDM PHY layer.
  bool m_supportLpSc;                   //!< Flag to indicate whether we support LP-SC PHY layer.
  /* Channel measurements */
  uint8_t m_lastRcpiValue;              //!< The Received channel power indicator (RCPI) value of the last received packet.

};

} //namespace ns3

#endif /* DMG_WIFI_PHY_H */
